/*
    Copyright (C)2007 by Marten Svanfeldt
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

#include "csplugincommon/rendermanager/posteffects.h"

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

#include <stdarg.h>

using namespace CS::RenderManager;

namespace
{
  static const char messageID[] = "crystalspace.posteffects.parser";

#define CS_TOKEN_ITEM_FILE "libs/csplugincommon/rendermanager/posteffects.tok"
#include "cstool/tokenlist.h"
#undef CS_TOKEN_ITEM_FILE
}

PostEffectLayersParser::PostEffectLayersParser (iObjectRegistry* objReg)
: objReg (objReg)
{
  InitTokenTable (xmltokens);
  synldr = csQueryRegistry<iSyntaxService> (objReg);
}

PostEffectLayersParser::~PostEffectLayersParser()
{
}

bool PostEffectLayersParser::AddLayersFromDocument (iDocumentNode* node, 
                                                    iPostEffect* effect) const
{
  ShadersLayers shaders;

  csRef<iDocumentNodeIterator> it = node->GetNodes();
  while (it->HasNext())
  {
    csRef<iDocumentNode> child = it->Next();
    if (child->GetType() != CS_NODE_ELEMENT) continue;

    csStringID id = xmltokens.Request (child->GetValue());
    switch (id)
    {
      case XMLTOKEN_LAYER:
      {
        csRef<iPostEffectLayer> layer = effect->AddLayer(LayerDesc());
        if (!ParseLayer (child, layer, shaders))
          return false;
      }
      break;
      default:
        synldr->ReportBadToken (child);
        return false;
    }
  }

  return true;
}

bool PostEffectLayersParser::AddLayersFromFile (const char* filename, 
                                                iPostEffect* effect) const
{
  csRef<iDocumentSystem> docsys = csQueryRegistry<iDocumentSystem> (
    objReg);
  if (!docsys.IsValid())
    docsys.AttachNew (new csTinyDocumentSystem ());

  csRef<iVFS> vfs = csQueryRegistry<iVFS> (objReg);
  CS_ASSERT(vfs);
  csRef<iFile> file = vfs->Open (filename, VFS_FILE_READ);
  if (!file)
  {
    csReport (objReg, CS_REPORTER_SEVERITY_WARNING, messageID,
      "Error opening %s", CS::Quote::Single (filename));
    return false;
  }

  csRef<iDocument> doc = docsys->CreateDocument();
  const char* error = doc->Parse (file);
  if (error != 0)
  {
    csReport (objReg, CS_REPORTER_SEVERITY_WARNING, messageID,
      "Error parsing %s: %s", CS::Quote::Single (filename), error);
    return false;
  }

  csRef<iDocumentNode> docRoot = doc->GetRoot();
  if (!docRoot) return false;
  csRef<iDocumentNode> postEffectNode = docRoot->GetNode ("posteffect");
  if (!postEffectNode)
  {
    csReport (objReg, CS_REPORTER_SEVERITY_WARNING, messageID,
      "No <posteffect> in %s", CS::Quote::Single (filename));
    return false;
  }

  return AddLayersFromDocument (postEffectNode, effect);
}



bool PostEffectLayersParser::ParseLayer (iDocumentNode* layerNode,
                                         iPostEffectLayer* layer, ShadersLayers& shaders) const
{
  LayerDesc desc;
  csString shader;
  bool defaultmip;
  int defaultmaxmip;
  int downsample;
  DownsampleAxis axis;

  if (!GetLayerAttributes (layerNode, desc.name, shader, downsample, axis, defaultmip, defaultmaxmip))
    return false;

  desc.layerShader = shaders.Get (shader, 0);
  if (!desc.layerShader.IsValid())
  {
    csRef<iLoader> loader (csQueryRegistry<iLoader> (objReg));
    desc.layerShader = loader->LoadShader (shader);
    if (!desc.layerShader.IsValid()) return false;
    shaders.Put (shader, desc.layerShader);
  }

  int numInput = 0;
  int numOutput = 0;

  csRef<iDocumentNodeIterator> it = layerNode->GetNodes();
  while (it->HasNext())
  {
    csRef<iDocumentNode> child = it->Next();
    if (child->GetType() != CS_NODE_ELEMENT) continue;

    csStringID id = xmltokens.Request (child->GetValue());
    switch (id)
    {
      case XMLTOKEN_INPUT:
      {
        PostEffectLayerInputMap inp;
        if (!ParseInput (child, inp))
          return false;
        desc.inputs.Push (inp);
        ++numInput;
      }
      break;
      case XMLTOKEN_OUTPUT:
      {
        PostEffectLayerOptions opt;
        if (!ParseOutput (child, opt, defaultmip, defaultmaxmip))
          return false;
        opt.info.downsample = downsample;
        opt.info.axis = axis;
        desc.outputs.Push (opt);
        ++numOutput;
      }
      break;
      case XMLTOKEN_PARAMETER:
      {
        csRef<csShaderVariable> sv;
        sv.AttachNew (new csShaderVariable);
        if (!synldr->ParseShaderVar (0, child, *sv))
          return false;
        layer->AddDefaultVar (sv);
      }
      break;
      default:
        synldr->ReportBadToken (child);
        return false;
    }
  }
  // push default input / output
  if (numInput == 0) desc.inputs.Push (PostEffectLayerInputMap());
  if (numOutput == 0) desc.outputs.Push (PostEffectLayerOptions());

  layer->SetLayerDesc (desc);
  return true;
}

bool PostEffectLayersParser::GetLayerAttributes (iDocumentNode* layerNode,
                                                 csString& name, csString& shader, int& downsample,
                                                 DownsampleAxis& axis,bool& mip, int& maxmip) const
{
  name = layerNode->GetAttributeValue ("name", "");
  shader = layerNode->GetAttributeValue ("shader", "");
  downsample = layerNode->GetAttributeValueAsInt ("downsample", 0);
  const char * caxis = layerNode->GetAttributeValue ("axis", "XY");
  csString saxis = caxis;
  if (saxis.CompareNoCase("X")) axis = AXIS_X;
  else if (saxis.CompareNoCase("Y")) axis = AXIS_Y;
  else if (saxis.CompareNoCase("XY")) axis = AXIS_XY;
  else
  {
    axis = AXIS_XY;
    csReport (objReg, CS_REPORTER_SEVERITY_WARNING, messageID,
      "Invalid downsample axis, using %s instead.", CS::Quote::Single ("XY"));
  }
  mip = layerNode->GetAttributeValueAsBool ("mipmap", false);
  maxmip = layerNode->GetAttributeValueAsInt ("maxmipmap", -1);
  if (name.Find(".") != (size_t)-1)
  {
    csReport (objReg, CS_REPORTER_SEVERITY_ERROR, messageID,
      "Invalid layer name: %s, character \'.\' is not allowed!", CS::Quote::Single (name));
    return false;
  }
  return true;
}
bool PostEffectLayersParser::ParseInput (iDocumentNode* inputNode, PostEffectLayerInputMap& inp) const
{
  inp.type = STATIC;
  const char *value = inputNode->GetAttributeValue ("source");
  if (value == nullptr)
  {
    value = inputNode->GetAttributeValue ("layer", "");
    inp.type = AUTO;
  }
  if (inputNode->GetAttributeValue ("source") && inputNode->GetAttributeValue ("layer"))
  {
    csReport (objReg, CS_REPORTER_SEVERITY_WARNING, messageID,
      "Mutual exclusive <source> and <layer> tag found in the same input! Using value from <source>.");
  }
  inp.sourceName = value;
  inp.svTextureName = inputNode->GetAttributeValue ("texname", "tex diffuse");
  inp.svTexcoordName = inputNode->GetAttributeValue ("texcoord", "texture coordinate 0");
  inp.svPixelSizeName =  inputNode->GetAttributeValue ("pixelsize", "");
  return true;
}

bool PostEffectLayersParser::ParseOutput (iDocumentNode* outputNode, PostEffectLayerOptions& opt,
                                          bool default_mip, int default_maxmip) const
{
  opt.name = outputNode->GetAttributeValue ("name", "");
  opt.info.format = outputNode->GetAttributeValue ("format", "argb8");
  opt.info.reusable = outputNode->GetAttributeValueAsBool ("reusable", true);
  opt.info.mipmap = outputNode->GetAttributeValueAsBool ("mipmap", default_mip);
  opt.info.maxMipmap = outputNode->GetAttributeValueAsInt ("maxmipmap", default_maxmip);
  return true;
}
