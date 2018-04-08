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

// TODO: Remove all of that and/or move it in DemoApplication

#ifndef __PHYSTUT2_ITEMS_H__
#define __PHYSTUT2_ITEMS_H__

#include "ivaria/physics.h"
#include "ivaria/collisions.h"

#include "imesh/animesh.h"
#include "imesh/animnode/ragdoll2.h"
#include "imesh/modifiableterrain.h"

#include "iengine/movable.h"

#include "csutil/scf.h"
#include "csutil/list.h"
#include "csutil/csobject.h"

static const int MaxFunctionsPerItem = 14;

class Agent;

class Item;
class ItemTemplate;
class ItemMgr;

// #########################################################################################################################
// Inventory

/**
 * An inventory is a collection of all items that belong to some object
 */
class Inventory
{
  friend class Agent;

  Agent* owner;
  csRefArray<Item> items;

public:
  Inventory()
  {
  }
  
  Agent* GetOwner() const { return owner; }
  void SetOwner(Agent* o) { owner = o; }
  
  csRefArray<Item> GetItems() { return items; }
  
  Item* GetItem(int i) { return items[i]; }

  Item* AddItem(ItemTemplate& templ);
};

// #########################################################################################################################
// Items

/**
 * One function of an item
 */
class ItemFunction
{
protected:
  csString name;

public:
  ItemFunction(const char* name) : name(name) {}
  const char* GetName() const { return name; }

  /// Tries to use the item. Returns false, if function could not be used.
  virtual bool Use(Item* item) = 0;
};


/**
 * An ItemTemplate is shared by all instances of the type of item it represents.
 * That means for every type of item, you only need one global instance of ItemTemplate.
 */
class ItemTemplate
{
  friend class Item;
  friend class ItemMgr;
private:
  int index;
  csString name;
  
protected:
  csArray<ItemFunction*> primaryFunctions;
  csArray<ItemFunction*> secondaryFunctions;

public:
  ItemTemplate()
  {
  }

  void AddPrimaryFunction(ItemFunction* function)
  {
    primaryFunctions.Push(function);
  }

  void AddSecondaryFunction(ItemFunction* function)
  {
    secondaryFunctions.Push(function);
  }

  /// Remove all functions from this template
  void Clear()
  {
    primaryFunctions.DeleteAll();
    secondaryFunctions.DeleteAll();
  }

  /// The index within the ItemMgr's array of all ItemTemplates
  int GetIndex() const { return index; }
  
  const csArray<ItemFunction*>& GetPrimaryFunctions() const { return primaryFunctions; }
  const csArray<ItemFunction*>& GetSecondaryFunctions() const { return secondaryFunctions; }

  ItemFunction* GetPrimaryFunction(int i) { return primaryFunctions[i]; }
  ItemFunction* GetSecondaryFunction(int i) { return secondaryFunctions[i]; }

  int GetFunctionCount() const { return primaryFunctions.GetSize() + secondaryFunctions.GetSize(); }
  
  const char* GetName() const { return name; }
};

/**
 * Represents a stack of items (with only one by default), attached to an iCollisionObject.
 * Items are just movables that have a specific set of functions which can be used by actors.
 */
class Item : public scfVirtImplementationExt1<Item, csObject, iBase>
{
  friend class ItemMgr;
  friend class Inventory;

protected:
  Inventory* inventory;
  int amount;
  ItemTemplate* templ;
  csOrthoTransform relativeTransform;

  void InitItem(ItemTemplate* t)
  {
    templ = t;
  }

public:
  Item();

  /// Get the inventory to which this item currently belongs
  inline Inventory* GetInventory() const { return inventory; }
  /// Set the inventory to which this item currently belongs
  inline void SetInventory(Inventory* inv) { inventory = inv; }

  inline int GetAmount() const { return amount; }
  inline void SetAmount(int value) { amount = value; }
  
  inline ItemTemplate& GetTemplate() { return *templ; }

  /// The name of this item
  virtual const char* GetName() const { return templ->GetName (); }

  inline Agent* GetOwner() const;

  /// Returns the sector that this item belongs to
  inline CS::Physics::iPhysicalSector* GetSector() const;

  /// Returns the system that this item belongs to
  inline CS::Physics::iPhysicalSystem* GetSystem() const;

  /// Transform of this item in world space
  inline csReversibleTransform GetFullTransform() const;

};

/**
 * Manages templates
 */
class ItemMgr
{
public:
  static ItemMgr* Instance;
  
public:
  csArray<ItemTemplate> Templates;

public:
  ItemMgr() 
  {
    Instance = this;
  }

public:
  csPtr<Item> CreateItem(ItemTemplate& templ);

  ItemTemplate& CreateTemplate(const char* name);

  size_t GetTemplateCount() { return Templates.GetSize(); }

  ItemTemplate& GetTemplate(size_t i) { return Templates[i]; }
};

#endif
