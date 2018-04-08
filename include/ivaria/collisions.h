/*
    Copyright (C) 2011-2013 Christian Van Brussel, Institute of Information
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

#ifndef __CS_IVARIA_COLLISION2_H__
#define __CS_IVARIA_COLLISION2_H__

/**\file
 * Collision interfaces
 */

#include "csutil/scf.h"
#include "iutil/strset.h"
#include "ivaria/colliders.h"

struct iCamera;
struct iMeshWrapper;
struct iMovable;
struct iPortal;
struct iSceneNode;
struct iSector;
struct iTerrainFactory;
struct iTerrainSystem;
struct iTriangleMesh;
struct iView;

namespace CS {
namespace Physics {

struct iPhysicalBody;
struct iPhysicalSector;
struct iPhysicalSystem;

}
}

namespace CS {
namespace Collisions {

struct iCollisionActor;
struct iCollisionObject;
struct iCollisionSector;
struct iCollisionSystem;
struct iCollisionTerrain;

/**
 * The type of a collision object.
 */
enum CollisionObjectType
{
  COLLISION_OBJECT_SIMPLE = 0,     /*!< The collision object is a simple static collision object.
				     It can never be upcast to a iPhysicalObject.*/
  COLLISION_OBJECT_PHYSICAL,       /*!< The collision object is a physical object and can be
				     upcast to a iPhysicalObject. */
  COLLISION_OBJECT_GHOST,          /*!< The collision object is a ghost. */
  COLLISION_OBJECT_ACTOR,          /*!< The collision object is an actor. */
  COLLISION_OBJECT_TERRAIN         /*!< The collision object is a terrain. */
};

/**
 * A structure used to return the result of hit beam.
 */
struct HitBeamResult
{
  HitBeamResult ()
  : hasHit (false), object (0), isect (0.0f), normal (0.0f), vertexIndex (0)
  {}

  /// Whether the beam has hit a body or not.
  bool hasHit;

  /// The collision object that was hit, or \a nullptr if no object was hit.
  iCollisionObject* object;

  /// Intersection point in world space.
  csVector3 isect;

  /// Normal to the surface of the body at the intersection point.
  csVector3 normal;

  /**
   * The index of the closest vertex of the soft body to be hit. This is only valid
   * if it is a soft body which is hit.
   */
  size_t vertexIndex;
};

/**
 * A structure used to describe a collision contact between two objects.
 */
struct iCollisionContact : public virtual iBase
{
  SCF_INTERFACE (CS::Collisions::iCollisionContact, 1, 0, 0);

  /// Get the collision position of A in world space.
  virtual csVector3 GetPositionOnA () const = 0;

  /// Get the collision position of B in world space.
  virtual csVector3 GetPositionOnB () const = 0;

  /// Get the normal of the hit position on A in world space.
  //virtual csVector3 GetNormalOnA () const = 0;

  /// Get the normal of the hit position on B in world space.
  virtual csVector3 GetNormalOnB () const = 0;

  /// Get the depth of penetration.
  virtual float GetPenetration () const = 0;
};

/**
 * A structure used to return the collision data between two objects.
 * When two objects are in contact, they have one or more
 * CS::Collisions::iCollisionContact.
 */
struct iCollisionData : public virtual iBase
{
  SCF_INTERFACE (CS::Collisions::iCollisionData, 1, 0, 0);

  /// Get the first collision object (A).
  virtual iCollisionObject* GetObjectA () const = 0;

  /// Get the second collision object (B).
  virtual iCollisionObject* GetObjectB () const = 0;

  /// Get the count of contacts between those two objects
  virtual size_t GetContactCount () const = 0;

  /// Get the contact at the given index
  virtual iCollisionContact* GetContact (size_t index) = 0;
};

/**
 * A list of CS::Collisions::iCollisionData.
 */
struct iCollisionDataList : public virtual iBase
{
  SCF_INTERFACE (CS::Collisions::iCollisionDataList, 1, 0, 0);

  /// Get the count of collisions in this list
  virtual size_t GetCollisionCount () const = 0;

