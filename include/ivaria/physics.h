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
#ifndef __IVARIA_PHYSICS_H__
#define __IVARIA_PHYSICS_H__

/**\file
 * Physics interfaces
 */

#include "csgeom/tri.h"
#include "csutil/scf.h"
#include "csutil/scf_interface.h"
#include "iengine/mesh.h"
#include "iengine/engine.h"
#include "imesh/genmesh.h"
#include "iutil/objreg.h"
#include "ivaria/collisions.h"

namespace CS { 
namespace Physics {

struct iJoint;
struct iKinematicCallback;
struct iPhysicalSystem;
struct iPhysicalSector;
struct iRigidBody;
struct iSoftBody;
struct iVehicle;
struct iVehicleFactory;
struct iVehicleWheelFactory;

/**
 * The state of a rigid body.
 */
enum RigidBodyState
{
  STATE_STATIC = 0,    /*!< The body is in the static state. */
  STATE_DYNAMIC,       /*!< The body is in the dynamic state. */
  STATE_KINEMATIC      /*!< The body is in the kinematic state. */
};

/**
 * The type of a physical body.
 */
enum PhysicalObjectType
{
  PHYSICAL_OBJECT_RIGIDBODY = 0,  /*!< The physical object is a rigid body. */
  PHYSICAL_OBJECT_SOFTBODY       /*!< The physical object is a soft body. */
};

/**
 * The debug mode to be used by the iPhysicalSystem.
 */
enum DebugMode
{
  DEBUG_NOTHING = 0,     /*!< Nothing will be displayed. */
  DEBUG_COLLIDERS = 1,   /*!< Display the colliders of the bodies. */
  DEBUG_AABB = 2,        /*!< Display the axis aligned bounding boxes of the bodies. */
  DEBUG_JOINTS = 4       /*!< Display the joint positions and limits. */
};

/**
 * The mode of duplication used for the faces of the mesh. That is, if the mesh is
 * double sided, and whether the vertices and triangles that are duplicated are
 * interleaved or contiguous.
 */
enum MeshDuplicationMode
{
  MESH_DUPLICATION_NONE = 0,
  /*!< The faces of the mesh are not double sided, i.e. the vertices and
	triangles are not duplicated. */
  MESH_DUPLICATION_INTERLEAVED,
  /*!< The faces of the mesh are double sided, and the duplicated vertices
	and triangles are interleaved with the original ones. */
  MESH_DUPLICATION_CONTIGUOUS
  /*!< The faces of the mesh are double sided, and the duplicated vertices
    and triangles are packed contiguously at the end of their buffer. */
};

/**
 * Collection of all properties of a physical object
 */
struct iPhysicalObjectFactory : public virtual CS::Collisions::iCollisionObjectFactory
{
  SCF_INTERFACE (CS::Physics::iPhysicalObjectFactory, 1, 0, 0);

  /// Get the PhysicalObjectType of the object whose data is stored in this properties object
  virtual CS::Physics::PhysicalObjectType GetPhysicalObjectType () const = 0;

  /// Get the density of all objects that will be constructed by this factory. The default value is 1.0f.
  virtual float GetDensity () const = 0;
  /// Set the density of all objects that will be constructed by this factory
  virtual void SetDensity (float value) = 0;
    
  /// Get the mass of all objects that will be constructed by this factory. The default value is 1.0f.
  // TODO: explain difference density/mass
  virtual float GetMass () const = 0;
  /// Set the mass of all objects that will be constructed by this factory
  virtual void SetMass (float value) = 0;

  /// Set the friction of all objects that will be constructed by this factory. The default value is 10.0f.
  virtual void SetFriction (float value) = 0;
  /// Get the friction of all objects that will be constructed by this factory
  virtual float GetFriction () const = 0;
    
  /// Whether this object is affected by gravity. Enabled by default.
  virtual bool GetGravityEnabled () const = 0;
  /// Whether this object is affected by gravity
  virtual void SetGravityEnabled (bool enabled) = 0;
};

/**
 * A base interface of physical bodies. 
 * iRigidBody and iSoftBody will be derived from this one.
 */
struct iPhysicalBody : public virtual CS::Collisions::iCollisionObject
{
  SCF_INTERFACE (CS::Physics::iPhysicalBody, 1, 0, 3);

  /// Get the type of this physical body.
  virtual PhysicalObjectType GetPhysicalObjectType () const = 0;

  /**
   * Query the iRigidBody interface of this body. It returns NULL if the
   * interface is not valid, ie GetPhysicalObjectType() is not CS::Physics::PHYSICAL_OBJECT_RIGIDBODY.
   */
  virtual iRigidBody* QueryRigidBody () = 0;

  /**
   * Query the iSoftBody interface of this body. It returns NULL if the
   * interface is not valid, ie GetPhysicalObjectType() is not CS::Physics::PHYSICAL_OBJECT_SOFTBODY.
   */
  virtual iSoftBody* QuerySoftBody () = 0;

  /**
   * Get whether this object is currently animated dynamically by the physical
   * simulation, that is if it is either a dynamic actor, a soft body, or a
   * dynamic rigid body. The only objects that are not dynamic are the rigid
   * bodies that are either in static or kinematic state.
   */
  virtual bool IsDynamic () const = 0;

  /// Set the enabled state of this body
  virtual void SetEnabled (bool enabled) = 0;

  /// Get the enabled state of this body
  virtual bool GetEnabled () const = 0;

  /// Set the total mass of this body.
  virtual void SetMass (float mass) = 0;

  /// Get the mass of this body.
  virtual float GetMass () const = 0;

  /// Get the density of the body.
  virtual float GetDensity () const = 0;

  /**
   * Set the density of this collider. If the mass of the body was not defined
   * then it will be computed from this. But iSoftBody must use SetMass instead. 
   * 
   * You should be really careful when using densities because most of the
   * game physics libraries do not work well when objects with large mass
   * differences interact. It is safer to artificially keep the mass of moving
   * objects in a safe range (from 1 to 100 kilogram for example).
   */
  virtual void SetDensity (float density) = 0;

  /// Return the volume of this body.
  virtual float GetVolume () const = 0;
  
  /// Add a force to the whole body.
  CS_DEPRECATED_METHOD_MSG("Deprecated in 2.2. Use ApplyImpulse() instead.")
  virtual void AddForce (const csVector3& force) = 0;
  
  /// Get the linear velocity (translational velocity component).
  virtual csVector3 GetLinearVelocity () const = 0;
  /// Set the linear velocity (translational velocity component).
  virtual void SetLinearVelocity (const csVector3& vel) = 0;

  /**
   * Set the friction of this body.
   * [0,1] for soft body.
   */
  virtual void SetFriction (float friction) = 0;

  /// Get the friction of this body.
  virtual float GetFriction () const = 0;

  /// Whether this object is affected by gravity
  virtual void SetGravityEnabled (bool enabled) = 0;
  /// Whether this object is affected by gravity
  virtual bool GetGravityEnabled () const = 0;

  /**
   * Apply an impulse on the physical body. The impulse is applied on the center
   * of gravity of the body.
   *
   * The impulse will be applied for the next simulation step. If you want a continuous
   * impulse, then you need to apply it manually at each step.
   *
   * \param impulse The impulse vector, in world coordinates.
   * \sa CS::Physics::iRigidBody::ApplyImpulse(), CS::Physics::iSoftBody::ApplyImpulse()
   */
  virtual void ApplyImpulse (const csVector3& impulse) = 0;
};

// TODO: There are a lot more configurable parameters - See btRigidBodyConstructionInfo:
/*
  ///best simulation results using zero restitution.
  btScalar			m_restitution;

  btScalar			m_linearSleepingThreshold;
  btScalar			m_angularSleepingThreshold;

  //Additional damping can help avoiding lowpass jitter motion, help stability for ragdolls etc.
  //Such damping is undesirable, so once the overall simulation quality of the rigid body dynamics system has improved, this should become obsolete
  bool				m_additionalDamping;
  btScalar			m_additionalDampingFactor;
  btScalar			m_additionalLinearDampingThresholdSqr;
  btScalar			m_additionalAngularDampingThresholdSqr;
  btScalar			m_additionalAngularDampingFactor;
*/

/**
 * Collection of all properties of a rigid body
 */
struct iRigidBodyFactory : public virtual iPhysicalObjectFactory
{
  SCF_INTERFACE (CS::Physics::iRigidBodyFactory, 1, 0, 0);

