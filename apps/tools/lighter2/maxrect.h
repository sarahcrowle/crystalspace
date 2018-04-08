/*
    Copyright (C) 2001-2005 by Jorrit Tyberghein
		  2003-2005 by Frank Richter

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

#ifndef __MAXREC_H__
#define __MAXREC_H__


#include "csutil/blockallocator.h"

#include "csgeom/csrect.h"

namespace lighter
{

#define TESTMAXRECTLIGHTER

#ifndef TESTMAXRECTLIGHTER
  typedef CS::SubRectanglesCompact MaxRectanglesCompact;
  typedef CS::SubRectangles MaxRectangles;
#else
/**
 * A class managing allocations of sub-rectangles. i.e. this class represents
 * a rectangular region from which a client can allocate smaller rectangles
 * until the region is full.
 * Based on the implementation of Jukka Jylänki
 */

class MaxRectangles : public CS::Memory::CustomAllocated
{
public:
  /**
   * Sub-rectangle
   */
  class SubRect
  {
  private :
    csRect rect;
    // Rect splitted by the placement of the rectangle
    csArray<csRect> splittedRect; 
    //
    size_t allocIndex;
  protected :
    friend class MaxRectangles;
    typedef csBlockAllocator<SubRect> SubRectAlloc;
    friend class csBlockAllocator<SubRect>; // SubRectAlloc

    /// Return the area this subrectangle covers
    const csRect& GetRect() const { return rect; }
  };
protected:
  /// Dimensions of this region.
  csRect region;
  /// The total area of the current region
  float regionArea;
  /// Array of the allocated rectangles
  csArray<SubRect*> allocatedRects;
  /// Array of the empty rectangles
  csArray<csRect> freeRects;
  /// Area used by allocated rectangles
  float usedArea;
  //
  csRect minimumRectangle;
  bool minRectDirty;

  bool needPrune;
  size_t notPrunedIndex;

public:
  inline SubRect* AllocSubrect ()
  { 
    SubRect* sr = alloc.Alloc(); 
    return sr;
  }
  void FreeSubrect (SubRect* sr);

  SubRect::SubRectAlloc alloc;

  /// Allocate a new empty region with the given size.
  MaxRectangles (const csRect& region);
  MaxRectangles (const MaxRectangles& other);

  /// Remove this region and sub-regions.
  virtual ~MaxRectangles (){}

  /// Get the rectangle for this region.
  const csRect& GetRectangle () const { return region; }

  /**
   * Free all rectangles in this region.
   */
  virtual void Clear ();

  /**
   * Allocate a new rectangle. Returns 0 if there is no room
   */
  virtual SubRect* Alloc (int w, int h, csRect& rect);

  /**
   * Reclaim a subrectangle, meaning, the space occupied by the subrect can be
   * reused by subsequent Alloc() calls.
   */
  void Reclaim (SubRect* subrect);

  /**
   *
   */
  csRect FindBestPosition(int width, int height,
    int &bestShortSideFit, int &bestLongSideFit) const;

  /**
   *
   */
  csRect FindBestPositionBL(int width, int height,
    int &bestY, int &bestX) const;

  /**
   * Increase the size of the region.
   * You can only grow upwards.
   */
  virtual bool Grow (int newWidth, int newHeight);

  /**
   * Decrease the size of the region.
   * If the region can't be shrunk to the desired size because some already 
   * allocated subrectangles would be cut off \c false is returned. You
   * can check if shrinking to a size is possible by comparing the result of
   * GetMinimumRectangle() with the desired size.
   */
  virtual bool Shrink (int newWidth, int newHeight);

  SubRect* Place(const csRect& rect);

  /**
   * Return the rectangle to which the allocator can be shrunk to at most.
   */
  csRect GetMinimumRectangle ();

  /**
   * Return the area used by allocated subrectangles
   */
  float GetUsedArea() const;

  /**
   * Place the maxrectangles of another allocator into a rectangle allocated
   * from this allocator.
   */
  virtual bool PlaceInto (const MaxRectangles* rectangles, 
    SubRect* subRect);

  /**
   * Returns whether the allocator is empty (ie no rectangles have been 
   * allocated at all or all allocated rectangles have been reclaimed).
   */
  bool IsEmpty () const { return allocatedRects.IsEmpty(); }

  /**
   * For debugging: dump all free rectangles.
   * \param object_reg Object registry. Used to obtain some required plugins.
   * \param tag String appended to the filename dumped to.
   */
  void Dump (iObjectRegistry* object_reg, const char* tag = 0);

  /**
   * For debugging: dump all free rectangles.
   * Works the same as Dump(iObjectRegistry*, const char*), although has no 
   * effect Crystal Space was not compiled in debug mode.
   */
  void Dump (const char* tag = 0);

  virtual float GetFreeArea() const
  {
    return regionArea - usedArea;
  }

private :
  void inline DeleteFreeRect (size_t index)
  {
    freeRects.DeleteIndex(index);
    if (index < notPrunedIndex)
      notPrunedIndex--;
  }

  void PruneFreeList();

  bool SplitFreeRect(const csRect& freeRect,const csRect& allocatedRect);

  void CollapseFreeRect(const csRect& free);

};

class MaxRectanglesCompact : public MaxRectangles
{
  const csRect maxRectSize;
  const float maxRectArea;
  bool growPO2;
  
  inline int NewSize (int amount, int inc)
  { return growPO2 ? csFindNearestPowerOf2 (amount + inc) : amount + inc; }
public:
  MaxRectanglesCompact (const csRect& maxRectSize);
  MaxRectanglesCompact (const MaxRectanglesCompact& other);

  void Clear ();
  SubRect* Alloc (int w, int h, csRect& rect);

  /// Return the upper limit of the rectangle.
  const csRect& GetMaximumRectangle () const { return maxRectSize; }

  /**
   * Enable growing to PO2 dimensions. Means that if an enlargement
   * of the rectangle is necessary it will be to a PO2 dimension.
   * Useful when e.g. the rectangle is to be used as a texture.
   */
  void SetGrowPO2 (bool growPO2) { this->growPO2 = growPO2; }
  /// Return whether growing to PO2 dimensions is enabled.
  bool GetGrowPO2 () const { return growPO2; }

  /// Allocate a rectangle but don't attempt growing to fit.
  SubRect* AllocNoGrow (int w, int h, csRect& rect)
  {
    return MaxRectangles::Alloc (w, h, rect);
  }

  /// Get the maximal free space if the rectangle is not able to expand
  float GetFreeAreaNoGrow() const
  {
    return MaxRectangles::GetFreeArea();
  }

  /// Get the maximal free space of the rectangle
  float GetFreeArea() const
  {
    return maxRectArea - usedArea;
  }
};
#endif

}

#endif // __MAXREC_H__
