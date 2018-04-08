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
#include "cssysdef.h"
#include "csgeom/poly3d.h"
#include "csgeom/sphere.h"
#include "cstool/genmeshbuilder.h"
#include "cstool/materialbuilder.h"
#include "iengine/campos.h"
#include "imap/services.h"
#include "imesh/genmesh.h"
#include "imesh/terrain2.h"
#include "ivaria/convexdecompose.h"
#include "ivaria/engseq.h"

#include "physdemo.h"

using namespace CS::Collisions;
using namespace CS::Physics;

PhysDemo physDemo;

PhysDemo::PhysDemo ()
  : DemoApplication ("CrystalSpace.PhysTut2"),
  isSoftBodyWorld (true), do_bullet_debug (false),
  do_soft_debug (false), allStatic (false), 
  paused (false), simulationSpeed (1.0f),
  dragging (false), softDragging (false),
  debugMode (DEBUG_COLLIDERS),
  moveSpeed (3.f),
  turnSpeed (1.6f),
  actorMode (ActorModePhysical),
  //actorMode (ActorModeNoclip),
  selectedItem (nullptr)
{
}

PhysDemo::~PhysDemo ()
{
}

void PhysDemo::PrintHelp ()
{
  csCommandLineHelper commandLineHelper;

  // Command line options
  commandLineHelper.AddCommandLineOption
    ("phys_engine", "Specify which physics plugin to use", csVariant ("bullet2"));
  commandLineHelper.AddCommandLineOption
    ("concave", "Enable the dynamic concave objects", csVariant (true));
  commandLineHelper.AddCommandLineOption
    ("soft", "Enable the soft bodies", csVariant (true));
  commandLineHelper.AddCommandLineOption
    ("mapfile", csString ().Format ("Specify a map file to be loaded"), csVariant (""));
  commandLineHelper.AddCommandLineOption
    ("convexdecompose", csString ().Format ("Whether to perform convex decomposition on all render meshes"), csVariant (false));

  // Printing help
  commandLineHelper.PrintApplicationHelp
    (GetObjectRegistry (), "phystut2",
    "phystut2 <OPTIONS>",
    "Tutorial for the physics2 plugin for Crystal Space. ");
}

bool PhysDemo::OnInitialize (int argc, char* argv[])
{
  // Default behavior from DemoApplication
  if (!DemoApplication::OnInitialize (argc, argv))
    return false;

  csBaseEventHandler::Initialize (GetObjectRegistry ());
  if (!RegisterQueue (GetObjectRegistry (), csevAllEvents (GetObjectRegistry ())))
    return ReportError ("Failed to set up event handler!");

  csRef<iPluginManager> plugmgr = csQueryRegistry<iPluginManager> (GetObjectRegistry ());
  
  // Load the Physics plugin
  physicalSystem = csLoadPlugin<CS::Physics::iPhysicalSystem> (plugmgr, "crystalspace.physics.bullet");
  if (!physicalSystem) return ReportError ("Could not load the Bullet plugin!");
  //physicalSystem->SetInternalScale (10.0f);

  // Load Convex Decomposition plugin
  convexDecomposer = csLoadPlugin<iConvexDecomposer> (plugmgr, "crystalspace.mesh.convexdecompose.hacd");
  if (!convexDecomposer) 
  {
    // It is not vital to the operation of this demo
    ReportWarning ("Could not load the convex decomposition plugin!");
  }
  
  // Check whether or not the dynamic concave objects are enabled
  bool dynamicConcave = clp->GetBoolOption ("concave", true);
  physicalSystem->SetDynamicConcaveEnabled (dynamicConcave);

  // Check whether or not the soft bodies are enabled
  isSoftBodyWorld = clp->GetBoolOption ("soft", true);
  physicalSystem->SetSoftBodyEnabled (isSoftBodyWorld);

  // Initialize the collision helper
  collisionHelper.Initialize (GetObjectRegistry (), physicalSystem);

  // Get the commandline parser
  csRef<iCommandLineParser> clp = csQueryRegistry<iCommandLineParser> (GetObjectRegistry ());

  // Load the soft body animation control plugin & factory
  if (isSoftBodyWorld)
  {
    softBodyAnimationType = csLoadPlugin<CS::Animation::iSoftBodyAnimationControl2Type>
      (plugmgr, "crystalspace.physics.softanim2");
    if (!softBodyAnimationType)
      return ReportError ("Could not load the soft body animation controller for the genmeshes!");

    softBodyAnimationFactory = softBodyAnimationType->CreateAnimationControlFactory ();
  }

  // Load the ragdoll plugin
  ragdollManager = csLoadPlugin<CS::Animation::iSkeletonRagdollNodeManager2>
    (plugmgr, "crystalspace.mesh.animesh.animnode.ragdoll2");
  if (!ragdollManager)
    return ReportError ("Failed to locate the ragdoll manager!");

  // Load the skeleton model plugin
  modelManager = csLoadPlugin<CS::Animation::iSkeletonModelManager>
    (plugmgr, "crystalspace.mesh.animesh.model");
  if (!modelManager)
    return ReportError ("Failed to locate the skeleton model plugin!");

  return true;
}

