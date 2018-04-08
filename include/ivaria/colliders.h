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

#ifndef __CS_IVARIA_COLLIDERS_H__
#define __CS_IVARIA_COLLIDERS_H__

/**\file
 * Collision primitives interfaces
 */

#include "csutil/scf.h"
#include "csutil/csstring.h"
#include "csgeom/vector2.h"
#include "csgeom/vector3.h"
#include "csgeom/matrix3.h"
#include "csgeom/transfrm.h"
#include "csgeom/plane3.h"
#include "iutil/object.h"

struct iCamera;
struct iMeshWrapper;
struct iMovable;
struct iPortal;
struct iSector;
struct iTerrainCell;
struct iTerrainSystem;
struct iTriangleMesh;
struct iView;

namespace CS {
namespace Physics {

struct iPhysicalBody;

}
}

namespace CS {
namespace Collisions {
  
/**
 * The type of a collider.
 */
enum ColliderType
{
  COLLIDER_INVALID = 0,            /*!< Undefined collider type. */
  COLLIDER_BOX,                    /*!< The collider is a box. */
  COLLIDER_SPHERE,                 /*!< The collider is a sphere. */
  COLLIDER_CYLINDER,               /*!< The collider is a cylinder. */
  COLLIDER_CAPSULE,                /*!< The collider is a capsule. */
  COLLIDER_CONE,                   /*!< The collider is a cone. */
  COLLIDER_PLANE,                  /*!< The collider is a plane. */
  COLLIDER_CONVEX_MESH,            /*!< The collider is a convex mesh. */
  COLLIDER_CONCAVE_MESH,           /*!< The collider is a concave mesh. */
  COLLIDER_CONCAVE_MESH_SCALED,    /*!< The collider is a scaled concave mesh. */
  COLLIDER_TERRAIN_CELL,           /*!< The collider is a terrain cell. */
  COLLIDER_COMPOUND                /*!< The collider is an empty (hence supposedly compound) collider. */
};

/**
 * A base interface for colliders. 
 * Other colliders will be derived from this one.
 */
struct iCollider : public virtual iBase
{
  SCF_INTERFACE (CS::Collisions::iCollider, 1, 0, 0);

  /// Get the type of this collider. 
  virtual ColliderType GetColliderType () const = 0;

  /// Set the scale of the collider shape on X/Y/Z axis.
  virtual void SetLocalScale (const csVector3& scale) = 0;

  /// Get the scale along the X/Y/Z axes.
  virtual csVector3 GetLocalScale () const = 0;
  
  /// Get the volume of this collider
  virtual float GetVolume () const = 0;

  /**
   * Whether this collider (and all its children) can potentially be animated dynamically
   * by the physical simulation. All colliders are potentially dynamic, excepted the
   * concave and scaled concave meshes, the planes, and the terrains.
   */
  virtual bool IsDynamic () const = 0;

  /// Add a child collider, with a fixed relative transform regarding this collider.
  virtual void AddChild (iCollider* collider,
			 const csOrthoTransform& transform = csOrthoTransform ()) = 0;

  /// Remove the given child collider from this collider.
  virtual void RemoveChild (iCollider* collider) = 0;

  /// Remove the child collider with the given index from this collider.
  virtual void RemoveChild (size_t index) = 0;

  /// Get the count of child colliders in this collider
  virtual size_t GetChildrenCount () const = 0;

  /// Get the collider with the given index.
  virtual iCollider* GetChild (size_t index) = 0; 

  /// Set the relative transform of the collider with the given index.
  virtual void SetChildTransform (size_t index, const csOrthoTransform& transform) = 0;

  /// Get the relative transform of the collider with the given index.
  virtual const csOrthoTransform& GetChildTransform (size_t index) const = 0;
};

/**
 * A box collider.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateColliderBox()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionObject::GetCollider()
 * 
 * Main users of this interface:
 * - iCollisionObject
 */
struct iColliderBox : public virtual iCollider
{
  SCF_INTERFACE (CS::Collisions::iColliderBox, 1, 0, 0);

