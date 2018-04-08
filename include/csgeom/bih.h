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

#ifndef __CS_BIH_H__
#define __CS_BIH_H__

#include "csgeom/spatialtree.h"

#include "csutil/scf_implementation.h"

/**\file
 * Bounding Intervall Hierachy implementation.
 */
/**\addtogroup geom_utils
 * @{ */

namespace CS
{
namespace Geometry
{
enum
{
  CS_BIH_AXISINVALID = -1,
  CS_BIH_AXISX = 0,
  CS_BIH_AXISY = 1,
  CS_BIH_AXISZ = 2
};

/**
 * A Bounding Intervall Hierachy (BIH).
 * A BIH is a binary tree that organizes 3D space.
 * This implementation is dynamic. It allows moving, adding, and
 * removing of objects which will alter the tree dynamically.
 * The main purpose of this tree is to provide fast approximate
 * front to back ordering.
 * <p>
 * The BIH supports delayed insertion. When objects are inserted
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
class BIH :
  public SpatialTree<BIH<ChildType>, ChildType>
{
  friend class SpatialTree<BIH<ChildType>, ChildType>;
public:
  // convenience typedefs
  typedef BIH<ChildType> Self;
  typedef SpatialTree<Self, ChildType> SpatialTreeType;
  typedef typename SpatialTreeType::Child Child;
  typedef typename SpatialTreeType::BoundType BoundType;

  /**
   * A callback function for visiting a BIH node. If this function
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
  typedef float (Split)[4];

  // data for our split if we have one
  int splitAxis; // One of CS_KDTREE_AXIS
  Split split;    // upper/lower bounds for both childs (may overlap)

  // blocking value - if a split is of bad quality we may block for
  // a number of addiditions/removals to disallow further split attempts
  float blockThreshold; // minimum quality that'll be accepted

  // helper structure used to find split locations
  struct SortElement
  {
    float min;
    float max;
    //float size;
    int idx;

    static int compareMin(SortElement const& l, SortElement const& r)
    {
      if(l.min < r.min) return -1;
      else if(l.min > r.min) return 1;
      else return 0;
    }

    static int compareMax(SortElement const& l, SortElement const& r)
    {
      if(l.max < r.max) return -1;
      else if(l.max > r.max) return 1;
      else return 0;
    }
  };

  /**
   * Find the best split position for a given axis. This will
   * return good positions depending on tree balancing (i.e. try
   * to have same amount of objects left and right) and also
   * maximizing the space cutoff (i.e. the amount of empty space
   * that can be neglected after this split).
   * It will return the amount of cutoff (positive is generally good,
   * negative bad - the bigger, the better) and the locations for the
   * split in the split value.
   */
  template<int axis, bool low, bool first>
  void FindBestSplitLocation(float& bestQuality, SortElement* (&buffers)[2])
  {
    // validate storage
    CS_ASSERT(this->objects && this->numObjects <= this->maxObjects);

    // get our working buffer
    SortElement* set = buffers[0];

    // fill sort array
    for(int i = 0; i < this->numObjects; ++i)
    {
      SortElement& s = set[i];
      s.min = this->objects[i]->GetMin(axis);
      s.max = this->objects[i]->GetMax(axis);
      s.idx = i;
    }

    // check simple partitioning
    // sort by according bound
    if(low)
    {
      qsort(set, this->numObjects, sizeof(SortElement), (int (*)(void const*, void const*))SortElement::compareMin);
    }
    else
    {
      qsort(set, this->numObjects, sizeof(SortElement), (int (*)(void const*, void const*))SortElement::compareMax);
    }

    // create our split
    Split split;

    // partition in the middle - grab according bounds from sorted list
    if(low)
    {
      split[2] = set[0].min;
      split[0] = set[this->numObjects >> 1].min;
    }
    else
    {
      split[1] = set[(this->numObjects >> 1) - 1].max;
      split[3] = set[this->numObjects - 1].max;
    }

    // grab the other bounds
    if(low)
    {
      // get upper bound for lower part
      split[3] = -std::numeric_limits<float>::max();
      for(int i = 0; i < (this->numObjects >> 1); ++i)
      {
	split[3] = csMax(split[3], set[i].max);
      }

      // get upper bound for higher part
      split[1] = -std::numeric_limits<float>::max();
      for(int i = this->numObjects >> 1; i < this->numObjects; ++i)
      {
	split[1] = csMax(split[1], set[i].max);
      }
    }
    else
    {
      // get lower bound for lower part
      split[0] = std::numeric_limits<float>::max();
      for(int i = 0; i < (this->numObjects >> 1); ++i)
      {
	split[0] = csMin(split[0], set[i].min);
      }

      // get lower bound for upper part
      split[2] = std::numeric_limits<float>::max();
      for(int i = this->numObjects >> 1; i < this->numObjects; ++i)
      {
	split[2] = csMin(split[2], set[i].min);
      }
    }

    // check if second interval is contained in first - if so swap them
    if(split[0] <= split[2] && split[1] >= split[3])
    {
      std::swap(split[0], split[2]);
      std::swap(split[1], split[3]);
    }

    // calculate quality
    float quality = split[2] - split[1]; // center cutoff - ignore border cutoff

    // check whether our split is better than previous ones
    // if there are any - if so set it as best split
    if(first || quality > bestQuality)
    {
      // set our quality as best
      bestQuality = quality;

      // set our split axis
      splitAxis = axis;

      // swap buffers
      buffers[0] = buffers[1];
      buffers[1] = set;

      // set our split as best one
      for(int i = 0; i < 4; ++i)
      {
	this->split[i] = split[i];
      }
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

    // split axis must be valid
    CS_ASSERT(splitAxis == CS_BIH_AXISX || splitAxis == CS_BIH_AXISY || splitAxis == CS_BIH_AXISZ);

    // split values must be normal
    for(int i = 0; i < 4; ++i)
      CS_ASSERT(CS::IsFinite(split[i]) && !CS::IsNaN(split[i]));

    // lower and upper split must both contain something
    CS_ASSERT(split[0] < split[1] && split[2] < split[3]);

    // if one split completely contains the other the contained one must be the first one
    CS_ASSERT(!(split[0] <= split[2] && split[1] >= split[3]));

    // keep track how many objects couldn't be distributed
    int failed = 0;

    // distribute objects
    for(int i = 0; i < this->numObjects; ++i)
    {
      Child* obj = this->objects[i];
      float min = obj->GetMin(splitAxis);
      float max = obj->GetMax(splitAxis);

      // check whether it belongs to child1
      if(min >= split[0] && max <= split[1])
      {
	// add object to child1
	this->child1->AddObject(obj);

	// change leaf
	obj->ReplaceLeaf(this, this->child1);
      }
      // else it belongs to child2
      else if(min >= split[2] && max <= split[3])
      {
	// add object to child2
	this->child2->AddObject(obj);

	// change leaf
	obj->ReplaceLeaf(this, this->child2);
      }
      else
      {
	// keep it, but put it in front of the array
	// so we can truncate at the end
	this->objects[failed] = obj;
	++failed;
      }
    }

    // update object count for our node (this is equivalent
    // to a truncation of our array)
    this->numObjects = failed;
  }

  /// Make the tree empty.
  void ClearSplit()
  {
    // invalidate split
    splitAxis = CS_BIH_AXISINVALID;
  }

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

public:
  /// Create a new empty BIH.
  BIH() :
    // split initialization
    splitAxis(CS_BIH_AXISINVALID), blockThreshold(0.0f)
  {
  }

  virtual ~BIH()
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

      // add ancestor as leaf to the object
      obj->AddLeaf(tree);
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
    CS_ASSERT(
	 ((this->child1 == nullptr) && (this->child2 == nullptr) && (splitAxis == CS_BIH_AXISINVALID))
      || ((this->child1 != nullptr) && (this->child2 != nullptr) && (splitAxis != CS_BIH_AXISINVALID))
      );

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
      // check that we are not leaf - leaves must have objects
      CS_ASSERT((splitAxis != CS_BIH_AXISINVALID) || (this->parent == nullptr));
      return;
    }

