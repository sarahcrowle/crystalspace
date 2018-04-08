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
 * Create and handle vehicles
 */

#include "cssysdef.h"
#include "csgeom/poly3d.h"
#include "csgeom/sphere.h"
#include "iengine/portal.h"
#include "imesh/genmesh.h"
#include "imesh/terrain2.h"
#include "cstool/genmeshbuilder.h"
#include "cstool/materialbuilder.h"
#include "csutil/floatrand.h"
#include "physdemo.h"

using namespace CS::Collisions;
using namespace CS::Physics;
using namespace CS::Geometry;

// TODO: remove that

// Default vehicle parameters:

static const float steeringIncrement (.04);
static const float steeringMax (.3);
static const float engineForce (10000);
static const float frontBrakeForce (100);
static const float rearBrakeForce (100);
static const float handBrakeForce (1000);
static const float chassisMass (800);

// The Wheels:

/// Amount of axles
const static int AxleCount (2);

/// Amount of wheels per axle
const static int WheelsPerAxle (2);

/// Length of suspension as factor of the chassis height
const static float SuspensionLengthFactor (1.2);

/// Wheel radius
const static float WheelRadius (.5);

/// Wheel width
const static float WheelWidth (.6);

// The Chassis:

/// Chassis has two parts: One top box
const static csVector3 ChassisSizeTop (3, 2, 3);

/// and one bottom box
const static csVector3 ChassisSizeBottom (3, .9, 5);

// The grid of tires:

/// Relative position to chassis of the first wheel on the left side
const static csVector3 WheelTopLeft (-.6, -.7, .45);

/// Relative position to chassis of the last wheel on the left side
const static csVector3 WheelBottomLeft (-.6, -.7, -.45);

/// Relative position to chassis of the first wheel on the right side
const static csVector3 WheelTopRight (.6, -.7, .45);

// Actor <-> Vehicle interaction

/// The actor seat pos relative to a vehicle
const static csVector3 VehicleActorPos (0);

/// The speed with which the actor moves sideward when jumping out of a vehicle
const static float ActorBailSideSpeed (3);