  /// Create a rigid body
  virtual csPtr<CS::Physics::iRigidBody> CreateRigidBody () = 0;

  /// Set the dynamic state of this rigid body factory.
  virtual void SetState (CS::Physics::RigidBodyState state) = 0;
  virtual CS::Physics::RigidBodyState GetState () const = 0;

  /// Set the elasticity of this rigid body factory.
  virtual void SetElasticity (float value) = 0;

  /// Get the elasticity of this rigid body factory.
  virtual float GetElasticity () const = 0;
  /**
   * Set the linear Damping for this rigid body. The damping correspond to
   * how much the movements of the objects will be reduced. It is a value
   * between 0 and 1, giving the ratio of speed that will be reduced
   * in one second. 0 means that the movement will not be reduced, while
   * 1 means that the object will not move.
   * The default value is 0.
   * \sa iDynamicSystem::SetLinearDamping()
   */
  virtual void SetLinearDamping (float d) = 0;

  /// Get the linear Damping for this rigid body.
  virtual float GetLinearDamping () const = 0;

  /**
   * Set the angular Damping for this rigid body. The damping correspond to
   * how much the movements of the objects will be reduced. It is a value
   * between 0 and 1, giving the ratio of speed that will be reduced
   * in one second. 0 means that the movement will not be reduced, while
   * 1 means that the object will not move.
   * The default value is 0.
   */
  virtual void SetAngularDamping (float d) = 0;

  /// Get the angular Damping for this rigid body.
  virtual float GetAngularDamping () const = 0;
};

/**
 * This is the interface for a rigid body.
 * It keeps all properties for the body.
 * It can also be attached to a movable or a bone,
 * to automatically update it.
 *
 * Main creators of instances implementing this interface:
 * - iPhysicalSystem::CreateRigidBody()
 * 
 *  Main ways to get pointers to this interface:
 * - iPhysicalSector::GetRigidBody()
 * 
 * Main users of this interface:
 * - iPhysicalSector
 *
 * \sa CS::Physics::iSoftBody
 */
struct iRigidBody : public virtual iPhysicalBody
{
  SCF_INTERFACE (CS::Physics::iRigidBody, 1, 0, 1);

  /// Get the current state of the body.
  virtual RigidBodyState GetState () const = 0;
  
  /// Set the current state of the body.
  virtual void SetState (RigidBodyState state) = 0;
  
  /// Get the elasticity of this rigid body.
  virtual float GetElasticity () = 0;
  /// Set the elasticity of this rigid body.
  virtual void SetElasticity (float elasticity) = 0;
  
  /// Get the angular velocity (rotation)
  virtual csVector3 GetAngularVelocity () const = 0;
  /// Set the angular velocity (rotation).
  virtual void SetAngularVelocity (const csVector3& vel) = 0;

  /// Add a torque (world space) (active for one timestep).
  CS_DEPRECATED_METHOD_MSG("Deprecated in 2.2. Use ApplyTorque() instead.")
  virtual void AddTorque (const csVector3& torque) = 0;

  /// Add a force (local space) (active for one timestep).
  CS_DEPRECATED_METHOD_MSG("Deprecated in 2.2. Use ApplyImpulse() instead.")
  virtual void AddRelForce (const csVector3& force) = 0;

  /// Add a torque (local space) (active for one timestep).
  CS_DEPRECATED_METHOD_MSG("Deprecated in 2.2. Use ApplyTorque() instead.")
  virtual void AddRelTorque (const csVector3& torque) = 0;

  /**
   * Add a force (world space) at a specific position (world space)
   * (active for one timestep)
   */
  CS_DEPRECATED_METHOD_MSG("Deprecated in 2.2. Use ApplyImpulse() instead.")
  virtual void AddForceAtPos (const csVector3& force,
      const csVector3& pos) = 0;

  /**
   * Add a force (world space) at a specific position (local space)
   * (active for one timestep)
   */
  CS_DEPRECATED_METHOD_MSG("Deprecated in 2.2. Use ApplyImpulse() instead.")
  virtual void AddForceAtRelPos (const csVector3& force,
      const csVector3& pos) = 0;

  /**
   * Add a force (local space) at a specific position (world space)
   * (active for one timestep)
   */
  CS_DEPRECATED_METHOD_MSG("Deprecated in 2.2. Use ApplyImpulse() instead.")
  virtual void AddRelForceAtPos (const csVector3& force,
      const csVector3& pos) = 0;

  /**
   * Add a force (local space) at a specific position (local space)
   * (active for one timestep)
   */
  CS_DEPRECATED_METHOD_MSG("Deprecated in 2.2. Use ApplyImpulse() instead.")
  virtual void AddRelForceAtRelPos (const csVector3& force,
      const csVector3& pos) = 0;

  /// Get the total force currently applied on this body (in world coordinates).
  virtual csVector3 GetTotalForce () const = 0;

  /// Get the total torque currently applied on this body (in world coordinates).
  virtual csVector3 GetTotalTorque () const = 0;

  /**
   * Set the callback to be used to update the transform of the kinematic body.
   * If no callback are provided then the dynamic system will use a default one.
   */
  // TODO: rename in Listener
  virtual void SetKinematicCallback (iKinematicCallback* cb) = 0;

  /// Get the callback used to update the transform of the kinematic body.
  virtual iKinematicCallback* GetKinematicCallback () = 0;

  /**
   * Set the linear Damping for this rigid body. The dampening correspond to
   * how much the movements of the objects will be reduced. It is a value
   * between 0 and 1, giving the ratio of speed that will be reduced
   * in one second. 0 means that the movement will not be reduced, while
   * 1 means that the object will not move.
   * The default value is 0.
   * \sa iDynamicSystem::SetLinearDamping ()
   */
  virtual void SetLinearDamping (float d) = 0;

  /// Get the linear Damping for this rigid body.
  virtual float GetLinearDamping () = 0;

  /**
   * Set the angular damping for this rigid body. The dampening correspond to
   * how much the movements of the objects will be reduced. It is a value
   * between 0 and 1, giving the ratio of speed that will be reduced
   * in one second. 0 means that the movement will not be reduced, while
   * 1 means that the object will not move.
   * The default value is 0.
   */
  virtual void SetAngularDamping (float d) = 0;

  /// Get the angular damping for this rigid body.
  virtual float GetAngularDamping () = 0;
  
  /// Get the angular factor for this rigid body.
  virtual csVector3 GetAngularFactor () const = 0;
  /**
   * Set the angular factor for this rigid body.
   * \todo Document me
   */
  virtual void SetAngularFactor (const csVector3& f) = 0;

  /**
   * Apply an impulse on the rigid body at the specified position.
   *
   * The impulse will be applied for the next simulation step. If you want a continuous
   * impulse, then you need to apply it manually at each step.
   *
   * \param impulse The impulse vector, in world coordinates.
   * \param position The position where the impulse is applied, in world coordinates.
   * \sa CS::Physics::iPhysicalBody::ApplyImpulse(), CS::Physics::iRigidBody::ApplyTorque()
   */
  virtual void ApplyImpulse (const csVector3& impulse, const csVector3& position) = 0;

