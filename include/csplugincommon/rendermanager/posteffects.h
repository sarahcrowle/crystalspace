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

#ifndef __CS_CSPLUGINCOMMON_RENDERMANAGER_POSTEFFECTS_H__
#define __CS_CSPLUGINCOMMON_RENDERMANAGER_POSTEFFECTS_H__

/**\file
 * Post processing effects manager
 */

#include "csgfx/shadervarcontext.h"
#include "csplugincommon/rendermanager/rendertree.h"
#include "csutil/scf.h"
#include "csutil/scf_implementation.h"
#include "csutil/array.h"
#include "csutil/dirtyaccessarray.h"
#include "csutil/genericresourcecache.h"
#include "csutil/parray.h"
#include "csutil/ref.h"
#include "imap/services.h"
#include "ivideo/shader/shader.h"

struct iGraphics3D;
struct iObjectRegistry;
struct iRenderView;
struct iShader;
struct iSyntaxService;
struct iTextureHandle;
struct iView;
struct iRenderManagerPostEffects;

namespace CS
{
  namespace RenderManager
  {
    struct iPostEffectLayer;

    /*
     * Downsample axis specifies in wich axis the 
     * output texture will be downsampled.
     */
    enum DownsampleAxis
    {
      /// Downsample in x axis
      AXIS_X = 1,
      /// Downsample in y axis
      AXIS_Y = 2,
      /// Downsample in both axis
      AXIS_XY = (AXIS_X | AXIS_Y)
    };

    struct TextureAllocationInfo
    {
      /// Generate mipmaps for this layer
      bool mipmap;

      /// Highest mipmap level to generate
      int maxMipmap;

      /**
       * Reduce output size. Each downsample step reduces the output by half 
       * in the specified axis.
       */
      int downsample;

      /// The axis to downsample
      DownsampleAxis axis;

      /// Prevent texture reuse. Useful for readback or feedback effects.
      bool reusable;

      /// The render target texture format
      csString format;

      TextureAllocationInfo() : mipmap (false), maxMipmap (-1), downsample (0), axis(AXIS_XY), reusable (true), format ("argb8") {}

      bool operator==(const TextureAllocationInfo& other) const
      { 
        return (mipmap == other.mipmap)
          && (maxMipmap == other.maxMipmap)
          && (downsample == other.downsample)
          && (axis == other.axis)
          && (reusable == other.reusable)
          && (format == other.format);
      }
    };

    /**
     * Options for a post processing layer
     */
    struct PostEffectLayerOptions
    {
      TextureAllocationInfo info;

      ///Manual render target
      csRef<iTextureHandle> renderTarget;

      /// If not empty render to this rectangle of the target texture
      csRect targetRect;

      /// RenderTarget name
      csString name;

      bool operator==(const PostEffectLayerOptions& other) const
      { 
        return (info == other.info)
          && (renderTarget == other.renderTarget)
          && (targetRect == other.targetRect)
          && (name == other.name);
      }
    };

    /// Defines the type of input
    enum LayerInputType
    {
      /// uses layers output
      AUTO,
      /// texture loaded from a file
      STATIC,
      /// custom input
      MANUAL
    };

    /// Custom input mapping for a post processing layer
    struct PostEffectLayerInputMap
    {
      /// the input type
      LayerInputType type;

      csRef<iTextureHandle> inputTexture;

      /// name used to setup or load the input texture
      csString sourceName;

      /// Name of the shader variable to provide the input layer texture in
      csString svTextureName;

      /**
       * Name of the shader variable to provide the texture coordinates for the
       * input layer texture in
       */
      csString svTexcoordName;

      /**
       * If not empty the SV with that name receives the 'pixel size'
       * (values to add to X/Y to get next input pixel) for this input.
       */
      csString svPixelSizeName;

      /**
       * If not empty specifies the rectangle of the input texture, in pixels,
       * the be used as input for the layer.
       */
      csRect sourceRect;

      PostEffectLayerInputMap () : type (AUTO), svTextureName ("tex diffuse"),
        svTexcoordName ("texture coordinate 0") {}
    };

    /*
     * Custom layer processor interface
     *
     * This interface is intended to perform custom
     * pre/post processing and also update shaders
     * variables if needed.
     */
    class iCustomProcessor : public virtual iBase
    {
    public:
      SCF_INTERFACE (iCustomProcessor, 1, 0, 0);

