/*
    Copyright (C) 2012 by Dominik Seifert

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

#include "physicsfactories.h"

using namespace CS::Collisions;
using namespace CS::Physics;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

  //----------------------- BulletCollisionObjectFactory -----------------------

  BulletCollisionObjectFactory::BulletCollisionObjectFactory
    (csBulletSystem* system, CS::Collisions::iCollider* collider)
    : scfImplementationType (this), system (system), collider (collider),
    group (system->GetDefaultGroup ())
  {}

  void BulletCollisionObjectFactory::SetCollisionGroup (CS::Collisions::iCollisionGroup* group)
  {
    this->group = dynamic_cast<CollisionGroup*> (group);
    if (!this->group) this->group = system->GetDefaultGroup ();
  }

  CS::Collisions::iCollisionGroup* BulletCollisionObjectFactory::GetCollisionGroup () const
  {
    return group;
  }

  void BulletCollisionObjectFactory::SetColliderTransform (const csOrthoTransform& transform)
  {
    this->transform = transform;
  }

  const csOrthoTransform& BulletCollisionObjectFactory::GetColliderTransform () const
  {
    return transform;
  }

  //----------------------- BulletPhysicalObjectFactory -----------------------

  float BulletPhysicalObjectFactory::GetDensity () const
  {
    if (mass < SMALL_EPSILON)
      return 0.0f;
    return mass / collider->GetVolume ();
  }

  void BulletPhysicalObjectFactory::SetDensity (float density)
  {
    if (collider && collider->IsDynamic ())
      mass = density * collider->GetVolume ();
  }

  void BulletPhysicalObjectFactory::SetCollider
    (CS::Collisions::iCollider* collider,
     const csOrthoTransform& transform)
  {
    this->collider = collider;
    this->transform = transform;
    // TODO: keep that?
    if (!collider || !collider->IsDynamic ())
      mass = 0.0f;
  }

  //----------------------- BulletRigidBodyFactory -----------------------

  csPtr<CS::Collisions::iCollisionObject> BulletRigidBodyFactory::CreateCollisionObject () 
  {
    csRef<CS::Physics::iRigidBody> body = CreateRigidBody ();
    return csPtr<CS::Collisions::iCollisionObject> (body);
  }

  csPtr<CS::Physics::iRigidBody> BulletRigidBodyFactory::CreateRigidBody ()
  {
    if (!collider)
    {
      system->ReportError ("No collider supplied when creating a rigid body");
      return csPtr<CS::Physics::iRigidBody> (nullptr);
    }

    return new csBulletRigidBody (this);
  }

  //----------------------- BulletSoftBodyFactory -----------------------

  csPtr<CS::Collisions::iCollisionObject> BulletSoftBodyFactory::CreateCollisionObject () 
  { 
    csRef<CS::Physics::iSoftBody> body = CreateSoftBody ();
    return csRef<CS::Collisions::iCollisionObject> (body);
  }

  csPtr<CS::Physics::iSoftBody> BulletSoftRopeFactory::CreateSoftBody ()
  { 
    btSoftBody* body = btSoftBodyHelpers::CreateRope
      (*system->GetSoftBodyWorldInfo (), CSToBullet (GetStart (), system->GetInternalScale ()),
      CSToBullet (GetEnd (), system->GetInternalScale ()), int (GetNodeCount ()) - 1, 0);

    //hard-coded parameters for hair ropes
    body->m_cfg.kDP = 0.08f; // damping
    body->m_cfg.piterations = 16; // no white zone
    body->m_cfg.timescale = 2;

    csRef<csBulletSoftBody> csBody;
    csBody.AttachNew (new csBulletSoftBody (system, body));
    csBody->CreateSoftBodyObject (this);
    return csPtr<CS::Physics::iSoftBody> (csBody);
  }

  csPtr<CS::Physics::iSoftBody> BulletSoftClothFactory::CreateSoftBody ()
  {
    btSoftBody* body = btSoftBodyHelpers::CreatePatch
      (*system->GetSoftBodyWorldInfo (), CSToBullet (corners[0], system->GetInternalScale ()),
      CSToBullet (corners[1], system->GetInternalScale ()),
      CSToBullet (corners[2], system->GetInternalScale ()),
      CSToBullet (corners[3], system->GetInternalScale ()), 
      int (counts[0]), 
      int (counts[1]), 0,
      withDiagonals);
    body->m_cfg.collisions |= btSoftBody::fCollision::VF_SS;

    csRef<csBulletSoftBody> csBody;
    csBody.AttachNew (new csBulletSoftBody (system, body));
    csBody->CreateSoftBodyObject (this);
    return csPtr<CS::Physics::iSoftBody> (csBody);
  }

  csPtr<CS::Physics::iSoftBody> BulletSoftMeshFactory::CreateSoftBody ()
  {
    if (!mesh) return csPtr<CS::Physics::iSoftBody> (nullptr);
    // TODO: warning message for empty meshes

    // Check that the count of vertices and triangles is OK
    CS_ASSERT (mode == MESH_DUPLICATION_NONE
	       || (mesh->GetVertexCount () % 2 == 0 || mesh->GetTriangleCount () % 2 == 0));

    // Create the array of vertices
    int vertexCount = mode == MESH_DUPLICATION_NONE ?
      mesh->GetVertexCount () : mesh->GetVertexCount () / 2;
    btScalar* vertices = new btScalar[vertexCount * 3];
    csVector3* originalVertices = mesh->GetVertices ();
    int j = 0;
    for (int i = 0; i < vertexCount; i++)
    {
      // TODO: use bodyTransform
      csVector3 vertex = originalVertices[j]/* * bodyTransform.GetInverse ()*/ * system->GetInternalScale ();
      vertices[i * 3] = vertex[0];
      vertices[i * 3 + 1] = vertex[1];
      vertices[i * 3 + 2] = vertex[2];

      if (mode == MESH_DUPLICATION_INTERLEAVED) j += 2;
      else j++;
    }

    // Create the array of triangles
    int triangleCount = mode == MESH_DUPLICATION_NONE ?
      mesh->GetTriangleCount () : mesh->GetTriangleCount () / 2;
    int* triangles = new int[triangleCount * 3];
    csTriangle* originalTriangles = mesh->GetTriangles ();
    j = 0;
    for (int i = 0; i < triangleCount; i++)
    {
      csTriangle& triangle = originalTriangles[j];

      if (mode == MESH_DUPLICATION_INTERLEAVED)
      {
	triangles[i * 3] = triangle.a / 2;
	triangles[i * 3 + 1] = triangle.b / 2;
	triangles[i * 3 + 2] = triangle.c / 2;

	j += 2;
      }
      else
      {
	triangles[i * 3] = triangle.a;
	triangles[i * 3 + 1] = triangle.b;
	triangles[i * 3 + 2] = triangle.c;

	j++;
      }
    }

    // Create the soft body
    btSoftBody* body = btSoftBodyHelpers::CreateFromTriMesh
      (*system->GetSoftBodyWorldInfo (), vertices, triangles, triangleCount, false);

    body->m_cfg.piterations = 10;
    body->m_cfg.collisions |= btSoftBody::fCollision::SDF_RS; //?
    body->m_cfg.collisions |= btSoftBody::fCollision::VF_SS;
    body->m_materials[0]->m_kLST = 1;

    csRef<csBulletSoftBody> csBody;
    csBody.AttachNew (new csBulletSoftBody (system, body));
    csBody->CreateSoftBodyObject (this);
    return csPtr<CS::Physics::iSoftBody> (csBody);
  }

}
CS_PLUGIN_NAMESPACE_END (Bullet2)