  /// Get the collision at the given index
  virtual iCollisionData* GetCollision (size_t index) const = 0;
};

/**
 * This is the interface for attaching a collision callback to a collision object.
 *
 * Main ways to get pointers to this interface:
 * - application specific
 * 
 * Main users of this interface:
 * - iCollisionSystem
 */
struct iCollisionCallback : public virtual iBase
{
  SCF_INTERFACE (CS::Collisions::iCollisionCallback, 1, 0, 0);

  /**
   * A collision occurred.
   */
  virtual void OnCollision (iCollisionData* collision) = 0; 
};

/**
 * Collision groups allow to filter the collisions occuring between the objects in
 * the system. Each iCollisionObject is associated with a collision group, and the
 * user can define whether or not the objects from one group will collide with the
 * objects of another.
 *
 * There is a maximum of 16 collision groups that can be created in total, and those
 * groups cannot (currently) be removed once created. You should therefore be careful
 * when defining and managing your set of collision groups.
 *
 * The collision system will always create one default collision group named "Default".
 * This collision group is associated by default to all collision objects without any
 * valid group.
 * 
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateCollisionGroup()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionSystem::FindCollisionGroup()
 * 
 * Main users of this interface:
 * - iCollisionSystem, iCollisionObject
 */
struct iCollisionGroup : public virtual iBase
{
  /// Get the name of this collision group
  virtual const char* GetName () const = 0;

  /**
   * Set whether or not the objects from this group will collide with the objects of
   * the group \a other. By default, all groups will collide with all others.
   *
   * Note that it is valid to use the same group as the parameter and the one being
   * called. This defines whether or not the objects of the group will collide together.
   *
   * \param other The other group that will or not collide with this one.
   * \param enabled Whether or not the collisions are enabled between the two groups.
   */
  virtual void SetCollisionEnabled (iCollisionGroup* other, bool enabled) = 0;

  /**
   * Get whether or not the objects from this group will collide with the objects of
   * the group \a other.
   */
  virtual bool GetCollisionEnabled (iCollisionGroup* other) = 0;
};

/**
 * Factory to create instances of iCollisionObject.
 */
struct iCollisionObjectFactory : public virtual iBase
{
  SCF_INTERFACE (CS::Collisions::iCollisionObjectFactory, 1, 0, 0);

  /// Return the underlying object
  virtual iObject *QueryObject () = 0;

  /// Get the system of this factory
  virtual iCollisionSystem* GetSystem () const = 0;

  /// Create an instance
  virtual csPtr<iCollisionObject> CreateCollisionObject () = 0;

  /// Set the collider of this factory
  virtual void SetCollider (iCollider* value,
			    const csOrthoTransform& transform = csOrthoTransform ()) = 0;
  /// Get the collider of this factory
  virtual iCollider* GetCollider () const = 0;

  /// Set the relative transform of the collider of this object
  virtual void SetColliderTransform (const csOrthoTransform& transform) = 0;
  /// Get the relative transform of the collider of this object
  virtual const csOrthoTransform& GetColliderTransform () const = 0;

  /// Set the collision group of this factory
  virtual void SetCollisionGroup (iCollisionGroup* group) = 0;
  /// Get the collision group of this factory
  virtual iCollisionGroup* GetCollisionGroup () const = 0;
};

/**
 * This is the interface of a collision object. 
 *It contains the collision information of the object.
 * 
 * Main creators of instances implementing this interface:
 * - iCollisionObjectFactory::CreateCollisionObject()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionSystem::GetCollisionObject()
 * 
 * Main users of this interface:
 * - iCollisionSystem
 */
struct iCollisionObject : public virtual iBase
{
  SCF_INTERFACE (CS::Collisions::iCollisionObject, 1, 0, 0);

  /// Return the underlying object
  virtual iObject *QueryObject (void) = 0;

  /// Return the physical body pointer if it's a physical body, or nullptr otherwise.
  virtual CS::Physics::iPhysicalBody* QueryPhysicalBody () = 0;

  /// Return the actor pointer if it's an actor, or nullptr.
  virtual iCollisionActor* QueryCollisionActor () = 0;

  /**
   * Put this object into the given collision sector. The object will now be
   * part of the physical simulation.
   * \note This is equivalent to a call to
   * CS::Collisions::iCollisionSector::AddCollisionObject().
   */
  virtual void SetSector (iCollisionSector* sector) = 0;

  /**
   * Return the collision sector containing this object, or nullptr if it is not
   * in a sector (therefore not currently part of the physical simulation).
   */
  virtual iCollisionSector* GetSector () const = 0;

