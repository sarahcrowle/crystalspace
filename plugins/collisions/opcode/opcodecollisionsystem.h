/*
Copyright (C) 2011 by Liu Lu

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free
Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __CS_OPCODE_COLLISIONSYSTEM_H__
#define __CS_OPCODE_COLLISIONSYSTEM_H__

#include "iutil/comp.h"
#include "csutil/scf.h"
#include "csutil/nobjvec.h"
#include "csutil/scf_implementation.h"
#include "ivaria/collisions.h"
#include "iengine/sector.h"
#include "iengine/movable.h"
#include "csutil/csobject.h"
#include "Opcode.h"
#include "opcodecollisionsector.h"


CS_PLUGIN_NAMESPACE_BEGIN (Opcode2)
{
  class csOpcodeCollisionSystem;
  class csOpcodeCollisionObject;


  class CollisionGroupVector : public csArray<CS::Collisions::CollisionGroup>
  {
  public:
    CollisionGroupVector () : csArray<CS::Collisions::CollisionGroup> () {}
    static int CompareKey (CS::Collisions::CollisionGroup const& item,
      char const* const& key)
    {
      return strcmp (item.name.GetData (), key);
    }
    static csArrayCmp<CS::Collisions::CollisionGroup, char const*>
      KeyCmp(char const* k)
    {
      return csArrayCmp<CS::Collisions::CollisionGroup, char const*> (k,CompareKey);
    }
  };

  class csOpcodeCollisionSystem : public scfImplementation2<
    csOpcodeCollisionSystem, CS::Collisions::iCollisionSystem,
    iComponent>
  {
    friend class csOpcodeCollider;
  private:
    iObjectRegistry* object_reg;
    csRefArrayObject<csOpcodeCollisionSector> collSectors;
    csStringID baseID;
    csStringID colldetID;

    CollisionGroupVector collGroups;
    size_t systemFilterCount;
  public:
    csOpcodeCollisionSystem (iBase* iParent);
    virtual ~csOpcodeCollisionSystem ();

    // iComponent
    virtual bool Initialize (iObjectRegistry* object_reg);

    // iCollisionSystem
    virtual void SetInternalScale (float scale);
    virtual csPtr<CS::Collisions::iColliderCompound> CreateColliderCompound()
    { return csPtr<CS::Collisions::iColliderCompound> (nullptr); }
    virtual csPtr<CS::Collisions::iColliderConvexMesh> CreateColliderConvexMesh (
      iMeshWrapper* mesh, bool simplify = false);
    virtual csPtr<CS::Collisions::iColliderConcaveMesh> CreateColliderConcaveMesh (iMeshWrapper* mesh);
    virtual csPtr<CS::Collisions::iColliderConcaveMeshScaled> CreateColliderConcaveMeshScaled
      (CS::Collisions::iColliderConcaveMesh* collider, csVector3 scale);
    virtual csPtr<CS::Collisions::iColliderCylinder> CreateColliderCylinder (float length, float radius);
    virtual csPtr<CS::Collisions::iColliderBox> CreateColliderBox (const csVector3& size);
    virtual csPtr<CS::Collisions::iColliderSphere> CreateColliderSphere (float radius);
    virtual csPtr<CS::Collisions::iColliderCapsule> CreateColliderCapsule (float length, float radius);
    virtual csPtr<CS::Collisions::iColliderCone> CreateColliderCone (float length, float radius);
    virtual csPtr<CS::Collisions::iColliderPlane> CreateColliderPlane (const csPlane3& plane);
    virtual csPtr<CS::Collisions::iCollisionTerrain> CreateCollisionTerrain (iTerrainSystem* terrain,
      float minHeight = 0, float maxHeight = 0);

    virtual csPtr<CS::Collisions::iCollisionObject> CreateCollisionObject (CS::Collisions::CollisionObjectProperties* props);
    virtual csPtr<CS::Collisions::iGhostCollisionObject> CreateGhostCollisionObject (CS::Collisions::GhostCollisionObjectProperties* props);
    virtual csPtr<CS::Collisions::iCollisionActor> CreateCollisionActor (CS::Collisions::CollisionActorProperties* props);
    virtual csPtr<CS::Collisions::iCollisionSector> CreateCollisionSector ();
    virtual CS::Collisions::iCollisionSector* FindCollisionSector (const char* name);
    virtual CS::Collisions::iCollisionSector* GetCollisionSector (const iSector* sceneSector);

    virtual void DecomposeConcaveMesh (CS::Collisions::iCollider* object,
      iMeshWrapper* mesh, bool simplify = false); 

    static void OpcodeReportV (int severity, const char* message, 
      va_list args);

    virtual CS::Collisions::CollisionGroup& CreateCollisionGroup (const char* name);
    virtual CS::Collisions::CollisionGroup& FindCollisionGroup (const char* name);

    virtual void SetGroupCollision (const char* name1, const char* name2, bool collide);
    virtual bool GetGroupCollision (const char* name1, const char* name2);

  public:
    static iObjectRegistry* rep_object_reg;
  };
}
CS_PLUGIN_NAMESPACE_END (Opcode2)
#endif
