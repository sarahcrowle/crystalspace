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
#include "csgeom/sphere.h"
#include "csgeom/tri.h"
#include "csutil/sysfunc.h"
#include "iengine/movable.h"
#include "iengine/portal.h"
#include "iengine/portalcontainer.h"
#include "iengine/scenenode.h"
#include "igeom/trimesh.h"
#include "imesh/animesh.h"
#include "imesh/genmesh.h"
#include "imesh/object.h"
#include "imesh/objmodel.h"
#include "iutil/evdefs.h"
#include "iutil/objreg.h"
#include "ivaria/view.h"

#include "csutil/custom_new_disable.h"

#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"
#include "BulletSoftBody/btSoftBody.h"
#include "BulletSoftBody/btSoftRigidDynamicsWorld.h"
#include "BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"

#include "csutil/custom_new_enable.h"

#include "common2.h"
#include "colliderprimitives.h"
#include "collisionterrain.h"
#include "rigidbody2.h"
#include "softbody2.h"
#include "collisionactor.h"
#include "joint2.h"

#include "vehicle.h"

using namespace CS::Collisions;
using namespace CS::Physics;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

SCF_IMPLEMENT_FACTORY (csBulletSystem)

//--------------------------------- CollisionGroup ---------------------------------

CollisionGroup::CollisionGroup (const char* name, char index)
  : scfImplementationType (this), name (name), index (index)
{
  value = 1 << index;
  //mask = ~value;
  mask = -1;
}

void CollisionGroup::SetCollisionEnabled (iCollisionGroup* other, bool enabled)
{
  CollisionGroup* group = dynamic_cast<CollisionGroup*> (other);
  if (enabled)
  {
    mask |= 1 << group->index;
    group->mask |= 1 << index;
  }
  else
  {
    mask &= ~(1 << group->index);
    group->mask &= ~(1 << index);
  }
}

bool CollisionGroup::GetCollisionEnabled (iCollisionGroup* other)
{
  CollisionGroup* group = dynamic_cast<CollisionGroup*> (other);
  return mask & (1 << group->index);
}

//--------------------------------- csBulletSystem ---------------------------------

csBulletSystem::csBulletSystem (iBase* iParent)
  : scfImplementationType (this, iParent),
  internalScale (1.0f), inverseInternalScale (1.0f),
  simulationSpeed (1.0f), worldTimeStep (1.0f / 60.0f), worldMaxSteps (1),
  stepIterations (10), isSoftWorld (true), concaveEnabled (false),
  linearDampening (0.1f), angularDampening (0.1f),
  linearDisableThreshold (0.8f), angularDisableThreshold (1.0f),
  timeDisableThreshold (0.0f), debugDraw (nullptr)
{
  defaultInfo = new btSoftBodyWorldInfo;

  // Create the default collision group
  csRef<CollisionGroup> group;
  group.AttachNew (new CollisionGroup ("Default", 0));
  defaultGroup = collisionGroups.Put ("Default", group);
}

csBulletSystem::~csBulletSystem ()
{
  collSectors.DeleteAll ();
  collisionGroups.DeleteAll ();
  delete debugDraw;
  // TODO: unregister

  // Unregister from the event queue
  if (eventQueue)
    eventQueue->RemoveListener (this);
}

bool csBulletSystem::Initialize (iObjectRegistry* object_reg)
{
  this->object_reg = object_reg;

  // If there are no current collision system registered to the object
  // registry, then register itself to it
  csRef<CS::Collisions::iCollisionSystem> collisionSystem =
    csQueryRegistry<CS::Collisions::iCollisionSystem> (object_reg);
  if (!collisionSystem)
    object_reg->Register (this, "CS::Collisions::iCollisionSystem");

  // Find references to the engine objects
  vc = csQueryRegistry<iVirtualClock> (object_reg);
  if (!vc) return ReportError ("Failed to locate virtual clock!");

  eventQueue = csQueryRegistry<iEventQueue> (object_reg);
  if (!eventQueue) return ReportError ("Failed to locate event queue!");

  // Register to the event queue
  csEventID events[2] = { csevFrame (object_reg), CS_EVENTLIST_END };
  eventQueue->RegisterListener (this, events);

  return true;
}