  /// Return the type of the collision object.
  virtual CollisionObjectType GetObjectType () const = 0;
  
  /// Set whether or not this object may be excluded from deactivation.
  virtual void SetDeactivable (bool d) = 0;
  /// Get whether or not this object may be excluded from deactivation.
  virtual bool GetDeactivable () const = 0;

  /// Set the iSceneNode attached to this collision object. Its transform will always coincide with the object's transform
  virtual void SetAttachedSceneNode (iSceneNode* sceneNode) = 0;
  /// Get the iSceneNode attached to this collision object. Its transform will always coincide with the object's transform
  virtual iSceneNode* GetAttachedSceneNode () const = 0;

  /**
   * Set the camera attached to this collision object. Its position will be updated
   * automatically when this object is moved.
   */
  virtual void SetAttachedCamera (iCamera* camera) = 0;

  /// Get the camera attached to this collision object.
  virtual iCamera* GetAttachedCamera () const = 0;

  /// Set the collider that defines this object's shape
  virtual void SetCollider (iCollider* collider,
			    const csOrthoTransform& transform = csOrthoTransform ()) = 0;

  /// Get the collider that defines this object's shape
  virtual iCollider* GetCollider () const = 0;

  /// Set the relative transform of the collider of this object
  virtual void SetColliderTransform (const csOrthoTransform& transform) = 0;

  /// Get the relative transform of the collider of this object
  virtual const csOrthoTransform& GetColliderTransform () const = 0;

  /// Set the transform of this object.
  virtual void SetTransform (const csOrthoTransform& trans) = 0;

  /// Get the transform of this object.
  virtual csOrthoTransform GetTransform () const = 0;
  
  /// Rebuild this collision object.
  // TODO: objects are not spawn correctly nor at the correct location if this
  // method is called before having the object in a sector
  //virtual void RebuildObject () = 0;

  /// Set the collision group of this object
  virtual void SetCollisionGroup (iCollisionGroup* group) = 0;

  /// Get the collision group of this object
  virtual iCollisionGroup* GetCollisionGroup () const = 0;

  /**
   * Set a callback to be executed when this body collides with another.
   * If nullptr, then no callback is executed.
   * \todo Currently no callback will be triggered unless the test method Collide() is used.
   */
  virtual void SetCollisionCallback (iCollisionCallback* cb) = 0;

  /// Get the collision response callback.
  virtual iCollisionCallback* GetCollisionCallback () = 0;

  /// Test for the collisions with another collision object
  // TODO: really useful?
  virtual csPtr<iCollisionData> Collide (iCollisionObject* otherObject) const = 0;

  /// Follow a beam from start to end and return whether or not this body was hit.
  // TODO: add a collision filter parameter
  virtual HitBeamResult HitBeam (const csVector3& start, const csVector3& end) const = 0;

  /// Get the count of collision objects in contact with this object.
  virtual size_t GetContactObjectsCount () = 0;

  /// Get the collision object in contact with this object at the given index.
  virtual iCollisionObject* GetContactObject (size_t index) = 0;
};

/**
 * Factory to create instances of iCollisionTerrain.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateCollisionTerrainFactory()
 */
struct iCollisionTerrainFactory : public virtual iCollisionObjectFactory
{
  SCF_INTERFACE (CS::Collisions::iCollisionTerrainFactory, 1, 0, 0);

  /// Get the terrain mesh factory associated with this collision object.
  virtual iTerrainFactory* GetTerrainFactory () const = 0;

  /// Create an instance
  virtual csPtr<iCollisionTerrain> CreateTerrain (iTerrainSystem* system) = 0;
};

/**
 * A collision terrain is associated with a iTerrainSystem mesh, and consists
 * of multiple cell colliders (CS::Collisions::iColliderTerrainCell objects).
 *
 * This collision object is particular in the sense that it manages automatically
 * by itself the addition and removal of cell colliders.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionTerrainFactory::CreateInstance()
 * 
 * Main users of this interface:
 * - iCollisionSector
 */
struct iCollisionTerrain : public virtual iCollisionObject
{
  SCF_INTERFACE (CS::Collisions::iCollisionTerrain, 1, 0, 0);

