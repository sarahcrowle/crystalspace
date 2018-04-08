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

#ifndef __CS_SPATIAL_TREE_H__
#define __CS_SPATIAL_TREE_H__

#include "csgeom/box.h"
#include "csgeom/sphere.h"

#include "csutil/blockallocator.h"
#include "csutil/ref.h"
#include "csutil/scfstr.h"
#include "csutil/floatrand.h"
#include "csutil/metautils.h"

#include <limits>

namespace CS
{
namespace Geometry
{
  namespace SpatialTreeChild
  {
    class ChildBase
    {
    private:
      // Pointer back to the original object.
      void* object;

    public:
      uint32 timestamp;

      // create new child base with no object
      ChildBase() : object(nullptr)
      {
      }

      /**
       * Set the pointer to the black box object.
       */
      inline void SetObject(void* obj)
      {
	object = obj;
      }

      /**
       * Get the pointer to the black box object.
       */
      inline void* GetObject() const
      {
	return object;
      }
    };

    class SingleLeafChild : public ChildBase
    {
    private:
      void* leaf; // pointer to the tree holding this child

    public:
      // leaf setter/getter
      inline void* GetLeaf() const
      {
	return leaf;
      }

      inline void AddLeaf(void* l)
      {
	leaf = l;
      }

      inline void ReplaceLeaf(void* oldLeaf, void* newLeaf)
      {
	CS_ASSERT(leaf == oldLeaf);

	leaf = newLeaf;
      }

      bool DebugCheck(csString& str, void* parent)
      {
	if(leaf != parent)
	{
	  str.AppendFmt("SingleLeafChild failure: (%d,%s)\n", int(__LINE__),
	    "leaf doesn't match parent");
	  return false;
	}
	return true;
      }
    };

    class MultiLeafChild : public ChildBase
    {
    private:
      // leaves we belong to
      void** leaves;

      // amount of leaves we're holding
      int leafCount;

      // amount of leaves we can hold
      int maxLeafCount;

    public:
      MultiLeafChild() :
	leaves(nullptr), leafCount(0), maxLeafCount(0)
      {
      }

      ~MultiLeafChild()
      {
	cs_free(leaves);
      }

      /// Physically add a leaf to this child.
      inline void AddLeaf(void* leaf)
      {
	// validate storage
	CS_ASSERT((maxLeafCount == 0) == (leaves == nullptr));

	// check whether we have to grow the storage
	if(leafCount >= maxLeafCount)
	{
	  // get new storage size
	  maxLeafCount += 3;

	  // relocate storage
	  void** relocation = static_cast<void**>(
	    cs_realloc(leaves, sizeof(void*)*maxLeafCount));

	  // validate relocation
	  CS_ASSERT(relocation);

	  // update storage location
	  leaves = relocation;
	}

	leaves[leafCount++] = leaf;
      }

      /// Physically remove a leaf from this child.
      inline void RemoveLeaf(int idx)
      {
	// ensure index is valid
	CS_ASSERT(idx >= 0 && idx < leafCount);

	// validate storage
	CS_ASSERT((maxLeafCount == 0) == (leaves == nullptr));

	// indicate we drop an element
	--leafCount;

	// if the element to remove is not the last element
	// put the last element where the one to remove was
	if(idx < leafCount)
	{
	  leaves[idx] = leaves[leafCount];
	}
      }

      /// Physically remove a leaf from this child.
      inline void RemoveLeaf(void* leaf)
      {
	RemoveLeaf(FindLeaf(leaf));
      }

      /// Remove all leaves from this child.
      inline void RemoveLeaves()
      {
	// simply truncate leaves
	leafCount = 0;
      }

      /**
       * Replace a leaf with another one. This is more
       * efficient than doing RemoveLeaf/AddLeaf and it is
       * useful in many cases where you want to move a child
       * in the tree.
       */
      inline void ReplaceLeaf(void* oldLeaf, void* newLeaf)
      {
	// find the leaf to replace
	int idx = FindLeaf(oldLeaf);

	// ensure it was found
	CS_ASSERT(idx >= 0);

	// replace it
	leaves[idx] = newLeaf;
      }

      /**
       * Find leaf.
       */
      inline int FindLeaf(void* leaf)
      {
	// validate storage
	CS_ASSERT((maxLeafCount == 0) == (leaves == nullptr));

	// go over all elements and check whether we found it
	for(int i = 0; i < leafCount; ++i)
	{
	  if(leaves[i] == leaf)
	  {
	    // return current index
	    return i;
	  }
	}

	// return negative index to indicate a failure
	return -1;
      }