      /*
       * Pre process function.
       * Everything that needs to be done before drawing goes here.
       */
      virtual bool PreProcess (PostEffectLayerInputMap* inputs, size_t count) = 0;

      /*
       * Post process function.
       * Everything that needs to be done after drawing goes here.
       */
      virtual bool PostProcess (iTextureHandle * output, uint id) = 0;
    };

    /// Layer descriptor
    struct LayerDesc
    {
      csDirtyAccessArray<PostEffectLayerInputMap> inputs;
      csDirtyAccessArray<PostEffectLayerOptions> outputs;
      csRef<iShader> layerShader;
      csRef<iCustomProcessor> layerProcessor;
      csString name;

      LayerDesc () {}

      LayerDesc (iShader* shader, const char * layerName = ""): layerShader(shader), name(layerName)
      {
        inputs.Push(PostEffectLayerInputMap());
        outputs.Push(PostEffectLayerOptions());
      }

      void AddInput (PostEffectLayerInputMap& inp)
      {
        inputs.Push(inp);
      }

      void AddOutput (PostEffectLayerOptions& opt)
      {
        outputs.Push(opt);
      }

    };

    /*
     *  A post effect layer.
     */
    struct iPostEffectLayer : public virtual iBase
    {
      SCF_INTERFACE (iPostEffectLayer, 1, 0, 0);

      /// Get the shader variable context of this layer.
      virtual iShaderVariableContext* GetSVContext () const = 0;

      /// Get the inputs to this layer
      virtual const PostEffectLayerInputMap * GetInputs (size_t& size) const = 0;

      /// Get the layer options
      virtual const PostEffectLayerOptions * GetOptions (size_t& size) const = 0;

      ///Get the layer name
      virtual const char * GetName () const = 0;

      ///Set the layer descriptor
      virtual void SetLayerDesc (LayerDesc &desc) = 0;

      ///Get the layer descriptor
      virtual LayerDesc& GetLayerDesc () = 0;

      ///Adds the given variable to default sv context
      virtual void AddDefaultVar(csShaderVariable *var) = 0;
    };

    /*
     * SetupView callback
     */
    class iSetupViewCallback : public virtual iBase
    {
    public:
      SCF_INTERFACE (iSetupViewCallback, 1, 0, 0);

      /// calback
      virtual bool operator () (int width, int height) = 0;
    };

    /// Describes where the posteffect output will be drawn
    enum PostEffectDrawTarget
    {
      /// uses the specified target texture
      TARGET,
      /// draws to the monitor screen
      SCREEN,
      /// draws to the internal posteffect's texture
      NONE
    };

    /**
     * Interface for post processing effects.
     *
     * Post processing effects provide a simple way to render the screen to a texture
     * and then use a number of full screen passes with settable shader to get the desired
     * effect.
     *
     * To use post processing effects, you need to create a iPostEffect,
     * setup it, usually from an external xml file and then add it to the post effect manager.
     * Can be added as many effects as you want, the post effect manager will take care of
     * chaining correctly the effects.
     * Example:
     * \code
     * postMgr = scfQueryInterface<iRenderManagerPostEffects> (rm);
     * csPtr<iPostEffect> effect = postMgr->CreatePostEffect ("example effect");
     * effect->LoadFromFile ("/data/myeffect.xml");
     * postMgr->AddPostEffect (pEffect1);
     * \endcode
     *
     * Post processing setups are a graph of effects (with nodes called "layers"
     * for historic reasons). Each node has one or more outputs and multiple inputs.
     * Inputs can be the output of another node, statics textures or a custom 
     * texture input.
     */
    struct iPostEffect : public virtual iBase
    {
      SCF_INTERFACE (iPostEffect, 1, 0, 0);

      /// Get the name of this post pocessing effect
      virtual const char* GetName () = 0;

      /**
       * Set up the post processing management for a view.
       * \returns Whether the manager has changed. If \c true some values,
       *   such as the screen texture, must be reobtained from the manager.
       */
      virtual bool SetupView (uint width, uint height) = 0;


      /// Get the texture to render a scene to for post processing.
      virtual iTextureHandle* GetScreenTarget () = 0;

      /**
       * Draw post processing effects to the specified
       * target.
       */
      virtual void DrawPostEffect (RenderTreeBase& renderTree, PostEffectDrawTarget flag = TARGET) = 0;

      /// Add an effect pass.
      virtual iPostEffectLayer* AddLayer (const LayerDesc &desc) = 0;

