/*
  Copyright (C) 2010 by Mike Gist and Claudiu Mihail
  Copyright (C) 2013 by Matthieu Kraus

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
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

#ifndef __CS_RENDERMANAGER_OCCLUVIS_H__
#define __CS_RENDERMANAGER_OCCLUVIS_H__

// set acceleration structure to use:
#define OCCLUVIS_TREETYPE_KD    0
#define OCCLUVIS_TREETYPE_BIH   1
#define OCCLUVIS_TREETYPE_BVH   2

#define OCCLUVIS_TREETYPE OCCLUVIS_TREETYPE_BIH

#if (OCCLUVIS_TREETYPE == OCCLUVIS_TREETYPE_KD)
#  include "csgeom/kdtree.h"
#elif (OCCLUVIS_TREETYPE == OCCLUVIS_TREETYPE_BIH)
#  include "csgeom/bih.h"
#elif (OCCLUVIS_TREETYPE == OCCLUVIS_TREETYPE_BVH)
#  include "csgeom/bvh.h"
#else
#  error Unknown occluvis tree type
#endif
#include "csutil/hash.h"
#include "csutil/refarr.h"
#include "csutil/randomgen.h"
#include "csutil/scf_implementation.h"
#include "iengine/engine.h"
#include "iengine/movable.h"
#include "iengine/mesh.h"
#include "iengine/sector.h"
#include "iengine/viscull.h"
#include "imesh/objmodel.h"

struct iGraphics3D;
struct iMeshWrapper;
struct iObjectRegistry;
class csOccluvis;

#include "csutil/deprecated_warn_off.h"

namespace CS
{
  namespace RenderManager
  {
    // Frustum culling visibility results.
    enum NodeVisibility
    {
      NODE_INVISIBLE,
      NODE_VISIBLE,
      NODE_INSIDE
    };

    // Occlusion visibility results.
    enum OcclusionVisibility
    {
      VISIBLE,
      UNKNOWN,
      INVISIBLE,
      INVALID
    };

    typedef csArray<iVisibilityObject*, csArrayElementHandler<iVisibilityObject*>,
    CS::Container::ArrayAllocDefault, csArrayCapacityFixedGrow<256> >
    VistestObjectsArray;

    struct IntersectSegmentFront2BackData
    {
      csSegment3 seg;
      csVector3 isect;
      float sqdist;		// Squared distance between seg.start and isect.
      float r;
      iMeshWrapper* mesh;
      int polygon_idx;
      VistestObjectsArray* vector;	// If not-null we need all objects.
      bool accurate;
      bool bf;
    };

    struct QueryData : public csRefCount
    {
      unsigned int uOQuery;
      uint32 uNextCheck;
      OcclusionVisibility eResult;
      OcclusionVisibility eLastResult;

      QueryData()
        : uOQuery(0), uNextCheck(0),
	  eResult(INVALID), eLastResult(INVALID)
      {}
    };

    struct TreeNodeVisibilityData
    {
      TreeNodeVisibilityData() : g3d(nullptr)
      {
      }

      ~TreeNodeVisibilityData()
      {
	// we cannot free anything without a backend
        if(g3d)
        {
	  // free query data for all render views
          FreeQueryData();
        }
      }

      QueryData* GetQueryData(iGraphics3D* ig3d, iRenderView* rview)
      {
	// get the query data for this render view if we already have some
        csRef<QueryData> queryData = RViewQueries.Get(csPtrKey<iRenderView>(rview), csRef<QueryData>());

	// check whether we already have data
        if(!queryData.IsValid())
        { // no, create it
	  // cache our graphics backend as we'll need it to delete our queries
          g3d = ig3d;

	  // create new query data
          queryData.AttachNew(new QueryData);

	  // create a query object
          g3d->OQInitQueries(&queryData->uOQuery, 1);

	  // add it to our query list
	  queries.Push(queryData->uOQuery);

	  // add the new data to our lookup hash for this rview
          RViewQueries.Put(csPtrKey<iRenderView>(rview), queryData);
        }

	// return obtained data
        return queryData;
      }

      void FreeQueryData()
      {
	// delete queries
	g3d->OQDelQueries(queries.GetArray(), static_cast<int>(queries.GetSize()));
	queries.Empty();

	// clear hash
        RViewQueries.DeleteAll();
      }

    private:
      // graphics backend - required to free data on destruction
      iGraphics3D* g3d;

      // convenience typedef for hash type
      typedef csHash<csRef<QueryData>, csPtrKey<iRenderView> > RViewQueryHash;

      // hash that holds query data for each render view
      RViewQueryHash RViewQueries;

      // housekeeping array with all queries for this node
      csDirtyAccessArray<unsigned> queries;
    };

#if (OCCLUVIS_TREETYPE == OCCLUVIS_TREETYPE_KD)
    typedef csKDTree VisTree;
#elif (OCCLUVIS_TREETYPE == OCCLUVIS_TREETYPE_BIH)
    typedef csBIH VisTree;
#elif (OCCLUVIS_TREETYPE == OCCLUVIS_TREETYPE_BVH)
    typedef csBVH VisTree;
#endif
    typedef VisTree::Child VisTreeNode;

    class CS_CRYSTALSPACE_EXPORT csOccluvis :
      public scfImplementationExt1<csOccluvis, VisTree, iVisibilityCuller>
    {
    private:
      iObjectRegistry* object_reg;
      csRef<iGraphics3D> g3d;
      csRef<iEngine> engine;
      csRef<iShaderManager> shaderMgr;
      csRef<iStringSet> stringSet;
      csRef<iShaderVarStringSet> svStrings;

      struct NodeMeshList : public csRefCount
      {
        NodeMeshList() : numMeshes(0), meshList(nullptr)
        {
        }

        ~NodeMeshList()
        {
          for(int i = 0; i < numMeshes; ++i)
	  {
	    cs_free(meshList[i].rmeshes);
	  }
	  cs_free(meshList);
        }

        int numMeshes;
        uint framePassed;
        csBitArray onlyTestZ;
        bool alwaysVisible;
        VisTreeNode* node;
        csSectorVisibleRenderMeshes* meshList;
      };

      // Structure of common f2b data.
      struct Front2BackData
      {
        csVector3 pos;
        iRenderView* rview;
        csPlane3* frustum;
        iVisibilityCullerListener* viscallback;
	csRefArray<NodeMeshList>* meshList;
	csOccluvis* parent;
      };

      class csVisibilityObjectWrapper
        : public scfImplementation2<csVisibilityObjectWrapper, iMovableListener, iObjectModelListener>
      {
      private:
	// Updating method issued by listeners.
	void Update()
	{
	  // recalculate bounding box
	  visobj->GetMeshWrapper()->GetWorldBoundingBox();

	  // move object in the tree
	  culler->MoveObject(node, visobj->GetBBox());
	}

      public:
        csVisibilityObjectWrapper(csOccluvis* culler, VisTreeNode* node, iVisibilityObject* visobj)
	  : scfImplementationType(this), culler(culler), node(node), visobj(visobj)
	{
	}

        /// The object model has changed.
        virtual void ObjectModelChanged(iObjectModel* model)
	{
	  Update();
	}

        /// The movable has changed.
        virtual void MovableChanged(iMovable* movable)
	{
	  Update();
	}

        /// The movable is about to be destroyed.
        virtual void MovableDestroyed(iMovable*)
	{
	}

	/// Getter for the associated object.
        inline iVisibilityObject* GetVisObject() const
        {
          return visobj;
        }

	/// Getter for the associated tree node.
	inline VisTreeNode* GetNode() const
	{
	  return node;
	}

      private:
        csOccluvis* culler;
	VisTreeNode* node;
        iVisibilityObject* visobj;
      };

      struct TreeNodeData
      {
	iVisibilityObject* visobj;
	TreeNodeVisibilityData data;
      };

      // Array of visibility objects.
      // @@@TODO: this is only a housekeeping thing - a hash may be better suited for faster deletion
      csRefArray<csVisibilityObjectWrapper> visObjects;

      // RNG for offsetting first query for visible nodes
      csRandomGen RNG;

      // Set to true to pass all objects regardless of visibility next VisCull.
      bool bAllVisible;

      // Frame skip parameter
      unsigned visibilityFrameSkip;

      // Hash of mesh nodes for a render view.
      csHash<csRefArray<NodeMeshList>*, csPtrKey<iRenderView> > nodeMeshHash;

      // Hashes of MeshList objects for visibility objects.
      typedef csHash<NodeMeshList*, csPtrKey<iVisibilityObject> > VisObjMeshHash;
      typedef csHash<VisObjMeshHash, csPtrKey<iRenderView> > VisObjMeshHashHash;
      VisObjMeshHashHash visobjMeshHashHash;

      // Array of vistest objects (used in the box/sphere/etc. tests).
      VistestObjectsArray vistestObjects;
      bool vistestObjectsInuse;

      // currently active queries
      csArray<QueryData*> queries;

      // Depth test shader type ID.
      csStringID depthTestID;

      // Depth write shader type ID.
      csStringID depthWriteID;

      // Fallback depth write shader type ID.
      csStringID fbDepthWriteID;

      // Name of the default depth write/test shader.
      csString defaultShader;

      // Shader variable stack for depth rendering.
      csShaderVariableStack shaderVarStack;

      friend class F2BSorter;

    protected:
      /**
       * Reads back the results of all currently active queries.
       */
      void FinishQueries();

      /**
       * Renders the meshes in the given list.
       */
      template<bool bQueryVisibility>
      void RenderMeshes(VisTreeNode* node,
                        iRenderView* rview,
                        size_t& lastTicket,
                        iShader*& lastShader,
                        iShaderVariableContext* shadervars,
                        NodeMeshList*& nodeMeshList);

      /**
       * Trasverses the tree from Front to Back.
       */
      template<bool bDoFrustumCulling>
      static bool TraverseTreeF2B(VisTree* node, Front2BackData& f2bData,
#	if (OCCLUVIS_TREETYPE == OCCLUVIS_TREETYPE_KD)
	uint32 timestamp,
#	endif
	uint32& frustum_mask);

      /**
       * Traverses the tree from Front to Back checking for intersections with a segment.
       */
      template<bool sloppy>
      static bool TraverseIntersectSegment(VisTree* node, IntersectSegmentFront2BackData& data,
#	if (OCCLUVIS_TREETYPE == OCCLUVIS_TREETYPE_KD)
	uint32 timestamp,
#	endif
	uint32& frustumMask);

      /**
       * Returns the visibility data of a node.
       */
      inline static TreeNodeVisibilityData& GetNodeVisData(VisTreeNode* node)
      {
	return static_cast<TreeNodeData*>(node->GetObject())->data;
      }

      inline static iVisibilityObject* GetNodeVisObject(VisTreeNode* node)
      {
	return static_cast<TreeNodeData*>(node->GetObject())->visobj;
      }

      /**
       * Returns the visibility of a view-node pair.
       */
      OcclusionVisibility GetNodeVisibility(VisTreeNode* node, iRenderView* rview);

      /**
       * Returns whether to check the visibility of a view-node pair.
       */
      bool CheckNodeVisibility(VisTreeNode* node, iRenderView* rview);

      /**
       * Begin a occlusion query of a view-node pair.
       */
      void BeginNodeQuery(VisTreeNode* node, iRenderView* rview);

      /**
       * Mark all meshes as visible.
       */
      void MarkAllVisible(VisTree* node, Front2BackData& f2bData);

      /**
       * Traverses the tree checking for objects that intersect with the given box.
       */
      void TraverseTreeBox(VisTree* node, VistestObjectsArray* voArray, csBox3 const& box);

      /**
       * Traverses the tree checking for objects that intersect with the given sphere.
       */
      void TraverseTreeSphere(VisTree* node, VistestObjectsArray* voArray,
        csVector3 const& centre, float const sqradius);

      /**
       * Traverses the tree checking for objects that intersect with the given sphere.
       */
      void TraverseTreeSphere(VisTree* node, iVisibilityCullerListener* viscallback,
        csVector3 const& centre, float const sqradius);

      /**
       * Transverses the tree checking for objects that are in the volume 
       * formed by the set of planes.
       */
      void TraverseTreePlanes(VisTree* node, VistestObjectsArray* voArray,
	csPlane3* planes, uint32 frustum_mask);

      /**
      * Transverses the tree checking for objects that are in the volume 
      * formed by the set of planes.
       */
      void TraverseTreePlanes(VisTree* node, iVisibilityCullerListener* viscallback,
        csPlane3* planes, uint32 frustum_mask);

    public:
      csOccluvis(iObjectRegistry* object_reg);
      virtual ~csOccluvis();

      // iVisibilityCuller

      /**
       * Initial setup executed before first usage.
       */
      virtual void Setup(char const* defaultShaderName);

      /**
       * Register a visibility object with this culler.
       */
      virtual void RegisterVisObject(iVisibilityObject* visobj);

      /**
       * Unregister a visibility object with this culler.
       */
      virtual void UnregisterVisObject(iVisibilityObject* visobj);

      /**
       * Do a visibility test from a given viewpoint.
       */
      virtual bool VisTest(iRenderView* rview, iVisibilityCullerListener* viscallback, int, int);

      /**
       * Mark all objects intersecting the box as visible.
       */
      virtual csPtr<iVisibilityObjectIterator> VisTest(csBox3 const& box);

      /**
       * Mark all objects intersecting the sphere as visible.
       */
      virtual csPtr<iVisibilityObjectIterator> VisTest(csSphere const& sphere);

      /**
       * Mark all objects contained in the polyhedron formed by the planes as visible.
       */
      virtual csPtr<iVisibilityObjectIterator> VisTest(csPlane3* planes, int num_planes);

      /**
       * Notify the callback about all objects intersecting the sphere.
       */
      virtual void VisTest(csSphere const& sphere, iVisibilityCullerListener* viscallback);

      /**
       * Notify the callback about all objects contained in the polyhedron formed by the planes.
       */
      virtual void VisTest(csPlane3* planes, int num_planes, iVisibilityCullerListener* viscallback);

      /**
       * Return all objects hit by the beam. May include objects that aren't hit, but are close to the beam.
       */
      virtual csPtr<iVisibilityObjectIterator> IntersectSegmentSloppy(csVector3 const& start, csVector3 const& end);

      /**
       * Return all objects hit by the beam.
       * If bf is true backface culling is done (only valid if accurate is true).
       */
      virtual csPtr<iVisibilityObjectIterator> IntersectSegment(
        csVector3 const& start, csVector3 const& end,
	bool accurate = false, bool bf = false);

      /**
       * Intersect a beam using this culler and return the intersection
       * point, the mesh and optional polygon index. If the returned mesh is 0
       * then this means that the object belonging to the culler itself was
       * hit. Some meshes don't support returning polygon indices in which case
       * that field will always be -1.
       * If accurate is false then a less accurate (and faster) method is used.
       * In that case the polygon index will never be filled.
       * If bf is true then backface culling is used. This only works if 'accurate'
       * is set to true.
       */
      virtual bool IntersectSegment(csVector3 const& start,
        csVector3 const& end, csVector3& isect, float* pr = nullptr,
        iMeshWrapper** p_mesh = nullptr, int* poly_idx = nullptr,
        bool accurate = true, bool bf = false);

      /**
       * Prepare culling for the next frame.
       */
      virtual bool RenderViscull(iRenderView* rview, iShaderVariableContext* shadervars);

     /**
      * Mark that we're about to perform precaching.
      */
      virtual void BeginPrecacheCulling()
      {
        bAllVisible = true;
      }

      /**
       * Mark that we've finished precaching.
       */
      virtual void EndPrecacheCulling()
      {
        bAllVisible = false;
      }

      // deprecated method
      virtual void PrecacheCulling()
      {
	CS_ASSERT_MSG("Call (Begin/End)PrecacheCulling!", false);
      }

      /**
       * Parse additional parameters - returns error message on error, else nullptr.
       */
      virtual char const* ParseCullerParameters(iDocumentNode* node)
      {
	return 0;
      }
    };

    class F2BSorter
    {
    public:
      F2BSorter(iEngine* engine, csVector3 const& cameraOrigin)
        : cameraOrigin(cameraOrigin)
      {
        portalPriority = engine->GetRenderPriority("portal");
      }

      bool operator()(csOccluvis::NodeMeshList* const& m1,
                      csOccluvis::NodeMeshList* const& m2);

    private:
      csVector3 const& cameraOrigin;
      CS::Graphics::RenderPriority portalPriority;
    };

    class csOccluvisObjIt :
      public scfImplementation1<csOccluvisObjIt, iVisibilityObjectIterator>
    {
    private:
      VistestObjectsArray* vector;
      size_t position;
      bool* vistestObjectsInUse;

    public:
      csOccluvisObjIt(VistestObjectsArray* vector,
        bool* vistestObjectsInUse) :
        scfImplementationType(this),
	vector(vector), vistestObjectsInUse(vistestObjectsInUse)
      {
        if(vistestObjectsInUse != nullptr)
	{
	  *vistestObjectsInUse = true;
	}

        Reset();
      }
      virtual ~csOccluvisObjIt()
      {
        // If the vistest_objects_inuse pointer is not null we set the
        // bool to false to indicate we're no longer using the base
        // vector. Otherwise we delete the vector.
        if(vistestObjectsInUse != nullptr)
	{
          *vistestObjectsInUse = false;
	}
        else
	{
          delete vector;
	}
      }

      virtual iVisibilityObject* Next()
      {
        if(position == csArrayItemNotFound)
	{
	  return 0;
	}

        iVisibilityObject* vo = vector->Get(position);
        ++position;

        if(position == vector->GetSize())
	{
          position = csArrayItemNotFound;
	}

        return vo;
      }

      virtual void Reset()
      {
        if(vector == nullptr || vector->IsEmpty())
	{
          position = csArrayItemNotFound;
	}
        else
	{
          position = 0;
	}
      }

      virtual bool HasNext() const
      {
        return ((position != csArrayItemNotFound) && position <= vector->GetSize());
      }
    };
  }
}

#include "csutil/deprecated_warn_on.h"

#endif // __CS_RENDERMANAGER_OCCLUVIS_H__
