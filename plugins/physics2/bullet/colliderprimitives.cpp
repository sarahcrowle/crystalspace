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

#include "csgeom/sphere.h"
#include "csgeom/tri.h"
#include "csutil/stringquote.h"
#include "igeom/trimesh.h"
#include "imesh/objmodel.h"

// Bullet includes
#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"
#include "BulletCollision/Gimpact/btGImpactShape.h"
#include "BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h"
#include "BulletCollision/CollisionShapes/btBoxShape.h"
#include "BulletCollision/CollisionShapes/btCapsuleShape.h"
#include "BulletCollision/CollisionShapes/btConeShape.h"
#include "BulletCollision/CollisionShapes/btConvexHullShape.h"
#include "BulletCollision/CollisionShapes/btCylinderShape.h"
#include "BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h"
#include "BulletCollision/CollisionShapes/btShapeHull.h"
#include "BulletCollision/CollisionShapes/btSphereShape.h"

#include "colliderprimitives.h"
#include "collisionobject2.h"
#include "bulletsystem.h"

//#include "csutil/custom_new_enable.h"

using namespace CS::Collisions;

CS_PLUGIN_NAMESPACE_BEGIN(Bullet2)
{

//#include "csutil/custom_new_disable.h"

// #######################################################################################################
// Box

csBulletColliderBox::csBulletColliderBox (const csVector3& boxSize, csBulletSystem* sys)
  : scfImplementationType (this), boxSize (boxSize)
{
  collSystem = sys;
  shape = new btBoxShape (CSToBullet (boxSize*0.5f, collSystem->GetInternalScale ()));
  margin = 0.04f * collSystem->GetInverseInternalScale ();
  GetOrCreateBulletShape();
}

csBulletColliderBox::~csBulletColliderBox ()
{
  delete shape;
}

float csBulletColliderBox::ComputeShapeVolume() const 
{
  return boxSize.x * boxSize.y * boxSize.z;
}

// #######################################################################################################
// Sphere

csBulletColliderSphere::csBulletColliderSphere (float radius, csBulletSystem* sys)
  : scfImplementationType (this), radius (radius)
{
  collSystem = sys;
  shape = new btSphereShape (radius * collSystem->GetInternalScale ());
  margin = 0.04f * collSystem->GetInverseInternalScale ();
  GetOrCreateBulletShape();
}

csBulletColliderSphere::~csBulletColliderSphere ()
{
  delete shape;
}

float csBulletColliderSphere::ComputeShapeVolume() const 
{
  return (4/3.f) * PI * radius * radius * radius;
}

void csBulletColliderSphere::SetMargin (float margin)
{
  if (margin > 0.0f)
  {
    this->margin = margin;
    shape->setMargin ((margin + radius)* collSystem->GetInternalScale ());
  }
}

// #######################################################################################################
// Cylinder

csBulletColliderCylinder::csBulletColliderCylinder (float length, float radius, csBulletSystem* sys)
  : scfImplementationType (this), length (length), radius (radius)
{
  collSystem = sys;
  
  shape = new btCylinderShape (btVector3 (radius * collSystem->GetInternalScale (),
    length * collSystem->GetInternalScale () * 0.5f,
    radius * collSystem->GetInternalScale ()));
  /*shape = new btCylinderShapeZ (btVector3 (radius * collSystem->GetInternalScale (),
    radius * collSystem->GetInternalScale (),
    length * collSystem->GetInternalScale () * 0.5f));*/
  margin = 0.04f * collSystem->GetInverseInternalScale ();
  GetOrCreateBulletShape();
}

csBulletColliderCylinder::~csBulletColliderCylinder ()
{
  delete shape;
}

float csBulletColliderCylinder::ComputeShapeVolume() const 
{
  return PI * radius * radius * length;
}

void csBulletColliderCylinder::GetCylinderGeometry (float& length, float& radius) const
{
  length = this->length;
  radius = this->radius;
}

// #######################################################################################################
// Capsule

csBulletColliderCapsule::csBulletColliderCapsule (float length, float radius, csBulletSystem* sys)
  : scfImplementationType (this), length (length), radius (radius)
{
  collSystem = sys;
  shape = new btCapsuleShapeZ (radius * collSystem->GetInternalScale (),
    length * collSystem->GetInternalScale ());
  margin = 0.04f * collSystem->GetInverseInternalScale ();
  GetOrCreateBulletShape();
}

csBulletColliderCapsule::~csBulletColliderCapsule ()
{
  delete shape;
}

float csBulletColliderCapsule::ComputeShapeVolume() const 
{
  return PI * radius * radius * length 
    + 1.333333f * PI * radius * radius * radius;
}

void csBulletColliderCapsule::GetCapsuleGeometry (float& length, float& radius) const
{
  length = this->length;
  radius = this->radius;
}

// #######################################################################################################
// Cone

csBulletColliderCone::csBulletColliderCone (float length, float radius, csBulletSystem* sys)
  : scfImplementationType (this), length (length), radius (radius)
{
  collSystem = sys;
  shape = new btConeShapeZ (radius * collSystem->GetInternalScale (),
    length * collSystem->GetInternalScale ());
  margin = 0.04f * collSystem->GetInverseInternalScale ();
  GetOrCreateBulletShape();
}

csBulletColliderCone::~csBulletColliderCone ()
{
  delete shape;
}

float csBulletColliderCone::ComputeShapeVolume() const 
{
  return 0.333333f * PI * radius * radius * length;
}

void csBulletColliderCone::GetConeGeometry (float& length, float& radius) const
{
  length = this->length;
  radius = this->radius;
}

// #######################################################################################################
// Plane

csBulletColliderPlane::csBulletColliderPlane (const csPlane3& plane, 
                                              csBulletSystem* sys)
  : scfImplementationType (this), plane (plane)
{
  collSystem = sys;
  csVector3 normal = plane.GetNormal ();
  shape = new btStaticPlaneShape (btVector3 (normal.x,normal.y,normal.z),
    plane.D () * collSystem->GetInternalScale ());
  margin = 0.04f * collSystem->GetInverseInternalScale ();
  GetOrCreateBulletShape();
}

csBulletColliderPlane::~csBulletColliderPlane ()
{
  delete shape;
}

float csBulletColliderPlane::ComputeShapeVolume() const 
{
  return 0.f;
}

// #######################################################################################################
// Convex Mesh

csBulletColliderConvexMesh::csBulletColliderConvexMesh
(iTriangleMesh* triMesh, csBulletSystem* sys)
  : scfImplementationType (this), mesh (triMesh)
{
  collSystem = sys;

  btConvexHullShape* convexShape = new btConvexHullShape ();
  
  csVector3 *c_vertex = triMesh->GetVertices ();

  // TODO: move that code in CollisionHelper?
  //if (simplify)
  if (false)
  {
    btTriangleMesh* btTriMesh; // = ...
    btConvexShape* tmpConvexShape = new btConvexTriangleMeshShape (btTriMesh);
    btShapeHull* hull = new btShapeHull (tmpConvexShape);
    //btScalar marg = tmpConvexShape->getMargin ();
    //hull->buildHull (marg);
    hull->buildHull (10.f);
    tmpConvexShape->setUserPointer (hull);

    for (int i=0 ; i < hull->numVertices ();i++)
    {
      convexShape->addPoint (hull->getVertexPointer ()[i]);	
    }

    delete hull;
    delete tmpConvexShape;
    delete btTriMesh;
  }
  else
  {
    size_t vertexCount = triMesh->GetVertexCount ();
    for (size_t i = 0; i < vertexCount; i++)
    {
      convexShape->addPoint (CSToBullet (c_vertex[i], collSystem->GetInternalScale ()));
    }
  }

  shape = convexShape;
  margin = 0.04f * collSystem->GetInverseInternalScale ();

  GetOrCreateBulletShape();
}

csBulletColliderConvexMesh::~csBulletColliderConvexMesh ()
{
  delete shape;
}

float csBulletColliderConvexMesh::ComputeShapeVolume () const 
{
  if (!mesh->GetTriangleCount ())
  {
    // TODO: this is a really rough estimation
    btVector3 center;
    btScalar radius;
    shape->getBoundingSphere (center, radius);
    return 1.333333f * PI * radius * radius * radius;
  }

  csTriangle* c_triangle = mesh->GetTriangles ();
  csVector3* c_vertex = mesh->GetVertices ();
  csVector3 origin = c_vertex[c_triangle[0].a];

  float volume = 0.0f;
  size_t triangleCount = mesh->GetTriangleCount ();
  for (size_t i = 1; i < triangleCount; i++)
  {
    csVector3 a = c_vertex[c_triangle[i].a] - origin;
    csVector3 b = c_vertex[c_triangle[i].b] - origin;
    csVector3 c = c_vertex[c_triangle[i].c] - origin;
    csVector3 d;
    // TODO: is internal scale used?
    d.Cross (b, c);
    volume += btFabs (a * d);
  }

  volume /= 6.0f;
  return volume;
}

// #######################################################################################################
// Concave Mesh

csBulletColliderConcaveMesh::csBulletColliderConcaveMesh
(iTriangleMesh* mesh, btTriangleMesh* triMesh, csBulletSystem* sys, bool dynamicEnabled)
  : scfImplementationType (this), mesh (mesh), triMesh (triMesh)
{
  collSystem = sys;

  // Create either a dynamic or a static collider
  if (dynamicEnabled && collSystem->GetDynamicConcaveEnabled ())
  {
    btGImpactMeshShape* concaveShape = new btGImpactMeshShape (triMesh);
    concaveShape->updateBound ();
    shape = concaveShape;
    dynamic = true;
  }

  else
  {
    btBvhTriangleMeshShape* treeShape = new btBvhTriangleMeshShape (triMesh, true);
    treeShape->recalcLocalAabb();
    shape = treeShape;
    dynamic = false;
  }

  margin = 0.04f * collSystem->GetInverseInternalScale ();
  GetOrCreateBulletShape();
}

csBulletColliderConcaveMesh::~csBulletColliderConcaveMesh ()
{
  delete shape;
  delete triMesh;
}

float csBulletColliderConcaveMesh::ComputeShapeVolume () const 
{
  return 1.0f;

  //if (!mesh->GetTriangleCount ())
  {
    // TODO: this is a really rough estimation
    btVector3 center;
    btScalar radius;
    shape->getBoundingSphere (center, radius);
    return 1.333333f * PI * radius * radius * radius;
  }

  // TODO: using the indices of the btTriangleMesh would be both more efficient and accurate
  csTriangle* c_triangle = mesh->GetTriangles ();
  csVector3* c_vertex = mesh->GetVertices ();
  csVector3 origin = c_vertex[c_triangle[0].a];

  float volume = 0.0f;
  size_t triangleCount = mesh->GetTriangleCount ();
  for (size_t i = 1; i < triangleCount; i++)
  {
    csVector3 a = c_vertex[c_triangle[i].a] - origin;
    csVector3 b = c_vertex[c_triangle[i].b] - origin;
    csVector3 c = c_vertex[c_triangle[i].c] - origin;
    csVector3 d;
    // TODO: is internal scale used?
    d.Cross (b, c);
    volume += btFabs (a * d);
  }

  volume /= 6.0f;
  return volume;
}

// #######################################################################################################
// Concave Mesh scaled

csBulletColliderConcaveMeshScaled::csBulletColliderConcaveMeshScaled (CS::Collisions::iColliderConcaveMesh* collider,
                                                                      const csVector3& scale, csBulletSystem* sys)
  : scfImplementationType (this), originalShape (nullptr)
{
  collSystem = sys;
  this->scale = scale;
  originalCollider = dynamic_cast<csBulletColliderConcaveMesh*> (collider);

  btBvhTriangleMeshShape* meshShape;
  if (originalCollider->shape->getShapeType () == TRIANGLE_MESH_SHAPE_PROXYTYPE)
    meshShape = dynamic_cast<btBvhTriangleMeshShape*> (originalCollider->shape);

  else
  {
    meshShape = new btBvhTriangleMeshShape (originalCollider->triMesh, true);
    meshShape->recalcLocalAabb ();
    originalShape = meshShape;
  }

  shape = new btScaledBvhTriangleMeshShape (meshShape, btVector3 (scale[0], scale[1] , scale[2]));
  margin = 0.04f * collSystem->GetInverseInternalScale ();
}

csBulletColliderConcaveMeshScaled::~csBulletColliderConcaveMeshScaled ()
{
  delete shape;
  delete originalShape;
}

float csBulletColliderConcaveMeshScaled::ComputeShapeVolume() const 
{
  return originalCollider->GetVolume () * scale.x * scale.y * scale.z;
}

}
CS_PLUGIN_NAMESPACE_END (Bullet2)
