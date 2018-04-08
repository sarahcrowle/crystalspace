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
#ifndef __PHYSTUT2_H__
#define __PHYSTUT2_H__

#include "cssysdef.h"
#include "cstool/collisionhelper.h"
#include "cstool/demoapplication.h"
#include "csutil/weakref.h"

#include "imesh/animesh.h"
#include "imesh/animnode/ragdoll2.h"
#include "imesh/modifiableterrain.h"
#include "imesh/skeletonmodel.h"
#include "imesh/softanim.h"

#include "ivaria/collisions.h"
#include "ivaria/physics.h"
#include "ivaria/vehicle.h"

#include "agent.h"

struct iCameraPosition;

// Some global variables
static const float DefaultDensity (300);
static const float DefaultFriction (.5);
static const float DefaultElasticity (0.1f);

static const int UpAxis = 1;
static const int HorizontalAxis1 = 0;
static const int HorizontalAxis2 = 2;
static const csVector3 UpVector (0, 1, 0);

// Actor/Camera modes
enum ActorMode
{
  ActorModeNone,
  ActorModePhysical,
  ActorModeNoclip
};

static const csVector3 ActorDimensions (0.5f, 1.8f, 0.5f);

// Levels
enum PhysDemoLevel
{
  PhysDemoLevelNone,
  PhysDemoLevelBox,
  PhysDemoLevelPortals,
  PhysDemoLevelTerrain,
  PhysDemoLevelCastle
};

// Navigation input keys
static const int KeyUp = CSKEY_PGUP;
static const int KeyDown = CSKEY_PGDN;
static const int KeyJump = CSKEY_SPACE;
static const int KeyHandbrake = CSKEY_SPACE;

#if 1
// For QWERTY keyboards:
static const int KeyForward = 'w';
static const int KeyBackward = 's';
static const int KeyLeft = 'a';
static const int KeyRight = 'd';
#else
// For AZERTY keyboards:
static const int KeyForward = 'z';
static const int KeyBackward = 's';
static const int KeyLeft = 'q';
static const int KeyRight = 'd';
#endif

/// Retreives folder and file information from a full (unix-style) path
inline void GetFolderAndFile (const char* _path, csString& folder, csString& filename)
{
  const csString path (_path);
  size_t index = path.FindLast ('/');
  if (index != (size_t) -1)
  {
    folder = path.Slice (0, index);
    filename = path.Slice (index + 1);
  }
  else
  {
    folder = "";
    filename = path;
    filename.Trim ();
  }
}

/**
 * Re-usable pair of a collider with a render mesh. 
 * Can be used to create new RigidBody objects.
 */
class RenderMeshColliderPair
{
public:
  csRef<CS::Collisions::iCollider> Collider;
  csRef<iMeshFactoryWrapper> MeshFactory;

  /// Creates a new RigidBody from the given collider and render mesh
  csPtr<CS::Physics::iRigidBody> SpawnRigidBody
    (const csOrthoTransform& trans, float friction = 1, float density = 30);
};

class PhysDemo : public CS::Utility::DemoApplication
{
  friend class RenderMeshColliderPair;

public:
  CS::Collisions::CollisionHelper collisionHelper;
  csRef<CS::Physics::iPhysicalSystem> physicalSystem;
  csRef<CS::Collisions::iConvexDecomposer> convexDecomposer;

  csRef<CS::Animation::iSoftBodyAnimationControl2Type> softBodyAnimationType;

  csRef<iGenMeshAnimationControlFactory> softBodyAnimationFactory;
  bool isSoftBodyWorld;

  // Meshes
  csRef<CS::Collisions::iColliderConcaveMesh> mainCollider;

  RenderMeshColliderPair stackBoxMeshPair;

  // Configuration related
  bool autodisable;
  int phys_engine_id;
  bool do_bullet_debug;
  bool do_soft_debug;

  // Dynamic simulation related
  bool allStatic;
  bool paused;
  float simulationSpeed;

  // Dragging related
  bool dragging;
  bool softDragging;
  csRef<CS::Physics::iJoint> dragJoint;
  csRef<CS::Physics::iSoftBody> draggedBody;
  
  size_t draggedVertex;
  float dragDistance;
  float linearDampening, angularDampening;

  // Camera & actors
  CS::Physics::DebugMode debugMode;
  float moveSpeed, turnSpeed;
  ActorMode actorMode;

  Agent player;
  Item* selectedItem;
  csRef<CS::Collisions::iCollisionActor> actor;

  // Ragdoll related
  csRef<CS::Animation::iSkeletonRagdollNodeManager2> ragdollManager;
  csRef<CS::Animation::iSkeletonModelManager> modelManager;

  // Cut & Paste related
  csRef<CS::Physics::iPhysicalBody> clipboardBody;
  csRef<iMovable> clipboardMovable;

  // Collider

  // Ghost
  csWeakRef<CS::Collisions::iCollisionObject> ghostObject;

  // Terrain
  /// The feeder to which the current terrain mod is applied (if any)
  csRef<iModifiableDataFeeder> moddedTerrainFeeder;
  /// The currently applied terrain mode (if any)
  csRef<iTerrainModifier> terrainMod;