  /**
   * Apply a rotational torque on the rigid body.
   *
   * The torque will be applied for the next simulation step. If you want a continuous
   * torque, then you need to apply it manually at each step.
   *
   * \param torque The torque vector, in world coordinates.
   * \sa CS::Physics::iPhysicalBody::ApplyImpulse(), CS::Physics::iRigidBody::ApplyImpulse()
   */
  virtual void ApplyTorque (const csVector3& torque) = 0;
};

/**
 * This class can be implemented in order to update the position of an anchor of a
 * CS::Physics::iSoftBody. This can be used to try to control manually the
 * position of a vertex of a soft body.
 *
 * \warning This feature uses a hack around the physical simulation of soft bodies
 * and may not always be stable. Use it at your own risk.
 * \sa CS::Physics::iSoftBody::AnchorVertex (size_t,iAnchorAnimationControl)
 */
struct iAnchorAnimationControl : public virtual iBase
{
  SCF_INTERFACE (CS::Physics::iAnchorAnimationControl, 1, 0, 0);

  /**
   * Return the new position of the anchor, in world coordinates.
   */
  virtual csVector3 GetAnchorPosition () const = 0;
};

/**
 * Factory for the creation of instances of iSoftBody
 */
struct iSoftBodyFactory : public virtual iPhysicalObjectFactory
{
  SCF_INTERFACE (CS::Physics::iSoftBodyFactory, 1, 0, 0);

  /// Create a soft body
  virtual csPtr<iSoftBody> CreateSoftBody () = 0;

  // TODO: Re-select an interesting set of soft body parameters

  /// Set linear stiffness coefficient [0,1]. The default value is 1.0f.
  virtual void SetLinearStiffness (float stiffness) = 0;

  /// Set area/angular stiffness coefficient [0,1]. The default value is 1.0f.
  virtual void SetAngularStiffness (float stiffness) = 0;

  /// Set volume stiffness coefficient [0,1]. The default value is 1.0f.
  virtual void SetVolumeStiffness (float stiffness) = 0;

  /// Set soft vs rigid hardness [0,1] (cluster only). The default value is 0.1f.
  //virtual void SetSRHardness (float hardness) = 0;

  /// Set soft vs kinetic hardness [0,1] (cluster only). The default value is 1.0f.
  //virtual void SetSKHardness (float hardness) = 0;

  /// Set soft vs soft hardness [0,1] (cluster only). The default value is 0.5f.
  //virtual void SetSSHardness (float hardness) = 0;

  /// Set soft vs rigid impulse split [0,1] (cluster only). The default value is 0.5f.
  //virtual void SetSRImpulse (float impulse) = 0;

  /// Set soft vs rigid impulse split [0,1] (cluster only). The default value is 0.5f.
  //virtual void SetSKImpulse (float impulse) = 0;

  /// Set soft vs rigid impulse split [0,1] (cluster only). The default value is 0.5f.
  //virtual void SetSSImpulse (float impulse) = 0;

  /// Set damping coefficient [0,1]. The default value is 0.0f.
  virtual void SetDamping (float damping) = 0;

  /// Set drag coefficient [0,+inf]. The default value is 0.0f.
  virtual void SetDrag (float drag) = 0;

  /// Set lift coefficient [0,+inf]. The default value is 0.0f.
  virtual void SetLift (float lift) = 0;

  /// Set pressure coefficient [-inf,+inf]. The default value is 0.0f.
  virtual void SetPressure (float pressure) = 0;

  /// Set volume conversation coefficient [0,+inf]. The default value is 0.0f.
  virtual void SetVolumeConversationCoefficient (float conversation) = 0;

  /// Set pose matching coefficient [0,1]. The default value is 0.0f.
  virtual void SetShapeMatchThreshold (float matching) = 0;

  /// Set rigid contacts hardness [0,1]. The default value is 1.0f.
  //virtual void SetRContactsHardness (float hardness) = 0;

  /// Set kinetic contacts hardness [0,1]. The default value is 0.1f.
  //virtual void SetKContactsHardness (float hardness) = 0;

  /// Set soft contacts hardness [0,1]. The default value is 1.0f.
  //virtual void SetSContactsHardness (float hardness) = 0;

  /// Set anchors hardness [0,1]. The default value is 0.7f.
  //virtual void SetAnchorsHardness (float hardness) = 0;

  /// Set true in order to use pose matching. The default value is \a false
  virtual void SetShapeMatching (bool match) = 0;

  /**
   * Generate bending constraints between the vertices of this soft body.
   * This can be used to make the body more rigid.
   * \param distance Maximum number of triangle edges that can exist
   * between two vertices in order to have a constraint generated for
   * this pair of vertices. Typical values are 2 or 3.
   */
  virtual void GenerateBendingConstraints (size_t distance) = 0;
};

/**
 * Used to create a one-dimensional softbody
 */
struct iSoftRopeFactory : public virtual iSoftBodyFactory
{
  SCF_INTERFACE (CS::Physics::iSoftRopeFactory, 1, 0, 0);

  /// Get the start position of the rope
  virtual const csVector3& GetStart () const = 0;
  /// Set the start position of the rope
  virtual void SetStart (const csVector3& v) = 0;
    
  /// Get the end position of the rope
  virtual const csVector3& GetEnd () const = 0;
  /// Set the end position of the rope
  virtual void SetEnd (const csVector3& v) = 0;
    
  /// Get the amount of nodes along the rope
  virtual size_t GetNodeCount () const = 0;
  /// Set the amount of nodes along the rope
  virtual void SetNodeCount (size_t c) = 0;
};

/**
 * Used to create a two-dimensional softbody, typically a cloth.
 * \sa CS::Physics::SoftBodyHelper
 */
struct iSoftClothFactory : public virtual iSoftBodyFactory
{
  SCF_INTERFACE (CS::Physics::iSoftClothFactory, 1, 0, 0);

  /// Get the four corners of the cloth
  virtual const csVector3* GetCorners () const = 0;

  /**
   * Set the four corners of the cloth
   * \param topLeft The position of the top left corner.
   * \param topRight The position of the top right corner.
   * \param bottomLeft The position of the bottom left corner.
   * \param bottomRight The position of the bottom right corner.
   * \param withDiagonals Whether there must be diagonal segments in the cloth
   * or not. Diagonal segments will make the cloth more rigid.
   * \remark You must call SetSoftBodyWorld() prior to this.
   */
  virtual void SetCorners (csVector3 topLeft, csVector3 topRight,
			   csVector3 bottomLeft, csVector3 bottomRight) = 0;

  /**
   * Get the two segment counts along the two primary axes
   * \param countH Number of horizontal segments in the cloth.
   * \param countV Number of vertical segments in the cloth.
   */
  virtual void GetSegmentCounts (size_t& countH, size_t& countV) const = 0;

  /**
   * Set the two segment counts along the two primary axes
   * \param countH Number of horizontal segments in the cloth.
   * \param countV Number of vertical segments in the cloth.
   */
  virtual void SetSegmentCounts (size_t countH, size_t countV) = 0;
    
  /**
   * Get whether or not there must be diagonal segments in the cloth.
   * Diagonal segments will make the cloth more rigid.
   */
  virtual bool GetDiagonals () const = 0;

  /**
   * Set whether or not there must be diagonal segments in the cloth.
   * Diagonal segments will make the cloth more rigid.
   */
  virtual void SetDiagonals (bool d) = 0;
};
  
/**
 * Used to create an arbitrary softbody defined by a given triangle mesh
 */
struct iSoftMeshFactory : public virtual iSoftBodyFactory
{
  SCF_INTERFACE (CS::Physics::iSoftMeshFactory, 1, 0, 0);

  /// Get the triangle mesh defining the shape of the soft body
  virtual iTriangleMesh* GetMesh () const = 0;
  /// Set the triangle mesh defining the shape of the soft body
  virtual void SetMesh (iTriangleMesh* mesh) = 0;

  /// Get the duplication mode being used on the vertices of the triangle mesh
  virtual MeshDuplicationMode GetDuplicationMode () const = 0;
  /// Set the duplication mode being used on the vertices of the triangle mesh
  virtual void SetDuplicationMode (MeshDuplicationMode mode) = 0;
};

/**
 * A soft body is a physical body that can be deformed by the physical
 * simulation. It can be used to simulate eg ropes, clothes or any soft
 * volumetric object.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateCollisionObject()
 * 
 * Main ways to get pointers to this interface: 
 * - iPhysicalSector::GetSoftBody()
 * 
 * Main users of this interface:
 * - iPhysicalSector
 *
 * \sa CS::Physics::iRigidBody CS::Physics::iSoftBody CS::Animation::iSoftBodyAnimationControl
 */
struct iSoftBody : public virtual iPhysicalBody
{
  SCF_INTERFACE (CS::Physics::iSoftBody, 1, 0, 1);

