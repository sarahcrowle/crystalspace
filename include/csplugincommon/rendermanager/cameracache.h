/*
    Copyright (C) 2013 by Frank Richter

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

#ifndef __CS_CSPLUGINCOMMON_RENDERMANAGER_CAMERACACHE_H__
#define __CS_CSPLUGINCOMMON_RENDERMANAGER_CAMERACACHE_H__

/**\file
 * Helper class to cache clones of cameras.
 */

#include "iengine/camera.h"

#include "csutil/priorityqueue.h"
#include "csutil/weakkeyedhash.h"
#include "csutil/weakref.h"

struct iEngine;

namespace CS
{
  namespace RenderManager
  {
    /**
     * Helper class to cache clones of cameras.
     * Either clones a camera, or returns a previously created clone of the same camera.
     */
    class CS_CRYSTALSPACE_EXPORT CameraCache
    {
      struct CameraClone
      {
        /// Frame number this camera was last used
        uint lastFrame;
        /// Camera number of the original cam when the clone was taken
        long origCamNum;
        /// Camera of the clone after the last sync
        long cloneCamNum;
        csRef<iCustomMatrixCamera> cam;

        CameraClone (uint lastFrame, iCamera* oldCam, iCustomMatrixCamera* newCam)
          : lastFrame (lastFrame),
            origCamNum (oldCam->GetCameraNumber()),
            cloneCamNum (newCam->GetCamera()->GetCameraNumber()),
            cam (newCam) {}

        // Used by PriorityQueue
        bool operator< (const CameraClone& other) const
        {
          // Return clones with the lowest frame number first
          return lastFrame > other.lastFrame;
        }
      };
      typedef CS::Utility::PriorityQueue<CameraClone> CameraCloneQueue;
      typedef CS::Container::WeakKeyedHash<CameraCloneQueue, csWeakRef<iCamera> > ClonesHash;
      ClonesHash clones;
      csWeakRef<iEngine> engine;
    public:
      /// Initialize camera cache
      void Initialize (iEngine* engine);

      /**
       * Flags on what properties of the camera to keep in sync automatically.
       * Properties that are immediately changed anyway should can be omitted from
       * the flags for optimization purposes.
       */
      enum
      {
        /// Sync camera transforms
        syncTransform = 1,
        /// Sync camera sector
        syncSector = 2,
        /// Sync mirrored flag
        syncMirrored = 4,
        /// Sync far plane
        syncFarPlane = 8,
        /// Sync "only portals" flag
        syncOnlyPortals = 16,
        /// Sync projection matrix
        syncProjection = 32,

        /// Sync everything
        syncAll = syncTransform | syncSector | syncMirrored | syncFarPlane
            | syncOnlyPortals | syncProjection,
        /// Sync nothing
        syncNothing = 0
      };
      iCamera* GetClone (iCamera* originalCam, uint syncFlags);

      /// Remove all clones for cameras that disappeared
      void Purge ();
    };
  } // namespace RenderManager
} // namespace CS

#endif // __CS_CSPLUGINCOMMON_RENDERMANAGER_CAMERACACHE_H__
