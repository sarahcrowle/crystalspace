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

#ifndef __CS_BULLET_JOINT_H__
#define __CS_BULLET_JOINT_H__

#include "bulletsystem.h"
#include "common2.h"

#include "csutil/custom_new_disable.h"
#include "BulletSoftBody/btSoftBody.h"
#include "csutil/custom_new_enable.h"

class btGeneric6DofConstraint;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

#define JOINT_SOFT 1
#define JOINT_SPRING 2
#define JOINT_POSITION 4
#define JOINT_TRANSFORM 8
#define JOINT_EQUIL_POINT 16
#define JOINT_INSIDE_WORLD 32

enum csJointType
{
  RIGID_PIVOT_JOINT,
  RIGID_P2P_JOINT,
  RIGID_HINGE_JOINT,
  RIGID_SLIDE_JOINT,
  RIGID_CONETWIST_JOINT,
  RIGID_6DOF_JOINT,
  SOFT_LINEAR_JOINT,
  SOFT_ANGULAR_JOINT,
};

class JointFactory : public scfVirtImplementationExt1<
  JointFactory, csObject, CS::Physics::iJointFactory> 
{
  friend class csBulletJoint;

private:
  csBulletSystem* system;
  csJointType type;

  bool transConstraintX;
  bool transConstraintY;
  bool transConstraintZ;

  bool rotConstraintX;
  bool rotConstraintY;
  bool rotConstraintZ;

  csVector3 minDist;
  csVector3 maxDist;
  csVector3 minAngle;
  csVector3 maxAngle;

  csVector3 bounce;
  csVector3 desiredVelocity;
  csVector3 maxForce;

  bool isSpring;

  csVector3 linearStiff;
  csVector3 angularStiff;
  csVector3 linearDamp;
  csVector3 angularDamp;
  csVector3 linearEquilPoint;
  csVector3 angularEquilPoint;
  float threshold;

  int axis;

public:
  JointFactory (csBulletSystem* system);

  virtual csPtr<CS::Physics::iJoint> CreateJoint ();

  virtual void SetType (csJointType type)
  { this->type = type; }

  virtual void SetTransConstraints (bool X, bool Y, bool Z)
  {
    transConstraintX = X;
    transConstraintY = Y;
    transConstraintZ = Z;
  }

  virtual void GetTransConstraints (bool& X, bool& Y, bool& Z) const
  {
    X = transConstraintX;
    Y = transConstraintY;
    Z = transConstraintZ;
  }

  virtual void SetRotConstraints (bool X, bool Y, bool Z)
  {
    rotConstraintX = X;
    rotConstraintY = Y;
    rotConstraintZ = Z;
  }

  virtual void GetRotConstraints (bool& X, bool& Y, bool& Z) const
  {
    X = rotConstraintX;
    Y = rotConstraintY;
    Z = rotConstraintZ;
  }

  virtual void SetMinimumDistance (const csVector3& distance)
  { minDist = distance; }
  virtual const csVector3& GetMinimumDistance () const
  { return minDist; }
  virtual void SetMaximumDistance (const csVector3& distance)
  { maxDist = distance; }
  virtual const csVector3& GetMaximumDistance () const
  { return maxDist; }

  virtual void SetMinimumAngle (const csVector3& angle)
  { minAngle = angle; }
  virtual const csVector3& GetMinimumAngle () const
  { return minAngle; }
  virtual void SetMaximumAngle (const csVector3& angle)
  { maxAngle = angle; }
  virtual const csVector3& GetMaximumAngle () const
  { return maxAngle; }

  virtual void SetBounce (const csVector3& bounce)
  { this->bounce = bounce; }
  virtual const csVector3& GetBounce () const
  { return bounce; }

  virtual void SetDesiredVelocity (const csVector3& velocity)
  { desiredVelocity = velocity; }
  virtual const csVector3& GetDesiredVelocity () const
  { return desiredVelocity; }
  virtual void SetMaxForce (const csVector3& force)
  { maxForce = force; }
  virtual const csVector3& GetMaxForce () const
  { return maxForce; }

  virtual void SetSpring (bool isSpring)
  { this->isSpring = isSpring; }

  virtual void SetLinearStiffness (const csVector3& stiffness)
  { linearStiff = stiffness; }
  virtual const csVector3& GetLinearStiffness () const
  { return linearStiff; }
  virtual void SetAngularStiffness (const csVector3& stiffness)
  { angularStiff = stiffness; }
  virtual const csVector3& GetAngularStiffness () const
  { return angularStiff; }

  virtual void SetLinearDamping (const csVector3& damping)
  { linearDamp = damping; }
  virtual const csVector3& GetLinearDamping () const
  { return linearDamp; }
  virtual void SetAngularDamping (const csVector3& damping)
  { angularDamp = damping; }
  virtual const csVector3& GetAngularDamping () const
  { return angularDamp; }
  
  virtual void SetLinearEquilibriumPoint (const csVector3& point)
  { linearEquilPoint = point; }
  virtual void SetAngularEquilibriumPoint (const csVector3& point)
  { angularEquilPoint = point; }

  virtual void SetBreakingImpulseThreshold (float threshold)
  { this->threshold = threshold; }
  virtual float GetBreakingImpulseThreshold () const
  { return threshold; }

  void SetAxis (int axis)
  { this->axis = axis; }
};

