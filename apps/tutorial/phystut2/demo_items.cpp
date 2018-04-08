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
 * Create and handle some items that actors can use
 */

#include "cssysdef.h"
#include "csgeom/poly3d.h"
#include "csgeom/sphere.h"
#include "iengine/campos.h"
#include "imesh/genmesh.h"
#include "imesh/terrain2.h"
#include "cstool/genmeshbuilder.h"
#include "cstool/materialbuilder.h"
#include "csutil/floatrand.h"
#include "physdemo.h"

#include "ivaria/colliders.h"

using namespace CS::Collisions;
using namespace CS::Physics;
using namespace CS::Geometry;


ItemTemplate* camMgr;


#define ItemFunctionClassName(name) name##function

#define ItemFunctionClassNameI(name, i) name##function##i

// Creates a new ItemFunction class with the given name, description and code
// and adds it as a "kind" (Primary or Secondary) function to the ItemTemplate called "templ"
#define AddItemFunction(kind, name, desc, code) \
  AddItemFunctionToTempl(templ, kind, name, desc, code)

#define AddItemFunctionToTempl(templ, kind, name, desc, code) \
  { \
  class ItemFunctionClassName(name) : public ItemFunction { \
  public: ItemFunctionClassName(name)() : ItemFunction(desc) {} \
  bool Use (Item* item) { code; return true; } \
  }; \
  templ.Add##kind##Function(new ItemFunctionClassName(name)); \
  } 
  

// TODO: Cache-friendly storage of polymorphic objects

void PhysDemo::CreateItemTemplates ()
{
  {
    // Primitive Object Spawner
    ItemTemplate& templ = ItemMgr::Instance->CreateTemplate ("Primitive Object Spawner");
    
    AddItemFunction (Secondary, SpawnSphere, "Spawn sphere", physDemo.SpawnSphere ());
    AddItemFunction (Secondary, SpawnBox, "Spawn box", physDemo.SpawnBox ());
    AddItemFunction (Secondary, SpawnCapsule, "Spawn capsule", physDemo.SpawnCapsule ());
    AddItemFunction (Secondary, SpawnCylinder, "Spawn cylinder", physDemo.SpawnCylinder ());
    AddItemFunction (Secondary, SpawnCone, "Spawn cone", physDemo.SpawnCone ());
    AddItemFunction (Secondary, SpawnBoxStacks, "Spawn box stacks", physDemo.SpawnBoxStacks (10, 5, .5, 80));
  }

  {
    // Complex Object Spawner
    ItemTemplate& templ = ItemMgr::Instance->CreateTemplate ("Complex Object Spawner");
    
    AddItemFunction (Secondary, SpawnCompound, "Spawn compound", physDemo.SpawnCompound ());
    AddItemFunction (Secondary, SpawnConcaveMesh, "Spawn concave mesh", physDemo.SpawnConcaveMesh ());
    AddItemFunction (Secondary, SpawnJointed, "Spawn jointed", physDemo.SpawnJointed ());
    AddItemFunction (Secondary, SpawnChain, "Spawn chain", physDemo.SpawnChain ());
    AddItemFunction (Secondary, SpawnParticles, "Spawn particles", physDemo.SpawnParticles ());
    AddItemFunction (Secondary, SpawnKrystalRagdoll, "Spawn Krystal", physDemo.SpawnKrystalRagdoll ());
    AddItemFunction (Secondary, SpawnFrankieRagdoll, "Spawn Frankie", physDemo.SpawnFrankieRagdoll ());
  }

  if (isSoftBodyWorld)
  {
    // Softbody Object Spawner spawns softbodies, such as cloth or ropes
    ItemTemplate& templ = ItemMgr::Instance->CreateTemplate ("Softbody Object Spawner");

    AddItemFunction (Secondary, SpawnRope, "Spawn rope", physDemo.SpawnRope ());
    AddItemFunction (Secondary, SpawnCloth, "Spawn cloth", physDemo.SpawnCloth ());
    AddItemFunction (Secondary, SpawnSoftBody, "Spawn soft ball", physDemo.SpawnSoftBody ());
  }
  
  {
    // Object & Force Manipulator: Manipulates objects and introduces fun forces
    ItemTemplate& templ = ItemMgr::Instance->CreateTemplate ("Object & Force Manipulator");

    AddItemFunction (Secondary, PullObject, "Pull object", physDemo.PullObject ());
    AddItemFunction (Secondary, DeleteObject, "Delete object", physDemo.DeleteObject ());
    AddItemFunction (Secondary, ToggleObjectState, "Toggle dynamic", physDemo.ToggleObjectState ());
  }

  {
    // Vehicle Manager
    ItemTemplate& templ = ItemMgr::Instance->CreateTemplate ("Vehicle Manager");
    
    AddItemFunction (Primary, EnterTargetVehicle, "Enter target vehicle", physDemo.EnterTargetVehicle ());
    AddItemFunction (Primary, DeleteTargetVehicle, "Delete target vehicle", physDemo.DeleteTargetVehicle ());
    
    AddItemFunction (Secondary, SpawnVehicle, "Spawn new vehicle", physDemo.SpawnVehicle ());
    AddItemFunction (Secondary, AccelerateVehicle, "Leave current vehicle", physDemo.LeaveCurrentVehicle ());
    AddItemFunction (Secondary, AccelerateVehicle, "Accelerate target vehicle", physDemo.AccelerateTargetVehicle ());
  }

  {
    // Level Manager
    ItemTemplate& templ = ItemMgr::Instance->CreateTemplate ("Level Manager");
    
    AddItemFunction (Secondary, SwitchToBox, "Go to Box level", physDemo.SetLevel (PhysDemoLevelBox));
    AddItemFunction (Secondary, SwitchToPortals, "Go to Portals level", physDemo.SetLevel (PhysDemoLevelPortals));
    AddItemFunction (Secondary, SwitchToTerrain, "Go to Terrain level", physDemo.SetLevel (PhysDemoLevelTerrain));
    AddItemFunction (Secondary, SwitchToCastle, "Go to Castle level (static)", physDemo.SetLevel (PhysDemoLevelCastle));
    AddItemFunction (Secondary, SwitchToCastleWithDecomp, "Go to Castle level (dynamic, loads very slowly)", physDemo.SetLevel (PhysDemoLevelCastle, true));
  }

  {
    // Camera Manager
    ItemTemplate& templ = ItemMgr::Instance->CreateTemplate ("Camera Manager");
    
    // Functions are added dynamically
    camMgr = &templ;
  }
}



class TeleToPosFunction : public ItemFunction 
{
  iCameraPosition* pos;

public:
  TeleToPosFunction (iCameraPosition* pos) : ItemFunction (pos->QueryObject ()->GetName ()), pos (pos)
  {
  }
  
  bool Use (Item* item)
  { 
    physDemo.TeleportObject (item->GetInventory ()->GetOwner ()->GetObject (), pos); 
    return true;
  }
};

void PhysDemo::UpdateCameraManager ()
{
  camMgr->Clear ();

  for (size_t i = 0; i < size_t (engine->GetCameraPositions ()->GetCount ()); ++i)
  {
    iCameraPosition* pos = engine->GetCameraPositions ()->Get (i);

    camMgr->AddSecondaryFunction (new TeleToPosFunction (pos));
  }
}

/**
 * 
 */
