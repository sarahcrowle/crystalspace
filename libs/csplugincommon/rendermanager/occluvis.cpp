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

#include <cssysdef.h>

#include "csgeom/math3d.h"
#include "csgeom/sphere.h"
#include "csgfx/renderbuffer.h"
#include "csplugincommon/rendermanager/occluvis.h"
#include "cstool/rbuflock.h"
#include "cstool/rendermeshholder.h"
#include "csutil/cfgacc.h"
#include "iengine/camera.h"
#include "iengine/engine.h"
#include "iengine/portal.h"
#include "iengine/portalcontainer.h"
#include "iengine/rview.h"
#include "iutil/object.h"
#include "iutil/objreg.h"
#include "ivideo/material.h"

#include <limits>

namespace CS
{
  namespace RenderManager
  {
    void csOccluvis::FinishQueries()
    {
      // check whether there are any queries to finish
      if(queries.IsEmpty())
      {
	// nothing to be done
	return;
      }

      // allocate array for query results
      CS_ALLOC_STACK_ARRAY_FALLBACK(bool, visibility, queries.GetSize(), 50000);
      CS_ALLOC_STACK_ARRAY_FALLBACK(unsigned, query, queries.GetSize(), 50000);

      // check for allocation failure
      CS_ASSERT(visibility);
      CS_ASSERT(query);

      for(size_t i = 0; i < queries.GetSize(); ++i)
      {
	query[i] = queries[i]->uOQuery;
      }

      // get query results
      g3d->OQVisibleQueries(query, visibility, static_cast<int>(queries.GetSize()));

      // get current frame number
      uint32 uNextCheck = engine->GetCurrentFrameNumber() + visibilityFrameSkip;

      // write results back
      for(size_t i = 0; i < queries.GetSize(); ++i)
      {
	// convenience variable
	QueryData* queryData = queries[i];

	// write back query result
	queryData->eResult = visibility[i] ? VISIBLE : INVISIBLE;

	// assume visible nodes will stay visible for a specified amount of time
	if(queryData->eResult == VISIBLE)
	{
	  queryData->uNextCheck = uNextCheck;

	  // add random delay when nodes first become visible to prevent synchronization
	  if(queryData->eLastResult != VISIBLE)
	  {
	    queryData->uNextCheck += RNG.Get(visibilityFrameSkip);
	  }
	}
      }

      // clear active queries
      queries.Empty();
    }