class csBulletJoint : public scfImplementation1<
  csBulletJoint, CS::Physics::iJoint>
{
  friend class csBulletSystem;
  friend class csBulletSector;
  friend class CollisionPortal;

private:
  // TODO: 'system' is redundant with the one of the sector
  csBulletSystem* system;
  csBulletSector* sector;

  csOrthoTransform transform;
  csVector3 linearStiff;
  csVector3 angularStiff;
  csVector3 linearDamp;
  csVector3 angularDamp;
  csVector3 linearEquilPoint;
  csVector3 angularEquilPoint;

  csVector3 position;
  btVector3 minDist;
  btVector3 maxDist;
  csVector3 minAngle;
  csVector3 maxAngle;

  csVector3 bounce;
  csVector3 desiredVelocity;
  btVector3 maxForce;
  btTransform frA, frB;
  csJointType type;
  btTypedConstraint* rigidJoint;
  btSoftBody::Joint* softJoint;
  csRef<CS::Physics::iPhysicalBody> bodies[2];
  float threshold;
  int axis;

  char jointFlag;

  bool transConstraintX;
  bool transConstraintY;
  bool transConstraintZ;

  bool rotConstraintX;
  bool rotConstraintY;
  bool rotConstraintZ;

public:
  csBulletJoint (csBulletSystem* system);
  csBulletJoint (csBulletSystem* system, JointFactory* factory);
  virtual ~csBulletJoint ();

  void SetType (csJointType type) {this->type = type;}

  virtual void Attach (CS::Physics::iPhysicalBody* body1, CS::Physics::iPhysicalBody* body2,
    bool forceUpdate = true);

  virtual CS::Physics::iPhysicalBody* GetAttachedBody (int index)
  {
    CS_ASSERT (index >= 0 && index <= 1);
    return bodies[index];
  }

  virtual void SetTransform (const csOrthoTransform& trans,
    bool forceUpdate = false);

  virtual csOrthoTransform GetTransform () const 
  {return transform;}

  virtual void SetPosition (const csVector3& position,
    bool forceUpdate = false);
  virtual const csVector3& GetPosition () const {return position;}

  virtual void SetTransConstraints (bool X, 
    bool Y, bool Z, bool forceUpdate = false);
  virtual void GetTransConstraints (bool& X, bool& Y, bool& Z) const
  {
    X = transConstraintX;
    Y = transConstraintY;
    Z = transConstraintZ;
  }

  virtual void SetMinimumDistance (const csVector3& dist,
    bool forceUpdate = false);
  virtual csVector3 GetMinimumDistance () const
  { return BulletToCS (minDist, system->GetInverseInternalScale ()); }
  virtual void SetMaximumDistance (const csVector3& dist,
    bool forceUpdate = false);
  virtual csVector3 GetMaximumDistance () const
  { return BulletToCS (maxDist, system->GetInverseInternalScale ()); }

  virtual void SetRotConstraints (bool X, 
    bool Y, bool Z, bool forceUpdate = false);
  virtual void GetRotConstraints (bool& X, bool& Y, bool& Z) const
  {
    X = rotConstraintX;
    Y = rotConstraintY;
    Z = rotConstraintZ;
  }

  virtual void SetMinimumAngle (const csVector3& angle,
    bool forceUpdate = false);
  virtual const csVector3& GetMinimumAngle () const {return minAngle;}
  virtual void SetMaximumAngle (const csVector3& angle,
    bool forceUpdate = false);
  virtual const csVector3& GetMaximumAngle () const {return maxAngle;}

  virtual void SetBounce (const csVector3& bounce,
    bool forceUpdate = false);
  virtual const csVector3& GetBounce () const {return bounce;}

  virtual void SetDesiredVelocity (const csVector3& velo,
    bool forceUpdate = false);
  virtual const csVector3& GetDesiredVelocity () const {return desiredVelocity;}

  virtual void SetMaxForce (const csVector3& force,
    bool forceUpdate = false);
  virtual csVector3 GetMaxForce () const;

  virtual bool RebuildJoint ();

  virtual void SetSpring(bool isSpring, bool forceUpdate = false);

  virtual void SetLinearStiffness (const csVector3& stiff, bool forceUpdate = false);
  virtual const csVector3& GetLinearStiffness () const {return linearStiff;}
  virtual void SetAngularStiffness (const csVector3& stiff, bool forceUpdate = false);
  virtual const csVector3& GetAngularStiffness () const {return angularStiff;}

  virtual void SetLinearDamping (const csVector3& damp, bool forceUpdate = false);
  virtual const csVector3& GetLinearDamping () const {return linearDamp;}
  virtual void SetAngularDamping (const csVector3& damp, bool forceUpdate = false);
  virtual const csVector3& GetAngularDamping () const {return angularDamp;}

  virtual void SetLinearEquilibriumPoint (const csVector3& point, bool forceUpdate = false);
  virtual void SetAngularEquilibriumPoint (const csVector3& point, bool forceUpdate = false);

  virtual void SetBreakingImpulseThreshold (float threshold, bool forceUpdate = false);
  virtual float GetBreakingImpulseThreshold () const {return threshold;}

  void AddBulletJoint ();
  void RemoveBulletJoint ();
};
}
CS_PLUGIN_NAMESPACE_END (Bullet2)
#endif
