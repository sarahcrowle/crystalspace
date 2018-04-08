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

#ifndef __CS_BULLET_PORTAL_H__
#define __CS_BULLET_PORTAL_H__

#include "bulletsector.h"
#include "bulletsystem.h"
#include "collisionobject2.h"
#include "ivaria/collisions.h"
#include "ivaria/physics.h"

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

class CollisionPortal;
class PortalTraversalData;

// TODO: what's this?
struct PhysObjSynchronizer
{  
};

class RigidBodySynchronizer : public PhysObjSynchronizer
{
  //csOrthoTransform oldTransform;

public:
  RigidBodySynchronizer () {}

  void Synchronize (csBulletRigidBody* obj, csBulletRigidBody* cloneObj);
};

class SoftBodySynchronizer : public PhysObjSynchronizer
{
  csArray<btVector3> oldCloneVels;

public:
  SoftBodySynchronizer () {}
  virtual ~SoftBodySynchronizer () { }

  void SetupSynchronizer (csBulletSoftBody* cloneObj);

  void Synchronize (csBulletSoftBody* obj, csBulletSoftBody* cloneObj);
};

/**
 * Represents all the data required by portals to manage traversal of physical objects
 */
class PortalTraversalData
{
public:
  /// Whether this belongs to the original object (or the cloned object)
  bool IsOriginal;

  /// The portal being traversed (same for original and copy)
  CollisionPortal* Portal;

  /// The traversing object
  csWeakRef<csBulletCollisionObject> Object;
  
  /// The cloned object
  csWeakRef<csBulletCollisionObject> OtherObject;
  
  /// Helper that aids the traversal of the object
  PhysObjSynchronizer* Synchronizer;

public:
  PortalTraversalData (bool isOriginal, CollisionPortal* portal, csBulletCollisionObject* obj) : 
      IsOriginal (isOriginal), Portal (portal), Object (obj), Synchronizer (nullptr)
  {
  }

  virtual ~PortalTraversalData ()
  {
    if (Synchronizer)
      delete Synchronizer;
  }
};

/**
 * A collision portal guides the traversal of physical objects through a portal, and ensures that impulses on either side of the portal
 * effect the other side. The portal achieves that by creating a copy of the object and placing it on the other side and syncing force,
 * velocity and position between the two.
 */
class CollisionPortal
{
//private:
public:
  /**
   * The portal as described by the scene
   */
  iPortal* portal;

  /**
   * The transform that converts coordinates of this portal to coordinates of the other side
   */
  csOrthoTransform warpTrans;

  csBulletSector* sourceSector;
  csBulletSector* targetSector;

  /**
   * Represents the portal in the bullet world
   */ 
  btGhostObject* ghostPortal;

  /**
   * All objects that are currently touching this portal
   */
  csRefArray<csBulletCollisionObject> objects;

  /**
   * All transforms of all objects that are currently touching this portal
   */
  csArray<csOrthoTransform> transforms;

public:
  CollisionPortal (iPortal* portal, const csOrthoTransform& meshTrans, csBulletSector* sourceSector);
  ~CollisionPortal ();

  bool CanTraverse (csBulletCollisionObject* obj);

  inline bool IsTraversingThisPortal (csBulletCollisionObject* obj)
  { return obj->portalData && obj->portalData->Portal == this; }

  void UpdateCollisionsPreStep (csBulletSector* sector);
  void UpdateCollisionsPostStep (csBulletSector* sector) {}

  bool IsOnOtherSide (csBulletCollisionObject* obj);
  void Traverse (csBulletCollisionObject* obj);

  /// Synchronize the two symbiotic objects and correct position, velocity and force
  void Synchronize (csBulletCollisionObject* obj, csBulletCollisionObject* cloneObj);

  /// Removes this portal from its sector
  void RemoveFromSector ();

  /// Removes the given object from the context of this portal
  void RemoveTraversingObject (csBulletCollisionObject* obj);
};

}
CS_PLUGIN_NAMESPACE_END (Bullet2)

#endif