    // check whether we already have a split for this node
    if(splitAxis == CS_BIH_AXISINVALID) // nope
    {
      // do we have enough objects for a new split?
      if(this->numObjects < this->minSplitObjects)
      {
	// nope, nothing to be done
	return;
      }
      else
      {
	// time for some fun - find the axis with the best split

	// keep track of our best quality
	float quality;

	// allocate buffers to hold the split
	SortElement* buffers[2];
	buffers[0] = static_cast<SortElement*>(cs_malloc(sizeof(SortElement)*this->numObjects));
	buffers[1] = static_cast<SortElement*>(cs_malloc(sizeof(SortElement)*this->numObjects));

	// validate allocations
	CS_ASSERT(buffers[0]);
	CS_ASSERT(buffers[1]);

	// evaluate all splits
	FindBestSplitLocation<CS_BIH_AXISX, true, true>(quality, buffers);   // low  X
	FindBestSplitLocation<CS_BIH_AXISX, false, false>(quality, buffers); // high X
	FindBestSplitLocation<CS_BIH_AXISY, true, false>(quality, buffers);  // low  Y
	FindBestSplitLocation<CS_BIH_AXISY, false, false>(quality, buffers); // high Y
	FindBestSplitLocation<CS_BIH_AXISZ, true, false>(quality, buffers);  // low  Z
	FindBestSplitLocation<CS_BIH_AXISZ, false, false>(quality, buffers); // high Z

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
	    // get object
	    Child* object = this->objects[buffers[1][i].idx];

	    // add object to child
	    this->child1->AddObject(object);

	    // replace leaf
	    object->ReplaceLeaf(this, this->child1);
	  }

	  // second child gets upper half
	  for(int i = this->numObjects >> 1; i < this->numObjects; ++i)
	  {
	    // get object
	    Child* object = this->objects[buffers[1][i].idx];

	    // add object to child
	    this->child2->AddObject(object);

	    // replace leaf
	    object->ReplaceLeaf(this, this->child2);
	  }

	  // truncate our object list as we distributed all childs
	  this->numObjects = 0;

	  // update their boxes so they know where they belong
	  // first child gets cut according to first interval
	  this->child1->box = this->box;
	  this->child1->box.SetMin(splitAxis, split[0]);
	  this->child1->box.SetMax(splitAxis, split[1]);
	  // second child gets cut according to second interval
	  this->child2->box = this->box;
	  this->child2->box.SetMin(splitAxis, split[2]);
	  this->child2->box.SetMax(splitAxis, split[3]);

	  // update our estimated object count
	  this->estimateObjects = this->child1->numObjects + this->child2->numObjects;
	}
	else
	{
	  // ok, this "winner" was crap - let's wait a bit before we try again
	  // it's tiring to run tournaments, you know
	  this->block = this->blockTime;
	  splitAxis = CS_BIH_AXISINVALID;
	}

