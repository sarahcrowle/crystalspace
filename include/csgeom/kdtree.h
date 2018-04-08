/*
    Copyright (C) 2002 by Jorrit Tyberghein

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

#ifndef __CS_KDTREE_H__
#define __CS_KDTREE_H__

#include "csgeom/spatialtree.h"

#include "csutil/scf_implementation.h"

#include "iutil/dbghelp.h"

/**\file
 * KD-tree implementation.
 */
/**\addtogroup geom_utils
 * @{ */

struct iGraphics3D;

namespace CS
{
namespace Geometry
{
enum
{
  CS_KDTREE_AXISINVALID = -1,
  CS_KDTREE_AXISX = 0,
  CS_KDTREE_AXISY = 1,
  CS_KDTREE_AXISZ = 2
};

/**
 * A KD-tree.
 * A KD-tree is a binary tree that organizes 3D space.
 * This implementation is dynamic. It allows moving, adding, and
 * removing of objects which will alter the tree dynamically.
 * The main purpose of this tree is to provide for an approximate
 * front to back ordering.
 * <p>
 * The KD-tree supports delayed insertion. When objects are inserted
 * in the tree they are not immediatelly distributed over the
 * nodes but instead they remain in the main node until it is really
 * needed to distribute them. The advantage of this is that you can
 * insert/remove a lot of objects in the tree and then do the distribution
 * calculation only once. This is more efficient and it also generates
 * a better tree as more information is available then.
 */
template<class ChildType>
class KDTree :
  public SpatialTree<KDTree<ChildType>, ChildType>
{
  friend class SpatialTree<KDTree<ChildType>, ChildType>;
public:
  // convenience typedefs
  typedef KDTree<ChildType> Self;
  typedef SpatialTree<Self, ChildType> SpatialTreeType;
  typedef typename SpatialTreeType::Child Child;
  typedef typename SpatialTreeType::BoundType BoundType;

  /**
   * A callback function for visiting a KD-tree node. If this function
   * returns true the traversal will continue. Otherwise Front2Back()
   * will stop.
   * <p>
   * This function is itself responsible for calling Distribute() on
   * the given treenode to ensure that the objects in this node
   * are properly distributed to the children. If the function doesn't
   * want or need this functionality it doesn't have to do Distribute().
   * <p>
   * If this function decides to process the given node then it is
   * also responsible for checking the timestamp of every child in this
   * node with the timestamp given to this function. If this timestamp
   * is different the child has not been processed before. This function
   * should then update the timestamp of the child. If this is not done
   * then some objects will be encountered multiple times. In some
   * cases this may not be a problem or even desired.
   * <p>
   * 'frustum_mask' can be modified by this function to reduce the number
   * of plane tests (for frustum culling) that have to occur for children
   * of this node.
   */
  typedef bool (VisitFunc)(Self* treenode, void* userdata,
	  uint32 timestamp, uint32& frustum_mask);

private:
  // One of CS_KDTREE_AXIS?
  int splitAxis;
  // Where is the split?
  float splitLocation;

  // Current timestamp we are using for Front2Back(). Objects that
  // have the same timestamp are already visited during Front2Back().
  uint32 globalTimestamp;

  /**
   * Unlink an object from the kd-tree. The 'Child' instance
   * will NOT be deleted.
   */
  void UnlinkObject(Child* object)
  {
    // remove this object from all leaves it belongs to
    for(int i = 0; i < object->GetLeafCount(); ++i)
    {
      // get one of the leaves
      Self* leaf = static_cast<Self*>(object->GetLeaf(i));

      // check it actually knows about the object
      int idx = leaf->FindObject(object);

      // ensure the object is known to the leaf
      CS_ASSERT(idx != -1);

      // remove the object from the leaf
      leaf->RemoveObject(idx);

      // reduce distribution block on leaf
      if(leaf->block > 0)
	--leaf->block;
    }

    // clear all leaves for the object
    object->RemoveLeaves();
  }

  /**
   * Find the best split position for a given axis. This will
   * return a good position depending on tree balancing (i.e. try
   * to have as many objects left as right) and also minimizing the
   * number of objects that are cut. It will return a quality
   * value which is 0 for very bad and positive for good. It will
   * also return the location to split in the 'split_loc' value.
   * If this function returns a negative quality this means the
   * split should not be performed at all.
   */
  long FindBestSplitLocation (int axis, float& split_loc)
  {
    // If we have only two objects we use the middle of the
    // empty space between the two if there is any.
    if(this->numObjects == 2)
    {
      // get the object bounidng boxes to attempt a split
      const csBox3& bbox0 = this->objects[0]->GetBBox();
      const csBox3& bbox1 = this->objects[1]->GetBBox();

      // check whether they objects are separable
      // test whether the first object is left of the second one
      // (small threshold to avoid a bad split location)
      float max0 = bbox0.Max(axis);
      float min1 = bbox1.Min(axis);
      if(max0 < min1-.01)
      {
	// split in the middle of the empty space
	split_loc = max0 + (min1-max0) * 0.5;

	// validate split
	CS_ASSERT(split_loc > max0 && split_loc < min1);

	// perfect seperation is a good split
	return 10.0;
      }

      // test whether the first object is right of the second one
      // (small threshold to avoid a bad split location)
      float min0 = bbox0.Min(axis);
      float max1 = bbox1.Max(axis);
      if(max1 < min0-.01)
      {
	// split in the middle of the empty space
	split_loc = max1 + (min0-max1) * 0.5;

	// validate split
	CS_ASSERT(split_loc > max1 && split_loc < min0);

	// perfect separation is a good split
	return 10.0;
      }

      // the objects cannot be separated, so there's no point in splitting
      return -1.0;
    }

    // Find minimum and maximum value along the axis.
    // allocate arrays to hold object bounds
    CS_ALLOC_STACK_ARRAY_FALLBACK(float, objectsMin, this->numObjects, 50000);
    CS_ALLOC_STACK_ARRAY_FALLBACK(float, objectsMax, this->numObjects, 50000);

    // initialize minimum and maximum
    float mina =  std::numeric_limits<float>::max();
    float maxa = -std::numeric_limits<float>::max();

    // iterate over all objects filling the arrays and updating the overall bounds
    for(int i = 0 ; i < this->numObjects ; i++)
    {
      // get object bounding box
      const csBox3& bbox = this->objects[i]->GetBBox();

      // get minimum and maximum
      float mi = bbox.Min(axis);
      float ma = bbox.Max(axis);

      // set object bounds
      objectsMin[i] = mi;
      objectsMax[i] = ma;

      // update overall bounds
      if (mi < mina)
      {
	mina = mi;
      }
      if(ma > maxa)
      {
	maxa = ma;
      }
    }

    // clamp the overall bounds to the node box as they may exceed it
    // due to objects belonging to multiple nodes
    if(mina < this->box.Min(axis))
    {
      mina = this->box.Min(axis);
    }
    if(maxa > this->box.Max(axis))
    {
      maxa = this->box.Max(axis);
    }

    // reject the split if the interval is too small
    if(fabs(mina - maxa) < 0.0001f)
    {
      return -1.0f;
    }

    // attempt to find a split
    // intialize best quality
    long best_qual = -2;

    // try a few different splits
    for(int attempt = 0; attempt < 5; ++attempt)
    {
      // we'll try to find a split in the middle of our bounds
      float a = (mina + maxa) / 2.0f;

      // categorize all objects into left and right ones,
      // so we can evaluate the split quality
      int left = 0;
      int right = 0;
      for(int i = 0; i < this->numObjects; ++i)
      {
	// check whether this object would go into the left ndoe
	if(objectsMax[i] < a-.0001)
	{
	  ++left;
	}
	// check whether this object would go into the right node
	else if(objectsMin[i] > a+.0001)
	{
	  ++right;
	}
      }

      // evaluate the split quality
      int qual;

      // if either side of the split is empty this is a bad split
      // which we shouldn't take
      if(left == 0 || right == 0)
      {
	qual = -1.0;
      }
      else
      {
	// resulting quality is based on how many objects could be
	// seperated and how balanced they are and is strictly positive
	qual = 3 * long(left + right) - long(ABS (left - right));
      }

      // update our best quality and split location if this is the best split
      // so far
      if(qual > best_qual)
      {
	best_qual = qual;
	split_loc = a;
      }

      // if the split was leftish, try a split more to the left
      if(left <= right)
      {
	maxa = a;
      }
      // else try one more to the right
      else
      {
	mina = a;
      }
    }

    // return our best split quality
    return best_qual;
  }

  /**
   * If this node is a leaf then we will split the objects currently
   * in this leaf according to the pre-filled in splitAxis
   * and splitLocation.
   */
  void DistributeLeafObjects()
  {
    // ensure we have a valid split axis
    CS_ASSERT(splitAxis >= CS_KDTREE_AXISX && splitAxis <= CS_KDTREE_AXISZ);

    // go over all objects and add them to the according child(s)
    for(int i = 0; i < this->numObjects; ++i)
    {
      // get object bounding box so we can categorize it
      const csBox3& bbox = this->objects[i]->GetBBox();

      // get upper and lower bound
      float bbox_min = bbox.Min(splitAxis);
      float bbox_max = bbox.Max(splitAxis);

      // keep track whether we already removed ourself as leaf for the object
      bool leaf_replaced = false;

      // check whether the object belongs in the left node
      // @@@NOTE: SMALL_EPSILON is used to ensure that when bbox_min
      //          is equal to bbox_max we don't get a situation where
      //          both of the if's are not used.
      if(bbox_min-SMALL_EPSILON <= splitLocation)
      {
	// remove us as leaf for the object and set the child as leaf
	this->objects[i]->ReplaceLeaf(this, this->child1);

	// indicate we're already removed as leaf
	leaf_replaced = true;

	// add object to the child
	this->child1->AddObject(this->objects[i]);
      }
      // check whether the object (also) belongs to the right node
      if(bbox_max >= splitLocation)
      {
	// if we already removed ourself, simply add the other leaf
	if(leaf_replaced)
	{
	  this->objects[i]->AddLeaf(this->child2);
	}
	// else remove ourself and add the leaf
	else
	{
	  this->objects[i]->ReplaceLeaf(this, this->child2);
	  leaf_replaced = true;
	}

	// add object to the child
	this->child2->AddObject(this->objects[i]);
      }

      // ensure the object went into a child
      CS_ASSERT(leaf_replaced);
    }

    // update our object count
    this->numObjects = 0;

    // @@@TODO: Clean up objects array if there are too many objects?
    //          There should be some threshold at least.
  }

  /**
   * Traverse the tree from front to back. Every node of the
   * tree will be encountered. Returns false if traversal should stop.
   * The mask parameter is optionally used for frustum checking.
   * Front2Back will pass it to the tree nodes.
   */
  void Front2Back (const csVector3& pos, VisitFunc* func,
        void* userdata, uint32 cur_timestamp, uint32 frustum_mask)
  {
    // check whether we want to continue the traversal
    if(!func(this, userdata, cur_timestamp, frustum_mask))
    {
      // we don't, abort
      return;
    }

    // ensure we have either two or no children
    CS_ASSERT((this->child1 == nullptr) == (this->child2 == nullptr));

    // check whether we have children
    if(this->child1)
    {
      // check whether left one goes first
      if(pos[splitAxis] <= splitLocation)
      {
	// yes, continue with left, then right one
	this->child1->Front2Back(pos, func, userdata, cur_timestamp, frustum_mask);
	this->child2->Front2Back(pos, func, userdata, cur_timestamp, frustum_mask);
      }
      else
      {
	// no, continue with right, then left one
	this->child2->Front2Back(pos, func, userdata, cur_timestamp, frustum_mask);
	this->child1->Front2Back(pos, func, userdata, cur_timestamp, frustum_mask);
      }
    }
  }

  /**
   * Traverse the tree in undefined order. Every node of the
   * tree will be encountered. Returns false if traversal should stop.
   * The mask parameter is optionally used for frustum checking.
   * Front2Back will pass it to the tree nodes.
   */
  void TraverseRandom(VisitFunc* func,
        void* userdata, uint32 cur_timestamp, uint32 frustum_mask)
  {
    // check whether we want to continue the traversal
    if(!func(this, userdata, cur_timestamp, frustum_mask))
    {
      // we don't, abort
      return;
    }

    // ensure we have either two or no children
    CS_ASSERT((this->child1 == nullptr) == (this->child2 == nullptr));

    // check whether we have children
    if(this->child1)
    {
      // we do, continue traversal there
      this->child1->TraverseRandom(func, userdata, cur_timestamp, frustum_mask);
      this->child2->TraverseRandom(func, userdata, cur_timestamp, frustum_mask);
    }
  }

  /**
   * Reset timestamps of all objects in this treenode.
   */
  void ResetTimestamps()
  {
    // clear timestamps for all objects
    for(int i = 0; i < this->numObjects; ++i)
    {
      this->objects[i]->timestamp = 0;
    }

    // ensure we have either two or no children
    CS_ASSERT((this->child1 == nullptr) == (this->child2 == nullptr));

    // check whether we have children
    if(this->child1)
    {
      // also reset timestamps for children
      this->child1->ResetTimestamps();
      this->child2->ResetTimestamps();
    }
  }

  void ClearSplit()
  {
    // clear split
    splitAxis = CS_KDTREE_AXISINVALID;
  }

public:
  /// Create a new empty KD-tree.
  KDTree() :
    // split initialization
    splitAxis(CS_KDTREE_AXISINVALID),

    // global timestamp initialization
    globalTimestamp(1)
  {
  }

  /**
   * Move an object (give it a new bounding box).
   */
  void MoveObject(Child* object, BoundType const& bounds)
  {
    // ensure the object actually belongs somewhere
    CS_ASSERT(object->GetLeafCount() > 0);

    // get old box
    csBox3 oldBBox(object->GetBBox());

    // update bounds for the object
    object->SetBounds(bounds);

    // get new box
    csBox3 const& newBBox = object->GetBBox();

    // First check if the bounding box actually changed.
    csVector3 dmin = oldBBox.Min() - newBBox.Min();
    csVector3 dmax = oldBBox.Max() - newBBox.Max();
    if((dmin < .00001f) && (dmax < .00001f))
    {
      return;
    }

    // get the first leaf for the object
    Self* leaf = static_cast<Self*>(object->GetLeaf(0));

    // if the object only belongs to one leaf check whether the leaf still
    // contains the whole bounding box of the object - if yes we don't have
    // to do anything
    if(object->GetLeafCount() == 1)
    {
      if(leaf->GetNodeBBox().Contains(newBBox))
      {
	// Even after moving we are still completely inside the bounding box
	// of the current leaf.
	if (leaf->block > 0)
	  leaf->block--;
	return;
      }
    }

    // remove object from all current leaves
    UnlinkObject(object);

    // find the first parent of an old leaf that contains the new bounding box
    while(leaf->parent && !leaf->GetNodeBBox().Contains(newBBox))
    {
      leaf = leaf->parent;
    }

    // add the object to it
    leaf->AddObjectInternal(object);

    // add the target as leaf
    object->AddLeaf(leaf);
  }

  /**
   * Distribute all objects in this node to its children.
   * This may also create new children if needed. Note that this
   * will only distribute one level (this node) and will not
   * recurse into the children.
   */
  void Distribute()
  {
    // check whether there is anything to distribute and
    // whether distribution is blocked
    if(this->numObjects == 0 || this->block > 0)
    {
      // nothing to be done
      return;
    }

    // ensure we have either no childs or two childs
    CS_ASSERT((this->child1 == nullptr) == (this->child2 == nullptr));

    // if we already have childs simply distribute the objects among
    // our children
    if(this->child1)
    {
      // distribute the objects
      DistributeLeafObjects();

      // ensure nothing is left
      CS_ASSERT(this->numObjects == 0);

      // update the estimated object count
      this->estimateObjects = this->child1->GetEstimatedObjectCount()
	  + this->child2->GetEstimatedObjectCount();
    }
    // we don't have children yet, so we have to try and find a split
    // if we actually have enough objects to justify a distribution
    else if(this->numObjects > this->minSplitObjects)
    {
      // to find a split location we evaluate multiple options for each
      // axis and use the one with the best quality
      float best_split_loc;
      float split_loc_tmp;

      // start with the x axis
      int best_axis = CS_KDTREE_AXISX;
      long best_qual = FindBestSplitLocation(CS_KDTREE_AXISX, best_split_loc);

      // check y axis
      long qual = FindBestSplitLocation(CS_KDTREE_AXISY, split_loc_tmp);

      // check whether it's better than x
      if(qual > best_qual)
      {
	// it is, set it as best
	best_axis = CS_KDTREE_AXISY;
	best_qual = qual;
	best_split_loc = split_loc_tmp;
      }

      // check z axis
      qual = FindBestSplitLocation(CS_KDTREE_AXISZ, split_loc_tmp);

      // check whether it's better than x and y
      if(qual > best_qual)
      {
	// it is, set it as best
	best_axis = CS_KDTREE_AXISZ;
	best_qual = qual;
	best_split_loc = split_loc_tmp;
      }

      // check whether the best split is good enough
      if(best_qual > 0)
      {
	// it is, set it as split
	splitAxis = best_axis;
	splitLocation = best_split_loc;

	// allocate children
	this->child1 = this->TreeAlloc().Alloc();
	this->child2 = this->TreeAlloc().Alloc();

	// validate allocations
	CS_ASSERT(this->child1);
	CS_ASSERT(this->child2);

	// set us as parent
	this->child1->SetParent(this);
	this->child2->SetParent(this);

	// set object descriptor
	this->child1->SetObjectDescriptor(this->descriptor);
	this->child2->SetObjectDescriptor(this->descriptor);

	// set bounding boxes
	this->child1->box = this->GetNodeBBox();
	this->child1->box.SetMax(splitAxis, splitLocation);
	this->child2->box = this->GetNodeBBox();
	this->child2->box.SetMin(splitAxis, splitLocation);

	// distribute objects according to the split
	DistributeLeafObjects();

	// ensure all objects are distributed
	CS_ASSERT(this->numObjects == 0);

	// update estimated object count
	this->estimateObjects = this->child1->GetEstimatedObjectCount()
	  + this->child2->GetEstimatedObjectCount();
      }
      else
      {
	// bad split, block distribution
	this->block = this->blockTime;
      }
    }
  }

  /**
   * Traverse the tree in random order.
   * The mask parameter is optionally used for frustum checking.
   * TraverseRandom will pass it to the tree nodes.
   */
  void TraverseRandom(VisitFunc* func, void* userdata, uint32 frustum_mask)
  {
    NewTraversal();
    TraverseRandom(func, userdata, globalTimestamp, frustum_mask);
  }

  /**
   * Traverse the tree from front to back. Every node of the
   * tree will be encountered.
   * The mask parameter is optionally used for frustum checking.
   * Front2Back will pass it to the tree nodes.
   */
  void Front2Back(const csVector3& pos, VisitFunc* func, void* userdata, uint32 frustum_mask)
  {
    NewTraversal();
    Front2Back(pos, func, userdata, globalTimestamp, frustum_mask);
  }

  /**
   * Start a new traversal. This will basically make a new
   * timestamp and return it. You can then use that timestamp
   * to check if objects have been visited already. This function
   * is automatically called by Front2Back() but it can be useful
   * to call this if you plan to do a manual traversal of the tree.
   */
  uint32 NewTraversal()
  {
    // use the parent timestamp if we have a parent
    if(this->parent)
    {
      return this->parent->NewTraversal();
    }

    // For safety reasons we will reset all timestamps to 0
    // for all objects in the tree and also set the global
    // timestamp to 1 again every 4.000.000.000 calls of Front2Back
    if(globalTimestamp > 4000000000u)
    {
      ResetTimestamps();
      globalTimestamp = 1;
    }
    else
    {
      ++globalTimestamp;
    }
    return globalTimestamp;
  }

private:
  bool DebugCheckSplit(csString& str)
  {
    // ensure the split axis is valid
    if(!(splitAxis >= CS_KDTREE_AXISX && splitAxis <= CS_KDTREE_AXISZ))
    {
      str.AppendFmt("KDTree failure: (%d,%s): %s\n", int(__LINE__),
	"invalid split axis", "splitAxis >= CS_KDTREE_AXISX && splitAxis <= CS_KDTREE_AXISZ");
    }
    // ensure the split location is contained in the node bounding box
    else if(!(splitLocation >= this->GetNodeBBox().Min(splitAxis)))
    {
      str.AppendFmt("KDTree failure: (%d,%s): %s\n", int(__LINE__),
	"invalid split", "splitLocation >= GetNodeBBox().Min(splitAxis)");
    }
    // ensure the split location is contained in the node bounding box
    else if(!(splitLocation <= this->GetNodeBBox().Max(splitAxis)))
    {
      str.AppendFmt("KDTree failure: (%d,%s): %s\n", int(__LINE__),
	"invalid split", "splitLocation <= GetNodeBBox().Max(splitAxis)");
    }
    else
    {
      return true;
    }
    return false;
  }

  void DebugDumpSplit(csString& str, csString const& indent)
  {
    // ensure axis is valid
    CS_ASSERT(splitAxis >= CS_KDTREE_AXISX && splitAxis <= CS_KDTREE_AXISZ);

    // append our split split data
    char axis[3] = {'x','y','z'};
    str.AppendFmt("%s   axis=%c loc=%g\n",
	indent.GetData(), axis[splitAxis], splitLocation);
  }
};

} // namespace Geometry
} // namespace CS

typedef CS::Geometry::SpatialTreeChild::BoxChild<true> csKDTreeChild;
typedef CS::Geometry::KDTree<csKDTreeChild> csKDTree;
typedef csKDTree::VisitFunc csKDTreeVisitFunc;
typedef csKDTree::iObjectDescriptor iKDTreeObjectDescriptor;
typedef csKDTree::iUserData iKDTreeUserData;

/** @} */

#endif // __CS_KDTREE_H__