bool PhysDemo::Application ()
{
  // Default behavior from DemoApplication
  if (!DemoApplication::Application ())
    return false;

  // Setup the camera
  cameraManager->SetCamera (view->GetCamera ());

  // Set a background color
  float value = 0.298f;
  csColor4 color (value, value, value, 1.f);
  view->SetBackgroundColor (&color);

  // Disable the camera manager
  cameraManager->SetCameraMode (CS::Utility::CAMERA_NO_MOVE);
  cameraManager->SetMouseMoveEnabled (false);

  // Initialize Player items
  CreateItemTemplates ();
  for (size_t i = 0; i < ItemMgr::Instance->GetTemplateCount (); ++i)
  {
    ItemTemplate& templ = ItemMgr::Instance->GetTemplate (i);
    player.GetInventory ().AddItem (templ);
  }

  if (ItemMgr::Instance->GetTemplateCount ())
    selectedItem = player.GetInventory ().GetItem (0);
  selectedIndex = 0;

  // Load specified scene
  csRef<iCommandLineParser> clp = csQueryRegistry<iCommandLineParser> (GetObjectRegistry ());
  csString pathname = clp->GetOption ("mapfile");
  bool convexdecompose = clp->GetBoolOption ("convexdecompose");

  // Load or create scene
  if (SetLevel (pathname, convexdecompose))
  {
    Run ();
    return true;
  }
  return false;
}

void PhysDemo::Reset ()
{
  // reset all other variables
  mainCollider = nullptr;

  stackBoxMeshPair.Collider = nullptr;
  stackBoxMeshPair.MeshFactory = nullptr;

  dragJoint = nullptr;
  draggedBody = nullptr;

  player.SetObject (nullptr);
  actor = nullptr;

  clipboardBody = nullptr;
  clipboardMovable = nullptr;
  
  ghostObject = nullptr;

  moddedTerrainFeeder = nullptr;
  terrainMod = nullptr;

  walls = nullptr;

  // Remove all physical sectors
  physicalSystem->DeleteAll ();

  // Remove everything in the engine that existed before
  engine->DeleteAll ();
  engine->ResetWorldSpecificSettings ();
  engine->GetCameraPositions ()->RemoveAll ();
  csRef<iEngineSequenceManager> seqMgr = csQueryRegistry<iEngineSequenceManager> (object_reg);
  if (seqMgr)
  {
    seqMgr->RemoveTriggers ();
    seqMgr->RemoveSequences ();
  }
}

CS::Physics::iPhysicalSector* PhysDemo::GetCurrentSector () const
{ 
  iCollisionSector* collisionSector =
    physicalSystem->FindCollisionSector (view->GetCamera ()->GetSector ());
  if (!collisionSector) return nullptr;
  return collisionSector->QueryPhysicalSector ();
}

bool PhysDemo::SetLevel (PhysDemoLevel level, bool convexDecomp)
{
  csString path;
  switch (level)
  {
  case PhysDemoLevelBox:
    break;

  case PhysDemoLevelPortals:
    path = "/data/portals/world";
    break;

  case PhysDemoLevelTerrain:
    path = "/lev/terraini/world";
    break;

  case PhysDemoLevelCastle:
    path = "/data/castle/world";
    break;

  default:
    break;
  }
  return SetLevel (path, convexDecomp);
}