  /// Get the terrain system.
  virtual iTerrainSystem* GetTerrain () const = 0;

  /// Get the terrain cell collider with the given index
  virtual iColliderTerrainCell* GetCell (size_t index) const = 0;

  /// Get the terrain cell collider associated with the given terrain cell
  virtual iColliderTerrainCell* GetCell (iTerrainCell* cell) const = 0;

  /// Get the count of terrain cell colliders in this collision terrain
  virtual size_t GetCellCount () const = 0;
};

/**
 * A iCollisionActor is dedicated to actor motion controlling, and can be used to manage
 * eg a Player or a Non-Player Character object.
 *
 * Although most physical bodies cannot be moved freely in the virtual environment for
 * stability reasons, actors can be used to move objects around the environnement, while
 * keeping reactions to the collisions that are generated and handling gravity. They will
 * also interact with the dynamic objects that are collided by pushing away the objects
 * that are hit.
 *
 * The actual collider that represents the actor always floats <step height>
 * above the ground to be able to move smoothly over terrain and small obstacles.
 *
 * \remark The collider of iCollisionActor must be a convex shape, for example a
 * capsule or a sphere.
 *
 * Main ways to create instances implementing this interface:
 * - CS::Collisions::iCollisionSystem::CreateCollisionActorFactory()
 *
 * Main users of this interface:
 * - iCollisionSystem
 */
struct iCollisionActorFactory : public virtual iCollisionObjectFactory
{
  SCF_INTERFACE (CS::Collisions::iCollisionActorFactory, 1, 0, 0);

  /// Create an instance of a collision actor
  virtual csPtr<iCollisionActor> CreateCollisionActor () = 0;

  /// Get the max vertical threshold that this actor can step over
  virtual float GetStepHeight () const = 0;
  /// Set the max vertical threshold that this actor can step over
  virtual void SetStepHeight (float h) = 0;

  /**
   * Get the maximum slope, in radians. The maximum slope determines the maximum
   * angle that the actor can walk up.
   */
  virtual float GetMaximumSlope () const = 0;
  /**
   * The maximum slope determines the maximum angle that the actor can walk up.
   * The slope angle is measured in radians. The default value is 0.7854f (45 degree).
   */
  virtual void SetMaximumSlope (float slope) = 0;

  /// Get the jump speed
  // TODO: use a speed parameter in Jump() instead?
  virtual float GetJumpSpeed () const = 0;
  /// Set the jump speed
  virtual void SetJumpSpeed (float s)  = 0;

  /// Determines how much the actor can control movement when free falling (1 = completely, 0 = not at all)
  //virtual float GetAirControlFactor () const = 0;
  /// Determines how much the actor can control movement when free falling (1 = completely, 0 = not at all)
  //virtual void SetAirControlFactor (float f) = 0;
};

/**
 * A iCollisionActor is dedicated to actor motion controlling, and can be used to manage
 * eg a Player or a Non-Player Character object.
 *
 * Although most physical bodies cannot be moved freely in the virtual environment for
 * stability reasons, actors can be used to move objects around the environnement, while
 * keeping reactions to the collisions that are generated and handling gravity. They will
 * also interact with the dynamic objects that are collided by pushing away the objects
 * that are hit.
 *
 * The actual collider that represents the actor always floats <step height>
 * above the ground to be able to move smoothly over terrain and small obstacles.
 *
 * \remark The collider of iCollisionActor must be a convex shape, for example a
 * capsule or a sphere.
 *
 * Main ways to create instances implementing this interface:
 * - CS::Collisions::iCollisionActorFactory::CreateCollisionActor()
 * 
 * Main users of this interface:
 * - iCollisionSector
 */
struct iCollisionActor : public virtual iCollisionObject
{
  SCF_INTERFACE (CS::Collisions::iCollisionActor, 1, 0, 0);

  /// Get the maximum vertical threshold that this actor can step over
  virtual float GetStepHeight () const = 0;
  /// Set the maximum vertical threshold that this actor can step over
  virtual void SetStepHeight (float h) = 0;

  /**
   * Get the maximum slope, in radians. The maximum slope determines the maximum
   * angle that the actor can walk up.
   */
  virtual float GetMaximumSlope () const = 0;
  /**
   * The maximum slope determines the maximum angle that the actor can walk up.
   * The slope angle is measured in radians. The default value is 0.7854f (45 degree).
   */
  virtual void SetMaximumSlope (float slope) = 0;

