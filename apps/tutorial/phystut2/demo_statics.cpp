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

/**
 * Load / setup / modify the static (non-interactive) background and scenery objects
 */

#include "cssysdef.h"
#include "csgeom/poly3d.h"
#include "csgeom/sphere.h"
#include "iengine/portal.h"
#include "imesh/genmesh.h"
#include "imesh/terrain2.h"
#include "cstool/genmeshbuilder.h"
#include "cstool/materialbuilder.h"
#include "physdemo.h"

#include "iengine/campos.h"

const static float StaticElasticity = 0.1f;

const csString DefaultSectorName ("defaultsector");

using namespace CS::Collisions;
using namespace CS::Physics;
using namespace CS::Geometry;

void PhysDemo::CreateBoxRoom (const csVector3& roomExtents, const csVector3& pos, float wallThickness)
{
  // The boxes that make up floor and ceiling
  // AABB over these two is the entire room, including walls
  csVector3 yExtents (
    roomExtents[HorizontalAxis1] + 2 * wallThickness,
    wallThickness,
    roomExtents[HorizontalAxis2] + 2 * wallThickness);
  RenderMeshColliderPair yLimitingPair;
  CreateBoxMeshColliderPair (yLimitingPair, yExtents);

  // The boxes that make up one pair of walls
  // AABB over these two is the y-limited part of the room
  csVector3 xExtents (
    wallThickness,
    roomExtents[HorizontalAxis1],
    roomExtents[HorizontalAxis2] + 2 * wallThickness);
  RenderMeshColliderPair xLimitingPair;
  CreateBoxMeshColliderPair (xLimitingPair, xExtents);

  // The boxes that make up the other pair of walls
  // AABB over these two is the y- and x- limited part of the room
  csVector3 zExtents (
    roomExtents[HorizontalAxis2],
    roomExtents[HorizontalAxis1],
    wallThickness);
  RenderMeshColliderPair zLimitingPair;
  CreateBoxMeshColliderPair (zLimitingPair, zExtents);
  
  //csVector3 halfRoomExtents = roomExtents / 2;
  csVector3 distances = (roomExtents + csVector3 (wallThickness)) / 2;

  SpawnRigidBody (xLimitingPair, pos + csVector3 (distances[HorizontalAxis1], 0, 0), 0, 0)->SetElasticity (StaticElasticity);
  SpawnRigidBody (xLimitingPair, pos + csVector3 (-distances[HorizontalAxis1], 0, 0), 0, 0)->SetElasticity (StaticElasticity);

  SpawnRigidBody (yLimitingPair, pos + csVector3 (0, distances[UpAxis], 0), 0, 0)->SetElasticity (StaticElasticity);
  SpawnRigidBody (yLimitingPair, pos + csVector3 (0, -distances[UpAxis], 0), 15, 0)->SetElasticity (StaticElasticity);

  SpawnRigidBody (zLimitingPair, pos + csVector3 (0, 0, distances[HorizontalAxis2]), 0, 0)->SetElasticity (StaticElasticity);
  SpawnRigidBody (zLimitingPair, pos + csVector3 (0, 0, -distances[HorizontalAxis2]), 0, 0)->SetElasticity (StaticElasticity);
}

void PhysDemo::CreateBoxRoom (float size)
{ 
  // Create and setup sector
  room = engine->CreateSector (DefaultSectorName);
  iPhysicalSector* sector =
    physicalSystem->CreateCollisionSector (room)->QueryPhysicalSector ();
  if (player.GetObject ())
    sector->AddCollisionObject (player.GetObject ());
  view->GetCamera ()->SetSector (room);

  // Setup the camera position
  iCameraPosition* pos = engine->GetCameraPositions ()->NewCameraPosition ("Center");
  pos->Set (DefaultSectorName, csVector3 (0, 1.0f - size * .5f, 0), csVector3 (0, 0, 1), UpVector);

  // Create the room
  csVector3 roomExtents (size);
  csVector3 halfRoomExtents (.5f * roomExtents);
  csVector3 roomPos (0);
  float wallThickness = 5;
  CreateBoxRoom (roomExtents, roomPos, wallThickness);

  // Set up some lights
  csRef<iLight> light;
  iLightList* lightList = room->GetLights ();
  lightList->RemoveAll ();

  room->SetDynamicAmbientLight (csColor (0.3f, 0.3f, 0.3f));

  float distance = size * 0.6f;
  light = engine->CreateLight (0, csVector3 (distance, 0.0f, 0.0f), size, csColor (1));
  lightList->Add (light);

  light = engine->CreateLight (0, csVector3 (-distance, 0.0f,  0.0f), size, csColor (1));
  lightList->Add (light);

  light = engine->CreateLight (0, csVector3 (0.0f, distance, 0.0f), size, csColor (1));
  lightList->Add (light);

  light = engine->CreateLight (0, csVector3 (0.0f, -distance, 0.0f), size, csColor (1));
  lightList->Add (light);

  light = engine->CreateLight (0, csVector3 (0.0f, 0.0f, distance), size, csColor (1));
  lightList->Add (light);

  light = engine->CreateLight (0, csVector3 (0.0f, 0.0f, -distance), size, csColor (1));
  lightList->Add (light);

  engine->Prepare ();
  CS::Lighting::SimpleStaticLighter::ShineLights (room, engine, 6);
}

bool PhysDemo::LoadLevel (const char* pathname, bool convexDecomp)
{
  csString folder, filename;
  GetFolderAndFile (pathname, folder, filename);

  printf ("Loading level: %s...\n", pathname);

  // Load the level file
  csRef<iVFS> VFS (csQueryRegistry<iVFS> (GetObjectRegistry ()));

  VFS->PushDir ();
  if (!VFS->ChDir (folder.GetData ()))
  {
    ReportError ("ERROR: Couldn't find directory \"%s\" for level: %s", folder.GetData (), pathname);
    return false;
  }

  if (!loader->LoadMapFile (filename, false))
  {
    ReportError ("ERROR: Couldn't load file \"%s\" for level: %s", filename.GetData (), pathname);
    return false;
  }
  VFS->PopDir ();
  
  // Initialize the physical world from the content of the engine
  collisionHelper.InitializeCollisionObjects (engine);

  // Set the default sector
  room = engine->GetSectors ()->Get (0);
  view->GetCamera ()->SetSector (room);

  CS_ASSERT (room && "Invalid level - Has no sectors.");

  printf ("Done - level loaded: %s\n", pathname);
  return true;
}

iModifiableDataFeeder* PhysDemo::GetFirstTerrainModDataFeeder (CS::Collisions::iCollisionSector* sector)
{
  // iterate over all terrains in the sector
/*
  for (size_t i = 0; i < sector->GetCollisionTerrainCount (); ++i)
  {
    iTerrainSystem* terrain = sector->GetCollisionTerrain (i)->GetTerrain ();
    csRef<iMeshObject> terrainObj = scfQueryInterface<iMeshObject>(terrain);
    if (terrainObj)
    {
      // Get the factory
      csRef<iTerrainFactory> factory = scfQueryInterface<iTerrainFactory>(terrainObj->GetFactory ());
      CS_ASSERT (factory);

      // Get the data feeder and check if its modifiable
      // NOTE: By default, data feeders are not modifiable
      csRef<iModifiableDataFeeder> terrainFeeder = scfQueryInterface<iModifiableDataFeeder> (factory->GetFeeder ());
      if (terrainFeeder)
      {
        return terrainFeeder;
      }
    }
  }
*/
  return nullptr;
}
