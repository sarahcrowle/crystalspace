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
* The main loop
*/

#include "cssysdef.h"
#include "csgeom/poly3d.h"
#include "csgeom/sphere.h"
#include "csgeom/quaternion.h"
#include "iengine/portal.h"
#include "imesh/genmesh.h"
#include "imesh/terrain2.h"
#include "cstool/genmeshbuilder.h"
#include "cstool/materialbuilder.h"
#include "physdemo.h"

using namespace CS::Collisions;
using namespace CS::Physics;

void PhysDemo::Frame ()
{
  // Update the demo's state information
  UpdateHUD ();

  // Update the actor
  if (actorMode != ActorModeNoclip)
  {
    if (!paused)
    {
      if (vehicle)
      {
	// Update vehicle
	MoveActorVehicle ();
      }
      else
      {
	// Update actor
	MoveActor ();
      }
    }
  }

  // Update the position of the object currently dragged by the mouse
  UpdateDragging ();

  // Default behavior from DemoApplication
  DemoApplication::Frame ();

  // Display debug informations
  DoDebugDraw ();
}

void PhysDemo::MoveActor ()
{
  // Compute the rotation delta of the actor from the keys activated
  float yaw = 0.0f;
  //float pitch = 0.0f;
  float rotationSpeed = vc->GetElapsedSeconds () * turnSpeed;

  if (kbd->GetKeyState (CSKEY_SHIFT))
    rotationSpeed *= 3.0f;

  if (kbd->GetKeyState (CSKEY_RIGHT) && !kbd->GetKeyState (CSKEY_CTRL))
    yaw += rotationSpeed;

  if (kbd->GetKeyState (CSKEY_LEFT) && !kbd->GetKeyState (CSKEY_CTRL))
    yaw -= rotationSpeed;
/*
  if (kbd->GetKeyState (KeyUp))
    pitch -= rotationSpeed;

  if (kbd->GetKeyState (KeyDown))
    pitch += rotationSpeed;
*/
  // Apply the actor rotation
  player.GetActor ()->Rotate (yaw);

  // Compute the speed of the actor from the keys activated
  csVector3 speed (0.0f);

  float motionSpeed = moveSpeed;

  if (kbd->GetKeyState (CSKEY_SHIFT))
    motionSpeed *= 3.0f;

  if (kbd->GetKeyState (KeyForward) || kbd->GetKeyState (CSKEY_UP))
    speed[2] += motionSpeed;

  if (kbd->GetKeyState (KeyBackward) || kbd->GetKeyState (CSKEY_DOWN))
    speed[2] -= motionSpeed;

  if (kbd->GetKeyState (KeyLeft)
      || (kbd->GetKeyState (CSKEY_LEFT) && kbd->GetKeyState (CSKEY_CTRL)))
    speed[0] -= motionSpeed;

  if (kbd->GetKeyState (KeyRight)
      || (kbd->GetKeyState (CSKEY_RIGHT) && kbd->GetKeyState (CSKEY_CTRL)))
    speed[0] += motionSpeed;

  // Apply the actor speed
  player.GetActor ()->SetSpeed (speed);

  // Jump if needed
  if (kbd->GetKeyState (KeyJump) && player.GetActor ()->IsOnGround ())
    player.GetActor ()->Jump ();

  // TODO: Update the pitch of the camera
}

void PhysDemo::UpdateDragging ()
{
  if (dragging)
  {
    iCamera* cam = view->GetCamera ();
    csOrthoTransform& camTrans = cam->GetTransform ();

    // Keep the drag joint at the same distance to the camera
    csVector2 v2d (mouse->GetLastX (), mouse->GetLastY ());
    csVector3 v3d = view->InvProject (v2d, 1000.f);
    csVector3 startBeam = camTrans.GetOrigin ();
    csVector3 endBeam = camTrans.This2Other (v3d);

    csVector3 newPosition = endBeam - startBeam;
    newPosition.Normalize ();
    newPosition = camTrans.GetOrigin () + newPosition * dragDistance;
    // TODO: switch the joint's collision sector if crossing a portal...
    dragJoint->SetPosition (newPosition);
  }
}

void PhysDemo::UpdateHUD ()
{
  hudManager->GetStateDescriptions ()->Empty ();
  csString txt;

  if (vehicle
      || (player.GetObject () && player.GetObject ()->QueryPhysicalBody ()))
  {
    float speed;
    if (vehicle)
    {
      speed = vehicle->GetSpeedKMH ();
    }
    else
    {
      speed = 3.6f * player.GetObject ()->QueryPhysicalBody ()->GetLinearVelocity ().Norm ();
    }
    txt.Format ("Speed : %.3f km/h", speed);
    hudManager->GetStateDescriptions ()->Push (txt);
  }

  txt.Format ("Collision objects: %zu", GetCurrentSector ()->GetCollisionObjectCount ());
  hudManager->GetStateDescriptions ()->Push (txt);

  txt.Format ("Rigid bodies: %zu", GetCurrentSector ()->GetRigidBodyCount ());
  hudManager->GetStateDescriptions ()->Push (txt);

  if (isSoftBodyWorld)
  {
    txt.Format ("Soft bodies: %zu", GetCurrentSector ()->GetSoftBodyCount ());
    hudManager->GetStateDescriptions ()->Push (txt);
  }

  switch (actorMode)
  {
  case ActorModePhysical:
    hudManager->GetStateDescriptions ()->Push (csString ("Actor mode: Physical"));
    break;

  case ActorModeNoclip:
    hudManager->GetStateDescriptions ()->Push (csString ("Actor mode: No Clip"));
    break;

  default:
    break;
  }
}

void PhysDemo::DoDebugDraw ()
{
  if (do_bullet_debug)
    physicalSystem->DebugDraw (view);
  else if (isSoftBodyWorld && do_soft_debug)
    for (size_t i = 0; i < GetCurrentSector ()->GetSoftBodyCount (); i++)
    {
      CS::Physics::iSoftBody* softBody = GetCurrentSector ()->GetSoftBody (i);
      csRef<CS::Physics::iSoftBody> bulletSoftBody = 
        scfQueryInterface<CS::Physics::iSoftBody> (softBody);
      if (softBody->GetVertexCount ())
        bulletSoftBody->DebugDraw (view);
    }
}
