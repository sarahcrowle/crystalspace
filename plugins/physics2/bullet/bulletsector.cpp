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

#include "cssysdef.h"
#include "imesh/softanim.h"
#include "imesh/animesh.h"
#include "iengine/scenenode.h"
#include "iengine/movable.h"
#include "csgeom/sphere.h"
#include "csgeom/tri.h"
#include "imesh/genmesh.h"
#include "imesh/object.h"
#include "csutil/sysfunc.h"
#include "iutil/objreg.h"
#include "ivaria/view.h"
#include "ivaria/collisions.h"

#include "igeom/trimesh.h"
#include "iengine/portal.h"
#include "iengine/portalcontainer.h"

#include "csutil/custom_new_disable.h"

#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"
#include "BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h"
#include "BulletSoftBody/btSoftBody.h"
#include "BulletSoftBody/btSoftRigidDynamicsWorld.h"
#include "BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"

#include "csutil/custom_new_enable.h"

#include "bulletsystem.h"
#include "colliderprimitives.h"
#include "collisionactor.h"
#include "collisiondata.h"
#include "common2.h"
#include "joint2.h"
#include "portal.h"
#include "rigidbody2.h"
#include "softbody2.h"
#include "vehicle.h"

const float COLLISION_THRESHOLD = 0.01f;

#define WORLD_AABB_DIMENSIONS 10000.0f

