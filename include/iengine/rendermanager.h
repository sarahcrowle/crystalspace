/*
    Copyright (C) 2007 by Marten Svanfeldt

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

#ifndef __CS_IENGINE_RENDERMANAGER_H__
#define __CS_IENGINE_RENDERMANAGER_H__

/**\file
 * Render manager interfaces
 */

#include "csutil/scf_interface.h"
#include "ivaria/view.h"

#include "csplugincommon/rendermanager/posteffects.h"
#include "csplugincommon/rendermanager/rendertree.h"

struct iTextureHandle;
struct iVisibilityCuller;

/**
 * Common render manager interface.
 */
struct iRenderManager : public virtual iBase
{
  SCF_INTERFACE(iRenderManager,2,0,1);

  /// Render the given view into the framebuffer.
  virtual bool RenderView (iView* view) = 0;

  /**
   * Render the given view into the framebuffer (special precache variant).
   * This method is used by the engine for a precache draw. Usually it's
   * behaviour differs from RenderView() and is thus unsuitable for normal
   * drawing.
   */
  virtual bool PrecacheView (iView* view) = 0;
};

/**
 * Interface for automatic view-to-texture rendering. Exposed by render
 * managers which support this functionality.
 */
struct iRenderManagerTargets : public virtual iBase
{
  SCF_INTERFACE(iRenderManagerTargets,1,0,1);

  /// Flags for target registration
  enum TargetFlags
  {
    /// Only render to the target once
    updateOnce = 1,
    /**
     * Assumes the target is used every frame - means it is rendered to
     * every frame.
     * \remark If this flag is set, but the texture is actually not used,
     *   this is a waste of cycles. Consider manual marking with MarkAsUsed()
     *   if the texture is only used some times.
     */
    assumeAlwaysUsed = 2,

    /// Clear the frame buffer before rendering to it.
    clearScreen = 4
  };
  /**
   * Register a texture and view that should be rendered to the texture.
   * The view is rendered automatically when the texture is used.
   * \param target The texture to render to.
   * \param view The view to render.
   * \param subtexture The subtexture. Typically the face of a cube map
   *   texture.
   * \param flags Combination of TargetFlags.
   * \remark If the combination \a target, \a subtexture was mapped to another
   * view before that mapping is removed.
   */
  virtual void RegisterRenderTarget (iTextureHandle* target, 
    iView* view, int subtexture = 0, uint flags = 0) = 0;
  /// Unregister a texture to automatically render to.
  virtual void UnregisterRenderTarget (iTextureHandle* target,
    int subtexture = 0) = 0;
  /**
   * Manually mark a texture as used.
   * Useful when the texture isn't used in the world itself (e.g. for HUD
   * rendering) and thus is not detected as used by the render manager.
   */
  virtual void MarkAsUsed (iTextureHandle* target) = 0;
};

/**
 * Interface to add post-effects layers
 */
struct iRenderManagerPostEffects : public virtual iBase
{
  SCF_INTERFACE(iRenderManagerPostEffects,1,0,0);

  /**
   * Create a new post effect. You must still add it to the chain of effects
   * through AddPostEffect() or InsertPostEffect().
   */
  virtual csPtr<CS::RenderManager::iPostEffect> CreatePostEffect (const char* name) const = 0;

  /// Add an effect at the end of the chain of effects.
  virtual void AddPostEffect (CS::RenderManager::iPostEffect* effect) = 0;
  /// Insert an effect before the effect at position \a index.
  virtual bool InsertPostEffect (CS::RenderManager::iPostEffect* effect, size_t index) = 0;  

  /// Remove the effect at the given position.
  virtual bool RemovePostEffect (size_t index) = 0;

  /// Remove the given effect from the chain of effects.
  virtual bool RemovePostEffect (CS::RenderManager::iPostEffect* effect) = 0;

  /**
   * Find the index of the post effect with the given name, or (size_t) ~0
   * if it is not found.
   */
  virtual size_t FindPostEffect (const char* name) const = 0;

  /// Get the total number of effects.
  virtual size_t GetPostEffectCount () const = 0;
  /// Get the effect at the given position.
  virtual CS::RenderManager::iPostEffect* GetPostEffect (size_t index) = 0;

  /// Get the texture to render a scene to for post processing.
  virtual iTextureHandle* GetScreenTarget () const = 0;

  /// Get the depth texture to render.
  virtual iTextureHandle* GetDepthTarget () const = 0;

  /// Draw all the effects in the chain of effects.
  virtual void DrawPostEffects (CS::RenderManager::RenderTreeBase& renderTree) = 0;

  /// Set the render target used to ultimatively render to.
  virtual void SetEffectsOutputTarget (iTextureHandle* tex) = 0;
  
  /// Get the render target used to ultimatively render to.
  virtual iTextureHandle* GetEffectsOutputTarget () const = 0;
    
  /**
    * Set up post processing manager for a view.
    * \returns Whether the manager has changed. If \c true some values,
    * such as the screen texture, must be reobtained from the manager.
    * \a perspectiveFixup returns a matrix that should be applied
    * after the normal perspective matrix (this is needed as the
    * screen texture may be larger than the desired viewport and thus
    * the projection must be corrected for that).
    */
  virtual bool SetupView (iView* view, CS::Math::Matrix4& perspectiveFixup) = 0;

  /**
    * Returns whether the screen space is flipped in Y direction. This usually
    * happens when rendering to a texture due post effects.
    */
  virtual bool ScreenSpaceYFlipped () const = 0;

  /// Enables/Disables the post effects  
  virtual void SetPostEffectsEnabled(bool status) = 0;

  /// Sets the depth buffer so that the post effect can access the depth
  virtual void SetDepthBuffer (iTextureHandle * depth) = 0;
};

/**
 * Interface for render managers which implement a visibility culler.
 */
struct iRenderManagerVisCull : public virtual iBase
{
  SCF_INTERFACE(iRenderManagerVisCull,1,0,0);

  /// Create a new visibility culler
  virtual csPtr<iVisibilityCuller> GetVisCuller () = 0;
};

#endif // __CS_IENGINE_RENDERMANAGER_H__