bool csBulletSystem::HandleEvent (iEvent& event)
{
  if (simulationSpeed > SMALL_EPSILON)
    StepSimulation (vc->GetElapsedSeconds ());

  return false;
}

void csBulletSystem::SetSimulationSpeed (float speed)
{
  simulationSpeed = speed;
}

void csBulletSystem::SetStepParameters (float timeStep,
					size_t maxSteps,
					size_t iterations)
{
  worldTimeStep = timeStep;
  worldMaxSteps = maxSteps;
  stepIterations = (int) iterations;

  for (size_t i = 0; i < collSectors.GetSize (); i++)
  {
    btContactSolverInfo& info = collSectors[i]->bulletWorld->getSolverInfo ();
    info.m_numIterations = (int) iterations;
  }
}

void csBulletSystem::StepSimulation (float duration)
{
  if (simulationSpeed > SMALL_EPSILON)
    duration *= simulationSpeed;

  for (size_t i = 0; i < collSectors.GetSize (); i++)
    collSectors[i]->Step (duration);
}

void csBulletSystem::SetSoftBodyEnabled (bool enabled)
{
  if (collSectors.GetSize ())
  {
    ReportError ("Could not toggle soft body simulation if there is"
		 " any active collision sectors");
    return;
  }

  isSoftWorld = enabled;
}

void csBulletSystem::SetDynamicConcaveEnabled (bool enabled)
{
  if (collSectors.GetSize ())
  {
    ReportError ("Could not toggle dynamic concave simulation if there is"
		 " any active collision sectors");
    return;
  }

  concaveEnabled = enabled;
}

void csBulletSystem::SetInternalScale (float scale)
{
  // update parameters
  internalScale = scale;
  inverseInternalScale = 1.0f / scale;
}

void csBulletSystem::SetLinearDamping (float damping)
{
  linearDampening = damping;
}

void csBulletSystem::SetAngularDamping (float damping)
{
  angularDampening = damping;
}

void csBulletSystem::SetAutoDisableParams (float linear, float angular,
					   float time)
{
  linearDisableThreshold = linear;
  angularDisableThreshold = angular;
  timeDisableThreshold = time;
}

csPtr<CS::Collisions::iCollider> csBulletSystem::CreateCollider ()
{
  return new csBulletCollider (this);
}

csPtr<CS::Collisions::iColliderConvexMesh> csBulletSystem::CreateColliderConvexMesh
  (iTriangleMesh* mesh)
{
  if (!mesh)
  {
    ReportError ("No triangle mesh supplied for the creation of a convex collider");
    return csPtr<CS::Collisions::iColliderConvexMesh> (nullptr);
  }

  if (!mesh->GetVertexCount ())
  {
    ReportError ("No vertices in the mesh supplied for the creation of a convex collider");
    return csPtr<CS::Collisions::iColliderConvexMesh> (nullptr);
  }

  return new csBulletColliderConvexMesh (mesh, this);
}

csPtr<CS::Collisions::iColliderConcaveMesh> csBulletSystem::CreateColliderConcaveMesh
  (iTriangleMesh* mesh, bool dynamicEnabled)
{
  if (!mesh)
  {
    ReportError ("No triangle mesh supplied for the creation of a concave collider");
    return csPtr<CS::Collisions::iColliderConcaveMesh> (nullptr);
  }

  if (!mesh->GetTriangleCount ())
  {
    ReportError ("No triangles in the mesh supplied for the creation of a concave collider");
    return csPtr<CS::Collisions::iColliderConcaveMesh> (nullptr);
  }

  btTriangleMesh* btTriMesh = CreateBulletTriMesh (mesh);
  return new csBulletColliderConcaveMesh (mesh, btTriMesh, this, dynamicEnabled);
}