      // Get number of leaves this child belongs to.
      inline int GetLeafCount() const
      {
	return leafCount;
      }

      // Get a leaf this child belongs to.
      inline void* GetLeaf(int idx = 0) const
      {
	// ensure index is valid
	CS_ASSERT(idx >= 0 && idx < leafCount);

	// validate storage
	CS_ASSERT((maxLeafCount == 0) == (leaves == nullptr));
	CS_ASSERT(leafCount <= maxLeafCount);

	// return requested leaf
	return leaves[idx];
      }

      bool DebugCheck(csString& str, void* leaf)
      {
	// ensure the parent only occurs once in the list
	int count = 0;
	// check all leaves this object is associated with
	for(int i = 0; i < leafCount; ++i)
	{
	  // check whether this is the parent
	  if(leaves[i] == leaf)
	  {
	    // we are in the leaf list
	    ++count;

	    // ensure we didn't already occur earlier in the list
	    if(count > 1)
	    {
	      str.AppendFmt("MultiLeafChild failure: (%d,%s)\n", int(__LINE__),
		"parent occured multiple times");
	      return false;
	    }
	  }
	}
	if(count < 1)
	{
	  str.AppendFmt("MultiLeafChild failure: (%d,%s)\n", int(__LINE__),
	    "leaf list doesn't contain parent");
	  return false;
	}
	return true;
      }
    };

    template<bool multiLeaf>
    class BoxChild :
      public CS::Meta::TypeSelect<multiLeaf, MultiLeafChild, SingleLeafChild>::Type,
      public csBox3
    {
    public:
      // used by tree implementation
      typedef csBox3 BoundType;

      // set user-supplied boundaries
      inline void SetBounds(BoundType const& bounds)
      {
	Set(bounds.Min(), bounds.Max());
      }

      // create a random bound for benchmarking purposes
      static BoundType RandomBound()
      {
	static csRandomFloatGen rng;
	float x = rng.Get(-50,50);
	float y = rng.Get(-50,50);
	float z = rng.Get(-50,50);
	return BoundType(x, y, z, rng.Get(x, x+7),
	  rng.Get(y, y+7), rng.Get(z, z+7));
      }

      /**
       * Get the bounding box of this object.
       */
      inline csBox3 const& GetBBox() const
      {
	return *this;
      }
    };

    template<bool multiLeaf>
    class SphereChild :
      public CS::Meta::TypeSelect<multiLeaf, MultiLeafChild, SingleLeafChild>::Type,
      public csSphere
    {
    private:
      csBox3 box;

    public:
      // used by implementation
      typedef csSphere BoundType;

      // set user-supplied boundaries
      inline void SetBounds(BoundType const& bounds)
      {
	// copy from other sphere
	SetCenter(bounds.GetCenter());
	SetRadius(bounds.GetRadius());

	// update our bounding box
	box.Set(bounds.GetCenter() - csVector3(bounds.GetRadius()),
		bounds.GetCenter() + csVector3(bounds.GetRadius()));
      }

      // create a random bound for benchmarking purposes
      static BoundType RandomBound()
      {
	static csRandomFloatGen rng;
	return BoundType(
	  csVector3(rng.Get(-50,50), rng.Get(-50,50), rng.Get(-50,50)),
	  rng.Get(0,7));
      }

      /**
       * Get the bounding box of this object.
       */
      inline csBox3 const& GetBBox() const
      {
	return box;
      }
    };
  }