    template<bool bQueryVisibility>
    void csOccluvis::RenderMeshes(VisTreeNode* node,
                                  iRenderView* rview,
                                  size_t& lastTicket,
                                  iShader*& lastShader,
                                  iShaderVariableContext* shadervars,
                                  NodeMeshList*& nodeMeshList)
    {
      // start occlusion query
      if(bQueryVisibility)
      {
        BeginNodeQuery(node, rview);
      }

      // draw all meshes in this node
      for(int currRenderMesh = 0, m = 0; m < nodeMeshList->numMeshes; ++m)
      {
	// get mesh wrapper for current mesh
        iMeshWrapper* mw = nodeMeshList->meshList[m].imesh;

	// draw all render meshes in this mesh wrapper
        for(int i = 0; i < nodeMeshList->meshList[m].num; ++i, ++currRenderMesh)
        {
	  // get current render mesh
          csRenderMesh* rm = nodeMeshList->meshList[m].rmeshes[i];

	  // get z-buffer mode for this render mesh
	  csZBufMode zMode = nodeMeshList->onlyTestZ[currRenderMesh] ? CS_ZBUF_TEST : CS_ZBUF_USE;

	  // skip this mesh if it's testing and we aren't querying visibility
	  if(!bQueryVisibility && zMode == CS_ZBUF_TEST)
	  {
	    continue;
	  }

          // set z-buffer mode for this render mesh
          g3d->SetZMode(zMode);

          // find the according shader - if any - to use for this render mesh
          iShader* depthShader = nullptr;

	  // check whether we have a material - else we use default behaviour
          if(rm->material != nullptr)
          {
	    // get material
            iMaterial* mat = rm->material->GetMaterial();

	    // check whether we're only testing against the z-buffer
            if(zMode == CS_ZBUF_TEST)
            {
	      // get occlusion specific depth test shader if any
              depthShader = mat->GetShader(depthTestID);
            }
	    // we're writing to the depth buffer
            else
            {
	      // get occlusion specific depth write shader if any
              depthShader = mat->GetShader(depthWriteID);

	      // if none was found, try the generic depth write shader
              if(depthShader == nullptr)
              {
		// get generic depth write shader if any
                depthShader = mat->GetShader(fbDepthWriteID);
              }
            }

	    // if none of the above could be used, try the default shader
            if(depthShader == nullptr)
            {
              // @@@TODO: Come up with some better check or remove check when shader setup perf is better.
              if(shadervars != nullptr)
              {
                depthShader = shaderMgr->GetShader(defaultShader);
              }
            }
          }

          // disable the alpha test as it disables fast depth writing paths
          // test against z-buffer for smooth alpha mode
          // binary alpha mode must use *null depth write shader
          CS::Graphics::RenderMeshModes modes(*rm);
          modes.mixmode &= ~CS_MIXMODE_ALPHATEST_MASK;
          modes.mixmode |= CS_MIXMODE_ALPHATEST_DISABLE;

	  // check whether this render mesh is not a portal
          if(rm->portal == nullptr)
          {
	    // if we don't have a depth shader to use, use basic mesh setup
            if(depthShader == nullptr)
            {
	      // setup vertex attributes
              csVertexAttrib vA = CS_VATTRIB_POSITION;

	      // setup render buffers
              iRenderBuffer *rB = rm->buffers->GetRenderBuffer(CS_BUFFER_POSITION);

	      // activate render buffers for vertex atributes
              g3d->ActivateBuffers(&vA, &rB, 1);

	      // draw this render mesh
              g3d->DrawMeshBasic(rm, modes);

	      // deactivate render buffers for vertex attributes
              g3d->DeactivateBuffers(&vA, 1);
            }
	    // we have a depth shader
            else
            {
	      // clear shader variable stack
	      shaderVarStack.Clear();

              // add shader variables from the mesh wrapper
              mw->GetSVContext()->PushVariables(shaderVarStack);

	      // add shader variables from specific context if any
              if(shadervars != nullptr)
	      {
		shadervars->PushVariables(shaderVarStack);
	      }

              // get a ticker for our shader
              size_t ticket = depthShader->GetTicket(modes, shaderVarStack);

              // switch shader as necessary
              if(depthShader != lastShader || ticket != lastTicket)
              {
		// check whether we have a previous shader
                if(lastShader)
		{
		  // deactivate it
                  lastShader->DeactivatePass(lastTicket);
		}

                // @@@NOTE: depth shaders mustn't use more than one pass
		CS_ASSERT(depthShader->GetNumberOfPasses(ticket) == 1);

		// activate the pass for our new shader
                if(!depthShader->ActivatePass(ticket, 0))
		{ // failed
		  // clear previous shader as we already deactivated it
		  lastShader = nullptr;

		  // continue with next render mesh
                  continue;
		}

		// set our new shader and ticket responsibly
                lastShader = depthShader;
                lastTicket = ticket;
              }

              // setup the pass for this render mesh
              if(!depthShader->SetupPass(ticket, rm, modes, shaderVarStack))
	      { // pass setup failed
		// continue with next render mesh
                continue;
	      }

              // draw the mesh using the shader
              g3d->DrawMesh(rm, modes, shaderVarStack);

              // tear down pass setup for this render mesh
              depthShader->TeardownPass(ticket);
            }
          }
	  // this render mesh is a portal
          else
          {
            // @@@TODO: We're recalculating the rendermesh here, it would
            // be better to reuse the existing portal rendermesh.. but how?

	    // get vertex count 
	    size_t vertexCount = rm->portal->GetTotalVertexCount();

	    // maximum amount of vertices for the clipped result is
	    // 3 times the total amount of vertices
	    size_t vertexCountClipped = vertexCount * 3;

	    // get portal count
	    size_t portalCount = rm->portal->GetPortalCount();

	    // setup arrays for our vertices and vertex count per polygon
	    // we'll get one polygon per portal

	    // screeen space vertices
            csDirtyAccessArray<csVector2> allPortalVerts2d(vertexCountClipped);
	    allPortalVerts2d.SetSize(vertexCountClipped);

	    // camera space vertices
            csDirtyAccessArray<csVector3> allPortalVerts3d(vertexCountClipped);
	    allPortalVerts3d.SetSize(vertexCountClipped);

	    // number of vertices per polygon
            csDirtyAccessArray<size_t> allPortalVertsNums(portalCount);
	    allPortalVertsNums.SetSize(portalCount);

	    // compute our screen and camera space polygons
            rm->portal->ComputeScreenPolygons(rview, 
	      allPortalVerts2d.GetArray(), allPortalVerts3d.GetArray(),
	      vertexCountClipped, allPortalVertsNums.GetArray(),
              rview->GetGraphics3D()->GetWidth(), rview->GetGraphics3D()->GetHeight());

	    // get a renderbuffer holder
            csRenderBufferHolder holder;

	    // create a scrap rendermesh we'll use for our portals
	    csRenderMesh portalRM;

	    // set our render buffer holder
            portalRM.buffers = &holder;

	    // set index start as it's constant among our meshes
            portalRM.indexstart = 0;

	    // set mesh type to triangle fan (used for portals)
            portalRM.meshtype = CS_MESHTYPE_TRIANGLEFAN;

	    // set z-buffer mode to test only
            portalRM.z_buf_mode = CS_ZBUF_TEST;

	    // set our camera transform
            portalRM.object2world = rview->GetCamera()->GetTransform();

	    // set mixmode to transparent (don't touch frame buffer, only modify z-buffer)
            portalRM.mixmode = CS_FX_TRANSPARENT;

	    // create our vertex attribute
            csVertexAttrib vA = CS_VATTRIB_POSITION;

	    // get our flat vertex data
	    csVector3* portalVerts = allPortalVerts3d.GetArray();

	    // go over all portals to create their render meshes and draw them
            for(size_t p = 0; p < portalCount; ++p)
            {
	      // get vertex count for this portal
              size_t count = allPortalVertsNums[p];

	      // copy position buffer
	      {
		// create buffer
		csRef<csRenderBuffer> buffer = csRenderBuffer::CreateRenderBuffer(count, CS_BUF_STREAM, CS_BUFCOMP_FLOAT, 3);

		// copy from our source
		buffer->CopyInto(portalVerts, count);

		// attach buffer
		holder.SetRenderBuffer(CS_BUFFER_POSITION, buffer);
	      }

	      // create index buffer
	      {
		// create buffer
		csRef<csRenderBuffer> buffer = csRenderBuffer::CreateIndexRenderBuffer(count, CS_BUF_STREAM, CS_BUFCOMP_UNSIGNED_INT, 0, 4);

		// fill it
		{
		  csRenderBufferLock<uint> lock(buffer);
		  for(size_t i = 0; i < count; ++i)
		    *lock++ = uint(i);
		}

		// set buffer
		holder.SetRenderBuffer(CS_BUFFER_INDEX, buffer);
	      }

	      // set index end for this mesh
              portalRM.indexend = uint(count);

	      // get our position buffer for this mesh
              iRenderBuffer* rB = holder.GetRenderBuffer(CS_BUFFER_POSITION);

	      // activate the buffer for the position attribute
              g3d->ActivateBuffers(&vA, &rB, 1);

	      // draw this render mesh
              g3d->DrawMeshBasic(&portalRM, portalRM);

	      // deactivate our position buffer
              g3d->DeactivateBuffers(&vA, 1);

	      // offset our vertex buffer for the next portal
              portalVerts += count;
            }
          }
        }
      }

      // finish occlusion query
      if(bQueryVisibility)
      {
        g3d->OQEndQuery();
      }
    }