  /// Set the mass of a node by index.
  virtual void SetVertexMass (float mass, size_t index) = 0;

  /// Get the mass of a node by index.
  virtual float GetVertexMass (size_t index) = 0;

  /// Return the count of vertices of this soft body.
  virtual size_t GetVertexCount () = 0;

  /// Return the position in world coordinates of the given vertex.
  virtual csVector3 GetVertexPosition (size_t index) const = 0;

  /// Return the count of triangles of this soft body.
  virtual size_t GetTriangleCount () = 0;

  /// Return the triangle with the given index.
  virtual csTriangle GetTriangle (size_t index) const = 0;

  /// Return the normal vector in world coordinates for the given vertex.
  virtual csVector3 GetVertexNormal (size_t index) const = 0;

  /// Anchor the given vertex to its current position. This vertex will no more move.
  virtual void AnchorVertex (size_t vertexIndex) = 0;

  /**
   * Anchor the given vertex to the given rigid body. The relative position of the
   * vertex and the body will remain constant.
   */
  virtual void AnchorVertex (size_t vertexIndex,
      iRigidBody* body) = 0;

  /**
   * Anchor the given vertex to the given controller. The relative position of the
   * vertex and the controller will remain constant.
   */
  virtual void AnchorVertex (size_t vertexIndex,
      iAnchorAnimationControl* controller) = 0;

  /**
   * Update the position of the anchor of the given vertex relatively to the anchored
   * rigid body. This can be used to have a finer control of the anchor position
   * relatively to the rigid body.
   *
   * This would work only if you called AnchorVertex (size_t,iRigidBody*) before.
   * The position to be provided is in world coordinates.
   *
   * \warning The stability of the simulation can be lost if you move the position too far
   * from the previous position.
   * \sa CS::Animation::iSoftBodyAnimationControl::CreateAnimatedMeshAnchor()
   */
  virtual void UpdateAnchor (size_t vertexIndex,
      csVector3& position) = 0;

  /**
   * Remove the given anchor. This won't work if you anchored the vertex to a rigid body, due
   * to a limitation in the Bullet library.
   */
  virtual void RemoveAnchor (size_t vertexIndex) = 0;

  /// Set the linear stiffness coefficient [0,1]. The default value is 1.0f.
  virtual void SetLinearStiffness (float stiffness) = 0;

  /// Set the area/angular stiffness coefficient [0,1]. The default value is 1.0f.
  virtual void SetAngularStiffness (float stiffness) = 0;

  /// Set the volume stiffness coefficient [0,1]. The default value is 1.0f.
  virtual void SetVolumeStiffness (float stiffness) = 0;

  /// Reset the collision flag to 0.
  //virtual void ResetCollisionFlag () = 0;

  /// Set true if use cluster vs convex handling for rigid vs soft collision detection.
  //virtual void SetClusterCollisionRS (bool cluster) = 0;

  /// Get true if use cluster vs convex handling for rigid vs soft collision detection.
  //virtual bool GetClusterCollisionRS () = 0;

  /// Set true if use cluster vs cluster handling for soft vs soft collision detection.
  //virtual void SetClusterCollisionSS (bool cluster) = 0;

  /// Get true if use cluster vs cluster handling for soft vs soft collision detection.
  //virtual bool GetClusterCollisionSS () = 0;

  /// Set soft vs rigid hardness [0,1] (cluster only). The default value is 0.1f.
  //virtual void SetSRHardness (float hardness) = 0;

  /// Set soft vs kinetic hardness [0,1] (cluster only). The default value is 1.0f.
  //virtual void SetSKHardness (float hardness) = 0;

  /// Set soft vs soft hardness [0,1] (cluster only). The default value is 0.5f.
  //virtual void SetSSHardness (float hardness) = 0;

  /// Set soft vs rigid impulse split [0,1] (cluster only). The default value is 0.5f.
  //virtual void SetSRImpulse (float impulse) = 0;

  /// Set soft vs rigid impulse split [0,1] (cluster only). The default value is 0.5f.
  //virtual void SetSKImpulse (float impulse) = 0;

  /// Set soft vs rigid impulse split [0,1] (cluster only). The default value is 0.5f.
  //virtual void SetSSImpulse (float impulse) = 0;

  /// Set velocities correction factor (Baumgarte).
  //virtual void SetVeloCorrectionFactor (float factor) = 0;

  /// Set damping coefficient [0,1]. The default value is 0.0f.
  virtual void SetDamping (float damping) = 0;

  /// Set drag coefficient [0,+inf]. The default value is 0.0f.
  virtual void SetDrag (float drag) = 0;

  /// Set lift coefficient [0,+inf]. The default value is 0.0f.
  virtual void SetLift (float lift) = 0;

  /// Set pressure coefficient [-inf,+inf]. The default value is 0.0f.
  virtual void SetPressure (float pressure) = 0;

  /// Set volume conversation coefficient [0,+inf]. The default value is 0.0f.
  virtual void SetVolumeConversationCoefficient (float conversation) = 0;

  /// Set pose matching coefficient [0,1]. The default value is 0.0f.
  virtual void SetShapeMatchThreshold (float matching) = 0;

  /// Set rigid contacts hardness [0,1]. The default value is 1.0f.
  //virtual void SetRContactsHardness (float hardness) = 0;

  /// Set kinetic contacts hardness [0,1]. The default value is 0.1f.
  //virtual void SetKContactsHardness (float hardness) = 0;

  /// Set soft contacts hardness [0,1]. The default value is 1.0f.
  //virtual void SetSContactsHardness (float hardness) = 0;

  /// Set anchors hardness [0,1]. The default value is 0.7f.
  // TODO: use specific values per anchors instead?
  //virtual void SetAnchorsHardness (float hardness) = 0;

  /// Set velocities solver iterations.
  //virtual void SetVeloSolverIterations (int iter) = 0;

  /// Set positions solver iterations.
  //virtual void SetPositionIterations (int iter) = 0;

  /// Set drift solver iterations.
  //virtual void SetDriftIterations (int iter) = 0;

  /// Set cluster solver iterations.
  //virtual void SetClusterIterations (int iter) = 0;

  /// Set true if use pose matching. The default value is \a false.
  virtual void SetShapeMatching (bool match) = 0;

  /**
   * Generate bending constraints between the vertices of this soft body.
   * This can be used to make the body more rigid.
   * \param distance Maximum number of triangle edges that can exist
   * between two vertices in order to have a constraint generated for
   * this pair of vertices. Typical values are 2 or 3.
   */
  // TODO: remove?
  virtual void GenerateBendingConstraints (size_t distance) = 0;

  /// Generate cluster for the soft body.
  //virtual void GenerateCluster (int iter) = 0;

  /// Count of all nodes
  //virtual size_t GetNodeCount () const = 0;

  /// The linear velocity of the given node
  virtual csVector3 GetLinearVelocity (size_t nodeIndex) const = 0;
  /// The linear velocity of the given node
  virtual void SetLinearVelocity (size_t nodeIndex, const csVector3& vel) = 0;

  /**
   * Set the wind velocity of the whole body.
   */
  virtual void SetWindVelocity (const csVector3& velocity) = 0;

  /// Get the wind velocity of the whole body.
  virtual const csVector3 GetWindVelocity () const = 0;

  /// Add a force at the given vertex of the body.
  CS_DEPRECATED_METHOD_MSG("Deprecated in 2.2. Use ApplyImpulse() instead.")
  virtual void AddForce (const csVector3& force, size_t vertexIndex) = 0;

