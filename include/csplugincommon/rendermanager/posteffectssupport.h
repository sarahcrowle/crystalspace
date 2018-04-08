/*
    Copyright (C) 2010 by Frank Richter

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

#ifndef __CS_CSPLUGINCOMMON_RENDERMANAGER_POSTEFFECTSSUPPORT_H__
#define __CS_CSPLUGINCOMMON_RENDERMANAGER_POSTEFFECTSSUPPORT_H__

/**\file
 * Drop-in post effects support for render managers
 */

#include "iengine/rendermanager.h"
#include "csplugincommon/rendermanager/posteffects.h"

namespace CS
{
  namespace RenderManager
  {
    /**
     * Add post effects support to a render manager.
     * Usage:
     * - Must be a base class of the render manager implementation.
     * - Render manager implementation should add
     *   <tt>iRenderManagerPostEffects<iDebugHelper></tt> to their SCF implementation
     *   base class.
     * - Access to the post processing effects manager is available via the member
     *   `postEffects'.
     */
    class CS_CRYSTALSPACE_EXPORT PostEffectsSupport : public virtual iRenderManagerPostEffects
    {
    protected:
      csRef<iPostEffectManager> postEffectManager;
      csRefArray<iPostEffect> postEffects;
      csRef<iTextureHandle> pingPong[2];
      csRef<iTextureHandle> depthHook;
      bool autoHook;
      bool changed, enabled;
      CS::StringIDValue matProjID, invMatProjID, focalLenID, invFocalLenID, texDepthID;

    public:
      PostEffectsSupport ();
      virtual ~PostEffectsSupport ();

      /**
       * Initialize post processing effects support.
       * \param objectReg Object registry. 
       * \param configKey Render manager configuration key. 
       */
      void Initialize (iObjectRegistry* objectReg, const char* configKey);

      /**\name iRenderManagerPostEffects implementation
       * @{ */

      csPtr<iPostEffect> CreatePostEffect (const char* name) const;
      void AddPostEffect (iPostEffect* effect);      
      bool InsertPostEffect (iPostEffect* effect, size_t index);

      size_t FindPostEffect (const char* name) const;

      bool RemovePostEffect (size_t index);
      bool RemovePostEffect (iPostEffect* effect);

      size_t GetPostEffectCount () const;
      iPostEffect* GetPostEffect (size_t index);

      iTextureHandle* GetScreenTarget () const;
      iTextureHandle* GetDepthTarget () const;

      void SetEffectsOutputTarget (iTextureHandle* tex);  
      iTextureHandle* GetEffectsOutputTarget () const;

      void DrawPostEffects (RenderTreeBase& renderTree);

      bool SetupView (iView* view, CS::Math::Matrix4& perspectiveFixup);

      bool ScreenSpaceYFlipped () const;

      void SetPostEffectsEnabled(bool status);

      /**
       * Sets the depth buffer so that the post effect can access the depth.
       * If passed a nullptr(or not set) the manager will automatically allocate 
       * and provide one.
       */
      void SetDepthBuffer (iTextureHandle * depth);
      /** @} */

    private:
      bool SetupView (uint width, uint height, CS::Math::Matrix4& perspectiveFixup);

      bool SetupCommonSVs (iView* view);
    };

  } // namespace RenderManager
} // namespace CS

#endif // __CS_CSPLUGINCOMMON_RENDERMANAGER_POSTEFFECTSSUPPORT_H__