  template<class Self, class ChildType>
  class SpatialTree :
    public scfImplementation1<SpatialTree<Self, ChildType>, iDebugHelper>
  {
  public:
    typedef ChildType Child;
    typedef typename Child::BoundType BoundType;
    typedef SpatialTree<Self, Child> SpatialTreeType;

    /**
     * The data type for user data to be attached to the KDTree.
     * It provides no functions but makes it possible to do a direct cast
     * for performance instead of doing an scfQueryInterface.
     */
    struct iUserData : public virtual iBase
    {
    };

    /**
     * If you implement this interface then you can give that to the
     * KDtree. The KDtree can then use this to find the description of an object.
     * This can be used for debugging as the KDtree will print out that description
     * if it finds something is wrong.
     */
    struct iObjectDescriptor : public virtual iBase
    {
      virtual csPtr<iString> DescribeObject(Child* child) = 0;
    };

  private:
    // allocators
    csBlockAllocator<Child>* childAlloc;
    csBlockAllocator<Self>* treeAlloc;

    // An optional user object for this node.
    csRef<iUserData> userObject;

  protected:
    // description of this node - used for debugging
    csRef<iObjectDescriptor> descriptor;

    // childs of this branch
    // either both of those are valid (this is a branch)
    // or both of them are nullptr (this is a leaf)
    Self* child1;
    Self* child2;
    // the parent node in the tree - nullptr if this is the root
    Self* parent;

    // bounding box box of the node itself
    csBox3 box;

    // Objects in this node. If this node also has children (child1
    // and child2) then the objects here have to be moved to these
    // children. The 'Distribute()' function will do that.
    Child** objects;     // storage
    int numObjects;      // current storage used
    int maxObjects;      // current storage capacity
    int estimateObjects; // estimated number of objects in whole tree
    enum { objectGrowth = 80 };

    // Minimum amount of objects in this tree before we consider splitting.
    int minSplitObjects;

    // Disallow Distribute().
    // If this flag > 0 it means that we cannot find a good split
    // location for the current list of objects. So in that case we don't
    // split at all and set this flag to DISALLOW_DISTRIBUTE_TIME so
    // that we will no longer attempt to distribute for a while. Whenever
    // objects are added or removed to this node this flag will be decreased
    // so that when it becomes 0 we can make a new Distribute() attempt can
    // be made. This situation should be rare though.
    int block;
    static int const blockTime = 20; // maybe set this based on minSplitObjects?

    // get tree allocator
    inline csBlockAllocator<Self>& TreeAlloc()
    {
      if(treeAlloc == nullptr)
      {
	if(parent == nullptr)
	{
	  treeAlloc = new csBlockAllocator<Self>();
	}
	else
	{
	  treeAlloc = &parent->TreeAlloc();
	}
      }
      return *treeAlloc;
    }

    // get child allocator
    inline csBlockAllocator<Child>& ChildAlloc()
    {
      if(childAlloc == nullptr)
      {
	if(parent == nullptr)
	{
	  childAlloc = new csBlockAllocator<Child>();
	}
	else
	{
	  childAlloc = &parent->ChildAlloc();
	}
      }
      return *childAlloc;
    }

    /// Set parent of this tree node.
    inline void SetParent(Self* p)
    {
      parent = p;
    }

    /// Add an object to this tree node.
    void AddObjectInternal(Child* obj)
    {
      CS_ASSERT(obj);

      if(block > 0)
      {
	--block;
      }

      AddObject(obj);
    }

    /// Physically add a child to this tree node.
    void AddObject(Child* obj)
    {
      // validate obejct
      CS_ASSERT(obj);

      // validate storage
      CS_ASSERT((maxObjects == 0) == (objects == nullptr));

      // check whether we have to grow the storage
      if(numObjects >= maxObjects)
      {
	// get new storage size
	maxObjects += csMin(maxObjects+2, static_cast<int>(objectGrowth));

	// relocate storage
	Child** relocation = static_cast<Child**>(cs_realloc(objects, sizeof(Child*)*maxObjects));

	// validate relocation
	CS_ASSERT(relocation);

	// update storage location
	objects = relocation;
      }

      // add object
      objects[numObjects] = obj;

      // update object counts
      ++numObjects;
      ++estimateObjects;
    }

    /// Physically remove a child from this tree node.
    void RemoveObject(int idx)
    {
      CS_ASSERT(objects);
      CS_ASSERT(idx >= 0 && idx < this->numObjects);

      // update object counts
      --numObjects;
      --estimateObjects;

      // if the element to remove is not the last element
      // put the last element where the one to remove was
      if(idx < numObjects)
      {
	objects[idx] = objects[this->numObjects];
      }
    }

    /// Find an object. Returns -1 if not found.
    int FindObject(Child* obj)
    {
      CS_ASSERT(obj);
      CS_ASSERT((objects == nullptr) == (maxObjects == 0));
      CS_ASSERT(numObjects <= maxObjects);

      // iterate over all objects
      for(int i = 0; i < this->numObjects; ++i)
      {
	// check whether we found the object
	if(objects[i] == obj)
	{
	  // we did - return it's index
	  return i;
	}
      }

      // couldn't locate object
      return -1;
    }

    /**
     * Flatten the children of this node to the given node.
     */
    void FlattenTo(Self* node)
    {
      // check we aren't trying to flatten ourselves
      CS_ASSERT(this != node);

      // ensure we have either two or no children
      CS_ASSERT((child1 == nullptr) == (child2 == nullptr));

      // check whether we're a leaf
      if(child1)
      {
	// flatten childs
	child1->FlattenTo(node);
	child2->FlattenTo(node);
      }

      // check the storage is valid
      CS_ASSERT((maxObjects == 0) == (objects == nullptr));
      CS_ASSERT(numObjects <= maxObjects);

      // add our own (undistributed) objects to the target
      for(int i = 0; i < numObjects; ++i)
      {
	// get object
	Child* object = objects[i];

	// add object to target
	node->AddObject(object);

	// change leaf accordingly
	object->ReplaceLeaf(static_cast<Self*>(this), node);
      }

      // free our storage
      cs_free(objects);
      objects = nullptr;
      maxObjects = 0;
      numObjects = 0;
    }

  public:
    /// Create empty spatial tree.
    SpatialTree() :
      // scf initialization
      scfImplementation1<SpatialTree<Self, Child>, iDebugHelper>(this),

      // allocator initialization
      childAlloc(nullptr), treeAlloc(nullptr),

      // child-parent initialization
      child1(nullptr), child2(nullptr), parent(nullptr),

      // box initialization
      box(-std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),
	  std::numeric_limits<float>::max(),std::numeric_limits<float>::max(),std::numeric_limits<float>::max()),

