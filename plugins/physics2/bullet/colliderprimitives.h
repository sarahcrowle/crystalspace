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

/**
 * Collection of primitive colliders
 */

#ifndef __CS_BULLET_COLLIDERPRIMITIVES_H__
#define __CS_BULLET_COLLIDERPRIMITIVES_H__

#include "csgeom/plane3.h"
#include "ivaria/collisions.h"
#include "common2.h"
#include "collider.h"

class btBoxShape;
class btSphereShape;
class btCylinderShapeZ;
class btCapsuleShapeZ;
class btConeShapeZ;
class btStaticPlaneShape;
class btConvexHullShape;
class btCollisionShape;
class btScaledBvhTriangleMeshShape;
class btBvhTriangleMeshShape;
class btGImpactMeshShape;
class btTriangleMesh;
struct csLockedHeightData;
struct iTerrainSystem;
struct iTriangleMesh;


CS_PLUGIN_NAMESPACE_BEGIN(Bullet2)
{

class csBulletSector;
class csBulletSystem;
class csBulletCollider;

class csBulletColliderBox: 
  public scfVirtImplementationExt1<csBulletColliderBox,
  csBulletCollider, CS::Collisions::iColliderBox>
{
  csVector3 boxSize;

protected:
  virtual float ComputeShapeVolume() const;

public:
  csBulletColliderBox (const csVector3& boxSize, csBulletSystem* sys);
  virtual ~csBulletColliderBox ();

  virtual CS::Collisions::ColliderType GetColliderType () const
  { return CS::Collisions::COLLIDER_BOX; }

  virtual const csVector3& GetBoxGeometry () const { return boxSize; }
};

class csBulletColliderSphere :
  public scfVirtImplementationExt1<csBulletColliderSphere,
    csBulletCollider, CS::Collisions::iColliderSphere>
{
  float radius;
  
protected:
  virtual float ComputeShapeVolume() const;

public:
  csBulletColliderSphere (float radius, csBulletSystem* sys);
  virtual ~csBulletColliderSphere ();

  virtual CS::Collisions::ColliderType GetColliderType () const
  { return CS::Collisions::COLLIDER_SPHERE; }
  virtual void SetMargin (float margin);

  virtual float GetSphereGeometry () const { return radius; }
};

class csBulletColliderCylinder:
  public scfVirtImplementationExt1<csBulletColliderCylinder,
  csBulletCollider, CS::Collisions::iColliderCylinder>
{
  float length;
  float radius;
  
protected:
  virtual float ComputeShapeVolume() const;

public:
  csBulletColliderCylinder (float length, float radius, csBulletSystem* sys);
  virtual ~csBulletColliderCylinder ();

  virtual CS::Collisions::ColliderType GetColliderType () const
  { return CS::Collisions::COLLIDER_CYLINDER; }

  virtual void GetCylinderGeometry (float& length, float& radius) const;
};

class csBulletColliderCapsule: 
  public scfVirtImplementationExt1<csBulletColliderCapsule,
  csBulletCollider, CS::Collisions::iColliderCapsule>
{
  float length;
  float radius;
  
protected:
  virtual float ComputeShapeVolume() const;

public:
  csBulletColliderCapsule (float length, float radius, csBulletSystem* sys);
  virtual ~csBulletColliderCapsule ();

  virtual CS::Collisions::ColliderType GetColliderType () const
  { return CS::Collisions::COLLIDER_CAPSULE; }

  virtual void GetCapsuleGeometry (float& length, float& radius) const;
};

class csBulletColliderCone:
  public scfVirtImplementationExt1<csBulletColliderCone,
  csBulletCollider, CS::Collisions::iColliderCone>
{
  float length;
  float radius;
  
protected:
  virtual float ComputeShapeVolume() const;

public:
  csBulletColliderCone (float length, float radius, csBulletSystem* sys);
  virtual ~csBulletColliderCone ();

  virtual CS::Collisions::ColliderType GetColliderType () const
  { return CS::Collisions::COLLIDER_CONE; }

  virtual void GetConeGeometry (float& length, float& radius) const;
};

class csBulletColliderPlane:
  public scfVirtImplementationExt1<csBulletColliderPlane,
  csBulletCollider, CS::Collisions::iColliderPlane>
{
  csPlane3 plane;

protected:
  virtual float ComputeShapeVolume() const;

public:
  csBulletColliderPlane (const csPlane3& plane, csBulletSystem* sys);
  virtual ~csBulletColliderPlane ();

  virtual CS::Collisions::ColliderType GetColliderType () const
  { return CS::Collisions::COLLIDER_PLANE; }

  virtual inline bool IsBaseDynamic () const
  { return false; }

  virtual void SetLocalScale (const csVector3& scale) {}

  virtual const csPlane3& GetPlaneGeometry () const { return plane; }
};

class csBulletColliderConvexMesh:
  public scfVirtImplementationExt1<csBulletColliderConvexMesh,
  csBulletCollider, CS::Collisions::iColliderConvexMesh>
{
  csRef<iTriangleMesh> mesh;

protected:
  virtual float ComputeShapeVolume() const;
  
public:
  csBulletColliderConvexMesh (iTriangleMesh* triMesh, csBulletSystem* sys);
  virtual ~csBulletColliderConvexMesh ();

  virtual CS::Collisions::ColliderType GetColliderType () const
  { return CS::Collisions::COLLIDER_CONVEX_MESH; }

  virtual iTriangleMesh* GetMesh () const { return mesh; }
};

class csBulletColliderConcaveMesh:
  public scfVirtImplementationExt1<csBulletColliderConcaveMesh, 
  csBulletCollider, CS::Collisions::iColliderConcaveMesh>
{
  friend class csBulletColliderConcaveMeshScaled;
  iTriangleMesh* mesh;
  btTriangleMesh* triMesh;
  bool dynamic;

protected:
  virtual float ComputeShapeVolume() const;

public:
  csBulletColliderConcaveMesh (iTriangleMesh* mesh, btTriangleMesh* triMesh,
			       csBulletSystem* sys, bool dynamicEnabled);
  virtual ~csBulletColliderConcaveMesh ();

  virtual CS::Collisions::ColliderType GetColliderType () const
  { return CS::Collisions::COLLIDER_CONCAVE_MESH; }

  virtual bool IsBaseDynamic () const
  { return dynamic;  }

  virtual iTriangleMesh* GetMesh () const { return mesh; }
};

class csBulletColliderConcaveMeshScaled:
  public scfVirtImplementationExt1<csBulletColliderConcaveMeshScaled,
  csBulletCollider, CS::Collisions::iColliderConcaveMeshScaled>
{
  csBulletColliderConcaveMesh* originalCollider;
  btBvhTriangleMeshShape* originalShape;
  csVector3 scale;

protected:
  virtual float ComputeShapeVolume() const;

public:
  csBulletColliderConcaveMeshScaled (CS::Collisions::iColliderConcaveMesh* collider,
				     const csVector3& scale, csBulletSystem* sys);
  virtual ~csBulletColliderConcaveMeshScaled();

  virtual CS::Collisions::ColliderType GetColliderType () const
  { return CS::Collisions::COLLIDER_CONCAVE_MESH_SCALED; }

  virtual inline bool IsBaseDynamic () const
  { return false; }

  virtual CS::Collisions::iColliderConcaveMesh* GetCollider () const
  { return dynamic_cast<CS::Collisions::iColliderConcaveMesh*> (originalCollider); }
  virtual const csVector3& GetScale () const { return scale; }
};
}
CS_PLUGIN_NAMESPACE_END(Bullet2)
#endif
