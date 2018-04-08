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
#include "cssysdef.h"

#include "csutil/stringquote.h"
#include "igeom/trimesh.h"
#include "imesh/objmodel.h"

// Bullet includes
#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"
#include "BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h"
#include "BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h"

#include "collisionterrain.h"
#include "collisionobject2.h"
#include "bulletsystem.h"

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

/**
 * Computes the index into a 1D bullet array, given a 2D CS index (in x/y)
 */
static inline int CSToBulletIndex2D (int x, int y, int w, int h)
{
  return (h-y-1) * w + x;
}

//--------------------------------- csBulletColliderTerrainCell ---------------------------------

csBulletColliderTerrainCell::csBulletColliderTerrainCell
  (csBulletCollisionTerrain* terrain, float* gridData, iTerrainCell *cell,
   float minHeight, float maxHeight)
  :  scfImplementationType (this), terrain (terrain), cell (cell), heightData (gridData)
{
  // Create the Bullet shape
  shape = new btHeightfieldTerrainShape (cell->GetGridWidth (), cell->GetGridHeight (),
					 gridData, 1.0f, minHeight, maxHeight,
					 1, PHY_FLOAT, false);
  
  // Apply the local scaling on the shape
  const csVector3& size = cell->GetSize ();
  csBulletSystem* system = terrain->factory->system;
  btVector3 localScale (size[0] * system->GetInternalScale () / (cell->GetGridWidth () - 1),
			system->GetInternalScale (),
			size[2] * system->GetInternalScale () / (cell->GetGridHeight () - 1));
  shape->setLocalScaling (localScale);

  margin = 0.04 * system->GetInverseInternalScale ();

  // Create the rigid body
  body = new btRigidBody (0, 0, shape, btVector3 (0, 0, 0));	
  body->setUserPointer (dynamic_cast<CS::Collisions::iCollisionObject*> (terrain));
}

csBulletColliderTerrainCell::~csBulletColliderTerrainCell ()
{
  delete body;
  delete shape;
  delete heightData;
}

void csBulletColliderTerrainCell::UpdateHeight (const csRect& area)
{
  int w = cell->GetGridWidth ();
  int h = cell->GetGridHeight ();
  csLockedHeightData newData = cell->LockHeightData (area);

  for (int y = 0; y < area.Height (); y++)
  {
    for (int x = 0; x < area.Width (); x++)
    {
      heightData[CSToBulletIndex2D (x + area.xmin, y + area.ymin, w, h)] = newData.data[y * w + x];
    }
  }
}

//--------------------------------- csBulletCollisionTerrainFactory ---------------------------------

csBulletCollisionTerrainFactory::csBulletCollisionTerrainFactory
(csBulletSystem* system, iTerrainFactory* terrain)
  : scfImplementationType (this, system, nullptr), terrain (terrain)
{}

CS::Collisions::iCollider* csBulletCollisionTerrainFactory::GetCollider () const
{
  return nullptr;
}

const csOrthoTransform& csBulletCollisionTerrainFactory::GetColliderTransform () const
{
  return transform;
}

iTerrainFactory* csBulletCollisionTerrainFactory::GetTerrainFactory () const
{
  return terrain;
}

csPtr<iCollisionTerrain> csBulletCollisionTerrainFactory::CreateTerrain
(iTerrainSystem* terrain)
{
  // Check that the terrain factory is the same
  csRef<iMeshObject> mesh = scfQueryInterface<iMeshObject> (terrain);
  csRef<iTerrainFactory> factory = scfQueryInterface<iTerrainFactory> (mesh->GetFactory ());
  if (this->terrain != factory)
  {
    system->ReportWarning ("Could not create the terrain collision object "
			   "because the terrain factory is not the same");
    return csPtr<iCollisionTerrain> (nullptr);
  }

  // Create the collision object
  return new csBulletCollisionTerrain (this, terrain);
}

//--------------------------------- csBulletCollisionTerrain ---------------------------------

csBulletCollisionTerrain::csBulletCollisionTerrain
  (csBulletCollisionTerrainFactory* factory, iTerrainSystem* terrain,
   float minimumHeight, float maximumHeight)
    : scfImplementationType (this, factory->system), factory (factory),
      terrainSystem (terrain), minimumHeight (minimumHeight), maximumHeight (maximumHeight)
{
  group = factory->group;

  // Register to the terrain events
  terrain->AddCellLoadListener (this);
  terrain->AddCellHeightUpdateListener (this);

  // Find the transform of the terrain
  csRef<iMeshObject> mesh = scfQueryInterface<iMeshObject> (terrain);
  terrainTransform = mesh->GetMeshWrapper ()->GetMovable ()->GetFullTransform ();

  // Create the colliders of the cells that are already loaded
  for (size_t i = 0; i < terrainSystem->GetCellCount (); i++)
  {
    iTerrainCell* cell = terrainSystem->GetCell (i);
    if (cell->GetLoadState () != iTerrainCell::Loaded)
      continue;

    CreateCellCollider (cell);       
  }
}