      // objects initialization
      objects(nullptr), numObjects(0), maxObjects(0), estimateObjects(0),

      // split initialization
      minSplitObjects(10), block(0)
    {
    }

    /// Destroy the tree.
    ~SpatialTree()
    {
      Clear();
    }

    /// Get parent of this tree node.
    inline Self* GetParent() const
    {
      return parent;
    }

    /**
     * Set the minimum amount of objects before we consider splitting this tree.
     * By default this is set to 10.
     */
    inline void SetMinimumSplitAmount(int m)
    {
      minSplitObjects = m;
    }

    /// Make the tree empty.
    void Clear()
    {
      // free first child
      TreeAlloc().Free(child1);
      child1 = nullptr;

      // free second child
      TreeAlloc().Free(child2);
      child2 = nullptr;

      // free objects
      for(int i = 0; i < numObjects; ++i)
      {
	ChildAlloc().Free(objects[i]);
      }
      numObjects = 0;
      estimateObjects = 0;

      // free object storage
      cs_free(objects);
      objects = nullptr;
      maxObjects = 0;

      // reset blocking state
      block = 0;

      // invalidate user data
      userObject.Invalidate();

      // check if we're the root node
      if(parent == nullptr)
      {
	// delete tree allocator
	delete treeAlloc;
	treeAlloc = nullptr;

	// delete child allocator
	delete childAlloc;
	childAlloc = nullptr;
      }

      // clear split info
      static_cast<Self*>(this)->ClearSplit();
    }

    /// Get the user object attached to this node.
    inline iUserData* GetUserObject() const
    {
      return userObject;
    }

    /**
     * Set the user object for this node. Can be 0 to clear
     * it. The old user object will be DecRef'ed and the (optional)
     * new one will be IncRef'ed.
     */
    inline void SetUserObject(iUserData* obj)
    {
      userObject = obj;
    }

    /// For debugging: set the object descriptor.
    inline void SetObjectDescriptor(iObjectDescriptor* d)
    {
      descriptor = d;
    }

    /**
     * Add an object to the tree.
     * Returns a Child pointer which represents the object
     * inside the tree. Object addition is delayed. This function
     * will not yet alter the structure of the tree. Distribute()
     * will do that.
     */
    Child* AddObject(BoundType const& bounds, void* object)
    {
      // validate the object is valid
      CS_ASSERT(object);

      // allocate tree child to hold this object
      Child* obj = ChildAlloc().Alloc();

      // set boundaries
      obj->SetBounds(bounds);

      // validate the object has valid boundaries
  #   ifdef CS_DEBUG
      for(int i = 0; i < 3; ++i)
      {
	// validate minimum is normal
	float min = obj->GetBBox().GetMin(i);
	CS_ASSERT(CS::IsFinite(min) && !CS::IsNaN(min));

	// validate maximum is normal
	float max = obj->GetBBox().GetMax(i);
	CS_ASSERT(CS::IsFinite(max) && !CS::IsNaN(max));

	// validate minimum is smaller than maximum
	CS_ASSERT(min <= max);
      }
  #   endif

      // set object
      obj->SetObject(object);

      // add child to the tree
      AddObjectInternal(obj);

      // let it know we're it's leaf
      obj->AddLeaf(static_cast<Self*>(this));

      return obj;
    }