using namespace CS::Collisions;
using namespace CS::Physics;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
  //----------------------------------- PointContactResult's -----------------------------------

  struct PointContactResultSingle : public btCollisionWorld::ContactResultCallback
  {
    csRef<CollisionData> data;
    csBulletSystem* system;

    PointContactResultSingle (csBulletSystem* system)
      : system (system) 
    {
    }

#if (CS_BULLET_VERSION >= 281)
    virtual btScalar addSingleResult (btManifoldPoint& cp,
				      const btCollisionObjectWrapper* colObj0, int partId0, int index0,
				      const btCollisionObjectWrapper* colObj1, int partId1, int index1)
    {
      if (!data)
      {
	data.AttachNew (new CollisionData ());
	data->objectA = static_cast<CS::Collisions::iCollisionObject*>
	  (colObj0->getCollisionObject ()->getUserPointer ());
	data->objectB = static_cast<CS::Collisions::iCollisionObject*>
	  (colObj1->getCollisionObject ()->getUserPointer ());
      }

      csRef<CollisionContact> contact;
      contact.AttachNew (new CollisionContact ());
      contact->positionOnA = BulletToCS (cp.m_positionWorldOnA, system->GetInverseInternalScale ());
      contact->positionOnB = BulletToCS (cp.m_positionWorldOnB, system->GetInverseInternalScale ());
      contact->normalOnB = BulletToCS (cp.m_normalWorldOnB, system->GetInverseInternalScale ());
      contact->penetration = cp.m_distance1 * system->GetInverseInternalScale ();
      data->contacts.Push (contact);

      return 0;
    }
#else
    virtual btScalar addSingleResult (btManifoldPoint& cp, const btCollisionObject* colObj0,
				      int partId0, int index0, const btCollisionObject* colObj1,
				      int partId1, int index1)
    {
      if (!data)
      {
	data.AttachNew (new CollisionData ());
	data->objectA = static_cast<CS::Collisions::iCollisionObject*> (colObj0->getUserPointer ());
	data->objectB = static_cast<CS::Collisions::iCollisionObject*> (colObj1->getUserPointer ());
      }

      csRef<CollisionContact> contact;
      contact.AttachNew (new CollisionContact ());
      contact->positionOnA = BulletToCS (cp.m_positionWorldOnA, system->GetInverseInternalScale ());
      contact->positionOnB = BulletToCS (cp.m_positionWorldOnB, system->GetInverseInternalScale ());
      contact->normalOnB = BulletToCS (cp.m_normalWorldOnB, system->GetInverseInternalScale ());
      contact->penetration = cp.m_distance1 * system->GetInverseInternalScale ();
      data->contacts.Push (contact);

      return 0;
    }
#endif
  };

  struct PointContactResultMulti : public btCollisionWorld::ContactResultCallback
  {
    csRef<CollisionDataList> collisions;
    csBulletSystem* system;

    PointContactResultMulti (csBulletSystem* system)
      : system (system) 
    {
      collisions.AttachNew (new CollisionDataList ());
    }

#if (CS_BULLET_VERSION >= 281)
    virtual btScalar addSingleResult (btManifoldPoint& cp,
				      const btCollisionObjectWrapper* colObj0, int partId0, int index0,
				      const btCollisionObjectWrapper* colObj1, int partId1, int index1)
    {
      csRef<CollisionData> data;
      data.AttachNew (new CollisionData ());
      data->objectA = static_cast<CS::Collisions::iCollisionObject*>
	(colObj0->getCollisionObject ()->getUserPointer ());
      data->objectB = static_cast<CS::Collisions::iCollisionObject*>
	(colObj1->getCollisionObject ()->getUserPointer ());

      csRef<CollisionContact> contact;
      contact.AttachNew (new CollisionContact ());
      contact->positionOnA = BulletToCS (cp.m_positionWorldOnA, system->GetInverseInternalScale ());
      contact->positionOnB = BulletToCS (cp.m_positionWorldOnB, system->GetInverseInternalScale ());
      contact->normalOnB = BulletToCS (cp.m_normalWorldOnB, system->GetInverseInternalScale ());
      contact->penetration = cp.m_distance1 * system->GetInverseInternalScale ();

      data->contacts.Push (contact);
      collisions->collisions.Push (data);

      return 0;
    }
#else
    virtual btScalar addSingleResult (btManifoldPoint& cp, const btCollisionObject* colObj0,
				      int partId0, int index0, const btCollisionObject* colObj1,
				      int partId1, int index1)
    {
      csRef<CollisionData> data;
      data.AttachNew (new CollisionData ());
      data->objectA = static_cast<CS::Collisions::iCollisionObject*> (colObj0->getUserPointer ());
      data->objectB = static_cast<CS::Collisions::iCollisionObject*> (colObj1->getUserPointer ());

      csRef<CollisionContact> contact;
      contact.AttachNew (new CollisionContact ());
      contact->positionOnA = BulletToCS (cp.m_positionWorldOnA, system->GetInverseInternalScale ());
      contact->positionOnB = BulletToCS (cp.m_positionWorldOnB, system->GetInverseInternalScale ());
      contact->normalOnB = BulletToCS (cp.m_normalWorldOnB, system->GetInverseInternalScale ());
      contact->penetration = cp.m_distance1 * system->GetInverseInternalScale ();

      data->contacts.Push (contact);
      collisions->collisions.Push (data);

      return 0;
    }
#endif
  };

  //----------------------------------- csBulletSector -----------------------------------

  void PreTickCallback (btDynamicsWorld* world, btScalar timeStep)
  {
    csBulletSector* sector = (csBulletSector*) (world->getWorldUserInfo ());
    sector->UpdateSoftBodies (timeStep);
  }

  csBulletSector::csBulletSector (csBulletSystem* system)
    : scfImplementationType (this), system (system), softWorldInfo (nullptr),
    linearDampening (system->linearDampening), angularDampening (system->angularDampening),
    linearDisableThreshold (system->linearDisableThreshold),
    angularDisableThreshold (system->angularDisableThreshold),
    timeDisableThreshold (system->timeDisableThreshold)
  {
    solver = new btSequentialImpulseConstraintSolver ();

    // TODO: those dimensions will not fit every worlds
    const int maxProxies = 32766;
    btVector3 worldAabbMin (-WORLD_AABB_DIMENSIONS, -WORLD_AABB_DIMENSIONS, -WORLD_AABB_DIMENSIONS);
    btVector3 worldAabbMax (WORLD_AABB_DIMENSIONS, WORLD_AABB_DIMENSIONS, WORLD_AABB_DIMENSIONS);
    broadphase = new btAxisSweep3 (worldAabbMin, worldAabbMax, maxProxies);
    broadphase->getOverlappingPairCache ()->setInternalGhostPairCallback (new btGhostPairCallback ());

    csVector3 v (0.0f, -9.81f, 0.0f);
    btVector3 gravity = CSToBullet (v, system->GetInternalScale ());

    if (system->isSoftWorld)
    {
      configuration = new btSoftBodyRigidBodyCollisionConfiguration ();
      dispatcher = new btCollisionDispatcher (configuration);
      bulletWorld = new btSoftRigidDynamicsWorld
        (dispatcher, broadphase, solver, configuration);

      softWorldInfo = new btSoftBodyWorldInfo ();
      softWorldInfo->m_broadphase = broadphase;
      softWorldInfo->m_dispatcher = dispatcher;
      softWorldInfo->m_gravity = gravity;
      softWorldInfo->air_density = 1.2f;
      softWorldInfo->water_density = 0.0f;
      softWorldInfo->water_offset = 0.0f;
      softWorldInfo->water_normal = btVector3 (0.0f, 1.0f, 0.0f);
      softWorldInfo->m_sparsesdf.Initialize ();
    }
    else
    {
      configuration = new btDefaultCollisionConfiguration ();
      dispatcher = new btCollisionDispatcher (configuration);
      bulletWorld = new btDiscreteDynamicsWorld
        (dispatcher, broadphase, solver, configuration);
    }

    if (system->GetDynamicConcaveEnabled ())
      btGImpactCollisionAlgorithm::registerAlgorithm (dispatcher);

    btContactSolverInfo& info = bulletWorld->getSolverInfo ();
    info.m_numIterations = system->stepIterations;

    bulletWorld->setGravity (gravity);

    if (system->debugDraw)
      bulletWorld->setDebugDrawer (system->debugDraw);

    // Register a pre-tick callback
    // TODO: remove?
    if (system->isSoftWorld)
      bulletWorld->setInternalTickCallback (PreTickCallback, this, true);
  }

  csBulletSector::~csBulletSector ()
  {
    DeleteAll ();

    delete bulletWorld;
    delete dispatcher;
    delete configuration;
    delete solver;
    delete softWorldInfo;
    delete broadphase;
  }

  void csBulletSector::DeleteAll ()
  {
    // remove updatables
    while (updatables.GetSize ())
    {
      RemoveUpdatable (updatables[updatables.GetSize () - 1]);
    }

    // remove actors
    for (size_t i = 0; i < actors.GetSize (); ++i)
    {
      actors[i]->RemoveBulletObject ();
    }

    // remove portals
    for (size_t i = 0; i < portals.GetSize (); ++i)
    {
      bulletWorld->removeCollisionObject (portals[i]->ghostPortal);
    }

    // remove collision objects
    for (size_t i = 0; i < collisionObjects.GetSize (); ++i)
    {
      collisionObjects[i]->RemoveBulletObject ();
    }

    // remove constraints
    for (size_t i = 0; i < joints.GetSize (); ++i)
    {
      joints[i]->RemoveBulletJoint ();
    }

    joints.DeleteAll ();
    actors.DeleteAll ();
    softBodies.DeleteAll ();
    rigidBodies.DeleteAll ();
    collisionObjects.DeleteAll ();
    portals.DeleteAll ();
  }

  void csBulletSector::SetGravity (const csVector3& v)
  {
    // first re-activate all objects
    for (size_t i = 0; i < collisionObjects.GetSize (); ++i)
    {
      if (collisionObjects[i]->GetObjectType () == COLLISION_OBJECT_PHYSICAL
	  && collisionObjects[i]->QueryPhysicalBody ()->IsDynamic ())
      {
        collisionObjects[i]->btObject->activate (true);
      }
    }

    btVector3 gravity = CSToBullet (v, system->GetInternalScale ());
    bulletWorld->setGravity (gravity);

    if (softWorldInfo)
      softWorldInfo->m_gravity = gravity;

    // Update the gravity of the actors
    for (size_t i = 0; i < actors.GetSize (); i++)
      actors[i]->SetGravity (gravity[1]);
  }

  csVector3 csBulletSector::GetGravity () const
  {
    btVector3 gravity = bulletWorld->getGravity ();
    return BulletToCS (gravity, system->GetInverseInternalScale ());
  }

  void csBulletSector::SetSector (iSector* sector)
  {
    if (this->sector == sector) return;
    
    this->sector = sector;

    // Update the engine sector of the attached objects
    for (size_t i = 0; i < collisionObjects.GetSize (); i++)
    {
      iCollisionObject* object = collisionObjects[i];
      if (object->GetAttachedSceneNode ())
	object->GetAttachedSceneNode ()->GetMovable ()->SetSector (sector);
      if (object->GetAttachedCamera ())
	object->GetAttachedCamera ()->SetSector (sector);
    }
  }

  void csBulletSector::SetLinearDamping (float d)
  {
    linearDampening = d;
  }

  void csBulletSector::SetAngularDamping (float d)
  {
    angularDampening = d;
  }

  void csBulletSector::SetAutoDisableParams (float linear, float angular,
    float time)
  {
    linearDisableThreshold = linear;
    angularDisableThreshold = angular;
    timeDisableThreshold = time;
  }

  void csBulletSector::Step (float duration)
  {
    // TODO: notify some stepping callback

    // Call updatable pre-steps
    for (size_t i = 0; i < updatables.GetSize (); i++)
      updatables[i]->PreStep (duration);

    // Update traversing objects before simulation
    UpdateCollisionPortalsPreStep ();

    // Step the simulation
    bulletWorld->stepSimulation (duration, system->worldMaxSteps, system->worldTimeStep);
    
    // Update traversing objects after simulation
    UpdateCollisionPortalsPostStep ();

    // Check for collisions
    //CheckCollisions ();

    // Call updatable post-steps
    for (size_t i = 0; i < updatables.GetSize (); i++)
      updatables[i]->PostStep (duration);
  }

  void csBulletSector::UpdateCollisionPortalsPreStep ()
  {
    for (size_t i = 0; i < portals.GetSize (); i++)
    {
      portals[i]->UpdateCollisionsPreStep (this);
    }
  }

  void csBulletSector::UpdateCollisionPortalsPostStep ()
  {
    for (size_t i = 0; i < portals.GetSize (); i++)
    {
      portals[i]->UpdateCollisionsPostStep (this);
    }
  }

  void csBulletSector::UpdateSoftBodies (float timeStep)
  {
    for (csWeakRefArray<csBulletSoftBody>::Iterator it = anchoredSoftBodies.GetIterator (); it.HasNext (); )
    {
      csBulletSoftBody* body = static_cast<csBulletSoftBody*> (it.Next ());
      body->UpdateAnchorInternalTick (timeStep);
    }
  }

  void csBulletSector::AddRigidBody (CS::Physics::iRigidBody* body)
  {
    csRef<csBulletRigidBody> bulletBody (dynamic_cast<csBulletRigidBody*> (body));
    rigidBodies.Push (bulletBody);

    bulletBody->sector = this;

    // TODO: Body might have set its own damping
    bulletBody->SetLinearDamping (linearDampening);
    bulletBody->SetAngularDamping (angularDampening);
    bulletBody->btBody->setSleepingThresholds (linearDisableThreshold, angularDisableThreshold);
    bulletBody->btBody->setDeactivationTime (timeDisableThreshold);
    bulletBody->AddBulletObject ();
  }

  CS::Physics::iRigidBody* csBulletSector::GetRigidBody (size_t index)
  {
    return rigidBodies[index]->QueryRigidBody ();
  }

  void csBulletSector::AddSoftBody (CS::Physics::iSoftBody* body)
  {
    CS_ASSERT (system->isSoftWorld);

    csRef<csBulletSoftBody> btBody (dynamic_cast<csBulletSoftBody*> (body));
    softBodies.Push (btBody);
    AddUpdatable (btBody);

    btBody->sector = this;
    btBody->AddBulletObject ();

    // Update the reference to the soft body hold by the animation controller of the attached mesh
    // TODO: manage that from the animation controller?
    iSceneNode* sceneNode = body->GetAttachedSceneNode ();
    if (sceneNode)
    {
      iMeshWrapper* mesh = sceneNode->QueryMesh ();
      if (!mesh) return;

      csRef<iGeneralMeshState> meshState =
        scfQueryInterface<iGeneralMeshState> (mesh->GetMeshObject ());
      if (!meshState) return;

      iGenMeshAnimationControl* animationControl = meshState->GetAnimationControl ();
      if (!animationControl) return;

      csRef<CS::Animation::iSoftBodyAnimationControl2> softAnimationControl =
        scfQueryInterface<CS::Animation::iSoftBodyAnimationControl2> (animationControl);
      if (!softAnimationControl) return;

      if (!softAnimationControl->GetSoftBody ())
        softAnimationControl->SetSoftBody (body);
    }
  }

  CS::Physics::iSoftBody* csBulletSector::GetSoftBody (size_t index)
  {
    return softBodies[index]->QuerySoftBody ();
  }

  void csBulletSector::AddJoint (CS::Physics::iJoint* joint)
  {
    // TODO: all objects need to be removed from their previous sector
    csBulletJoint* csJoint = dynamic_cast<csBulletJoint*> (joint);
    CS_ASSERT (csJoint);
    csJoint->sector = this;
    csJoint->AddBulletJoint ();
    joints.Push (csJoint);
  }

  void csBulletSector::RemoveJoint (CS::Physics::iJoint* joint)
  {
    csBulletJoint* csJoint = dynamic_cast<csBulletJoint*> (joint);
    CS_ASSERT (csJoint);

    // re-activate the now free bodies
    if (csJoint->bodies[0])
    {
      dynamic_cast<csBulletCollisionObject*> (&*csJoint->bodies[0])->btObject->activate (true);
    }
    if (csJoint->bodies[1])
    {
      dynamic_cast<csBulletCollisionObject*> (&*csJoint->bodies[1])->btObject->activate (true);
    }

    csJoint->RemoveBulletJoint ();
    joints.Delete (csJoint);
  }

  void csBulletSector::AddCollisionObject (CS::Collisions::iCollisionObject* object)
  {
    if (object->GetSector ())
    {
      object->GetSector ()->RemoveCollisionObject (object);
    }

    csBulletCollisionObject* obj (dynamic_cast<csBulletCollisionObject*> (object));

#ifdef CS_DEBUG
    // TODO: unused
    //printf ("Adding object \"%s\" (0x%lx) to sector: 0x%lx\n", object->QueryObject ()->GetName (), obj->btObject, this);
#endif

    switch (obj->GetObjectType ())
    {
    case CS::Collisions::COLLISION_OBJECT_ACTOR:
      {
	csRef<BulletCollisionActor> actor (dynamic_cast<BulletCollisionActor*> (obj));
	actors.Push (actor);
	actor->sector = this;
	actor->AddBulletObject ();
       }
      break;
    case CS::Collisions::COLLISION_OBJECT_PHYSICAL:
      {
        iPhysicalBody* phyBody = obj->QueryPhysicalBody ();

        if (phyBody->QueryRigidBody ())
        {
          AddRigidBody (phyBody->QueryRigidBody ());
        }
        else
        {
	  CS_ASSERT (system->isSoftWorld);
          AddSoftBody (phyBody->QuerySoftBody ());
        }
      }
      break;
    default:
      {
        // Ghost objects
        obj->sector = this;
        //obj->btObject->setCollisionFlags (obj->btObject->getCollisionFlags () | btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT);

        obj->AddBulletObject ();
      }
      break;
    }

    // Update the engine sector of the attached objects
    if (object->GetAttachedSceneNode () && sector)
    {
      object->GetAttachedSceneNode ()->GetMovable ()->SetSector (sector);
      object->GetAttachedSceneNode ()->GetMovable ()->UpdateMove ();
    }
    if (object->GetAttachedCamera () && sector)
      object->GetAttachedCamera ()->SetSector (sector);

    // Add all objects to the collisionObjects list
    collisionObjects.Push (obj);
  }

  void csBulletSector::RemoveCollisionObject (CS::Collisions::iCollisionObject* object)
  {
    csBulletCollisionObject* collObject = dynamic_cast<csBulletCollisionObject*> (object);
    if (!collObject)
      return;

    bool removed = collObject->RemoveBulletObject ();
    if (removed)
    {
      collisionObjects.Delete (collObject);

      // Update the engine sector of the attached objects
      if (object->GetAttachedSceneNode () && sector)
      {
	object->GetAttachedSceneNode ()->GetMovable ()->SetSector (nullptr);
	object->GetAttachedSceneNode ()->GetMovable ()->UpdateMove ();
      }
      if (object->GetAttachedCamera ())
	object->GetAttachedCamera ()->SetSector (nullptr);

      if (collObject->GetObjectType () == CS::Collisions::COLLISION_OBJECT_ACTOR)
	actors.Delete (static_cast<BulletCollisionActor*> (collObject));

      iPhysicalBody* phyBody = dynamic_cast<iPhysicalBody*> (object);
      if (phyBody)
      {
	if (phyBody->QueryRigidBody ())
	  rigidBodies.Delete (dynamic_cast<csBulletRigidBody*> (phyBody->QueryRigidBody ()));

	else
	{
	  csBulletSoftBody* btBody = dynamic_cast<csBulletSoftBody*> (phyBody->QuerySoftBody ());
	  softBodies.Delete (btBody);
	  RemoveUpdatable (btBody);
	}
      }
    }
  }

  CS::Collisions::iCollisionObject* csBulletSector::GetCollisionObject (size_t index)
  {
    if (index >= 0 && index < collisionObjects.GetSize ())
    {
      return collisionObjects[index];
    }
    else
    {
      return nullptr;
    }
  }

  void csBulletSector::AddPortal (iPortal* portal, const csOrthoTransform& meshTrans)
  {
    CollisionPortal* newPortal = new CollisionPortal (portal, meshTrans, this);
    portals.Push (newPortal);
  }

  void csBulletSector::RemovePortal (iPortal* portal)
  {
    for (size_t i = 0; i < portals.GetSize (); i++)
    {
      if (portals[i]->portal == portal)
      {
        portals[i]->RemoveFromSector ();
        portals.DeleteIndexFast (i);
        return;
      }
    }
  }

  void csBulletSector::AddVehicle (CS::Physics::iVehicle* vehicle)
  {
    csRef<iUpdatable> u = scfQueryInterface<iUpdatable> (vehicle);
    AddUpdatable (u);
  }
  
  void csBulletSector::RemoveVehicle (CS::Physics::iVehicle* vehicle)
  {
    csRef<iUpdatable> u = scfQueryInterface<iUpdatable> (vehicle);
    RemoveUpdatable (u);
  }

  void csBulletSector::AddUpdatable (iUpdatable* u)
  {
    updatables.Push (u);

    if (u->GetCollisionObject ())
    {
      AddCollisionObject (u->GetCollisionObject ());
    }

    csRef<BulletActionWrapper> wrapper = scfQueryInterface<BulletActionWrapper> (u);
    if (wrapper && wrapper->GetBulletAction ())
    {
      // It was an internal action that also defines a bullet action
      bulletWorld->addAction (wrapper->GetBulletAction ());
    }

    u->OnAdded (this);
  }

  void csBulletSector::RemoveUpdatable (iUpdatable* u)
  {
    if (u->GetCollisionObject ())
    {
      RemoveCollisionObject (u->GetCollisionObject ());
    }

    csRef<BulletActionWrapper> wrapper = scfQueryInterface<BulletActionWrapper> (u);
    if (wrapper && wrapper->GetBulletAction ())
    {
      bulletWorld->removeAction (wrapper->GetBulletAction ());
    }
    
    u->OnRemoved (this);

    updatables.Delete (u);   // delete last because that might be the last hard reference
  }

  CS::Collisions::HitBeamResult csBulletSector::HitBeam
    (const csVector3& start, const csVector3& end) const
  {
    CS::Collisions::HitBeamResult result;
    HitBeam (start, end, result);
    return result;
  }

  CS::Collisions::HitBeamResult csBulletSector::HitBeamPortals
    (const csVector3& start, const csVector3& end) const
  {
    // TODO: warp transform the end vector when crossing a portal

    CS::Collisions::HitBeamResult result;
    const btGhostObject* hitPortal = HitBeam (start, end, result);

    if (result.object == nullptr && hitPortal)
    {
      //Portals are not included.
      for (size_t i = 0; i < portals.GetSize (); i++)
      {
        if (portals[i]->ghostPortal == hitPortal)
        {
          if (!portals[i]->portal->CompleteSector (0))
          {
            result.hasHit = false;
            return result;
          }
          else
          {
            /// Something wrong with the transform.
            csOrthoTransform warpWor;
            csVector3 newStart = result.isect;
            csVector3 newEnd = end;
            if (portals[i]->portal->GetFlags ().Check (CS_PORTAL_WARP))
            {
              newStart = portals[i]->warpTrans.This2Other (newStart);
              newEnd = portals[i]->warpTrans.This2Other (newEnd);
            }

            result = portals[i]->targetSector->HitBeamPortal (newStart, newEnd);
            return result;
          }
        }
      }

    }

    return result;
  }

  const btGhostObject* csBulletSector::HitBeam (const csVector3& start, const csVector3& end,
						CS::Collisions::HitBeamResult& result) const
  {
    btVector3 rayFrom = CSToBullet (start, system->GetInternalScale ());
    btVector3 rayTo = CSToBullet (end, system->GetInternalScale ());

    btCollisionWorld::ClosestRayResultCallback rayCallback (rayFrom, rayTo);
    bulletWorld->rayTest (rayFrom, rayTo, rayCallback);

    if (rayCallback.hasHit ())
    {
      CS::Collisions::iCollisionObject* collObject = static_cast<CS::Collisions::iCollisionObject*>
	(rayCallback.m_collisionObject->getUserPointer ());

      result.hasHit = true;

      if (rayCallback.m_collisionObject->getInternalType () == btCollisionObject::CO_GHOST_OBJECT
	  && rayCallback.m_collisionObject->getUserPointer () == nullptr)
      {
        // hit a ghost object (potentially a portal...)
        collObject = nullptr;
        result.hasHit = false;
        const btGhostObject* hitPortal = btGhostObject::upcast (rayCallback.m_collisionObject);
        if (hitPortal)
        {
          // this might be of interest to the caller
          result.object = collObject;
          result.isect = BulletToCS (rayCallback.m_hitPointWorld,
            system->GetInverseInternalScale ());
          result.normal = BulletToCS (rayCallback.m_hitNormalWorld,
            system->GetInverseInternalScale ());
	  return hitPortal;
        }
      }

      else if (rayCallback.m_collisionObject->getInternalType () == btCollisionObject::CO_SOFT_BODY)
      {
        // hit a soft body
        const btSoftBody* body = btSoftBody::upcast (rayCallback.m_collisionObject);

	result.object = collObject;
	result.isect = BulletToCS (rayCallback.m_hitPointWorld,
				   system->GetInverseInternalScale ());
	result.normal = BulletToCS (rayCallback.m_hitNormalWorld,
				    system->GetInverseInternalScale ());	

	// Search the index of the closest vertex being hit
	float closestDistance = 10000.f;
	for (int i = 0; i < body->m_nodes.size (); i++)
	{
	  const btSoftBody::Node& node = body->m_nodes[i];
	  float distance = (node.m_x - rayCallback.m_hitPointWorld).length2 ();
	  if (distance < closestDistance)
	  {
	    closestDistance = distance;
	    result.vertexIndex = i;
	  }
	}
      } //softBody

      else
      { 
        // hit a terrain or a "normal" object
        result.object = collObject;
        result.isect = BulletToCS (rayCallback.m_hitPointWorld,
          system->GetInverseInternalScale ());
        result.normal = BulletToCS (rayCallback.m_hitNormalWorld,
          system->GetInverseInternalScale ());
        return nullptr;
      } // not softBody
    } //has hit

    return nullptr;
  }

  CS::Collisions::HitBeamResult csBulletSector::RigidHitBeam
    (btCollisionObject* object, const csVector3& start, const csVector3& end) const
  {
    btVector3 rayFrom = CSToBullet (start, system->GetInternalScale ());
    btVector3 rayTo = CSToBullet (end, system->GetInternalScale ());

    CS::Collisions::HitBeamResult result;

    if (object->getInternalType () == btCollisionObject::CO_SOFT_BODY)
    {
      btSoftBody* body = btSoftBody::upcast (object);
      btSoftBody::sRayCast ray;

      if (body->rayTest (rayFrom, rayTo, ray))
      {
        result.hasHit = true;
        result.object = static_cast<csBulletCollisionObject*> (object->getUserPointer ());

        btVector3 impact = rayFrom + (rayTo - rayFrom) * ray.fraction;
        switch (ray.feature)
        {
        case btSoftBody::eFeature::Face:
          {
            btSoftBody::Face& face = body->m_faces[ray.index];
            btSoftBody::Node* node = face.m_n[0];
            float distance = (node->m_x - impact).length2 ();

            for (int i = 1; i < 3; i++)
            {
              float nodeDistance = (face.m_n[i]->m_x - impact).length2 ();
              if (nodeDistance < distance)
              {
                node = face.m_n[i];
                distance = nodeDistance;
              }
            }

            result.isect = BulletToCS (node->m_x,
              system->GetInverseInternalScale ());
            result.normal = BulletToCS (node->m_n,
              system->GetInverseInternalScale ());	
            result.vertexIndex = size_t (node - &body->m_nodes[0]);
            break;
          }
        default:
          break;
        }
        return result;
      }
    }

    else
    {
      btCollisionWorld::ClosestRayResultCallback rayCallback (rayFrom, rayTo);

      //Ghost Object part?

      btTransform	rayFromTrans;
      btTransform	rayToTrans;

      rayFromTrans.setIdentity ();
      rayFromTrans.setOrigin (rayFrom);
      rayToTrans.setIdentity ();
      rayToTrans.setOrigin (rayTo);

      bulletWorld->rayTestSingle (rayFromTrans, rayToTrans, object,
        object->getCollisionShape (),
        object->getWorldTransform (),
        rayCallback);

      if (rayCallback.hasHit ())
      {
        result.hasHit = true;
        result.object = static_cast<csBulletCollisionObject*> (object->getUserPointer ());
        result.isect = BulletToCS (rayCallback.m_hitPointWorld,
          system->GetInverseInternalScale ());
        result.normal = BulletToCS (rayCallback.m_hitNormalWorld,
          system->GetInverseInternalScale ());
        return result;
      }
    }
    return result;
  }

  csPtr<CS::Collisions::iCollisionDataList> csBulletSector::CollisionTest
    (CS::Collisions::iCollisionObject* object)
  {
    if (!object) return csPtr<CS::Collisions::iCollisionDataList> (nullptr);

    csBulletCollisionObject* collObject = dynamic_cast<csBulletCollisionObject*> (object);
    if (collObject->IsPassive ()) return csPtr<CS::Collisions::iCollisionDataList> (nullptr);

    csRef<CollisionDataList> collisions;

    if (collObject->GetObjectType () == COLLISION_OBJECT_PHYSICAL)
    {
      PointContactResultMulti result (system);
      bulletWorld->contactTest (collObject->btObject, result);
      collisions = result.collisions;
      CS_ASSERT (collisions);
    }

    else
    {
      btPairCachingGhostObject* ghost = static_cast<btPairCachingGhostObject*>
	(btGhostObject::upcast (collObject->btObject));

      bulletWorld->getDispatcher ()->dispatchAllCollisionPairs
	(ghost->getOverlappingPairCache (), bulletWorld->getDispatchInfo (),
	 bulletWorld->getDispatcher ());

      for (int i = 0; i < ghost->getOverlappingPairCache ()->getNumOverlappingPairs (); i++)
      {
        btManifoldArray manifoldArray;
        btBroadphasePair* collisionPair =
	  &ghost->getOverlappingPairCache ()->getOverlappingPairArray ()[i];

        if (collisionPair->m_algorithm)
          collisionPair->m_algorithm->getAllContactManifolds (manifoldArray);

        for (int j = 0; j < manifoldArray.size (); j++)
        {
          btPersistentManifold* manifold = manifoldArray[j];
          const btCollisionObject* objA =
	    static_cast<const btCollisionObject*> (manifold->getBody0 ());
          const btCollisionObject* objB =
	    static_cast<const btCollisionObject*> (manifold->getBody1 ());
          CS::Collisions::iCollisionObject* csCOA =
	    static_cast<CS::Collisions::iCollisionObject*> (objA->getUserPointer ());
          CS::Collisions::iCollisionObject* csCOB =
	    static_cast<CS::Collisions::iCollisionObject*> (objB->getUserPointer ());
          for (int p=0;p<manifold->getNumContacts ();p++)
          {
	    csRef<CollisionData> data;
	    data.AttachNew (new CollisionData ());
            data->objectA = csCOA;
            data->objectB = csCOB;

            const btManifoldPoint& pt = manifold->getContactPoint (p);
	    csRef<CollisionContact> contact;
	    contact.AttachNew (new CollisionContact ());
            contact->positionOnA = BulletToCS (pt.m_positionWorldOnA,
					      system->GetInverseInternalScale ());
            contact->positionOnB = BulletToCS (pt.m_positionWorldOnB,
					      system->GetInverseInternalScale ());
            contact->normalOnB = BulletToCS (pt.m_normalWorldOnB,
					    system->GetInverseInternalScale ());
            contact->penetration = pt.m_distance1 * system->GetInverseInternalScale ();

	    data->contacts.Push (contact);
	    collisions->collisions.Push (data);
          }
        }
      }
    }

    if (collObject->GetPortalData () && collObject->GetPortalData ()->OtherObject)
    {
      // The object is traversing a portal and thus has a clone that is in symbiosis with itself
      // TODO: objects colliding through both side of the portal will be added twice
      csBulletCollisionObject* portalClone = collObject->GetPortalData ()->OtherObject;
      csRef<CS::Collisions::iCollisionDataList> portalCollisions =
	portalClone->sector->CollisionTest (portalClone);

      for (size_t i = 0; i < portalCollisions->GetCollisionCount (); i++)
      {
	CS::Collisions::iCollisionData* collision = portalCollisions->GetCollision (i);
	csRef<CollisionData> data;
	data.AttachNew (new CollisionData ());

	data->objectA = object;
	data->objectB = collision->GetObjectB ();

        if (collision->GetObjectA () == portalClone)
        {
	  for (size_t j = 0; j < collision->GetContactCount (); j++)
	  {
	    iCollisionContact* portalContact = collision->GetContact (j);

	    csRef<CollisionContact> contact;
	    contact.AttachNew (new CollisionContact ());

	    csVector3 vec = portalClone->GetTransform ().Other2This (portalContact->GetPositionOnA ());
	    contact->positionOnA = collObject->GetTransform ().This2Other (vec);
	    // What's the position of the other object? Still in the other side of the portal?
	    contact->positionOnB = portalContact->GetPositionOnB ();
	    vec = portalClone->GetTransform ().Other2ThisRelative (portalContact->GetNormalOnB ());
	    contact->normalOnB = collObject->GetTransform ().This2OtherRelative (vec);
	    contact->penetration = portalContact->GetPenetration ();
	    data->contacts.Push (contact);
	  }
        }
        else
        {
	  for (size_t j = 0; j < collision->GetContactCount (); j++)
	  {
	    iCollisionContact* portalContact = collision->GetContact (j);

	    csRef<CollisionContact> contact;
	    contact.AttachNew (new CollisionContact ());

	    csVector3 vec = portalClone->GetTransform ().Other2This (portalContact->GetPositionOnB ());
	    contact->positionOnB = collObject->GetTransform ().This2Other (vec);
	    // What's the position of the other object? Still in the other side of the portal?
	    contact->positionOnA = portalContact->GetPositionOnA ();
	    vec = portalClone->GetTransform ().Other2ThisRelative (portalContact->GetNormalOnB ());
	    contact->normalOnB = collObject->GetTransform ().This2OtherRelative (vec);
	    contact->penetration = portalContact->GetPenetration ();
	    data->contacts.Push (contact);
	  }
	}

	collisions->collisions.Push (data);
      }
    }

    return collisions;
  }

  csPtr<CS::Collisions::iCollisionData> csBulletSector::BulletCollide
    (btCollisionObject* objectA, btCollisionObject* objectB)
  {
    PointContactResultSingle result (system);
    bulletWorld->contactPairTest (objectA, objectB, result);
    return result.data;
  }

  void csBulletSector::CheckCollisions ()
  {
    int numManifolds = bulletWorld->getDispatcher ()->getNumManifolds ();

    for (size_t i = 0; i < collisionObjects.GetSize (); i++)
      collisionObjects[i]->contactObjects.Empty ();

    // Could not get contacted softBody?
    for (int i = 0; i < numManifolds; i++)
    {
      btPersistentManifold* contactManifold =
        bulletWorld->getDispatcher ()->getManifoldByIndexInternal (i);
      if (contactManifold->getNumContacts ())
      {
        const btCollisionObject* obA =
          static_cast<const btCollisionObject*> (contactManifold->getBody0 ());
        const btCollisionObject* obB =
          static_cast<const btCollisionObject*> (contactManifold->getBody1 ());

        csBulletCollisionObject* csCOA = dynamic_cast <csBulletCollisionObject*>
	  (static_cast<CS::Collisions::iCollisionObject*> (obA->getUserPointer ()));
        csBulletCollisionObject* csCOB = dynamic_cast <csBulletCollisionObject*>
	  (static_cast<CS::Collisions::iCollisionObject*> (obB->getUserPointer ()));

        if (csCOA && csCOA->GetObjectType () == COLLISION_OBJECT_PHYSICAL)
          if (csCOA->contactObjects.Contains (csCOB) == csArrayItemNotFound)
            csCOA->contactObjects.Push (csCOB);

        if (csCOB && csCOB->GetObjectType () == COLLISION_OBJECT_PHYSICAL)
          if (csCOB->contactObjects.Contains (csCOA) == csArrayItemNotFound)
            csCOB->contactObjects.Push (csCOA);
      }
    }
  }

  bool csBulletSector::Save (const char* filename)
  {
    // Check that the version of the Bullet library can handle serialization.
#ifndef CS_HAVE_BULLET_SERIALIZER
    return false;
#else

    // create a large enough buffer
    int maxSerializeBufferSize = 1024 * 1024 * 5;

    btDefaultSerializer* serializer = new btDefaultSerializer (maxSerializeBufferSize);
    bulletWorld->serialize (serializer);

    FILE* file = fopen (filename,"wb");
    if (!file) return false;

    if (fwrite (serializer->getBufferPointer (), serializer->getCurrentBufferSize (), 1, file)
      != 1)
    {
      fclose (file);
      return false;
    }

    if (fclose (file) == EOF) return false;

    return true;

#endif
  }

}
CS_PLUGIN_NAMESPACE_END (Bullet2)