        // free our buffers as we don't need them anymore
	cs_free(buffers[0]);
	cs_free(buffers[1]);
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

    // ensure our storage is valid
    CS_ASSERT(this->objects != nullptr && this->maxObjects >= this->numObjects);
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

    // check whether we have childs to traverse
    if(splitAxis != CS_BIH_AXISINVALID)
    {
      // ensure childs exist
      CS_ASSERT(this->child1);
      CS_ASSERT(this->child2);

      // traverse childs
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

    // check whether we have childs to traverse
    if(splitAxis != CS_BIH_AXISINVALID)
    {
      // ensure childs exist
      CS_ASSERT(this->child1);
      CS_ASSERT(this->child2);

      // traverse first child first if it is part of the interval
      if(pos[splitAxis] < split[1])
      {
	this->child1->Front2Back(pos, func, data, frustumMask);
	this->child2->Front2Back(pos, func, data, frustumMask);
      }
      // else go for the second one first
      else
      {
	this->child2->Front2Back(pos, func, data, frustumMask);
	this->child1->Front2Back(pos, func, data, frustumMask);
      }
    }
  }

private:
  // debugging functions
  bool DebugCheckSplit(csString& str)
  {
    // ensure the split axis is valid
    if(!(splitAxis >= CS_BIH_AXISX && splitAxis <= CS_BIH_AXISZ))
    {
      str.AppendFmt("BIH failure: (%d,%s): %s\n", int(__LINE__),
	"invalid split axis", "splitAxis >= CS_KDTREE_AXISX && splitAxis <= CS_KDTREE_AXISZ");
    }
    // ensure the split location is contained in the node bounding box
    else if(!(
      split[0] >= this->GetNodeBBox().Min(splitAxis) &&
      split[1] >= this->GetNodeBBox().Min(splitAxis) &&
      split[2] >= this->GetNodeBBox().Min(splitAxis) &&
      split[3] >= this->GetNodeBBox().Min(splitAxis) 
      ))
    {
      str.AppendFmt("BIH failure: (%d,%s): %s\n", int(__LINE__),
	"invalid split", "split >= GetNodeBBox().Min(splitAxis)");
    }
    // ensure the split location is contained in the node bounding box
    else if(!(
      split[0] <= this->GetNodeBBox().Max(splitAxis) &&
      split[1] <= this->GetNodeBBox().Max(splitAxis) &&
      split[2] <= this->GetNodeBBox().Max(splitAxis) &&
      split[3] <= this->GetNodeBBox().Max(splitAxis) 
      ))
    {
      str.AppendFmt("BIH failure: (%d,%s): %s\n", int(__LINE__),
	"invalid split", "splitLocation <= GetNodeBBox().Max(splitAxis)");
    }
    else if(!(split[0] <= split[1] && split[2] <= split[3]))
    {
      str.AppendFmt("BIH failure: (%d,%s): %s\n", int(__LINE__),
	"invalid split", "split[0] <= split[1] && split[2] <= split[3]");
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
    CS_ASSERT(splitAxis >= CS_BIH_AXISX && splitAxis <= CS_BIH_AXISZ);

    // append our split split data
    char axis[3] = {'x','y','z'};
    str.AppendFmt("%s   axis=%c left=(%g - %g) right=(%g - %g)\n",
	indent.GetData(), axis[splitAxis], split[0], split[1], split[2], split[3]);
  }
};

} // namespace Geometry
} // namespace CS

typedef CS::Geometry::SpatialTreeChild::BoxChild<false> csBIHChild;
typedef CS::Geometry::BIH<csBIHChild> csBIH;

/** @} */

#endif // __CS_BIH_H__