csPtr<CS::Collisions::iColliderConcaveMeshScaled> csBulletSystem::CreateColliderConcaveMeshScaled (
  CS::Collisions::iColliderConcaveMesh* collider, const csVector3& scale)
{
  if (!collider)
  {
    ReportError ("No concave collider supplied for the creation of a scaled concave collider");
    return csPtr<CS::Collisions::iColliderConcaveMeshScaled> (nullptr);
  }

  return new csBulletColliderConcaveMeshScaled (collider, scale, this);
}

csPtr<CS::Collisions::iColliderCylinder> csBulletSystem::CreateColliderCylinder (float length, float radius)
{
  return new csBulletColliderCylinder (length, radius, this);
}

csPtr<CS::Collisions::iColliderBox> csBulletSystem::CreateColliderBox (const csVector3& size)
{
  return new csBulletColliderBox (size, this);
} 

csPtr<CS::Collisions::iColliderSphere> csBulletSystem::CreateColliderSphere (float radius)
{
  return new csBulletColliderSphere (radius, this);
}

csPtr<CS::Collisions::iColliderCapsule> csBulletSystem::CreateColliderCapsule (float length, float radius)
{
  return new csBulletColliderCapsule (length, radius, this);
}

csPtr<CS::Collisions::iColliderCone> csBulletSystem::CreateColliderCone (float length, float radius)
{
  return new csBulletColliderCone (length, radius, this);
}

csPtr<CS::Collisions::iColliderPlane> csBulletSystem::CreateColliderPlane (const csPlane3& plane)
{
  return new csBulletColliderPlane (plane, this);
}

CS::Collisions::iCollisionSector* csBulletSystem::CreateCollisionSector (iSector* sector)
{
  // Look first if there is already a collision sector associated to the given engine sector
  if (sector)
  {
    CS::Collisions::iCollisionSector* collisionSector = FindCollisionSector (sector);
    if (collisionSector) return collisionSector;
  }

  // Create a new collision sector
  csRef<csBulletSector> collSector = csPtr<csBulletSector> (new csBulletSector (this));
  if (sector) collSector->SetSector (sector);
  collSectors.Push (collSector);
  return collSector;
}

void csBulletSystem::DeleteCollisionSector (CS::Collisions::iCollisionSector* sector)
{
  csBulletSector* collSector = dynamic_cast<csBulletSector*> (sector);
  collSectors.Delete (collSector);
}

void csBulletSystem::DeleteCollisionSectors ()
{
  while (collSectors.GetSize () > 0)
  {
    collSectors.DeleteIndex (0);
  }
}

CS::Collisions::iCollisionSector* csBulletSystem::FindCollisionSector (const iSector* sector)
{
  // TODO: use a hash for faster access
  for (size_t i = 0; i < collSectors.GetSize (); i++)
    if (collSectors[i]->GetSector () == sector)
      return collSectors[i];

  return nullptr;
}

CS::Collisions::iCollisionSector* csBulletSystem::FindCollisionSector (const char* name)
{
  return collSectors.FindByName (name);
}

// Groups

CS::Collisions::iCollisionGroup* csBulletSystem::CreateCollisionGroup (const char* name)
{
  csRef<CollisionGroup>* group = collisionGroups.GetElementPointer (name);
  if (group) return *group;

  size_t groupCount = collisionGroups.GetSize ();
  // It is safer to use 16 as the global lower bound limit
  //if (groupCount >= sizeof (CS::Collisions::CollisionGroupMask) * 8)
  if (groupCount >= 16)
    return nullptr;

  csRef<CollisionGroup> newGroup;
  newGroup.AttachNew (new CollisionGroup (name, groupCount));
  collisionGroups.Put (name, newGroup);
  return newGroup;
}

CS::Collisions::iCollisionGroup* csBulletSystem::FindCollisionGroup (const char* name) const
{
  const csRef<CollisionGroup>* group = collisionGroups.GetElementPointer (name);
  if (group) return *group;
  else return nullptr;
}

size_t csBulletSystem::GetCollisionGroupCount () const
{
  return collisionGroups.GetSize ();
}