csPtr<iVehicle> PhysDemo::CreateVehicle ()
{
  // compute dependent parameters
  csVector3 topPos (0, .5f * ChassisSizeTop.y, - .5f * (ChassisSizeBottom.z - ChassisSizeTop.z));
  csVector3 botPos (0, -.5f * ChassisSizeBottom.y, 0);

  // Setup the chassis

  csRef<CS::Collisions::iCollider> chassisCollider = physicalSystem->CreateCollider ();
  csRef<iColliderBox> chassisTopCollider = physicalSystem->CreateColliderBox (ChassisSizeTop);
  csRef<iColliderBox> chassisBottomCollider = physicalSystem->CreateColliderBox (ChassisSizeBottom);
  botPos.y = 0;
  chassisCollider->AddChild (chassisBottomCollider, csOrthoTransform (csMatrix3 (), botPos));
  
  // Create the vehicle factory
  csRef<iVehicleFactory> fact = physicalSystem->CreateVehicleFactory (chassisCollider);
  fact->SetElasticity (DefaultElasticity);
  fact->SetFriction (DefaultFriction);
  fact->SetMass (chassisMass);

  // Create the brakes
  iVehicleBrake* handBrake = fact->CreateBrake ();
  handBrake->SetMaximumForce (handBrakeForce);
  iVehicleBrake* frontBrake = fact->CreateBrake ();
  frontBrake->SetMaximumForce (frontBrakeForce);
  iVehicleBrake* rearBrake = fact->CreateBrake ();
  rearBrake->SetMaximumForce (rearBrakeForce);

  // Create the steering wheel
  iVehicleSteeringDevice* steeringWheel = fact->CreateSteeringDevice ();
  steeringWheel->SetMaximumSteering (steeringMax);

  // Setup the wheels
  for (int axle = 0; axle < AxleCount; ++axle)
  {
    float axleFactor = axle / (AxleCount - 1);
    for (int inAxleN = 0; inAxleN < WheelsPerAxle; ++inAxleN)
    {
      float inAxleFactor = inAxleN / (WheelsPerAxle - 1);

      // Place the wheel uniformly along the grid of axles
      csVector3 pos (WheelTopLeft);
      pos += inAxleFactor * (WheelTopRight - WheelTopLeft);
      pos += axleFactor * (WheelBottomLeft - WheelTopLeft);
      pos = ScaleVector3 (ChassisSizeBottom, pos);

      csRef<iVehicleWheelFactory> wheelFact = physicalSystem->CreateVehicleWheelFactory ();
      wheelFact->SetRollInfluence (0);
      wheelFact->SetFrictionCoefficient (DefaultFriction);
      wheelFact->SetAxleOrientation (csVector3 (-1, 0, 0));
      wheelFact->SetIsWheelDriven (axle == 0);
      wheelFact->SetRadius (WheelRadius);
      wheelFact->SetSuspensionLength (SuspensionLengthFactor * ChassisSizeBottom.y);
      wheelFact->SetSuspensionOrientation (csVector3 (0, -1, 0));
      wheelFact->SetWheelPosition (pos);

      // Setup the way the wheel is controlled by the vehicle
      size_t index = fact->GetWheelFactoryCount ();
      if (axle == 0)
      {
        // front wheel
        steeringWheel->AddAffectedWheel (index);
        frontBrake->AddAffectedWheel (index);
      }
      else if (axle > 0)
      {
        // rear wheel
        rearBrake->AddAffectedWheel (index);
        handBrake->AddAffectedWheel (index);
      }

      fact->AddWheelFactory (wheelFact);
    }
  }

  // Create vehicle
  csRef<iVehicle> vehicle = fact->CreateVehicle (GetCurrentSector ());

  // Set meshes (FIXME)
  csRef<iMeshWrapper> chassisMesh = CreateBoxMesh (ChassisSizeBottom, "misty", "chassis");
  vehicle->SetAttachedSceneNode (chassisMesh->QuerySceneNode ());
  for (size_t i = 0; i < fact->GetWheelFactoryCount (); ++i)
  {
    csRef<iMeshWrapper> wheelMesh = CreateCylinderYMesh (WheelWidth, WheelRadius);
    CS_ASSERT (wheelMesh);
    iVehicleWheel* wheel = vehicle->GetWheel (i);
    wheel->SetAttachedSceneNode (wheelMesh->QuerySceneNode ());
  }   

  GetCurrentSector ()->AddVehicle (vehicle);

  return csPtr<iVehicle>(vehicle);
}

void PhysDemo::MoveActorVehicle ()
{
  // Steering
  float steering = 0.0f;

  if (kbd->GetKeyState (KeyLeft) || kbd->GetKeyState (CSKEY_LEFT))
    steering -= steeringIncrement;

  if (kbd->GetKeyState (KeyRight) || kbd->GetKeyState (CSKEY_RIGHT))
    steering += steeringIncrement;

  vehicle->Steer (0, steering);

  // Acceleration
  if (kbd->GetKeyState (KeyForward) || kbd->GetKeyState (CSKEY_UP))
    vehicle->SetEngineForce (engineForce);

  else 
  {
    // do nothing because engine force is resetted automatically
    //vehicle->SetEngineForce (vehicle->GetEngineForce () * .99);
  }

  if (kbd->GetKeyState (KeyBackward) || kbd->GetKeyState (CSKEY_DOWN))
  {
    // Backward
    vehicle->SetEngineForce (-engineForce / 10);
  }

  if (kbd->GetKeyState (KeyHandbrake))
  {
    // Apply the handbrake (which is the first brake that has been added to the vehicle).
    vehicle->Brake (0);
  }
}

