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

#ifndef __CS_BULLET_SOFTBODY_H__
#define __CS_BULLET_SOFTBODY_H__

#include "bulletsystem.h"
#include "common2.h"
#include "physicalbody.h"
#include "updatable.h"

class btSoftBody;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

class csBulletSoftBody : public scfImplementationExt2<
  csBulletSoftBody, 
  csPhysicalBody,
  CS::Physics::iSoftBody,
  iUpdatable>
{
  friend class csBulletRigidBody;
  friend class csBulletJoint;
  friend class csBulletSector;
  friend class CollisionPortal;
  
  friend class BulletSoftRopeFactory;
  friend class BulletSoftClothFactory;
  friend class BulletSoftMeshFactory;

  using csPhysicalBody::SetEnabled;
  using csPhysicalBody::GetEnabled;
  using csPhysicalBody::GetLinearVelocity;
  using csPhysicalBody::SetLinearVelocity;

private:
  struct AnimatedAnchor
  {
    AnimatedAnchor (size_t vertexIndex, CS::Physics::iAnchorAnimationControl* controller)
      : vertexIndex (vertexIndex), controller (controller) {}

    size_t vertexIndex;
    csRef<CS::Physics::iAnchorAnimationControl> controller;
    btVector3 position;
  };

  btSoftBody* btBody;
  short anchorCount;
  csArray<AnimatedAnchor> animatedAnchors;
  bool gravityEnabled;

public:
  void CreateSoftBodyObject (BulletSoftBodyFactory* factory);

public:
  csBulletSoftBody (csBulletSystem* phySys, btSoftBody* body);
  virtual ~csBulletSoftBody ();

  //iCollisionObject
  virtual iObject* QueryObject () { return (iObject*) this; }

  virtual void SetAttachedCamera (iCamera* camera)
  { csBulletCollisionObject::SetAttachedCamera (camera); }
  virtual iCamera* GetAttachedCamera ()
  { return csBulletCollisionObject::GetAttachedCamera (); }

  virtual void RebuildObject ();

  virtual void SetTransform (const csOrthoTransform& trans);

  // btSoftBody don't use collision shape.
  virtual void AddCollider
    (CS::Collisions::iCollider* collider,
     const csOrthoTransform& relaTrans = csOrthoTransform (csMatrix3 (), csVector3 (0))) {}
  virtual void RemoveCollider (CS::Collisions::iCollider* collider) {}
  virtual void RemoveCollider (size_t index) {}

  virtual CS::Collisions::iCollider* GetCollider (size_t index) {return nullptr;}

  virtual CS::Collisions::HitBeamResult HitBeam (const csVector3& start, const csVector3& end);

  btSoftBody* GetBulletSoftPointer () {return btBody;}
  virtual bool RemoveBulletObject ();
  virtual bool AddBulletObject ();

  //iPhysicalBody

  virtual CS::Physics::PhysicalObjectType GetPhysicalObjectType () const
  { return CS::Physics::PHYSICAL_OBJECT_SOFTBODY; }
  virtual CS::Physics::iRigidBody* QueryRigidBody ()
  { return dynamic_cast<CS::Physics::iRigidBody*>(this); }
  virtual CS::Physics::iSoftBody* QuerySoftBody ()
  { return dynamic_cast<CS::Physics::iSoftBody*>(this); }

  virtual void AddForce (const csVector3& force);
  virtual void ApplyImpulse (const csVector3& impulse);
  virtual void ApplyImpulse (const csVector3& impulse, size_t vertexIndex);

  virtual void SetAngularVelocity (const csVector3& vel)
  { /* does nothing for now */ }
  virtual csVector3 GetAngularVelocity () const
  { /* does nothing for now */ return csVector3 (0, 0, 0); }

  virtual void SetMass (float mass);
  virtual float GetMass () const;

  virtual void SetFriction (float friction);
  virtual void SetDensity (float density);
  
  virtual float GetVolume () const;

  virtual void SetVertexMass (float mass, size_t index);
  virtual float GetVertexMass (size_t index);

  virtual size_t GetVertexCount ();
  virtual csVector3 GetVertexPosition (size_t index) const;

  virtual void AnchorVertex (size_t vertexIndex);
  virtual void AnchorVertex (size_t vertexIndex,
      CS::Physics::iRigidBody* body);
  virtual void AnchorVertex (size_t vertexIndex,
      CS::Physics::iAnchorAnimationControl* controller);

  virtual void UpdateAnchor (size_t vertexIndex,
      csVector3& position);
  virtual void RemoveAnchor (size_t vertexIndex);

  virtual csVector3 GetLinearVelocity () const;
  virtual void SetLinearVelocity (const csVector3& vel);
  
  virtual csVector3 GetLinearVelocity (size_t vertexIndex) const;
  virtual void SetLinearVelocity (size_t vertexIndex, const csVector3& velocity);

  virtual void SetWindVelocity (const csVector3& velocity);
  virtual const csVector3 GetWindVelocity () const;

  virtual void AddForce (const csVector3& force, size_t vertexIndex);

  virtual size_t GetTriangleCount ();

  virtual csTriangle GetTriangle (size_t index) const;

  virtual csVector3 GetVertexNormal (size_t index) const;

  //Bullet::iSoftBody

  virtual void DebugDraw (iView* rView);

  virtual void SetLinearStiffness (float stiffness);
  virtual void SetAngularStiffness (float stiffness);
  virtual void SetVolumeStiffness (float stiffness);

  virtual void ResetCollisionFlag ();

  virtual void SetClusterCollisionRS (bool cluster);
  virtual bool GetClusterCollisionRS ();
  virtual void SetClusterCollisionSS (bool cluster);
  virtual bool GetClusterCollisionSS ();

  virtual void SetSRHardness (float hardness);
  virtual void SetSKHardness (float hardness);
  virtual void SetSSHardness (float hardness);

  virtual void SetSRImpulse (float impulse);
  virtual void SetSKImpulse (float impulse);
  virtual void SetSSImpulse (float impulse);

  virtual void SetVeloCorrectionFactor (float factor);

  virtual void SetDamping (float damping);
  virtual void SetDrag (float drag);
  virtual void SetLift (float lift);
  virtual void SetPressure (float pressure);

  virtual void SetVolumeConversationCoefficient (float conversation);
  virtual void SetShapeMatchThreshold (float matching);

  virtual void SetRContactsHardness (float hardness);
  virtual void SetKContactsHardness (float hardness);
  virtual void SetSContactsHardness (float hardness);
  virtual void SetAnchorsHardness (float hardness);

  virtual void SetVeloSolverIterations (int iter);
  virtual void SetPositionIterations (int iter);
  virtual void SetDriftIterations (int iter);
  virtual void SetClusterIterations (int iter);

  virtual void SetShapeMatching (bool match);
  virtual void GenerateBendingConstraints (size_t distance);

  virtual void GenerateCluster (int iter);

  // Nodes
  virtual size_t GetNodeCount () const;

  /// Whether this object is affected by gravity
  virtual bool GetGravityEnabled () const { return gravityEnabled; }
  /// Whether this object is affected by gravity
  virtual void SetGravityEnabled (bool enabled) { gravityEnabled = enabled; }

  // iUpdatable
  /// Update the softbody before a simulation step
  virtual void PreStep (float dt);
  void UpdateAnchorInternalTick (btScalar timeStep);
  
  /// Update the softbody after a simulation step
  virtual void PostStep (float dt) {}

  /// We don't want the AddUpdatable method to add this object again when adding it as an updatable
  virtual CS::Collisions::iCollisionObject* GetCollisionObject ()
  { return nullptr; }

  /// Called when updatable is added to the given sector
  virtual void OnAdded (csBulletSector* sector) {}

  /// Called when updatable is removed from the given sector
  virtual void OnRemoved (csBulletSector* sector) {} 
};
}
CS_PLUGIN_NAMESPACE_END (Bullet2)
#endif
