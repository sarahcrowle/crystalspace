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

#ifndef __CS_BULLET_VEHICLE_H__
#define __CS_BULLET_VEHICLE_H__

#include "ivaria/vehicle.h"
#include "rigidbody2.h"
#include "updatable.h"

#include "BulletDynamics/Vehicle/btRaycastVehicle.h"
#include "BulletDynamics/Dynamics/btActionInterface.h"

struct iSceneNode;

/**
 * Vehicle TODO:
 * - Add sockets for seats to make sure that passenger transforms are relative to vehicle
 * - Add mesh factories & materials to Vehicle- and VehicleWheel- factories
 * - Add convex ray caster for wheels (currently wheels are physically assumed to be thin disks, guided by a single down-ward ray-cast)
 * - Add proper ray casting and collision support for vehicle wheels (by adding ghost objects for wheels)
 * - Add gears and gearbox
 * - Produce a pretty sample vehicle in phystut2 that looks *real*
 * - Make sure that actor animations interact correctly with vehicles (correct entering/leaving of vehicles of different size & velocity etc)
 */

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
  class BulletVehicle;
  class BulletVehicleFactory;
  class csBulletRigidBody;

  class BulletVehicleWheel : public scfVirtImplementationExt1<
    BulletVehicleWheel, csObject, CS::Physics::iVehicleWheel>
  {
    friend class BulletVehicle;
    friend class BulletVehicleFactory;
    friend class BulletVehicleWheelFactory;

    csBulletSystem* sys;
    csRef<iSceneNode> sceneNode;
    btWheelInfo& btWheel;
    // TODO: is this used?
    bool isDriven, isAffectedByBrake;

  public:
    BulletVehicleWheel (csBulletSystem* sys, btWheelInfo& btWheel);
    virtual ~BulletVehicleWheel ();

    virtual iSceneNode* GetAttachedSceneNode () const { return sceneNode; }
    virtual void SetAttachedSceneNode (iSceneNode* m) { sceneNode = m; }

    virtual float GetSuspensionStiffness () const { return btWheel.m_suspensionStiffness; }
    virtual void SetSuspensionStiffness (float s) { btWheel.m_suspensionStiffness = s; }

    virtual float GetSuspensionDamping () const
    { return btWheel.m_wheelsDampingCompression; }
    virtual void SetSuspensionDamping (float s)
    { btWheel.m_wheelsDampingCompression = btWheel.m_wheelsDampingRelaxation = s; }

    virtual float GetMaxSuspensionDisplacementCM () const { return btWheel.m_maxSuspensionTravelCm; }
    virtual void SetMaxSuspensionDisplacementCM (float s) { btWheel.m_maxSuspensionTravelCm = s; }

    virtual float GetFrictionCoefficient () const { return btWheel.m_frictionSlip; }
    virtual void SetFrictionCoefficient (float s) { btWheel.m_frictionSlip = s; }

    virtual float GetMaxSuspensionForce () const { return btWheel.m_maxSuspensionForce; }
    virtual void SetMaxSuspensionForce (float s) { btWheel.m_maxSuspensionForce = s; }

    virtual float GetRollInfluence () const { return btWheel.m_rollInfluence; }
    virtual void SetRollInfluence (float infl) { btWheel.m_rollInfluence = infl; }

    virtual bool GetIsWheelDriven () const { return isDriven; }
    virtual void SetIsWheelDriven (bool d) { isDriven = d; }

    virtual bool GetIsWheelAffectedByBrake () const { return isAffectedByBrake; }
    virtual void SetIsWheelAffectedByBrake (bool b) { isAffectedByBrake = b; }

    virtual float GetSuspensionLength () const { return btWheel.m_suspensionRestLength1; }
    virtual void SetSuspensionLength (float s) { btWheel.m_suspensionRestLength1 = s; }

    virtual float GetRadius () const { return btWheel.m_wheelsRadius; }
    virtual void SetRadius (float s) { btWheel.m_wheelsRadius = s; }

    csVector3 GetWheelPosition () const
    { return BulletToCS (btWheel.m_chassisConnectionPointCS, sys->GetInternalScale ()); }
    void SetWheelPosition (const csVector3& p)
    { btWheel.m_chassisConnectionPointCS = CSToBullet (p, sys->GetInverseInternalScale ()); }

    csVector3 GetWheelOrientation () const
    { return BulletToCS (btWheel.m_wheelDirectionCS, sys->GetInternalScale ()); }
    void SetSuspensionOrientation (const csVector3& o)
    { btWheel.m_wheelDirectionCS = CSToBullet (o, sys->GetInverseInternalScale ()); }

    csVector3 GetAxleOrientation () const
    { return BulletToCS (btWheel.m_wheelAxleCS, sys->GetInternalScale ()); }
    void SetAxleOrientation (const csVector3& o)
    { btWheel.m_wheelAxleCS = CSToBullet (o, sys->GetInverseInternalScale ()); }

    const float GetRotation () const { return btWheel.m_rotation; }
    void SetRotation (float r) { btWheel.m_rotation = r; }
  };

  class BulletVehicleWheelFactory : public scfVirtImplementationExt1<
    BulletVehicleWheelFactory, csObject, CS::Physics::iVehicleWheelFactory>
  {
    friend class BulletVehicleFactory;

  private:
    csBulletSystem* sys;
    btRaycastVehicle::btVehicleTuning tuning;
    float rollInfluence;
    csVector3 pos, wheelOrientation, axleOrientation;
    float suspensionLength;
    float radius;
    bool isDriven;

  public:
    BulletVehicleWheelFactory (csBulletSystem* sys);
    virtual ~BulletVehicleWheelFactory ();

    csPtr<CS::Physics::iVehicleWheel> CreateWheel (btWheelInfo& btWheel);

    virtual float GetSuspensionStiffness () const { return tuning.m_suspensionStiffness; }
    virtual void SetSuspensionStiffness (float s) { tuning.m_suspensionStiffness = s; }

    virtual float GetSuspensionDamping () const { return tuning.m_suspensionCompression; }
    virtual void SetSuspensionDamping (float s) { tuning.m_suspensionCompression = tuning.m_suspensionDamping = s; }

    virtual float GetMaxSuspensionDisplacementCM () const { return tuning.m_maxSuspensionTravelCm; }
    virtual void SetMaxSuspensionDisplacementCM (float s) { tuning.m_maxSuspensionTravelCm = s; }

    virtual float GetFrictionCoefficient () const { return tuning.m_frictionSlip; }
    virtual void SetFrictionCoefficient (float s) { tuning.m_frictionSlip = s; }

    virtual float GetMaxSuspensionForce () const { return tuning.m_maxSuspensionForce; }
    virtual void SetMaxSuspensionForce (float s) { tuning.m_maxSuspensionForce = s; }

    virtual float GetRollInfluence () const { return rollInfluence; }
    virtual void SetRollInfluence (float infl) { rollInfluence = infl; }

    virtual bool GetIsWheelDriven () const { return isDriven; }
    virtual void SetIsWheelDriven (bool d) { isDriven = d; }

    virtual float GetSuspensionLength () const { return suspensionLength; }
    virtual void SetSuspensionLength (float s) { suspensionLength = s; }

    virtual float GetRadius () const { return radius; }
    virtual void SetRadius (float r) { radius = r; }

    const csVector3& GetWheelPosition () const { return pos; }
    void SetWheelPosition (const csVector3& p) { pos = p; }

    csVector3 GetWheelOrientation () const { return wheelOrientation; }
    void SetSuspensionOrientation (const csVector3& o) { wheelOrientation = o; }

    const csVector3& GetAxleOrientation () const { return axleOrientation; }
    void SetAxleOrientation (const csVector3& o) { axleOrientation = o; }
  };

  class VehicleBrake : public scfImplementation1<VehicleBrake, CS::Physics::iVehicleBrake>
  {
    float maxForce;
    csArray<size_t> affectedWheels;

  public:
    VehicleBrake ()
      : scfImplementationType (this),
      maxForce (1000.0f)
      {}
    virtual ~VehicleBrake () {}

    virtual inline float GetMaximumForce () const { return maxForce; }
    virtual inline void SetMaximumForce (float f) { maxForce = f; }

    virtual inline size_t GetAffectedWheelCount () const
    { return affectedWheels.GetSize (); }
    virtual inline size_t GetAffectedWheel (size_t index) const
    { return affectedWheels[index]; }
    virtual inline void AddAffectedWheel (size_t index)
    { affectedWheels.Push (index); }
  };

  class VehicleSteeringDevice : public scfImplementation1
    <VehicleSteeringDevice, CS::Physics::iVehicleSteeringDevice>
  {
    float maxForce;
    csArray<size_t> affectedWheels;

  public:
    VehicleSteeringDevice ()
      : scfImplementationType (this),
      maxForce (.4f)
      {}
    virtual ~VehicleSteeringDevice () {}

    virtual inline float GetMaximumSteering () const { return maxForce; }
    virtual inline void SetMaximumSteering (float f) { maxForce = f; }

    virtual inline size_t GetAffectedWheelCount () const
    { return affectedWheels.GetSize (); }
    virtual inline size_t GetAffectedWheel (size_t index) const
    { return affectedWheels[index]; }
    virtual inline void AddAffectedWheel (size_t index)
    { affectedWheels.Push (index); }
  };

  class BulletVehicleFactory : public scfVirtImplementationExt1<
    BulletVehicleFactory, BulletRigidBodyFactory, CS::Physics::iVehicleFactory>
  {
    friend class BulletVehicle;

  private:
    csBulletSystem* sys;
    csRefArray<CS::Physics::iVehicleWheelFactory> wheelInfos;
    csRefArray<VehicleBrake> brakes;
    csRefArray<VehicleSteeringDevice> steeringDevices;

  public:
    BulletVehicleFactory (csBulletSystem* sys, CS::Collisions::iCollider* collider);
    virtual ~BulletVehicleFactory ();

    virtual csPtr<CS::Physics::iVehicle> CreateVehicle (CS::Physics::iPhysicalSector* sector);

    virtual void AddWheelFactory (CS::Physics::iVehicleWheelFactory* info)
    { wheelInfos.Push (info); }
    virtual size_t GetWheelFactoryCount ()
    { return wheelInfos.GetSize (); }
    virtual CS::Physics::iVehicleWheelFactory* GetWheelFactory (size_t index) 
    { return wheelInfos[index]; }

    virtual CS::Physics::iVehicleBrake* CreateBrake ();
    virtual CS::Physics::iVehicleSteeringDevice* CreateSteeringDevice ();
  };

  class BulletVehicle : public scfImplementationExt3<BulletVehicle, csBulletRigidBody, 
    CS::Physics::iVehicle, BulletActionWrapper, scfFakeInterface<iUpdatable> >
  {
    friend class BulletVehicleFactory;

  private:
    csRef<BulletVehicleFactory> factory;
    btRaycastVehicle* btVehicle;
    csRefArray<CS::Physics::iVehicleWheel> wheels;

  public:
    BulletVehicle (BulletVehicleFactory* factory);
    virtual ~BulletVehicle ();

    virtual float GetEngineForce () const;
    virtual void SetEngineForce (float f);
    
    virtual void Brake (size_t index, float scale = 1.0f);
    virtual void Steer (size_t index, float scale = 0.4f);

    virtual size_t GetDrivenWheelCount () const;
    virtual CS::Physics::iVehicleWheel* GetWheel (size_t index) const
    { return wheels[index]; }

    virtual CS::Collisions::iCollisionObject* GetCollisionObject () { return this; }

    virtual float GetSpeedKMH () const { return btVehicle->getCurrentSpeedKmHour (); }

    virtual void PreStep (float dt) {}
    virtual void PostStep (float dt);

    virtual void OnAdded (csBulletSector* sector);
    virtual void OnRemoved (csBulletSector* sector);

    virtual btActionInterface* GetBulletAction () { return btVehicle; }
  };

}
CS_PLUGIN_NAMESPACE_END (Bullet2)
#endif
