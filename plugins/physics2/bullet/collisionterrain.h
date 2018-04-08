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

#ifndef __CS_BULLET_COLLISIONTERRAIN_H__
#define __CS_BULLET_COLLISIONTERRAIN_H__

#include "BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"
#include "csutil/weakref.h"
#include "csgeom/plane3.h"
#include "iengine/scenenode.h"
#include "imesh/terrain2.h"
#include "ivaria/collisions.h"
#include "common2.h"
#include "collisionobject2.h"
#include "colliderprimitives.h"
#include "rigidbody2.h"

struct csLockedHeightData;
struct iTerrainSystem;
struct iTriangleMesh;

using namespace CS::Collisions;
using namespace CS::Physics;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

class csBulletCollisionTerrain;
class csBulletSector;
class csBulletSystem;

//--------------------------------- csBulletColliderTerrainCell ---------------------------------

class csBulletColliderTerrainCell : public scfVirtImplementationExt1<
  csBulletColliderTerrainCell, csBulletCollider, iColliderTerrainCell>
{
  friend class csBulletCollisionTerrain;
  
private:
  csBulletCollisionTerrain* terrain;
  csWeakRef<iTerrainCell> cell;
  float* heightData;
  btHeightfieldTerrainShape* shape;
  btRigidBody* body;
  btVector3 localScale;

  void UpdateHeight (const csRect& area);

protected:
  virtual float ComputeShapeVolume () const { return 0; }

public:
  csBulletColliderTerrainCell (csBulletCollisionTerrain* terrain, float* gridData, iTerrainCell* cell,
			       float minHeight, float maxHeight);
  virtual ~csBulletColliderTerrainCell ();

  //-- CS::Collisions::iCollider
  virtual ColliderType GetColliderType () const
  { return COLLIDER_TERRAIN_CELL; }

  virtual inline bool IsBaseDynamic () const
  { return false; }

  //-- CS::Collisions::iColliderTerrainCell
  virtual iTerrainCell* GetCell () const { return cell; }
};

//--------------------------------- csBulletCollisionTerrainFactory ---------------------------------

class csBulletCollisionTerrainFactory :
  public scfVirtImplementationExt1<csBulletCollisionTerrainFactory, 
    BulletCollisionObjectFactory, iCollisionTerrainFactory>
{
  friend class csBulletColliderTerrainCell;

  csRef<iTerrainFactory> terrain;

public:
  csBulletCollisionTerrainFactory (csBulletSystem* system, iTerrainFactory* terrain);

  //-- CS::Collisions::iCollisionObjectFactory
  virtual void SetCollider (CS::Collisions::iCollider* value,
			    const csOrthoTransform& transform = csOrthoTransform ()) {}
  virtual CS::Collisions::iCollider* GetCollider () const;

  virtual void SetColliderTransform (const csOrthoTransform& transform) {}
  virtual const csOrthoTransform& GetColliderTransform () const;

  //-- CS::Collisions::iCollisionTerrainFactory
  virtual iTerrainFactory* GetTerrainFactory () const;
  virtual csPtr<iCollisionTerrain> CreateTerrain (iTerrainSystem* terrain);
};

//--------------------------------- csBulletCollisionTerrain ---------------------------------

class csBulletCollisionTerrain :
  public scfVirtImplementationExt3<csBulletCollisionTerrain, 
    csBulletCollisionObject, iCollisionTerrain,
    iTerrainCellLoadCallback, iTerrainCellHeightDataCallback>
{
  friend class csBulletSector;
  friend class csBulletCollisionObject;
  friend class csBulletColliderTerrainCell;

  csRef<csBulletCollisionTerrainFactory> factory;
  csRefArray<csBulletColliderTerrainCell> cells;
  csOrthoTransform terrainTransform;
  csRef<iTerrainSystem> terrainSystem;
  float minimumHeight;
  float maximumHeight;
  
  csRef<CS::Physics::iRigidBodyFactory> cellFactory;

  void CreateCellCollider (iTerrainCell* cell);

public:
  csBulletCollisionTerrain (csBulletCollisionTerrainFactory* factory, iTerrainSystem* terrain,
			    float minimumHeight = 0.0f, float maximumHeight = 0.0f);
  virtual ~csBulletCollisionTerrain ();

  //-- CS::Collisions::iCollisionObject
  virtual CollisionObjectType GetObjectType () const
  { return COLLISION_OBJECT_TERRAIN; }
  virtual iObject* QueryObject () { return this; }

  virtual void SetAttachedSceneNode (iSceneNode* newSceneNode)
  { sceneNode = newSceneNode; }
  virtual void SetTransform (const csOrthoTransform& trans) {}
  virtual csOrthoTransform GetTransform () const
  { return terrainTransform; }
  virtual void SetRotation (const csMatrix3& rot) {}

  virtual void RebuildObject ();
  virtual bool AddBulletObject ();
  virtual bool RemoveBulletObject ();

  //-- CS::Collisions::iCollisionTerrain
  virtual iTerrainSystem* GetTerrain () const { return terrainSystem; }
  virtual iColliderTerrainCell* GetCell (size_t index) const;
  virtual iColliderTerrainCell* GetCell (iTerrainCell* cell) const;
  virtual size_t GetCellCount () const;

  //-- iTerrainCellLoadCallback
  virtual void OnCellLoad (iTerrainCell *cell);
  virtual void OnCellPreLoad (iTerrainCell *cell);
  virtual void OnCellUnload (iTerrainCell *cell);

  //-- iTerrainCellHeightDataCallback
  virtual void OnHeightUpdate (iTerrainCell* cell, const csRect& rectangle);
};

}
CS_PLUGIN_NAMESPACE_END (Bullet2)
#endif
