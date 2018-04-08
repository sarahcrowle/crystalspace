/*
    Copyright (C) 2012 by Dominik Seifert
    Copyright (C) 2011 by Liu Lu

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

#ifndef __CS_BULLET_COLLISIONGHOST_H__
#define __CS_BULLET_COLLISIONGHOST_H__

#include "bulletsystem.h"
#include "common2.h"
#include "collisionobject2.h"

#include "csutil/custom_new_disable.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include "csutil/custom_new_enable.h"

CS_PLUGIN_NAMESPACE_BEGIN(Bullet2)
{

class BulletGhostCollisionObjectFactory : public scfVirtImplementationExt1<
  BulletGhostCollisionObjectFactory, BulletCollisionObjectFactory, CS::Collisions::iCollisionObjectFactory> 
{
public:
  BulletGhostCollisionObjectFactory
    (csBulletSystem* system, CS::Collisions::iCollider* collider = nullptr, const char* name = "GhostObject")
    : scfImplementationType (this, system, collider)
  {
  }
    
  /// Create a new object
  virtual csPtr<CS::Collisions::iCollisionObject> CreateGhostCollisionObject();
  virtual csPtr<CS::Collisions::iCollisionObject> CreateCollisionObject();
};

class csBulletGhostCollisionObject : public scfVirtImplementationExt1<
  csBulletGhostCollisionObject, csBulletCollisionObject, CS::Collisions::iCollisionObject>
{
public:
  void CreateGhostCollisionObject(CS::Collisions::iCollisionObjectFactory* props);

public:
  csBulletGhostCollisionObject(csBulletSystem* sys);
  virtual ~csBulletGhostCollisionObject();
  virtual bool IsPhysicalObject() const { return false; }
  virtual CS::Collisions::CollisionObjectType GetObjectType () const
  { return CS::Collisions::COLLISION_OBJECT_GHOST; }

  btPairCachingGhostObject* GetPairCachingGhostObject() const
  { 
    btGhostObject* go = btGhostObject::upcast(btObject);
    btPairCachingGhostObject* obj = (btPairCachingGhostObject*) (go); 
    return obj;
  }
  
  virtual void RebuildObject();
  virtual bool AddBulletObject();
  virtual bool RemoveBulletObject();
};
}
CS_PLUGIN_NAMESPACE_END (Bullet2)

#endif