    template<bool bDoFrustumCulling>
    bool csOccluvis::TraverseTreeF2B(VisTree* node,
                                     Front2BackData& f2bData,
#				    if (OCCLUVIS_TREETYPE == 0)
				    uint32 timestamp,
#				    endif
				    uint32& frustumMask)
    {
      // check whether we should perform frustum culling
      if(bDoFrustumCulling)
      {
	// get bbox for this node
	csBox3 bbox = node->GetNodeBBox();

	// check whether we're not inside the box
	if(!bbox.Contains(f2bData.pos))
	{
	  uint32 newMask;
	  // check whether this node is completely invisible
	  if(!csIntersect3::BoxFrustum(bbox, f2bData.frustum, frustumMask, newMask))
	  {
	    // this node is completely invisible - stop traversal
	    return false;
	  }
	  // check whether this node is completely visible - i.e. there are no active clipping planes
	  else if(newMask == 0)
	  {
	    // continue traversal without frustum culling as this node (and by
	    // extension all child nodes) are completely contained in the frustum
	    // hack around gcc 4.4 being unwilling to do a cast
            bool (*func)(VisTree*,
                   Front2BackData&,
#                  if (OCCLUVIS_TREETYPE == 0)
                   uint32,
#                  endif
                   uint32&) = &TraverseTreeF2B<false>;
	    node->Front2Back(f2bData.pos, (VisTree::VisitFunc*)func, &f2bData, frustumMask);
	    return false;
	  }

	  // update frustum mask
	  frustumMask = newMask;
	}

	// node is partially visible
      }

      // we're continuing traversal - make sure objects are distributed
      node->Distribute();

      // test objects in this node
      VisTreeNode** objects = node->GetObjects();
      for(int i = 0; i < node->GetObjectCount(); ++i)
      {
        // get visibility object for this tree object
        iVisibilityObject* visobj = GetNodeVisObject(objects[i]);

#	if (OCCLUVIS_TREETYPE == 0)
	// test whether we already visited this node
	if(objects[i]->timestamp == timestamp)
	{
	  // skip this object
	  continue;
	}

	// update object timestamp
	objects[i]->timestamp = timestamp;
#	endif

        // skip this object if it's marked invisible
        if(visobj->GetMeshWrapper()->GetFlags().Check(CS_ENTITY_INVISIBLEMESH))
        {
	  continue;
	}

	// get visible meshes in this object
        csSectorVisibleRenderMeshes* sectorMeshList;
        int const numMeshes = f2bData.viscallback->GetVisibleMeshes(visobj->GetMeshWrapper(), frustumMask, sectorMeshList);

	// check whether there are any visible ones
        bool hasMeshes = false;
        for(int j = 0; j < numMeshes; ++j)
        {
          hasMeshes |= (sectorMeshList[j].num > 0);
        }

	// continue with the next object if this one has no visible meshes
	if(!hasMeshes)
	{
	  continue;
	}

	// get a mesh hash for this object in this render view
        VisObjMeshHash& visobjMeshHash = f2bData.parent->visobjMeshHashHash.GetOrCreate(f2bData.rview);

	// get a mesh list from the hash
        csRef<NodeMeshList> meshes = visobjMeshHash.Get(csPtrKey<iVisibilityObject>(visobj), csRef<NodeMeshList>());

	// if there's none, yet, create one
        if(!meshes.IsValid())
        {
          meshes.AttachNew(new NodeMeshList());
          visobjMeshHash.Put(visobj, meshes);
        }

        // update the mesh list data
        meshes->node = objects[i];
        meshes->framePassed = f2bData.parent->engine->GetCurrentFrameNumber();

	// backup old mesh count
	int const oldNumMeshes = meshes->numMeshes;

        // resize mesh list if needed
	if(meshes->meshList == nullptr || oldNumMeshes != numMeshes)
	{
	  // we mustn't have meshes without a storage
	  CS_ASSERT(!(meshes->meshList == nullptr && oldNumMeshes != 0));

	  // iterate over to-be-freed mesh lists
	  for(int m = numMeshes; m < oldNumMeshes; ++m)
	  {
	    // free no longer needed render mesh storage
	    cs_free(meshes->meshList[m].rmeshes);
	  }

	  // relocate storage
	  // @@@TODO: maybe we should check for numMeshes == 0 here and free in that case
	  meshes->meshList = (csSectorVisibleRenderMeshes*)cs_realloc(
	    meshes->meshList, sizeof(csSectorVisibleRenderMeshes) * numMeshes);

	  // check for allocation failure
	  CS_ASSERT(meshes->meshList);

	  // clear storage for uninit lists
	  for(int m = oldNumMeshes; m < numMeshes; ++m)
	  {
	    meshes->meshList[m].num = 0;
	    meshes->meshList[m].rmeshes = nullptr;
	  }

	  // update number of meshes
	  meshes->numMeshes = numMeshes;
	}

	// perform a semi-deep copy of the original list and get the total number of
	// render meshes
	size_t numRenderMeshes = 0;
	for(int m = 0; m < numMeshes; ++m)
	{
	  // get number of render meshes in this mesh
	  int num = sectorMeshList[m].num;

	  // copy mesh reference
	  meshes->meshList[m].imesh = sectorMeshList[m].imesh;

	  // resize render mesh storage if needed
	  if(meshes->meshList[m].rmeshes == nullptr || meshes->meshList[m].num != num)
	  {
	    // relocate storage
	    meshes->meshList[m].rmeshes = (csRenderMesh**)cs_realloc(
	      meshes->meshList[m].rmeshes,
	      sizeof(csRenderMesh*) * num);

	    // check for allocation failure
	    CS_ASSERT(meshes->meshList[m].rmeshes);

	    // update number of render meshes
	    meshes->meshList[m].num = num;
	  }

	  // copy render mesh references
	  memcpy(meshes->meshList[m].rmeshes, sectorMeshList[m].rmeshes, sizeof(csRenderMesh*) * num);

	  // update total number of render meshes
	  numRenderMeshes += num;
	}

        // check for 'always visible'
        switch(visobj->GetMeshWrapper()->GetZBufMode())
        {
        case CS_ZBUF_NONE:
        case CS_ZBUF_INVERT: // @@@TODO: why is INVERT always visible? it's just an inverted TEST
        case CS_ZBUF_FILL:
          {
            meshes->alwaysVisible = true;
            break;
          }
        default:
          {
            meshes->alwaysVisible = visobj->GetMeshWrapper()->GetFlags().Check(CS_ENTITY_ALWAYSVISIBLE);
            break;
          }
        }

        // Check for 'test only'
        bool bNeverDrawAny = false;

        // portals never perform depth writes
        if(visobj->GetMeshWrapper()->GetPortalContainer())
        {
          bNeverDrawAny = true;
        }
        else
        {
          // check the z-buffer mode
          switch(visobj->GetMeshWrapper()->GetZBufMode())
          {
          case CS_ZBUF_NONE:
          case CS_ZBUF_INVERT:
          case CS_ZBUF_TEST:
          case CS_ZBUF_EQUAL:
            {
              bNeverDrawAny = true;
              break;
            }
          default:
            break;
          }
        }

	// relocate our per-mesh data storage
        meshes->onlyTestZ.SetSize(numRenderMeshes);

	// if the mesh has a 'test only' z-mode, just update it for all render meshes
	if(bNeverDrawAny)
	{
	  meshes->onlyTestZ.SetAll();
	}
	// if the mesh performs depth writes we have to check individual meshes for their modes
	else
	{
	  // check 'test only' state of each render mesh
	  for(int currRenderMesh = 0, m = 0; m < numMeshes; ++m)
	  {
	    // go through all render meshes
	    for(int r = 0; r < sectorMeshList[m].num; ++r, ++currRenderMesh)
	    {
	      // the mesh wrapper uses depth writes
	      bool bOnlyTestZ = false;

	      // check the render mesh for a test only depth-buffer mode
	      switch(sectorMeshList[m].rmeshes[r]->z_buf_mode)
	      {
	      case CS_ZBUF_NONE:
	      case CS_ZBUF_INVERT:
	      case CS_ZBUF_TEST:
	      case CS_ZBUF_EQUAL:
		{
		  bOnlyTestZ = true;
		  break;
		}
	      default:
		break;
	      }

	      // if the render mesh doesn't use such a mode, it's material may still prohibit
	      // depth writes - check whether it has a material
	      if(!bOnlyTestZ && sectorMeshList[m].rmeshes[r]->material)
	      {
		// get the material
		iMaterial* mat = sectorMeshList[m].rmeshes[r]->material->GetMaterial();

		// get the occlusion specific depth write shader
		iShader* depthShader = mat->GetShader(f2bData.parent->depthWriteID);

		// check whether it's null
		if(depthShader == nullptr)
		{
		  // get the default depth write shader
		  depthShader = mat->GetShader(f2bData.parent->fbDepthWriteID);
		}

		// check whether it's the "*null" shader which prohibits depth writes
		if(depthShader == f2bData.parent->shaderMgr->GetShader("*null"))
		{
		  bOnlyTestZ = true;
		}
	      }

	      meshes->onlyTestZ.Set(currRenderMesh, bOnlyTestZ);
	    }
          }
        }

        f2bData.meshList->PushSmart(meshes);
      }

      return true;
    }