      /// Remove the given layer
      virtual bool RemoveLayer (iPostEffectLayer* layer) = 0;

      /// Get the layer by name
      virtual iPostEffectLayer* GetLayer (const char * name) = 0;

      /// Get the layer by index
      virtual iPostEffectLayer* GetLayer (int num) = 0;

      /// Remove all layers
      virtual void ClearLayers () = 0;

      /// Get the layer representing the "screen" a scene is rendered to.
      virtual iPostEffectLayer* GetScreenLayer () = 0;

      /// Get the layer shader variable stack
      virtual void GetLayerRenderSVs (iPostEffectLayer* layer, csShaderVariableStack& svStack) = 0;

      /// Get the output texture of a layer.
      virtual iTextureHandle* GetLayerOutput (const iPostEffectLayer* layer) = 0;

      /// Set the effect's output render target.
      virtual void SetOutputTarget (iTextureHandle* tex) = 0;

      /// Get the effect's output render target.
      virtual iTextureHandle* GetOutputTarget () const = 0; 

      /// Set the effect's input texture.
      virtual void SetInputTexture (iTextureHandle* tex) = 0;

      /// Get the effect's input texture.
      virtual iTextureHandle* GetInputTexture () const  = 0;

      /**
       * Returns whether the screen space is flipped in Y direction. This usually
       * happens when rendering to a texture due post effects.
       */
      virtual bool ScreenSpaceYFlipped () = 0;

      /// Loads the effect from the given file
      virtual bool LoadFromFile (const char * fileName) = 0;

      /// Setup the effect
      virtual bool Construct(bool forced) = 0;

      /// Clean up all allocated textures (references)
      virtual void Clear() = 0;

      /// callback for setupview function
      virtual void SetSetupViewCallback(iSetupViewCallback * pCallback) = 0;
    };

    /**
     * Factory for post-effects.
     */
    // TODO: iPostEffectFactory/System/Component
    struct iPostEffectManager : public virtual iBase
    {
      SCF_INTERFACE (iPostEffectManager, 1, 0, 0);

      /// Creates a post-effect
      virtual csPtr<iPostEffect> CreatePostEffect (const char* name) = 0;

      /// Request a texture with the given info
      virtual csPtr<iTextureHandle> RequestTexture(TextureAllocationInfo& info, int num) = 0;

      /// Setup the view
      virtual bool SetupView (uint width, uint height) = 0;

      /// Get the shader variable context shared between all effects
      virtual iShaderVariableContext* GetSharedSVs () = 0;

      /// Get the string id value
      virtual CS::StringIDValue GetStringID(const char * str) const = 0;
    };

    // @@@ TODO: give a simple example
    /// Helper class to parse post processing effect configurations.
    class CS_CRYSTALSPACE_EXPORT PostEffectLayersParser :
      public CS::Memory::CustomAllocated
    {
      csStringHash xmltokens;
      iObjectRegistry* objReg;
      csRef<iSyntaxService> synldr;

      typedef csHash<csRef<iShader>, csString> ShadersLayers;

      bool ParseLayer (iDocumentNode* layerNode, iPostEffectLayer* layer, ShadersLayers& shaders) const;
      bool GetLayerAttributes (iDocumentNode* layerNode, csString& name, csString& shader, int& downsample, DownsampleAxis& axis, bool& mip, int& maxmip) const;
      bool ParseInput (iDocumentNode* inputNode, PostEffectLayerInputMap& inp) const;
      bool ParseOutput (iDocumentNode* outputNode, PostEffectLayerOptions& opt, bool default_mip, int default_maxmip) const;

    public:
      /// Constructor
      PostEffectLayersParser (iObjectRegistry* objReg);
      ~PostEffectLayersParser ();

      /**
       * Parse the post-effect layers from the given document node, and add them to
       * the given post effect.
       * \param node The document node to parse the layers from.
       * \param effect The post-effect where the layers will be added
       */
      bool AddLayersFromDocument (iDocumentNode* node, iPostEffect* effect) const;

      /**
       * Parse the post-effect layers from the given XML file, and add them to
       * the given post effect.
       * \param filename A VFS path to the XML file containing the definition of the
       * post effect layers. The XML root node of the file must be "posteffect".
       * \param effect The post-effect where the layers will be added
       */
      bool AddLayersFromFile (const char* filename, iPostEffect* effect) const;
    };

  } // namespace RenderManager
} // namespace CS

#endif