iCollisionGroup* csBulletSystem::GetCollisionGroup (size_t index) const
{
  CS_ASSERT (index < collisionGroups.GetSize ());
  csHash< csRef<CollisionGroup>, const char*>::ConstGlobalIterator it =
    collisionGroups.GetIterator ();
  for (int i = 0; i < ((int) index) - 1; i++) it.Next ();
  return it.Next ();
}

// ###############################################################################################################
// Physical Objects

// Joints

csPtr<CS::Physics::iJointFactory> csBulletSystem::CreateJointFactory ()
{
  csRef<JointFactory> factory = csPtr<JointFactory> (new JointFactory (this));
  return csPtr<CS::Physics::iJointFactory> (factory);
}

csPtr<CS::Physics::iJointFactory> csBulletSystem::CreateP2PJointFactory ()
{
  csRef<JointFactory> factory = csPtr<JointFactory> (new JointFactory (this));
  factory->SetTransConstraints (true, true, true);
/*
  csVector3 trans (0.0f);
  factory->SetMinimumDistance (trans);
  factory->SetMaximumDistance (trans);
*/
  factory->SetType (RIGID_P2P_JOINT);
  return csPtr<CS::Physics::iJointFactory> (factory);
}

csPtr<CS::Physics::iJointFactory> csBulletSystem::CreateSlideJointFactory
(float minDist, float maxDist, float minAngle, float maxAngle, int axis)
{
  if (axis < 0 || axis > 2)
    return csPtr<CS::Physics::iJointFactory> (nullptr);
  csRef<JointFactory> factory = csPtr<JointFactory> (new JointFactory (this));
  factory->SetTransConstraints (true, true, true);
  factory->SetRotConstraints (true, true, true);
  csVector3 minDistant (0.0f);
  csVector3 maxDistant (0.0f);

  minDistant[axis] = minDist;
  maxDistant[axis] = maxDist;
  factory->SetMinimumDistance (minDistant);
  factory->SetMaximumDistance (maxDistant);
  minDistant[axis] = minAngle;
  maxDistant[axis] = maxAngle;
  factory->SetMinimumAngle (minDistant);
  factory->SetMaximumAngle (maxDistant);
  factory->SetType (RIGID_SLIDE_JOINT);
  return csPtr<CS::Physics::iJointFactory> (factory);
}

csPtr<CS::Physics::iJointFactory> csBulletSystem::CreateHingeJointFactory
(float minAngle, float maxAngle, int axis)
{
  if (axis < 0 || axis > 2)
    return csPtr<CS::Physics::iJointFactory> (nullptr);
  csRef<JointFactory> factory = csPtr<JointFactory> (new JointFactory (this));
  csVector3 minDistant (0.0f);
  csVector3 maxDistant (0.0f);
  minDistant[axis] = minAngle;
  maxDistant[axis] = maxAngle;
  factory->SetMinimumAngle (minDistant);
  factory->SetMaximumAngle (maxDistant);
  factory->SetType (RIGID_HINGE_JOINT);
  factory->SetAxis (axis);
  return csPtr<CS::Physics::iJointFactory> (factory);
}

csPtr<CS::Physics::iJointFactory> csBulletSystem::CreateConeTwistJointFactory
(float swingSpan1, float swingSpan2, float twistSpan) 
{
  csRef<JointFactory> factory = csPtr<JointFactory> (new JointFactory (this));
  factory->SetTransConstraints (true, true, true);
  factory->SetRotConstraints (true, true, true);

  csVector3 minDistant (0.0f);
  csVector3 maxDistant (0.0f);
  factory->SetMaximumDistance (minDistant);
  factory->SetMinimumDistance (maxDistant);
  minDistant.Set (-twistSpan, -swingSpan2, -swingSpan1);  
  maxDistant.Set (twistSpan, swingSpan2, swingSpan1); 
  factory->SetMinimumAngle (minDistant);
  factory->SetMaximumAngle (maxDistant);
  factory->SetType (RIGID_CONETWIST_JOINT);
  return csPtr<CS::Physics::iJointFactory> (factory);
}

