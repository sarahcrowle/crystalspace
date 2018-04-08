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

#ifndef __CS_BULLET_PHYSICALOBJECT_H__
#define __CS_BULLET_PHYSICALOBJECT_H__

#include "common2.h"
#include "collisionobject2.h"

#include "ivaria/physics.h"

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

  /**
  * Class extended by csBulletRigidBody and csBulletSoftBody
  */

class csPhysicalBody : public scfVirtImplementationExt1<
  csPhysicalBody, csBulletCollisionObject, CS::Physics::iPhysicalBody>
{
  friend class csBulletSystem;
  
protected:
  // TODO: mass should probably be stored instead of density
  btScalar density;

  void CreatePhysicalBodyObject (CS::Physics::iPhysicalObjectFactory* props);

public:
  csPhysicalBody (csBulletSystem* sys);
  virtual ~csPhysicalBody ();

  virtual CS::Physics::iPhysicalBody* QueryPhysicalBody () {return this;}
  
  virtual CS::Collisions::CollisionObjectType GetObjectType () const
  { return CS::Collisions::COLLISION_OBJECT_PHYSICAL; }

  virtual CS::Physics::PhysicalObjectType GetPhysicalObjectType () const = 0;
  virtual CS::Physics::iRigidBody* QueryRigidBody () = 0;
  virtual CS::Physics::iSoftBody* QuerySoftBody () = 0;

  virtual bool IsDynamic() const { return true; }

  virtual void SetEnabled (bool enabled);
  virtual bool GetEnabled () const;

  virtual void SetFriction (float friction) { btObject->setFriction (friction); }
  virtual float GetFriction () const {return btObject->getFriction ();}

  virtual void SetMass (float mass) = 0;
  virtual float GetMass () const = 0;

  virtual float GetDensity () const {return density;}
  virtual void SetDensity (float density) = 0;

  virtual void SetLinearVelocity (const csVector3& vel) = 0;
  virtual csVector3 GetLinearVelocity () const = 0;

  virtual void SetAngularVelocity (const csVector3& vel) = 0;
  virtual csVector3 GetAngularVelocity () const = 0;
  
  /// Whether this object is affected by gravity
  virtual bool GetGravityEnabled () const { return true; }
  /// Whether this object is affected by gravity
  virtual void SetGravityEnabled (bool enabled) {}

  virtual void AddForce (const csVector3& force) = 0;
  virtual void ApplyImpulse (const csVector3& impulse) = 0;
  
  virtual csPtr<CS::Collisions::iCollisionObject> ClonePassivePortalObject ();
};

}
CS_PLUGIN_NAMESPACE_END (Bullet2)

#endif