  // Vehicles
  csRef<CS::Physics::iVehicle> vehicle;

  // Static environment
  csString currentMap;
  csRef<iMeshWrapper> walls;

  size_t selectedIndex;

private:
  void Frame ();
  bool OnKeyboard (iEvent &event);

  bool OnMouseDown (iEvent &event);
  bool OnMouseUp (iEvent &event);

public:
  // Camera
  void UpdateActorMode (ActorMode newActorMode);

  // Spawning objects
  void CreateBoxCollider (csRef<CS::Collisions::iColliderBox>& colliderPtr, csRef<iMeshWrapper>& meshPtr, const csVector3& extents);

  csRef<CS::Physics::iRigidBody> SpawnBox (bool setVelocity = true);
  csRef<CS::Physics::iRigidBody> SpawnBox (const csVector3& extents, const csVector3& pos, bool setVelocity = true);
  CS::Physics::iRigidBody* SpawnBox (const RenderMeshColliderPair& pair, const csVector3& pos, bool setVelocity = true);
  CS::Physics::iRigidBody* SpawnSphere ();
  CS::Physics::iRigidBody* SpawnCone (bool setVelocity = true);
  CS::Physics::iRigidBody* SpawnCylinder (bool setVelocity = true);
  CS::Physics::iRigidBody* SpawnCapsule (float length = rand () % 3 / 50.f + .7f,
    float radius = rand () % 10 / 50.f + .2f, bool setVelocity = true);
  CS::Collisions::iCollisionObject* SpawnConcaveMesh ();
  CS::Physics::iRigidBody* SpawnConvexMesh (bool setVelocity = true);
  CS::Physics::iRigidBody* SpawnCompound (bool setVelocity = true);
  CS::Physics::iJoint* SpawnJointed ();
  void SpawnChain ();
  void SpawnParticles ();
  void LoadFrankieRagdoll ();
  void LoadKrystalRagdoll ();
  void SpawnFrankieRagdoll ();
  void SpawnKrystalRagdoll ();
  void SpawnRope ();
  CS::Physics::iSoftBody* SpawnCloth ();
  CS::Physics::iSoftBody* SpawnSoftBody (bool setVelocity = true);
  void SpawnBoxStacks (int stackNum = 4, int stackHeight = 4, float boxLen = .5f, float mass = 20.f);
  
  CS::Physics::iRigidBody* SpawnChainLink ();

  /**
   * Room is the inside of a cuboid of the given size, and wall thickness
   */
  void CreateBoxRoom (const csVector3& roomExtents, const csVector3& pos = csVector3 (0), float wallThickness = 5);

  /// Create the default box room with a given side-length
  void CreateBoxRoom (float size);
  
  /// Load a scene from file
  bool LoadLevel (const char* filepath, bool convexDecomp = false);

  void CreateGhostCylinder ();
  void ApplyGhostSlowEffect ();

  /// Creates a new rigid body, places it at the given pos and, optionally, gives it some initial momentum
  csRef<CS::Physics::iRigidBody> SpawnRigidBody (RenderMeshColliderPair& pair, const csVector3& pos, 
    float friction = 1, float density = 30, bool setVelocity = false);

  // removes everything and resets things
  void ResetCurrentLevel ();

public:
  PhysDemo ();
  virtual ~PhysDemo ();

  void PrintHelp ();
  bool OnInitialize (int argc, char* argv[]);
  bool Application ();

  void SetupHUD ();

  friend class MouseAnchorAnimationControl;
  csRef<CS::Physics::iAnchorAnimationControl> grabAnimationControl;

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Transformation utilities

  /**
   * The location of the actor's head, i.e. the location of the camera
   */
  csVector3 GetActorPos () const
  { return view->GetCamera ()->GetTransform ().GetOrigin (); }

  /**
   * The location where the actor stands
   */
  csVector3 GetActorFeetPos () const
  { return GetActorPos () - csVector3 (0, .5f * ActorDimensions.y, 0); }

  /**
   * Position of the camera
   */
  csVector3 GetCameraPosition () const
  { return view->GetCamera ()->GetTransform ().GetOrigin (); }

  /**
   * Normalized direction of the camera
   */
  csVector3 GetCameraDirection () const
  { return view->GetCamera ()->GetTransform ().GetT2O () * csVector3 (0, 0, 1); }

  /**
   * Normalized direction of the camera, but in the same XZ plane (ignoring vertical direction of the camera)
   */
  csVector3 GetCameraDirectionXZ () const
  { csVector3 dist = view->GetCamera ()->GetTransform ().GetT2O () * csVector3 (0, 0, 1); dist.y = 0; dist.Normalize (); return dist; }

  /**
   * Point in the given distance in front of the camera
   */
  csVector3 GetPointInFront (float distance) const
  { return GetActorPos () + (GetCameraDirection () * distance); }

  /**
   * Point in the given distance in front of the actor's feet
   */
  csVector3 GetPointInFrontOfFeet (float distance) const
  { return GetActorFeetPos () + (GetCameraDirection () * distance); }

