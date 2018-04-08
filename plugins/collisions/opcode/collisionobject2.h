/*
  Copyright (C) 2011 by Liu Lu

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __CS_OPCODE_COLLISIONOBJECT_H__
#define __CS_OPCODE_COLLISIONOBJECT_H__

#include "iengine/camera.h"
#include "opcodecollisionsystem.h"
#include "colliders2.h"

CS_PLUGIN_NAMESPACE_BEGIN (Opcode2)
{
class csOpcodeCollisionSector;
class csOpcodeCollisionSystem;
class csOpcodeCollider;

class csOpcodeCollisionObject: public scfImplementationExt1<
  csOpcodeCollisionObject, csObject, CS::Collisions::iGhostCollisionObject>
{
  friend class csOpcodeCollisionSector;
private:
  csOpcodeCollisionSystem* system;
  csOpcodeCollisionSector* sector;
  csWeakRef<iMovable> movable;
  csWeakRef<iCamera> camera;
  csRef<CS::Collisions::iCollider> collider;
  CS::Collisions::CollisionObjectType type;
  CS::Collisions::CollisionGroup collGroup;
  csRef<CS::Collisions::iCollisionCallback> collCb;
  csRefArray<csOpcodeCollisionObject> contactObjects;
  csOrthoTransform transform;

  bool insideWorld;
  
public:
  void CreateCollisionObject(CS::Collisions::CollisionObjectProperties* props);

public:
  csOpcodeCollisionObject (csOpcodeCollisionSystem* sys);
  virtual ~csOpcodeCollisionObject ();

  virtual iObject* QueryObject (void) { return (iObject*) this; }
  virtual CS::Collisions::iCollisionObject* QueryCollisionObject () {return dynamic_cast<iCollisionObject*> (this);}
  virtual CS::Physics::iPhysicalBody* QueryPhysicalBody () {return nullptr;}
  virtual CS::Collisions::iActor* QueryActor () {return nullptr;}

  /// Returns the sector to which is this object has been added or nullptr, if not in world
  virtual CS::Collisions::iCollisionSector* GetSector () const { return sector; }

  /// Whether this object is currently added to the world
  virtual bool IsInWorld () const { return sector != nullptr; }

  /// Whether this is a rigid or soft body object
  virtual bool IsPhysicalObject() const { return false; }

  virtual CS::Collisions::CollisionObjectType GetObjectType () const 
  {
      // doesn't really matter
      return CS::Collisions::COLLISION_OBJECT_GHOST;
  }

  virtual void SetAttachedMovable (iMovable* movable){this->movable = movable;}
  virtual iMovable* GetAttachedMovable (){return movable;}

  virtual void SetAttachedCamera (iCamera* camera){this->camera = camera;}
  virtual iCamera* GetAttachedCamera () const {return camera;}

  virtual void SetTransform (const csOrthoTransform& trans);
  virtual csOrthoTransform GetTransform () const;

  virtual void SetRotation (const csMatrix3& rot);
  virtual void Rotate (const csVector3& v, float angle);
  virtual void IncreasePitch(float pitchDelta);
  virtual void IncreaseYaw(float yawDelta);

  virtual void GetAABB(csVector3& aabbMin, csVector3& aabbMax) const { /* TODO */ }
  
  virtual void SetCollider (CS::Collisions::iCollider* newCollider) { collider = newCollider; }
  virtual CS::Collisions::iCollider* GetCollider () const { return collider; }

  virtual void RebuildObject ();
  
  virtual void SetCollisionGroup (const char* name);
  virtual void SetCollisionGroup (const CS::Collisions::CollisionGroup& group);
  virtual const CS::Collisions::CollisionGroup& GetCollisionGroup () const {return collGroup;}

  virtual void SetCollisionCallback (CS::Collisions::iCollisionCallback* cb) {collCb = cb;}
  virtual CS::Collisions::iCollisionCallback* GetCollisionCallback () {return collCb;}

  virtual bool Collide (iCollisionObject* otherObject);
  virtual CS::Collisions::HitBeamResult HitBeam (const csVector3& start, const csVector3& end);

  virtual size_t GetContactObjectsCount ();
  virtual CS::Collisions::iCollisionObject* GetContactObject (size_t index);
};
}
CS_PLUGIN_NAMESPACE_END (Opcode2)
#endif
