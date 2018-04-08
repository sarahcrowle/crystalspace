/*
  Copyright (C) 2006 by Marten Svanfeldt

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

#ifndef __CS_MESH_PARTICLES_H__
#define __CS_MESH_PARTICLES_H__

#include "cstool/modifiableimpl.h"
#include "cstool/objmodel.h"
#include "cstool/rendermeshholder.h"
#include "csutil/scf_implementation.h"
#include "csutil/flags.h"
#include "csutil/radixsort.h"
#include "csutil/weakref.h"

#include "imesh/object.h"
#include "imesh/particles.h"
#include "iutil/comp.h"
#include "iutil/modifiable.h"
#include "ivideo/rndbuf.h"

CS_PLUGIN_NAMESPACE_BEGIN(Particles)
{
  struct iVertexSetup;

  /**
  * Particle object type
  */
  class ParticlesMeshObjectType : public scfImplementation2<ParticlesMeshObjectType,
                                                            iMeshObjectType,
                                                            iComponent>
  {
  public:
    ParticlesMeshObjectType (iBase* parent);
    virtual ~ParticlesMeshObjectType ();

    /// Initialize
    virtual bool Initialize (iObjectRegistry* object_reg);

    /// Create a new factory
    virtual csPtr<iMeshObjectFactory> NewFactory ();

  public:
    iObjectRegistry* object_reg;
  };

  class ParticleBase : public CS::Utility::iModifiable
  {
  protected:
    //-- iParticleSystemBase
    csParticleRenderOrientation particleOrientation;
    csParticleRotationMode rotationMode;
    csParticleSortMode sortMode;
    csParticleIntegrationMode integrationMode;
    csParticleTransformMode transformMode;
    csVector3 commonDirection;
    bool individualSize;
    csVector2 particleSize;
    csBox3 minBB;

  public:
    ParticleBase ();

    /**\name CS::Utility::iModifiable implementation
      * @{ */
    MODIF_DECLARE (8);
    MODIF_GETDESCRIPTION_BEGIN ("PARTICLE_MESH", "Particle mesh object");

    MODIF_GETDESCRIPTION_CENUM_DECLARE ();
    MODIF_GETDESCRIPTION_CENUM_PUSH (CS_PARTICLE_CAMERAFACE, 
				     "Billboard always facing the camera");
    MODIF_GETDESCRIPTION_CENUM_PUSH (CS_PARTICLE_CAMERAFACE_APPROX,
				     "Billboard always facing the camera direction");
    MODIF_GETDESCRIPTION_CENUM_PUSH (CS_PARTICLE_ORIENT_COMMON,
				     "Orient around a common direction (y/up direction), facing the camera");
    MODIF_GETDESCRIPTION_CENUM_PUSH (CS_PARTICLE_ORIENT_COMMON_APPROX,
				     "Orient around a common direction (y/up direction), facing the camera, using the camera direction");
    MODIF_GETDESCRIPTION_CENUM_PUSH (CS_PARTICLE_ORIENT_VELOCITY,
				     "Use velocity vector as direction");
    MODIF_GETDESCRIPTION_CENUM_PUSH (CS_PARTICLE_ORIENT_SELF,
				     "Orient particles according to their internal rotation");
    MODIF_GETDESCRIPTION_CENUM_PUSH (CS_PARTICLE_ORIENT_SELF_FORWARD,
				     "Orient self forward - towards camera");
    MODIF_GETDESCRIPTION_CENUM (LONG, "ORIENTATION", "Orientation", "Particle render orientation");

    MODIF_GETDESCRIPTION_CENUM_DECLARE ();
    MODIF_GETDESCRIPTION_CENUM_PUSH (CS_PARTICLE_ROTATE_NONE, "Do not take rotation into account at all");
    MODIF_GETDESCRIPTION_CENUM_PUSH (CS_PARTICLE_ROTATE_TEXCOORD, "Rotate the texture coordinates");
    MODIF_GETDESCRIPTION_CENUM_PUSH (CS_PARTICLE_ROTATE_VERTICES,
				     "Rotate the particle vertices in the billboard plane");
    MODIF_GETDESCRIPTION_CENUM (LONG, "ROTATION", "Rotation mode", "Rotation mode applied on each particle");

    MODIF_GETDESCRIPTION_CENUM_DECLARE ();
    MODIF_GETDESCRIPTION_CENUM_PUSH (CS_PARTICLE_SORT_NONE, "No sorting");
    MODIF_GETDESCRIPTION_CENUM_PUSH (CS_PARTICLE_SORT_DISTANCE, "Sort by distance to the camera");
    MODIF_GETDESCRIPTION_CENUM_PUSH (CS_PARTICLE_SORT_DOT,
				     "Sort by the dot product of the normalized camera vector and the particle direction");
    MODIF_GETDESCRIPTION_CENUM (LONG, "SORT", "Sort mode", "Mode used to sort the particles before rendering them");

    MODIF_GETDESCRIPTION_CENUM_DECLARE ();
    MODIF_GETDESCRIPTION_CENUM_PUSH (CS_PARTICLE_INTEGRATE_NONE, "No integration");
    MODIF_GETDESCRIPTION_CENUM_PUSH (CS_PARTICLE_INTEGRATE_LINEAR, "Integrate linear velocity into linear position");
    MODIF_GETDESCRIPTION_CENUM_PUSH (CS_PARTICLE_INTEGRATE_BOTH, "Integrate both linear and angular velocity into pose");
    MODIF_GETDESCRIPTION_CENUM (LONG, "INTEGRATION", "Integration mode",
				"Mode used to integrate the linear and/or angular velocities of the particles");

    MODIF_GETDESCRIPTION_CENUM_DECLARE ();
    MODIF_GETDESCRIPTION_CENUM_PUSH (CS_PARTICLE_LOCAL_MODE,
				     "Fully local mode - all positions and coordinates are relative to the particle system");
    MODIF_GETDESCRIPTION_CENUM_PUSH (CS_PARTICLE_LOCAL_EMITTER,
				     "Particle positions and effectors operate in world space, emitters in local mode");
    MODIF_GETDESCRIPTION_CENUM_PUSH (CS_PARTICLE_WORLD_MODE, "All coordinates are in world space");
    MODIF_GETDESCRIPTION_CENUM (LONG, "TRANSFORMATION", "Transformation mode",
				"Coordinate system to be used for the computation of the transforms of the particles");

    MODIF_GETDESCRIPTION (BOOL, "SIZE_INDIV", "Individual size", "Whether or not individual sizes should be computed for each particles");
    MODIF_GETDESCRIPTION (VECTOR2, "SIZE_PARTICLE", "Particle size", "Size to be used for all particles");
    MODIF_GETDESCRIPTION (VECTOR3, "DIRECTION", "Common direction", "Common motion direction for all particles");

    MODIF_GETDESCRIPTION_END ();

    MODIF_GETPARAMETERVALUE_BEGIN ();
    MODIF_GETPARAMETERVALUE (0, Long, particleOrientation);
    MODIF_GETPARAMETERVALUE (1, Long, rotationMode);
    MODIF_GETPARAMETERVALUE (2, Long, sortMode);
    MODIF_GETPARAMETERVALUE (3, Long, integrationMode);
    MODIF_GETPARAMETERVALUE (4, Long, transformMode);
    MODIF_GETPARAMETERVALUE (5, Bool, individualSize);
    MODIF_GETPARAMETERVALUE (6, Vector2, particleSize);
    MODIF_GETPARAMETERVALUE (7, Vector3, commonDirection);
    MODIF_GETPARAMETERVALUE_END ();

    MODIF_SETPARAMETERVALUE_BEGIN ();
    MODIF_SETPARAMETERVALUE_ENUM (0, Long, particleOrientation, csParticleRenderOrientation);
    MODIF_SETPARAMETERVALUE_ENUM (1, Long, rotationMode, csParticleRotationMode);
    MODIF_SETPARAMETERVALUE_ENUM (2, Long, sortMode, csParticleSortMode);
    MODIF_SETPARAMETERVALUE_ENUM (3, Long, integrationMode, csParticleIntegrationMode);
    MODIF_SETPARAMETERVALUE_ENUM (4, Long, transformMode, csParticleTransformMode);
    MODIF_SETPARAMETERVALUE (5, Bool, individualSize);
    MODIF_SETPARAMETERVALUE (6, Vector2, particleSize);
    MODIF_SETPARAMETERVALUE (7, Vector3, commonDirection);
    MODIF_SETPARAMETERVALUE_END ();

    // TODO: modifiable's listeners should be notified also when the state of the object is
    // changed by some other way than through the iModifiable interface

    /** @} */
  };

  /**
  * Particle object factory
  */
  class ParticlesMeshFactory : public scfImplementation4<ParticlesMeshFactory,
                                                         iMeshObjectFactory,
                                                         iParticleSystemFactory,
                                                         scfFakeInterface<CS::Utility::iModifiable>,
                                                         scfFakeInterface<iParticleSystemBase> >,
                               public ParticleBase
  {
  public:
    /// Constructor
    ParticlesMeshFactory (ParticlesMeshObjectType* objectType);
    virtual ~ParticlesMeshFactory();

    //-- Local methods

    /// Get the object type
    inline ParticlesMeshObjectType* GetObjectType () const
    {
      return objectType;
    }

    /**\name iMeshObjectFactory implementation
     * @{ */
    virtual csFlags& GetFlags ()
    {
      return flags;
    }

    /// Create an instance of iMeshObject.
    virtual csPtr<iMeshObject> NewInstance ();

    virtual csPtr<iMeshObjectFactory> Clone ();

    virtual void HardTransform (const csReversibleTransform& t) // Not supported
    {}

    virtual bool SupportsHardTransform () const
    {
      return false;
    }

    virtual void SetMeshFactoryWrapper (iMeshFactoryWrapper* logparent)
    {
      factoryWrapper = logparent;
    }

    virtual iMeshFactoryWrapper* GetMeshFactoryWrapper () const
    {
      return factoryWrapper;
    }

    virtual iMeshObjectType* GetMeshObjectType () const
    {
      return objectType;
    }

    virtual iObjectModel* GetObjectModel ()
    {
      return 0;
    }

    virtual bool SetMaterialWrapper (iMaterialWrapper* material) 
    {
      materialWrapper = material;
      return true;
    }

    virtual iMaterialWrapper* GetMaterialWrapper () const 
    {
      return materialWrapper;
    }

    virtual void SetMixMode (uint mode)
    {
      mixMode = mode;
    }
   
    virtual uint GetMixMode () const
    {
      return mixMode;
    }
    /** @} */

    /**\name iParticleSystemFactory implementation
     * @{ */
    virtual void SetDeepCreation (bool deep)
    {
      deepCreation = deep;
    }

    virtual bool GetDeepCreation () const
    {
      return deepCreation;
    }
    /** @} */

    /**\name iParticleSystemBase implementation
     * @{ */
    virtual void SetParticleRenderOrientation (csParticleRenderOrientation o)
    {
      particleOrientation = o;
    }

    virtual csParticleRenderOrientation GetParticleRenderOrientation () const
    {
      return particleOrientation;
    }

    virtual void SetRotationMode (csParticleRotationMode mode)
    {
      rotationMode = mode;
    }

    virtual csParticleRotationMode GetRotationMode () const
    {
      return rotationMode;
    }

    virtual void SetSortMode (csParticleSortMode mode)
    {
      sortMode = mode;
    }

    virtual csParticleSortMode GetSortMode () const
    {
      return sortMode;
    }

    virtual void SetIntegrationMode (csParticleIntegrationMode mode)
    {
      integrationMode = mode;
    }

    virtual csParticleIntegrationMode GetIntegrationMode () 
    {
      return integrationMode;
    }

    virtual void SetCommonDirection (const csVector3& direction)
    {
      commonDirection = direction;
    }

    virtual const csVector3& GetCommonDirection () const
    {
      return commonDirection;
    }

    virtual void SetTransformMode (csParticleTransformMode mode)
    {
      transformMode = mode;
    }

    virtual csParticleTransformMode GetTransformMode () const
    {
      return transformMode;
    }

    virtual void SetUseIndividualSize (bool individual)
    {
      individualSize = individual;
    }

    virtual bool GetUseIndividualSize () const
    {
      return individualSize;
    }

    virtual void SetParticleSize (const csVector2& size)
    {
      particleSize = size;
    }

    virtual const csVector2& GetParticleSize () const
    {
      return particleSize;
    }

    virtual void SetMinBoundingBox (const csBox3& box)
    {
      minBB = box;
    }

    virtual const csBox3& GetMinBoundingBox () const
    {
      return minBB;
    }

    virtual void AddEmitter (iParticleEmitter* emitter)
    {
      emitters.PushSmart (emitter);
    }

    virtual iParticleEmitter* GetEmitter (size_t index) const
    {
      return emitters[index];
    }

    virtual void RemoveEmitter (size_t index)
    {
      emitters.DeleteIndex (index);
    }

    virtual size_t GetEmitterCount () const
    {
      return emitters.GetSize ();
    }

    virtual void AddEffector (iParticleEffector* effector)
    {
      effectors.PushSmart (effector);
    }

    virtual iParticleEffector* GetEffector (size_t index) const
    {
      return effectors[index];
    }

    virtual void RemoveEffector (size_t index)
    {
      effectors.DeleteIndex (index);
    }

    virtual size_t GetEffectorCount () const
    {
      return effectors.GetSize ();
    }
    /** @} */

  public:
    csRef<ParticlesMeshObjectType> objectType;

  private:
    //-- Needed only for iMeshObjectFactory
    csFlags flags;
    iMeshFactoryWrapper* factoryWrapper;
    iMaterialWrapper *materialWrapper;
    uint mixMode;

    //-- iParticleSystemFactory properties
    bool deepCreation;

    csRefArray<iParticleEmitter> emitters;
    csRefArray<iParticleEffector> effectors;
  };

  /**
  * Particle mesh object
  */
  class ParticlesMeshObject : public scfImplementationExt4<ParticlesMeshObject,
                                                           csObjectModel,
                                                           iMeshObject,
                                                           iParticleSystem,
                                                           scfFakeInterface<CS::Utility::iModifiable>,
                                                           scfFakeInterface<iParticleSystemBase> >,
                              public ParticleBase
   {
  public:
    /// Constructor
    ParticlesMeshObject (ParticlesMeshFactory* factory);
    virtual ~ParticlesMeshObject (); 

    //-- Local
    /// Reserve some size in the particle buffer
    void ReserveNewParticles (size_t numNew);

    /// Setup index buffer
    void SetupIndexBuffer (csRenderBufferHolder* bufferHolder,
      const csReversibleTransform& o2c);

    /// Setup vertex buffer
    void SetupVertexBuffer (csRenderBufferHolder* bufferHolder,
      const csReversibleTransform& o2c);

    /// Update the TC buffer
    void UpdateTexCoordBuffer ();
    /// Update the color buffer
    void UpdateColorBuffer ();

    /// Invalidate vertex setup
    void InvalidateVertexSetup ();


    /**\name iMeshObject implementation
     * @{ */
    virtual iMeshObjectFactory* GetFactory () const
    {
      return factory;
    }

    virtual csFlags& GetFlags ()
    {
      return flags;
    }

    virtual csPtr<iMeshObject> Clone ();

    virtual csRenderMesh** GetRenderMeshes (int& num, iRenderView* rview, 
      iMovable* movable, uint32 frustum_mask);

    virtual void SetVisibleCallback (iMeshObjectDrawCallback* cb)
    {
      visCallback = cb;
    }

    virtual iMeshObjectDrawCallback* GetVisibleCallback () const
    {
      return visCallback;
    }

    virtual void NextFrame (csTicks current_time,const csVector3& pos, 
      uint currentFrame);

    virtual void HardTransform (const csReversibleTransform& t) //Not supported
    {}

    virtual bool SupportsHardTransform () const
    {
      return false;
    }

    virtual bool HitBeamOutline (const csVector3& start,
      const csVector3& end, csVector3& isect, float* pr)
    {
      return false;
    }

    virtual bool HitBeamObject (const csVector3& start, const csVector3& end,
      csVector3& isect, float* pr, int* polygon_idx = 0,
      iMaterialWrapper** material = 0, bool bf = false) 
    {
      return false;
    }

    virtual void SetMeshWrapper (iMeshWrapper* logparent)
    {
      meshWrapper = logparent;

      // Check for delayed advance.
      if(delayedAdvance > 0)
      {
        Advance(delayedAdvance);
        delayedAdvance = 0;
      }
    }

    virtual iMeshWrapper* GetMeshWrapper () const 
    {
      return meshWrapper;
    }

    virtual iObjectModel* GetObjectModel () 
    {
      return this;
    }

    virtual bool SetColor (const csColor& color)
    {
      return false;
    }

    virtual bool GetColor (csColor& color) const
    {
      return false;
    }

    virtual iTerrainSystem* GetTerrainColldet () { return 0; }

    virtual bool SetMaterialWrapper (iMaterialWrapper* material)
    {
      materialWrapper = material;
      return true;
    }

    virtual iMaterialWrapper* GetMaterialWrapper () const
    {
      return materialWrapper;
    }

    virtual void SetMixMode (uint mode)
    {
      mixMode = mode;
    }

    virtual uint GetMixMode () const
    {
      return mixMode;
    }

    virtual void PositionChild (iMeshObject* child,csTicks current_time)
    {}
    virtual void BuildDecal(const csVector3* pos, float decalRadius,
            iDecalBuilder* decalBuilder)
    {
    }
    /** @} */

    /**\name iObjectModel implementation
     * @{ */
    virtual const csBox3& GetObjectBoundingBox ();

    virtual void SetObjectBoundingBox (const csBox3& bbox);

    virtual void GetRadius (float& radius, csVector3& center);
    /** @} */


    /**\name iParticleSystem implementation
     * @{ */
    virtual size_t GetParticleCount () const
    {
      return particleBuffer.particleCount;
    }

    virtual csParticle* GetParticle (size_t index)
    {
      return particleBuffer.particleData+index;
    }

    virtual csParticleAux* GetParticleAux (size_t index)
    {
      return particleBuffer.particleAuxData+index;
    }

    virtual csParticleBuffer* LockForExternalControl (size_t maxParticles);
    
    virtual void Advance (csTicks time);
    /** @} */

    /**\name iParticleSystemBase implementation
     * @{ */
    virtual void SetParticleRenderOrientation (csParticleRenderOrientation o)
    {
      particleOrientation = o;
      InvalidateVertexSetup ();
    }

    virtual csParticleRenderOrientation GetParticleRenderOrientation () const
    {
      return particleOrientation;
    }

    virtual void SetRotationMode (csParticleRotationMode mode)
    {
      rotationMode = mode;
      InvalidateVertexSetup ();
      // Make sure TC buffer is regenerated
      tcBuffer = 0;
    }

    virtual csParticleRotationMode GetRotationMode () const
    {
      return rotationMode;
    }

    virtual void SetSortMode (csParticleSortMode mode)
    {
      sortMode = mode;
      InvalidateVertexSetup ();
    }

    virtual csParticleSortMode GetSortMode () const
    {
      return sortMode;
    }

    virtual void SetIntegrationMode (csParticleIntegrationMode mode)
    {
      integrationMode = mode;
    }

    virtual csParticleIntegrationMode GetIntegrationMode () 
    {
      return integrationMode;
    }

    virtual void SetCommonDirection (const csVector3& direction)
    {
      commonDirection = direction;
      InvalidateVertexSetup ();
    }

    virtual const csVector3& GetCommonDirection () const
    {
      return commonDirection;
    }

    virtual void SetTransformMode (csParticleTransformMode mode)
    {
      transformMode = mode;
    }

    virtual csParticleTransformMode GetTransformMode () const
    {
      return transformMode;
    }

    virtual void SetUseIndividualSize (bool individual)
    {
      individualSize = individual;
      InvalidateVertexSetup ();
    }

    virtual bool GetUseIndividualSize () const
    {
      return individualSize;
    }

    virtual void SetParticleSize (const csVector2& size)
    {
      particleSize = size;
      InvalidateVertexSetup ();
    }

    virtual const csVector2& GetParticleSize () const
    {
      return particleSize;
    }

    /// Set the smallest bounding box particle system should use
    virtual void SetMinBoundingBox (const csBox3& box) 
    {
      minBB = box;
    }

    /// Get the smallest bounding box particle system should use
    virtual const csBox3& GetMinBoundingBox () const
    {
      return minBB;
    }

    virtual void AddEmitter (iParticleEmitter* emitter)
    {
      emitters.PushSmart (emitter);
    }

    virtual iParticleEmitter* GetEmitter (size_t index) const
    {
      return emitters[index];
    }

    virtual void RemoveEmitter (size_t index)
    {
      emitters.DeleteIndex (index);
    }

    virtual size_t GetEmitterCount () const
    {
      return emitters.GetSize ();
    }

    virtual void AddEffector (iParticleEffector* effector)
    {
      effectors.PushSmart (effector);
    }

    virtual iParticleEffector* GetEffector (size_t index) const
    {
      return effectors[index];
    }

    virtual void RemoveEffector (size_t index)
    {
      effectors.DeleteIndex (index);
    }

    virtual size_t GetEffectorCount () const
    {
      return effectors.GetSize ();
    }
    /** @} */
    
  private:
    friend class ParticlesMeshFactory;

  public:
    ParticlesMeshFactory* factory;

  private:
    iVertexSetup* vertexSetup; //Helper object

    // For delayed advance.
    csTicks delayedAdvance;
    
    /**
     * Advance particle system by given amount of seconds.
     * \warning Does not do capping of the duration; too large values can
     *  cause undesired effects like "particle system explosion".
     */
    void Advance (float dt, float& newRadiusSq);

    //-- iMeshObject
    iMeshWrapper* meshWrapper;
    csFlags flags;
    csRef<iMeshObjectDrawCallback> visCallback;
    csRef<iMaterialWrapper> materialWrapper;
    uint mixMode;

    csTicks lastUpdateTime;
    uint lastFrameNumber;
    float totalParticleTime;
    
    csRenderMeshHolder rmHolder;
    csRef<iRenderBuffer> unsortedIndexBuffer;

    //-- iObjectModel
    float radius, minRadius;
    csBox3 objectBB;

    //-- iParticleSystem
    csParticleBuffer particleBuffer;
    uint8* rawBuffer;
    size_t particleAllocatedSize;
    bool externalControl;

    csRefArray<iParticleEmitter> emitters;
    csRefArray<iParticleEffector> effectors;

    csRadixSorter indexSorter;

    //-- iRenderBufferAccessor
    csRef<iRenderBuffer> tcBuffer;
    csRef<iRenderBuffer> colorBuffer;

    //-- iRenderBufferAccessor
    void PreGetBuffer (csRenderBufferHolder* holder, 
      csRenderBufferName buffer);


    class RenderBufferAccessor : 
      public scfImplementation1<RenderBufferAccessor, iRenderBufferAccessor>
    {
    public:
      CS_LEAKGUARD_DECLARE (RenderBufferAccessor);
      csWeakRef<ParticlesMeshObject> parent;
      virtual ~RenderBufferAccessor () { }
      RenderBufferAccessor (ParticlesMeshObject* parent)
        : scfImplementationType (this)
      {
        this->parent = parent;
      }
      virtual void PreGetBuffer (csRenderBufferHolder* holder,
        csRenderBufferName buffer)
      {
        if (parent) parent->PreGetBuffer (holder, buffer);
      }
    };
    csRef<RenderBufferAccessor> renderBufferAccessor;

  };
}
CS_PLUGIN_NAMESPACE_END(Particles)

#endif
