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
#ifndef __CS_BULLET_PHYSICS_H__
#define __CS_BULLET_PHYSICS_H__

#include "csutil/csobject.h"
#include "csutil/eventhandlers.h"
#include "csutil/hash.h"
#include "csutil/nobjvec.h"
#include "csutil/scf.h"
#include "csutil/scf_implementation.h"
#include "iengine/movable.h"
#include "iengine/portal.h"
#include "iengine/sector.h"
#include "iutil/comp.h"
#include "iutil/eventh.h"
#include "iutil/eventq.h"
#include "iutil/virtclk.h"
#include "ivaria/collisions.h"
#include "ivaria/reporter.h"
#include "ivaria/physics.h"
#include "ivaria/view.h"

#include "physicsfactories.h"

static const char* msgid = "crystalspace.physics.bullet";

struct iSector;
class btCollisionObject;
class btCompoundShape;
class btDynamicsWorld;
class btCollisionDispatcher;
class btDefaultCollisionConfiguration;
class btSequentialImpulseConstraintSolver;
struct btSoftBodyWorldInfo;
class btBroadphaseInterface;
class btTriangleMesh;

#if (CS_BULLET_VERSION == 282)
// Suppress stupid compiler warning
#include "csutil/custom_new_disable.h"
#include "LinearMath/btScalar.h"
#include "csutil/custom_new_enable.h"

inline int btGetInfinityMask ()
{
  return btInfinityMask;
}
#endif

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
  
class csBulletCollider;
class BulletCollisionActor;
class csBulletCollisionObject;
class csBulletDebugDraw;
class csBulletGhostCollisionObject;
class csBulletJoint;
class csBulletRigidBody;
class csBulletSector;
class csBulletSoftBody;
class csBulletSystem;

class CollisionGroup : public scfImplementation1<CollisionGroup,
  CS::Collisions::iCollisionGroup>
{
private:
  csString name;
  char index;

public:
  /// The value of the group.
  int value;

  /// The mask of the group.
  int mask;

  CollisionGroup (const char* name, char index);

   virtual const char* GetName () const
   { return name.GetData (); }

   virtual void SetCollisionEnabled (iCollisionGroup* other, bool enabled);
   virtual bool GetCollisionEnabled (iCollisionGroup* other);
};