    OcclusionVisibility csOccluvis::GetNodeVisibility(VisTreeNode* node, iRenderView* rview)
    {
      // get query data for our node
      QueryData* queryData = GetNodeVisData(node).GetQueryData(g3d, rview);

      // mark node visible if we don't have a valid result
      if(queryData->eResult == INVALID)
      {
        return VISIBLE;
      }

      // mark node visibility according to cached result
      return queryData->eResult;
    }

    bool csOccluvis::CheckNodeVisibility(VisTreeNode* node, iRenderView* rview)
    {
      // get current frame number
      uint32 uFrame = engine->GetCurrentFrameNumber();

      // get query data for our node
      QueryData* queryData = GetNodeVisData(node).GetQueryData(g3d, rview);

      // check whether it's time to perform a new test, yet
      return uFrame >= queryData->uNextCheck;
    }

    void csOccluvis::BeginNodeQuery(VisTreeNode* node, iRenderView* rview)
    {
      // get query data for our node
      QueryData* queryData = GetNodeVisData(node).GetQueryData(g3d, rview);

      // backup old result
      queryData->eLastResult = queryData->eResult;

      // clear cached result
      queryData->eResult = UNKNOWN;

      // add query to active query list
      queries.Push(queryData);

      // start query
      g3d->OQBeginQuery(queryData->uOQuery);
    }

    csOccluvis::csOccluvis(iObjectRegistry* object_reg)
      : scfImplementationType(this), object_reg(object_reg),
        shaderVarStack(0,0)
    {
      // get config to read settings
      csConfigAccess config(object_reg);

      // get iGraphics3D ref
      g3d = csQueryRegistry<iGraphics3D>(object_reg);

      // get iEngine ref
      engine = csQueryRegistry<iEngine>(object_reg);

      // get iShaderManager ref
      shaderMgr = csQueryRegistry<iShaderManager>(object_reg);

      // get iStringSet ref
      stringSet = csQueryRegistryTagInterface<iStringSet>(
        object_reg, "crystalspace.shared.stringset");

      // get iShaderVarStringSet ref
      svStrings = csQueryRegistryTagInterface<iShaderVarStringSet>(
        object_reg, "crystalspace.shader.variablenameset");

      // disable precaching
      bAllVisible = false;

      // disable extern object list
      vistestObjectsInuse = false;

      // get frame skip parameter from config
      visibilityFrameSkip = config->GetInt("Engine.Occluvis.FrameSkip", 10);
    }

    csOccluvis::~csOccluvis()
    {
      // get all mesh lists
      csArray<csRefArray<NodeMeshList>*> nodeMeshLists = nodeMeshHash.GetAll();

      // iterate over all mesh lists and delete them
      for(size_t i = 0; i < nodeMeshLists.GetSize(); ++i)
      {
        delete nodeMeshLists[i];
      }
    }

    void csOccluvis::Setup(char const* defaultShaderName)
    {
      // set name of default shader
      defaultShader = defaultShaderName;

      // get id for occlusion specific depth write shader
      depthWriteID = stringSet->Request("oc_depthwrite");

      // get id for occlusion specific depth test shader
      depthTestID = stringSet->Request("oc_depthtest");

      // get id for generic depth write shader
      fbDepthWriteID = stringSet->Request("depthwrite");
    }

    void csOccluvis::RegisterVisObject(iVisibilityObject* visobj)
    {
      // ensure the object is valid
      CS_ASSERT(visobj);

      // get the moveable
      iMovable* movable = visobj->GetMovable();
      CS_ASSERT(movable);

      // get the object model
      iObjectModel* objModel = visobj->GetObjectModel();
      CS_ASSERT(objModel);

      // update the bbox
      visobj->GetMeshWrapper()->GetWorldBoundingBox();

      // create tree node data
      TreeNodeData* data = new TreeNodeData;

      // set it's visibility object
      data->visobj = visobj;

      // add the object to the tree
      VisTreeNode* node = AddObject(visobj->GetBBox(), data);

      // create our wrapper for it
      csRef<csVisibilityObjectWrapper> visobjWrap;
      visobjWrap.AttachNew(new csVisibilityObjectWrapper(this, node, visobj));

      // add the wrapper as movable listener
      movable->AddListener(visobjWrap);

      // add the wrapper as object model listener
      objModel->AddListener(visobjWrap);

      // add the wrapper to our own list
      visObjects.Push(visobjWrap);
    }

    void csOccluvis::UnregisterVisObject(iVisibilityObject* visobj)
    {
      // ensure the object is valid
      CS_ASSERT(visobj);

      // keep track whether this object belongs to us
      bool found = false;

      // find the wrapper for this object and delete it
      for(size_t i = 0; i < visObjects.GetSize(); ++i)
      {
	// get our wrapper
        csVisibilityObjectWrapper* visobjWrap = visObjects[i];
	CS_ASSERT(visobjWrap);

	// check whether it belongs to our object
        if(visobjWrap->GetVisObject() == visobj)
        {
	  // found it
	  found = true;

	  // get the tree node
	  VisTreeNode* node = visobjWrap->GetNode();
	  CS_ASSERT(node);

	  // get movable
          iMovable* movable = visobj->GetMovable();
	  CS_ASSERT(movable);

	  // get object model
	  iObjectModel* objModel = visobj->GetObjectModel();
	  CS_ASSERT(objModel);

	  // remove movable listener
          movable->RemoveListener(visobjWrap);

	  // remove object model listener
          objModel->RemoveListener(visobjWrap);

	  // free node data
	  delete static_cast<TreeNodeData*>(node->GetObject());

	  // remove object from tree
	  RemoveObject(node);

	  // delete object
          visObjects.DeleteIndexFast(i);
          break;
        }
      }

      // ensure the object was found
      if(!found)
          return;

      // get all mesh lists for the various render views
      csArray<csRefArray<NodeMeshList>*> nodeMeshLists = nodeMeshHash.GetAll();

      // go over the hash of hashes so we can delete our object from all lists
      VisObjMeshHashHash::GlobalIterator itr = visobjMeshHashHash.GetIterator();
      while(itr.HasNext())
      {
	// get the current hash
        VisObjMeshHash& visobjMeshHash = itr.Next();

	// get all lists in this hash that belong to our object
        csArray<NodeMeshList*> meshLists = visobjMeshHash.GetAll(csPtrKey<iVisibilityObject>(visobj));

	// go over the list of lists to delete the ones belonging to our object
        for(size_t i = 0; i < nodeMeshLists.GetSize(); ++i)
        {
          for(size_t j = 0; j < meshLists.GetSize(); ++j)
          {
            nodeMeshLists[i]->Delete(meshLists[j]);
          }
        }

	// clear the lists we just removed from our hash
        visobjMeshHash.DeleteAll(csPtrKey<iVisibilityObject>(visobj));
      }
    }