  /// Get the box geometry of this collider.
  virtual const csVector3& GetBoxGeometry () const = 0;
};

/**
 * A sphere collider.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateColliderSphere()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionObject::GetCollider()
 * 
 * Main users of this interface:
 * - iCollisionObject
 */
struct iColliderSphere : public virtual iCollider
{
  SCF_INTERFACE (CS::Collisions::iColliderSphere, 1, 0, 0);

  /// Get the sphere geometry of this collider.
  virtual float GetSphereGeometry () const = 0;
};

/**
 * A cylinder collider.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateColliderCylinder()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionObject::GetCollider()
 * 
 * Main users of this interface:
 * - iCollisionObject
 */
struct iColliderCylinder : public virtual iCollider
{
  SCF_INTERFACE (CS::Collisions::iColliderCylinder, 1, 0, 0);

  /// Get the cylinder geometry of this collider.
  virtual void GetCylinderGeometry (float& length, float& radius) const = 0;
};

/**
 * A capsule collider.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateColliderCapsule()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionObject::GetCollider()
 * 
 * Main users of this interface:
 * - iCollisionObject
 */
struct iColliderCapsule : public virtual iCollider
{
  SCF_INTERFACE (CS::Collisions::iColliderCapsule, 1, 0, 0);

  /// Get the capsule geometry of this collider.
  virtual void GetCapsuleGeometry (float& length, float& radius) const = 0;
};

/**
 * A cone collider.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateColliderCone()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionObject::GetCollider()
 * 
 * Main users of this interface:
 * - iCollisionObject
 */
struct iColliderCone : public virtual iCollider
{
  SCF_INTERFACE (CS::Collisions::iColliderCone, 1, 0, 0);

  /// Get the cone geometry of this collider.
  virtual void GetConeGeometry (float& length, float& radius) const = 0;
};

/**
 * A static plane collider.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateColliderPlane()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionObject::GetCollider()
 * 
 * Main users of this interface:
 * - iCollisionObject
 */
struct iColliderPlane : public virtual iCollider
{
  SCF_INTERFACE (CS::Collisions::iColliderPlane, 1, 0, 0);

  /// Get the plane geometry of this collider.
  virtual const csPlane3& GetPlaneGeometry () const = 0;
};

/**
 * A convex mesh collider.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateColliderConvexMesh()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionObject::GetCollider()
 * 
 * Main users of this interface:
 * - iCollisionObject
 */
struct iColliderConvexMesh : public virtual iCollider
{
  SCF_INTERFACE (CS::Collisions::iColliderConvexMesh, 1, 0, 0);

  /// Get the triangle mesh of this collider.
  virtual iTriangleMesh* GetMesh () const = 0;
};

/**
 * A static concave mesh collider.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateColliderConcaveMesh()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionObject::GetCollider()
 * 
 * Main users of this interface:
 * - iCollisionObject
 */
struct iColliderConcaveMesh : public virtual iCollider
{
  SCF_INTERFACE (CS::Collisions::iColliderConcaveMesh, 1, 0, 0);

  /// Get the mesh factory of this collider.
  virtual iTriangleMesh* GetMesh () const = 0;
};

/**
 * A scaled static concave mesh collider.
 *
 * Main creators of instances implementing this interface:
 * - iCollisionSystem::CreateColliderConcaveMeshScaled()
 * 
 * Main ways to get pointers to this interface:
 * - iCollisionObject::GetCollider()
 * 
 * Main users of this interface:
 * - iCollisionObject
 */
struct iColliderConcaveMeshScaled : public virtual iCollider
{
  SCF_INTERFACE (CS::Collisions::iColliderConcaveMeshScaled, 1, 0, 0);

  /// Get the concave collider scaled by this collider.
  virtual iColliderConcaveMesh* GetCollider () const = 0;

  /// Get the scale of this collider.
  virtual const csVector3& GetScale () const = 0;
};

/**
 * A terrain cell collider, part of a iCollisionTerrain.
 *
 * Terrain cells cannot be created manually, they are created instead
 * automatically by the associated iCollisionTerrain.
 */
struct iColliderTerrainCell : public virtual iCollider
{
  SCF_INTERFACE (CS::Collisions::iColliderTerrainCell, 1, 0, 0);

  /// Get the terrain cell associated with this collider
  virtual iTerrainCell* GetCell () const = 0;
};

}
}

#endif
