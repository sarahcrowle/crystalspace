/**
* Take care of particles that interact with the physical world
*/

#include "physparticles.h"
#include "particles.h"

#include "iengine/sector.h"
#include "iengine/movable.h"
#include "ivaria/collisions.h"

#include "csutil/floatrand.h"

using namespace CS::Collisions;

CS_PLUGIN_NAMESPACE_BEGIN (Particles)
{
  CS_IMPLEMENT_STATIC_VAR (GetVGen, csRandomVectorGen, ());

  ParticleEffectorPhysical::ParticleEffectorPhysical ()
    : scfImplementationType (this), acceleration (0.0f), force (0.0f),
    randomAcceleration (0.0f), do_randomAcceleration (false)
  {
    SetRestitutionMagnitude (0.5);
  }

  csPtr<iParticleEffector> ParticleEffectorPhysical::Clone () const
  {
    // TODO
    return csPtr<iParticleEffector> (nullptr);
  }

  void ParticleEffectorPhysical::EffectParticles (iParticleSystemBase* system,
    const csParticleBuffer& particleBuffer, float dt, float totalTime)
  {
    ParticlesMeshObject* meshObject = dynamic_cast<ParticlesMeshObject*> (system);
    iMeshWrapper* wrapper = meshObject->GetMeshWrapper ();
    iMovable* movable = wrapper->GetMovable ();
    csReversibleTransform trans = movable->GetFullTransform ();
    iSector* sector = movable->GetSectors ()->GetCount () ?
      movable->GetSectors ()->Get (0) : nullptr;

    // Find the collision system from the object registry
    csRef<CS::Collisions::iCollisionSystem> collisionSystem =
      csQueryRegistry<CS::Collisions::iCollisionSystem>
      (meshObject->factory->objectType->object_reg);

    // If the simulation speed is null then don't do anything
    if (collisionSystem->GetSimulationSpeed () < SMALL_EPSILON)
      return;
    dt *= collisionSystem->GetSimulationSpeed ();

    // Find the collision sector associated to the sector of the mesh
    csRef<CS::Collisions::iCollisionSector> collisionSector;
    if (collisionSystem && sector)
      collisionSector = collisionSystem->FindCollisionSector (sector);

    for (size_t idx = 0; idx < particleBuffer.particleCount; ++idx)
    {
      csParticle& particle = particleBuffer.particleData[idx];
      //csParticleAux& particleAux = particleBuffer.particleAuxData[idx];

      csVector3 a = acceleration;
      if (collisionSector) a += collisionSector->GetGravity ();

      if (do_randomAcceleration)
      {
        csVector3 r = GetVGen ()->Get ();
        a.x += r.x * randomAcceleration.x;
        a.y += r.y * randomAcceleration.y;
        a.z += r.z * randomAcceleration.z;
      }

      // Check for a collision
      csVector3 newVel = particle.linearVelocity + (a + force / particle.mass) * dt;
      csVector3 newPos = particle.position + newVel * dt;

      csVector3 currentPos = trans.This2Other (particle.position);
      newPos = trans.This2Other (newPos);

      if (collisionSector)
      {
	HitBeamResult hitResult = collisionSector->HitBeam (currentPos, newPos);
	if (hitResult.hasHit)
	{
	  // Resolve the collision
	  particle.position = trans.Other2This (hitResult.isect);
	  hitResult.normal.Normalize ();

	  float normalSpeed = hitResult.normal * newVel;
	  csVector3 normalVel = normalSpeed * hitResult.normal;
	  csVector3 tangentialVel = newVel - normalVel;

	  newVel = -restitution.x * normalVel + restitution.y * tangentialVel;
	}
      }

      particle.linearVelocity = newVel;
    }
  }
}
CS_PLUGIN_NAMESPACE_END (Particles)