    /**
     * Remove an object from the tree. The 'Child' instance
     * will be deleted.
     */
    void RemoveObject(Child* obj)
    {
      // unlink object from tree
      static_cast<Self*>(this)->UnlinkObject(obj);

      // free object
      ChildAlloc().Free(obj);
    }

    /**
     * Do a full distribution of this node and all children.
     */
    inline void FullDistribute()
    {
      // distribute our objects
      static_cast<Self*>(this)->Distribute();

      // ensure we got two children or none
      CS_ASSERT((child1 == nullptr) == (child2 == nullptr));

      // check whether we're a leaf (i.e. distribution succeeded)
      if(child1)
      {
	// distribute child objects
	child1->FullDistribute();
	child2->FullDistribute();
      }
    }

    /**
     * Do a full flatten of this node. This means that all
     * objects are put back in the object list of this node and
     * the children are removed.
     * Calling this on a leaf results in undefined behaviour.
     */
    void Flatten()
    {
      // validate childs
      CS_ASSERT(child1);
      CS_ASSERT(child2);

      // flatten childs
      child1->FlattenTo(static_cast<Self*>(this));
      child2->FlattenTo(static_cast<Self*>(this));

      // delete childs
      TreeAlloc().Free(child1);
      child1 = nullptr;
      TreeAlloc().Free(child2);
      child2 = nullptr;

      // reset split
      static_cast<Self*>(this)->ClearSplit();
    }


    /**
     * Get first child.
     */
    inline Self* GetChild1() const
    {
      return child1;
    }

    /**
     * Get second child.
     */
    inline Self* GetChild2() const
    {
      return child2;
    }

    /**
     * Return the number of objects in this node.
     */
    inline int GetObjectCount() const
    {
      return numObjects;
    }

    /**
     * Get the estimated total number of objects in this node and
     * all children. This is only an estimate as it isn't kept up-to-date
     * constantly but it should give a rough idea about the complexity
     * of this node.
     */
    inline int GetEstimatedObjectCount() const
    {
      return estimateObjects;
    }

    /**
     * Return the array of objects in this node.
     */
    inline Child** GetObjects() const
    {
      return objects;
    }

    /**
     * Return the bounding box of the node itself (includes all children).
     */
    inline csBox3 const& GetNodeBBox() const
    {
      return box;
    }

