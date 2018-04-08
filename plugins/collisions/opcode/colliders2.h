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

#ifndef __CS_OPCODE_COLLIDER_H__
#define __CS_OPCODE_COLLIDER_H__

#include "imesh/terrain2.h"
#include "ivaria/collisions.h"
#include "Opcode.h"
#include "opcodecollisionsystem.h"

CS_PLUGIN_NAMESPACE_BEGIN (Opcode2)
{
class csOpcodeCollider : public scfImplementation1<
  csOpcodeCollider, CS::Collisions::iColliderConcaveMesh>
{
  friend class csOpcodeCollisionObject;
  friend class csOpcodeCollisionSector;

protected:
  csOpcodeCollisionSystem* system;
  iMeshWrapper* mesh;
  float volume;
  Opcode::Model* model;
  Opcode::MeshInterface opcMeshInt;
  unsigned int* indexholder;
  Point *vertholder;
  csVector3 scale;
  csBox3 aabbox;

  static void MeshCallback (udword triangle_index, 
    Opcode::VertexPointers& triangle, void* user_data);

  // for overriding
  csOpcodeCollider (csOpcodeCollisionSystem* sys) : scfImplementationType (this), system (sys), mesh (nullptr) {}

public:
  csOpcodeCollider (iMeshWrapper* mesh, csOpcodeCollisionSystem* sys);
  virtual ~csOpcodeCollider();
  virtual CS::Collisions::ColliderType GetColliderType () const {return CS::Collisions::COLLIDER_CONCAVE_MESH;}
  virtual iMeshWrapper* GetMesh () {return mesh;}
  virtual void SetLocalScale (const csVector3& scale) {}
  virtual const csVector3& GetLocalScale () const {return scale;}
  virtual void SetMargin (float margin) {}
  virtual float GetMargin () const {return 0;}
  virtual float GetVolume () const {return volume;}
  virtual bool IsDynamic () const {return false;}

  virtual void GetAABB(csVector3& aabbMin, csVector3& aabbMax) const { /* TODO */ }
  
  virtual void AddCollider (CS::Collisions::iCollider* collider, const csOrthoTransform& relaTrans = csOrthoTransform ()) {}
  virtual void RemoveCollider (CS::Collisions::iCollider* collider) {}
  virtual void RemoveCollider (size_t index) {}

  virtual CS::Collisions::iCollider* GetCollider (size_t index) { return this; }
  virtual size_t GetColliderCount () { return 1;}
  
  /// Get the frame of reference
  virtual csOrthoTransform GetPrincipalAxisTransform() const 
  {
    csOrthoTransform trans;
    return trans;
  }

  /// Set the frame of reference
  virtual void SetPrincipalAxisTransform(const csOrthoTransform& trans)
  {
    // Does nothing for now
  }
};

class csOpcodeColliderTerrain : public scfVirtImplementationExt1<
  csOpcodeColliderTerrain, 
  csOpcodeCollider, 
  CS::Collisions::iColliderTerrain>
{
  friend class csOpcodeCollisionObject;

  static void MeshCallback (udword triangle_index, 
    Opcode::VertexPointers& triangle, void* user_data);

public:
  iTerrainCell* cell;
  csOrthoTransform cellTransform;

  csOpcodeColliderTerrain(iTerrainCell* cell, csOrthoTransform trans, csOpcodeCollisionSystem* sys);
  virtual ~csOpcodeColliderTerrain();
  
  virtual CS::Collisions::ColliderType GetColliderType () const {return CS::Collisions::COLLIDER_TERRAIN;}
  virtual iTerrainCell* GetCell() const { return cell; }

  /// Get the frame of reference
  virtual csOrthoTransform GetPrincipalAxisTransform() const 
  {
    csOrthoTransform trans;
    return trans;
  }

  /// Set the frame of reference
  virtual void SetPrincipalAxisTransform(const csOrthoTransform& trans)
  {
    // Does nothing for now
  }
};

class csOpcodeCollisionTerrain : public scfVirtImplementation2<
  csOpcodeCollisionTerrain,
  CS::Collisions::iCollisionTerrain, 
  iTerrainCellLoadCallback>
{
  friend class csOpcodeCollisionObject;
  friend class csOpcodeCollisionSector;

  csRefArray<csOpcodeCollisionObject> objects;
  iTerrainSystem* terrainSystem;
  csOpcodeCollisionSystem* system;

  csOpcodeCollisionSector* sector;
  csOrthoTransform terrainTransform;
  float volume;
  bool unload;
  csVector3 scale;

public:
  csOpcodeCollisionTerrain(iTerrainSystem* terrain, csOpcodeCollisionSystem* sys);
  virtual ~csOpcodeCollisionTerrain();
  virtual CS::Collisions::ColliderType GetType() const {return CS::Collisions::COLLIDER_TERRAIN;}
  virtual iTerrainSystem* GetTerrain() const {return terrainSystem;}
  virtual void SetLocalScale (const csVector3& scale) {}
  virtual const csVector3& GetLocalScale () const {return scale;}
  virtual void SetMargin (float margin) {}
  virtual float GetMargin () const {return 0;}
  virtual float GetVolume () const {return volume;}

  virtual void OnCellLoad (iTerrainCell *cell);
  virtual void OnCellPreLoad (iTerrainCell *cell);
  virtual void OnCellUnload (iTerrainCell *cell);

  void LoadCellToCollider (iTerrainCell* cell);
};
}
CS_PLUGIN_NAMESPACE_END (Opcode2)
#endif
