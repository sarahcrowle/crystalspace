/*
  Copyright (C) 2005 by Marten Svanfeldt

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

#ifndef __LIGHTMAPUV_SIMPLE_H__
#define __LIGHTMAPUV_SIMPLE_H__

#include "primitive.h"
#include "lightmapuv.h"

#include "maxrect.h"

// Uncomment to dump subrectangles used during PrepareLighting()
// #define DUMP_SUBRECTANGLES

namespace lighter
{

  class ArraysOneUV
  {
  public:
    class ArrayType
    {
      int u, v;
    public:
      ArrayType (int u, int v) : u (u), v (v) {}

      size_t GetSize () const { return 1; }
      csVector2 GetUVSize (size_t n) const 
      { 
        return csVector2 (u, v);
      }
    };
    ArrayType pseudoArray;
    ArraysOneUV (int u, int v) : pseudoArray (u, v) {}

    size_t GetSize() const { return 1; }
    ArrayType Get (size_t index) const 
    { return pseudoArray; }
  };

  class SimpleUVObjectLayouter;

  class SimpleUVFactoryLayouter : public LightmapUVFactoryLayouter
  {
  public:
    SimpleUVFactoryLayouter (LightmapPtrDelArray& lightmaps) : 
      prepared (false), globalLightmaps (lightmaps)
    {}

    virtual csPtr<LightmapUVObjectLayouter> LayoutFactory (
      const FactoryPrimitiveArray& inPrims, ObjectFactoryVertexData& vertexData,
      const ObjectFactory* factory,
      csArray<FactoryPrimitiveArray>& outPrims,
      csBitArray& usedVerts, bool noSplit);

    virtual void PrepareLighting (Statistics::Progress& progress);

    uint AllocLightmap (uint lmW, uint lmH);
  protected:
    friend class SimpleUVObjectLayouter;

    bool prepared;
    LightmapPtrDelArray& globalLightmaps;
    csArray<csVector2> groupMinuvs;

    /**
     * Vertex equality criterium, for determination of nieghbouring primitives.
     */
    enum VertexEquality
    {
      /// Two vertices are considered equal if they're the same vertex.
      Pedantic,
      /**
       * Two vertices are considered equal if they have the same position and
       * normal.
       */
      PositionAndNormal,
      /// Two vertices are considered equal if they have the same position
      Position
    };

    struct Edge
    {
      const FactoryPrimitive& prim;
      size_t a, b;

      Edge (const FactoryPrimitive& prim, size_t a, size_t b) : 
        prim (prim), a (a), b (b) {}

      bool equals (VertexEquality veq, const Edge& other);
    };
    struct UberPrimitive
    {
      FactoryPrimitiveArray prims;
      csArray<Edge> outsideEdges;
      VertexEquality equality;

      UberPrimitive (VertexEquality equality, 
        const FactoryPrimitive& startPrim);

      bool UsesEdge (const Edge& edge);
      void AddPrimitive (const FactoryPrimitive& prim);
    };
    typedef csSafeCopyArray<UberPrimitive> UberPrimArray;

    void DetermineNeighbouringPrims (
      const FactoryPrimitiveArray& inPrims, ObjectFactoryVertexData& vertexData,
      csArray<FactoryPrimitiveArray>& outPrims);

    bool AllocLightmap (LightmapPtrDelArray& lightmaps, int u, int v, 
      csRect &lightmapArea, int &lightmapID);

    bool ProjectPrimitives (FactoryPrimitiveArray& prims, 
      csBitArray &usedVerts, float lmscale, 
      Vector2Array& lightmapUVs, bool noSplit);
    void ScaleLightmapUVs (FactoryPrimitiveArray& prims, 
      Vector2Array& lightmapUVs, float lmscale);

  public:
    struct SectorAndPDBits
    {
      SectorAndPDBits():sector(0){};

      SectorAndPDBits(Sector* sector, csBitArray& pdBits)
        :sector(sector),pdBits(pdBits)
      {}

      Sector* sector;
      csBitArray pdBits;
    };
  protected:
    struct LayoutTransform
    {
      explicit LayoutTransform(csRect initialRect) :rect(initialRect) {}

      uint lmID;
      csRect rect;
    };

    /* Allocate the layout in the globallightmaps
     *
     */
    void AllocLayoutArray(csArray<LayoutTransform*>& layouts);

    void QueuePDGroupLayout(LayoutTransform* layout, 
      Sector* sector, const csBitArray& pdBits);

    /* PD lighting queueing
     *
     * In order to more efficiently lay out PD-lit primitives they're
     * queued and laid out in PrepareLighting.
     */
    void QueuePDPrimitives (SimpleUVObjectLayouter* layouter, 
      PrimitiveArray &prims,  size_t layoutID, size_t groupNum, 
      Sector* sector, const csBitArray& pdBits, 
      const csArray<csVector2>& uvsizes);

    /// One set of primitives that needs to be laid out onto one allocator.
    struct QueuedPDPrimitives
    {
      SimpleUVObjectLayouter* layouter;
      size_t layoutID;
      size_t groupNum;
      PrimitiveArray* prims;
      csArray<csVector2> uvsizes;
    };
    typedef csArray<QueuedPDPrimitives> QueuedPDPArray;

    typedef csHash<QueuedPDPArray, SectorAndPDBits> PDQueuesHash;
    PDQueuesHash pdQueues;

    //Layout transform affected by pd-light
    typedef csHash<csArray<LayoutTransform*>, SectorAndPDBits > 
      LayoutTransformHash;

    LayoutTransformHash pdLayoutTransforms;

    csArray<LayoutTransform*> globalLayoutTransforms;

    static int SortLayoutTransform (LayoutTransform* const & l1,
      LayoutTransform* const & l2);

    /// Pair of a PD-lit primitive queues array and the affecting PD lights.
    struct PDLQueue
    {
      Sector* sector;
      csBitArray pdBits;
      QueuedPDPArray* queue;
    };
    static int SortPDLQueues (const PDLQueue& p1, const PDLQueue& p2);

    // Allocate space in globallightmaps for non affected by
    // pd lights primitives
    void PrepareLightingPDL(csArray<PDLQueue>& queuesPDL,
      Statistics::Progress& prepareProgress); 

    void PrepareLightingNoPDL(csArray<PDLQueue>& queuesNoPDL,
      Statistics::Progress& progressNonPDL); 

    /// Mixins for AllocAllPrims
    class ArraysQPDPA
    {
      const QueuedPDPArray& arrays;
    public:
      class ArrayType
      {
        const QueuedPDPrimitives& queue;
      public:
        ArrayType (const QueuedPDPrimitives& queue) : queue (queue) {}

        size_t GetSize () const { return queue.uvsizes.GetSize(); }
        const csVector2& GetUVSize (size_t n) const { return queue.uvsizes[n]; }
      };

      ArraysQPDPA (const QueuedPDPArray& arrays) : arrays (arrays) {}

      size_t GetSize() const { return arrays.GetSize(); }
      ArrayType Get (size_t index) const { return ArrayType (arrays[index]); }
    };

    /**
     * Primitive queues layouted onto a number of allocators.
     * Used in PrepareLighting.
     */
    struct LayoutedQueue
    {
      /// PD light bits for the queues
      csBitArray pdBits;
      Sector* sector;

      struct Map
      {
        struct Queue
        {
          /// The queues primitives
          const QueuedPDPrimitives* srcQueue;
          /// Positions on the allocator
          csArray<csVector2> positions;
        };
        csArray<Queue> queues;
        /// Rectangle allocators
        MaxRectanglesCompact* alloc;
      };
      /// The queues and each allocator
      csArray<Map> maps;
    };
    static int SortLQMaps (LayoutedQueue::Map const& s1, 
      LayoutedQueue::Map const& s2);

    class AllocLQ
    {
      LayoutedQueue& queue;
    public:
      AllocLQ (LayoutedQueue& queue) : queue (queue) {}

      size_t GetSize() const { return queue.maps.GetSize(); }
      MaxRectanglesCompact& Get (size_t n) { return *queue.maps[n].alloc; }
      MaxRectanglesCompact& New (size_t& index) 
      { 
        csRect area (0, 0, globalConfig.GetLMProperties ().maxLightmapU,
          globalConfig.GetLMProperties ().maxLightmapV);
        MaxRectanglesCompact* alloc = new MaxRectanglesCompact (area);
        index = queue.maps.GetSize();
        queue.maps.GetExtend (index).alloc = alloc;
        /*alloc.SetGrowPO2 (true);*/
        return *alloc; 
      }
      void Delete (size_t index)
      {
        delete queue.maps[index].alloc;
        queue.maps.DeleteIndex (index);
      }
      void CleanEmpty ()
      {
        size_t i = 0;
        while (i < queue.maps.GetSize())
        {
          if (queue.maps[i].alloc->IsEmpty())
          {
            delete queue.maps[i].alloc;
            queue.maps.DeleteIndex (i);
          }
          else
            i++;
        }
      }
    };
    class ArraysLQ
    {
      csArray<MaxRectanglesCompact*> allocs;
    public:
      struct AllocOrigin
      {
        size_t lq;
        size_t alloc;
      };
      csArray<AllocOrigin> origins;

      class ArrayType
      {
        MaxRectanglesCompact* alloc;
      public:
        ArrayType (MaxRectanglesCompact* alloc) : alloc (alloc) {}

        size_t GetSize () const { return 1; }
        csVector2 GetUVSize (size_t n) const 
        { 
          const csRect& rect = alloc->GetRectangle ();
          return csVector2 (rect.xmax, rect.ymax);
        }
      };
      ArraysLQ (csArray<LayoutedQueue>& queues)
      {
        for (size_t q = 0; q < queues.GetSize(); q++)
        {
          AllocOrigin org;
          org.lq = q;
          for (size_t a = 0; a < queues[q].maps.GetSize(); a++)
          {
            MaxRectanglesCompact* allocP = queues[q].maps[a].alloc;
            allocs.Push (allocP);
            org.alloc = a;
            origins.Push (org);
          }
        }
      }

      size_t GetSize() const { return allocs.GetSize(); }
      ArrayType Get (size_t index) const 
      { return ArrayType (allocs[index]); }
    };
    class ArraysOneLQ
    {
      csArray<MaxRectanglesCompact*> allocs;
    public:
      typedef ArraysLQ::ArrayType ArrayType;

      ArraysOneLQ (LayoutedQueue& queue)
      {
        for (size_t a = 0; a < queue.maps.GetSize(); a++)
        {
          MaxRectanglesCompact* allocP = queue.maps[a].alloc;
          allocs.Push (allocP);
        }
      }

      size_t GetSize() const { return allocs.GetSize(); }
      ArrayType Get (size_t index) const 
      { return ArrayType (allocs[index]); }
    };
  };

  class SimpleUVObjectLayouter : public LightmapUVObjectLayouter
  {
  public:
    SimpleUVObjectLayouter (SimpleUVFactoryLayouter* parent) : parent (parent)
    {}

    virtual size_t LayoutUVOnPrimitives (PrimitiveArray &prims, 
      size_t groupNum, Sector* sector, const csBitArray& pdBits);

    virtual size_t LayoutUVOnGroup (PrimitiveArray &prims,
      size_t groupNum, Sector* sector, const csBitArray& pdBits);

    virtual void FinalLightmapLayout (PrimitiveArray &prims, size_t layoutID,
      size_t groupNum, ObjectVertexData& vertexData, bool unwrappedByObject, uint& lmID);

    virtual float getFilledRatio(size_t groupNum)
    {
      return surfaceGroup[groupNum].filledRatio;
    }
  protected:
    friend class SimpleUVFactoryLayouter;
    csRef<SimpleUVFactoryLayouter> parent;
    Vector2Array lightmapUVs;
    /* Records which faces were determined to be coplanar
     * - outer array: a set of groups for each lightmaps
     * - next inner array: contains the groups
     * - innermost array: primitive indices
     */
    csArray<csArray<SizeTArray> > coplanarGroups;
    struct PDLayoutedGroup
    {
      uint lmID;
      csArray<csVector2> remaps;
    };
    csHash<PDLayoutedGroup, size_t> pdLayouts;
    csArray<csArray<csVector2> > uvsizes;

    csArray<SimpleUVFactoryLayouter::LayoutTransform*> layoutTransforms;

    struct SurfaceAndRatio
    {
      SurfaceAndRatio():filledRatio(0){}

      // Lightmap surface for group
      csRect rect;
      // Lightmap filled ratio
      float filledRatio;
    };
    csArray<SurfaceAndRatio> surfaceGroup;

    /**
     * Minimum UVs for each *layout* (since a group can appear multiple times
     * on lightmaps)
     */
    csArray<csArray<csVector2> > minuvs;

    void ComputeMinUVs (PrimitiveArray& prims, size_t groupNum,
      csArray<csVector2>& minuvs);

    void LayoutQueuedPrims (PrimitiveArray &prims, size_t groupNum, 
      size_t layoutID, size_t lightmap, const csArray<csVector2>& positions, 
      int dx, int dy);
  };

} // namespace lighter

template<>
class csHashComputer<lighter::SimpleUVFactoryLayouter::SectorAndPDBits>
{
public:
  static uint ComputeHash (
    lighter::SimpleUVFactoryLayouter::SectorAndPDBits const& key)
  {
    uint hash = csHashComputer<csBitArray>::ComputeHash (key.pdBits);
    hash ^= uint (uintptr_t (key.sector));
    return hash;
  }
};

template<>
class csComparator<lighter::SimpleUVFactoryLayouter::SectorAndPDBits,
                   lighter::SimpleUVFactoryLayouter::SectorAndPDBits>
{
public:
  static int Compare (
    lighter::SimpleUVFactoryLayouter::SectorAndPDBits const& r1,
    lighter::SimpleUVFactoryLayouter::SectorAndPDBits const& r2)
  {
    int c = csComparator<lighter::Sector*, lighter::Sector*>::Compare (
      r1.sector, r2.sector);
    if (c == 0)
      c = csComparator<csBitArray, csBitArray>::Compare (r1.pdBits, r2.pdBits);
    return c;
  }
};


#endif // __LIGHTMAPUV_SIMPLE_H__