    // debugging functions
  private:
      // debugging functions
    // perform various sanity checks on the tree for validation
    bool DebugCheckTreeTraversal (csString& str)
    {
#     define SPATIALTREE_TEST(test,msg) \
      if(!(test)) \
      { \
	str.AppendFmt("SpatialTree failure (%d,%s): %s\n", int(__LINE__), \
	    #msg, #test); \
	return false; \
      }

      // ensure we have either none or two children
      SPATIALTREE_TEST ((child1 == nullptr) == (child2 == nullptr), "child consistency");

      // check whether we have children
      if(child1)
      {
	//-------
	// Test-cases in case this is a node.
	//-------

	// ensure the node bounding box contains the bounding boxes of the children
	SPATIALTREE_TEST(GetNodeBBox().Contains(child1->GetNodeBBox()), "box mismatch");
	SPATIALTREE_TEST(GetNodeBBox().Contains(child2->GetNodeBBox()), "box mismatch");

	// compute union of child bounding boxes
	csBox3 new_box = child1->GetNodeBBox() + child2->GetNodeBBox();

	// enure the node bounding box is the union of the child bounding boxes
	SPATIALTREE_TEST(new_box == GetNodeBBox(), "box mismatch");

	// ensure we're set as parent for our children
	SPATIALTREE_TEST(child1->parent == this, "parent check");
	SPATIALTREE_TEST(child2->parent == this, "parent check");

	// validate split
	if(!static_cast<Self*>(this)->DebugCheckSplit(str))
	{
	  return false;
	}

	// perform checks for our children
	if(!child1->DebugCheckTreeTraversal(str))
	{
	  return false;
	}
	if(!child2->DebugCheckTreeTraversal(str))
	{
	  return false;
	}
      }

      //-------
      // Test-cases in case this is a leaf (or not a leaf but has
      // objects waiting for distribution).
      //-------

      // ensure we have a storage if we should be able to hold objects
      SPATIALTREE_TEST((maxObjects == 0) == (objects == nullptr), "object list");

      // ensure we don't have more objects than our storage can hold
      SPATIALTREE_TEST(numObjects <= maxObjects, "object list");

      // check all objects for validity
      for(int i = 0; i < numObjects; ++i)
      {
	// get the current object
	Child* o = objects[i];

	// check object
	if(!o->DebugCheck(str, static_cast<Self*>(this)))
	{
	  return false;
	}
      }

      // all checks passed
      return true;

#     undef SPATIALTREE_TEST
    }

    void DebugStatisticsTraversal(int& totalObjects,
	  int& totalNodes, int& totalLeaves, int depth, int& maxDepth,
	  float& balanceQuality)
    {
      // keep track of the total amount of objects
      totalObjects += numObjects;

      CS_ASSERT((child1 == nullptr) == (child2 == nullptr));

      // check what kind of node we have
      if(child1)
      {
	// we got a branch (it has children)
	++totalNodes;
      }
      else
      {
	// no children, we have a leaf
	++totalLeaves;
      }

      // we are one level deeper than our parent
      ++depth;

      // update maxDepth if this is deeper than the deepest
      // branch found so far
      if(depth > maxDepth)
      {
	maxDepth = depth;
      }

      // check whether we have children
      if(child1)
      {
	// we do, check how many objects are in each of them
	// so we can evaluate balancing

	// grab statistics for left child
	int left = 0;
	child1->DebugStatisticsTraversal(left, totalNodes,
	    totalLeaves, depth, maxDepth, balanceQuality);

	// grab statistics for right child
	int right = 0;
	child2->DebugStatisticsTraversal(right, totalNodes,
	    totalLeaves, depth, maxDepth, balanceQuality);

	// add the objects of the child to the total amount of objects
	totalObjects += left;
	totalObjects += right;

	// calculate balance
	balanceQuality += 2.0 * float(left) / float(left+right);
      }
    }

    csPtr<iString> DebugStatisticsTraversal()
    {
      // get a scf string to output our results to
      scfString* rc = new scfString();

      // get it's associated cs string so we can work with it more easily
      csString& str = rc->GetCsString();

      // place holders for results
      // total amount of objects in the tree
      int totalObjects = 0;
      // total amount of nodes in the tree
      int totalNodes = 0;
      // total amount of leaves in the tree
      int totalLeaves = 0;
      // highest depth level reached
      int maxDepth = 0;

      // overall quality of the tree
      float balanceQuality = 0;

      // collect the statictics by traversing the tree
      DebugStatisticsTraversal(totalObjects, totalNodes, totalLeaves,
	    0, maxDepth, balanceQuality);

      // format our output
      str.Format("#o=%d #n=%d #l=%d maxd=%d balqual=%g\n",
	    totalObjects, totalNodes, totalLeaves, maxDepth,
	    balanceQuality / float(totalNodes));

      // return output
      return csPtr<iString>((iString*)rc);
    }

    csTicks DebugBenchmark(int numIterations)
    {
      // start of first benchmark
      csTicks pass0 = csGetTicks();

      // tree building benchmark:
      // build numIterations random trees
      for(int i = 0 ; i < numIterations; ++i)
      {
	// clear the tree
	Clear();

	// build a random one
	for(int j = 0; j < 500; ++j)
	{
	  // add a random object
	  AddObject(Child::RandomBound(), (void*)0);

	  // distribute after 20 insertions
	  if(i % 20 == 0)
	  {
	    FullDistribute();
	  }
	}
      }

      // end of first/start of second benchmark
      csTicks pass1 = csGetTicks();

      // unoptimized tree traversal benchmark:
      // perform numIterations traversals in approximate
      // front to back order on an incremently built tree
      for(int i = 0; i < numIterations; ++i)
      {
	static_cast<Self*>(this)->Front2Back(csVector3(0, 0, 0),
			    DebugBenchmarkTraversal, nullptr, 0);
      }

      // end of second/start of third benchmark
      csTicks pass2 = csGetTicks();

      // tree distribution benchmark:
      // flatten the tree completely and completely
      // distribute it numIterations times
      for(int i = 0 ; i < numIterations; ++i)
      {
	Flatten();
	FullDistribute();
      }

      // end of third/start of last benchmark
      csTicks pass3 = csGetTicks();

      // optimized tree traversal benchmark:
      // perform numIterations traversals in approximate
      // front to back order on a tree that was distributed
      // with all information available
      for(int i = 0; i < numIterations; ++i)
      {
	static_cast<Self*>(this)->Front2Back(csVector3(0, 0, 0),
			    DebugBenchmarkTraversal, nullptr, 0);
      }

      // end of last benchmark
      csTicks pass4 = csGetTicks();

      // output results
      csPrintf ("Creating the tree:        %u ms\n", pass1-pass0);
      csPrintf ("Unoptimized Front2Back:   %u ms\n", pass2-pass1);
      csPrintf ("Flatten + FullDistribute: %u ms\n", pass3-pass2);
      csPrintf ("Optimized Front2Back:     %u ms\n", pass4-pass3);

      return pass4-pass0;
    }

    static bool DebugBenchmarkTraversal(Self* treenode, void*, uint32&)
    {
      treenode->Distribute();

      int numObjects = treenode->GetObjectCount();
      Child** objects = treenode->GetObjects();
      for(int i = 0; i < numObjects; ++i)
      {
	objects[i]->SetObject(nullptr);
      }

      return true;
    }

    static bool DebugBenchmarkTraversal(Self* treenode,
		  void*, uint32 timestamp, uint32&)
    {
      treenode->Distribute ();

      int numObjects = treenode->GetObjectCount();
      Child** objects = treenode->GetObjects();
      for(int i = 0; i < numObjects; ++i)
      {
	if(objects[i]->timestamp != timestamp)
	{
	  objects[i]->timestamp = timestamp;
	  objects[i]->SetObject(nullptr);
	}
      }

      return true;
    }

  void DebugDumpTraversal(csString& str, int indent)
  {
    // get a string for indentation
    csString ind("");
    ind.PadLeft(indent);

    // get debug statistics for this node
    csRef<iString> stats = DebugStatisticsTraversal();

    // append our data to the dump
    str.AppendFmt ("%s SpatialTree block=%d\n%s     box=%s\n%s %s",
	  ind.GetData(), block,
	  ind.GetData(), GetNodeBBox().Description().GetData(),
	  ind.GetData(), stats->GetData());

    // append object count
    str.AppendFmt("%s   %d objects\n", ind.GetData(), numObjects);

    // ensure we have either two or no children
    CS_ASSERT((child1 == nullptr) == (child2 == nullptr));

    // check whether we have children
    if(child1)
    {
      // dump split data
      static_cast<Self*>(this)->DebugDumpSplit(str, ind);

      // dump our children
      child1->DebugDumpTraversal(str, indent+2);
      child2->DebugDumpTraversal(str, indent+2);
    }
  }

  public:
    // iDebugHelper
    // indicate that we support statetest, text dump and benchmark
    virtual int GetSupportedTests () const
    {
      return CS_DBGHELP_STATETEST |
	CS_DBGHELP_TXTDUMP | CS_DBGHELP_BENCHMARK;
    }

    // performs a state test
    virtual csPtr<iString> StateTest()
    {
      // allocate output
      scfString* rc = new scfString();

      // perform check
      if(!DebugCheckTreeTraversal(rc->GetCsString()))
      {
	// return error if it failed
	return csPtr<iString>(rc);
      }

      // free output as there is none
      delete rc;

      // return empty result to indicate no error occured
      return csPtr<iString>(nullptr);
    }

    // performs a benchmark and returns the time it took
    virtual csTicks Benchmark(int numIterations)
    {
      return DebugBenchmark(numIterations);
    }

    // performs a text dump of the tree
    virtual csPtr<iString> Dump()
    {
      scfString* rc = new scfString();
      DebugDumpTraversal(rc->GetCsString(), 0);
      return csPtr<iString>(rc);
    }

    // we don't support graphical dumping
    virtual void Dump(iGraphics3D* /*g3d*/)
    {
    }

    // handles debug commands - as we don't have any
    // simply indicate we didn't handle that command
    virtual bool DebugCommand(const char*)
    {
      return false;
    }
  };
} // Geometry
} // CS

#endif // __CS_SPATIAL_TREE_H__
