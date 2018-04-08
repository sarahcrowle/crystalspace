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
 * This code is a (very very) basic implementation of an item and weapon system.
 */

#include "cssysdef.h"
#include "items.h"
#include "agent.h"

using namespace CS::Collisions;
using namespace CS::Physics;

ItemMgr* ItemMgr::Instance = new ItemMgr;

Item* Inventory::AddItem(ItemTemplate& templ)
{
  csRef<Item> item = ItemMgr::Instance->CreateItem(templ);
  item->inventory = this;
  items.Push(item);
  return item;
}

Item::Item() : 
  scfImplementationType(this),
  amount(1)
{
}

csReversibleTransform Item::GetFullTransform() const
{
  if (inventory)
  {
    return relativeTransform * GetOwner()->GetObject()->GetTransform();
  }
  return relativeTransform;
}

Agent* Item::GetOwner() const
{
  return !inventory ? nullptr : inventory->GetOwner();
}

/// Returns the sector that this item belongs to
CS::Physics::iPhysicalSector* Item::GetSector() const 
{
  return !inventory ? nullptr : dynamic_cast<CS::Physics::iPhysicalSector*>(inventory->GetOwner()->GetObject()->GetSector());
}

/// Returns the system that this item belongs to
CS::Physics::iPhysicalSystem* Item::GetSystem() const 
{ 
  return !inventory ? nullptr : dynamic_cast<CS::Physics::iPhysicalSystem*>(inventory->GetOwner()->GetObject()->GetSector()->GetSystem()); 
}


csPtr<Item> ItemMgr::CreateItem(ItemTemplate& templ)
{
  Item* item = new Item();
  item->InitItem(&templ);
  return csPtr<Item>(item);
}

ItemTemplate& ItemMgr::CreateTemplate(const char* name)
{
  size_t i = Templates.GetSize();
  Templates.SetSize(i + 1);
  ItemTemplate& templ = Templates[i];
  templ.index = i;
  templ.name = name;
  return templ;
}
