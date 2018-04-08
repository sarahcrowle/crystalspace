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
#include "vehicle.h"

#include "csutil/hash.h"
#include "iengine/scenenode.h"

using namespace CS::Collisions;
using namespace CS::Physics;

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

  // ####################################################################################################################################
  // Wheel 
  BulletVehicleWheel::BulletVehicleWheel (csBulletSystem* sys, btWheelInfo& btWheel) : scfImplementationType (this),
    sys (sys),
    btWheel (btWheel),
    isDriven (true), isAffectedByBrake (true)
  {
  }

  BulletVehicleWheel::~BulletVehicleWheel ()
  {
  }

  // ####################################################################################################################################
  // Wheel Factory

  BulletVehicleWheelFactory::BulletVehicleWheelFactory (csBulletSystem* sys) : scfImplementationType (this),
    sys (sys),
    rollInfluence (.1),
    pos (0.0f),
    wheelOrientation (1.0f, 0.0f, 0.0f),
    axleOrientation (0.0f, 1.0f, 0.0f),
    suspensionLength (1),
    radius (.5),
    isDriven (true)
  {
  }

  BulletVehicleWheelFactory::~BulletVehicleWheelFactory ()
  {
  }

  csPtr<iVehicleWheel> BulletVehicleWheelFactory::CreateWheel (btWheelInfo& btWheel)
  {
    BulletVehicleWheel* wheel = new BulletVehicleWheel (sys, btWheel);
    wheel->SetRollInfluence (GetRollInfluence ());
    wheel->SetAxleOrientation (GetAxleOrientation ());
    wheel->SetIsWheelDriven (GetIsWheelDriven ());
    wheel->SetSuspensionLength (GetSuspensionLength ());
    wheel->SetSuspensionOrientation (GetWheelOrientation ());
    wheel->SetWheelPosition (GetWheelPosition ());
    return csPtr<iVehicleWheel> (wheel);
  }

  // ####################################################################################################################################
  // Vehicle Factory

  BulletVehicleFactory::BulletVehicleFactory (csBulletSystem* sys, CS::Collisions::iCollider* collider) :
    scfImplementationType (this, sys, collider),
    sys (sys)
    {
    }

  BulletVehicleFactory::~BulletVehicleFactory ()
  {
  }

  csPtr<iVehicle> BulletVehicleFactory::CreateVehicle (CS::Physics::iPhysicalSector* isector)
  {
    // TODO: move this code in the vehicle constructor
    csRef<BulletVehicle> vehicle = csPtr<BulletVehicle> (new BulletVehicle (this));
    csBulletRigidBody* chassis = dynamic_cast<csBulletRigidBody*> (&*vehicle);
    // TODO: really?
    chassis->SetMass (100.0f);
    chassis->SetState (STATE_DYNAMIC);
    chassis->SetDeactivable (false);
    
    csBulletSector* sector = dynamic_cast<csBulletSector*> (isector);
    btVehicleRaycaster* rayCaster = new btDefaultVehicleRaycaster (sector->GetBulletWorld ());

    // create vehicle
    static const btRaycastVehicle::btVehicleTuning tuning;    // this is unused anyway
    vehicle->btVehicle = new btRaycastVehicle (tuning, chassis->GetBulletRigidPointer (), rayCaster);
    vehicle->btVehicle->setCoordinateSystem (0, 1, 2);

    // create & add bullet wheel objects
    for (size_t i = 0; i < wheelInfos.GetSize (); ++i)
    {
      iVehicleWheelFactory* info = wheelInfos[i];

      vehicle->btVehicle->addWheel (
        CSToBullet (info->GetWheelPosition (), sys->GetInternalScale ()),
        CSToBullet (info->GetWheelOrientation (), sys->GetInternalScale ()),
        CSToBullet (info->GetAxleOrientation (), sys->GetInternalScale ()), 
        info->GetSuspensionLength (),
        info->GetRadius (),
        //info->tuning,
	// TODO
	tuning,
        true                // isFrontWheel is unnecessary and unused information
        );
    }
    
    // create & add CS wheel objects
    // must separate from bullet allocation because the wheel objects will be moved during creation time
    for (size_t i = 0; i < wheelInfos.GetSize (); ++i)
    {
      iVehicleWheelFactory* info = wheelInfos[i];

      btWheelInfo& btWheel = vehicle->btVehicle->getWheelInfo (i);

      BulletVehicleWheelFactory* factory = dynamic_cast<BulletVehicleWheelFactory*> (info);
      csRef<iVehicleWheel> wheel = factory->CreateWheel (btWheel);
      vehicle->wheels.Push (wheel);
    }
    
    return csPtr<iVehicle> (vehicle);
  }

  CS::Physics::iVehicleBrake* BulletVehicleFactory::CreateBrake ()
  {
    csRef<VehicleBrake> brake;
    brake.AttachNew (new VehicleBrake ());
    brakes.Push (brake);
    return brake;
  }

  CS::Physics::iVehicleSteeringDevice* BulletVehicleFactory::CreateSteeringDevice ()
  {
    csRef<VehicleSteeringDevice> steeringDevice;
    steeringDevice.AttachNew (new VehicleSteeringDevice ());
    steeringDevices.Push (steeringDevice);
    return steeringDevice;
  }

  // ####################################################################################################################################
  // Vehicle

  BulletVehicle::BulletVehicle (BulletVehicleFactory* factory)
    : scfImplementationType (this, factory), factory (factory)
  {
  }

  BulletVehicle::~BulletVehicle ()
  {
    delete btVehicle;
  }

  float BulletVehicle::GetEngineForce () const
  {
    for (size_t i = 0; i < wheels.GetSize (); ++i)
    {
      iVehicleWheel* iwheel = wheels[i];
      if (iwheel->GetIsWheelDriven ())
      {
        BulletVehicleWheel* wheel = dynamic_cast<BulletVehicleWheel*> (iwheel);
        return GetDrivenWheelCount () * wheel->btWheel.m_engineForce;
      }
    }
    return 0;
  }

  void BulletVehicle::SetEngineForce (float f)
  {
    f /= GetDrivenWheelCount ();   // divide equally among the driving wheels
    for (size_t i = 0; i < wheels.GetSize (); ++i)
    {
      iVehicleWheel* iwheel = wheels[i];
      if (iwheel->GetIsWheelDriven ())
      {
        BulletVehicleWheel* wheel = dynamic_cast<BulletVehicleWheel*> (iwheel);
        wheel->btWheel.m_engineForce = f;
      }
    }
  }

  size_t BulletVehicle::GetDrivenWheelCount () const
  {
    size_t count = 0;
    for (size_t i = 0; i < wheels.GetSize (); ++i)
    {
      iVehicleWheel* iwheel = wheels[i];
      if (iwheel->GetIsWheelDriven ())
      {
        ++count;
      }
    }
    return count;
  }

  void BulletVehicle::Brake (size_t index, float scale)
  {
    // apply brake
    VehicleBrake* brake = factory->brakes[index];
    float force = scale * brake->GetMaximumForce () / brake->GetAffectedWheelCount ();
    for (size_t i = 0; i < brake->GetAffectedWheelCount (); i++)
    {
      iVehicleWheel* iwheel = wheels[brake->GetAffectedWheel (i)];
      BulletVehicleWheel* wheel = dynamic_cast<BulletVehicleWheel*> (iwheel);

      // add a brake force
      wheel->btWheel.m_brake += force;
    }
  }

  void BulletVehicle::Steer (size_t index, float increment)
  {
    VehicleSteeringDevice* steeringDevice = factory->steeringDevices[index];
    float cap = steeringDevice->GetMaximumSteering ();
    iVehicleWheel* iwheel = wheels[steeringDevice->GetAffectedWheel (0)];
    BulletVehicleWheel* wheel = dynamic_cast<BulletVehicleWheel*> (iwheel);

    // Compute a new steering value for the first wheel
    float& steering = wheel->btWheel.m_steering;
    steering += increment;
    if (steering > cap)
      steering = cap;
    else if (steering < -cap)
      steering = -cap;

    // Apply the same steering to the other affected wheels
    for (size_t i = 1; i < steeringDevice->GetAffectedWheelCount (); ++i)
    {
      iVehicleWheel* iwheel = wheels[steeringDevice->GetAffectedWheel (i)];
      BulletVehicleWheel* wheel = dynamic_cast<BulletVehicleWheel*> (iwheel);
      wheel->btWheel.m_steering = steering;
    }
  }

  void BulletVehicle::PostStep (float dt)
  {
    // update chassis movable to prevent jittering
    iMovable* movable = GetAttachedSceneNode ()->GetMovable ();
    movable->SetFullTransform (GetTransform ());
    movable->UpdateMove ();

    // post-process wheels:
    for (size_t i = 0; i < wheels.GetSize (); ++i)
    {
      iVehicleWheel* iwheel = wheels[i];
      BulletVehicleWheel* wheel = dynamic_cast<BulletVehicleWheel*> (iwheel);

      // reset engine force
      wheel->btWheel.m_engineForce = 0;

      // reset brake force
      wheel->btWheel.m_brake = 0;
      
      // update transformation
      if (wheels[i]->GetAttachedSceneNode ())
      {
        wheels[i]->GetAttachedSceneNode ()->GetMovable ()->SetFullTransform
	  (BulletToCS (wheel->btWheel.m_worldTransform, factory->sys->GetInverseInternalScale ()));
        wheels[i]->GetAttachedSceneNode ()->GetMovable ()->UpdateMove ();
      }
    }
  }

  void BulletVehicle::OnAdded (csBulletSector* sector)
  {
    // TODO: Re-create the vehicle body here instead of at creation time in order
    // to get rid of the 'sector' creation parameter

    // Add the wheel scene nodes to the engine sector
    if (sector->sector)
      for (size_t i = 0; i < wheels.GetSize (); ++i)
      {
	iVehicleWheel* wheel = wheels[i];
	if (wheel->GetAttachedSceneNode ())
	{
	  wheel->GetAttachedSceneNode ()->GetMovable ()->SetSector (sector->sector);
	  wheel->GetAttachedSceneNode ()->GetMovable ()->UpdateMove ();
	}
      }
  }

  void BulletVehicle::OnRemoved (csBulletSector* sector)
  {
    // Remove the wheel scene nodes from the engine sector
    if (sector->sector)
      for (size_t i = 0; i < wheels.GetSize (); ++i)
      {
	iVehicleWheel* wheel = wheels[i];
	if (wheel->GetAttachedSceneNode ())
	{
	  wheel->GetAttachedSceneNode ()->GetMovable ()->SetSector (nullptr);
	  wheel->GetAttachedSceneNode ()->GetMovable ()->UpdateMove ();
	}
      }
  }

}
CS_PLUGIN_NAMESPACE_END (Bullet2)
