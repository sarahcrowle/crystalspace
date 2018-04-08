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
#ifndef __CS_BULLET_SECTOR_H__
#define __CS_BULLET_SECTOR_H__

#include "csutil/csobject.h"
#include "csutil/scf.h"
#include "csutil/scf_implementation.h"
#include "csutil/nobjvec.h"
#include "iengine/movable.h"
#include "iengine/portal.h"
#include "iengine/sector.h"
#include "iutil/comp.h"
#include "ivaria/collisions.h"
#include "ivaria/physics.h"
#include "ivaria/reporter.h"
#include "ivaria/view.h"

#include "updatable.h"

#include "csutil/custom_new_disable.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include "csutil/custom_new_enable.h"

struct iSector;
struct iMovable;

class btCollisionObject;
class btCompoundShape;
class btDynamicsWorld;
class btCollisionDispatcher;
class btDefaultCollisionConfiguration;
class btSequentialImpulseConstraintSolver;
class btBroadphaseInterface;
struct btSoftBodyWorldInfo;
class btActionInterface;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

class csBulletSystem;
class csBulletSector;
class csBulletDebugDraw;
class csBulletRigidBody;
class csBulletSoftBody;
class BulletCollisionActor;
class csBulletCollisionObject;
class csBulletCollider;
class csBulletJoint;
class CollisionPortal;

// TODO: what's this?
struct BulletActionWrapper : public virtual iUpdatable
{
  SCF_INTERFACE (CS::Plugin::Bullet2::BulletActionWrapper, 1, 0, 0);

  virtual btActionInterface* GetBulletAction () = 0;
};

// Also implements iPhysicalSector
class csBulletSector : public scfVirtImplementationExt2<
  csBulletSector, csObject,
  CS::Physics::iPhysicalSector,
  CS::Collisions::iCollisionSector>
{
  friend class csBulletCollisionObject;
  friend class BulletCollisionActor;
  friend class csBulletRigidBody;
  friend class csBulletSoftBody;
  friend class csBulletJoint;
  friend class csBulletCollisionTerrain;
  friend class csBulletKinematicMotionState;
  friend class csBulletMotionState;
  friend class csBulletSystem;
  friend class csBulletGhostCollisionObject;
  friend class BulletVehicle;
  friend class CollisionPortal;

  csBulletSystem* system;

  btDynamicsWorld* bulletWorld;
  btCollisionDispatcher* dispatcher;
  btDefaultCollisionConfiguration* configuration;
  btSequentialImpulseConstraintSolver* solver;
  btBroadphaseInterface* broadphase;
  btSoftBodyWorldInfo* softWorldInfo;

  float linearDampening;
  float angularDampening;

  float linearDisableThreshold;
  float angularDisableThreshold;
  float timeDisableThreshold;

  csRef<iSector> sector;

  csRefArray<csBulletJoint> joints;
  // TODO: csRefArray of portals?
  csArray<CollisionPortal*> portals;
  csRefArrayObject<csBulletCollisionObject> collisionObjects;
  csRefArrayObject<csBulletRigidBody> rigidBodies;
  csRefArrayObject<csBulletSoftBody> softBodies;
  csWeakRefArray<csBulletSoftBody> anchoredSoftBodies;
  csRefArray<BulletCollisionActor> actors;
  csRefArray<iUpdatable> updatables;

  void CheckCollisions ();
  void UpdateCollisionPortalsPreStep ();
  void UpdateCollisionPortalsPostStep ();

  void AddRigidBody (CS::Physics::iRigidBody* body);
  void AddSoftBody (CS::Physics::iSoftBody* body);

public:
  csBulletSector (csBulletSystem* system);
  virtual ~csBulletSector ();
  
  //-- iCollisionSector
  virtual CS::Collisions::iCollisionSystem* GetSystem () const
  { return (CS::Collisions::iCollisionSystem*) system; }

  virtual iObject* QueryObject () const
  { return (iObject*) this; }

  virtual CS::Collisions::CollisionObjectType GetSectorType () const
  { return CS::Collisions::COLLISION_OBJECT_PHYSICAL; }
  virtual iPhysicalSector* QueryPhysicalSector () const
  { return (iPhysicalSector*) this; }

  virtual void SetGravity (const csVector3& v);
  virtual csVector3 GetGravity () const;

  inline btDynamicsWorld* GetBulletWorld () const { return bulletWorld; }

  virtual void AddCollisionObject (CS::Collisions::iCollisionObject* object);
  virtual void RemoveCollisionObject (CS::Collisions::iCollisionObject* object);

  virtual size_t GetCollisionObjectCount () {return collisionObjects.GetSize ();}
  virtual CS::Collisions::iCollisionObject* GetCollisionObject (size_t index);

  virtual void AddPortal (iPortal* portal, const csOrthoTransform& meshTrans);
  virtual void RemovePortal (iPortal* portal);

  virtual void SetSector (iSector* sector);
  virtual iSector* GetSector () { return sector; }

  virtual CS::Collisions::HitBeamResult HitBeam
    (const csVector3& start, const csVector3& end) const;
  inline virtual CS::Collisions::HitBeamResult HitBeamPortal
    (const csVector3& start, const csVector3& end) const
  { return HitBeamPortals (start, end); }
  virtual CS::Collisions::HitBeamResult HitBeamPortals
    (const csVector3& start, const csVector3& end) const;

  const btGhostObject* HitBeam (const csVector3& start, const csVector3& end,
				CS::Collisions::HitBeamResult& result) const;
  CS::Collisions::HitBeamResult RigidHitBeam
    (btCollisionObject* object, const csVector3& start, const csVector3& end) const;

  virtual csPtr<CS::Collisions::iCollisionDataList> CollisionTest
    (CS::Collisions::iCollisionObject* object);

  csPtr<CS::Collisions::iCollisionData> BulletCollide
    (btCollisionObject* objectA, btCollisionObject* objectB);

  virtual void DeleteAll ();

  //-- iPhysicalSector
  virtual void SetLinearDamping (float d);
  virtual float GetLinearDamping () const {return linearDampening;}

  virtual void SetAngularDamping (float d);
  virtual float GetAngularDamping () const {return angularDampening;}

  virtual void SetAutoDisableParams (float linear,
    float angular, float time);

  virtual size_t GetRigidBodyCount () {return rigidBodies.GetSize ();}
  virtual CS::Physics::iRigidBody* GetRigidBody (size_t index);

  virtual size_t GetSoftBodyCount () {return softBodies.GetSize ();}
  virtual CS::Physics::iSoftBody* GetSoftBody (size_t index);

  virtual void AddJoint (CS::Physics::iJoint* joint);
  virtual void RemoveJoint (CS::Physics::iJoint* joint);

  virtual void AddVehicle (CS::Physics::iVehicle* vehicle);
  virtual void RemoveVehicle (CS::Physics::iVehicle* vehicle);

  //Bullet::iPhysicalSector
  //Currently will not use gimpact shape...
  //virtual void SetGimpactEnabled (bool enabled);
  //virtual bool GetGimpactEnabled ();

  virtual void AddUpdatable (iUpdatable* u);
  virtual void RemoveUpdatable (iUpdatable* u);

  // Internal methods
  void Step (float duration);
  void UpdateSoftBodies (float timeStep);

  virtual bool Save (const char* filename);
};

}
CS_PLUGIN_NAMESPACE_END (Bullet2)

#endif
