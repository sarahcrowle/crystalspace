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
 * A very basic Agent implementation.
 */

#ifndef __PHYSTUT2_AGENT_H
#define __PHYSTUT2_AGENT_H

// TODO: Remove all of that

#include "items.h"

/**
 * An agent is either a player or an AI.
 * An agent owns her own inventory.
 * An agent is represented by some iCollisionObject.
 */
class Agent
{
protected:
  Inventory inventory;
  csRef<CS::Collisions::iCollisionObject> object;
  
public:
  Agent()
  {
    inventory.owner = this;
  }

  Inventory& GetInventory () { return inventory; }

  CS::Collisions::iCollisionObject* GetObject () const
  { return object; }
  void SetObject (CS::Collisions::iCollisionObject* o)
  { object = o; } 

  CS::Collisions::iCollisionActor* GetActor () const
  { return object ? object->QueryCollisionActor () : nullptr; }
};

#endif