  /**
   * Draw the debug informations of this soft body. This has to be called
   * at each frame, and will add 2D lines on top of the rendered scene.
   */
  virtual void DebugDraw (iView* rView) = 0;

  /**
   * Apply an impulse on the soft body at the position of the specified vertex.
   *
   * The impulse will be applied for the next simulation step. If you want a continuous
   * impulse, then you need to apply it manually at each step.
   *
   * \param impulse The impulse vector, in world coordinates.
   * \param vertexIndex The index of the vertex where the impulse is applied,
   * in world coordinates.
   * \sa CS::Physics::iPhysicalBody::ApplyImpulse()
   */
  virtual void ApplyImpulse (const csVector3& impulse, size_t vertexIndex) = 0;
};

/**
 * General helper class for CS::Physics::iSoftBody.
 */
// TODO: move in a lib tool class
struct SoftBodyHelper
{
  /**
   * Create a genmesh from the given cloth soft body. This may be useful eg if you
   * created the soft body using a iSoftClothFactory.
   * The genmesh will be double-sided, in order to have correct normals on both
   * sides of the cloth (ie the vertices of the soft body will be duplicated for the
   * genmesh). The duplication mode of the faces of the mesh generated is
   * CS::Physics::MESH_DUPLICATION_CONTIGUOUS, that parameter can therefore be used
   * e.g. in CS::Animation::iSoftBodyAnimationControl::SetSoftBody().
   * \param object_reg The object registry
   * \param factoryName The name of the factory to be created
   * \param scale The scale to be applied on the UV mapping.
   */
static csPtr<iMeshFactoryWrapper> CreateClothGenMeshFactory
(iObjectRegistry* object_reg, const char* factoryName, iSoftBody* cloth,
 csVector2 scale = csVector2 (1.0f))
  {
    csRef<iEngine> engine = csQueryRegistry<iEngine> (object_reg);

    // Create the cloth mesh factory.
    csRef<iMeshFactoryWrapper> clothFact = engine->CreateMeshFactory
      ("crystalspace.mesh.object.genmesh", factoryName);
    if (!clothFact)
      return 0;

    csRef<iGeneralFactoryState> gmstate = scfQueryInterface<iGeneralFactoryState>
      (clothFact->GetMeshObjectFactory ());

    // Create the vertices and normals of the genmesh
    size_t vertexCount = cloth->GetVertexCount ();
    gmstate->SetVertexCount (int (vertexCount * 2));
    csVector3* vertices = gmstate->GetVertices ();
    csVector3* normals = gmstate->GetNormals ();

    for (size_t i = 0; i < vertexCount; i++)
    {
      vertices[i] = cloth->GetVertexPosition (i);
      vertices[i + vertexCount] = cloth->GetVertexPosition (i);
      
      normals[i] = cloth->GetVertexNormal (i);
      normals[i + vertexCount] = cloth->GetVertexNormal (i);
      normals[i] = csVector3 (0.f, 0.f, 1.f);
      normals[i + vertexCount] = csVector3 (0.f, 0.f, 1.f);
    }

    // Create the triangles of the genmesh
    gmstate->SetTriangleCount (int (cloth->GetTriangleCount ()) * 2);
    csTriangle* triangles = gmstate->GetTriangles ();
    for (size_t i = 0; i < cloth->GetTriangleCount (); i++)
    {
      csTriangle triangle = cloth->GetTriangle (i);
      triangles[i * 2] = triangle;
      triangles[i * 2 + 1] = csTriangle (int (triangle[2] + vertexCount),
					 int (triangle[1] + vertexCount),
					 int (triangle[0] + vertexCount));
    }

    // Set up the texels of the genmesh
    csVector2* texels = gmstate->GetTexels ();
    for (size_t i = 0; i < vertexCount * 2; i++)
    {
      csVector3 p = vertices[i];
      csVector3 n = normals[i];
      csVector3 a1, a2;
      csPlane3::FindOrthogonalPoints (n, a1, a2);

      // Calculate the closest point from point 'p' on the plane given
      // by 'n' and the origin.
      csVector3 closest = p - n * (n * p);

      csVector2 uv;
      uv.x = a1 * closest * scale.x;
      uv.y = a2 * closest * scale.y;
      texels[i] = uv;
    }

    gmstate->Invalidate ();

    return csPtr<iMeshFactoryWrapper> (clothFact);
  }
};

/**
 * A joint that can constrain the relative motion between two iPhysicalBody.
 * For instance if all motion in along the local X axis is constrained
 * then the bodies will stay motionless relative to each other
 * along an x axis rotated and positioned by the joint's transform.
 *
 * Main creators of instances implementing this interface:
 * - iPhysicalSystem::CreateJoint()
 * 
 * Main users of this interface:
 * - iPhysicalSector
 */
struct iJointFactory : public virtual iBase
{
  SCF_INTERFACE (CS::Physics::iJointFactory, 1, 0, 0);

  /// Create a joint instance
  virtual csPtr<CS::Physics::iJoint> CreateJoint () = 0;

  /**
   * Set the translation constraints on the 3 axes. If true is
   * passed for an axis then the Joint will constrain all motion along
   * that axis (ie no motion will be allowed). If false is passed in then all motion along that
   * axis is free, but bounded by the minimum and maximum distance
   * if set. Set force_update to true if you want to apply the changes 
   * right away.
   */
  virtual void SetTransConstraints (bool X, bool Y, bool Z) = 0;

  /**
   * Get the translation constraints on the 3 axes. A value of \a true means that
   * no motion are allowed along that axis.
   */
  virtual void GetTransConstraints (bool& X, bool& Y, bool& Z) const = 0;

  /**
   * Set the minimum allowed distance between the two bodies. Set force_update to true if 
   * you want to apply the changes right away.
   */
  virtual void SetMinimumDistance (const csVector3& dist) = 0;

  /// Get the minimum allowed distance between the two bodies.
  virtual const csVector3& GetMinimumDistance () const = 0;

  /**
   * Set the maximum allowed distance between the two bodies. Set force_update to true if 
   * you want to apply the changes right away.
   */
  virtual void SetMaximumDistance (const csVector3& dist) = 0;

  /// Get the maximum allowed distance between the two bodies.
  virtual const csVector3& GetMaximumDistance () const = 0;

  /**
   * Set the rotational constraints on the 3 axes. If true is
   * passed for an axis then the Joint will constrain all rotation around
   * that axis (ie no motion will be allowed). If false is passed in then all rotation around that
   * axis is free, but bounded by the minimum and maximum angle
   * if set. Set force_update to true if you want to apply the changes 
   * right away.
   */
  virtual void SetRotConstraints (bool X, bool Y, bool Z) = 0;

  /**
   * Get the rotational constraints on the 3 axes. A value of \a true means that
   * no motion are allowed around that axis.
   */
  virtual void GetRotConstraints (bool& X, bool& Y, bool& Z) const = 0;

  /**
   * Set the minimum allowed angle between the two bodies, in radian. Set force_update to true if 
   * you want to apply the changes right away.
   */
  virtual void SetMinimumAngle (const csVector3& angle) = 0;

  /// Get the minimum allowed angle between the two bodies (in radian).
  virtual const csVector3& GetMinimumAngle () const = 0;

  /**
   * Set the maximum allowed angle between the two bodies (in radian). Set force_update to true if 
   * you want to apply the changes right away.
   */
  virtual void SetMaximumAngle (const csVector3& angle) = 0;

  /// Get the maximum allowed angle between the two bodies (in radian).
  virtual const csVector3& GetMaximumAngle () const = 0;

  /** 
   * Set the restitution of the joint's stop point (this is the 
   * elasticity of the joint when say throwing open a door how 
   * much it will bounce the door back closed when it hits).
   */
  virtual void SetBounce (const csVector3& bounce) = 0;

  /// Get the joint restitution.
  virtual const csVector3& GetBounce () const = 0;

  /**
   * Apply a motor velocity to joint (for instance on wheels). Set force_update to true if 
   * you want to apply the changes right away.
   */
  virtual void SetDesiredVelocity (const csVector3& velo) = 0;