    //======== VisTest =========================================================

    void csOccluvis::MarkAllVisible(VisTree* node, Front2BackData& f2bData)
    {
      // get objects in the node itself (no need to distribute here)
      VisTreeNode** objects = node->GetObjects();

      // go over all objects and mark them visible
      for(int i = 0; i < node->GetObjectCount(); ++i)
      {
	iVisibilityObject* object = GetNodeVisObject(objects[i]);
	f2bData.viscallback->ObjectVisible(object, object->GetMeshWrapper(), 0);
      }

      // traverse tree
      VisTree* child1 = node->GetChild1();
      VisTree* child2 = node->GetChild2();
      if(child1)
      {
	MarkAllVisible(child1, f2bData);
	MarkAllVisible(child2, f2bData);
      }
    }

    /*------------------------------------------------------------------*/
    /*--------------------------- MAIN DADDY ---------------------------*/
    /*------------------------------------------------------------------*/

    bool csOccluvis::VisTest(iRenderView* rview, iVisibilityCullerListener* viscallback, int, int)
    {
      // make sure query results are read back
      FinishQueries();

      // get the render context of this few
      csRenderContext* ctxt = rview->GetRenderContext();

      // get the frustum mask for that context
      uint32 frustumMask = ctxt->clip_planes_mask;

      // allocate front to back data
      Front2BackData f2bData;

      // set us as parent
      f2bData.parent = this;

      // set render view
      f2bData.rview = rview;

      // set render view
      f2bData.viscallback = viscallback;

      // set frustum planes
      f2bData.frustum = ctxt->clip_planes;

      // set camera position
      f2bData.pos = rview->GetCamera()->GetTransform().GetOrigin();

      // setup shader variable stack
      shaderVarStack.Setup(svStrings->GetSize());

      // render everything without culling if all meshes are to be marked visible
      if(bAllVisible)
      {
        // mark all visible
        MarkAllVisible(this, f2bData);

	// we're done
        return false;
      }

      // get node mesh lists array for this view
      csRefArray<NodeMeshList>* nodeMeshLists = nodeMeshHash.Get(csPtrKey<iRenderView>(rview), nullptr);

      // check whether we found one
      if(nodeMeshLists == nullptr)
      {
	// nope, allocate a new one
        nodeMeshLists = new csRefArray<NodeMeshList>();
        nodeMeshHash.Put(csPtrKey<iRenderView>(rview), nodeMeshLists);

	// check for allocation failure
	CS_ASSERT(nodeMeshLists);
      }

      // set it in the data
      f2bData.meshList = nodeMeshLists;

      // traverse tree in approximate front to back order and fill array of visible nodes
      // hack around gcc 4.4 being unwilling to do a cast
      bool (*func)(VisTree*,
                   Front2BackData&,
#                  if (OCCLUVIS_TREETYPE == 0)
                   uint32,
#                  endif
                   uint32&) = &TraverseTreeF2B<true>;
      Front2Back(f2bData.pos, (VisTree::VisitFunc*)func, &f2bData, frustumMask);

      // sort it front to back
      F2BSorter sorter(engine, f2bData.pos);
      nodeMeshLists->Sort(sorter);

      // iterate over node lists in reverse Front to Back order
      for(size_t n = nodeMeshLists->GetSize(); n > 0; --n)
      {
        NodeMeshList*& nodeMeshList = nodeMeshLists->Get(n-1);

        // check whether frustum checks passed
        if(nodeMeshList->framePassed == engine->GetCurrentFrameNumber())
        {
          // check whether this node is marked visibile
          if(nodeMeshList->alwaysVisible || GetNodeVisibility(nodeMeshList->node, rview) == VISIBLE)
          {
            // get visibility object for this node
	    iVisibilityObject* visobj = GetNodeVisObject(nodeMeshList->node);

	    // get it's mesh wrapper
            iMeshWrapper* mesh = visobj->GetMeshWrapper();

	    // mark this mesh visible
            f2bData.viscallback->MarkVisible(mesh, nodeMeshList->numMeshes, nodeMeshList->meshList);
          }
        }
      }

      return true;
    }

    bool csOccluvis::RenderViscull(iRenderView* rview, iShaderVariableContext* shadervars)
    {
      // if we're marking all visible this frame, just return
      if(bAllVisible)
        return false;

      // look up node mesh lists array for this render view
      csRefArray<NodeMeshList>* nodeMeshLists = nodeMeshHash.Get(csPtrKey<iRenderView>(rview), nullptr);

      // we cannot render without a mesh list
      // @@@RlyDontKnow: how does this happen if it does at all?
      // It happens when you run deferreddemo.
      //CS_ASSERT(nodeMeshLists);
      if (!nodeMeshLists)
        return false;

      // disable frame buffer writes
      // @@@NOTE: this may be overwritten by shaders
      g3d->SetWriteMask(false, false, false, false);

      // initialize last shader used to none
      iShader* lastShader = nullptr;

      // we'll keep track of the last used ticket
      size_t lastTicket = 0;

      // backup current z-buffer mode
      csZBufMode oldZMode = g3d->GetZMode();

      // backup current wireframe drawing seting
      bool wireframe = g3d->GetEdgeDrawing();

      // disable wireframe drawing if necessary
      if(wireframe)
	g3d->SetEdgeDrawing(false);

      // go over the node list performing a depth only pass and occlusion queries as needed
      for(size_t n = 0; n < nodeMeshLists->GetSize(); ++n)
      {
	// get next mesh list
        NodeMeshList*& nodeMeshList = (*nodeMeshLists)[n];
       
        // check whether frustum checks passed
        if(nodeMeshList->framePassed == engine->GetCurrentFrameNumber())
        {
	  // check whether the visibility of this node needs to be checked
          if(nodeMeshList->alwaysVisible || !CheckNodeVisibility(nodeMeshList->node, rview))
          {
            // no, the visibility of this list is already marked, render without queries
            RenderMeshes<false>(nodeMeshList->node, rview, lastTicket, lastShader, shadervars, nodeMeshList);
          }
          else
          {
            // yes, we don't know the visibility of this node, yet - render with queries
            RenderMeshes<true>(nodeMeshList->node, rview, lastTicket, lastShader, shadervars, nodeMeshList);
          }
        }
      }

      // Deactivate the last shader if there is any
      if(lastShader != nullptr)
      {
        lastShader->DeactivatePass(lastTicket);
      }

      // restore wireframe drawing mode
      if(wireframe)
	g3d->SetEdgeDrawing(true);

      // restore z-buffer mode
      g3d->SetZMode(oldZMode);

      // enable framebuffer writes again
      g3d->SetWriteMask(true, true, true, true);

      // force a depth clear in wireframe mode
      // @@@TODO: is this really needed? does it even work like it's done here?
      if(wireframe)
      {
	g3d->BeginDraw(g3d->GetCurrentDrawFlags() | CSDRAW_CLEARZBUFFER);
      }

      return true;
    }

