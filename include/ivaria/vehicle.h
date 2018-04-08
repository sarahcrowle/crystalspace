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

#ifndef __IVARIA_VEHICLE__
#define __IVARIA_VEHICLE__

/**\file
 * Physical vehicles
 */
/*
#include "csutil/csobject.h"
#include "csutil/array.h"
#include "csutil/scf.h"
#include "csutil/scf_interface.h"
#include "cstool/primitives.h"
#include "iengine/engine.h"
#include "iutil/objreg.h"
*/
#include "ivaria/physics.h"

namespace CS 
{
  namespace Physics 
  {
    struct iRigidBody;
    struct iRigidBodyFactory;
    struct iVehicle;

    /**
     * The wheel of a vehicle
     */
    struct iVehicleWheel : public virtual iBase
    {
      SCF_INTERFACE (CS::Physics::iVehicleWheel, 1, 0, 0);

      /// Get the scene node attached to this wheel.
      virtual iSceneNode* GetAttachedSceneNode () const = 0;
      /// Set the scene node attached to this wheel.
      virtual void SetAttachedSceneNode (iSceneNode* node) = 0;

      /// The spring constant. Stiffer suspension generates more force when spring is displaced.
      virtual float GetSuspensionStiffness () const = 0;
      /// The spring constant. Stiffer suspension generates more force when spring is displaced.
      virtual void SetSuspensionStiffness (float s) = 0;

      /// Suspension with more damping needs more force to be compressed and to relax
      virtual float GetSuspensionDamping () const = 0;
      /// Suspension with more damping needs more force to be compressed and to relax
      virtual void SetSuspensionDamping (float s) = 0;

      /// The suspension spring's endpoint can only be displaced from the equlibrium by +/- this value (in cm)
      virtual float GetMaxSuspensionDisplacementCM () const = 0;
      /// The suspension spring's endpoint can only be displaced from the equlibrium by +/- this value (in cm)
      virtual void SetMaxSuspensionDisplacementCM (float s) = 0;

      /**
      * When the tangential impulse on the wheel surpases suspension force times this value, it starts slipping.
      * It is very similar to muh in Coulomb's law.
      * A greater value reduces the chance of the vehicle slipping.
      * When on a wet or slippery road, the coefficient should be very small.
      */
      virtual float GetFrictionCoefficient () const = 0;
      /**
      * When the tangential impulse on the wheel surpases suspension force times this value, it starts slipping.
      * It is very similar to muh in Coulomb's law.
      * A greater value reduces the chance of the vehicle slipping.
      * When on a wet or slippery road, the coefficient should be very small.
      */
      virtual void SetFrictionCoefficient (float s) = 0;

      /// The max force to be applied from the wheel to the chassis, caused by an impact
      virtual float GetMaxSuspensionForce () const = 0;
      /// The max force to be applied from the wheel to the chassis, caused by an impact
      virtual void SetMaxSuspensionForce (float s) = 0;
      
      /// Value between 0 and 1 that determines how easily the car can roll over its side
      virtual float GetRollInfluence () const = 0;
      /// Value between 0 and 1 that determines how easily the car can roll over its side
      virtual void SetRollInfluence (float infl) = 0;
      
      // Geometry & Other

      /// Whether this wheel is driven by the engine
      virtual bool GetIsWheelDriven () const = 0;
      /// Whether this wheel is driven by the engine
      virtual void SetIsWheelDriven (bool d) = 0;

      /// Whether this wheel is driven by the engine
      virtual bool GetIsWheelAffectedByBrake () const = 0;
      /// Whether this wheel is driven by the engine
      virtual void SetIsWheelAffectedByBrake (bool b) = 0;

      /// Length of the suspension in equilibrium
      virtual float GetSuspensionLength () const = 0;
      /// Length of the suspension in equilibrium
      virtual void SetSuspensionLength (float s) = 0;

      /// Radius of the wheel
      virtual float GetRadius () const = 0;
      /// Radius of the wheel
      virtual void SetRadius (float s) = 0;

      /// The position of the wheel relative to the chassis
      virtual csVector3 GetWheelPosition () const = 0;
      /// Value between 0 and 1 that determines how easily the car can roll over its side
      virtual void SetWheelPosition (const csVector3& p) = 0;

      /// Unit vector that describes the current rotation of the wheel (perpendicular to its axle)
      virtual csVector3 GetWheelOrientation () const = 0;
      /// Unit vector that describes the current rotation of the wheel (perpendicular to its axle)
      virtual void SetSuspensionOrientation (const csVector3& o) = 0;

