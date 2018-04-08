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

#ifndef __CS_BULLET_COLLISIONOBJECT_H__
#define __CS_BULLET_COLLISIONOBJECT_H__

#include "bulletsystem.h"
#include "common2.h"
#include "colliderprimitives.h"

struct iSceneNode;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

class CollisionGroup;
class PortalTraversalData;

class csBulletCollisionObject : public scfVirtImplementationExt1<
  csBulletCollisionObject, csObject,
  CS::Collisions::iCollisionObject>
{
  friend class csBulletSector;
  friend class csBulletSystem;
  friend class csBulletMotionState;
  friend class csBulletKinematicMotionState;
  friend class csBulletJoint;
  friend class csBulletCollisionTerrain;
  friend class csBulletGhostCollisionObject;
  friend class CollisionPortal;

protected:
  csRef<csBulletCollider> collider;
  csOrthoTransform colliderTransform;
  // TODO: kept only if there are some listeners? or not at all?
  csRefArray<csBulletCollisionObject> contactObjects;
  // TODO: notion of jointed islands instead?
  csArray<CS::Physics::iJoint*> joints;
  // TODO: this does not allow to switch an object from one system to another
  CollisionGroup* group;
  csRef<iSceneNode> sceneNode;
  csWeakRef<iCamera> camera;
  csRef<CS::Collisions::iCollisionCallback> collCb;

  csBulletSector* sector;
  // TODO: this is redundant with the one of the factory
  csBulletSystem* system;

  // TODO: move this data into the collision portal
  PortalTraversalData* portalData;
  
  btCollisionObject* btObject;

  // TODO: this is redundant with the sector
  bool insideWorld;

  void CreateCollisionObject (CS::Collisions::iCollisionObjectFactory* props);

public:
  csBulletCollisionObject (csBulletSystem* sys);

  virtual ~csBulletCollisionObject ();

  virtual iObject* QueryObject () { return (iObject*) this; }
  virtual CS::Physics::iPhysicalBody* QueryPhysicalBody () { return nullptr; }
  virtual CS::Collisions::iCollisionActor* QueryCollisionActor () { return nullptr; }

  virtual CS::Collisions::iCollisionSystem* GetSystem () const { return system; }
  virtual void SetSector (CS::Collisions::iCollisionSector* sector);
  virtual CS::Collisions::iCollisionSector* GetSector () const { return sector; }

  virtual CS::Collisions::CollisionObjectType GetObjectType () const = 0;

  virtual void SetAttachedSceneNode (iSceneNode* newSceneNode);
  virtual iSceneNode* GetAttachedSceneNode () const { return sceneNode; }

  virtual void SetAttachedCamera (iCamera* camera);
  virtual iCamera* GetAttachedCamera () const { return camera; }

  virtual CS::Collisions::iCollider* GetCollider () const { return collider; }
  virtual void SetCollider (CS::Collisions::iCollider* collider,
			    const csOrthoTransform& transform = csOrthoTransform ());

  virtual void SetColliderTransform (const csOrthoTransform& transform);
  virtual const csOrthoTransform& GetColliderTransform () const;

  virtual void SetTransform (const csOrthoTransform& trans);
  virtual csOrthoTransform GetTransform () const;

  virtual void RebuildObject () = 0;

  virtual void SetCollisionGroup (CS::Collisions::iCollisionGroup* group);
  virtual CS::Collisions::iCollisionGroup* GetCollisionGroup () const;

  virtual void SetCollisionCallback (CS::Collisions::iCollisionCallback* cb) {collCb = cb;}
  virtual CS::Collisions::iCollisionCallback* GetCollisionCallback () {return collCb;}

  virtual csPtr<CS::Collisions::iCollisionData> Collide
    (CS::Collisions::iCollisionObject* otherObject) const;

  virtual CS::Collisions::HitBeamResult HitBeam
    (const csVector3& start, const csVector3& end) const;

  virtual size_t GetContactObjectsCount ();
  virtual CS::Collisions::iCollisionObject* GetContactObject (size_t index);

  btCollisionObject* GetBulletCollisionPointer () { return btObject; }

  virtual bool AddBulletObject () = 0;
  virtual bool RemoveBulletObject () = 0;

  /// Whether this object may be excluded from deactivation.
  virtual bool GetDeactivable () const
  { return btObject->getActivationState () == DISABLE_DEACTIVATION; }
  /// Whether this object may be excluded from deactivation.
  virtual void SetDeactivable (bool d)
  { btObject->setActivationState (d ? 0 : DISABLE_DEACTIVATION); }

  /// Clone this object
  virtual btCollisionObject* CreateBulletObject ()
  {
    return nullptr;
  }
  
  /// Clone this object
  virtual csPtr<CS::Collisions::iCollisionObject> CloneObject () 
  { 
    return csPtr<CS::Collisions::iCollisionObject> (nullptr); 
  }
  
  /**
   * Clone this object for use with portals.
   * Might adjust some object properties necessary for an accurate portal simulation.
   */
  virtual csPtr<CS::Collisions::iCollisionObject> ClonePassivePortalObject () 
  { 
    return CloneObject ();
  }

  /**
   * Passive objects, such as portal clones, are not supposed to be tempered with
   */
  virtual bool IsPassive () const;

  inline PortalTraversalData* GetPortalData () const
  {
    return portalData;
  }
};
}
CS_PLUGIN_NAMESPACE_END (Bullet2)
#endif