  /**
   * Point in the given distance in front of the camera, but in the same XZ plane (ignoring vertical direction of the camera)
   */
  csVector3 GetPointInFrontXZ (float distance) const
  { return GetActorPos () + (GetCameraDirectionXZ () * distance); }

  /**
   * Point in the given distance in front of the camera, but in the same XZ plane (ignoring vertical direction of the camera)
   */
  csVector3 GetPointInFrontOfFeetXZ (float distance) const
  { return GetActorFeetPos () + (GetCameraDirectionXZ () * distance); }

  /// Find the ground contact point beneath pos
  bool GetPointOnGroundBeneathPos (const csVector3& pos, csVector3& groundPos) const;

  /// Find the ground contact point above pos
  bool GetPointOnGroundAbovePos (const csVector3& pos, csVector3& groundPos) const;

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Geometry utilities

  csPtr<iMeshWrapper> CreateCylinderYMesh (float length, float radius, const char* materialName = "objtexture", const char* meshName = "cylinder");

  csPtr<iMeshWrapper> CreateBoxMesh (const csVector3& extents, const char* materialName = "objtexture", const char* meshName = "box");
  
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Object & Actor utilities

  /// Pick the object that is under the cursor
  bool PickCursorObject (CS::Collisions::HitBeamResult& result);

  /// Get the owning entity of the given collision object (actor, vehicle, etc)
  ::iBase* GetOwner (CS::Collisions::iCollisionObject* obj);

  /// Teleport given object to given location
  void TeleportObject (CS::Collisions::iCollisionObject* obj, iCameraPosition* pos);

  bool IsActor (CS::Collisions::iCollisionObject* obj) const;
  
  /// Adds a force to the given object that pulls it toward the actor. If not given, picks currently targeted object.
  void PullObject (CS::Collisions::iCollisionObject* obj = nullptr);

  /// Toggles between DYNAMIC/STATIC state of the given object. If not given, picks currently targeted object.
  void ToggleObjectState (CS::Collisions::iCollisionObject* obj = nullptr);

  /// Deletes the given object. If not given, picks currently targeted object.
  void DeleteObject (CS::Collisions::iCollisionObject* obj = nullptr);
  
  
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Items

  void CreateItemTemplates ();

  void UpdateCameraManager ();
  

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Mesh & Collider Utilities

  void CreateBoxMeshColliderPair (RenderMeshColliderPair& pair, const csVector3& extents);
    

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Vehicles
  
  csPtr<CS::Physics::iVehicle> CreateVehicle ();

  void EnterTargetVehicle ();
  void LeaveCurrentVehicle ();

  void SpawnVehicle ();
  void DeleteTargetVehicle ();

  void AccelerateTargetVehicle ();

  CS::Physics::iVehicle* GetTargetVehicle ();


  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Frame

  void MoveActor ();
  void MoveActorVehicle ();

  void UpdateDragging ();

  void UpdateHUD ();

  void DoDebugDraw ();

  
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Level setup & management
  
  CS::Physics::iPhysicalSector* GetCurrentSector () const;

  /// Clears & deletes the scene
  void Reset ();

  /// Reset the current scene and setup the given level
  bool SetLevel (PhysDemoLevel level, bool convexDecomp = false);
  /// Reset the current scene and setup the given level
  bool SetLevel (const char* pathname, bool convexDecomp = false);

  /// Get the iModifiableDataFeeder of the first terrain that has one in the given sector (if any)
  iModifiableDataFeeder* GetFirstTerrainModDataFeeder (CS::Collisions::iCollisionSector* sector);

  
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Physics stuff

  bool IsDynamic (CS::Collisions::iCollisionObject* obj) const;

  void SetGravity (const csVector3& g);
  
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Other

  bool IsGravityOff ()
  { return GetCurrentSector ()->GetGravity ().SquaredNorm () > SMALL_EPSILON; }
};

class MouseAnchorAnimationControl : public scfImplementation1
  <MouseAnchorAnimationControl, CS::Physics::iAnchorAnimationControl>
{
public:
  MouseAnchorAnimationControl (PhysDemo* simple)
    : scfImplementationType (this), simple (simple) {}

  csVector3 GetAnchorPosition () const;

private:
  PhysDemo* simple;
};

extern PhysDemo physDemo;

/// Utility: Component-wise vector multiplication
inline csVector3 ScaleVector3 (const csVector3& v1, const csVector3& v2)
{
  csVector3 v3;
  v3.x = v1.x * v2.x;
  v3.y = v1.y * v2.y;
  v3.z = v1.z * v2.z;
  return v3;
}

// TODO: Replace with CS' equivalent of std::bind and std::function
#define CallOnAllSectors (code) \
  for (size_t iiiii = 0; iiiii < physDemo.physicalSystem->GetCollisionSectorCount (); ++iiiii) \
  { \
    csRef<iPhysicalSector>(scfQueryInterface<iPhysicalSector>(physDemo.physicalSystem->GetCollisionSector (iiiii)))->code; \
  }

#endif