bool PhysDemo::SetLevel (const char* mapPath, bool convexDecomp)
{
  // Reset scene
  Reset ();

  // Initialize the actor
  UpdateActorMode (actorMode);

  // Preload some materials and objects
  if (!loader->LoadTexture ("fire", "/lib/std/castle/fire1.png")) return ReportError ("Error loading texture: fire");
  if (!loader->LoadTexture ("stone", "/lib/std/stone4.gif")) return ReportError ("Could not load texture: stone");
  if (!loader->LoadTexture ("objtexture", "/lib/std/blobby.jpg")) return ReportError ("Error loading texture: blobby");
  if (!loader->LoadTexture ("misty", "/lib/std/misty.jpg")) return ReportError ("Error loading texture: misty");
  if (!loader->LoadTexture ("spark", "/lib/std/spark.png")) return ReportError ("Error loading texture: spark");

  if (!loader->LoadLibraryFile ("/data/phystut/flag.xml")) return ReportError ("Error loading library: flag");
  if (!loader->LoadLibraryFile ("/data/phystut/chainlink")) return ReportError ("Error loading library: chainlink");
  
  // Create the environment
  bool loaded = false;
  if (mapPath)
  {
    loaded = LoadLevel (mapPath, convexDecomp);
    currentMap = mapPath;
    if (!loaded)
    {
      ReportWarning ("Falling back to default level: Box room");
    }
  }
  
  if (!loaded)
  {
    // fall back to default
    CreateBoxRoom (30.0f);
    currentMap = "";
  }

  // Finalize stuff in the engine after scene setup
  engine->Prepare ();

  // Update Camera Manager item
  UpdateCameraManager ();

  // Initialize the HUD
  SetupHUD ();

  // Create a default camera positions if there are none
  iCameraPositionList* cameraPositions = engine->GetCameraPositions ();
  if (!cameraPositions->GetCount ())
  {
    iCameraPosition* position = cameraPositions->NewCameraPosition ("start");
    position->SetSector (engine->GetSectors ()->Get (0)->QueryObject ()->GetName ());
  }

  // Move the actor to the initial position
  TeleportObject (player.GetObject (), cameraPositions->Get (0));

  return true;
}

void PhysDemo::SetupHUD ()
{
  // Setup the descriptions in the HUD
  iStringArray& desc = *hudManager->GetKeyDescriptions ();
  desc.Empty ();

  if (selectedItem && selectedItem->GetTemplate ().GetPrimaryFunctions ().GetSize ())
  {
    ItemFunction* func;
    func = selectedItem->GetTemplate ().GetPrimaryFunction (0);
    desc.Push (csString ("Left Mouse Button: ") + func->GetName ());
    func = selectedItem->GetTemplate ().GetPrimaryFunction (1);
    desc.Push (csString ("Right Mouse Button: ") + func->GetName ());
  }
  else
  {
    desc.Push ("Left Mouse: fire!");
    desc.Push ("Right Mouse: drag object");
  }
  
  desc.Push ("Q, E, PGUP, PGDN: Pan Camera");
  desc.Push ("W, A, S, D: Move");
  desc.Push ("SPACE: Jump");

  desc.Push ("R: reset");

  desc.Push ("CTRL-x: cut selected object");
  desc.Push ("CTRL-v: paste object");
  
  desc.Push ("C: switch between actor modes (dynamic, kinematic, noclip)");
  desc.Push ("P: pause the simulation");
  desc.Push ("O: toggle speed of simulation");
  desc.Push ("L: toggle Bullet debug display");

  desc.Push ("K: toggle display of collisions");
  desc.Push ("G: toggle gravity");

  desc.Push ("]: Toggle heightmap modifier");

  desc.Push ("CTRL-i: start profiling");
  desc.Push ("CTRL-o: stop profiling");
  desc.Push ("CTRL-p: dump profile");
  
  if (selectedItem)
  {
    ItemTemplate& templ = selectedItem->GetTemplate ();
    desc.Push (csString ().Format ("--Current Tool (%s)--", templ.GetName ()));
    for (size_t i = 0; i < csMin (templ.GetPrimaryFunctions ().GetSize (), size_t (2)); ++i)
    {
      ItemFunction* func = templ.GetPrimaryFunction (i);
      desc.Push (csString ().Format (" %s: %s", i == 0 ? "LMB" : "RMB", func->GetName ()));
    }
    for (size_t i = 0; i < templ.GetSecondaryFunctions ().GetSize (); ++i)
    {
      ItemFunction* func = templ.GetSecondaryFunction (i);
      desc.Push (csString ().Format (" %zu: %s", i+1, func->GetName ()));
    }
  }
  else
  {
    desc.Push ("--(No Tool selected)--");
  }

  desc.Push ("--Tools--");
  for (size_t i = 0; i < player.GetInventory ().GetItems ().GetSize (); ++i)
  {
    Item* item = player.GetInventory ().GetItem (i);
    desc.Push (csString ().Format (" F%zu: %s", i+1, item->GetName ()));
  }
}