csBulletCollisionTerrain::~csBulletCollisionTerrain ()
{
  if (sector)
    for (size_t i = 0; i < cells.GetSize (); i++)
      sector->bulletWorld->removeRigidBody (cells[i]->body);
}

void csBulletCollisionTerrain::RebuildObject ()
{
}

bool csBulletCollisionTerrain::RemoveBulletObject ()
{
  if (sector)
    for (size_t i = 0; i < cells.GetSize (); i++)
      sector->bulletWorld->removeRigidBody (cells[i]->body);
  return true;
}

bool csBulletCollisionTerrain::AddBulletObject ()
{
  if (sector)
    for (size_t i = 0; i < cells.GetSize (); i++)
      sector->bulletWorld->addRigidBody (cells[i]->body, group->value, group->mask);
  return true;
}

iColliderTerrainCell* csBulletCollisionTerrain::GetCell (size_t index) const
{
  return cells[index];
}

iColliderTerrainCell* csBulletCollisionTerrain::GetCell (iTerrainCell* cell) const
{
  for (size_t i = 0; i < cells.GetSize (); i++)
    if (cells[i]->cell == cell)
      return cells[i];
  return nullptr;
}

size_t csBulletCollisionTerrain::GetCellCount () const
{
  return cells.GetSize ();
}

void csBulletCollisionTerrain::OnCellLoad (iTerrainCell *cell)
{
  CreateCellCollider (cell);
}

void csBulletCollisionTerrain::OnCellPreLoad (iTerrainCell *cell)
{
}

void csBulletCollisionTerrain::OnCellUnload (iTerrainCell *cell)
{
  for (size_t i = 0; i < cells.GetSize (); ++i)
  {
    if (cells[i]->cell == cell) 
    {
      sector->bulletWorld->removeRigidBody (cells[i]->body);
      cells.DeleteIndexFast (i);
      break;
    }
  }
}

void csBulletCollisionTerrain::CreateCellCollider (iTerrainCell *cell)
{
  float minHeight = 0.0, maxHeight = 0.0;
  csLockedHeightData cellData = cell->GetHeightData ();

  // Check if the min/max have to be computed
  bool needExtremum = (minimumHeight < SMALL_EPSILON && maximumHeight < SMALL_EPSILON);
  if (needExtremum)
    minHeight = maxHeight = cellData.data[0];
  int gridWidth = cell->GetGridWidth ();
  int gridHeight = cell->GetGridHeight ();

  // Initialize the bullet terrain height data
  float* btHeightData = new float[gridHeight*gridWidth];
  for (int j = 0; j < gridHeight; j++)
  {
    for (int i = 0; i < gridWidth; i++)
    {
      float height =
	btHeightData[CSToBulletIndex2D (i, j, gridWidth, gridHeight)]
	= cellData.data[j * gridWidth + i];

      if (needExtremum)
      {
        minHeight = csMin (minHeight, height);
        maxHeight = csMax (maxHeight, height);
      }
    }
  }

  // Set the transform
  csOrthoTransform cellTransform (terrainTransform);
  csVector3 cellPosition  (cell->GetPosition ()[0], 0.0f, cell->GetPosition ()[1]);

  csVector3 cellCenter (
    cell->GetSize ()[0] * 0.5f,
    (maxHeight - minHeight) * 0.5f + minHeight,
    cell->GetSize ()[2] * 0.5f);

  cellTransform.SetOrigin (
    terrainTransform.GetOrigin () +
    terrainTransform.This2OtherRelative (cellPosition) +
    terrainTransform.This2OtherRelative (cellCenter));

  // Create the terrain shape
  csBulletColliderTerrainCell* collider = new csBulletColliderTerrainCell (
    this, btHeightData, cell, minHeight, maxHeight);

  // Set the transform of the cell's collider
  btTransform btTransform = CSToBullet (cellTransform, system->GetInternalScale ());
  collider->body->setWorldTransform (btTransform);
  
  // Add to the list of cells and to the Bullet world
  cells.Push (collider);
  if (sector)
    sector->bulletWorld->addRigidBody (collider->body, group->value, group->mask);
}

// height in the given rectangle of the given cell has changed
void csBulletCollisionTerrain::OnHeightUpdate (iTerrainCell* cell, const csRect& rectangle) 
{
  // Find the cell collider
  for (size_t i = 0; i < cells.GetSize (); i++)
    if (cells[i]->cell == cell)
    {
      cells[i]->UpdateHeight (rectangle);
      return;
    }
}

}
CS_PLUGIN_NAMESPACE_END (Bullet2)