  /// Get the desired velocity of the joint motor.
  virtual const csVector3& GetDesiredVelocity () const = 0;

  /**
   * Set the maximum force that can be applied by the joint motor to reach the desired velocity.
   * Set force_update to true if  you want to apply the changes right away.
   */
  virtual void SetMaxForce (const csVector3& force) = 0;

  /// Get the maximum force that can be applied by the joint motor to reach the desired velocity.
  virtual const csVector3& GetMaxForce () const = 0;

  /// Set this joint to a spring joint.
  virtual void SetSpring (bool isSpring) = 0;

  /// Set the linear stiffness of the spring.
  virtual void SetLinearStiffness (const csVector3& stiffness) = 0;

  /// Get the linear stiffness of the spring.
  virtual const csVector3& GetLinearStiffness () const = 0;

  /// Set the angular stiffness of the spring.
  virtual void SetAngularStiffness (const csVector3& stiffness) = 0;

  /// Get the angular stiffness of the spring.
  virtual const csVector3& GetAngularStiffness () const = 0;

  /// Set the linear damping of the spring.
  virtual void SetLinearDamping (const csVector3& damp) = 0;

  /// Get the linear damping of the spring.
  virtual const csVector3& GetLinearDamping () const = 0;

  /// Set the angular damping of the spring.
  virtual void SetAngularDamping (const csVector3& damp) = 0;

  /// Get the angular damping of the spring.
  virtual const csVector3& GetAngularDamping () const = 0;
  
  /// Set the value to an equilibrium point for translation.
  virtual void SetLinearEquilibriumPoint (const csVector3& point) = 0;

  /// Set the value to an equilibrium point for rotation.
  virtual void SetAngularEquilibriumPoint (const csVector3& point) = 0;

  /// Set the threshold of a breaking impulse.
  virtual void SetBreakingImpulseThreshold (float threshold) = 0;

  /// Get the threshold of a breaking impulse.
  virtual float GetBreakingImpulseThreshold () const = 0;
};

/**
 * A joint that can constrain the relative motion between two iPhysicalBody.
 * For instance if all motion in along the local X axis is constrained
 * then the bodies will stay motionless relative to each other
 * along an x axis rotated and positioned by the joint's transform.
 *
 * Main creators of instances implementing this interface:
 * - iPhysicalSystem::CreateJoint()
 * 
 * Main users of this interface:
 * - iPhysicalSector
 */
struct iJoint : public virtual iBase
{
  SCF_INTERFACE (CS::Physics::iJoint, 1, 0, 0);

  // TODO: remove forceUpdate
  // TODO: Set/GetSector

  /**
   * Set the rigid bodies that will be affected by this joint. Set force_update to true if 
   * you want to apply the changes right away.
   */
  virtual void Attach (iPhysicalBody* body1, iPhysicalBody* body2,
      bool forceUpdate = true) = 0;

  /// Get the attached body with the given index (valid values for body are 0 and 1).
  virtual iPhysicalBody* GetAttachedBody (int index) = 0;

  /**
   * Set the world transformation of the joint.
   * Set force_update to true if you want to apply the changes right away.
   */
  virtual void SetTransform (const csOrthoTransform& trans,
      bool forceUpdate = false) = 0;

  /// Get the world transformation of the joint.
  virtual csOrthoTransform GetTransform () const = 0;

  /// Set the new position of the joint, in world coordinates.
  virtual void SetPosition (const csVector3& position,
    bool forceUpdate = false) = 0;

  /// Get the current position of the joint, in world coordinates.
  virtual const csVector3& GetPosition () const = 0;

  /**
   * Set the translation constraints on the 3 axes. If true is
   * passed for an axis then the Joint will constrain all motion along
   * that axis (ie no motion will be allowed). If false is passed in then all motion along that
   * axis is free, but bounded by the minimum and maximum distance
   * if set. Set force_update to true if you want to apply the changes 
   * right away.
   */
  virtual void SetTransConstraints (bool X, 
      bool Y, bool Z, 
      bool forceUpdate = false) = 0;

  /**
   * Get the translation constraints on the 3 axes. A value of \a true means that
   * no motion are allowed along that axis.
   */
  virtual void GetTransConstraints (bool& X, bool& Y, bool& Z) const = 0;

  /**
   * Set the rotational constraints on the 3 axes. If true is
   * passed for an axis then the Joint will constrain all rotation around
   * that axis (ie no motion will be allowed). If false is passed in then all rotation around that
   * axis is free, but bounded by the minimum and maximum angle
   * if set. Set force_update to true if you want to apply the changes 
   * right away.
   */
  virtual void SetRotConstraints (bool X, 
      bool Y, bool Z, 
      bool forceUpdate = false) = 0;

  /**
   * Get the rotational constraints on the 3 axes. A value of \a true means that
   * no motion are allowed around that axis.
   */
  virtual void GetRotConstraints (bool& X, bool& Y, bool& Z) const = 0;

  /**
   * Set the minimum allowed distance between the two bodies. Set force_update to true if 
   * you want to apply the changes right away.
   */
  virtual void SetMinimumDistance (const csVector3& dist,
      bool forceUpdate = false) = 0;

  /// Get the minimum allowed distance between the two bodies.
  virtual csVector3 GetMinimumDistance () const = 0;

  /**
   * Set the maximum allowed distance between the two bodies. Set force_update to true if 
   * you want to apply the changes right away.
   */
  virtual void SetMaximumDistance (const csVector3& dist,
      bool forceUpdate = false) = 0;

  /// Get the maximum allowed distance between the two bodies.
  virtual csVector3 GetMaximumDistance () const = 0;

  /**
   * Set the minimum allowed angle between the two bodies, in radian. Set force_update to true if 
   * you want to apply the changes right away.
   */
  virtual void SetMinimumAngle (const csVector3& angle,
      bool forceUpdate = false) = 0;

  /// Get the minimum allowed angle between the two bodies (in radian).
  virtual const csVector3& GetMinimumAngle () const = 0;

  /**
   * Set the maximum allowed angle between the two bodies (in radian). Set force_update to true if 
   * you want to apply the changes right away.
   */
  virtual void SetMaximumAngle (const csVector3& angle,
      bool forceUpdate = false) = 0;

  /// Get the maximum allowed angle between the two bodies (in radian).
  virtual const csVector3& GetMaximumAngle () const = 0;

  /** 
   * Set the restitution of the joint's stop point (this is the 
   * elasticity of the joint when say throwing open a door how 
   * much it will bounce the door back closed when it hits).
   */
  virtual void SetBounce (const csVector3& bounce,
      bool forceUpdate = false) = 0;

  /// Get the joint restitution.
  virtual const csVector3& GetBounce () const = 0;

  /**
   * Apply a motor velocity to joint (for instance on wheels). Set force_update to true if 
   * you want to apply the changes right away.
   */
  virtual void SetDesiredVelocity (const csVector3& velo,
      bool forceUpdate = false) = 0;

  /// Get the desired velocity of the joint motor.
  virtual const csVector3& GetDesiredVelocity () const = 0;

  /**
   * Set the maximum force that can be applied by the joint motor to reach the desired velocity.
   * Set force_update to true if  you want to apply the changes right away.
   */
  virtual void SetMaxForce (const csVector3& force,
      bool forceUpdate = false) = 0;

  /// Get the maximum force that can be applied by the joint motor to reach the desired velocity.
  virtual csVector3 GetMaxForce () const = 0;

  /**
   * Rebuild the joint using the current setup. Return true if the rebuilding operation was successful
   * (otherwise the joint won't be active).
   */
  virtual bool RebuildJoint () = 0;

  /// Set this joint to a spring joint.
  virtual void SetSpring (bool isSpring, bool forceUpdate = false) = 0;

  /// Set the linear stiffness of the spring.
  virtual void SetLinearStiffness (const csVector3& stiffness, bool forceUpdate = false) = 0;

  /// Get the linear stiffness of the spring.
  virtual const csVector3& GetLinearStiffness () const = 0;

