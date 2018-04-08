/*
    Copyright (C) 2007 by Marten Svanfeldt
                  2013 by Pedro SOUZA

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "cssysdef.h"

#include "csgfx/renderbuffer.h"
#include "csgfx/shadervarcontext.h"
#include "csutil/cfgacc.h"
#include "csutil/dirtyaccessarray.h"
#include "csutil/objreg.h"
#include "csutil/stringquote.h"
#include "csutil/xmltiny.h"
#include "cstool/rbuflock.h"
#include "iengine/rview.h"
#include "imap/loader.h"
#include "imap/services.h"
#include "iutil/document.h"
#include "iutil/vfs.h"
#include "ivaria/reporter.h"
#include "ivaria/view.h"
#include "ivideo/graph3d.h"
#include "ivideo/texture.h"
#include "ivideo/txtmgr.h"

#include "csplugincommon/rendermanager/rendertree.h"
#include "posteffect.h"

#include "csutil/list.h"

#include <stdarg.h>

using namespace CS::RenderManager;

CS_PLUGIN_NAMESPACE_BEGIN (PostEffect)
{
  const char * messageID = "crystalspace.rendermanager.posteffect";

  class PriorityShaderVariableContext : 
    public scfImplementation1<PriorityShaderVariableContext,
    scfFakeInterface<iShaderVariableContext> >,
    public CS::Graphics::PriorityShaderVariableContextImpl
  {
  public:
    PriorityShaderVariableContext(iShaderVariableContext* parent = nullptr, int priority = 0) : 
        scfImplementationType (this), PriorityShaderVariableContextImpl(parent, priority) {}
  };

  CS_LEAKGUARD_IMPLEMENT (PostEffect);

  PostEffect::PostEffect (PostEffectManager* manager, const char* name)
    : scfImplementationType (this), manager (manager), name (name),
    dbgIntermediateTextures (~0)
  {
  }

  PostEffect::~PostEffect ()
  {
  }

  bool PostEffect::SetupView (uint width, uint height)
  {
    if (callback.IsValid ())
      (*callback)(width, height);

    bool dirty = layersDirty;
    Construct (false);
    return dirty;
  }


  iTextureHandle* PostEffect::GetScreenTarget ()
  {
    size_t n;
    if (postLayers.GetSize ())
    {
      const PostEffectLayerOptions * opts = postLayers[postLayers.GetSize () - 1]->GetOptions (n);
      if (n)
        return opts[0].renderTarget;
    }
    return nullptr;
  }
  bool PostEffect::AllocateTextures ()
  {
    DependencySolver ds;

    csArray<DependencySolver::LayerTextureInfo> result;
    if (!ds.Solve (postLayers, result))
    {
      csReport (manager->reg, CS_REPORTER_SEVERITY_ERROR, messageID,
        "%s", ds.GetLastError ());
      return false;
    }

    // clear all previous allocations
    Clear();

    TextureAllocationInfo info;
    int start = 0;
    //try reuse target texture
    if (result[0].info == info && target.IsValid ())
    {
      textures.Push(target); ++start;
    }
    //Alloc textures
    for (uint i = start; i < result.GetSize (); ++i)
    {
      csRef<iTextureHandle> tex = manager->RequestTexture (result[i].info, result[i].num);
      if (!tex.IsValid ())
      {
        csReport (manager->reg, CS_REPORTER_SEVERITY_ERROR, messageID,
          "Failed to allocate texture for %s posteffect", CS::Quote::Single (name));
        return false;
      }
      textures.Push (tex);
    }

    //check if all outpust are valid
    for (uint i = 0; i < postLayers.GetSize (); ++i)
    {
      for (uint j = 0; j < postLayers[i]->outputTextures.GetSize (); ++j)
      {
        if (postLayers[i]->outputTextures[j] < 0 && 
          !postLayers[i]->desc.outputs[j].renderTarget.IsValid ())
        {
          csReport (manager->reg, CS_REPORTER_SEVERITY_ERROR, messageID,
            "Error: output %d of layer %s is invalid!", i, CS::Quote::Single (postLayers[i]->desc.name));
          return false;
        }
      }
    }
    return true;
  }

  bool PostEffect::LinkLayers ()
  {
    DependencySolver ds;

    for (uint i = 0; i < postLayers.GetSize (); ++i)
    {
      //link inputs
      for (uint j = 0; j < postLayers[i]->desc.inputs.GetSize (); ++j)
      {
        switch(postLayers[i]->desc.inputs[j].type)
        {
        case AUTO:
          if (postLayers[i]->desc.inputs[j].sourceName.Compare("*screen") ||
             (postLayers[i]->desc.inputs[j].sourceName.Compare("") && i == 0))
            postLayers[i]->desc.inputs[j].inputTexture = input;
          else if (i > 0)
          {
            int l, o;
            if (!ds.GetOutput (postLayers, i, j, l, o)) return false;
            if (postLayers[l]->outputTextures[o] == -1) return false;
            postLayers[i]->desc.inputs[j].inputTexture = 
              textures[postLayers[l]->outputTextures[o]];
          } else
          {
              csReport (manager->reg, CS_REPORTER_SEVERITY_WARNING, messageID,
                "Input %s of layer %s of posteffect %s not linked!",
                CS::Quote::Single (postLayers[i]->desc.inputs[j].sourceName),
                CS::Quote::Single (postLayers[i]->desc.name),
                CS::Quote::Single (name));
              return false;
          }
          break;
        case STATIC:
          //load texture
          if (!postLayers[i]->desc.inputs[j].inputTexture.IsValid ())
          {
            postLayers[i]->desc.inputs[j].inputTexture =  
              manager->loader->LoadTexture (postLayers[i]->GetLayerDesc ().inputs[j].sourceName);
          }
          break;
        case MANUAL:
          //check if the provided texture is valid
          if (!postLayers[i]->desc.inputs[j].inputTexture.IsValid ()) return false;
          break;
        default:
          return false;
        }
      }
    }
    return true;
  }

  bool PostEffect::CreateRenderInfo()
  {
    indices = csRenderBuffer::CreateIndexRenderBuffer (4,
      CS_BUF_STATIC, CS_BUFCOMP_UNSIGNED_SHORT, 0, 3);
    {
      csRenderBufferLock<unsigned short> indexLock (indices);
      for (uint i = 0; i < 4; i++)
        indexLock[(size_t)i] = i;
    }
    for (uint i = 0; i < postLayers.GetSize (); ++i)
    {
      Layer& layer = *(postLayers[i]);

      LayerRenderInfo& renderInfo = layer.rInfo;
      renderInfo.buffers.AttachNew (new csRenderBufferHolder);

      renderInfo.fullscreenQuad.meshtype = CS_MESHTYPE_QUADS;
      renderInfo.fullscreenQuad.vertexCount = 4;
      renderInfo.fullscreenQuad.mixmode = CS_MIXMODE_BLEND (ONE, ZERO);
      renderInfo.fullscreenQuad.shader = layer.desc.layerShader;
      renderInfo.fullscreenQuad.renderBuffers = renderInfo.buffers;
      renderInfo.buffers->SetRenderBuffer (CS_BUFFER_INDEX, indices);

      renderInfo.vertBuf = csRenderBuffer::CreateRenderBuffer (4, CS_BUF_STATIC, 
        CS_BUFCOMP_FLOAT, 3);
      renderInfo.buffers->SetRenderBuffer (CS_BUFFER_POSITION, renderInfo.vertBuf);

      csRenderBufferLock<csVector3> screenQuadVerts (renderInfo.vertBuf);

      csRect targetRect (layer.desc.outputs[0].targetRect);
      int targetW, targetH;
      int tex_idx = layer.outputTextures[0];
      if (tex_idx == -1) return false;
      textures[tex_idx]->GetRendererDimensions (targetW, targetH);
      if (targetRect.IsEmpty ())
      {
        targetRect.Set (0, 0, targetW, targetH);
      }

      // Setup the vertices & texcoords
      screenQuadVerts[(size_t)0].Set (targetRect.xmin, targetRect.ymin, 0);
      screenQuadVerts[(size_t)1].Set (targetRect.xmax, targetRect.ymin, 0);
      screenQuadVerts[(size_t)2].Set (targetRect.xmax, targetRect.ymax, 0);
      screenQuadVerts[(size_t)3].Set (targetRect.xmin, targetRect.ymax, 0);

      float tcMulX, tcMulY;
      if (textures[tex_idx]->GetTextureType () == iTextureHandle::texTypeRect)
      {
        // Handle a rect texture
        // Rect texture use non-normalized texture coordinates
        tcMulX = 1;
        tcMulY = 1;
      }
      else
      {
        // 2D texture, coordinates are 0-1,0-1
        tcMulX = 1.0f/targetW;
        tcMulY = 1.0f/targetH;
      }

      renderInfo.texcoordBuf = csRenderBuffer::CreateRenderBuffer (4, CS_BUF_STATIC,
        CS_BUFCOMP_FLOAT, 2);

      csRenderBufferLock<csVector2> screenQuadTex (renderInfo.texcoordBuf);

      // Setup the texcoords
      screenQuadTex[(size_t)0].Set (targetRect.xmin*tcMulX, targetRect.ymin*tcMulY);
      screenQuadTex[(size_t)1].Set (targetRect.xmax*tcMulX, targetRect.ymin*tcMulY);
      screenQuadTex[(size_t)2].Set (targetRect.xmax*tcMulX, targetRect.ymax*tcMulY);
      screenQuadTex[(size_t)3].Set (targetRect.xmin*tcMulX, targetRect.ymax*tcMulY);

      renderInfo.buffers->SetRenderBuffer (CS_BUFFER_TEXCOORD0, renderInfo.texcoordBuf);
    }
    return true;
  }

  bool PostEffect::SetupShaderVars ()
  {
    for (uint i =0; i < postLayers.GetSize (); ++i)
    {
      csRef<iShaderVariableContext> svContext;
      PriorityShaderVariableContext* pContext = new PriorityShaderVariableContext ();
      svContext.AttachNew (pContext);


      Layer& layer = *(postLayers[i]);
      layer.rInfo.layerSVs = svContext;

      pContext->AddContext(layer.svDefaultContext, 1);
      pContext->AddContext(layer.svUserContext, 2);
      pContext->AddContext(manager->GetSharedSVs (), 3);

      for (uint j = 0; j < layer.GetLayerDesc ().inputs.GetSize (); ++j)
      {
        PostEffectLayerInputMap& inp = layer.GetLayerDesc ().inputs[j];
        csRef<csShaderVariable> sv;
        if (!inp.inputTexture.IsValid ()) return false;

        CS::ShaderVarStringID strID = manager->GetStringID (inp.svTextureName);
        sv.AttachNew (new csShaderVariable (strID));
        sv->SetValue (inp.inputTexture);

        svContext->AddVariable (sv);

        csRenderBufferName bufferName =
          csRenderBuffer::GetBufferNameFromDescr (inp.svTexcoordName);
        if (bufferName == CS_BUFFER_NONE)
        {
          CS::StringIDValue svName = manager->GetStringID (inp.svTexcoordName);
          sv.AttachNew (new csShaderVariable (svName));
          sv->SetValue (layer.rInfo.texcoordBuf);
          svContext->AddVariable (sv);
        }

        if (!inp.svPixelSizeName.Compare(""))
        {
          CS::StringIDValue svName = manager->GetStringID (inp.svPixelSizeName);
          sv.AttachNew (new csShaderVariable (svName));
          int width, height;
          inp.inputTexture->GetRendererDimensions(width, height);
          float inv_w, inv_h;
          inv_w = 1.0f / (float)width;
          inv_h = 1.0f / (float)height;
          sv->SetValue (csVector2(inv_w, inv_h));
          svContext->AddVariable (sv);
        }

        {
          CS::StringIDValue svName = manager->GetStringID ("inv resolution");
          sv.AttachNew (new csShaderVariable (svName));
          int width, height;
          if (layer.outputTextures[0] != -1)
            textures[layer.outputTextures[0]]->GetRendererDimensions(width, height);
          else
            layer.desc.outputs[0].renderTarget->GetRendererDimensions(width, height);
          float inv_w, inv_h;
          inv_w = 1.0f / (float)width;
          inv_h = 1.0f / (float)height;
          sv->SetValue (csVector2(inv_w, inv_h));
          svContext->AddVariable (sv);

          svName = manager->GetStringID ("resolution");
          sv.AttachNew (new csShaderVariable (svName));
          sv->SetValue (csVector2(width, height));
          svContext->AddVariable (sv);
        }
      }
      layer.rInfo.fullscreenQuad.dynDomain = layer.rInfo.layerSVs;
    }
    return true;
  }

  bool PostEffect::Construct (bool forced)
  {
    bool ret = true;
    for (uint i = 0; i < postLayers.GetSize(); ++i)
    {
      if (!postLayers[i]->IsValid()) return false;
      forced |= postLayers[i]->dirty;
      postLayers[i]->dirty = false;

    }
    if (layersDirty || forced)
    {
      ret = false;
      if (AllocateTextures () && LinkLayers ())
      {
        ret = CreateRenderInfo () && SetupShaderVars ();
      }
    }
    layersDirty = false;
    return ret;
  }

  void PostEffect::DrawPostEffect (RenderTreeBase& renderTree, PostEffectDrawTarget flag)
  { 
    manager->graphics3D->FinishDraw ();

    int n = postLayers.GetSize () - 1;
    for (int i = 0; i <= n; ++i)
    {

      postLayers[i]->doPreprocess();

      for (uint j = 0; j < postLayers[i]->outputTextures.GetSize (); ++j)
      {
        iTextureHandle *tex;
        if (postLayers[i]->outputTextures[j] < 0)
          tex = postLayers[i]->desc.outputs[j].renderTarget;
        else
          tex = textures[postLayers[i]->outputTextures[j]];
        //is last layer
        if (i == n)
        {
          tex = flag == TARGET ? (iTextureHandle*)target : (flag == SCREEN ? nullptr : tex); 
        }

        manager->graphics3D->SetRenderTarget (tex, false, 0,
          (csRenderTargetAttachment)(rtaColor0 + j));
      }
      int drawflags = CSDRAW_CLEARZBUFFER | CSDRAW_3DGRAPHICS;

      manager->graphics3D->BeginDraw (drawflags);
      manager->graphics3D->DrawSimpleMesh (postLayers[i]->rInfo.fullscreenQuad,
        csSimpleMeshScreenspace);
      manager->graphics3D->FinishDraw ();

      postLayers[i]->doPostprocess(textures);
    }
    manager->graphics3D->UnsetRenderTargets();
  }

  iPostEffectLayer* PostEffect::AddLayer (const LayerDesc &desc)
  {
    Layer* newLayer = new Layer (desc);
    postLayers.Push (newLayer);
    layersDirty = true;
    return newLayer;
  }


  bool PostEffect::RemoveLayer (iPostEffectLayer* layer)
  {
    size_t layerIndex = 0;
    for (; layerIndex < postLayers.GetSize (); layerIndex++)
    {
      if (postLayers[layerIndex] == layer) break;
    }
    if (layerIndex >= postLayers.GetSize ()) return false;

    postLayers.DeleteIndex (layerIndex);
    layersDirty = true;

    return true;
  }

  void PostEffect::ClearLayers ()
  {
    Clear();
    postLayers.DeleteAll ();
  }

  iTextureHandle* PostEffect::GetLayerOutput (const iPostEffectLayer* layer)
  {
    Layer * l = dynamic_cast<Layer*> ((iPostEffectLayer*)layer);
    if (l)
    {
      if (l->outputTextures[0] < 0)
        return l->desc.outputs[0].renderTarget;
      else
        return textures[l->outputTextures[0]];
    }
    return nullptr;
  }

  void PostEffect::GetLayerRenderSVs (iPostEffectLayer* layer, csShaderVariableStack& svStack)
  {
    Layer * l = dynamic_cast<Layer*> ((iPostEffectLayer*)layer);
    if (l)
    {
      Construct(false);
      l->rInfo.layerSVs->PushVariables(svStack);
    }
  }

  bool PostEffect::ScreenSpaceYFlipped ()
  {
    return (postLayers.GetSize () > 1);
  }

  bool PostEffect::LoadFromFile (const char * fileName)
  {
    return manager->postEffectParser->AddLayersFromFile(fileName, this);
  }

  void PostEffect::Clear()
  {
    textures.DeleteAll();
    for (uint i = 0; i < postLayers.GetSize (); ++i)
    {
      for (uint j = 0; j < postLayers[i]->desc.inputs.GetSize (); ++j)
      {
        if (postLayers[i]->desc.inputs[j].type == AUTO || postLayers[i]->desc.inputs[j].type == STATIC)
          postLayers[i]->desc.inputs[j].inputTexture = nullptr;
      }
    }
    layersDirty = true;
  }

  iPostEffectLayer * PostEffect::GetLayer (const char * name)
  {
    for (uint i = 0; i < postLayers.GetSize(); ++i)
    {
      if (postLayers[i]->desc.name.Compare(name))
        return postLayers[i];
    }
    return nullptr;
  }

  iPostEffectLayer * PostEffect::GetLayer (int  num)
  {
    if ((size_t)num < postLayers.GetSize())
      return postLayers[num];
    return nullptr;
  }

  void PostEffect::SetSetupViewCallback (iSetupViewCallback * pCallback)
  {
    callback = pCallback;
  }

  //--------Layer--------------------------------------------------------------

  CS_LEAKGUARD_IMPLEMENT (Layer);

  Layer::Layer (const LayerDesc &desc)
    : scfImplementationType (this), dirty(true)
  {
    this->desc = desc;
    svUserContext.AttachNew (new csShaderVariableContext);
    svDefaultContext.AttachNew (new csShaderVariableContext);
  }

  void Layer::AddDefaultVar(csShaderVariable *var)
  {
    svDefaultContext->AddVariable(var);
  }

  bool Layer::IsValid()
  {
    return desc.outputs.GetSize() > 0 && desc.layerShader.IsValid();
  }

  void Layer::doPreprocess()
  {
    if (desc.layerProcessor.IsValid())
      desc.layerProcessor->PreProcess(desc.inputs.GetArray (), desc.inputs.GetSize ());
  }

  void Layer::doPostprocess(csRefArray<iTextureHandle>& textures)
  {
    if (desc.layerProcessor.IsValid())
    {
      for (size_t i = 0; i < outputTextures.GetSize(); ++i)
        desc.layerProcessor->PostProcess(textures[outputTextures[i]], i);
    }
  }


  //-------PostEffectManager---------------------------------------------------

  SCF_IMPLEMENT_FACTORY (PostEffectManager);

  CS_LEAKGUARD_IMPLEMENT (PostEffectManager);

  PostEffectManager::PostEffectManager (iBase* parent)
    : scfImplementationType (this, parent), postEffectParser (nullptr)
  {
  }
  PostEffectManager::~PostEffectManager ()
  {
    delete postEffectParser;
  }

  bool PostEffectManager::Initialize (iObjectRegistry* objectReg)
  {
    reg = objectReg;
    graphics3D = csQueryRegistry<iGraphics3D> (objectReg);
    svStrings = csQueryRegistryTagInterface<iShaderVarStringSet> (objectReg,
      "crystalspace.shader.variablenameset");

    csConfigAccess cfg (objectReg);
    keepAllIntermediates =
      cfg->GetBool ("PostEffectManager.KeepAllIntermediates", false);

    curWidth = curHeight = 0;
    loader = csQueryRegistry<iLoader> (objectReg);
    postEffectParser = new PostEffectLayersParser (objectReg);
    svSharedContext.AttachNew (new csShaderVariableContext ());

    return true;
  }

  csPtr<iPostEffect> PostEffectManager::CreatePostEffect (const char* name)
  {
    csRef<iPostEffect> effect;
    effect.AttachNew (new PostEffect (this, name));

    return csPtr<iPostEffect> (effect);
  }

  csPtr<iTextureHandle> PostEffectManager::RequestTexture (TextureAllocationInfo& info, int num)
  {
    csRef<iTextureHandle> t;
    csString key;
    //format,downsample,axis,mipmap,id
    if (num >= 0)
    {
      key.Format("%s%d%d%d%d%d", info.format.GetData(), info.downsample, info.axis, info.mipmap, info.maxMipmap, num);
      t = renderTargets.Get (key, t);
    }

    if (!t.IsValid ())
    {
      uint texFlags =
        CS_TEXTURE_3D | CS_TEXTURE_NPOTS | CS_TEXTURE_CLAMP | CS_TEXTURE_SCALE_UP;
      if (!info.mipmap)
        texFlags |= CS_TEXTURE_NOMIPMAPS;
      texFlags |= CS_TEXTURE_CREATE_CLEAR;

      int texW = graphics3D->GetWidth ();
      int texH = graphics3D->GetHeight ();

      texW = (info.axis & AXIS_X) ? (texW >> info.downsample) : texW;
      texH = (info.axis & AXIS_Y) ? (texH >> info.downsample) : texH;

      t = graphics3D->GetTextureManager ()->CreateTexture (texW, texH, 
        csimg2D, info.format, texFlags);
      if (info.maxMipmap > 0)
        t->SetMipmapLimits (info.maxMipmap);

      if (num >= 0)
        renderTargets.PutUnique (key, t);
    }

    return csPtr<iTextureHandle> (t);
  }

  bool PostEffectManager::SetupView (uint width, uint height)
  {
    if ((curWidth != width) && (curHeight != height))
    {
      curWidth = width;
      curHeight = height;
      renderTargets.DeleteAll ();
      return true;
    }
    return false;
  }

  iShaderVariableContext* PostEffectManager::GetSharedSVs()
  {
    return svSharedContext;
  }

  CS::StringIDValue PostEffectManager::GetStringID(const char * str) const
  {
    return svStrings->Request(str);
  }

  //--------PostEffect::DependencySolver--------------------------------------------------------------
  bool PostEffect::DependencySolver::Solve (csPDelArray<Layer>& layers, csArray<PostEffect::DependencySolver::LayerTextureInfo>& result)
  {
    lastError = "";
    int n = layers.GetSize ()-1;
    for (uint i = 0; i < layers.GetSize (); ++i)
    {
      layers[i]->outputTextures.DeleteAll ();
      layers[i]->outputTextures.SetSize (layers[i]->desc.outputs.GetSize (), -1);
    }

    if (n >= 0)
    {
      Layer& layer = *(layers[n]);

      LayerTextureInfo info;
      //for each output add it to used textures list
      for (uint i = 0; i < layer.desc.outputs.GetSize (); ++i)
      {
        info = GetAvailableRT (layer.desc.outputs[i], n, result);
        //asign id to layer output
        layer.outputTextures[i] = info.idx;
      }
    }
    //for each layer
    for (int i = layers.GetSize ()-1; i > 0; --i)
    {
      UpdateUsedRT(i);
      //for each input
      for (uint j = 0; j < layers[i]->desc.inputs.GetSize (); ++j)
      {
        if (layers[i]->desc.inputs[j].type != AUTO) continue;
        if (layers[i]->desc.inputs[j].sourceName.Compare ("*screen")) continue;
        //check if format is valid
        if (!IsValid (layers[i]->desc.inputs[j])) continue;

        int l, o;
        //maps an input to a layer output
        if (!GetOutput (layers, i, j, l, o))
        {
          lastError.Format ("Could not find layer %s !", CS::Quote::Single (layers[i]->desc.inputs[j].sourceName));
          return false;
        }
        // layer i uses layer i + n, n>=0 as input
        if (i <= l)
        {
          lastError.Format ("Error: layer %s(%d) uses layer %s(%d) as input!", CS::Quote::Single (layers[i]->desc.name),
            i, CS::Quote::Single (layers[l]->desc.name), l);
          return false;
        }

        if (layers[l]->outputTextures[o] == -1)
        {
          LayerTextureInfo info = GetAvailableRT (layers[l]->desc.outputs[o], l, result);
          layers[l]->outputTextures[o] = info.idx;
        }
      }
    }
    return true;
  }

  PostEffect::DependencySolver::LayerTextureInfo PostEffect::DependencySolver::GetAvailableRT (PostEffectLayerOptions &opt, int layer, csArray<LayerTextureInfo>& result)
  {
    LayerTextureInfo ret;
    std::list<LayerTextureInfo>::iterator itor;

    if (!opt.info.reusable)
    {
      ret.info = opt.info;
      ret.num = -1;
      ret.layer = layer;
      ret.idx = result.GetSize ();
      result.Push (ret);
      return ret;
    }

    itor = avaliable.begin ();
    //try find it on avaliable list
    while (itor != avaliable.end ())
    {
      if ((*itor).info == opt.info)
      {
        ret = *itor;
        avaliable.erase (itor);
        used.push_back (ret);
        return ret;
      }
      itor++;
    }

    itor = used.begin ();
    int num = 0;
    while (itor != used.end ())
    {
      if ((*itor).info == opt.info)
      {
        ++num;
      }
      itor++;
    }
    ret.info = opt.info;
    ret.num = num;
    ret.layer = layer;
    ret.idx = result.GetSize ();
    result.Push (ret);
    used.push_back (ret);
    return ret;
  }

  bool PostEffect::DependencySolver::IsValid (PostEffectLayerInputMap& inp)
  {
    return true;
  }

  bool PostEffect::DependencySolver::GetOutput (csPDelArray<Layer>& layers, int inLayer, int inInput, int& outLayerNum, int& outNum)
  {
    csString name = layers[inLayer]->GetLayerDesc ().inputs[inInput].sourceName;
    if (name.Compare (""))
    {
      outLayerNum = --inLayer;
      outNum = 0;
      return true;
    }
    size_t pos = name.Find (".");
    csString layerName, outName;
    name.SubString (layerName, 0, pos);
    if (pos != (size_t)-1)
      name.SubString (outName, pos+1);
    for (uint i = 0; i < layers.GetSize (); ++i)
    {
      if (layerName.Compare (layers[i]->GetName()))
      {
        outLayerNum = i;
        if (outName.Compare (""))
        {
          outNum = 0;
          return true;
        }
        for (uint j = 0; j < layers[i]->GetLayerDesc ().outputs.GetSize (); ++j)
        {
          if (layers[i]->GetLayerDesc ().outputs[j].name.Compare (outName))
          {
            outNum = j;
            return true;
          }
        }
      }
    }
    return false;
  }

  void PostEffect::DependencySolver::UpdateUsedRT (int l)
  {
    std::list<LayerTextureInfo>::iterator itor;
    itor = used.begin ();
    while (itor != used.end ())
    {
      if ((*itor).layer > l)
      {
        avaliable.push_back (*itor);
        itor = used.erase (itor);
      }
      else
        itor++;
    }
  }

  const char * PostEffect::DependencySolver::GetLastError()
  {
    return lastError;
  }

}
CS_PLUGIN_NAMESPACE_END (PostEffect)
