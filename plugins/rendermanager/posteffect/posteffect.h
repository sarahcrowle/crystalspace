/*
    Copyright (C) 2007-2008 by Marten Svanfeldt
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

#ifndef __POSTEFFECT_H__
#define __POSTEFFECT_H__

/**\file
 * Post processing effects
 */

#include "csgfx/shadervarcontext.h"
#include "csplugincommon/rendermanager/rendertree.h"
#include "csplugincommon/rendermanager/posteffects.h"
#include "csutil/array.h"
#include "csutil/dirtyaccessarray.h"
#include "csutil/genericresourcecache.h"
#include "csutil/parray.h"
#include "csutil/ref.h"
#include "imap/services.h"
#include "ivideo/shader/shader.h"
#include "iutil/comp.h"
#include "csutil/scf_implementation.h"
#include "iengine/rendermanager.h"

#include <list>

struct iGraphics3D;
struct iObjectRegistry;
struct iRenderView;
struct iShader;
struct iSyntaxService;
struct iTextureHandle;
struct iView;

using namespace CS::RenderManager;

CS_PLUGIN_NAMESPACE_BEGIN (PostEffect)
{
  struct LayerRenderInfo
  {
    /// Input vertices for layer
    csRef<iRenderBuffer> vertBuf;
    /// Input texture coordinates
    csRef<iRenderBuffer> texcoordBuf;
    /// Shader vars
    csRef<iShaderVariableContext> layerSVs;
    /// Render buffers
    csRef<csRenderBufferHolder> buffers;
    /// Render mesh for layer
    csSimpleRenderMesh fullscreenQuad;
  };

  /// An effect layer.
  class Layer : 
    public scfImplementation1<Layer, iPostEffectLayer>
  {
  private:
    friend class PostEffect;

    LayerDesc desc;
    LayerRenderInfo rInfo;
    csRef<iShaderVariableContext> svUserContext;
    csRef<iShaderVariableContext> svDefaultContext;
    csArray<int> outputTextures;
    bool dirty;

    bool IsValid();

  public:
    CS_LEAKGUARD_DECLARE (Layer);

    Layer (const LayerDesc& desc);
    virtual ~Layer () {}

    //-- iPostEffectLayer
    /// Get the shader variables for this layer.
    iShaderVariableContext* GetSVContext () const { return svUserContext; }
    /// Get inputs to this layer
    const PostEffectLayerInputMap * GetInputs (size_t& size) const { size = desc.inputs.GetSize();
      return desc.inputs.GetArray (); }

    /// Get layer options
    const PostEffectLayerOptions * GetOptions (size_t& size) const { size = desc.outputs.GetSize();
      return desc.outputs.GetArray (); }

    /// Get the layer name
    const char * GetName () const {return desc.name;}

    /// Set this layer descriptor
    void SetLayerDesc (LayerDesc &desc) {this->desc = desc; dirty = true;}

    /// Get the layer descriptor
    LayerDesc& GetLayerDesc () {return desc;}

    /// Adds the given variable to the default shader variable context
    void AddDefaultVar(csShaderVariable *var);

    /// Apply the pre-process if there is one
    void doPreprocess();

    /// Apply the post-process if there is one
    void doPostprocess(csRefArray<iTextureHandle>& textures);

  };

  class PostEffectManager;

  class PostEffect :
    public scfImplementation1<PostEffect, iPostEffect>
  {

  public:  
    CS_LEAKGUARD_DECLARE (PostEffect);

    PostEffect (PostEffectManager* manager, const char* name);
    virtual ~PostEffect ();

    //-- iPostEffect

    /// Get the effect name
    const char* GetName () { return name; }

    /// Setup the view
    bool SetupView (uint width, uint height);


    /// Get the texture to render a scene to for post processing.
    iTextureHandle* GetScreenTarget ();

    /// Draw post processing effects
    void DrawPostEffect (RenderTreeBase& renderTree, PostEffectDrawTarget flag);


    /// Add an effect pass.
    iPostEffectLayer* AddLayer (const LayerDesc &desc);

    /// Remove a layer
    bool RemoveLayer (iPostEffectLayer* layer);

    /// get layer by name
    iPostEffectLayer* GetLayer (const char * name);

    ///get layer by index
    iPostEffectLayer* GetLayer (int num);

    /// Remove all layers
    void ClearLayers ();

    /// Get the layer representing the "screen" a scene is rendered to.
    iPostEffectLayer* GetScreenLayer () { return postLayers[postLayers.GetSize() -1]; }

    void GetLayerRenderSVs (iPostEffectLayer* layer, csShaderVariableStack& svStack);

    /// Get the output texture of a layer.
    iTextureHandle* GetLayerOutput (const iPostEffectLayer* layer);

    /// Set the effect's output render target.
    void SetOutputTarget (iTextureHandle* tex) { target = tex; layersDirty = true;}

    /// Get the effect's output render target.
    iTextureHandle* GetOutputTarget () const { return target; }

    /// Set the effect's input texture.
    void SetInputTexture (iTextureHandle* tex) { input = tex; layersDirty = true;}

    /// Get the effect's input texture.
    iTextureHandle* GetInputTexture () const { return input; }

    /**
     * Returns whether the screen space is flipped in Y direction. This usually
     * happens when rendering to a texture due post effects.
     */
    bool ScreenSpaceYFlipped ();


    /// Loads post effect defined in the given file
    bool LoadFromFile (const char * fileName);

    /**
     * Setup the effect, allocates and setup rendertargets,
     * mesh data (fullscreen quad) and shaders variables.
     */
    bool Construct (bool forced);

    /// Clean up all allocated textures (references)
    void Clear();

    /// callback for setupview function
    void SetSetupViewCallback (iSetupViewCallback * pCallback);
  private:

    bool AllocateTextures();
    bool LinkLayers();
    bool CreateRenderInfo();
    bool SetupShaderVars();


    PostEffectManager* manager;
    csString name;
    csRef<iRenderBuffer> indices;
    csRef<iTextureHandle> input, target;
    csRefArray<iTextureHandle> textures;
    uint dbgIntermediateTextures;
    csPDelArray<Layer> postLayers;
    bool layersDirty;
    csRef<iSetupViewCallback> callback;


    class DependencySolver
    {
    public:

      struct LayerTextureInfo
      {
        TextureAllocationInfo info;
        int num;
        int layer;
        int idx;
        LayerTextureInfo () : num(-1), layer(-1), idx(-1) {}
      };

      bool Solve (csPDelArray<Layer>& layers, csArray<LayerTextureInfo>& result);

      bool IsValid (PostEffectLayerInputMap& inp);

      bool GetOutput (csPDelArray<Layer>& layers, int inLayer, int inInput, int& outLayerNum, int& outNum);

      const char * GetLastError ();

    private:
      LayerTextureInfo GetAvailableRT (PostEffectLayerOptions &opt, int layer, csArray<LayerTextureInfo>& result);
      void UpdateUsedRT (int l);
      std::list<LayerTextureInfo> used;
      std::list<LayerTextureInfo> avaliable;
      csString lastError;
    };
  };

  class PostEffectManager : 
    public scfImplementation2<PostEffectManager, iPostEffectManager, iComponent>
  {
    friend class PostEffect;

  protected:
    csHash<csRef<iPostEffect>, const char*> postEffectsHash;
    csHash<csWeakRef<iTextureHandle>, csString> renderTargets;
    csRef<iGraphics3D> graphics3D;
    csRef<iShaderVarStringSet> svStrings;
    csRef<iLoader> loader;
    uint curWidth, curHeight;
    PostEffectLayersParser* postEffectParser;
    csRef<iShaderVariableContext> svSharedContext;
    iObjectRegistry* reg;
    bool keepAllIntermediates;

  public:
    CS_LEAKGUARD_DECLARE (PostEffectManager);

    PostEffectManager (iBase* parent);
    virtual ~PostEffectManager ();

    //-- iComponent
    virtual bool Initialize (iObjectRegistry* objectReg);

    //-- iPostEffectManager
    csPtr<iPostEffect> CreatePostEffect (const char* name);

    /**
     * Allocates a texture with the given TextureAllocationInfo and
     * num, if num >= 0 it tries to reuse a previous allocated
     * texture with same info and num;  
     */ 
    csPtr<iTextureHandle> RequestTexture (TextureAllocationInfo& info, int num);

    /**
     * Set up post processing manager for a view.
     * \returns Whether the manager has changed. If \c true some values,
     *   such as the screen texture, must be reobtained from the manager.   
     */    
    bool SetupView (uint width, uint height);

    /**
     * Returns the sv contexts with 'global'  
     * shared shader variables between the post-effects
     */ 
    iShaderVariableContext* GetSharedSVs ();

    CS::StringIDValue GetStringID (const char * str) const;
  };
}
CS_PLUGIN_NAMESPACE_END (PostEffect)

#endif
