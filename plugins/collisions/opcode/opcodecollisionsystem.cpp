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

#include "cssysdef.h"
#include "csqsqrt.h"
#include "ivaria/softanim.h"
#include "imesh/animesh.h"
#include "iengine/scenenode.h"
#include "iengine/mesh.h"
#include "iengine/light.h"
#include "csgeom/transfrm.h"
#include "csutil/scfstr.h"
#include "iutil/string.h"
#include "ivaria/reporter.h"
#include "csutil/scfarray.h"
#include "opcodecollisionsystem.h"
#include "collisionobject2.h"

CS_PLUGIN_NAMESPACE_BEGIN (Opcode2)
{
using namespace Opcode;

static const CS::Collisions::CollisionGroupMask CollisionsAllFilter = -1;

void Opcode_Log (const char* msg, ...)
{
  va_list args;
  va_start (args, msg);
  csOpcodeCollisionSystem::OpcodeReportV (CS_REPORTER_SEVERITY_NOTIFY, 
    msg, args);
  va_end (args);
}

bool Opcode_Err (const char* msg, ...)
{
  va_list args;
  va_start (args, msg);
  // Although it's called "..._Err", Opcode also reports less-than-fatal
  // messages through it
  csOpcodeCollisionSystem::OpcodeReportV (CS_REPORTER_SEVERITY_WARNING, 
    msg, args);
  va_end (args);
  return false;
}

SCF_IMPLEMENT_FACTORY (csOpcodeCollisionSystem)

iObjectRegistry* csOpcodeCollisionSystem::rep_object_reg = NULL;

csOpcodeCollisionSystem::csOpcodeCollisionSystem (iBase* iParent)
: scfImplementationType (this, iParent)
{

  CS::Collisions::CollisionGroup defaultGroup ("Default");
  defaultGroup.value = 1;
  defaultGroup.mask = CollisionsAllFilter;
  collGroups.Push (defaultGroup);

  CS::Collisions::CollisionGroup staticGroup ("Static");
  staticGroup.value = 2;
  staticGroup.mask = CollisionsAllFilter ^ 2;
  collGroups.Push (staticGroup);
  systemFilterCount = 2;
}

csOpcodeCollisionSystem::~csOpcodeCollisionSystem ()
{
  rep_object_reg = 0;
  collSectors.DeleteAll ();
}

bool csOpcodeCollisionSystem::Initialize (iObjectRegistry* object_reg)
{
  this->object_reg = object_reg;
  rep_object_reg = object_reg;
  csRef<iStringSet> strings = csQueryRegistryTagInterface<iStringSet> (
    object_reg, "crystalspace.shared.stringset");
  baseID = strings->Request ("base");
  colldetID = strings->Request ("colldet");
  return true;
}

void csOpcodeCollisionSystem::SetInternalScale (float scale)
{
  //use internal scale?
}

csPtr<CS::Collisions::iColliderConvexMesh> csOpcodeCollisionSystem::CreateColliderConvexMesh (iMeshWrapper* mesh, bool simplify /* = false */)
{
  return NULL;
}

csPtr<CS::Collisions::iColliderConcaveMesh> csOpcodeCollisionSystem::CreateColliderConcaveMesh (iMeshWrapper* mesh)
{
  return csPtr<CS::Collisions::iColliderConcaveMesh>(new csOpcodeCollider (mesh,this));
}

csPtr<CS::Collisions::iColliderConcaveMeshScaled> csOpcodeCollisionSystem::CreateColliderConcaveMeshScaled
(CS::Collisions::iColliderConcaveMesh* collider, csVector3 scale)
{
  return NULL;
}

csPtr<CS::Collisions::iColliderCylinder> csOpcodeCollisionSystem::CreateColliderCylinder (float length, float radius)
{
  return NULL;
}

csPtr<CS::Collisions::iColliderBox> csOpcodeCollisionSystem::CreateColliderBox (const csVector3& size)
{
  return NULL;
}

csPtr<CS::Collisions::iColliderSphere> csOpcodeCollisionSystem::CreateColliderSphere (float radius)
{
  return NULL;
}

csPtr<CS::Collisions::iColliderCapsule> csOpcodeCollisionSystem::CreateColliderCapsule (float length, float radius)
{
  return NULL;
}

csPtr<CS::Collisions::iColliderCone> csOpcodeCollisionSystem::CreateColliderCone (float length, float radius)
{
  return NULL;
}

csPtr<CS::Collisions::iColliderPlane> csOpcodeCollisionSystem::CreateColliderPlane (const csPlane3& plane)
{
  return NULL;
}

csPtr<CS::Collisions::iCollisionTerrain> csOpcodeCollisionSystem::CreateCollisionTerrain (iTerrainSystem* terrain,
                                                               float minHeight, float maxHeight)
{
  csRef<CS::Collisions::iCollisionTerrain> collider;
  collider.AttachNew (new csOpcodeCollisionTerrain (terrain, this));

  return csPtr<CS::Collisions::iCollisionTerrain>(collider);
}

csPtr<CS::Collisions::iCollisionObject> csOpcodeCollisionSystem::CreateCollisionObject (CS::Collisions::CollisionObjectProperties* props)
{
  csRef<csOpcodeCollisionObject> collObject;
  collObject.AttachNew (new csOpcodeCollisionObject (this));

  collObject->CreateCollisionObject(props);

  return csPtr<CS::Collisions::iCollisionObject>(collObject);
}

csPtr<CS::Collisions::iGhostCollisionObject> csOpcodeCollisionSystem::CreateGhostCollisionObject(CS::Collisions::GhostCollisionObjectProperties* props)
{
  csRef<csOpcodeCollisionObject> collObject;
  collObject.AttachNew (new csOpcodeCollisionObject (this));

  return csPtr<CS::Collisions::iGhostCollisionObject>(collObject);
}

csPtr<CS::Collisions::iCollisionActor> csOpcodeCollisionSystem::CreateCollisionActor (CS::Collisions::CollisionActorProperties* props)
{
  //TODO
  return NULL;
}

csPtr<CS::Collisions::iCollisionSector> csOpcodeCollisionSystem::CreateCollisionSector ()
{
  csRef<csOpcodeCollisionSector> collSector;
  collSector.AttachNew (new csOpcodeCollisionSector (this));

  collSectors.Push (collSector);
  return csPtr<CS::Collisions::iCollisionSector>(collSector);
}

CS::Collisions::iCollisionSector* csOpcodeCollisionSystem::FindCollisionSector (const char* name)
{
  return this->collSectors.FindByName (name);
}

CS::Collisions::iCollisionSector* csOpcodeCollisionSystem::GetCollisionSector (const iSector* sceneSector)
{
  for (size_t i = 0; i < collSectors.GetSize (); i++)
  {
    if (collSectors[i]->GetSector () == sceneSector)
    {
      return collSectors[i];
    }
  }
  return nullptr;
}

void csOpcodeCollisionSystem::DecomposeConcaveMesh (CS::Collisions::iCollider* object,
                                   iMeshWrapper* mesh, bool simplify)
{
}

void csOpcodeCollisionSystem::OpcodeReportV (int severity, const char* message, 
                           va_list args)
{
  csReportV (rep_object_reg,
    severity, "crystalspace.collisiondetection.opcode", message, args);
}

CS::Collisions::CollisionGroup& csOpcodeCollisionSystem::CreateCollisionGroup (const char* name)
{
  size_t groupCount = collGroups.GetSize ();
  if (groupCount >= sizeof (CS::Collisions::CollisionGroupMask) * 8)
    return collGroups[0];

  CS::Collisions::CollisionGroup newGroup(name);
  newGroup.value = 1 << groupCount;
  newGroup.mask = CollisionsAllFilter ^ newGroup.value;
  collGroups.Push (newGroup);
  return collGroups[groupCount];
}

CS::Collisions::CollisionGroup& csOpcodeCollisionSystem::FindCollisionGroup (const char* name)
{
  size_t index = collGroups.FindKey (CollisionGroupVector::KeyCmp (name));
  if (index == csArrayItemNotFound)
    return collGroups[0];
  else
    return collGroups[index];
}

void csOpcodeCollisionSystem::SetGroupCollision (const char* name1, const char* name2, bool collide)
{
  size_t index1 = collGroups.FindKey (CollisionGroupVector::KeyCmp (name1));
  size_t index2 = collGroups.FindKey (CollisionGroupVector::KeyCmp (name2));
  if (index1 == csArrayItemNotFound || index2 == csArrayItemNotFound)
    return;
  if (!collide)
  {
    if (index1 >= systemFilterCount)
      collGroups[index1].value &= ~(1 << index2);
    if (index2 >= systemFilterCount)
      collGroups[index2].value &= ~(1 << index1);
  }
  else
  {
    if (index1 >= systemFilterCount)
      collGroups[index1].value |= 1 << index2;
    if (index2 >= systemFilterCount)
      collGroups[index2].value |= 1 << index1;
  }
}

bool csOpcodeCollisionSystem::GetGroupCollision (const char* name1, const char* name2)
{
  size_t index1 = collGroups.FindKey (CollisionGroupVector::KeyCmp (name1));
  size_t index2 = collGroups.FindKey (CollisionGroupVector::KeyCmp (name2));
  if (index1 == csArrayItemNotFound || index2 == csArrayItemNotFound)
    return false;
  if ((collGroups[index1].value & (1 << index2)) != 0 
    || (collGroups[index2].value & (1 << index1)) != 0)
    return true;
  else
    return false;
}
}
CS_PLUGIN_NAMESPACE_END (Opcode2)
