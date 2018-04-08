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
#include "iengine/movable.h"
#include "collisionobject2.h"

using namespace CS::Collisions;

CS_PLUGIN_NAMESPACE_BEGIN (Opcode2)
{
void csOpcodeCollisionObject::CreateCollisionObject(CS::Collisions::CollisionObjectProperties* props)
{
  SetCollider(props->GetCollider());
  SetName(props->GetName());

  if (props->GetCollisionGroup().name.Length())
  {
    SetCollisionGroup(props->GetCollisionGroup());
  }
  else
  {
    SetCollisionGroup(system->FindCollisionGroup("Default"));
  }
}

csOpcodeCollisionObject::csOpcodeCollisionObject (csOpcodeCollisionSystem* sys)
  : scfImplementationType (this), system (sys), sector (NULL),
  collider (NULL),
  collCb (NULL)
{
  transform.Identity ();
}

csOpcodeCollisionObject::~csOpcodeCollisionObject ()
{

}

void csOpcodeCollisionObject::SetTransform (const csOrthoTransform& trans)
{
  transform = trans;
  if (movable)
    movable->SetFullTransform (transform);
  if (camera)
    camera->SetTransform (transform);
}

csOrthoTransform csOpcodeCollisionObject::GetTransform () const
{
  if (movable)
    return movable->GetFullTransform ();
  if (camera)
    return camera->GetTransform ();
  return transform;
}

void csOpcodeCollisionObject::SetRotation (const csMatrix3& rot)
{
  csOrthoTransform trans = GetTransform ();
  trans.SetT2O (rot);
  SetTransform (trans);
}

void csOpcodeCollisionObject::Rotate (const csVector3& v, float angle)
{
  csOrthoTransform trans = GetTransform ();
  trans.RotateThis (v, angle);
  SetTransform (trans);
}

void csOpcodeCollisionObject::IncreasePitch(float pitchDelta)
{
  Rotate(CS_VEC_TILT_UP, pitchDelta);
}

void csOpcodeCollisionObject::IncreaseYaw(float yawDelta)
{
  Rotate (CS_VEC_ROT_RIGHT, yawDelta);
}

void csOpcodeCollisionObject::RebuildObject ()
{
  // does nothing
}

void csOpcodeCollisionObject::SetCollisionGroup (const char* name)
{
  this->collGroup = system->FindCollisionGroup(name);
}

void csOpcodeCollisionObject::SetCollisionGroup (const CS::Collisions::CollisionGroup& group)
{
  this->collGroup = group;
}

bool csOpcodeCollisionObject::Collide (CS::Collisions::iCollisionObject* otherObject)
{
  csOpcodeCollisionObject* obj = dynamic_cast<csOpcodeCollisionObject*> (otherObject);
  bool collides = (obj->collGroup.value & collGroup.mask) != 0;
  collides = collides && (collGroup.value & obj->collGroup.mask);

  if(!collides)
    return false;

  csArray<CS::Collisions::CollisionData> collisions;

  bool contact = sector->CollideObject (this, obj, collisions);
  if (contact)
  {
    if (collCb)
    {
      collCb->OnCollision (this, obj, collisions);
    }
  }
  return contact;
}

CS::Collisions::HitBeamResult csOpcodeCollisionObject::HitBeam (const csVector3& start, const csVector3& end)
{
  //CS::Collisions::iCollider* col = collider;
  float dep;
  CS::Collisions::HitBeamResult result = sector->HitBeamObject (this, start, end, dep);
  return result;
}

//There's no step function now. Maybe use ContactTest is better.
size_t csOpcodeCollisionObject::GetContactObjectsCount (){return 0;}

CS::Collisions::iCollisionObject* csOpcodeCollisionObject::GetContactObject (size_t index)
{
  return NULL;
}
}
CS_PLUGIN_NAMESPACE_END (Opcode2)
