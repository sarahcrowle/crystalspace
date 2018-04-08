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
* Handle keyboard/mouse/etc input
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

using namespace CS::Collisions;
using namespace CS::Physics;

bool PhysDemo::OnKeyboard (iEvent &event)
{
  //DemoApplication::OnKeyboard (event);
  csKeyEventType eventtype = csKeyEventHelper::GetEventType (&event);
  if (eventtype != csKeyEventTypeDown) return false;

  utf32_char code = csKeyEventHelper::GetCookedCode (&event);
  if (code >= '1' && code <= '9')
  {
    // A number key has been pressed -> Use the tool function
    if (selectedItem)
    {
      size_t i = size_t (code - '1');
      if (i < selectedItem->GetTemplate ().GetSecondaryFunctions ().GetSize ())
      {
        ItemFunction* func = selectedItem->GetTemplate ().GetSecondaryFunction (i);
        if (func)
        {
          return func->Use (selectedItem);
        }
      }
    }
    return false;
  }

  else if (code == '+')
  {
    selectedIndex = (selectedIndex + 1) % player.GetInventory ().GetItems ().GetSize ();
    selectedItem = player.GetInventory ().GetItem (selectedIndex);
    SetupHUD ();
  }
  else if (code == '-')
  {
    if (!selectedIndex)
      selectedIndex = player.GetInventory ().GetItems ().GetSize () - 1;
    else selectedIndex--;
    selectedItem = player.GetInventory ().GetItem (selectedIndex);
    SetupHUD ();
  }

  else if (code >= CSKEY_F1 && code <= CSKEY_F12)
  {
    // F-key has been pressed -> Select tool
    size_t i = size_t (code - CSKEY_F1);
    if (i < player.GetInventory ().GetItems ().GetSize ())
    {
      // A different item has been selected: Select it and update HUD descriptions
      selectedItem = player.GetInventory ().GetItem (i);
      SetupHUD ();
      return true;
    }
    return false;
  }
  
  else  if (csKeyEventHelper::GetCookedCode (&event) == CSKEY_ESC)
  {
    csRef<iEventQueue> q (csQueryRegistry<iEventQueue> (GetObjectRegistry ()));
    if (q) q->GetEventOutlet ()->Broadcast (csevQuit (GetObjectRegistry ()));
    return true;
  }

  else if (code == 'n')
  {
    hudManager->SwitchKeysPage ();
    return true;
  }

  else if (code == 'r')
  {
    // reset
    physDemo.ResetCurrentLevel ();
    return true;
  }
  else if (code == 'c' && !vehicle)    // don't switch modes while in vehicle
  {
    // Toggle camera mode
    switch (actorMode)
    {
    case ActorModeNone:
    case ActorModePhysical:
      actorMode = ActorModeNoclip;
      break;

    case ActorModeNoclip:
      actorMode = ActorModePhysical;
      break;
    }

    UpdateActorMode (actorMode);
    return true;
  }

  else if (code == 'p')
  {
    // Toggle the pause mode of the dynamic simulation
    if (paused)
    {
      paused = false;
      physicalSystem->SetSimulationSpeed (simulationSpeed);
      printf ("Dynamic simulation resumed\n");
    }
    else
    {
      paused = true;
      physicalSystem->SetSimulationSpeed (0.0f);
      printf ("Dynamic simulation paused\n");
    }

    return true;
  }

  else if (code == 'o')
  {
    // Toggle the speed of the dynamic simulation
    if (simulationSpeed - 0.025f < EPSILON)
    {
      simulationSpeed = 1.0f;
      printf ("Dynamic simulation at normal speed\n");
    }
    else
    {
      simulationSpeed = 0.025f;
      printf ("Dynamic simulation slowed\n");
    }
    if (!paused)
      physicalSystem->SetSimulationSpeed (simulationSpeed);

    return true;
  }

  else if (code == 'k')
  {
    // Toggle dynamic system visual debug mode
    // TODO
    if (do_bullet_debug)
    {
      switch (debugMode)
      {
      case CS::Physics::DEBUG_NOTHING:
        debugMode = CS::Physics::DEBUG_COLLIDERS;
        break;
      case CS::Physics::DEBUG_COLLIDERS:
        debugMode = CS::Physics::DEBUG_AABB;
        break;
      case CS::Physics::DEBUG_AABB:
        debugMode = CS::Physics::DEBUG_JOINTS;
        break;
      case CS::Physics::DEBUG_JOINTS:
        debugMode = CS::Physics::DEBUG_NOTHING;
        break;
      }
      physicalSystem->SetDebugMode (debugMode);
    }
    return true;
  }
  else if (code == 'l')
  {
    // Toggle collision debug mode
    if (do_bullet_debug)
    {
      do_bullet_debug = false;
      do_soft_debug = true;
    }
    else if (do_soft_debug)
      do_soft_debug = false;
    else
      do_bullet_debug = true;

    return true;
  }
  else if (code == 'g')
  {
    // Toggle gravity.
    SetGravity (GetCurrentSector ()->GetGravity ().IsZero (EPSILON)? csVector3 (0.0f, -9.81f, 0.0f) : csVector3 (0));
    return true;
  }

  // Cut operation
  else if (csKeyEventHelper::GetRawCode (&event) == 'x'
    && kbd->GetKeyState (CSKEY_CTRL))
  {
    // Trace a beam to find if a rigid body was under the mouse cursor
    csRef<iCamera> camera = view->GetCamera ();
    csVector2 v2d (mouse->GetLastX (), mouse->GetLastY ());
    csVector3 v3d = view->InvProject (v2d, 1000.f);
    csVector3 startBeam = camera->GetTransform ().GetOrigin ();
    csVector3 endBeam = camera->GetTransform ().This2Other (v3d);

    CS::Collisions::HitBeamResult hitResult =
      GetCurrentSector ()->HitBeamPortals (startBeam, endBeam);
    if (hitResult.hasHit && IsDynamic (hitResult.object))
    {
      // Remove the body and the mesh from the simulation, and put them in the clipboard

      clipboardBody = hitResult.object->QueryPhysicalBody ();
      clipboardMovable = hitResult.object->GetAttachedSceneNode ()->GetMovable ();

      if (clipboardBody->QueryRigidBody ())
      {
        CS::Physics::iRigidBody* rigidBody = clipboardBody->QueryRigidBody ();
        if (rigidBody->GetState () == CS::Physics::STATE_DYNAMIC)
        {
          size_t count = GetCurrentSector ()->GetRigidBodyCount ();
          GetCurrentSector ()->RemoveCollisionObject (clipboardBody->QueryRigidBody ());
          //room->GetMeshes ()->Remove (clipboardMovable->GetSceneNode ()->QueryMesh ());
          if (GetCurrentSector ()->GetRigidBodyCount () == count)
            clipboardBody.Invalidate ();
        }
      }
      else
      {
        CS::Physics::iSoftBody* softBody = clipboardBody->QuerySoftBody ();
        GetCurrentSector ()->RemoveCollisionObject (softBody);
      }
    }
  }

  // Paste operation
  else if (csKeyEventHelper::GetRawCode (&event) == 'v'
    && kbd->GetKeyState (CSKEY_CTRL)
    && clipboardBody.IsValid ())
  {
    // Compute the new position of the body
    csRef<iCamera> camera = view->GetCamera ();
    csVector2 v2d (mouse->GetLastX (), mouse->GetLastY ());
    csVector3 v3d = view->InvProject (v2d, 1000.f);
    csVector3 startBeam = camera->GetTransform ().GetOrigin ();
    csVector3 endBeam = camera->GetTransform ().This2Other (v3d);

    csVector3 newPosition = endBeam - startBeam;
    newPosition.Normalize ();
    csOrthoTransform newTransform = camera->GetTransform ();
    newTransform.SetOrigin (newTransform.GetOrigin () + newPosition * 1.5f);

    // Put back the body from the clipboard to the simulation
    if (clipboardBody->QueryRigidBody ())
    {
      clipboardBody->SetTransform (newTransform);
      GetCurrentSector ()->AddCollisionObject (clipboardBody->QueryRigidBody ());
    }
    else
    {
      CS::Physics::iSoftBody* softBody = clipboardBody->QuerySoftBody ();
      GetCurrentSector ()->AddCollisionObject (softBody);
    }

    clipboardBody = 0;
    clipboardMovable = 0;
  }
  // TODO: move in tool options in order to free keyboard keys
  else if (csKeyEventHelper::GetRawCode (&event) == 'i'
    && kbd->GetKeyState (CSKEY_CTRL))
  {
    printf ("Starting profile...\n");
    physicalSystem->StartProfile ();
    return true;
  }

  else if (csKeyEventHelper::GetRawCode (&event) == 'o'
    && kbd->GetKeyState (CSKEY_CTRL))
  {
    printf ("Stopping profile...\n");
    physicalSystem->StopProfile ();
    return true;
  }

  else if (csKeyEventHelper::GetRawCode (&event) == 'p'
    && kbd->GetKeyState (CSKEY_CTRL))
  {
    physicalSystem->DumpProfile ();
    return true;
  }
  
  csRef<iCamera> cam = view->GetCamera ();
  switch (code)
  {
  case ']':
    // Terrain stuff
    if (!terrainMod)
    {
      // Get feeder
      moddedTerrainFeeder = GetFirstTerrainModDataFeeder (GetCurrentSector ());
      if (moddedTerrainFeeder)
      {
        // Apply new mod
        csVector3 pos (0);

        float len = 100;
        float height = 100;

        // TODO: The cells seem to have a different coordinate system, so "pos" is not in sector coordinates
        terrainMod = moddedTerrainFeeder->AddModifier (pos, len, height);
      }
      else
      {
        // Cannot modify terrain
        ReportWarning ("There is no modifiable terrain in this sector!");
      }
    }
    else
    {
      // Remove existing mod
      moddedTerrainFeeder->RemoveModifier (terrainMod);
      terrainMod = nullptr;
    }
    return true;
  }

  return false;
}

