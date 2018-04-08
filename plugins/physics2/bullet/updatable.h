/*
    Copyright (C) 2012 by Dominik Seifert

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
#ifndef __CS_PHYSICS_BULLET_UPDATABLE_H__
#define __CS_PHYSICS_BULLET_UPDATABLE_H__

#include "csutil/scf.h"

namespace CS {
namespace Collisions {
  struct iCollisionObject;
}
}

namespace CS {
namespace Physics {
  struct iPhysicalSector;
}
}

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

class csBulletSector;

/**
 * Custom code to be called upon every step
 */
struct iUpdatable : public virtual iBase
{
  SCF_INTERFACE (iUpdatable, 1, 0, 0);

  /**
   * Method is called before every step, given "dt" seconds have passed since the beginning of the last step.
   */
  virtual void PreStep (float dt) = 0;
  /**
   * Method is called after every step, given "dt" seconds have passed since the beginning of the last step.
   */
  virtual void PostStep (float dt) = 0;

  /// The collision object associated with this updatable (if any)
  virtual CS::Collisions::iCollisionObject* GetCollisionObject () = 0;

  /// Called when updatable is added to the given sector
  virtual void OnAdded (csBulletSector* sector) = 0;

  /// Called when updatable is removed from the given sector
  virtual void OnRemoved (csBulletSector* sector) = 0;
};

}
CS_PLUGIN_NAMESPACE_END (Bullet2)

#endif // __CS_PHYSICS_BULLET_UPDATABLE_H__
