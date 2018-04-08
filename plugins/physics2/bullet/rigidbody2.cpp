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

#include "cssysdef.h"
#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"

#include "iengine/scenenode.h"
#include "ivaria/collisions.h"
#include "rigidbody2.h"

#include "portal.h"

using namespace CS::Physics;
using namespace CS::Collisions;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
  csBulletRigidBody::csBulletRigidBody (BulletRigidBodyFactory* factory)
    : scfImplementationType (this, factory->system), factory (factory),
    btBody (nullptr), anchorCount (0)
  {
    collider = dynamic_cast<csBulletCollider*> (factory->GetCollider ());

    // TODO: handle no colliders

    // Build the bullet object
    btCollisionShape* shape = collider->GetOrCreateBulletShape ();
    btScalar mass = factory->GetMass ();//mass;
    btTransform trans = CSToBullet (factory->GetColliderTransform (), system->GetInternalScale ())
			 * collider->GetPrincipalAxisTransform ();
    // TODO: share the construction info at the factory level
    btRigidBody::btRigidBodyConstructionInfo infos
      // TODO motion states not needed if not dynamic
      (mass, CreateMotionState (trans), shape, collider->GetLocalInertia ());

    infos.m_friction = factory->GetFriction ();
    infos.m_restitution = factory->GetElasticity ();
    infos.m_linearDamping = factory->GetLinearDamping ();
    infos.m_angularDamping = factory->GetAngularDamping ();

    btObject = btBody = new btRigidBody (infos);
    btBody->setUserPointer (dynamic_cast<CS::Collisions::iCollisionObject*> (this));
    //btBody->setContactProcessingThreshold (1e18f);

    SetState (factory->state);

    group = dynamic_cast<CollisionGroup*> (factory->GetCollisionGroup ());
  }

  csBulletRigidBody::~csBulletRigidBody ()
  {
    delete motionState;
  }

  csBulletMotionState* csBulletRigidBody::CreateMotionState (const btTransform& trans)
  {
    return motionState = new csBulletMotionState
      (this, trans, CSToBullet (factory->GetColliderTransform (), system->GetInternalScale ())
       * collider->GetPrincipalAxisTransform ());
  }

  bool csBulletRigidBody::RemoveBulletObject ()
  {
    if (insideWorld)
    {
      if (anchorCount > 0)
      {
        // TODO: Fix this
        system->ReportWarning ("Cannot remove anchored body.\n");
        return false;
      }

      for (size_t i = 0; i < joints.GetSize (); i++)
        sector->RemoveJoint (joints[i]);

      sector->bulletWorld->removeRigidBody (btBody);
      insideWorld = false;

      if (portalData && portalData->Portal)
      {
        // Remove from portal
        portalData->Portal->RemoveTraversingObject (this);
      }

      return true;
    }
    return false;
  }

  bool csBulletRigidBody::AddBulletObject ()
  {
    // TODO: what's this method for?
    if (insideWorld)
      RemoveBulletObject ();
    
    sector->bulletWorld->addRigidBody (btBody, group->value, group->mask);

    insideWorld = true;
    return true;
  }

  void csBulletRigidBody::RebuildObject () 
  { 
    // See http://bulletphysics.org/Bullet/phpBB3/viewtopic.php?t=7988
    /**
     Quote:
     It could be that contact points are not updated because you changed the collision shape. 
     If you modify collision shapes, the best way is to remove the objects first, then make the modification, and re-insert it into the world. 
     Alternatively, you can manually flush the contact points for a given object using an API call, for example:
     world->getBroadphase ()->getOverlappingPairCache ()->cleanProxyFromPairs (object->getBroadphaseHandle (),world->getDispatcher ());
     */

    bool wasInWorld = insideWorld;
    if (insideWorld)
    {
      wasInWorld = true;
      RemoveBulletObject ();
    }
    
    // Create new object
    delete btObject;
    btObject = btBody = static_cast<btRigidBody*> (CreateBulletObject ());

    SetState (physicalState);

    if (wasInWorld)
    {
      AddBulletObject ();
    }
  }

  bool csBulletRigidBody::DoesGravityApply () const
  {
    return GetGravityEnabled () && btBody->getGravity ().length2 () > SMALL_EPSILON;
  }
  
  bool csBulletRigidBody::IsMovingUpward () const 
  { 
    // TODO: Consider scaling threshold by actor size or similar
    return btBody->getLinearVelocity ()[1] > 1.f * system->GetInternalScale (); 
  }

  void csBulletRigidBody::SetCollider (CS::Collisions::iCollider* collider)
  {
    csBulletCollisionObject::SetCollider (collider);
  }

  void csBulletRigidBody::SetMass (btScalar mass)
  {
    if (mass < SMALL_EPSILON)
      SetState (STATE_STATIC);

    else
      SetState (STATE_DYNAMIC);
  }
  
  void csBulletRigidBody::SetMassInternal (btScalar mass)
  {
    if (mass < SMALL_EPSILON)
    {
      btBody->setMassProps (0.0f, btVector3 (0.0f, 0.0f, 0.0f));
    }
    else
    {
      // we scale pre-computed inertia with actual mass
      btBody->setMassProps (mass, mass * collider->GetLocalInertia ());
    }
    btBody->updateInertiaTensor ();
  }

  void csBulletRigidBody::SetTransform (const csOrthoTransform& trans)
  {
    CS_ASSERT (btObject);
/*
    btTransform motionTransform;
    motionState->getWorldTransform (motionTransform);
    btTransform btTrans = motionTransform * motionState->inversePrincipalAxis;
*/
    btTransform btTrans = CSToBullet (trans, system->GetInternalScale ());
    
    if (insideWorld)
    {
      sector->bulletWorld->removeRigidBody (btRigidBody::upcast (btObject));
    }

    // Re-create the motion state
    btTransform principalAxis = motionState->inversePrincipalAxis.inverse ();
    delete motionState;
    CreateMotionState (btTrans * principalAxis);
    btBody->setMotionState (motionState);

    if (insideWorld)
    {
      sector->bulletWorld->addRigidBody (btRigidBody::upcast (btObject), group->value, group->mask);
    }
    
    iSceneNode* sceneNode = GetAttachedSceneNode ();
    if (sceneNode)
    {
      // Update movable
      sceneNode->GetMovable ()->SetFullTransform
	(BulletToCS (btTrans * motionState->inversePrincipalAxis, system->GetInverseInternalScale ()));
      sceneNode->GetMovable ()->UpdateMove ();
    }

    if (camera)
    {
      camera->SetTransform (BulletToCS (btTrans * motionState->inversePrincipalAxis, system->GetInverseInternalScale ()));
    }
  }

  csOrthoTransform csBulletRigidBody::GetTransform () const
  {
    btTransform transform;
    motionState->getWorldTransform (transform);
    return BulletToCS (transform * motionState->inversePrincipalAxis,
		       system->inverseInternalScale);
  }

  float csBulletRigidBody::GetMass () const
  {
    return 1.0f / btBody->getInvMass ();
  }

  void csBulletRigidBody::SetDensity (float density)
  {
    SetMass (density * collider->GetVolume ());
  }

  btScalar csBulletRigidBody::GetDensity () const
  {
    float volume = collider->GetVolume ();
    if (volume < SMALL_EPSILON) return 0.0f;
    return 1.0f / (btBody->getInvMass () * volume);
  }

  float csBulletRigidBody::GetVolume () const
  {
    return collider->GetVolume ();
  }

  float csBulletRigidBody::GetElasticity () 
  {
    return btBody->getRestitution ();
  }

  void csBulletRigidBody::SetElasticity (float elasticity)
  {
    btBody->setRestitution (elasticity);
  }

  void csBulletRigidBody::SetState (RigidBodyState state)
  {
    if (!collider->IsDynamic () && state == STATE_DYNAMIC)
    {
      state = STATE_STATIC;
    }
    if (physicalState != state || !insideWorld)
    {
      RigidBodyState previousState = physicalState;
      physicalState = state;

      if (!btBody) return;

      //  TODO: Clean this mess up
      if (insideWorld)
      {
        sector->bulletWorld->removeRigidBody (btBody);
      }

      btVector3 linearVelo = btBody->getInterpolationLinearVelocity ();
      btVector3 angularVelo = btBody->getInterpolationAngularVelocity ();

      if (previousState == STATE_KINEMATIC)
      {
        if (insideWorld)
        {
          // create new motion state
          btTransform trans;
          motionState->getWorldTransform (trans);
          trans *= motionState->inversePrincipalAxis;
          trans *= collider->GetPrincipalAxisTransform ();

          delete motionState;
          motionState = CreateMotionState (trans);
          btBody->setMotionState (motionState);
        }
      }

      switch (state)
      {
      case STATE_DYNAMIC:
        {
          btBody->setCollisionFlags (btBody->getCollisionFlags () & ~(btCollisionObject::CF_STATIC_OBJECT | btCollisionObject::CF_KINEMATIC_OBJECT));

          btBody->forceActivationState (ACTIVE_TAG);

          btBody->setLinearVelocity (linearVelo);
          btBody->setAngularVelocity (angularVelo);

          SetMassInternal (factory->mass);
          break;
        }
      case STATE_KINEMATIC:
        {
          btBody->setCollisionFlags (
            (btBody->getCollisionFlags ()
            | btCollisionObject::CF_KINEMATIC_OBJECT)
            & ~btCollisionObject::CF_STATIC_OBJECT);

          if (!kinematicCb)
          {
            kinematicCb.AttachNew (new csBulletDefaultKinematicCallback ());
          }

          // create new motion state
          btTransform principalAxis = motionState->inversePrincipalAxis.inverse ();
          btTransform trans;
          motionState->getWorldTransform (trans);
          delete motionState;
          motionState = new csBulletKinematicMotionState (this, trans, principalAxis);
          btBody->setMotionState (motionState);

          // set body kinematic
          btBody->setActivationState (DISABLE_DEACTIVATION);
          btBody->setInterpolationWorldTransform (btBody->getWorldTransform ());
          btBody->setInterpolationLinearVelocity (btVector3 (0.0f, 0.0f, 0.0f));
          btBody->setInterpolationAngularVelocity (btVector3 (0.0f, 0.0f, 0.0f));

          SetMassInternal (0);
          break;
        }
      case STATE_STATIC:
        {
          btBody->setCollisionFlags ((btBody->getCollisionFlags ()
            | btCollisionObject::CF_STATIC_OBJECT)
            & ~btCollisionObject::CF_KINEMATIC_OBJECT);
          btBody->setActivationState (ISLAND_SLEEPING);

          SetMassInternal (0);
          break;
        }
      }

      if (insideWorld)
      {
        sector->bulletWorld->addRigidBody (btBody);
      }

      if (state == STATE_DYNAMIC)
      {
        // Reactivate the body
        SetEnabled (true);
      }
    }
  }

  void csBulletRigidBody::SetLinearVelocity (const csVector3& vel)
  {
    if (!btBody || physicalState != STATE_DYNAMIC)
      return; 

    btBody->clearForces ();
    btBody->setLinearVelocity (CSToBullet (vel, system->GetInternalScale ()));
    btBody->activate ();
  }

  csVector3 csBulletRigidBody::GetLinearVelocity () const
  {
    const btVector3& vel = btBody->getLinearVelocity ();
    return BulletToCS (vel, system->GetInverseInternalScale ());
  }

  void csBulletRigidBody::SetAngularVelocity (const csVector3& vel)
  {
    if (!btBody || physicalState != STATE_DYNAMIC)
      return; 
    btBody->setAngularVelocity (CSToBullet (vel, 1));
    btBody->activate ();
  }

  csVector3 csBulletRigidBody::GetAngularVelocity () const
  {
    const btVector3& vel = btBody->getAngularVelocity ();
    return csVector3 (vel.getX (), vel.getY (), vel.getZ ());
  }

  void csBulletRigidBody::AddForce (const csVector3& force)
  {
    if (btBody)
    {
      btBody->applyCentralImpulse (CSToBullet (force, system->GetInternalScale ()));
      btBody->activate (true);
    }
  }

  void csBulletRigidBody::AddTorque (const csVector3& torque)
  {
    if (!btBody)
      return;

    // TODO: applyTorqueImpulse instead?
    btBody->applyTorque (CSToBullet (torque, system->GetInternalScale () * system->GetInternalScale ()));
    btBody->activate (true);
  }

  void csBulletRigidBody::AddRelForce (const csVector3& relForce)
  {
    if (!btBody)
      return;

    csOrthoTransform trans = GetTransform ();
    csVector3 force = trans.This2Other (relForce);
    btBody->applyCentralImpulse (CSToBullet (force, system->GetInternalScale ()));
    btBody->activate (true);
  }

  void csBulletRigidBody::AddRelTorque (const csVector3& torque)
  {
    if (!btBody)
      return; 

    csOrthoTransform trans = GetTransform ();
    csVector3 absTorque = trans.This2Other (torque);
    btBody->applyTorque (CSToBullet (absTorque, system->GetInternalScale () * system->GetInternalScale ()));
    btBody->activate (true);
  }

  void csBulletRigidBody::AddForceAtPos (const csVector3& force, const csVector3& pos)
  {
    if (!btBody)
      return; 

    btVector3 btForce (CSToBullet (force, system->GetInternalScale ()));
    btVector3 btPos (CSToBullet (pos, system->GetInternalScale ()));
    btVector3 btRelPos = btPos - btBody->getCenterOfMassPosition ();

    btBody->applyImpulse (btForce, btRelPos);
    btBody->activate (true);
  }

  void csBulletRigidBody::AddForceAtRelPos (const csVector3& force, const csVector3& relPos)
  {
    if (!btBody)
      return;

    csOrthoTransform trans = GetTransform ();
    btVector3 btForce (CSToBullet (force, system->GetInternalScale ()));
    csVector3 pos = trans.This2Other (relPos);
    btVector3 btPos (CSToBullet (pos, system->GetInternalScale ()));
    btVector3 btRelPos = btPos - btBody->getCenterOfMassPosition ();

    btBody->applyImpulse (btForce, btRelPos);
    btBody->activate (true);
  }

  void csBulletRigidBody::AddRelForceAtPos (const csVector3& relForce, const csVector3& pos)
  {
    if (!btBody)
      return; 
    
    csOrthoTransform trans = GetTransform ();
    btVector3 btForce = (CSToBullet (trans.This2Other (relForce), system->GetInternalScale ()));
    btVector3 btPos (CSToBullet (pos, system->GetInternalScale ()));
    btVector3 btRelPos = btPos - btBody->getCenterOfMassPosition ();

    btBody->applyImpulse (btForce, btRelPos);
    btBody->activate (true);
  }

  void csBulletRigidBody::AddRelForceAtRelPos (const csVector3& relForce, const csVector3& relPos)
  {
    if (!btBody)
      return; 
    
    csOrthoTransform trans = GetTransform ();
    btVector3 btForce = (CSToBullet (trans.This2Other (relForce), system->GetInternalScale ()));
    btVector3 btPos (CSToBullet (trans.This2Other (relPos), system->GetInternalScale ()));
    btVector3 btRelPos = btPos - btBody->getCenterOfMassPosition ();

    btBody->applyImpulse (btForce, btRelPos);
    btBody->activate (true);
  }

  void csBulletRigidBody::ApplyImpulse (const csVector3& impulse)
  {
    if (!btBody) return;

    btBody->applyCentralImpulse (CSToBullet (impulse, system->GetInternalScale ()));
    btBody->activate (true);
  }

  void csBulletRigidBody::ApplyImpulse (const csVector3& impulse, const csVector3& position)
  {
    if (!btBody) return; 

    btVector3 btImpulse (CSToBullet (impulse, system->GetInternalScale ()));
    btVector3 btPosition (CSToBullet (position, system->GetInternalScale ()));
    btVector3 btRelativePosition = btPosition - btBody->getCenterOfMassPosition ();

    btBody->applyImpulse (btImpulse, btRelativePosition);
    btBody->activate (true);
  }

  void csBulletRigidBody::ApplyTorque (const csVector3& torque)
  {
    if (!btBody) return;

    // TODO: applyTorqueImpulse instead?
    btBody->applyTorque (CSToBullet (torque, system->GetInternalScale () * system->GetInternalScale ()));
    btBody->activate (true);
  }

  csVector3 csBulletRigidBody::GetTotalForce () const
  {
    if (!btBody) return csVector3 (0.0f);

    return BulletToCS (btBody->getTotalForce (), system->GetInverseInternalScale ());
  }

  csVector3 csBulletRigidBody::GetTotalTorque () const
  {
    if (!btBody) return csVector3 (0.0f);

    return BulletToCS (btBody->getTotalTorque (), system->GetInverseInternalScale () * system->GetInverseInternalScale ());
  }

  float csBulletRigidBody::GetLinearDamping () 
  {
    return btBody->getLinearDamping ();
  }

  void csBulletRigidBody::SetLinearDamping (float d)
  {
    btBody->setDamping (d, GetAngularDamping ());
  }

  float csBulletRigidBody::GetAngularDamping ()
  {
    return btBody->getAngularDamping ();
  }

  void csBulletRigidBody::SetAngularDamping (float d)
  {
    btBody->setDamping (GetLinearDamping (), d);
  }

  csVector3 csBulletRigidBody::GetAngularFactor () const { return BulletToCS (btBody->getAngularFactor (), system->GetInverseInternalScale ()); }
  void csBulletRigidBody::SetAngularFactor (const csVector3& f) { btBody->setAngularFactor (CSToBullet (f, system->GetInternalScale ())); }

  btCollisionObject* csBulletRigidBody::CreateBulletObject ()
  {
    btTransform trans;
    motionState->getWorldTransform (trans);
    trans *= motionState->inversePrincipalAxis;
    trans *= CSToBullet (factory->GetColliderTransform (), system->GetInternalScale ());
    trans *= collider->GetPrincipalAxisTransform ();
    
    btCollisionShape* shape = collider->GetOrCreateBulletShape ();
    btScalar mass = GetMass ();

    btRigidBody::btRigidBodyConstructionInfo infos (mass, CreateMotionState (trans), shape, mass * collider->GetLocalInertia ());

    infos.m_friction = GetFriction ();
    infos.m_restitution = GetElasticity ();
    infos.m_linearDamping = GetLinearDamping ();
    infos.m_angularDamping = GetAngularDamping ();

    // Create a new rigid body
    btRigidBody* body = new btRigidBody (infos);

    body->setLinearVelocity (btBody->getLinearVelocity ());
    body->setAngularVelocity (btBody->getAngularVelocity ());
    body->setAngularFactor (btBody->getAngularFactor ());
    //clone->btBody->setLinearFactor (GetLinearFactor ());

    return body;
  }

  csPtr<iCollisionObject> csBulletRigidBody::CloneObject ()
  {
    csBulletRigidBody* clone = new csBulletRigidBody (factory);
    clone->collider = collider;
    clone->group = group;

    btTransform trans;
    trans.setIdentity ();
    clone->CreateMotionState (trans);

    clone->btObject = clone->btBody = static_cast<btRigidBody*> (CreateBulletObject ());
    clone->btBody->setUserPointer (dynamic_cast<CS::Collisions::iCollisionObject*> (this));

    clone->SetState (physicalState);

    // Wake both up
    SetEnabled (true);
    clone->SetEnabled (true);

    return csPtr<iCollisionObject> (clone);
  }

  bool csBulletRigidBody::GetGravityEnabled () const
  {
    return !(btBody->getFlags () & BT_DISABLE_WORLD_GRAVITY);
  }

  void csBulletRigidBody::SetGravityEnabled (bool enabled)
  {
    int flags = btBody->getFlags ();
    if (enabled)
    {
      flags &= ~BT_DISABLE_WORLD_GRAVITY;
    }
    else
    {
      flags |= BT_DISABLE_WORLD_GRAVITY;
    }
    btBody->setFlags (flags);
  }


  // ##########################################################################################
  // csBulletDefaultKinematicCallback implementation

  csBulletDefaultKinematicCallback::csBulletDefaultKinematicCallback ()
    : scfImplementationType (this)
  {
  }

  csBulletDefaultKinematicCallback::~csBulletDefaultKinematicCallback ()
  {
  }

  void csBulletDefaultKinematicCallback::GetBodyTransform
    (iRigidBody* body, csOrthoTransform& transform) const
  {

    csBulletRigidBody* rigBody = dynamic_cast<csBulletRigidBody*> (body);
    iSceneNode* sceneNode = rigBody->GetAttachedSceneNode ();
    if (sceneNode)
    {
      transform = sceneNode->GetMovable ()->GetFullTransform ();
      return;
    }
    iCamera* camera = rigBody->GetAttachedCamera ();
    if (camera)
    {
      transform = camera->GetTransform ();
      return;
    }
  }

}
CS_PLUGIN_NAMESPACE_END (Bullet2)