csPtr<CS::Physics::iJointFactory> csBulletSystem::CreateSoftLinearJointFactory ()
{
  csRef<JointFactory> factory = csPtr<JointFactory> (new JointFactory (this));
  factory->SetType (SOFT_LINEAR_JOINT);
  return csPtr<CS::Physics::iJointFactory> (factory);
}

csPtr<CS::Physics::iJointFactory> csBulletSystem::CreateSoftAngularJointFactory (int axis)
{
  if (axis < 0 || axis > 2)
    return csPtr<CS::Physics::iJointFactory> (nullptr);
  csRef<JointFactory> factory = csPtr<JointFactory> (new JointFactory (this));
  factory = new JointFactory (this);
  if (axis == 0)
    factory->SetRotConstraints (false, true, true);
  else if (axis == 1)
    factory->SetRotConstraints (true, false, false);
  else if (axis == 2)
    factory->SetRotConstraints (false, false, true);

  factory->SetType (SOFT_ANGULAR_JOINT);
  return csPtr<CS::Physics::iJointFactory> (factory);
}

csPtr<CS::Physics::iJointFactory> csBulletSystem::CreatePivotJointFactory ()
{
  csRef<JointFactory> factory = csPtr<JointFactory> (new JointFactory (this));
  factory->SetTransConstraints (true, true, true);
  csVector3 trans (0.0f);
  factory->SetMaximumDistance (trans);
  factory->SetMinimumDistance (trans);
  factory->SetType (RIGID_PIVOT_JOINT);
  return csPtr<CS::Physics::iJointFactory> (factory);
}

// Vehicles

/// Creates a new factory to produce vehicles
csPtr<iVehicleFactory> csBulletSystem::CreateVehicleFactory (CS::Collisions::iCollider* collider)
{
  return csPtr<iVehicleFactory> (new BulletVehicleFactory (this, collider));
}

/// Creates a new factory to produce vehicle wheels
csPtr<iVehicleWheelFactory> csBulletSystem::CreateVehicleWheelFactory ()
{
  return csPtr<iVehicleWheelFactory> (new BulletVehicleWheelFactory (this));
}

// Factory

csPtr<iCollisionObjectFactory> csBulletSystem::CreateCollisionObjectFactory
  (CS::Collisions::iCollider *collider)
{
  BulletRigidBodyFactory* factory = new BulletRigidBodyFactory (this, collider);
  // TODO: remove this system allocation
  factory->system = this;
  factory->SetState (STATE_STATIC);
  return csPtr<iCollisionObjectFactory> (factory);
}

csPtr<CS::Collisions::iCollisionObjectFactory>
 csBulletSystem::CreateGhostCollisionObjectFactory (CS::Collisions::iCollider* collider) 
{ 
  BulletGhostCollisionObjectFactory* fact = new BulletGhostCollisionObjectFactory (this, collider);
  fact->system = this;
  return csPtr<iCollisionObjectFactory> (fact); 
}

csPtr<CS::Collisions::iCollisionActorFactory> 
  csBulletSystem::CreateCollisionActorFactory (CS::Collisions::iCollider* collider) 
{
  BulletCollisionActorFactory* fact = new BulletCollisionActorFactory (this, collider);
  return csPtr<iCollisionActorFactory> (fact); 
}

csPtr<CS::Collisions::iCollisionTerrainFactory> csBulletSystem::CreateCollisionTerrainFactory
  (iTerrainFactory* terrain)
{
  return new csBulletCollisionTerrainFactory (this, terrain);
}

csPtr<CS::Physics::iRigidBodyFactory> 
  csBulletSystem::CreateRigidBodyFactory (CS::Collisions::iCollider* collider)
{
  BulletRigidBodyFactory* fact = new BulletRigidBodyFactory (this, collider);
  return csPtr<iRigidBodyFactory> (fact); 
}

csPtr<CS::Physics::iSoftRopeFactory> csBulletSystem::CreateSoftRopeFactory ()
{
  BulletSoftRopeFactory* fact = new BulletSoftRopeFactory (this);
  return csPtr<iSoftRopeFactory> (fact);
}

