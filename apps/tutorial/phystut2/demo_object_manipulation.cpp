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

/**
 * Manipulate and interact with objects
 */

#include "cssysdef.h"
#include "csgeom/poly3d.h"
#include "csgeom/sphere.h"
#include "iengine/portal.h"
#include "iengine/campos.h"
#include "imesh/genmesh.h"
#include "imesh/terrain2.h"
#include "ivaria/colliders.h"
#include "cstool/genmeshbuilder.h"
#include "cstool/materialbuilder.h"
#include "csutil/floatrand.h"

#include "physdemo.h"

using namespace CS::Collisions;
using namespace CS::Physics;
using namespace CS::Geometry;

bool PhysDemo::PickCursorObject (CS::Collisions::HitBeamResult& result)
{ 
  // Find the object under the cursor:

  // Compute the end beam points
  csRef<iCamera> camera = view->GetCamera ();
  csVector2 v2d (mouse->GetLastX (), mouse->GetLastY ());
  csVector3 v3d = view->InvProject (v2d, 10000);
  csVector3 startBeam = camera->GetTransform ().GetOrigin ();
  csVector3 endBeam = camera->GetTransform ().This2Other (v3d);

  // Trace the physical beam
  result = GetCurrentSector ()->HitBeamPortals (startBeam, endBeam);

  // Print the name of the object and its relations on standard output
  if (result.hasHit)
  {
    csPrintf ("Clicked on object:\n");
    csPrintf ("- Collision object name: %s\n",
	      CS::Quote::Single (result.object->QueryObject ()->GetName ()));

    // Query the parent iObject
    csPrintf ("- Parent object: ");
    iObject* parent = result.object->QueryObject ()->GetObjectParent ();
    if (!parent) csPrintf ("none\n");
    else csPrintf ("%s\n", CS::Quote::Single (parent->GetName ()));

    // Grab the scene node that is attached to the collision object
    csPrintf ("- Attached scene node: ");
    iSceneNode* node = result.object->GetAttachedSceneNode ();
    if (!node) csPrintf ("none\n\n");
    else
    {
      // Find the type of the node
      if (node->QueryMesh ())
	csPrintf ("mesh %s\n",
		  CS::Quote::Single (node->QueryMesh ()->QueryObject ()->GetName ()));
      else if (node->QueryLight ())
	csPrintf ("light %s\n",
		  CS::Quote::Single (node->QueryLight ()->QueryObject ()->GetName ()));
      else csPrintf ("unknown\n");
      csPrintf ("\n");
    }
  }

  return result.hasHit;
}

::iBase* PhysDemo::GetOwner (CS::Collisions::iCollisionObject* obj)
{
  // TODO: Need a generic mechanism to determine ownership of objects

  // check for actor
  if (obj->QueryCollisionActor ())
  {
    return obj->QueryCollisionActor ();
  }

  // check for vehicle
  iVehicle* vehicle = dynamic_cast<iVehicle*> (obj);
  if (vehicle)
  {
    return vehicle;
  }

  // no owner
  return nullptr;
}

void PhysDemo::PullObject (CS::Collisions::iCollisionObject* obj)
{
  csVector3 from;
  if (!obj)
  {
    // Pick object
    HitBeamResult result;
    if (!PickCursorObject (result) || !IsDynamic (result.object)) return;    // didn't hit anything dynamic
    obj = result.object;
    from = result.isect;
  }
  else
  {
    from = obj->GetTransform ().GetOrigin ();
  }
   
  iPhysicalBody* pb = obj->QueryPhysicalBody ();

  csVector3 posCorrection (2  * UpVector);

  csVector3 impulse (GetActorPos () - from - posCorrection);
  impulse.Normalize ();
  impulse *= 30 * pb->GetMass ();

  // prevent sliding problem
  csOrthoTransform trans = pb->GetTransform ();
  trans.SetOrigin (trans.GetOrigin () + posCorrection);
  pb->SetTransform (trans);

  pb->ApplyImpulse (impulse);
}

void PhysDemo::DeleteObject (CS::Collisions::iCollisionObject* obj)
{
  if (!obj)
  {
    // Pick object
    HitBeamResult result;
    if (!PickCursorObject (result)) return;    // didn't hit anything
    obj = result.object;
  }
   
  if (!GetOwner (obj))
  {
    // can only remove it, if it has no owner
    obj->GetSector ()->RemoveCollisionObject (obj);
  }
  else
  {
    // TODO: Handle removal of complex game entities generically
    ReportWarning ("Cannot trivially remove the given object because it is part of a complex object.\n");
  }
}

void PhysDemo::ToggleObjectState (CS::Collisions::iCollisionObject* obj)
{
  if (!obj)
  {
    // Pick object
    HitBeamResult result;
    if (!PickCursorObject (result)) return;    // didn't hit anything
    obj = result.object;
  }

  // The object must be a rigid body in order to be toggled
  if (!obj->QueryPhysicalBody () || !obj->QueryPhysicalBody ()->QueryRigidBody ())
    return;

  iRigidBody* body = obj->QueryPhysicalBody ()->QueryRigidBody ();
  RigidBodyState state = body->GetState ();

  // Don't toggle kinematic objects
  if (state == STATE_KINEMATIC) return;

  // Toggle the state of the object
  if (state == STATE_STATIC)
    body->SetState (STATE_DYNAMIC);
  else
    body->SetState (STATE_STATIC);
}

void PhysDemo::TeleportObject (CS::Collisions::iCollisionObject* obj, iCameraPosition* pos)
{
  // set transform
  csOrthoTransform trans (csMatrix3 (), pos->GetPosition ());
  trans.LookAt (pos->GetForwardVector (), pos->GetUpwardVector ());
  if (obj) obj->SetTransform (trans);
  else view->GetCamera ()->SetTransform (trans);

  // set sector
  iSector* isector = engine->FindSector (pos->GetSector ());
  if (obj)
  {
    iCollisionSector* collSector = physicalSystem->FindCollisionSector (isector);
    CS_ASSERT (collSector);
    collSector->AddCollisionObject (obj);
  }
  else view->GetCamera ()->SetSector (isector);
}