// ####################################################################################################################
// Misc stuff


void PhysDemo::UpdateActorMode (ActorMode newActorMode)
{
  actorMode = newActorMode;
  iSector* currentSector = view->GetCamera ()->GetSector ();

  if (actorMode == ActorModePhysical)
  {
    cameraManager->SetCameraMode (CS::Utility::CAMERA_NO_MOVE);

    if (!actor)
    {
      csRef<CS::Collisions::iColliderBox> collider =
	physicalSystem->CreateColliderBox (ActorDimensions);
      csRef<iCollisionActorFactory> factory =
	physicalSystem->CreateCollisionActorFactory (collider);
      factory->SetStepHeight (0.4);

      actor = factory->CreateCollisionActor ();
    }

    actor->SetTransform (view->GetCamera ()->GetTransform ());
    iCollisionSector* collisionSector =
      physicalSystem->FindCollisionSector (view->GetCamera ()->GetSector ());
    if (collisionSector)
      collisionSector->AddCollisionObject (actor);
    player.SetObject (actor);
    actor->SetAttachedCamera (view->GetCamera ());
  }

  else
  {
    // No clip mode
    if (actor)
    {
      iCollisionSector* collisionSector =
	physicalSystem->FindCollisionSector (currentSector);
      if (collisionSector)
	collisionSector->RemoveCollisionObject (actor);
      actor.Invalidate ();
      player.SetObject (nullptr);
    }

    view->GetCamera ()->SetSector (currentSector);
    cameraManager->SetCameraMode (CS::Utility::CAMERA_MOVE_FREE);
  }
}

bool PhysDemo::IsDynamic (CS::Collisions::iCollisionObject* obj) const
{
  return obj->QueryPhysicalBody () &&
    (!obj->QueryPhysicalBody ()->QueryRigidBody ()
     || obj->QueryPhysicalBody ()->QueryRigidBody ()->GetState () == STATE_DYNAMIC);
}

bool PhysDemo::IsActor (CS::Collisions::iCollisionObject* obj) const
{
  return obj->QueryCollisionActor () != nullptr;
}

void PhysDemo::SetGravity (const csVector3& g)
{
  for (size_t i = 0; i < physicalSystem->GetCollisionSectorCount (); ++i)
  {
    csRef<iPhysicalSector> sector = scfQueryInterface<iPhysicalSector>(physicalSystem->GetCollisionSector (i));
    sector->SetGravity (g);
  }
}

void PhysDemo::ResetCurrentLevel ()
{
  SetLevel (currentMap);
}

bool PhysDemo::GetPointOnGroundBeneathPos (const csVector3& pos, csVector3& groundPos) const
{
  csVector3 to = pos - 10000 * UpVector;
  HitBeamResult result = GetCurrentSector ()->HitBeam (pos, to);

  if (result.hasHit)
  {
    groundPos = result.isect;
    return true;
  }
  return false;
}

bool PhysDemo::GetPointOnGroundAbovePos (const csVector3& pos, csVector3& groundPos) const
{
  csVector3 to = pos + 10000 * UpVector;
  HitBeamResult result = GetCurrentSector ()->HitBeam (pos, to);

  if (result.hasHit)
  {
    groundPos = result.isect;
    return true;
  }
  return false;
}

//---------------------------------------------------------------------------

CS_IMPLEMENT_APPLICATION
  
/// We use this to keep the physical system alive until the end to make sure its destroyed last
// TODO: why the need to be destroyed last?
csRef<iPhysicalSystem> _physSys;

int main (int argc, char* argv[])
{
  _physSys = physDemo.physicalSystem;
  return physDemo.Main (argc, argv);
}