  /**
   * Set the current motion speed of this actor.
   * \param speed Relative movement vector in object space of the model (i.e.
   * 0,0,1 will move the model forward). 
   */
  virtual void SetSpeed (const csVector3& speed) = 0;
  //virtual csVector3 GetSpeed () const = 0;

  /// Get the jump speed of this actor
  virtual float GetJumpSpeed () const = 0;
  /// Set the jump speed of this actor
  virtual void SetJumpSpeed (float s) = 0;
/*  
  /// Determines how much the actor can control movement when free falling
  //virtual float GetAirControlFactor () const = 0;
  /// Determines how much the actor can control movement when free falling
  //virtual void SetAirControlFactor (float f) = 0;
  */
  /**
   * Apply the given delta rotation on this actor.
   * \param yaw Is the delta angle to be applied around the Y axis
   */
  virtual void Rotate (float yaw) = 0;

  /// Applies an upward impulse to this actor, and an inverse impulse to objects beneath
  virtual void Jump () = 0;

  /**
   * Return whether or not this actor touches the ground
   */
  virtual bool IsOnGround () const = 0;
};

/**
 * This is the interface for the collision sector.
 * It handles all collision detection of collision objects.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateCollisionSector()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionSystem::FindCollisionSector()
 *
 * \sa CS::Physics::iPhysicalSector CS::Physics::iPhysicalSector
 */
struct iCollisionSector : public virtual iBase
{
  SCF_INTERFACE (CS::Collisions::iCollisionSector, 1, 0, 0);

  /**\name General methods
   * @{ */

  /// Return the system that this sector belongs to
  virtual CS::Collisions::iCollisionSystem* GetSystem () const = 0;

  /// Return the underlying object
  virtual iObject* QueryObject (void) const = 0;

  /**
   * Return the type of this sector, that is either CS::Collisions::COLLISION_OBJECT_PHYSICAL
   * For sectors that can be upcast to a iPhysicalSector, or CS::Collisions::COLLISION_OBJECT_SIMPLE
   * for sectors that cannot be upcast to a iPhysicalSector.
   */
  virtual CollisionObjectType GetSectorType () const = 0;

  /**
   * Return a reference to the physical interface of this sector if its type is
   * CS::Collisions::COLLISION_OBJECT_PHYSICAL, or nullptr otherwise.
   */
  virtual CS::Physics::iPhysicalSector* QueryPhysicalSector () const = 0;

  /**
   * Set the engine iSector related to this collision sector. The iMovable that are 
   * attached to a iCollisionObject present in this collision sector will be put
   * automatically in the given engine sector. The portals in iSector will be added
   * to this collision sector.
   */
  virtual void SetSector (iSector* sector) = 0;

  /// Get the engine iSector related to this collision sector.
  virtual iSector* GetSector () = 0;

  /// Set the global gravity.
  virtual void SetGravity (const csVector3& v) = 0;

  /// Get the global gravity.
  virtual csVector3 GetGravity () const = 0;

  /** @} */

  /**\name Collision objects management
   * @{ */

  /**
   * Add a collision object into the sector.
   */
  // TODO: remove: The collision object has to be initialized.
  // TODO: rename all AddXXX() methods in Add()
  virtual void AddCollisionObject (iCollisionObject* object) = 0;

  /// Remove the given collision object from this sector
  virtual void RemoveCollisionObject (iCollisionObject* object) = 0;

  /// Get the count of collision objects.
  virtual size_t GetCollisionObjectCount () = 0;

  /// Get the collision object by index.
  virtual iCollisionObject* GetCollisionObject (size_t index) = 0;

  /** @} */

  /**\name Portals
   * @{ */

  /// Add a portal into the sector. Collision objects crossing a portal will be switched from iCollisionSector's.
  virtual void AddPortal (iPortal* portal, const csOrthoTransform& meshTrans) = 0;

  /// Remove the given portal from this sector.
  virtual void RemovePortal (iPortal* portal) = 0;

  /// Delete all objects in this collision sector.
  // TODO: mask for selecting the type/state/collgroup of the objects to be removed?
  // TODO: flag indicating whether the attached iSceneNode should be removed from the engine?
  virtual void DeleteAll () = 0;