  /// Set the angular stiffness of the spring.
  virtual void SetAngularStiffness (const csVector3& stiffness, bool forceUpdate = false) = 0;

  /// Get the angular stiffness of the spring.
  virtual const csVector3& GetAngularStiffness () const = 0;

  /// Set the linear damping of the spring.
  virtual void SetLinearDamping (const csVector3& damp, bool forceUpdate = false) = 0;

  /// Get the linear damping of the spring.
  virtual const csVector3& GetLinearDamping () const = 0;

  /// Set the angular damping of the spring.
  virtual void SetAngularDamping (const csVector3& damp, bool forceUpdate = false) = 0;

  /// Get the angular damping of the spring.
  virtual const csVector3& GetAngularDamping () const = 0;
  
  /// Set the value to an equilibrium point for translation.
  virtual void SetLinearEquilibriumPoint (const csVector3& point, bool forceUpdate = false) = 0;

  /// Set the value to an equilibrium point for rotation.
  virtual void SetAngularEquilibriumPoint (const csVector3& point, bool forceUpdate = false) = 0;

  /// Set the threshold of a breaking impulse.
  virtual void SetBreakingImpulseThreshold (float threshold, bool forceUpdate = false) = 0;

  /// Get the threshold of a breaking impulse.
  virtual float GetBreakingImpulseThreshold () const = 0;
};

/**
 * A callback to be implemented when you are using kinematic bodies. If no
 * callback are provided then the dynamic system will use a default one which
 * will update the transform of the body from the position of the attached
 * movable (see iCollisionObject::SetAttachedSceneNode()).
 * \sa CS::Physics::iRigidBody::SetKinematicCallback()
 */
// TODO: iKinematicListener
struct iKinematicCallback : public virtual iBase
{
  SCF_INTERFACE (CS::Physics::iKinematicCallback, 1, 0, 0);

  /**
   * Update the new transform of the rigid body.
   */
  virtual void GetBodyTransform (iRigidBody* body,
				 csOrthoTransform& transform) const = 0;
};

/**
 * This is the interface for the physical sector.
 * It manage all physical bodies.
 *
 * \sa CS::Collisions::iCollisionSector CS::Physics::iPhysicalSector
 */
struct iPhysicalSector : public virtual CS::Collisions::iCollisionSector
{
  SCF_INTERFACE (CS::Physics::iPhysicalSector, 1, 0, 0);

  /**
   * Set the parameters for auto disabling the computation of forces on the
   * physical objects in this sector.
   * \param linear Maximum linear movement to disable a body. Default value is 0.8.
   * \param angular Maximum angular movement to disable a body. Default value is 1.0.
   * \param steps Minimum number of steps the body meets linear and angular
   * requirements before it is disabled. Default value is 0.
   * \param time Minimum time the body needs to meet linear and angular
   * movement requirements before it is disabled. Default value is 0.0.
   * \remark With the Bullet plugin, the 'steps' parameter is ignored.
   * \remark With the Bullet plugin, calling this method will not affect bodies already
   * created.
   */
  virtual void SetAutoDisableParams (float linear, float angular, float time) = 0;

  /**
   * Set the default linear damping for all objects in this sector. The dampening
   * correspond to how much the movements of the objects will be reduced. It is a
   * value between 0 and 1, giving the ratio of speed that will be reduced
   * in one second. 0 means that the movement will not be reduced, while
   * 1 means that the object will not move.
   * The default value is 0.1f.
   * \sa CS::Physics::iRigidBody::SetLinearDamping ()
   */
  virtual void SetLinearDamping (float damping) = 0;

  /**
   * Get the default linear damping for all objects in this sector.
   */
  virtual float GetLinearDamping () const = 0;

  /**
   * Set the default angular damping for all objects in this sector. The dampening
   * correspond to how much the movements of the objects will be reduced. It is a
   * value between 0 and 1, giving the ratio of speed that will be reduced
   * in one second. 0 means that the movement will not be reduced, while
   * 1 means that the object will not move.
   * The default value is 0.1f.
   * \sa CS::Physics::iRigidBody::SetAngularDamping()
   */
  virtual void SetAngularDamping (float damping) = 0;

  /// Get the default angular damping for all objects in this sector.
  virtual float GetAngularDamping () const = 0;
  
  /// Get the count of rigid bodies.
  virtual size_t GetRigidBodyCount () = 0;

  /// Get the rigid body by index.
  virtual iRigidBody* GetRigidBody (size_t index) = 0;

  /// Get the count of soft bodies.
  virtual size_t GetSoftBodyCount () = 0;

  /// Get the soft body by index.
  virtual iSoftBody* GetSoftBody (size_t index) = 0;

  /// Add a joint to the sector. The joint must have attached two physical bodies.
  virtual void AddJoint (iJoint* joint) = 0;

  /// Remove a joint from this sector.
  virtual void RemoveJoint (iJoint* joint) = 0;

  /// Add a vehicle to the sector.
  virtual void AddVehicle (iVehicle* vehicle) = 0;

  /// Remove a vehicle from this sector.
  virtual void RemoveVehicle (iVehicle* vehicle) = 0;

  /**
   * Save the current state of this sector in a file. The format of the file is
   * internal to the dynamic simulation library being used (most probably Bullet).
   * \return True if the operation succeeded, false otherwise.
   */
  virtual bool Save (const char* filename) = 0;
};

/**
 * This is the interface for the actual plugin.
 * It is responsible for creating iPhysicalSector.
 *
 * Main creators of instances implementing this interface:
 * - Bullet plugin (crystalspace.physics.bullet2)
 * 
 * Main ways to get pointers to this interface:
 * - csQueryRegistry()
 * 
 * Main users of this interface:
 * - Dynamics loader plugin (crystalspace.dynamics.loader)
 *
 * \sa CS::Collisions::iCollisionSystem
 */
// TODO: step callback
struct iPhysicalSystem : public virtual CS::Collisions::iCollisionSystem
{
  SCF_INTERFACE (CS::Physics::iPhysicalSystem, 1, 0, 1);
  
  /**
   * Set the parameters of the constraint solver. Use this if you want to find a
   * compromise between accuracy of the simulation and performance cost.
   * \param timeStep The internal, constant, time step of the simulation, in seconds.
   * A smaller value gives better accuracy. Default value is 1/60 s (ie 0.0166 s).
   * \param maxSteps Maximum number of steps that Bullet is allowed to take each
   * time you call iPhysicalSector::Step(). If you pass a very small time step as
   * the first parameter, then you must increase the number of maxSteps to
   * compensate for this, otherwise your simulation is 'losing' time. Default value
   * is 1. If you pass maxSteps=0 to the function, then it will assume a variable
   * tick rate. Don't do it.
   * \param iterations Number of iterations of the constraint solver. A reasonable
   * range of iterations is from 4 (low quality, good performance) to 20 (good
   * quality, less but still reasonable performance). Default value is 10. 
   */
  virtual void SetStepParameters (float timeStep, size_t maxSteps,
    size_t iterations) = 0;  

  /**
   * Step the simulation forward by the given duration, in second.
   *
   * By default, you should not call manually this method since the physical system
   * is stepped automatically (unless you specified specifically to not do it through the
   * method CS::Collisions::iCollisionSystem::SetSimulationSpeed()).
   */
  virtual void StepSimulation (float duration) = 0;

  /**
   * Set whether or not this physical system can handle soft bodies. The default
   * value is true.
   * \warning You have to call this method before creating any collision sectors.
   */
  virtual void SetSoftBodyEnabled (bool enabled) = 0; 

  /**
   * Return whether or not this physical system can handle soft bodies.
   */
  virtual bool GetSoftBodyEnabled () const = 0;

  /**
   * Set the internal scale to be applied to the whole dynamic world. Use this
   * to put back the range of dimensions you use for your objects to the one
   * Bullet was designed for.
   * 
   * Bullet does not work well if the dimensions of your objects are smaller
   * than 0.1 to 1.0 units or bigger than 10 to 100 units. Use this method to
   * fix the problem.
   * 
   * \warning You have to call this method before adding any objects in the
   * world, otherwise the objects won't have the same scale.
   */
  virtual void SetInternalScale (float scale) = 0;

