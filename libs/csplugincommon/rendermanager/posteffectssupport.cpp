/*
    Copyright (C) 2010 by Frank Richter
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

#include "csplugincommon/rendermanager/posteffectssupport.h"

#include "csutil/cfgacc.h"
#include "imap/loader.h"
#include "iutil/plugin.h"

namespace CS
{
  namespace RenderManager
  {
    PostEffectsSupport::PostEffectsSupport () 
    {
    }

    PostEffectsSupport::~PostEffectsSupport ()
    {

    }

    void PostEffectsSupport::Initialize (iObjectRegistry* objectReg, const char* configKey)
    {
      csRef<iPluginManager> pluginManager = 
        csQueryRegistry<iPluginManager> (objectReg);

      postEffectManager = csLoadPlugin<iPostEffectManager>
        (pluginManager, "crystalspace.rendermanager.posteffect");

      matProjID = postEffectManager->GetStringID("matrix projection");
      invMatProjID = postEffectManager->GetStringID("matrix inv projection");
      focalLenID = postEffectManager->GetStringID("focal length");
      invFocalLenID = postEffectManager->GetStringID("inv focal length");
      texDepthID = postEffectManager->GetStringID("tex depth");

      autoHook = true;

      // Check for a post-effect to be applied
      if (configKey)
      {
        csString realConfigKey (configKey);
        realConfigKey.Append (".Effects");
        csConfigAccess cfg (objectReg);
        const char* effectsFile = cfg->GetStr (realConfigKey, nullptr);

        if (effectsFile)
        {
          csRef<iPostEffect> effect = CreatePostEffect (effectsFile);
          if (effect->LoadFromFile (effectsFile))
            AddPostEffect (effect);
        }
      }
    }

    csPtr<iPostEffect> PostEffectsSupport::CreatePostEffect (const char* name) const
    {
      if (!postEffectManager)
        return csPtr<iPostEffect> (nullptr);
      return postEffectManager->CreatePostEffect (name);
    }

    void PostEffectsSupport::AddPostEffect (iPostEffect* effect)
    {
      if (!effect) return;
      postEffects.Push (effect);
      changed = true;
    }

    bool PostEffectsSupport::InsertPostEffect (iPostEffect* effect, size_t index)
    {
      if (!effect) return false;

      size_t pfxCount = postEffects.GetSize ();
      if (index > pfxCount) return false;
      bool result = postEffects.Insert (index, effect);
      changed = true;

      return result;
    }

    size_t PostEffectsSupport::FindPostEffect (const char* name) const
    {
      for (size_t i = 0; i < postEffects.GetSize (); i++)
        if (strcmp (postEffects[i]->GetName (), name) == 0)
          return i;

      return (size_t) ~0;
    }

    bool PostEffectsSupport::RemovePostEffect (iPostEffect* effect)
    {
      changed = true;
      return postEffects.Delete (effect);
    }

    bool PostEffectsSupport::RemovePostEffect (size_t index)
    {
      changed = true;
      return postEffects.DeleteIndex (index);
    }

    size_t PostEffectsSupport::GetPostEffectCount () const
    {
      return postEffects.GetSize ();
    }

    iPostEffect* PostEffectsSupport::GetPostEffect (size_t index)
    {
      return postEffects.Get (index);
    }

    iTextureHandle* PostEffectsSupport::GetScreenTarget () const
    {
      if (postEffects.IsEmpty () || !enabled)
        return nullptr;
      return pingPong[1];
    }

    iTextureHandle* PostEffectsSupport::GetDepthTarget () const
    {
      if (postEffects.IsEmpty () || !enabled || !autoHook)
        return nullptr;
      return depthHook;
    }

    void PostEffectsSupport::SetEffectsOutputTarget (iTextureHandle* tex)
    {
      if (postEffects.GetSize () == 0) return;

      postEffects.Get (postEffects.GetSize () - 1)->SetOutputTarget (tex);
    }

    iTextureHandle* PostEffectsSupport::GetEffectsOutputTarget () const
    {
      if (postEffects.GetSize () == 0) 
        return nullptr;

      return postEffects.Get (postEffects.GetSize () - 1)->GetOutputTarget ();
    }

    void PostEffectsSupport::DrawPostEffects (RenderTreeBase& renderTree)
    {
      if (!enabled) return;

      int n = postEffects.GetSize ()-1;
      for (int i = 0; i < n; i++)
        postEffects.Get (i)->DrawPostEffect (renderTree);
      if (n>=0)
        postEffects.Get (n)->DrawPostEffect (renderTree, SCREEN);
      if (1)
      {
        renderTree.AddDebugTexture (pingPong[0]);
        renderTree.AddDebugTexture (pingPong[1]);
      }
    }

    bool PostEffectsSupport::SetupView (iView* view, CS::Math::Matrix4& perspectiveFixup)
    {
      uint width = view->GetContext ()->GetWidth ();
      uint height = view->GetContext ()->GetHeight ();
      bool ret = SetupView (width, height, perspectiveFixup);
      SetupCommonSVs (view);
      return ret;
    }

    bool PostEffectsSupport::SetupView (uint width, uint height, 
      CS::Math::Matrix4& perspectiveFixup)
    {
      size_t pfxCount = postEffects.GetSize ();
      if (!pfxCount)
      {
        perspectiveFixup = CS::Math::Matrix4 ();
        changed = false;
        return false;
      }
      TextureAllocationInfo info;

      // resolution changed
      if (postEffectManager->SetupView(width, height))
      {
        pingPong[0] = postEffectManager->RequestTexture (info, -1);
        pingPong[1] = postEffectManager->RequestTexture (info, -1);
        changed = true;
        if (autoHook)
        {
          info.format = "d32";
          depthHook = postEffectManager->RequestTexture (info, -1);
        }
      }
      

      bool effectsDataChanged = changed;
      for (size_t i = 0; i < pfxCount ; ++i)
      {
        iPostEffect* effect = postEffects.Get (i);
        if (changed)
        {
          effect->SetInputTexture(pingPong[(i+1)%2]);
          effect->SetOutputTarget(pingPong[i%2]);
        }
        effectsDataChanged |= effect->SetupView (width, height);
      }

      // Set the perspective fixup with the last effect
      iPostEffect* lastEffect = postEffects.Get (pfxCount - 1);
      iTextureHandle* screenTarget = lastEffect->GetScreenTarget ();
      if (screenTarget)
      {
        int targetW, targetH;
        screenTarget->GetRendererDimensions (targetW, targetH);
        float scaleX = float (width)/float (targetW);
        float scaleY = float (height)/float (targetH);
        perspectiveFixup = CS::Math::Matrix4 (
          scaleX, 0, 0, scaleX-1.0f,
          0, scaleY, 0, scaleY-1.0f,
          0, 0, 1, 0,
          0, 0, 0, 1);
      }
      else
      {
        perspectiveFixup = CS::Math::Matrix4 ();
      }

      changed = false;
      return effectsDataChanged;
    }

    bool PostEffectsSupport::ScreenSpaceYFlipped () const
    {
      for (size_t i = 0; i < postEffects.GetSize (); i++)
      {
        if (postEffects.Get (i)->ScreenSpaceYFlipped ())
          return true;
      }

      return false;
    }

    void PostEffectsSupport::SetDepthBuffer (iTextureHandle * depth)
    {
      autoHook = (depth == nullptr);
      depthHook = depth;
    }

    void PostEffectsSupport::SetPostEffectsEnabled(bool status)
    {
      enabled = status;
    }

    bool PostEffectsSupport::SetupCommonSVs (iView* view)
    {
      iCamera * cam = view->GetCamera();
      const CS::Math::Matrix4& proj = cam->GetProjectionMatrix();
      const CS::Math::Matrix4& inv_proj = cam->GetInvProjectionMatrix();


      csShaderVariable* sv = postEffectManager->GetSharedSVs()->GetVariableAdd(matProjID);
      sv->SetValue(proj);
      sv = postEffectManager->GetSharedSVs()->GetVariableAdd(invMatProjID);
      sv->SetValue(inv_proj);

      sv = postEffectManager->GetSharedSVs()->GetVariableAdd(focalLenID);
      sv->SetValue(csVector2(proj.m11, proj.m22));
      sv = postEffectManager->GetSharedSVs()->GetVariableAdd(invFocalLenID);
      sv->SetValue(csVector2(1.0f/proj.m11, 1.0f/proj.m22));

      sv = postEffectManager->GetSharedSVs()->GetVariableAdd(texDepthID);
      sv->SetValue(depthHook);
      return true;
    }


  } // namespace RenderManager
} // namespace CS