    bool F2BSorter::operator()(csOccluvis::NodeMeshList* const& m1,
                               csOccluvis::NodeMeshList* const& m2)
    {
      // get mesh wrappers from mesh lists
      iMeshWrapper* mw1 = m1->meshList->imesh;
      iMeshWrapper* mw2 = m2->meshList->imesh;

      // check whether they have the same rander priority
      if(mw1->GetRenderPriority() == mw2->GetRenderPriority())
      { // nope, order by distance
	// get node bounding boxes
        csBox3 const& m1box = m1->node->GetBBox();
        csBox3 const& m2box = m2->node->GetBBox();

	// get distance of the bounding boxes to the origin
        float const distSqRm1 = m1box.SquaredPosDist(cameraOrigin);
        float const distSqRm2 = m2box.SquaredPosDist(cameraOrigin);

	// nearest first
        return distSqRm1 < distSqRm2;
      }

      // portals are always rendered last
      if(mw1->GetRenderPriority() == portalPriority)
        return false;
      if(mw2->GetRenderPriority() == portalPriority)
        return true;

      // finally sort by priority
      return mw1->GetRenderPriority() < mw2->GetRenderPriority();
    }

    //======== VisTest box =====================================================
    void csOccluvis::TraverseTreeBox(VisTree* node,
                                     VistestObjectsArray* voArray,
                                     csBox3 const& box)
    {
      // check whether the node's bbox intersects with the target one
      if(!box.TestIntersect(node->GetNodeBBox()))
        return;

      // get all objects in this node
      VisTreeNode** objects = node->GetObjects();

      // go over all of them
      for(int i = 0; i < node->GetObjectCount(); ++i)
      {
	// get object
	VisTreeNode* object = objects[i];

	// check wheter it intersects with the box
	if(box.TestIntersect(object->GetBBox()))
	{
	  // add it to the list
	  voArray->Push(GetNodeVisObject(object));
	}
      }

      // check whether this node has childs
      if(node->GetChild1())
      {
	// continue traversal
        TraverseTreeBox(node->GetChild1(), voArray, box);
        TraverseTreeBox(node->GetChild2(), voArray, box);
      }
    }

    csPtr<iVisibilityObjectIterator> csOccluvis::VisTest(csBox3 const& box)
    {
      // visbility test object array
      VistestObjectsArray* v = nullptr;

      // check whether our objects array is already in use
      if(vistestObjectsInuse)
      {
        // it is, allocate a new one
        v = new VistestObjectsArray();

	// check for allocation failure
	CS_ASSERT(v);
      }
      // nope, it's free
      else
      {
	// set it
        v = &vistestObjects;

	// clear it
        vistestObjects.Empty();
      }

      // traverse tree performing the box visibility test
      TraverseTreeBox(this, v, box);

      // create an object iterator for the results
      csOccluvisObjIt* vobjit = new csOccluvisObjIt(v,
        vistestObjectsInuse ? nullptr : &vistestObjectsInuse);

      // return it
      return csPtr<iVisibilityObjectIterator>(vobjit);
    }

    //======== VisTest sphere ==================================================
    void csOccluvis::TraverseTreeSphere(VisTree* node,
                                        VistestObjectsArray* voArray,
                                        csVector3 const& centre,
                                        float const sqradius)
    {
      // check whether the node's bbox intersects with the target sphere
      if(!csIntersect3::BoxSphere(node->GetNodeBBox(), centre, sqradius))
        return;

      // get all objects in this node
      VisTreeNode** objects = node->GetObjects();

      // go over all of them
      for(int i = 0; i < node->GetObjectCount(); ++i)
      {
	// get object
	VisTreeNode* object = objects[i];

	// check wheter it intersects with the box
	if(csIntersect3::BoxSphere(object->GetBBox(), centre, sqradius))
	{
	  // add it to the list
	  voArray->Push(GetNodeVisObject(object));
	}
      }

      // check whether this node has childs
      if(node->GetChild1())
      {
	// continue traversal
        TraverseTreeSphere(node->GetChild1(), voArray, centre, sqradius);
        TraverseTreeSphere(node->GetChild2(), voArray, centre, sqradius);
      }
    }

    csPtr<iVisibilityObjectIterator> csOccluvis::VisTest(csSphere const& sphere)
    {
      // visbility test object array
      VistestObjectsArray* v = nullptr;

      // check whether our objects array is already in use
      if(vistestObjectsInuse)
      {
        // it is, allocate a new one
        v = new VistestObjectsArray();

	// check for allocation failure
	CS_ASSERT(v);
      }
      // nope, it's free
      else
      {
	// set it
        v = &vistestObjects;

	// clear it
        vistestObjects.Empty();
      }

      // traverse tree performing the sphere visibility test
      TraverseTreeSphere(this, v, sphere.GetCenter(),
        sphere.GetRadius() * sphere.GetRadius());

      // create an object iterator for the results
      csOccluvisObjIt* vobjit = new csOccluvisObjIt(v,
        vistestObjectsInuse ? nullptr : &vistestObjectsInuse);

      // return it
      return csPtr<iVisibilityObjectIterator>(vobjit);
    }

    void csOccluvis::TraverseTreeSphere(VisTree* node,
                                        iVisibilityCullerListener* viscallback,
                                        csVector3 const& centre,
                                        float const sqradius)
    {
      // check whether the node's bbox intersects with the target sphere
      if(!csIntersect3::BoxSphere(node->GetNodeBBox(), centre, sqradius))
        return;

      // get all objects in this node
      VisTreeNode** objects = node->GetObjects();

      // go over all of them
      for(int i = 0; i < node->GetObjectCount(); ++i)
      {
	// get object
	VisTreeNode* object = objects[i];

	// check wheter it intersects with the box
	if(csIntersect3::BoxSphere(object->GetBBox(), centre, sqradius))
	{
	  // get visibility object
	  iVisibilityObject* visobj = GetNodeVisObject(object);

	  // ensure it's valid
	  CS_ASSERT(visobj);

	  // notify listener about the visibility object
	  viscallback->ObjectVisible(visobj, visobj->GetMeshWrapper(), 0);
	}
      }

      // check whether this node has childs
      if(node->GetChild1())
      {
	// continue traversal
        TraverseTreeSphere(node->GetChild1(), viscallback, centre, sqradius);
        TraverseTreeSphere(node->GetChild2(), viscallback, centre, sqradius);
      }
    }

