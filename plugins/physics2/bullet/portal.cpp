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

#include "iengine/portal.h"
#include "iengine/portalcontainer.h"
#include "iengine/scenenode.h"
#include "csgeom/sphere.h"

#include "ivaria/collisions.h"

#include "csutil/custom_new_disable.h"

#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"

#include "csutil/custom_new_enable.h"

//#include "common2.h"
#include "portal.h"
#include "rigidbody2.h"
#include "softbody2.h"
#include "joint2.h"

using namespace CS::Collisions;
using namespace CS::Physics;

// TODO: Add new sector to sector mesh's sector list

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
  void SoftBodySynchronizer::SetupSynchronizer (csBulletSoftBody* cloneObj)
  {
    // Store all initial node velocities of the clone
    btSoftBody* btCloneObj = cloneObj->GetBulletSoftPointer ();
    for (size_t i = 0; i < cloneObj->GetNodeCount (); ++i)
    {
      oldCloneVels.Push (btCloneObj->m_nodes[i].m_v);
    }
  }

  CollisionPortal::CollisionPortal
    (iPortal* portal, const csOrthoTransform& meshTrans, csBulletSector* sourceSector)
    : portal (portal), sourceSector (sourceSector)
  {
    // Set the sector
    targetSector = dynamic_cast<csBulletSector*>
      (sourceSector->system->FindCollisionSector (portal->GetSector ()));

    // Create a ghost collisder for the portal
    ghostPortal = new btGhostObject ();

    // compute the size of the portal
    // WARNING: GetWorldVertices () initializes the portal plane - Always call this before GetObjectPlane ()!
    const csVector3* vert = portal->GetWorldVertices ();
    csVector3 portalPos (0);
    csBox3 box;
    for (int i = 0; i < portal->GetVertexIndicesCount (); i++)
    {
      csVector3 v (vert[portal->GetVertexIndices ()[i]]);
      portalPos += v;
      box.AddBoundingVertex (v);
    }

    portalPos /= portal->GetVertexIndicesCount ();

    // Compute the warp transform of the portal
    // TODO: This is not quite correct
    warpTrans = portal->GetWarp ().GetInverse ();
    warpTrans = meshTrans.GetInverse () * warpTrans * meshTrans;
    csVector3 normal = portal->GetObjectPlane ().GetNormal ();
    normal = meshTrans.GetT2O () * normal;              // rotate normal

    float maxEdge = csMax (box.GetSize ().x, box.GetSize ().y);
    maxEdge = csMax (maxEdge, box.GetSize ().z);
    float thresh = maxEdge * 0.05f;
    
    // place the portal at the center of the box that represents it
    csVector3 size = 0.499f * box.GetSize ();           // compute "flat" box size
    size = meshTrans.GetT2O () * size;                   // rotate box
    size += normal * thresh;                            // add thickness to box
    csVector3 centerDist = (size * normal) * normal;    // move origin into the center of the box
    
    csOrthoTransform realTrans;                         // no rotation necessary, since we already rotated everything
    realTrans.SetOrigin (portalPos + meshTrans.GetOrigin () + centerDist);
    ghostPortal->setWorldTransform (CSToBullet (realTrans, sourceSector->system->GetInternalScale ()));

    // give the portal it's shape and add it to the world
    btCollisionShape* shape = new btBoxShape (CSToBullet (size, sourceSector->system->GetInternalScale ()));

    ghostPortal->setCollisionShape (shape);
    ghostPortal->setCollisionFlags (ghostPortal->getCollisionFlags () | btCollisionObject::CF_NO_CONTACT_RESPONSE);
    
    sourceSector->bulletWorld->addCollisionObject (ghostPortal);
/*
    sourceSector->bulletWorld->addCollisionObject
      (ghostPortal,
       sourceSector->system->collGroups[CollisionGroupTypePortal].value,
       sourceSector->system->collGroups[CollisionGroupTypePortal].mask);
*/
  }

  CollisionPortal::~CollisionPortal ()
  {
    if (ghostPortal) 
    {
      delete ghostPortal->getCollisionShape ();
      delete ghostPortal;
    }
  }

  bool CollisionPortal::CanTraverse (csBulletCollisionObject* obj)
  {
    if (obj->IsPassive ())
    {
      // Clones and other passive objects cannot traverse portals
      return false;
    }

    // Only actors and dynamic objects can traverse portals
    return obj->GetObjectType () == COLLISION_OBJECT_ACTOR
      || (obj->GetObjectType () == COLLISION_OBJECT_PHYSICAL
	  && obj->QueryPhysicalBody ()->IsDynamic ());
  }
  
  bool CollisionPortal::IsOnOtherSide (csBulletCollisionObject* obj)
  {
    // Check if it has traversed the portal
    csOrthoTransform transform = obj->GetTransform ();
    csVector3 newPosition = transform.GetOrigin ();
    //csVector3 oldPosition = oldTrans[index].GetOrigin ();
    csVector3 orien1 = newPosition - portal->GetWorldSphere ().GetCenter ();
    //csVector3 orien2 = oldPosition - portal->GetWorldSphere ().GetCenter ();
    float a = orien1 * portal->GetWorldPlane ().GetNormal ();
    //float b = orien2 * portal->GetWorldPlane ().GetNormal ();
    return a > 0;
  }

  void CollisionPortal::Traverse (csBulletCollisionObject* obj)
  {
    // Remove object from Portal:
    RemoveTraversingObject (obj);

    // Warp transformation
    csOrthoTransform transform = obj->GetTransform () * warpTrans;
    if (targetSector != sourceSector)
    {
      // Move the body to new sector
      csRef<csBulletCollisionObject> safeRef (obj);  // This line demonstrates why passing native pointers fails.
      sourceSector->RemoveCollisionObject (obj);
      targetSector->AddCollisionObject (obj);
    }
    if (obj->QueryPhysicalBody () && obj->QueryPhysicalBody ()->QueryRigidBody ())
    {
      // Warp velocities
      iPhysicalBody* pb = obj->QueryPhysicalBody ();
      csBulletRigidBody* rb = dynamic_cast<csBulletRigidBody*> (pb->QueryRigidBody ());
      rb->SetLinearVelocity (warpTrans.GetT2O () * rb->GetLinearVelocity ());
      rb->SetAngularVelocity (warpTrans.GetT2O () * rb->GetAngularVelocity ()); 

      // TODO: Also warp force
      //rb->setfo (warpTrans.GetT2O () * rb->GetForce ()); 
    }
    obj->SetTransform (transform);
  }

  void CollisionPortal::UpdateCollisionsPreStep (csBulletSector* sector)
  {
    CS_ASSERT (sourceSector == sector);
    if (!targetSector)
    {
      // Lookup targetSector if the portal uses lazy sector lookup
      targetSector = dynamic_cast<csBulletSector*>
	(sourceSector->system->FindCollisionSector (portal->GetSector ()));

      if (!targetSector)
      {
        //  Problem: Sector might not be retreived before first rendering of portal
        //CS_ASSERT (!"targetSector not set in portal");
      }
    }
    
    // Iterate over all currently intersecting objects and update collision data
    for (int i = 0; i < ghostPortal->getNumOverlappingObjects (); ++i)
    {
      btCollisionObject* btObj = ghostPortal->getOverlappingObject (i);
      iCollisionObject* iObj = static_cast<iCollisionObject*> (btObj->getUserPointer ());
      csBulletCollisionObject* obj = dynamic_cast<csBulletCollisionObject*> (iObj);

      // Check if object can traverse
      if (!obj ||                         // not a valid object
        obj->GetSector () != sector ||     // object not updated yet
        !CanTraverse (obj))                // not a valid object
      {
        continue;
      }

      // If object is already past the mid-point of the portal, it must be ported immediately
      if (IsOnOtherSide (obj)) 
      {
        Traverse (obj);
#ifdef _DEBUG
        csPrintf (">>>> Obj traversed portal 0x%lx: 0x%lx\n", this, obj);
#endif
        continue;
      }

      // Object is in process of traversal:
      // Make sure that we already have or can create a clone
      csRef<csBulletCollisionObject> cloneObj;
      if (!obj->portalData || !obj->portalData->OtherObject)
      {
        csRef<iCollisionObject> newObj = obj->ClonePassivePortalObject ();

        cloneObj = csRef<csBulletCollisionObject>(dynamic_cast<csBulletCollisionObject*>(&*newObj));

        if (!cloneObj) continue;   // Cannot produce a clone
#ifdef CS_DEBUG
        cloneObj->QueryObject ()->SetName ("Clone of " + csString (obj->QueryObject ()->GetName ()));
#endif
      }
      else
      {
        cloneObj = obj->portalData->OtherObject;
      }


      // Start with data verification and consistency:
      PortalTraversalData* data = obj->portalData;

      size_t objIndex = objects.Find (obj);
      bool isNew = objIndex == csArrayItemNotFound;
      if (isNew)
      {
        // New object: Must not already be referencing this portal
        CS_ASSERT (data == nullptr || data->Portal != this);
      }
      else
      {
        // Traversing Object:
        //CS_ASSERT (data != nullptr && data->Portal == this);

        // remove from objects array (will be re-added later)
        objects.DeleteIndex (objIndex);
      }

      if (data == nullptr)
      {
        // Create new Portal Data container
        obj->portalData = data = new PortalTraversalData (true, this, obj);
      }
      else
      {
        // Set portal
        data->Portal = this;
      }

      // Set clone object
      if (!data->OtherObject)
      {
        data->OtherObject = cloneObj;
      }

      // TODO: Fix warps and also apply them here, correctly

      // At this point, data is all setup correctly
      if (isNew)
      {
#ifdef _DEBUG
        csPrintf (">>>> Obj setup at portal 0x%lx: 0x%lx\n", this, obj);
#endif

        // Add the scene node to the engine sector on the other side of the portal
        iSceneNode* node = obj->GetAttachedSceneNode ();
        if (node)
        {
          node->GetMovable ()->GetSectors ()->Add (targetSector->GetSector ());
          node->GetMovable ()->UpdateMove ();
        }

        // Setup clone object for first-time use:

        // Never put to sleep
        cloneObj->SetDeactivable (false);

        // Disable gravity (TODO: Fix fail OOP)
        if (cloneObj->QueryPhysicalBody ())
        {
          cloneObj->QueryPhysicalBody ()->SetGravityEnabled (false);
        }
/*
        else if (cloneObj->QueryActor ())
        {
          cloneObj->QueryActor ()->SetGravityEnabled (false);
        }
*/
        // No collisions with static objects
	// TODO: why not?
	// TODO: this is a hack with heavy side effects
/*
        CollisionGroup collGroup (cloneObj->GetCollisionGroup ());
        collGroup.mask &= ~CollisionGroupMaskValueStatic;
        cloneObj->SetCollisionGroup (collGroup);
*/
        // Add clone
        targetSector->AddCollisionObject (cloneObj);

        // Setup synchronizer
        if (data->Synchronizer)
        {
          delete data->Synchronizer;
          data->Synchronizer = nullptr;
        }
        if (obj->QueryPhysicalBody ())
        {
          if (obj->QueryPhysicalBody ()->QueryRigidBody ())
          {
            data->Synchronizer = new RigidBodySynchronizer ();
          }
          else if (obj->QueryPhysicalBody ()->QuerySoftBody ())
          {
            SoftBodySynchronizer* syncer = new SoftBodySynchronizer ();
            data->Synchronizer = syncer;
            syncer->SetupSynchronizer (dynamic_cast<csBulletSoftBody*>(cloneObj->QueryPhysicalBody ()->QuerySoftBody ()));
          }
        }
      }
      else
      {
        // NOTE: The object should not have been removed, moved or tempered with in any way other than by the solver
        CS_ASSERT (cloneObj->GetSector () == targetSector);

        // Synchronize the two objects
        if (data->Synchronizer)
        {
          Synchronize (obj, cloneObj);
        }
      }
    }

    // Update object book-keeping:

    // Remove all objects that are not touching the portal anymore, from the portal
    for (size_t i = 0; i < objects.GetSize (); ++i)
    {
      csRef<csBulletCollisionObject> oldObj = objects.Get (i);
      if (IsTraversingThisPortal (oldObj))
      {
        RemoveTraversingObject (oldObj);
      }
    }
    objects.DeleteAll ();      // clear array

    // re-add all existing objects:
    for (int i = 0; i < ghostPortal->getNumOverlappingObjects (); i++)
    {
      btCollisionObject* btObj = ghostPortal->getOverlappingObject (i);
      iCollisionObject* iObj = static_cast<iCollisionObject*> (btObj->getUserPointer ());
      csBulletCollisionObject* obj = dynamic_cast<csBulletCollisionObject*> (iObj);
      
      if (obj && IsTraversingThisPortal (obj))
      {
        objects.Push (obj);
      }
    }
  }
  
  /// Synchronize the two symbiotic objects and correct position, velocity and force
  void CollisionPortal::Synchronize (csBulletCollisionObject* obj, csBulletCollisionObject* cloneObj)
  {
    csOrthoTransform objTrans (obj->GetTransform ());
    csOrthoTransform cloneTrans (cloneObj->GetTransform ());
    
    //const csVector3& pos = objTrans.GetOrigin ();
    //const csVector3& clonePos = cloneTrans.GetOrigin ();

    if (obj->QueryPhysicalBody ())
    {
      iPhysicalBody* ipObj = obj->QueryPhysicalBody ();
      iPhysicalBody* ipCloneObj = cloneObj->QueryPhysicalBody ();
      CS_ASSERT (ipCloneObj);
      
      csPhysicalBody* pObj = dynamic_cast<csPhysicalBody*>(ipObj);
      csPhysicalBody* pCloneObj = dynamic_cast<csPhysicalBody*>(ipCloneObj);

      if (pObj->QuerySoftBody ())
      {
        // Soft Body:
        CS_ASSERT (pCloneObj->QuerySoftBody ());
        csBulletSoftBody* sObj = dynamic_cast<csBulletSoftBody*>(pObj->QuerySoftBody ());
        csBulletSoftBody* sCloneObj = dynamic_cast<csBulletSoftBody*>(pCloneObj->QuerySoftBody ());
        CS_ASSERT (sObj && sCloneObj);
        CS_ASSERT (sObj->GetNodeCount () == sCloneObj->GetNodeCount ());
        
        // Update velocities per node
        SoftBodySynchronizer& traverser = *(SoftBodySynchronizer*)&obj->portalData->Synchronizer;
        traverser.Synchronize (sObj, sCloneObj);
      }
      else
      {
        // Rigid Body:
        csBulletRigidBody* rObj = dynamic_cast<csBulletRigidBody*>(ipObj);
        csBulletRigidBody* rCloneObj = dynamic_cast<csBulletRigidBody*>(ipCloneObj);
        CS_ASSERT (rObj && rCloneObj);
        
        // Update CoM velocities
        RigidBodySynchronizer& traverser = *(RigidBodySynchronizer*)&obj->portalData->Synchronizer;
        traverser.Synchronize (rObj, rCloneObj);
      }
    }
  }

  void RigidBodySynchronizer::Synchronize (csBulletRigidBody* obj, csBulletRigidBody* cloneObj)
  {
    btRigidBody* btObj = obj->GetBulletRigidPointer ();
    btRigidBody* btCloneObj = cloneObj->GetBulletRigidPointer ();

#if (CS_BULLET_VERSION == 278)
    // Sync both rigid bodies
    btVector3 deltaVelLin = btCloneObj->getDeltaLinearVelocity ();
    btVector3 deltaVelAng = btCloneObj->getDeltaAngularVelocity ();
    
    btVector3 velLin = btObj->getLinearVelocity () + deltaVelLin;
    btVector3 velAng = btObj->getAngularVelocity () + deltaVelAng;

    btObj->setLinearVelocity (velLin);
    btObj->setAngularVelocity (velAng);
    btCloneObj->setLinearVelocity (velLin);
    btCloneObj->setAngularVelocity (velAng);
#else
    // TODO: real synchronization
    // Maybe it can be done by forwarding the collision contacts into both objects?

    // Sync the rigid body velocities
    btCloneObj->setLinearVelocity (btObj->getLinearVelocity ());
    btCloneObj->setAngularVelocity (btObj->getAngularVelocity ());
#endif
    
    // Sync the transforms
    cloneObj->SetTransform (obj->GetTransform ());
  }

  void SoftBodySynchronizer::Synchronize (csBulletSoftBody* obj, csBulletSoftBody* cloneObj)
  {
    btSoftBody* btObj = obj->GetBulletSoftPointer ();
    btSoftBody* btCloneObj = cloneObj->GetBulletSoftPointer ();
    
    // Soft body state must not be changed during traversal
    CS_ASSERT (oldCloneVels.GetSize () == obj->GetNodeCount ());

    // Sync every single node
    for (size_t i = 0; i < obj->GetNodeCount (); ++i)
    {
      btVector3 deltaPos = btCloneObj->m_nodes[i].m_x - btCloneObj->m_nodes[i].m_q;
      btVector3 deltaVel = btCloneObj->m_nodes[i].m_v - oldCloneVels.Get (i);

      btVector3 newPos = btObj->m_nodes[i].m_x + deltaPos;
      btVector3 newVel = btObj->m_nodes[i].m_v + deltaVel;

      // set node velocity
      btObj->m_nodes[i].m_v = btCloneObj->m_nodes[i].m_v = newVel;

      // set node position
      btObj->m_nodes[i].m_x = btCloneObj->m_nodes[i].m_x = newPos;

      // set old node velocity
      oldCloneVels.Put (i, btCloneObj->m_nodes[i].m_v);
    }

    // Sync the transforms
    cloneObj->SetTransform (obj->GetTransform ());
  }

  void CollisionPortal::RemoveFromSector ()
  {
    for (size_t j = 0; j < objects.GetSize (); j++)
    {
      if (objects[j]->portalData)
      {
        RemoveTraversingObject (objects[j]);
      }
    }
    objects.DeleteAll ();
    sourceSector->GetBulletWorld ()->removeCollisionObject (ghostPortal);
  }

  void CollisionPortal::RemoveTraversingObject (csBulletCollisionObject* obj)
  {
    // Can only remove original objects
    CS_ASSERT (!obj->IsPassive ());

    // Unset portal data and remove clone
    if (obj->portalData)
    {
      obj->portalData->Portal = nullptr;
      if (obj->portalData->OtherObject)
      {
        iSceneNode* node = obj->GetAttachedSceneNode ();
        if (node)
        {
          // Remove mesh from second sector
          for (size_t i = 0; node->GetMovable ()->GetSectors ()->GetCount () > 1;)
          {
            iSector* sec = node->GetMovable ()->GetSectors ()->Get (int (i));
            if (sec != obj->GetSector ()->GetSector ())
            {
              //sec->GetMeshes ()->Remove (node->QueryMesh ());
              node->GetMovable ()->GetSectors ()->Remove (sec);
              node->GetMovable ()->UpdateMove ();
            }
            else
            {
              ++i;
            }
          }
        }

        // Remove clone from CollisionSector
        targetSector->RemoveCollisionObject (obj->portalData->OtherObject);
        obj->portalData->OtherObject = nullptr;
      }
    }
  }

}
CS_PLUGIN_NAMESPACE_END (Bullet2)
