/*
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __CS_BULLET_PHYSICSFACTORIES_H__
#define __CS_BULLET_PHYSICSFACTORIES_H__

#include "igeom/trimesh.h"
#include "iutil/comp.h"
#include "csutil/scf.h"
#include "csutil/csstring.h"

#include "ivaria/physics.h"

class btCollisionObject;
class btCompoundShape;
class btDynamicsWorld;
class btCollisionDispatcher;
class btDefaultCollisionConfiguration;
class btSequentialImpulseConstraintSolver;
class btBroadphaseInterface;
struct btSoftBodyWorldInfo;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

  class csBulletSystem;
  class BulletSector;
  class CollisionGroup;

  /**
   * Base class for all object properties
   */
  class BulletCollisionObjectFactory : public scfVirtImplementationExt1<
    BulletCollisionObjectFactory, csObject, CS::Collisions::iCollisionObjectFactory> 
  {
    friend class csBulletSystem;

  public:
    // TODO: really used?
    csBulletSystem* system;
    csRef<CS::Collisions::iCollider> collider;
    CollisionGroup* group;
    csOrthoTransform transform;

  public:
    BulletCollisionObjectFactory (csBulletSystem* system, CS::Collisions::iCollider* collider);

    virtual iObject *QueryObject () { return this; }

    virtual CS::Collisions::iCollisionSystem* GetSystem () const
    { return (CS::Collisions::iCollisionSystem*) system; }

    // TODO: create a real object
    virtual csPtr<CS::Collisions::iCollisionObject> CreateCollisionObject ()
    { return csPtr<CS::Collisions::iCollisionObject> (nullptr); }

    virtual void SetCollider (CS::Collisions::iCollider* collider,
			      const csOrthoTransform& transform = csOrthoTransform ())
    {
      this->collider = collider;
      this->transform = transform;
    }
    virtual CS::Collisions::iCollider* GetCollider () const { return collider; }

    virtual void SetColliderTransform (const csOrthoTransform& transform);
    virtual const csOrthoTransform& GetColliderTransform () const;

    virtual void SetCollisionGroup (CS::Collisions::iCollisionGroup* group);
    virtual CS::Collisions::iCollisionGroup* GetCollisionGroup () const;
  };

  class BulletPhysicalObjectFactory : public scfVirtImplementationExt1<
    BulletPhysicalObjectFactory, BulletCollisionObjectFactory, CS::Physics::iPhysicalObjectFactory>
  {
  protected:
    float mass;
    float friction;
    bool gravityEnabled;

  public:
    BulletPhysicalObjectFactory (csBulletSystem* system, CS::Collisions::iCollider* collider = nullptr) : 
    scfImplementationType (this, system, collider),
      mass (1.0f), friction (10.0f),
      gravityEnabled (true)
    {}

    virtual float GetDensity () const;
    virtual void SetDensity (float value);
    
    virtual float GetMass () const
    { return mass; }
    virtual void SetMass (float value)
    { mass = value; }

    virtual void SetCollider (CS::Collisions::iCollider* collider,
			      const csOrthoTransform& transform = csOrthoTransform ());
 
    virtual void SetFriction (float value) { friction = value; }
    virtual float GetFriction () const { return friction; }
    
    virtual bool GetGravityEnabled () const { return gravityEnabled; }
    virtual void SetGravityEnabled (bool enabled) { gravityEnabled = enabled; }
  };

  class BulletRigidBodyFactory : public scfVirtImplementationExt1<
    BulletRigidBodyFactory, BulletPhysicalObjectFactory, CS::Physics::iRigidBodyFactory>
  {
    friend class csBulletRigidBody;

  protected:
    CS::Physics::RigidBodyState state;
    float elasticity;
    float linearDamping, angularDamping;

  public:
    BulletRigidBodyFactory (csBulletSystem* system, CS::Collisions::iCollider* collider = nullptr) : 
    scfImplementationType (this, system, collider), state (CS::Physics::STATE_DYNAMIC),
      elasticity (0.1f), linearDamping (0.01f), angularDamping (0.01f)
    {
    }

    virtual csPtr<CS::Physics::iRigidBody> CreateRigidBody ();
    virtual csPtr<CS::Collisions::iCollisionObject> CreateCollisionObject ();
    
    virtual CS::Physics::PhysicalObjectType GetPhysicalObjectType () const
    { return CS::Physics::PHYSICAL_OBJECT_RIGIDBODY; }

    virtual void SetState (CS::Physics::RigidBodyState state)
    {
      this->state = state;
      if (state == CS::Physics::STATE_DYNAMIC)
      {
	if (mass < SMALL_EPSILON)
	  mass = 1.0f;
      }
      else mass = 0.0f;
    }
    virtual CS::Physics::RigidBodyState GetState () const { return state; }

    virtual void SetElasticity (float value) { elasticity = value; }
    virtual float GetElasticity () const { return elasticity; }

    void SetLinearDamping (float d) { linearDamping = d; }
    float GetLinearDamping () const { return linearDamping; }

    virtual void SetAngularDamping (float d) { angularDamping = d; }
    virtual float GetAngularDamping () const { return angularDamping; }
  };

  class BulletSoftBodyFactory : public scfVirtImplementationExt1<
    BulletSoftBodyFactory, BulletPhysicalObjectFactory, CS::Physics::iSoftBodyFactory>
  {
    friend class csBulletSoftBody;

  protected:
    float linearStiffness;
    float angularStiffness;
    float volumeStiffness;
    float SRHhardness;
    float SKHhardness;
    float SSHhardness;
    float SRimpulse;
    float SKimpulse;
    float SSimpulse;
    float damping;
    float drag;
    float lift;
    float pressure;
    float conversation;
    float matching;
    float Rhardness;
    float Khardness;
    float Shardness;
    float anchorHardness;
    bool match;
    bool generateBending;
    size_t bendingDistance;

  public:
  BulletSoftBodyFactory (csBulletSystem* system)
    : scfImplementationType (this, system),
      linearStiffness (1.0f),
      angularStiffness (1.0f),
      volumeStiffness (1.0f),
      SRHhardness (0.1f),
      SKHhardness (1.0f),
      SSHhardness (0.5f),
      SRimpulse (0.5f),
      SKimpulse (0.5f),
      SSimpulse (0.5f),
      damping (.0f),
      drag (.0f),
      lift (.0f),
      pressure (.0f),
      conversation (.0f),
      matching (.0f),
      Rhardness (1.0f),
      Khardness (0.1f),
      Shardness (1.0f),
      anchorHardness (.7f),
      match (false),
      generateBending (false)
    {
      SetFriction (float (.2));    // between 0 and 1
    }

    virtual CS::Physics::PhysicalObjectType GetPhysicalObjectType () const
    { return CS::Physics::PHYSICAL_OBJECT_SOFTBODY; }

    /// Create a new object
    virtual csPtr<CS::Physics::iSoftBody> CreateSoftBody () = 0;
    virtual csPtr<CS::Collisions::iCollisionObject> CreateCollisionObject ();

    virtual void SetLinearStiffness (float stiffness)
    { linearStiffness = stiffness; }
    virtual void SetAngularStiffness (float stiffness)
    { angularStiffness = stiffness; }
    virtual void SetVolumeStiffness (float stiffness)
    { volumeStiffness = stiffness; }
    virtual void SetSRHardness (float hardness)
    { SRHhardness = hardness; }
    virtual void SetSKHardness (float hardness)
    { SKHhardness = hardness; }
    virtual void SetSSHardness (float hardness)
    { SSHhardness = hardness; }
    virtual void SetSRImpulse (float impulse)
    { SRimpulse = impulse;}
    virtual void SetSKImpulse (float impulse)
    { SKimpulse = impulse;}
    virtual void SetSSImpulse (float impulse)
    { SSimpulse = impulse;}
    virtual void SetDamping (float damping)
    { this->damping = damping; }
    virtual void SetDrag (float drag)
    { this->drag = drag; }
    virtual void SetLift (float lift)
    { this->lift = lift; }
    virtual void SetPressure (float pressure)
    { this->pressure = pressure; }
    virtual void SetVolumeConversationCoefficient (float conversation)
    { this->conversation = conversation; }
    virtual void SetShapeMatchThreshold (float matching)
    { this->matching = matching; }
    virtual void SetRContactsHardness (float hardness)
    { Rhardness = hardness; }
    virtual void SetKContactsHardness (float hardness)
    { Khardness = hardness; }
    virtual void SetSContactsHardness (float hardness)
    { Shardness = hardness; }
    virtual void SetAnchorsHardness (float hardness)
    { anchorHardness = hardness; }
    virtual void SetShapeMatching (bool match)
    { this->match = match; }
    virtual void GenerateBendingConstraints (size_t distance)
    { generateBending = true; bendingDistance = distance; }
  };

  class BulletSoftRopeFactory : public scfVirtImplementationExt1<
    BulletSoftRopeFactory, BulletSoftBodyFactory, CS::Physics::iSoftRopeFactory>
  {
  protected:
    csVector3 start, end;
    size_t nodeCount;

  public:
  BulletSoftRopeFactory (csBulletSystem* system) : scfImplementationType (this, system),
      start (0), end (0),
      nodeCount (10)
    {
    }

    virtual csPtr<CS::Physics::iSoftBody> CreateSoftBody ();

    virtual const csVector3& GetStart () const { return start; }
    virtual void SetStart (const csVector3& v) { start = v; }
    
    virtual const csVector3& GetEnd () const { return end; }
    virtual void SetEnd (const csVector3& v) { end = v; }
    
    virtual size_t GetNodeCount () const { return nodeCount; }
    virtual void SetNodeCount (size_t c) { nodeCount = c; }
  };

  class BulletSoftClothFactory : public scfVirtImplementationExt1<
    BulletSoftClothFactory, BulletSoftBodyFactory, CS::Physics::iSoftClothFactory> 
  {
  protected:
    csVector3 corners[4];
    size_t counts[2];
    bool withDiagonals;

  public:
  BulletSoftClothFactory (csBulletSystem* system) : scfImplementationType (this, system),
      withDiagonals (false)
    {
      for (size_t i = 0; i < 4; ++i) corners[i] = csVector3 (i);
      counts[0] = counts[1] = 10;
    }

    virtual csPtr<CS::Physics::iSoftBody> CreateSoftBody ();

    virtual const csVector3* GetCorners () const
    { return corners; }
    virtual void SetCorners (csVector3 topLeft, csVector3 topRight,
			     csVector3 bottomLeft, csVector3 bottomRight)
    {
      corners[0] = topLeft;
      corners[1] = topRight;
      corners[2] = bottomLeft;
      corners[3] = bottomRight;
    }

    virtual void GetSegmentCounts (size_t& count1, size_t& count2)
      const { count1 = counts[0]; count2 = counts[1]; }
    virtual void SetSegmentCounts (size_t count1, size_t count2)
    { counts[0] = count1; counts[1] = count2; }
    
    virtual bool GetDiagonals () const
    { return withDiagonals; }
    virtual void SetDiagonals (bool d)
    { withDiagonals = d; }
  };
  
  class BulletSoftMeshFactory : public scfVirtImplementationExt1<
    BulletSoftMeshFactory, BulletSoftBodyFactory, CS::Physics::iSoftMeshFactory> 
  {
  protected:
    csRef<iTriangleMesh> mesh;
    CS::Physics::MeshDuplicationMode mode;

  public:
    BulletSoftMeshFactory (csBulletSystem* system)
      : scfImplementationType (this, system), mode (CS::Physics::MESH_DUPLICATION_NONE)
    {
    }

    /// Create a new object
    virtual csPtr<CS::Physics::iSoftBody> CreateSoftBody ();

    virtual iTriangleMesh* GetMesh () const
    { return mesh; }
    virtual void SetMesh (iTriangleMesh* mesh)
    { this->mesh = mesh; }

    virtual CS::Physics::MeshDuplicationMode GetDuplicationMode () const
    { return mode; }
    virtual void SetDuplicationMode (CS::Physics::MeshDuplicationMode mode)
    { this->mode = mode; }
  };

}
CS_PLUGIN_NAMESPACE_END (Bullet2)

#endif
