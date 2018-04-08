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

#include "cssysdef.h"
#include "physicalbody.h"

using namespace CS::Collisions;
using namespace CS::Physics;


CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{
  void csPhysicalBody::CreatePhysicalBodyObject (iPhysicalObjectFactory* props)
  {
    CreateCollisionObject (props);
    SetMass (props->GetMass ());

    SetFriction (props->GetFriction ());
    SetGravityEnabled (props->GetGravityEnabled ());
  }

csPhysicalBody::csPhysicalBody (csBulletSystem* phySys)
: scfImplementationType (this, phySys)
{
}

csPhysicalBody::~csPhysicalBody ()
{
}

void csPhysicalBody::SetEnabled (bool enabled)
{
  CS_ASSERT (btObject);

  if (enabled)
    btObject->activate (true);

  else
  {
    SetLinearVelocity (csVector3 (0.0f));
    SetAngularVelocity (csVector3 (0.0f));
    // TODO: why the following line?
    btObject->setInterpolationWorldTransform (btObject->getWorldTransform ());
    btObject->setActivationState (ISLAND_SLEEPING);
  }
}

bool csPhysicalBody::GetEnabled () const
{
 CS_ASSERT (btObject);
 return btObject->isActive ();
}

csPtr<CS::Collisions::iCollisionObject> csPhysicalBody::ClonePassivePortalObject () 
{ 
  csRef<iCollisionObject> clone (CloneObject ());
  // TODO: cloning not working for soft bodies
  if (!clone) return csPtr<CS::Collisions::iCollisionObject> (nullptr);

  csRef<iPhysicalBody> obj = scfQueryInterface<iPhysicalBody> (clone);

  if (obj)
  {
    // disable gravity for cloned objects
    obj->SetGravityEnabled (false);
  }

  return csPtr<iCollisionObject>(obj);
}

}
CS_PLUGIN_NAMESPACE_END (Bullet2)
