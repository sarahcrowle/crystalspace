  /*
    Copyright (C) 2013 by Jorrit Tyberghein

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
#include "csqint.h"

#include "cstool/basemovable.h"
#include "iengine/light.h"
#include "iengine/mesh.h"
#include "iengine/camera.h"

namespace CS
{
namespace Engine
{

//---------------------------------------------------------------------------
MovableSectorList::MovableSectorList ()
  : scfImplementationType (this)
{
  movable = 0;
}

MovableSectorList::~MovableSectorList ()
{
  sectors.DeleteAll ();
}

bool MovableSectorList::PrepareSector (iSector* sector)
{
  // Check for a valid item.
  if (sector == 0) return false;

  bool rc = sector->PrepareMovable (movable);

  // Make sure camera and light only is in one sector
  CS_ASSERT (!(movable->GetLight () && sectors.GetSize () > 0));
  CS_ASSERT (!(movable->GetCamera () && sectors.GetSize () > 0));

  return rc;
}

int MovableSectorList::Add (iSector *obj)
{
  if (!PrepareSector (obj)) return -1;
  return (int)sectors.Push (obj);
}

bool MovableSectorList::Remove (iSector *obj)
{
  iMeshWrapper* object = movable->GetMeshWrapper ();
  if (object) object->RemoveFromSectors (obj);
  return sectors.Delete (obj);
}

bool MovableSectorList::Remove (int n)
{
  iSector* obj = Get (n);
  iMeshWrapper* object = movable->GetMeshWrapper ();
  if (object) object->RemoveFromSectors (obj);
  return sectors.DeleteIndex (n);
}

void MovableSectorList::RemoveAll ()
{
  movable->ClearSectors ();
}

int MovableSectorList::Find (iSector *obj) const
{
  return (int)sectors.Find (obj);
}

iSector *MovableSectorList::FindByName (const char *Name) const
{
  return sectors.FindByName (Name);
}

//---------------------------------------------------------------------------

BaseMovable::BaseMovable ()
  : scfImplementationType (this), is_identity (true), parent (0),
    meshobject (0), lightobject (0), cameraobject (0), updatenr (0)
{
  sectors.SetMovable (this);
}

BaseMovable::~BaseMovable ()
{
  size_t i = listeners.GetSize ();
  while (i > 0)
  {
    i--;
    iMovableListener *ml = listeners[i];
    ml->MovableDestroyed (this);
  }
}

void BaseMovable::SetPosition (iSector *home, const csVector3 &pos)
{
  obj.SetOrigin (pos);
  SetSector (home);
}

void BaseMovable::SetTransform (const csMatrix3 &matrix)
{
  obj.SetT2O (matrix);
}

void BaseMovable::SetFullTransform (const csReversibleTransform& t)
{
  if (parent == (BaseMovable*)nullptr)
    obj = t;
  else
    obj = t * parent->GetFullTransform ().GetInverse ();
}

void BaseMovable::SetFullPosition (const csVector3& v)
{
  if (parent == (BaseMovable*)nullptr)
    obj.SetOrigin (v);
  else
    obj.SetOrigin (parent->GetFullTransform ().Other2This (v));
}

void BaseMovable::MovePosition (const csVector3 &rel)
{
  obj.Translate (rel);
}

void BaseMovable::Transform (const csMatrix3 &matrix)
{
  obj.SetT2O (matrix * obj.GetT2O ());  
}

void BaseMovable::SetSector (iSector *sector)
{
  if (parent != 0) return;
  if (sectors.GetSize () == 1 && sector == sectors[0]) return ;
  ClearSectors ();
  if (sectors.PrepareSector (sector))
    sectors.Push (sector);
}

void BaseMovable::ClearSectors ()
{
  if (meshobject) meshobject->RemoveFromSectors ();
  sectors.Empty ();
}

void BaseMovable::AddListener (iMovableListener *listener)
{
  RemoveListener (listener);
  listeners.Push (listener);
}

void BaseMovable::RemoveListener (iMovableListener *listener)
{
  listeners.Delete (listener);
}

void BaseMovable::UpdateMove ()
{
  updatenr++;
  is_identity = obj.IsIdentity ();

  if (lightobject) lightobject->OnSetPosition ();

  size_t i;
  for (i = 0 ; i < scene_children.GetSize () ; i++)
    scene_children[i]->GetMovable ()->UpdateMove ();

  i = listeners.GetSize ();
  while (i > 0)
  {
    i--;
    iMovableListener *ml = listeners[i];
    ml->MovableChanged (this);
  }
}

iSceneNode* BaseMovable::GetSceneNode ()
{
  if (meshobject)
    return meshobject->QuerySceneNode ();
  if (lightobject)
    return lightobject->QuerySceneNode ();
  if (cameraobject)
    return cameraobject->QuerySceneNode ();
  return 0;
}

}
}

