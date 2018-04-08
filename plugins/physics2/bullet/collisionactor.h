/*
    Copyright (C) 2013 Christian Van Brussel, Institute of Information
      and Communication Technologies, Electronics and Applied Mathematics
      at Universite catholique de Louvain, Belgium
      http://www.uclouvain.be/en-icteam.html
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
#ifndef __CS_BULLET_COLLISIONACTOR_H__
#define __CS_BULLET_COLLISIONACTOR_H__

#include "common2.h"
#include "collisionghost.h"

#if (CS_BULLET_VERSION >= 281)

#include "csutil/custom_new_disable.h"
#include "BulletDynamics/Character/btKinematicCharacterController.h"
#include "csutil/custom_new_enable.h"

class KinematicCharacterController : public btKinematicCharacterController
{
public:
  KinematicCharacterController
    (btPairCachingGhostObject* ghostObject, btConvexShape* convexShape, btScalar stepHeight, int upAxis = 1)
    : btKinematicCharacterController (ghostObject, convexShape, stepHeight, upAxis)
  {}

  void setGravity (btScalar gravity)
  { m_gravity = gravity; }

  void setStepHeight (btScalar stepHeight)
  { m_stepHeight = stepHeight; }

  btScalar getStepHeight() const
  { return m_stepHeight; }

#if (CS_BULLET_VERSION == 281)
  void reset (btCollisionWorld*)
  { btKinematicCharacterController::reset (); }
#endif
};

#else

#include "csutil/custom_new_disable.h"
#include "btKinematicCharacterController.h"
#include "csutil/custom_new_enable.h"

class KinematicCharacterController : public csbtKinematicCharacterController
{
public:
  KinematicCharacterController
    (btPairCachingGhostObject* ghostObject, btConvexShape* convexShape, btScalar stepHeight, int upAxis = 1)
    : csbtKinematicCharacterController (ghostObject, convexShape, stepHeight, upAxis)
  {}

  void reset (btCollisionWorld*)
  { csbtKinematicCharacterController::reset (); }
};

#endif

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

class BulletCollisionActorFactory : public scfVirtImplementationExt1<
  BulletCollisionActorFactory, BulletGhostCollisionObjectFactory, CS::Collisions::iCollisionActorFactory> 
{
  friend class BulletCollisionActor;

  float stepHeight;
  float maximumSlope;
  float jumpSpeed;
  //float airControlFactor;

public:
  BulletCollisionActorFactory
    (csBulletSystem* system, CS::Collisions::iCollider* collider = nullptr)
    : scfImplementationType (this, system, collider),
    stepHeight (.4f),
    maximumSlope (0.785398163f),
    jumpSpeed (10.f)
    //,airControlFactor (0.04f)
    {}

  virtual csPtr<CS::Collisions::iCollisionActor> CreateCollisionActor ();
  virtual csPtr<CS::Collisions::iCollisionObject> CreateCollisionObject ();

  virtual float GetStepHeight () const { return stepHeight * system->GetInverseInternalScale (); }
  virtual void SetStepHeight (float h) { stepHeight = h * system->GetInternalScale (); }

  virtual float GetMaximumSlope () const { return maximumSlope; };
  virtual void SetMaximumSlope (float slope) { maximumSlope = slope; };

  virtual float GetJumpSpeed () const { return jumpSpeed * system->GetInverseInternalScale (); }
  virtual void SetJumpSpeed (float s) { jumpSpeed = s * system->GetInternalScale (); }

  //virtual float GetAirControlFactor () const { return airControlFactor; }
  //virtual void SetAirControlFactor (float f) { airControlFactor = f; }
};

class BulletCollisionActor : public scfVirtImplementationExt1<BulletCollisionActor,
    csBulletGhostCollisionObject, CS::Collisions::iCollisionActor>,
    public btActionInterface
{
  KinematicCharacterController* controller;
  //float airControlFactor;

public:
  BulletCollisionActor (BulletCollisionActorFactory* factory);
  virtual ~BulletCollisionActor ();

  virtual iObject* QueryObject () { return (iObject*) this; }

  //-- iCollisionObject
  virtual CS::Collisions::iCollisionObject* QueryCollisionObject ()
  { return dynamic_cast<CS::Collisions::iCollisionObject*> (this); }
  virtual CS::Collisions::iCollisionActor* QueryCollisionActor ()
  { return dynamic_cast<CS::Collisions::iCollisionActor*> (this); }

  virtual bool IsPhysicalObject () const { return false; }

  virtual CS::Collisions::CollisionObjectType GetObjectType () const
  { return CS::Collisions::COLLISION_OBJECT_ACTOR; }

  virtual bool AddBulletObject ();
  virtual bool RemoveBulletObject ();

  virtual void SetTransform (const csOrthoTransform& trans);

  //-- iCollisionActor
  virtual float GetStepHeight () const;
  virtual void SetStepHeight (float stepHeight);

  virtual float GetMaximumSlope () const;
  virtual void SetMaximumSlope (float slopeRadians);
  
  virtual float GetJumpSpeed () const { return 0; }
  virtual void SetJumpSpeed (float jumpSpeed);

  virtual void SetSpeed (const csVector3& speed);
  //virtual csVector3 GetSpeed () const;

  //virtual void SetRotation (const csMatrix3& rot);
  virtual void Rotate (float yaw);

  virtual void Jump ();

  virtual bool IsOnGround () const;

/*
  virtual float GetAirControlFactor () const { return airControlFactor; }
  virtual void SetAirControlFactor (float f) { airControlFactor = f; }
*/

  //-- btActionInterface
  void updateAction (btCollisionWorld* collisionWorld, btScalar deltaTime);
  void debugDraw (btIDebugDraw* debugDrawer) {}

  // Misc
  void SetGravity (float gravity);
  //float GetGravity () const;
};

}
CS_PLUGIN_NAMESPACE_END (Bullet2)
#endif