    void csOccluvis::VisTest(csSphere const& sphere, 
                             iVisibilityCullerListener* viscallback)
    {
      TraverseTreeSphere(this, viscallback, sphere.GetCenter(),
        sphere.GetRadius() * sphere.GetRadius());
    }

    //======== VisTest planes ==================================================
    void csOccluvis::TraverseTreePlanes(VisTree* node,
                                        VistestObjectsArray* voArray,
                                        csPlane3* planes,
                                        uint32 frustumMask)
    {
      // check whether the node's bbox intersects with the target frustum
      uint32 newMask;
      if(!csIntersect3::BoxFrustum(node->GetNodeBBox(), planes, frustumMask, newMask))
        return;

      // get all objects in this node
      VisTreeNode** objects = node->GetObjects();

      // go over all of them
      for(int i = 0; i < node->GetObjectCount(); ++i)
      {
	// get object
	VisTreeNode* object = objects[i];

	// check wheter it intersects with the frustum
	if(csIntersect3::BoxFrustum(node->GetNodeBBox(), planes, frustumMask, newMask))
	{
	  // add it to the list
	  voArray->Push(GetNodeVisObject(object));
	}
      }

      // check whether this node has childs
      if(node->GetChild1())
      {
	// continue traversal
        TraverseTreePlanes(node->GetChild1(), voArray, planes, frustumMask);
        TraverseTreePlanes(node->GetChild2(), voArray, planes, frustumMask);
      }
    }

    csPtr<iVisibilityObjectIterator> csOccluvis::VisTest(csPlane3* planes, int numPlanes)
    {
      // visbility test object array
      VistestObjectsArray* v = nullptr;

      // check whether our objects array is already in use
      if(vistestObjectsInuse)
      {
        // it is, allocate a new one
        v = new VistestObjectsArray();

	// check for allocation failure
	CS_ASSERT(v);
      }
      // nope, it's free
      else
      {
	// set it
        v = &vistestObjects;

	// clear it
        vistestObjects.Empty();
      }

      // traverse tree performing the sphere visibility test
      TraverseTreePlanes(this, v, planes, numPlanes);

      // create an object iterator for the results
      csOccluvisObjIt* vobjit = new csOccluvisObjIt(v,
        vistestObjectsInuse ? nullptr : &vistestObjectsInuse);

      // return it
      return csPtr<iVisibilityObjectIterator>(vobjit);
    }

    void csOccluvis::TraverseTreePlanes(VisTree* node,
                                        iVisibilityCullerListener* viscallback,
                                        csPlane3* planes,
                                        uint32 frustumMask)
    {
      // check whether the node's bbox intersects with the target frustum
      uint32 newMask;
      if(!csIntersect3::BoxFrustum(node->GetNodeBBox(), planes, frustumMask, newMask))
        return;

      // get all objects in this node
      VisTreeNode** objects = node->GetObjects();

      // go over all of them
      for(int i = 0; i < node->GetObjectCount(); ++i)
      {
	// get object
	VisTreeNode* object = objects[i];

	// check wheter it intersects with the frustum
	if(csIntersect3::BoxFrustum(node->GetNodeBBox(), planes, frustumMask, newMask))
	{
	  // get visibility object
	  iVisibilityObject* visobj = GetNodeVisObject(object);

	  // ensure it's valid
	  CS_ASSERT(visobj);

	  // notify listener about the visbility object
	  viscallback->ObjectVisible(visobj, visobj->GetMeshWrapper(), 0);
	}
      }

      // check whether this node has childs
      if(node->GetChild1())
      {
	// continue traversal
        TraverseTreePlanes(node->GetChild1(), viscallback, planes, frustumMask);
        TraverseTreePlanes(node->GetChild2(), viscallback, planes, frustumMask);
      }
    }

    void csOccluvis::VisTest(csPlane3* planes, int num_planes,
                              iVisibilityCullerListener* viscallback)
    {
      TraverseTreePlanes(this, viscallback, planes, (1 << num_planes) - 1);
    }

    //======== IntersectSegment ================================================