void PhysDemo::EnterTargetVehicle ()
{
  vehicle = GetTargetVehicle ();
  if (!vehicle) return;
/*
  // actor should not physically interact anymore, since in this simple scene, it penetrates the chassis
  if (player.GetObject ()->QueryPhysicalBody () && player.GetObject ()->QueryPhysicalBody ()->QueryRigidBody ())
  {
    player.GetObject ()->QueryPhysicalBody ()->QueryRigidBody ()->SetState (STATE_KINEMATIC);
  }
*/
  // Attach the camera to the vehicle's body
  UpdateActorMode (ActorModeNone);
  vehicle->SetAttachedCamera (view->GetCamera ());
}

void PhysDemo::LeaveCurrentVehicle ()
{
  if (!vehicle) return;
/*
  // reset actor
  if (player.GetObject ()->QueryPhysicalBody () && player.GetObject ()->QueryPhysicalBody ()->QueryRigidBody ())
  {
    player.GetObject ()->QueryPhysicalBody ()->QueryRigidBody ()->SetState (STATE_DYNAMIC);
  }
  //player.GetObject ()->SetCollisionGroup ("Default");
  */

  vehicle->SetAttachedCamera (nullptr);
  UpdateActorMode (ActorModePhysical);
  
  // Move and accelerate actor:
  iCollisionObject* actorObj = player.GetObject ();

  const csOrthoTransform& vehicleTrans = vehicle->GetTransform ();
  csVector3 sideward = vehicleTrans.GetT2O ().Col1 ();

  // Place actor beside vehicle to avoid collision
  csOrthoTransform actorTrans = actorObj->GetTransform ();
  float sideDist = ChassisSizeBottom.x / 1.8 + 2 * ActorDimensions.x;
  actorTrans.Translate (sideDist * sideward);
  csVector3 pos (actorTrans.GetOrigin ());
  if (!GetPointOnGroundBeneathPos (pos, pos))
  {
    if (!GetPointOnGroundBeneathPos (pos, pos)) pos = actorTrans.GetOrigin ();
  }
  actorTrans.SetOrigin (pos + csVector3 (0.0f, ActorDimensions[UpAxis], 0.0f));   // place above terrain
  actorTrans.SetT2O (csMatrix3 ());                                                // no rotation
  actorObj->SetTransform (actorTrans);

  if (actorObj->QueryPhysicalBody ())
  {
    // Actor bails out with a small sideward velocity + velocity of the vehicle
    iPhysicalBody* actorBody = actorObj->QueryPhysicalBody ();

    csVector3 forward = vehicle->GetLinearVelocity ();
    actorBody->SetLinearVelocity (ActorBailSideSpeed * sideward + forward);
  }
  else
  {
    // nothing happens
  }

  // Unset the current vehicle
  vehicle = nullptr;
}

void PhysDemo::SpawnVehicle ()
{
  // Create a new vehicle
  csRef<iVehicle> vehicle = CreateVehicle ();
  csOrthoTransform trans (player.GetObject ()->GetTransform ());
  csVector3 forward (trans.GetT2O ().Col3 ());

  // Place in front of player
  csVector3 pos (trans.GetOrigin ());
  pos += 2 * ChassisSizeBottom.z * forward;
  pos[UpAxis] += 3 * ChassisSizeBottom.y;

  //csMatrix3 rotation (trans.GetT2O ());
  csMatrix3 rotation;
  vehicle->SetTransform (csOrthoTransform (rotation, pos));
}

void PhysDemo::DeleteTargetVehicle ()
{
  iVehicle* vehicle = GetTargetVehicle ();
  if (!vehicle) return;

  // Remove from world
  if (vehicle == this->vehicle)
  {
    LeaveCurrentVehicle ();
  }
  GetCurrentSector ()->RemoveVehicle (vehicle);
}

void PhysDemo::AccelerateTargetVehicle ()
{
  iVehicle* vehicle = GetTargetVehicle ();
  if (!vehicle) return;

  vehicle->SetEngineForce (engineForce);
}

iVehicle* PhysDemo::GetTargetVehicle ()
{
  HitBeamResult hitResult;
  if (PickCursorObject (hitResult))
  {
    csRef<iVehicle> vehicle = scfQueryInterface<iVehicle> (hitResult.object);
    return vehicle;
  }
  return nullptr;
}