  /** @} */

  /**\name Collision detection
   * @{ */

  /**
   * Follow a beam from start to end and return the first body that is hit. This version
   * ignores the portals and won't therefore cross them to another sector.
   * \param start The start of the beam
   * \param end The end of the beam
   */
  // TODO: allow resuming the beam test when an object has been hit in order to find the next
  // object on the beam
  virtual HitBeamResult HitBeam (const csVector3& start, const csVector3& end) const = 0;

  /**
   * Follow a beam from start to end and return the first body that is hit. This version
   * will cross any portal hit and continue the the hit test on the other side of it.
   * \param start The start of the beam
   * \param end The end of the beam
   * \deprecated Deprecated in 2.2. Use HitBeamPortals() instead
   */
  CS_DEPRECATED_METHOD_MSG("Deprecated in 2.2. Use HitBeamPortals() instead")
  virtual HitBeamResult HitBeamPortal (const csVector3& start, const csVector3& end) const = 0;

  /**
   * Follow a beam from start to end and return the first body that is hit. This version
   * will cross any portal hit and continue the the hit test on the other side of it.
   * \param start The start of the beam
   * \param end The end of the beam
   */
  virtual HitBeamResult HitBeamPortals (const csVector3& start, const csVector3& end) const = 0;

  /**
   * Perform a discrete collision test against all objects in this iCollisionSector,
   * and return the list of all collisions with the given object.
   */
  virtual csPtr<iCollisionDataList> CollisionTest (iCollisionObject* object) = 0;

  /** @} */
};

/**
 * This is the Collision plug-in. This plugin is a factory for creating
 * iCollider, iCollisionObject, iCollisionSector and iCollisionActor
 * entities. 
 *
 * Main creators of instances implementing this interface:
 * - Opcode plugin (crystalspace.collision.opcode2)
 * - Bullet plugin (crystalspace.physics.bullet2)
 * 
 * Main ways to get pointers to this interface:
 * - csQueryRegistry()
 * 
 * \sa CS::Physics::iPhysicalSystem
 */
// TODO: global collision callback
struct iCollisionSystem : public virtual iBase
{
  SCF_INTERFACE (CS::Collisions::iCollisionSystem, 3, 0, 0);

  /**\name General methods
   * @{ */

  /**
   * Return the physical system pointer if this interface is also implemented by
   * the system, or nullptr otherwise.
   */
  virtual CS::Physics::iPhysicalSystem* QueryPhysicalSystem () = 0;

  /**
   * Set the speed of the simulation, that is the time ratio that is applied
   * when updating the motion of the objects. The default value is 1.0f.
   *
   * If the speed is equal or lower than zero, then the simulation will not be
   * updated anymore (unless you call manually the method
   * CS::Physics::iPhysicalSystem::StepSimulation()).
   */
  virtual void SetSimulationSpeed (float speed) = 0;

  /**
   * Get the speed of the simulation, that is the time ratio that is applied
   * when updating the motion of the objects.
   */
  virtual float GetSimulationSpeed () const = 0;

  /// Reset the entire system and delete all sectors and collision groups
  virtual void DeleteAll () = 0;

  /** @} */

  /**\name Colliders
   * @{ */

  /**
   * Create an empty collider (it does not have a root shape, but only potentially
   * children). The collider type of this object is CS::Collisions::COLLIDER_COMPOUND.
   */
  virtual csPtr<iCollider> CreateCollider () = 0;

  /**
   * Create a convex mesh collider.
   *
   * \param mesh The triangle mesh defining the geometry of the collider.
   */
  virtual csPtr<iColliderConvexMesh> CreateColliderConvexMesh
    (iTriangleMesh* mesh) = 0;

  /**
   * Create a concave mesh collider.
   *
   * \param mesh The triangle mesh defining the geometry of the collider.
   * \param dynamicEnabled Whether or not this collider can be used for dynamic
   * concave rigid bodies. The simulation of dynamic concave objects must have
   * been enabled through iPhysicalSystem::SetDynamicConcaveEnabled(), otherwise
   * this parameter has no effect.
   */
  virtual csPtr<iColliderConcaveMesh> CreateColliderConcaveMesh
    (iTriangleMesh* mesh, bool dynamicEnabled = false) = 0;