bool PhysDemo::OnMouseDown (iEvent &event)
{
  size_t button = size_t (csMouseEventHelper::GetButton (&event));
  if (selectedItem && 
    selectedItem->GetTemplate ().GetPrimaryFunctions ().GetSize () && 
    button < selectedItem->GetTemplate ().GetPrimaryFunctions ().GetSize ())
  {
    // Use tool
    ItemFunction* func = selectedItem->GetTemplate ().GetPrimaryFunction (button);
    if (func)
    {
      return func->Use (selectedItem);
    }
  }

  // Tool has no mouse button overrides
  if (button == 0)
  {
    // Find the rigid body that was clicked on
    HitBeamResult hitResult;
    if (PickCursorObject (hitResult) && IsDynamic (hitResult.object))
    {
      csRef<CS::Physics::iPhysicalBody> physicalBody = hitResult.object->QueryPhysicalBody ();

      // Add an impulse at the point clicked
      csVector3 impulse = hitResult.isect - view->GetCamera ()->GetTransform ().GetOrigin ();
      impulse.Normalize ();

      // Scale the impulse to the total mass of the body
      impulse *= physicalBody->GetMass ();

      if (physicalBody->QueryRigidBody ())
	physicalBody->QueryRigidBody ()->ApplyImpulse (impulse, hitResult.isect);

      else
        physicalBody->QuerySoftBody ()->ApplyImpulse (impulse, hitResult.vertexIndex);
    }
    else
      return false;
    return true;
  }

  // Right mouse button: dragging
  else if (button == 1)
  {
    // Find the rigid body that was clicked on
    // Compute the end beam points
    csRef<iCamera> camera = view->GetCamera ();
    csVector2 v2d (mouse->GetLastX (), mouse->GetLastY ());
    csVector3 v3d = view->InvProject (v2d, 1000.f);
    csVector3 startBeam = camera->GetTransform ().GetOrigin ();
    csVector3 endBeam = camera->GetTransform ().This2Other (v3d);

    // Trace the physical beam
    CS::Collisions::HitBeamResult hitResult =
      GetCurrentSector ()->HitBeamPortals (startBeam, endBeam);
    if (!hitResult.hasHit || !hitResult.object) return false;

    if (IsDynamic (hitResult.object))
    {
      // Check if we hit a rigid body
      csRef<CS::Physics::iPhysicalBody> physicalBody = hitResult.object->QueryPhysicalBody ();
      if (physicalBody->QueryRigidBody ())
      {
        csRef<CS::Physics::iRigidBody> bulletBody = scfQueryInterface<CS::Physics::iRigidBody> (physicalBody);

        // Create a pivot joint at the point clicked
	csRef<CS::Physics::iJointFactory> jointFactory =
	  physicalSystem->CreatePivotJointFactory ();
	dragJoint = jointFactory->CreateJoint ();
	dragJoint->SetTransform (csOrthoTransform (csMatrix3 (), hitResult.isect));
	dragJoint->Attach (bulletBody, nullptr);
        bulletBody->GetSector ()->QueryPhysicalSector ()->AddJoint (dragJoint);

        dragging = true;
        dragDistance = (hitResult.isect - startBeam).Norm ();

        // Set some dampening on the rigid body to have a more stable dragging
        linearDampening = bulletBody->GetLinearDamping ();
        angularDampening = bulletBody->GetAngularDamping ();
        bulletBody->SetLinearDamping (0.9f);
        bulletBody->SetAngularDamping (0.9f);
      }
      else
      {
        softDragging = true;
        draggedBody = physicalBody->QuerySoftBody ();
        draggedVertex = hitResult.vertexIndex;
        dragDistance = (hitResult.isect - startBeam).Norm ();
        grabAnimationControl.AttachNew (new MouseAnchorAnimationControl (this));
        physicalBody->QuerySoftBody ()->AnchorVertex (hitResult.vertexIndex, grabAnimationControl);
      }
    }
    else 
      return false;
    return true;
  }

  return false;
}

