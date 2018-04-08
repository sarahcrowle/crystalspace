/*
    Copyright (C) 2002 by Jorrit Tyberghein
    Copyright (C) 2013 by Matthieu Kraus

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

#ifndef __CS_BVH_H__
#define __CS_BVH_H__

#include "csgeom/spatialtree.h"

#include "csutil/scf_implementation.h"

/**\file
 * Bounding Volume Hierachy implementation.
 */
/**\addtogroup geom_utils
 * @{ */

namespace CS
{
namespace Geometry
{
/**
 * A Bounding Volume Hierachy (BVH).
 * A BVH is a binary tree that organizes 3D space.
 * This implementation is dynamic. It allows moving, adding, and
 * removing of objects which will alter the tree dynamically.
 * The main purpose of this tree is to provide fast approximate
 * front to back ordering.
 * <p>
 * The BVH supports delayed insertion. When objects are inserted
 * in the tree they are not immediatelly distributed over the
 * nodes but instead they remain in the main node until it is really
 * needed to distribute them. The advantage of this is that you can
 * insert/remove a lot of objects in the tree and then do the distribution
 * calculation only once. This is more efficient and it also generates
 * a better tree as more information is available then.
 * <p>
 * As this implementation is supposed to be as fast as possible sanity
 * checks are only performed in debug mode, so those should be performed
 * additionally if needed.
 */
template<class ChildType>
class BVH :
  public SpatialTree<BVH<ChildType>, ChildType>
{
  friend class SpatialTree<BVH<ChildType>, ChildType>;
public:
  // convenience typedefs
  typedef BVH<ChildType> Self;
  typedef SpatialTree<Self, ChildType> SpatialTreeType;
  typedef typename SpatialTreeType::Child Child;
  typedef typename SpatialTreeType::BoundType BoundType;

  /**
   * A callback function for visiting a BVH node. If this function
   * returns true the traversal will continue. Otherwise Front2Back()
   * will return to the next traversal point or stop.
   * <p>
   * This function is itself responsible for calling Distribute() on
   * the given treenode to ensure that the objects in this node
   * are properly distributed to the children. If the function doesn't
   * want or need this functionality it doesn't have to do Distribute().
   * <p>
   * 'frustum_mask' can be modified by this function to reduce the number
   * of plane tests (for frustum culling) that have to occur for children
   * of this node.
   */
  typedef bool (VisitFunc)(Self* treenode, void* userdata, uint32& frustum_mask);

private:
  float blockThreshold; // minimum quality that'll be accepted

  // helper structure used to find split locations
  struct SortElement
  {
    float center;
    csBox3 box;
    int idx;

    static int compare(SortElement const& l, SortElement const& r)
    {
      if(l.center < r.center) return -1;
      else if(l.center > r.center) return 1;
      else return 0;
    }
  };


  /**
   * Unlink an object from the tree.
   */
  void UnlinkObject(Child* obj)
  {
    // get tree the object belongs to
    Self* tree = static_cast<Self*>(obj->GetLeaf());

    // validate tree
    CS_ASSERT(tree);

    // find object in the tree
    int idx = tree->FindObject(obj);

    // ensure it was found
    CS_ASSERT(idx >= 0);

    // unlink object from tree
    tree->RemoveObject(idx);

    // get parent of the tree
    Self* p = tree->GetParent();

    // check whether that leaf is empty and not root
    if(tree->numObjects == 0 && p != nullptr)
    {
      // it's empty and has a parent, flatten it
      p->Flatten();
    }
  }

  /**
   * If this node is a leaf then we will split the objects currently
   * in this leaf according to the pre-filled values of split_axis
   * and split.
   */
  void DistributeLeafObjects()
  {
    // we shouldn't be in blocking mode
    CS_ASSERT(this->block == 0);

    // we have to have children
    CS_ASSERT(this->child1 != nullptr && this->child2 != nullptr);

    // keep track how many objects couldn't be distributed
    int failed = 0;

    // get center and split axis as we sort objects
    // into children based on it
    int const split = this->box.GetSize().DominantAxis();
    float const center = this->box.GetCenter()[split];

    // distribute objects
    for(int i = 0; i < this->numObjects; ++i)
    {
      // get object to distribute
      Child* obj = this->objects[i];

      // get object box
      csBox3 const& objBox = obj->GetBBox();

      // check whether it belongs to child1
      if(objBox.GetCenter()[split] < center)
      {
	this->child1->AddObject(obj);
	this->child1->box += objBox;
      }
      // else it belongs to child2
      else
      {
	// add object to child2
	this->child2->AddObject(obj);
	this->child2->box += objBox;
      }
    }

    // update our bounding box
    this->box = this->child1->box + this->child2->box;

    // update object count for our node (this is equivalent
    // to a truncation of our array)
    this->numObjects = 0;
  }

  /// Make the tree empty.
  void ClearSplit()
  {
  }

public:
  /// Create a new empty BVH.
  BVH() :
    // split initialization
    blockThreshold(0.0f)
  {
  }

  /// Destroy the tree.
  virtual ~BVH()
  {
    this->Clear();
  }

  /**
   * Move an object (give it a new bound).
   */
  void MoveObject(Child* obj, BoundType const& newBound)
  {
    // get old bbox
    csBox3 oldBox = obj->GetBBox();

    // set new bounds
    obj->SetBounds(newBound);

    // get new bbox
    csBox3 newBox = obj->GetBBox();

    // validate it actually moved
    if((oldBox.Min() - newBox.Min()).IsZero() && (oldBox.Max() - newBox.Max()).IsZero())
    {
      // didn't move, nothing to be done
      return;
    }

    // get the leaf this object belongs to
    Self* tree = static_cast<Self*>(obj->GetLeaf());

    // validate the leaf is valid
    CS_ASSERT(tree);

    // check whether the object still fits in that node
    if(tree->GetNodeBBox().Contains(newBox))
    {
      // does fit, no need to actually move the object
      // decrement block count if there is a block
      if(tree->block > 0)
      {
	--tree->block;
      }
    }
    else
    {
      // find the object in it's tree
      int idx = tree->FindObject(obj);

      // validate it was found
      CS_ASSERT(idx >= 0);

      // remove of it from tree
      tree->RemoveObject(idx);

      // find the first ancestor it fits into
      for(tree = tree->GetParent(); tree; tree = tree->GetParent())
      {
	if(tree->GetNodeBBox().Contains(newBox))
	{
	  break;
	}
      }

      // validate it did fit somewhere (it should at least fit into root)
      CS_ASSERT(tree);

      // add the object to the ancestor we found
      tree->AddObjectInternal(obj);
    }
  }

  /**
   * Distribute all objects in this node to its children.
   * This may also create new children if needed. Note that this
   * will only distribute one level (this node) and will not
   * recurse into the children.
   */
  void Distribute()
  {
    // check our distribution state is consistent
    CS_ASSERT((this->child1 == nullptr) == (this->child2 == nullptr));

    // ensure our storage is valid
    CS_ASSERT(this->objects != nullptr && this->maxObjects >= this->numObjects);

    // check for distribution block due to failed attempts
    if(this->block > 0)
    {
      // we don't want to try again, yet
      return;
    }

    // check whether there is anything to distribute
    if(this->numObjects == 0)
    {
      // nothing to distribute
      // check that we are not a leaf - leaves must have objects
      CS_ASSERT(((this->child1 != nullptr) && (this->child2 != nullptr)) || (this->parent == nullptr));
      return;
    }

    // check whether we already have a split for this node
    if(this->child1 == nullptr) // nope
    {
      // do we have enough objects for a new split?
      if(this->numObjects < this->minSplitObjects)
      {
	// nope, nothing to be done
	return;
      }
      else
      {
	// allocate buffer to hold the splitalloc
	SortElement* buffer = static_cast<SortElement*>(cs_m(sizeof(SortElement)*this->numObjects));

	// validate allocation
	CS_ASSERT(buffer);

	// get split
	// split along longest axis
	int axis = this->box.GetSize().DominantAxis();

	// fill sort array
	for(int i = 0; i < this->numObjects; ++i)
	{
	  SortElement& s = buffer[i];
	  s.box = this->objects[i]->GetBBox();
	  s.center = s.box.GetCenter()[axis];
	  s.idx = i;
	}

	// sort by center
	qsort(buffer, this->numObjects, sizeof(SortElement), (int (*)(void const*, void const*))SortElement::compare);

	// calculate child boxes
	csBox3 leftBox;
	for(int i = 0; i < (this->numObjects >> 1); ++i)
	{
	  leftBox += buffer[i].box;
	}
	csBox3 rightBox;
	for(int i = this->numObjects >> 1; i < this->numObjects; ++i)
	{
	  rightBox += buffer[i].box;
	}

	// evaluate quality
	float quality = this->box.Volume() - leftBox.Volume() - rightBox.Volume();

	// check whether the best split is actually good enough
	if(quality > blockThreshold)
	{
	  // allocate childs
	  this->child1 = this->TreeAlloc().Alloc();
	  this->child2 = this->TreeAlloc().Alloc();

	  // verify allocations
	  CS_ASSERT(this->child1);
	  CS_ASSERT(this->child2);

	  // set us as their parent
	  this->child1->SetParent(this);
	  this->child2->SetParent(this);

	  // distribute objects according to the split we found
	  // first child gets lower half
	  for(int i = 0; i < (this->numObjects >> 1); ++i)
	  {
	    this->child1->AddObject(this->objects[buffer[i].idx]);
	  }

	  // second child gets upper half
	  for(int i = this->numObjects >> 1; i < this->numObjects; ++i)
	  {
	    this->child2->AddObject(this->objects[buffer[i].idx]);
	  }

	  // truncate our object list as we distributed all childs
	  this->numObjects = 0;

	  // update their boxes so they know where they belong
	  this->child1->box = leftBox;
	  this->child2->box = rightBox;

	  // update our estimated object count
	  this->estimateObjects = this->child1->numObjects + this->child2->numObjects;
	}
	else
	{
	  // bad split - let's wait a bit before we try again
	  this->block = this->blockTime;
	}

        // free our buffer as we don't need it anymore
	cs_free(buffer);
      }
    }
    else // yep, there's a split
    {
      // try to distribute the objects with our current split
      DistributeLeafObjects();

      // check whether it worked
      if(this->numObjects != 0)
      {
	// nope... any ideas? no? time to start from scratch I guess
	// @@@RlyDontKnow: TODO: we should try to fit them in somehow
	this->Flatten();
	Distribute();
      }
    }
  }

  /**
   * Traverse the tree in random order.
   * The mask parameter is optionally used for frustum checking.
   * TraverseRandom will pass it to the tree nodes.
   */
  void TraverseRandom(VisitFunc* func, void* data, uint32 frustumMask)
  {
    // validate we got a visiting function
    CS_ASSERT(func);

    // check whether we want to continue this traversal
    if(!func(this, data, frustumMask))
    {
      return;
    }

    // ensure child state is consistent
    CS_ASSERT((this->child1 == nullptr) == (this->child2 == nullptr));

    // check whether we have children to traverse
    if(this->child1)
    {
      // traverse children
      this->child1->TraverseRandom(func, data, frustumMask);
      this->child2->TraverseRandom(func, data, frustumMask);
    }
  }

  /**
   * Traverse the tree in approximate front to back order. Every node of
   * the tree will be encountered at most once. Returns false if traversal
   * in this branch should stop (may continue in an alternate branch
   * that may have been in front of this one if there is any - i.e. if
   * the childs of this node are overlapping).
   * The mask parameter is optionally used for frustum checking.
   * Front2Back will pass it to the tree nodes.
   */
  void Front2Back(csVector3 const& pos, VisitFunc* func, void* data, uint32 frustumMask)
  {
    // validate we got a visiting function
    CS_ASSERT(func);

    // check whether we want to continue the traversal
    if(!func(this, data, frustumMask))
    {
      return;
    }

    // ensure child state is consistent
    CS_ASSERT((this->child1 == nullptr) == (this->child2 == nullptr));

    // continue with children if we have any
    if(this->child1)
    {
      // check which child should be visited first based on the angle between
      // our direction and the difference between the centers of the children
      // get center difference vector
      csVector3 const centerDiff = this->child2->box.GetCenter() - this->child1->box.GetCenter();
      if(centerDiff * pos > 0)
      {
	// left child goes first
	this->child1->Front2Back(pos, func, data, frustumMask);
	this->child2->Front2Back(pos, func, data, frustumMask);
      }
      else
      {
	// right child goes first
	this->child2->Front2Back(pos, func, data, frustumMask);
	this->child1->Front2Back(pos, func, data, frustumMask);
      }
    }
  }

private:
  // debugging functions
  bool DebugCheckSplit(csString& str)
  {
    return true;
  }

  void DebugDumpSplit(csString& str, csString const& indent)
  {
  }
};

} // namespace Geometry
} // namespace CS

typedef CS::Geometry::SpatialTreeChild::BoxChild<false> csBVHChild;
typedef CS::Geometry::BVH<csBVHChild> csBVH;

/** @} */

#endif // __CS_BVH_H__