      /// Unit vector that describes the axle about which the wheel rotates
      virtual csVector3 GetAxleOrientation () const = 0;
      /// Unit vector that describes the axle about which the wheel rotates
      virtual void SetAxleOrientation (const csVector3& o) = 0;
      
      // Run-time parameters

      /// Rotation in radians
      virtual const float GetRotation () const = 0;
      /// Rotation in radians
      virtual void SetRotation (float r) = 0;
    };

    /**
     * The factory for the wheel of a vehicle
     */
    struct iVehicleWheelFactory : public virtual iBase
    {
      SCF_INTERFACE (CS::Physics::iVehicleWheelFactory, 1, 0, 0);

      /// The spring constant. Stiffer suspension generates more force when spring is displaced.
      virtual float GetSuspensionStiffness () const = 0;
      /// The spring constant. Stiffer suspension generates more force when spring is displaced.
      virtual void SetSuspensionStiffness (float s) = 0;

      /// Suspension with more damping needs more force to be compressed and to relax
      virtual float GetSuspensionDamping () const = 0;
      /// Suspension with more damping needs more force to be compressed and to relax
      virtual void SetSuspensionDamping (float s) = 0;

      /// The suspension spring's endpoint can only be displaced from the equlibrium by +/- this value (in cm)
      virtual float GetMaxSuspensionDisplacementCM () const = 0;
      /// The suspension spring's endpoint can only be displaced from the equlibrium by +/- this value (in cm)
      virtual void SetMaxSuspensionDisplacementCM (float s) = 0;

      /**
      * When the tangential impulse on the wheel surpases suspension force times this value, it starts slipping.
      * It is very similar to muh in Coulomb's law.
      * A greater value reduces the chance of the vehicle slipping.
      * When on a wet or slippery road, the coefficient should be very small.
      */
      virtual float GetFrictionCoefficient () const = 0;
      /**
      * When the tangential impulse on the wheel surpases suspension force times this value, it starts slipping.
      * It is very similar to muh in Coulomb's law.
      * A greater value reduces the chance of the vehicle slipping.
      * When on a wet or slippery road, the coefficient should be very small.
      */
      virtual void SetFrictionCoefficient (float s) = 0;

      /// The max force to be applied from the wheel to the chassis, caused by an impact
      virtual float GetMaxSuspensionForce () const = 0;
      /// The max force to be applied from the wheel to the chassis, caused by an impact
      virtual void SetMaxSuspensionForce (float s) = 0;
      
      /// Value between 0 and 1 that determines how easily the car can roll over its side
      virtual float GetRollInfluence () const = 0;
      /// Value between 0 and 1 that determines how easily the car can roll over its side
      virtual void SetRollInfluence (float infl) = 0;

      /// Whether this wheel is driven by the engine
      // TODO: creating engines might be cleaner
      virtual bool GetIsWheelDriven () const = 0;
      /// Whether this wheel is driven by the engine
      virtual void SetIsWheelDriven (bool d) = 0;

      /// Length of the suspension in equilibrium
      virtual float GetSuspensionLength () const = 0;
      /// Length of the suspension in equilibrium
      virtual void SetSuspensionLength (float s) = 0;

      /// Radius of the wheel
      virtual float GetRadius () const = 0;
      /// Radius of the wheel
      virtual void SetRadius (float r) = 0;

      /// The position of the wheel relative to the chassis
      virtual const csVector3& GetWheelPosition () const = 0;
      /// Value between 0 and 1 that determines how easily the car can roll over its side
      virtual void SetWheelPosition (const csVector3& p) = 0;

      /// Unit vector that describes the current rotation of the wheel (perpendicular to its axle)
      virtual csVector3 GetWheelOrientation () const = 0;
      /// Unit vector that describes the current rotation of the wheel (perpendicular to its axle)
      virtual void SetSuspensionOrientation (const csVector3& o) = 0;

      /// Unit vector that describes the axle about which the wheel rotates
      virtual const csVector3& GetAxleOrientation () const = 0;
      /// Unit vector that describes the axle about which the wheel rotates
      virtual void SetAxleOrientation (const csVector3& o) = 0;
    };

    /**
     * A brake acts on a given set of wheels with a given amount of force.
     */
    struct iVehicleBrake : public virtual iBase
    {
      SCF_INTERFACE (CS::Physics::iVehicleBrake, 1, 0, 0);