bool PhysDemo::OnMouseUp (iEvent &event)
{
  if (csMouseEventHelper::GetButton (&event) == 1
    && dragging)
  {
    dragging = false;

    // Put back the original dampening on the rigid body
    csRef<CS::Physics::iRigidBody> bulletBody =
      scfQueryInterface<CS::Physics::iRigidBody> (dragJoint->GetAttachedBody (0));
    bulletBody->SetLinearDamping (linearDampening);
    bulletBody->SetAngularDamping (angularDampening);

    // Remove the drag joint
    GetCurrentSector ()->RemoveJoint (dragJoint);
    dragJoint = nullptr;
    return true;
  }

  if (csMouseEventHelper::GetButton (&event) == 1
    && softDragging)
  {
    softDragging = false;
    draggedBody->RemoveAnchor (draggedVertex);
    draggedBody = nullptr;
  }

  return false;
}

// This method updates the position of the dragging for soft bodies
csVector3 MouseAnchorAnimationControl::GetAnchorPosition () const
{
  // Keep the drag joint at the same distance to the camera
  csRef<iCamera> camera = simple->view->GetCamera ();
  csVector2 v2d (simple->mouse->GetLastX (), simple->mouse->GetLastY ());
  csVector3 v3d = simple->view->InvProject (v2d, 1000.f);
  csVector3 startBeam = camera->GetTransform ().GetOrigin ();
  csVector3 endBeam = camera->GetTransform ().This2Other (v3d);

  csVector3 newPosition = endBeam - startBeam;
  newPosition.Normalize ();
  newPosition = camera->GetTransform ().GetOrigin () + newPosition * simple->dragDistance;
  return newPosition;
}