csPtr<CS::Physics::iSoftClothFactory> csBulletSystem::CreateSoftClothFactory ()
{
  BulletSoftClothFactory* fact = new BulletSoftClothFactory (this);
  return csPtr<iSoftClothFactory> (fact);
}

csPtr<CS::Physics::iSoftMeshFactory> csBulletSystem::CreateSoftMeshFactory ()
{
  BulletSoftMeshFactory* fact = new BulletSoftMeshFactory (this);
  return csPtr<iSoftMeshFactory> (fact);
}

void csBulletSystem::DeleteAll () 
{
  collSectors.DeleteAll ();
  collisionGroups.DeleteAll ();

  // Create the default collision group
  csRef<CollisionGroup> group;
  group.AttachNew (new CollisionGroup ("Default", 0));
  defaultGroup = collisionGroups.Put ("Default", group);
}

void csBulletSystem::InitDebugDraw ()
{
  if (!debugDraw)
  {
    debugDraw = new csBulletDebugDraw (inverseInternalScale);
    for (size_t i = 0; i < collSectors.GetSize (); i++)
      collSectors[i]->bulletWorld->setDebugDrawer (debugDraw);
  }
}

void csBulletSystem::DebugDraw (iView* view)
{
  DebugDraw (view->GetContext (), view->GetCamera ());
}

void csBulletSystem::DebugDraw (iGraphics3D* g3d, iCamera* camera)
{
  InitDebugDraw ();
  for (size_t i = 0; i < collSectors.GetSize (); i++)
    collSectors[i]->bulletWorld->debugDrawWorld ();
  debugDraw->DebugDraw (g3d, camera);
}

void csBulletSystem::SetDebugMode (CS::Physics::DebugMode mode)
{
  if (!debugDraw && mode)
    InitDebugDraw ();
  debugDraw->SetDebugMode (mode);
}

CS::Physics::DebugMode csBulletSystem::GetDebugMode ()
{
  if (!debugDraw)
    return CS::Physics::DEBUG_NOTHING;

  return debugDraw->GetDebugMode ();
}

void csBulletSystem::StartProfile ()
{
  CProfileManager::Start_Profile ("Crystal Space scene");
  InitDebugDraw ();
  debugDraw->StartProfile ();
}

void csBulletSystem::StopProfile ()
{
  CProfileManager::Stop_Profile ();
  debugDraw->StopProfile ();
}

void csBulletSystem::DumpProfile (bool resetProfile /* = true */)
{
  printf ("\n");
  printf ("==========================================================\n");
  printf ("====           Bullet dynamic scene profile           ====\n");
  printf ("==========================================================\n");
  CProfileManager::dumpAll ();
  printf ("==========================================================\n");
  printf ("\n");

  if (resetProfile)
    CProfileManager::Reset ();
}

btTriangleMesh* csBulletSystem::CreateBulletTriMesh (iTriangleMesh* triMesh)
{
  btTriangleMesh* btMesh = new btTriangleMesh ();
  
  size_t triangleCount = triMesh->GetTriangleCount ();
  //size_t vertexCount = triMesh->GetVertexCount ();

  size_t i;
  csTriangle *c_triangle = triMesh->GetTriangles ();
  csVector3 *c_vertex = triMesh->GetVertices ();
  for (i = 0; i < triangleCount; i++)
  {
    int index0 = c_triangle[i].a;
    int index1 = c_triangle[i].b;
    int index2 = c_triangle[i].c;

    btVector3 vertex0 (c_vertex[index0].x, c_vertex[index0].y, c_vertex[index0].z);
    btVector3 vertex1 (c_vertex[index1].x, c_vertex[index1].y, c_vertex[index1].z);
    btVector3 vertex2 (c_vertex[index2].x, c_vertex[index2].y, c_vertex[index2].z);

    vertex0 *= GetInternalScale ();
    vertex1 *= GetInternalScale ();
    vertex2 *= GetInternalScale ();

    btMesh->addTriangle (vertex0, vertex1, vertex2);
  }

  return btMesh;
}

}
CS_PLUGIN_NAMESPACE_END (Bullet2)
