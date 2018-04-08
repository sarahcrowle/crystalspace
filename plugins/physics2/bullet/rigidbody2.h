/*
    Copyright (C) 2011-2012 Christian Van Brussel, Institute of Information
      and Communication Technologies, Electronics and Applied Mathematics
      at Universite catholique de Louvain, Belgium
      http://www.uclouvain.be/en-icteam.html
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
#ifndef __CS_BULLET_RIGIDBODY_H__
#define __CS_BULLET_RIGIDBODY_H__

#include "common2.h"
#include "physicalbody.h"
#include "physicsfactories.h"
#include "ivaria/collisions.h"

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
class csBulletPhysicalSystem;
class csBulletDefaultKinematicCallback;

using CS::Physics::iPhysicalBody;
using CS::Physics::iRigidBody;
using CS::Physics::iSoftBody;

class csBulletRigidBody : public scfVirtImplementationExt1<csBulletRigidBody, 
    csPhysicalBody, 
    CS::Physics::iRigidBody>
{
  friend class csBulletKinematicMotionState;
  friend class csBulletSoftBody;
  friend class csBulletJoint;
  friend class csBulletSector;
  friend class CollisionPortal;

protected:
  csRef<BulletRigidBodyFactory> factory;
  btRigidBody* btBody;
  csBulletMotionState* motionState;
  CS::Physics::RigidBodyState physicalState;
  short anchorCount;
  csRef<CS::Physics::iKinematicCallback> kinematicCb;

protected:
  virtual csBulletMotionState* CreateMotionState (const btTransform& trans);

public:
  csBulletRigidBody (BulletRigidBodyFactory* factory);
  virtual ~csBulletRigidBody ();

  virtual iObject* QueryObject () { return (iObject*) this; }

  //-- iCollisionObject
  virtual void RebuildObject ();

  virtual void SetCollider (CS::Collisions::iCollider* collider);

  btRigidBody* GetBulletRigidPointer () {return btBody;}
  virtual bool RemoveBulletObject ();
  virtual bool AddBulletObject ();

  virtual void SetTransform (const csOrthoTransform& trans);
  virtual csOrthoTransform GetTransform () const;

  //-- iPhysicalBody
  virtual CS::Physics::PhysicalObjectType GetPhysicalObjectType () const
  { return CS::Physics::PHYSICAL_OBJECT_RIGIDBODY; }
  virtual iRigidBody* QueryRigidBody () { return dynamic_cast<iRigidBody*> (this); }
  virtual iSoftBody* QuerySoftBody () { return nullptr; }

  virtual bool IsDynamic () const
  { return physicalState == CS::Physics::STATE_DYNAMIC; }

  virtual void SetMass (btScalar mass);
  virtual btScalar GetMass () const;

  void SetMassInternal (btScalar mass);

  virtual btScalar GetDensity () const;
  virtual void SetDensity (btScalar density);

  virtual btScalar GetVolume () const;

  virtual void AddForce (const csVector3& force);

  virtual csVector3 GetLinearVelocity () const;
  virtual void SetLinearVelocity (const csVector3& vel);
  
  //-- iRigidBody
  virtual CS::Physics::RigidBodyState GetState () const {return physicalState;}
  virtual void SetState (CS::Physics::RigidBodyState state);

  virtual void SetElasticity (float elasticity);
  virtual float GetElasticity ();

  virtual void SetAngularVelocity (const csVector3& vel);
  virtual csVector3 GetAngularVelocity () const;

  virtual void AddTorque (const csVector3& torque);

  virtual void AddRelForce (const csVector3& force);
  virtual void AddRelTorque (const csVector3& torque);

  virtual void AddForceAtPos (const csVector3& force,
      const csVector3& pos);
  virtual void AddForceAtRelPos (const csVector3& force,
      const csVector3& pos);

  virtual void AddRelForceAtPos (const csVector3& force,
      const csVector3& pos);
  virtual void AddRelForceAtRelPos (const csVector3& force,
      const csVector3& pos);

  virtual void ApplyImpulse (const csVector3& impulse);
  virtual void ApplyImpulse (const csVector3& impulse, const csVector3& position);
  virtual void ApplyTorque (const csVector3& torque);

  virtual csVector3 GetTotalForce () const;
  virtual csVector3 GetTotalTorque () const;

  virtual void SetKinematicCallback (CS::Physics::iKinematicCallback* cb) {kinematicCb = cb;}
  virtual CS::Physics::iKinematicCallback* GetKinematicCallback () {return kinematicCb;}
  
  virtual btScalar GetLinearDamping ();
  virtual void SetLinearDamping (btScalar d);
  
  virtual btScalar GetAngularDamping ();
  virtual void SetAngularDamping (btScalar d);

  virtual csVector3 GetAngularFactor () const;
  virtual void SetAngularFactor (const csVector3& f);

  virtual btCollisionObject* CreateBulletObject ();
  virtual csPtr<CS::Collisions::iCollisionObject> CloneObject ();

  virtual bool GetGravityEnabled () const;
  virtual void SetGravityEnabled (bool enabled);

  // Some convinience methods:

  /// Whether gravity is currently affecting this object
  bool DoesGravityApply () const;
  bool IsMovingUpward () const;
};

class csBulletDefaultKinematicCallback : public scfImplementation1<
  csBulletDefaultKinematicCallback, CS::Physics::iKinematicCallback>
{
public:
  csBulletDefaultKinematicCallback ();
  virtual ~csBulletDefaultKinematicCallback ();
  virtual void GetBodyTransform (CS::Physics::iRigidBody* body, csOrthoTransform& transform) const;
};
}
CS_PLUGIN_NAMESPACE_END (Bullet2)

#endif
