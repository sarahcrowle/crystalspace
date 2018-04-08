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
 * iCollider implementation base class
 */

#ifndef __CS_BULLET_COLLIDER_H__
#define __CS_BULLET_COLLIDER_H__

#include "csutil/weakref.h"
#include "csgeom/plane3.h"
#include "ivaria/collisions.h"
#include "common2.h"

#include "csutil/custom_new_disable.h"

#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"

#include "csutil/custom_new_enable.h"

struct csLockedHeightData;
struct iTerrainSystem;
struct iTriangleMesh;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

class csBulletSector;
class csBulletSystem;
class csBulletCollider;

/**
 * A collection of colliders that belong to another collider
 */
struct csColliderCollection
{
  btCompoundShape compoundShape;

  short staticColliderCount;
  csRefArray<csBulletCollider> colliders;
  csArray<csOrthoTransform> transforms;

  csColliderCollection ()
  : staticColliderCount (0)
  {}
};

/**
 * Implementation of iCollider. Supports hierarchy of colliders.
 */
class csBulletCollider
  : public scfVirtImplementation1<csBulletCollider, CS::Collisions::iCollider>
{
  friend class csBulletCollisionObject;
  friend class BulletCollisionActor;
  friend class csBulletGhostCollisionObject;
  friend class csBulletRigidBody;
  friend class csBulletSystem;

protected:
  btCollisionShape* shape, *usedShape;
  float margin;
  csWeakRef<csBulletSystem> collSystem;

  bool dirty;
  csColliderCollection* children;
  
  float volume;
  btVector3 localInertia;
  btTransform principalAxisTransform;

  virtual float ComputeShapeVolume () const
  { return 0.f; }

  bool IsDirty () const;

  inline csColliderCollection* GetOrCreateChildren () 
  {
    if (!children)
    {
      children = new csColliderCollection;
    }
    return children;
  }

public:
  csBulletCollider ();
  csBulletCollider (csBulletSystem* system);
  virtual ~csBulletCollider ();

  virtual CS::Collisions::ColliderType GetColliderType () const
  { return CS::Collisions::COLLIDER_COMPOUND; }

  virtual void SetLocalScale (const csVector3& scale);
  virtual csVector3 GetLocalScale () const;

  virtual void SetMargin (float margin);
  virtual float GetMargin () const;
  virtual float GetVolume () const;

  virtual bool IsDynamic () const;
  virtual inline bool IsBaseDynamic () const
  { return true; }

  virtual void AddChild (CS::Collisions::iCollider* collider,
			    const csOrthoTransform& relaTrans = csOrthoTransform ());
  virtual void RemoveChild (CS::Collisions::iCollider* collider);
  virtual void RemoveChild (size_t index);

  virtual CS::Collisions::iCollider* GetChild (size_t index) ;
  virtual size_t GetChildrenCount () const
  { return children->colliders.GetSize (); }
  
  virtual void SetChildTransform (size_t index, const csOrthoTransform& transform);
  virtual const csOrthoTransform& GetChildTransform (size_t index) const;

  btCollisionShape* GetOrCreateBulletShape ();

  // TODO: remove?
  inline const btVector3& GetLocalInertia () const 
  { 
    return localInertia; 
  }

  inline const btTransform& GetPrincipalAxisTransform () const 
  {
    return principalAxisTransform; 
  }
};

}
CS_PLUGIN_NAMESPACE_END (Bullet2)
#endif
