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
#include "csutil/stringquote.h"
#include "imesh/objmodel.h"

// Bullet includes

#include "csutil/custom_new_disable.h"

#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"

#include "csutil/custom_new_enable.h"

#include "colliderprimitives.h"
#include "collisionobject2.h"
#include "bulletsystem.h"

using namespace CS::Collisions;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

  csBulletCollider::csBulletCollider ()
    : scfImplementationType (this), shape (nullptr), usedShape (nullptr), margin (0.f),
    collSystem (nullptr), dirty (true), children (nullptr), volume (0.f),
    localInertia (0.f, 0.f, 0.f)
  {
    principalAxisTransform.setIdentity ();
  }

  csBulletCollider::csBulletCollider (csBulletSystem* system)
    : scfImplementationType (this), shape (nullptr), usedShape (nullptr), margin (0.f),
    collSystem (system), dirty (true), children (nullptr), volume (0.f),
    localInertia (0.f, 0.f, 0.f)
  {
    principalAxisTransform.setIdentity ();
  }

  csBulletCollider::~csBulletCollider ()
  {
    if (children)
    {
      delete children;
    }
  }

  bool csBulletCollider::IsDirty () const
  {
    if (dirty) return true;

    // Recursively check if the collider tree has been changed
    if (children)
       for (size_t i = 0; i < children->colliders.GetSize (); i++)
       {
        csBulletCollider* collider =
	  dynamic_cast<csBulletCollider*> (children->colliders[i]);
	if (collider->IsDirty ()) return true;
       }

    return false;
  }

  // TODO: is it really working?
  csVector3 csBulletCollider::GetLocalScale () const
  {
    if (shape)
    {
      return BulletToCS (shape->getLocalScaling (), 1);
    }
    
    // TODO: Consider storing scale in a separate variable for compound shapes
    if (children && children->colliders.GetSize ())
    {
      return children->colliders[0]->GetLocalScale ();
    }
    return csVector3 (1);
  }

  void csBulletCollider::SetLocalScale (const csVector3& scale)
  {
    if (shape)
    {
      btVector3 btScale = CSToBullet (scale, 1);
      shape->setLocalScaling (btScale);
    }

    if (children)
    {
      for (size_t i = 0; i < children->colliders.GetSize (); i++)
      {
        iCollider* icoll = children->colliders[i];
        icoll->SetLocalScale (scale);
      }
    }
    
    // TODO: Re-compute volume after setting scale
  }

  void csBulletCollider::SetMargin (float margin)
  {
    if (margin > 0.0f)
    {
      // TODO: Fix this
      this->margin = margin;
      shape->setMargin (margin * collSystem->GetInternalScale ());
    }
  }

  float csBulletCollider::GetMargin () const
  {
    return margin;
  }

  float csBulletCollider::GetVolume () const
  {
    float volume = ComputeShapeVolume ();
    if (children)
      for (size_t i = 0; i < children->colliders.GetSize (); i++)
	volume += children->colliders[i]->GetVolume ();

    return volume;
  }

  btCollisionShape* csBulletCollider::GetOrCreateBulletShape ()
  {
    // TODO: Add methods iCollider::SetMass/SetTotalMass/SetDensity/SetTotalDensity
    // and revamp this code

    if (!IsDirty ()) return usedShape;

    dirty = false;

    if (children)
    {
      children->staticColliderCount = 0;
    }

    bool isDynamic = IsDynamic ();
    if (children && children->colliders.GetSize () > 0)
    {
      // create a new shape
      btCompoundShape& compound = children->compoundShape;

      // remove all it's children
      for (int i = compound.getNumChildShapes () - 1; i >= 0; --i)
      {
        compound.removeChildShapeByIndex (i);
      }

      volume = 0.f;
      int start = shape ? 1 : 0;
      int totalShapeCount = int (children->colliders.GetSize ()) + start;

      CS_ALLOC_STACK_ARRAY (float, masses, totalShapeCount);

      // add this collider's own shape
      btTransform identity;
      identity.setIdentity ();
      if (shape)
      {
        compound.addChildShape (identity, shape);
        volume = ComputeShapeVolume ();
        ++totalShapeCount;
        masses[0] = volume;
      }
      
      // add all children
      for (size_t i = 0; i < children->colliders.GetSize (); i++)
      {
        iCollider* icoll = children->colliders[i];
        csBulletCollider* coll = dynamic_cast<csBulletCollider*> (icoll);
        btTransform relaTrans = CSToBullet (children->transforms[i], collSystem->GetInternalScale ());
        btCollisionShape* childShape = coll->GetOrCreateBulletShape ();
        compound.addChildShape (relaTrans, childShape);

        if (!coll->IsDynamic ())
        {
          ++children->staticColliderCount;
        }

        btScalar childVolume = coll->GetVolume ();
        if (childVolume < EPSILON)
        {
          // shape is probably float -> Assign default volume of 1 for now
          childVolume = 1;
        }
        masses[start + i] = childVolume;
        volume += childVolume;
      }

      // compute principal axis
      if (isDynamic)
      {
        btVector3 principalInertia;
        children->compoundShape.calculatePrincipalAxisTransform
	  (masses, principalAxisTransform, principalInertia);
      }

      // Translate & rotate children relative to principal axis
      btTransform principalAxisTransformInverse = principalAxisTransform.inverse ();
      for (size_t i = 0; i < children->colliders.GetSize (); i++)
      {
        //iCollider* icoll = children->colliders[i];
        compound.updateChildTransform (i, principalAxisTransformInverse * compound.getChildTransform (i));
      }

      usedShape = &children->compoundShape;
    }

    else
    {
      principalAxisTransform.setIdentity ();
      usedShape = shape;
      volume = ComputeShapeVolume ();
    }

    if (usedShape && volume > SMALL_EPSILON && isDynamic)
    {
      usedShape->calculateLocalInertia (1, localInertia);      // inertia is proportional to mass
    }

    return usedShape;
  }

  bool csBulletCollider::IsDynamic () const
  {
    if (!IsBaseDynamic ()) return false;
    return !children || children->staticColliderCount == 0;
  }

  void csBulletCollider::AddChild (CS::Collisions::iCollider* iColl, const csOrthoTransform& relaTrans)
  {
    csRef<csBulletCollider> coll (dynamic_cast<csBulletCollider*> (iColl));

    csColliderCollection* children = GetOrCreateChildren ();

    dirty = true;
    children->colliders.Push (coll);
    children->transforms.Push (relaTrans);
  }

  void csBulletCollider::RemoveChild (CS::Collisions::iCollider* collider)
  {
    if (!children) return;

    for (size_t i =0; i < children->colliders.GetSize (); i++)
    {
      if (children->colliders[i] == collider)
      {
	children->colliders.DeleteIndex (i);
	children->transforms.DeleteIndex (i);
	dirty = true;
        return;
      }
    }
  }

  void csBulletCollider::RemoveChild (size_t index)
  {
    if (!children) return;

    children->colliders.DeleteIndex (index);
    children->transforms.DeleteIndex (index);
    dirty = true;
  }

  CS::Collisions::iCollider* csBulletCollider::GetChild (size_t index)
  {
    if (shape)
    {
      if (index == 0) return this;

      if (!children || index > children->colliders.GetSize ()) return nullptr;

      return children->colliders[index-1];
    }
    else
    {
      if (!children || index >= children->colliders.GetSize ()) return nullptr;

      return children->colliders[index];
    }
  }

  void csBulletCollider::SetChildTransform (size_t index, const csOrthoTransform& transform)
  {
    if (shape) children->transforms[index - 1] = transform;
    else children->transforms[index] = transform;
  }

  const csOrthoTransform& csBulletCollider::GetChildTransform (size_t index) const
  {
    if (shape) return children->transforms[index - 1];
    else return children->transforms[index];
  }

}
CS_PLUGIN_NAMESPACE_END (Bullet2)