  /**
   * Get the internal scale to be applied to the whole dynamic world.
   */
  virtual float GetInternalScale () const = 0;

  /**
   * Set the parameters for AutoDisable.
   * \param linear Maximum linear movement to disable a body. Default value is 0.8.
   * \param angular Maximum angular movement to disable a body. Default value is 1.0.
   * \param steps Minimum number of steps the body meets linear and angular
   * requirements before it is disabled. Default value is 0.
   * \param time Minimum time the body needs to meet linear and angular
   * movement requirements before it is disabled. Default value is 0.0.
   * \remark With the Bullet plugin, the 'steps' parameter is ignored.
   * \remark With the Bullet plugin, calling this method will not affect bodies already
   * created.
   */
  virtual void SetAutoDisableParams (float linear, float angular, float time) = 0;

  /**
   * Set the global linear damping of the whole system, that is for all
   * subsequent collision sectors that will be created.
   * The dampening correspond to how
   * much the movements of the objects will be reduced. It is a value
   * between 0 and 1, giving the ratio of speed that will be reduced
   * in one second. 0 means that the movement will not be reduced, while
   * 1 means that the object will not move.
   * The default value is 0.1f.
   * \sa CS::Physics::iRigidBody::SetLinearDamping ()
   */
  virtual void SetLinearDamping (float damping) = 0;

  /**
   * Get the global linear damping.
   */
  virtual float GetLinearDamping () const = 0;

  /**
   * Set the global angular damping of the whole system, that is for all
   * subsequent collision sectors that will be created.
   * The dampening correspond to how
   * much the movements of the objects will be reduced. It is a value
   * between 0 and 1, giving the ratio of speed that will be reduced
   * in one second. 0 means that the movement will not be reduced, while
   * 1 means that the object will not move.
   * The default value is 0.1f.
   * \sa CS::Physics::iRigidBody::SetAngularDamping()
   */
  virtual void SetAngularDamping (float damping) = 0;

  /// Get the global angular damping.
  virtual float GetAngularDamping () const = 0;
  
  /// Create a general 6DOF joint.
  virtual csPtr<iJointFactory> CreateJointFactory () = 0;

  /**
   * Create a P2P joint for rigid bodies by setting the position in 
   * world space.
   */
  virtual csPtr<iJointFactory> CreateP2PJointFactory () = 0;
  
  /**
   * Create a slide joint for rigid bodies.
   * \param trans The transform of the joint in world space.
   * \param minDist The min distance the body can move along the axis.
   * \param maxDist The max distance the body can move along the axis.
   * \param minAngle The min angle the body can rotate around the axis.
   * \param maxAngle The max angle the body can rotate around the axis.
   * \param axis The slide axis, can only be 0, 1, 2.
   */
  virtual csPtr<iJointFactory> CreateSlideJointFactory
    (float minDist, float maxDist, float minAngle, float maxAngle, int axis) = 0;

  /**
   * Create a hinge joint for rigid bodies.
   * \param position The position of the joint in world space.
   * \param minAngle The min angle the body can rotate around the axis.
   * \param maxAngle The max angle the body can rotate around the axis.
   * \param axis The axis of the hinge, can only be 0, 1, 2.
   */
  virtual csPtr<iJointFactory> CreateHingeJointFactory
    (float minAngle, float maxAngle, int axis) = 0;

  /**
   * Create a cone twist joint for rigid bodies.
   * \param trans The transform of the joint in world space.
   * \param swingSpan1 The swing span the body can rotate around the local Z axis of joint.
   * \param swingSpan2 The swing span the body can rotate around the local Y axis of joint.
   * \param twistSpan The twist span the body can rotate around the local X axis of joint.
   */
  virtual csPtr<iJointFactory> CreateConeTwistJointFactory
    (float swingSpan1,float swingSpan2,float twistSpan) = 0;

  /**
   * Create a linear joint for soft body by setting the position in 
   * world space.
   */
  virtual csPtr<iJointFactory> CreateSoftLinearJointFactory () = 0;

  /**
   * Create a angular joint for soft body by setting the rotation axis.
   * The axis can only be 0, 1, 2.
   */
  virtual csPtr<iJointFactory> CreateSoftAngularJointFactory (int axis) = 0;

  /**
   * Create a pivot joint to attach to a rigid body to a position in world space
   * in order to manipulate it.
   */
  virtual csPtr<iJointFactory> CreatePivotJointFactory () = 0;

  // Factories
  
  /// Create a rigid body factory
  virtual csPtr<iRigidBodyFactory> CreateRigidBodyFactory
    (CS::Collisions::iCollider* collider = nullptr) = 0;

  /// Create a soft rope factory
  virtual csPtr<iSoftRopeFactory> CreateSoftRopeFactory () = 0;

  /// Create a soft cloth factory
  virtual csPtr<iSoftClothFactory> CreateSoftClothFactory () = 0;

  /// Create a soft mesh factory
  virtual csPtr<iSoftMeshFactory> CreateSoftMeshFactory () = 0;

  // Vehicles

  /// Create a new factory to produce vehicles
  virtual csPtr<iVehicleFactory> CreateVehicleFactory
    (CS::Collisions::iCollider* collider = nullptr) = 0;
  
  /// Create a new factory to produce vehicle wheels
  virtual csPtr<iVehicleWheelFactory> CreateVehicleWheelFactory () = 0;
  
  /**
   * Draw the debug informations of the dynamic system. This has to be called
   * at each frame, and will add 2D lines on top of the rendered scene. The
   * objects to be displayed are defined by SetDebugMode().
   */
  virtual void DebugDraw (iView* rview) = 0;

  /**
   * Draw the debug informations of the dynamic system. This has to be called
   * at each frame, and will add 2D lines on top of the rendered scene. The
   * objects to be displayed are defined by SetDebugMode().
   * \param g3d The 3D renderer
   * \param camera The camera viewing the scene
   */
  virtual void DebugDraw (iGraphics3D* g3d, iCamera* camera) = 0;

  /**
   * Set the mode to be used when displaying debug informations. The default value
   * is 'CS::Physics::DEBUG_COLLIDERS | CS::Physics::DEBUG_JOINTS'.
   * \remark Don't forget to call DebugDraw() at each frame to effectively display
   * the debug informations.
   */
  virtual void SetDebugMode (DebugMode mode) = 0;

  /// Get the current mode used when displaying debug informations.
  virtual DebugMode GetDebugMode () = 0;  

  /**
   * Start the profiling of the simulation. This would add an overhead to the
   * computations, but allows to display meaningful information on the behavior
   * of the simulation.
   */ 
  virtual void StartProfile () = 0;

  /**
   * Stop the profiling of the simulation. This would add an overhead to the
   */
  virtual void StopProfile () = 0;

  /**
   * Dump the profile information on the standard output. StartProfile() must
   * have been called before.
   * \param resetProfile Whether or not the profile data must be reset after
   * the dumping.
   */
  virtual void DumpProfile (bool resetProfile = true) = 0;

  /**
   * Set whether or not this physical system can handle dynamic concave objects. The
   * default value is \a false.
   *
   * If set to \a false then all rigid bodies containing at least a
   * CS::Collisions::iColliderConcaveMesh or a
   * CS::Collisions::iColliderConcaveMeshScaled will remain static in all cases.
   *
   * Enabling dynamic concave object simulation may lead to performance or stability
   * issues. You might consider trying to use only convex colliders insteads, eg
   * obtained through the interface CS::Collisions::iConvexDecomposer.
   *
   * \warning You have to call this method before creating any collision sectors or
   * object factories.
   */
  virtual void SetDynamicConcaveEnabled (bool enabled) = 0; 

  /**
   * Return whether or not this physical system can handle dynamic concave objects.
   */
  virtual bool GetDynamicConcaveEnabled () const = 0;
};

}
}

#endif
