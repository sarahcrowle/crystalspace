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

#ifndef __CS_CSTOOL_EMPTYSCENENODE_H__
#define __CS_CSTOOL_EMPTYSCENENODE_H__

/**\file
 * Empty scene node implementation.
 */

#include "iengine/scenenode.h"
#include "cstool/basemovable.h"
#include "cstool/objmodel.h"

namespace CS
{
namespace Engine
{

/// Helper class for iSceneNode.
class /*CS_CRYSTALSPACE_EXPORT*/ SceneNodeHelper
      // Note: not exported b/c entirely implemented inline
{
public:
  static void SetParent (iSceneNode* this_node, iSceneNode* parent,
  	BaseMovable* this_movable)
  {
    BaseMovable* parent_mov = this_movable->GetParent ();
    if (!parent_mov && !parent) return;
    if (parent_mov && parent_mov->GetSceneNode () == parent) return;

    if (parent_mov)
    {
      csRefArray<iSceneNode>& parent_children = parent_mov->GetChildren ();
      size_t idx = parent_children.Find (this_node);
      CS_ASSERT (idx != csArrayItemNotFound);
      parent_children.DeleteIndex (idx);
    }

    if (parent)
      this_movable->SetParent ((BaseMovable*)(parent->GetMovable ()));
    else
      this_movable->SetParent (0);

    if (parent)
    {
      parent_mov = (BaseMovable*)(parent->GetMovable ());
      csRefArray<iSceneNode>& parent_children = parent_mov->GetChildren ();
#ifdef CS_DEBUG
      size_t idx = parent_children.Find (parent);
      CS_ASSERT (idx == csArrayItemNotFound);
#endif
      parent_children.Push (this_node);
    }
  }
};

class EmptySceneNode : public scfImplementationExt1<
					      EmptySceneNode, csObjectModel, iSceneNode>
{
private:
  BaseMovable movable;

public:
  EmptySceneNode () : scfImplementationType (this) { }
  virtual ~EmptySceneNode () { }

  virtual iMovable* GetMovable () const { return movable.QueryMovable (); }
  virtual iObjectModel* GetObjectModel () { return this; }
  virtual iMeshWrapper* QueryMesh () { return movable.GetMeshWrapper (); }
  virtual iLight* QueryLight () { return movable.GetLight (); }
  virtual iCamera* QueryCamera () { return movable.GetCamera (); }
  virtual void SetParent (iSceneNode* parent)
  {
    iMovable* parent_mov = movable.GetParent ();
    if (!parent_mov && !parent) return;
    if (parent_mov && parent_mov->GetSceneNode () == parent) return;
    if (parent)
      movable.ClearSectors ();

    SceneNodeHelper::SetParent ((iSceneNode*)this, parent, &movable);
  }
  virtual iSceneNode* GetParent () const
  {
    if (movable.GetParent ())
      return movable.GetParent ()->GetSceneNode ();
    else
      return 0;
  }
  virtual const csRefArray<iSceneNode>& GetChildren () const
  {
    return movable.GetChildren ();
  }
  virtual csPtr<iSceneNodeArray> GetChildrenArray () const
  {
    return csPtr<iSceneNodeArray> (
      new scfArrayWrapConst<iSceneNodeArray, csRefArray<iSceneNode> > (
      movable.GetChildren ()));
  }
};

} // namespace Engine
} // namespace CS

/** @} */

#endif // __CS_CSTOOL_EMPTYSCENENODE_H__