    // hitbeam tracing for objects, if sloppy is true, add all objects that may intersect
    // to the vector of possible hits, else find closest hit
    template<bool sloppy>
    bool csOccluvis::TraverseIntersectSegment(VisTree* node,
			  IntersectSegmentFront2BackData& data,
#			  if (OCCLUVIS_TREETYPE == 0)
			  uint32 timestamp,
#			  endif
			  uint32& frustumMask)
    {
      // distribute objects if necessary
      node->Distribute();

      // holder for intersection point
      csVector3 isect;

      // check whether node bounding box intersects with segment
      if(csIntersect3::BoxSegment(node->GetNodeBBox(), data.seg, isect) == -1)
      {
	// didn't intersect, abort
	return false;
      }

      // check whether we already have a hit
      if(!sloppy && data.mesh != nullptr)
      {
	// we do, check whether it's further than our box hit
	if(csSquaredDist::PointPoint(data.seg.Start(), isect) < data.sqdist)
	{
	  // it is, abort
	  return false;
	}
      }

      // get object count
      int const numObjects = node->GetObjectCount();

      // get objects
      VisTreeNode** objects = node->GetObjects();

      // go over all objects - keep track whether we got a hit
      bool hit = false;
      for(int i = 0; i < numObjects; ++i)
      {
	// get object
	iVisibilityObject* visobj = GetNodeVisObject(objects[i]);

#	if (OCCLUVIS_TREETYPE == 0)
	// test whether we already visited this object
	if(objects[i]->timestamp == timestamp)
	{
	  // skip this object
	  continue;
	}

	// update object timestamp
	objects[i]->timestamp = timestamp;
#	endif

	// get mesh for this object
	iMeshWrapper* mesh = visobj->GetMeshWrapper();

	// check if mesh is valid and whether it shall be hit by hitbeams
	if(mesh == nullptr || mesh->GetFlags().Check(CS_ENTITY_NOHITBEAM))
	{
	  // nope, continue with next one
	  continue;
	}

	// get object bounding box and test whether it intersects
	if(csIntersect3::BoxSegment(objects[i]->GetBBox(), data.seg, isect) != -1)
	{ // we got a possible intersection
	  // if this is a sloppy trace, just add it to the list and continue with the next one
	  if(sloppy)
	  {
	    data.vector->Push(visobj);
	    hit = true;
	    continue;
	  }

	  // this is not a sloppy trace, perform hit detection in object space

	  // get movable interface
	  iMovable* movable = visobj->GetMovable();

	  // validate it's present
	  CS_ASSERT(movable);

	  // check whether we have to perform any transformation of our segment
	  bool identity = movable->IsFullTransformIdentity();

	  // allocate our transformed segment
	  csSegment3 segObj;

	  // allocate transform placeholder (only used if a transform is necessary)
	  csReversibleTransform object2world;

	  // check whether a transform is needed
	  if(identity)
	  {
	    // no, just copy it over
	    segObj.Set(data.seg.Start(), data.seg.End());
	  }
	  // we do need one
	  else
	  {
	    // get full transform - This == Object, Other == World
	    object2world = movable->GetFullTransform();

	    // transform our segment to object space
	    segObj.Set(
	      object2world.Other2This(data.seg.Start()),
	      object2world.Other2This(data.seg.End())
	      );
	  }

	  // get mesh object (required for non-sloppy hit detection)
	  iMeshObject* meshObj = mesh->GetMeshObject();

	  // validate it's present
	  CS_ASSERT(meshObj);

	  // hit data
	  // whether the beam hit
	  bool hitObj;
	  // distance in segment space
	  float dist;
	  // index of the polygon that was hit if supported, else -1
	  int polyIndex = -1;

	  // check whether we are supposed to perform an accurate hit detection
	  if(data.accurate)
	  {
	    // an accurate hitbeam was requested, perform it
	    hitObj = meshObj->HitBeamObject(segObj.Start(), segObj.End(), isect,
					    &dist, &polyIndex, nullptr, data.bf);
	  }
	  // check failed
	  else
	  {
	    // we can get away with an inaccurate hit
	    hitObj = meshObj->HitBeamOutline(segObj.Start(), segObj.End(), isect, &dist);
	  }

	  // check whether we got a hit
	  if(hitObj)
	  {
	    // yep, we got a hit - check whether we should just collect them all
	    if(data.vector != nullptr)
	    {
	      // we should, add it
	      data.vector->Push(visobj);
	    }
	    // no, we only want the closest hit - check whether our hit is closer
	    else if(dist < data.r)
	    {
	      // yes, update closest hit in segment coordinates
	      data.r = dist;

	      // update mesh with closest hit
	      data.mesh = mesh;

	      // update polygon index for the hit
	      data.polygon_idx = polyIndex;

	      // check whether the intersection point has to be transformed
	      if(identity)
	      {
		// no, just update it
		data.isect = isect;
	      }
	      // it needs transformation
	      else
	      {
		// transform and set
		data.isect = object2world.This2Other(isect);
	      }

	      // update smallest distance in world space
	      data.sqdist = csSquaredDist::PointPoint(data.seg.Start(), data.isect);
	    }
	  }
	}
      }

      // continue traversal if we didn't get a hit
      return !hit;
    }

    csPtr<iVisibilityObjectIterator> csOccluvis::IntersectSegment(
        csVector3 const& start, csVector3 const& end, bool accurate,
	bool bf)
    {
      // allocate intersection data
      IntersectSegmentFront2BackData data;

      // set segment to be traced
      data.seg.Set(start, end);

      // set hit to invalid
      data.sqdist = std::numeric_limits<float>::max(); // maximum distance
      data.r = std::numeric_limits<float>::max(); // maximum segment space distance
      data.mesh = nullptr; // no mesh hit
      data.polygon_idx = -1; // no poly hit
      data.vector = new VistestObjectsArray(); // empty array
      data.accurate = accurate; // forward accuracy setting
      data.bf = bf; // forward culling setting

      // trace segment
      // hack around gcc 4.4 being unwilling to do a cast
      bool (*func)(VisTree*,
                   IntersectSegmentFront2BackData&,
#                  if (OCCLUVIS_TREETYPE == 0)
                   uint32,
#                  endif
                   uint32&) = &TraverseIntersectSegment<false>;
      Front2Back(end-start, (VisTree::VisitFunc*)func, &data, 0);

      // create object iterator for results and return it
      return csPtr<iVisibilityObjectIterator>(new csOccluvisObjIt(data.vector, 0));
    }

    bool csOccluvis::IntersectSegment(csVector3 const& start,
        csVector3 const& end, csVector3& isect, float* pr,
        iMeshWrapper** mesh, int* polyIndex,
        bool accurate, bool bf)
    {
      // allocate intersection data
      IntersectSegmentFront2BackData data;

      // set segment to be traced
      data.seg.Set(start, end);

      // set hit to invalid
      data.sqdist = std::numeric_limits<float>::max(); // maximum distance
      data.r = std::numeric_limits<float>::max(); // maximum segment space distance
      data.mesh = nullptr; // no mesh hit
      data.polygon_idx = -1; // no poly hit
      data.vector = new VistestObjectsArray(); // empty array
      data.accurate = accurate; // forward accuracy setting
      data.bf = bf; // forward culling setting

      // trace segment
      // hack around gcc 4.4 being unwilling to do a cast
      bool (*func)(VisTree*,
                   IntersectSegmentFront2BackData&,
#                  if (OCCLUVIS_TREETYPE == 0)
                   uint32,
#                  endif
                   uint32&) = &TraverseIntersectSegment<false>;
      Front2Back(end-start, (VisTree::VisitFunc*)func, &data, 0);

      // set intersection
      isect = data.isect;

      // set mesh if the caller requested it
      if(mesh != nullptr) *mesh = data.mesh;

      // set segment space distance if caller requested it
      if(pr != nullptr) *pr = data.r;

      // set the index of the polygon that was hit if request
      if(polyIndex != nullptr) *polyIndex = data.polygon_idx;

      // we got a result if we hit a mesh
      return data.mesh != nullptr;
    }

    csPtr<iVisibilityObjectIterator> csOccluvis::IntersectSegmentSloppy(
        csVector3 const& start, csVector3 const& end)
    {
      // allocate intersection data
      IntersectSegmentFront2BackData data;

      // set segment to be traced
      data.seg.Set(start, end);

      // allocate storage for results
      data.vector = new VistestObjectsArray();
      
      // trace segment sloppily
      // hack around gcc 4.4 being unwilling to do a cast
      bool (*func)(VisTree*,
                   IntersectSegmentFront2BackData&,
#                  if (OCCLUVIS_TREETYPE == 0)
                   uint32,
#                  endif
                   uint32&) = &TraverseIntersectSegment<true>;
      Front2Back(end-start, (VisTree::VisitFunc*)func, &data, 0);

      // create object iterator for results and return it
      return csPtr<iVisibilityObjectIterator>(new csOccluvisObjIt(data.vector, 0));
    }
  } // namespace Rendermanager
} // namespace CS