      /**
       * Get the maximum breaking force that can be applied by this brake. This
       * total amount of force will be divided up among all affected wheels.
       */
      virtual float GetMaximumForce () const = 0;
      /**
       * Set the maximum breaking force that can be applied by this brake. This
       * total amount of force will be divided up among all affected wheels. The
       * default value is 1000.0f.
       */
      virtual void SetMaximumForce (float f) = 0;

      /// Get the amount of wheels affected by this brake
      virtual size_t GetAffectedWheelCount () const = 0;
      /**
       * Get the wheel affected by this brake of the given index. The value
       * returned is the index of the wheel in the iVehicleFactory.
       */
      virtual size_t GetAffectedWheel (size_t index) const = 0;
      /**
       * Add a new wheel to be affected by this brake. \a index is the index of
       * the wheel in the iVehicleFactory.
       */
      virtual void AddAffectedWheel (size_t index) = 0;
    };
    
    /**
     * A steering device steers a given set of tires with a given maximum amount 
     */
    struct iVehicleSteeringDevice : public virtual iBase
    {
      SCF_INTERFACE (CS::Physics::iVehicleSteeringDevice, 1, 0, 0);

      /**
       * Get the maximum steering angle that can be applied by this device, in
       * radian. The default value is 0.4f.
       */
      virtual float GetMaximumSteering () const = 0;
      /**
       * Set the maximum steering angle that can be applied by this device, in
       * radian. The default value is 0.4f.
       */
      virtual void SetMaximumSteering (float angle) = 0;

      /// Get the amount of wheels affected by this steering device
      virtual size_t GetAffectedWheelCount () const = 0;
      /**
       * Get the wheel affected by this steering device of the given index. The value
       * returned is the index of the wheel in the iVehicleFactory.
       */
      virtual size_t GetAffectedWheel (size_t index) const = 0;
      /**
       * Add a new wheel to be affected by this steering device. \a index is the index of
       * the wheel in the iVehicleFactory.
       */
      virtual void AddAffectedWheel (size_t index) = 0;
    };

    /**
     * A factory to create instances of iVehicle.
     */
    // TODO: orient the vehicle around the use of an animesh? or use a dedicated animation controller?
    struct iVehicleFactory : public virtual  CS::Physics::iRigidBodyFactory
    {
      SCF_INTERFACE (CS::Physics::iVehicleFactory, 1, 0, 0);

      /**
       * Create a new vehicle
       * \todo Remove the sector parameter and allow vehicles to switch sectors
       */
      virtual csPtr<iVehicle> CreateVehicle (CS::Physics::iPhysicalSector* sector) = 0;

      /// Add a new wheel to this vehicle.
      virtual void AddWheelFactory (iVehicleWheelFactory* factory) = 0;

      /// Get the total number of wheel factories in this vehicle.
      virtual size_t GetWheelFactoryCount () = 0;

      /// Get the wheel factory of the given index.
      virtual iVehicleWheelFactory* GetWheelFactory (size_t index) = 0;

      /// Create a new brake, and add it to the list of brakes for this vehicle.
      virtual iVehicleBrake* CreateBrake () = 0;

      /// Create a new steering device, and add it to the list of devices for this vehicle.
      virtual iVehicleSteeringDevice* CreateSteeringDevice () = 0;
    };
    
    /**
     * A physical, wheeled, grounded vehicle.
     */
    struct iVehicle : public virtual CS::Physics::iRigidBody
    {
      SCF_INTERFACE (CS::Physics::iVehicle, 1, 0, 0);
      
      /// Get the wheel at the given index
      virtual iVehicleWheel* GetWheel (size_t index) const = 0;

      /// The engine force for the next time-step
      virtual float GetEngineForce () const = 0;
      /// The engine force for the next time-step
      virtual void SetEngineForce (float f) = 0;

      /**
       * Apply the given brake for the next simulation step.
       * \param index The index of the brake in the iVehicleFactory.
       * \param scale A scale to be applied on the braking force. The value is
       * supposed to be between 0 and 1.
       */
      virtual void Brake (size_t index, float scale = 1.0f) = 0;

      /**
       * Use the given steering device to increase the steering angle of the
       * device-controlled wheels.
       * \param index The index of the steering device in the iVehicleFactory.
       * \param force The steering increment to be applied on the steering angle.
       * The total amount of steering will be clamped by the maximum angle of the
       * device.
       */
      virtual void Steer (size_t index, float force = 0.4f) = 0;

      /// Get the current speed in km/h (actually in kilo units per hour)
      virtual float GetSpeedKMH () const = 0;
    };
  }
}

#endif
