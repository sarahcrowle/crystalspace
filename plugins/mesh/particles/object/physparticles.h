#ifndef __PHYSPARTICLES_H__
#define __PHYSPARTICLES_H__

#include "cssysdef.h"
#include "csutil/scf_implementation.h"
#include "csutil/scf.h"

#include "ivaria/physics.h"
#include "ivaria/collisions.h"

#include "imesh/emit.h"
#include "imesh/particles.h"

#include "builtineffectors.h"

CS_PLUGIN_NAMESPACE_BEGIN (Particles)
{
  /**
  * Behaves like the default ParticleEffectorForce but does not penetrate physical boundaries
  */ 
  class ParticleEffectorPhysical : public scfImplementation3<
    ParticleEffectorPhysical,
    iParticleBuiltinEffectorPhysical,
    scfFakeInterface<iParticleEffector>,
    //scfFakeInterface<ParticleEffectorForce>, 
    CS::Utility::iModifiable>
  {
  private:
    csVector3 acceleration;
    csVector3 force;
    csVector3 randomAcceleration;
    bool do_randomAcceleration;
    csVector2 restitution;

  public:
    ParticleEffectorPhysical ();

    //-- iParticleEffector
    virtual csPtr<iParticleEffector> Clone () const;

    virtual void EffectParticles (iParticleSystemBase* system,
      const csParticleBuffer& particleBuffer, float dt, float totalTime);

    //-- iParticleBuiltinEffectorForce
    virtual void SetAcceleration (const csVector3& acceleration)
    {
      this->acceleration = acceleration;
    }

    virtual const csVector3& GetAcceleration () const
    {
      return acceleration;
    }

    virtual void SetForce (const csVector3& force)
    {
      this->force = force;
    }

    virtual const csVector3& GetForce () const
    {
      return force; 
    }

    virtual void SetRandomAcceleration (const csVector3& magnitude)
    {
      randomAcceleration = magnitude;
      if (randomAcceleration < .000001f)
        do_randomAcceleration = false;
      else
        do_randomAcceleration = true;
    }

    virtual const csVector3& GetRandomAcceleration () const
    {
      return randomAcceleration;
    }

    virtual void SetRestitution (const csVector2& res) { restitution = res; }
    virtual csVector2 GetRestitution () const { return restitution; }

    virtual void SetRestitutionMagnitude (float rest) { restitution = csVector2 (rest / sqrt (2.f)); }
    virtual float GetRestitutionMagnitude () const { return sqrt (restitution * restitution); }    // what is the method to get a vector's magnitude?

    //-- CS::Utility::iModifiable
    MODIF_DECLARE (0);

    MODIF_GETDESCRIPTION_BEGIN ("EFFECT.PHYSICAL", "Physical effector");
    MODIF_GETDESCRIPTION_END ();

    MODIF_GETPARAMETERVALUE_BEGIN ();
    MODIF_GETPARAMETERVALUE_END ();

    MODIF_SETPARAMETERVALUE_BEGIN ();
    MODIF_SETPARAMETERVALUE_END ();
  };

}
CS_PLUGIN_NAMESPACE_END (Particles)

#endif