class csBulletSystem : public scfImplementationExt4<
  csBulletSystem, csObject,
  scfFakeInterface<CS::Collisions::iCollisionSystem>,
  CS::Physics::iPhysicalSystem, 
  iEventHandler,
  iComponent>
{
  friend class csBulletColliderConvexMesh;
  friend class csBulletColliderConcaveMesh;
  friend class csBulletSector;
  friend class CollisionPortal;
  friend class csBulletRigidBody;
  friend class csBulletSoftBody;

private:
  iObjectRegistry* object_reg;

  csRef<iEventQueue> eventQueue;
  csRef<iVirtualClock> vc;

  btSoftBodyWorldInfo* defaultInfo;

  float internalScale;
  float inverseInternalScale;

  csRefArrayObject<csBulletSector> collSectors;

  CollisionGroup* defaultGroup;
  csHash< csRef<CollisionGroup>, const char*> collisionGroups;

  float simulationSpeed;
  float worldTimeStep;
  int worldMaxSteps;
  int stepIterations;

  bool isSoftWorld;
  bool concaveEnabled;

  float linearDampening;
  float angularDampening;

  float linearDisableThreshold;
  float angularDisableThreshold;
  float timeDisableThreshold;

  csBulletDebugDraw* debugDraw;

public:
  csBulletSystem (iBase* iParent);
  virtual ~csBulletSystem ();

  //-- iComponent
  virtual bool Initialize (iObjectRegistry* object_reg);

  //-- iEventHandler
  bool HandleEvent (iEvent& event);

  //-- iCollisionSystem
  virtual CS::Physics::iPhysicalSystem* QueryPhysicalSystem ()
    { return dynamic_cast<iPhysicalSystem*> (this); }

  virtual void SetSimulationSpeed (float speed);
  virtual float GetSimulationSpeed () const
  { return simulationSpeed; }

  virtual csPtr<CS::Collisions::iCollider> CreateCollider ();
  virtual csPtr<CS::Collisions::iColliderConvexMesh> CreateColliderConvexMesh
    (iTriangleMesh* mesh);
  virtual csPtr<CS::Collisions::iColliderConcaveMesh> CreateColliderConcaveMesh
    (iTriangleMesh* mesh, bool dynamicEnabled = false);
  virtual csPtr<CS::Collisions::iColliderConcaveMeshScaled> CreateColliderConcaveMeshScaled
      (CS::Collisions::iColliderConcaveMesh* collider, const csVector3& scale);
  virtual csPtr<CS::Collisions::iColliderCylinder> CreateColliderCylinder
    (float length, float radius);
  virtual csPtr<CS::Collisions::iColliderBox> CreateColliderBox (const csVector3& size);
  virtual csPtr<CS::Collisions::iColliderSphere> CreateColliderSphere (float radius);
  virtual csPtr<CS::Collisions::iColliderCapsule> CreateColliderCapsule (float length, float radius);
  virtual csPtr<CS::Collisions::iColliderCone> CreateColliderCone (float length, float radius);
  virtual csPtr<CS::Collisions::iColliderPlane> CreateColliderPlane (const csPlane3& plane);
  
  virtual CS::Collisions::iCollisionSector* CreateCollisionSector (iSector* sector = nullptr);
  virtual void DeleteCollisionSector (CS::Collisions::iCollisionSector* sector);
  virtual void DeleteCollisionSectors ();
  virtual size_t GetCollisionSectorCount () const { return collSectors.GetSize (); }
  virtual CS::Collisions::iCollisionSector* GetCollisionSector (size_t index) 
  {
    return csRef<CS::Collisions::iCollisionSector>
      (scfQueryInterface<CS::Collisions::iCollisionSector> (collSectors.Get (index)));
  }
  virtual CS::Collisions::iCollisionSector* FindCollisionSector (const iSector* sceneSector);
  virtual CS::Collisions::iCollisionSector* FindCollisionSector (const char* name);

  virtual CS::Collisions::iCollisionGroup* CreateCollisionGroup (const char* name);
  virtual CS::Collisions::iCollisionGroup* FindCollisionGroup (const char* name) const;
  virtual size_t GetCollisionGroupCount () const;
  virtual CS::Collisions::iCollisionGroup* GetCollisionGroup (size_t index) const;

  //-- iPhysicalSystem
  virtual void SetStepParameters (float timeStep, size_t maxSteps, size_t iterations);
  virtual void StepSimulation (float duration);

  virtual void SetSoftBodyEnabled (bool enabled);
  virtual bool GetSoftBodyEnabled () const { return isSoftWorld; }

  virtual void SetDynamicConcaveEnabled (bool enabled);
  virtual bool GetDynamicConcaveEnabled () const { return concaveEnabled; }

  virtual void SetInternalScale (float scale);
  virtual float GetInternalScale () const { return internalScale; }

  virtual void SetLinearDamping (float d);
  virtual float GetLinearDamping () const { return linearDampening; }

  virtual void SetAngularDamping (float d);
  virtual float GetAngularDamping () const { return angularDampening; }

  virtual void SetAutoDisableParams (float linear, float angular, float time);

  // Factories
  virtual csPtr<CS::Collisions::iCollisionObjectFactory> CreateCollisionObjectFactory
    (CS::Collisions::iCollider *collider);

  virtual csPtr<CS::Collisions::iCollisionObjectFactory> CreateGhostCollisionObjectFactory
    (CS::Collisions::iCollider* collider = nullptr);

  virtual csPtr<CS::Collisions::iCollisionActorFactory> CreateCollisionActorFactory
    (CS::Collisions::iCollider* collider = nullptr);

  virtual csPtr<CS::Collisions::iCollisionTerrainFactory> CreateCollisionTerrainFactory
    (iTerrainFactory* terrain);

  virtual csPtr<CS::Physics::iRigidBodyFactory> CreateRigidBodyFactory
    (CS::Collisions::iCollider* collider = nullptr);

  virtual csPtr<CS::Physics::iSoftRopeFactory> CreateSoftRopeFactory ();
  virtual csPtr<CS::Physics::iSoftClothFactory> CreateSoftClothFactory () ;
  virtual csPtr<CS::Physics::iSoftMeshFactory> CreateSoftMeshFactory ();

  // Joints & Constraints

  virtual csPtr<CS::Physics::iJointFactory> CreateJointFactory ();
  virtual csPtr<CS::Physics::iJointFactory> CreateP2PJointFactory ();
  virtual csPtr<CS::Physics::iJointFactory> CreateSlideJointFactory
    (float minDist, float maxDist, float minAngle, float maxAngle, int axis);
  virtual csPtr<CS::Physics::iJointFactory> CreateHingeJointFactory
    (float minAngle, float maxAngle, int axis);
  virtual csPtr<CS::Physics::iJointFactory> CreateConeTwistJointFactory
    (float swingSpan1, float swingSpan2, float twistSpan);
  virtual csPtr<CS::Physics::iJointFactory> CreateSoftLinearJointFactory ();
  virtual csPtr<CS::Physics::iJointFactory> CreateSoftAngularJointFactory (int axis);
  virtual csPtr<CS::Physics::iJointFactory> CreatePivotJointFactory ();

  // Vehicles
  
  virtual csPtr<CS::Physics::iVehicleFactory> CreateVehicleFactory
    (CS::Collisions::iCollider* collider = nullptr);
  virtual csPtr<CS::Physics::iVehicleWheelFactory> CreateVehicleWheelFactory ();

  // Misc
  void DeleteAll ();

  virtual void InitDebugDraw ();
  virtual void DebugDraw (iView* rview);
  virtual void DebugDraw (iGraphics3D* g3d, iCamera* camera);
  virtual void SetDebugMode (CS::Physics::DebugMode mode);
  virtual CS::Physics::DebugMode GetDebugMode ();

  virtual void StartProfile ();
  virtual void StopProfile ();
  virtual void DumpProfile (bool resetProfile = true);

  // Internal stuff
  inline CollisionGroup* GetDefaultGroup () const { return defaultGroup; }
  // TODO: remove that
  inline btSoftBodyWorldInfo* GetSoftBodyWorldInfo () const { return defaultInfo; }
  inline float GetInverseInternalScale () {return inverseInternalScale;}
  inline float GetWorldTimeStep() const { return worldTimeStep; }

  btTriangleMesh* CreateBulletTriMesh (iTriangleMesh* triMesh);

  bool ReportError (const char* msg, ...);
  void ReportWarning (const char* msg, ...);

  // Declare this event handler as listening to the 'LOGIC' frame phase
  CS_EVENTHANDLER_PHASE_LOGIC (msgid);
};

}
CS_PLUGIN_NAMESPACE_END (Bullet2)

#endif