  /**
   * Create a static scaled concave mesh collider. This collider is quite memory efficient
   * since it re-uses the data defined by an original CS::Collider::iColliderConcaveMesh
   * (which remains untouched).
   *
   * This collider is always static and cannot be toggled dynamic.
   *
   * \param collider The original collider that will be scaled by this collider.
   * \param scale The scale to apply on the original collider.
   */
  virtual csPtr<iColliderConcaveMeshScaled> CreateColliderConcaveMeshScaled (
    iColliderConcaveMesh* collider, const csVector3& scale) = 0;

  /// Create a cylinder collider, oriented along the y-axis
  virtual csPtr<iColliderCylinder> CreateColliderCylinder (float length, float radius) = 0;

  /// Create a box collider.
  virtual csPtr<iColliderBox> CreateColliderBox (const csVector3& size) = 0;

  /// Create a sphere collider.
  virtual csPtr<iColliderSphere> CreateColliderSphere (float radius) = 0;

  /// Create a capsule collider.
  virtual csPtr<iColliderCapsule> CreateColliderCapsule (float length, float radius) = 0;

  /// Create a cone collider.
  virtual csPtr<iColliderCone> CreateColliderCone (float length, float radius) = 0;

  /// Create a static plane collider.
  virtual csPtr<iColliderPlane> CreateColliderPlane (const csPlane3& plane) = 0;

  /** @} */

  /**\name Collision sectors
   * @{ */

  /**
   * Create a new collision sector and add it to the system's set. If \a sector is provided
   * then it will first search if a previous collision sector associated to it is already
   * available. No new collision sector would be created in this case, and the previous one
   * would be returned instead.
   */
  virtual iCollisionSector* CreateCollisionSector (iSector* sector = nullptr) = 0;
  
  /// Delete the given collision sector
  virtual void DeleteCollisionSector (iCollisionSector* sector) = 0;

  /// Delete all collision sectors.
  virtual void DeleteCollisionSectors () = 0;
  
  /// Return the amount of sectors in this system
  virtual size_t GetCollisionSectorCount () const = 0;

  /// Get a collision sector by index
  virtual iCollisionSector* GetCollisionSector (size_t index) = 0; 
  
  /// Find a collision sector by its associated iSector, or nullptr if it has not been found
  virtual iCollisionSector* FindCollisionSector (const iSector* sceneSector) = 0;

  /// Find a collision sector by name.
  virtual iCollisionSector* FindCollisionSector (const char* name) = 0;

  /** @} */

  /**\name Collision groups
   * @{ */

  /**
   * Create a collision group of the given name. Return nullptr if the group could
   * not be created. If a group with the given name already exists, then return a
   * reference to this group.
   * \warning You cannot create more than 16 collision groups in total.
   * \warning Collision groups cannot be removed once created.
   */
  virtual iCollisionGroup* CreateCollisionGroup (const char* name) = 0;

  /// Find the collision group of the given name, or return nullptr if it has not been found.
  virtual iCollisionGroup* FindCollisionGroup (const char* name) const = 0;

  /// Get the count of collision groups in this system
  virtual size_t GetCollisionGroupCount () const = 0;

  /// Get a collision group by its index
  virtual iCollisionGroup* GetCollisionGroup (size_t index) const = 0;

  /** @} */

  /**\name Collision object factories
   * @{ */

  /// Create a iCollisionObjectFactory
  virtual csPtr<iCollisionObjectFactory> CreateCollisionObjectFactory
    (CS::Collisions::iCollider* collider = nullptr) = 0;

  /// Create a iCollisionObjectFactory of type CS::Collisions::COLLISION_OBJECT_GHOST
  virtual csPtr<iCollisionObjectFactory> CreateGhostCollisionObjectFactory
    (CS::Collisions::iCollider* collider = nullptr) = 0;

  /// Create a iCollisionActorFactory
  virtual csPtr<iCollisionActorFactory> CreateCollisionActorFactory
    (CS::Collisions::iCollider* collider = nullptr) = 0;

  /// Create a iCollisionTerrainFactory
  virtual csPtr<iCollisionTerrainFactory> CreateCollisionTerrainFactory
    (iTerrainFactory* terrain) = 0;

  /** @} */
};

} }

#endif
