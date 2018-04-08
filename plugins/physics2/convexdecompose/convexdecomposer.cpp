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

#include "cssysdef.h"

#include "convexdecomposer.h"

#include "igeom/trimesh.h"
#include "csgeom/trimesh.h"

#include "csutil/custom_new_disable.h"

#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"

#include "csutil/custom_new_enable.h"

#include "hacd/hacdCircularList.h"
#include "hacd/hacdVector.h"
#include "hacd/hacdICHull.h"
#include "hacd/hacdGraph.h"
#include "hacd/hacdHACD.h"

CS_PLUGIN_NAMESPACE_BEGIN (ConvexDecompose)
{
  SCF_IMPLEMENT_FACTORY (ConvexDecomposer);

  ConvexDecomposer::ConvexDecomposer (iBase* parent)
    : scfImplementationType (this, parent)
  {}

  ConvexDecomposer::~ConvexDecomposer ()
  {}

  bool ConvexDecomposer::Initialize (iObjectRegistry* object_reg)
  {
    this->object_reg= object_reg;
    return true;
  }

  void ConvexDecomposer::DecomposeMesh (iTriangleMesh* triMesh, csRefArray<iTriangleMesh>& result) const
  {
    //-----------------------------------------------
    // HACD
    //-----------------------------------------------

    // Setup
    std::vector< HACD::Vec3<HACD::Real> > hacdVerts;
    std::vector< HACD::Vec3<long> > hacdTris;

    {
      // Copy Vertices
      csVector3* nextVert = triMesh->GetVertices ();
      for (size_t i=0; i < triMesh->GetVertexCount (); ++i, ++nextVert ) 
      {
        HACD::Vec3<HACD::Real> vertex (nextVert->x, nextVert->y, nextVert->z);
        hacdVerts.push_back (vertex);
      }

      // Copy triangles
      csTriangle* nextTri = triMesh->GetTriangles ();
      for (size_t i=0; i < triMesh->GetTriangleCount (); ++i, ++nextTri)
      {
        HACD::Vec3<long> triangle (nextTri->a, nextTri->b, nextTri->c);
        hacdTris.push_back (triangle);
      }
    }

    // Set parameters
    HACD::HACD hacder;
    hacder.SetPoints (&hacdVerts[0]);
    hacder.SetNPoints (hacdVerts.size ());
    hacder.SetTriangles (&hacdTris[0]);
    hacder.SetNTriangles (hacdTris.size ());
    hacder.SetCompacityWeight (0.1);
    hacder.SetVolumeWeight (0.0);

    // Recommended parameters: 2 100 0 0 0 0
    size_t nClusters = 2;
    double concavity = 100;
    //bool invert = false;
    bool addExtraDistPoints = false;
    bool addNeighboursDistPoints = false;
    bool addFacesPoints = false;       

    hacder.SetNClusters (nClusters);                     // minimum number of clusters
    hacder.SetNVerticesPerCH (100);                      // max of 100 vertices per convex-hull
    hacder.SetConcavity (concavity);                     // maximum concavity
    hacder.SetAddExtraDistPoints (addExtraDistPoints);   
    hacder.SetAddNeighboursDistPoints (addNeighboursDistPoints);   
    hacder.SetAddFacesPoints (addFacesPoints); 

    // Decompose
    hacder.Compute ();

    // Produce convex trimeshes from the result
    nClusters = hacder.GetNClusters ();
    for (size_t c = 0; c < nClusters; c++)
    {
      // Get the data
      size_t nPoints = hacder.GetNPointsCH (c);
      size_t nTriangles = hacder.GetNTrianglesCH (c);

      //float* vertices = new float[nPoints*3];
      //unsigned int* triangles = new unsigned int[nTriangles*3];

      HACD::Vec3<HACD::Real> * nextVertex = new HACD::Vec3<HACD::Real>[nPoints];
      HACD::Vec3<long> * nextTri = new HACD::Vec3<long>[nTriangles];

      //hacder.DenormalizeData ();               // Move vertices back to where they were
      hacder.GetCH (c, nextVertex, nextTri);   // copy mesh data

      // Create trimesh
      csRef<csTriangleMesh> triMesh;
      triMesh.AttachNew (new csTriangleMesh);

      // Copy vertices
      for (unsigned int i=0; i < nPoints; ++i, ++nextVertex)
      {
        csVector3 vertex (nextVertex->X (), nextVertex->Y (), nextVertex->Z ());
        /*vertex *= localScaling;
        vertex -= centroid;*/
        triMesh->AddVertex (vertex);
      }

      // Copy indices
      for (size_t i=0; i < nTriangles; ++i, ++nextTri)
      {
        triMesh->AddTriangle (nextTri->X (), nextTri->Y (), nextTri->Z ());
      }
      
      // Yield result
      result.Push (triMesh);
    }
  }

}
CS_PLUGIN_NAMESPACE_END (ConvexDecompose)
