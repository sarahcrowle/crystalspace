/*
    Copyright (C) 2003-2005 by Frank Richter

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

#include "common.h"

#include "maxrect.h"

namespace lighter
{

#ifdef TESTMAXRECTLIGHTER

void MaxRectangles::FreeSubrect (SubRect* sr)
{
  if (sr == 0) return;
  alloc.Free (sr);
}

MaxRectangles::MaxRectangles (const csRect& region)
  : region(region), regionArea(region.Area())
{
  Clear();
}

MaxRectangles::MaxRectangles (const MaxRectangles& other)
  : region(other.region), regionArea(other.region.Area())
{
  Clear();
}

void MaxRectangles::Clear ()
{
  notPrunedIndex = 0;
  freeRects.DeleteAll();
  allocatedRects.DeleteAll();
  usedArea = 0.0f;

  if (!region.IsEmpty())
  {
    freeRects.Push(region);
  }
}

MaxRectangles::SubRect* MaxRectangles::Alloc (int w, int h, csRect& rect)
{
  int score1, score2;
  SubRect* resultRect = 0;

  if (needPrune)
  {
    PruneFreeList();
  }

  csRect foundRect = FindBestPositionBL(w,h,score1,score2);

  if (!foundRect.IsEmpty())
  {
    rect = foundRect;
    resultRect = Place(foundRect);
  }

  return resultRect;
}

void MaxRectangles::Reclaim (MaxRectangles::SubRect* subrect)
{
  size_t numParentsRect = subrect->splittedRect.GetSize();

  for (size_t i=0; i < numParentsRect; i++)
  {
    freeRects.Push(subrect->splittedRect[i]);
  }

  
  allocatedRects.Delete(subrect);
  minRectDirty = true;
  FreeSubrect(subrect);
  usedArea += subrect->rect.Area();

  // We added new rectangles which are highly succeptible
  // to contain some other rectangle but as can be a lot
  // of Reclaim call we delay the rectangle pruning
  needPrune = true;
}

csRect MaxRectangles::FindBestPosition(int width, int height,
    int &bestShortSideFit, int &bestLongSideFit) const
{
  csRect bestRect;

  bestShortSideFit = 99999999;

  for (size_t i=0; i < freeRects.GetSize(); i++)
  {
    int freeWidth = freeRects[i].Width();
    int freeHeight = freeRects[i].Height();

    if ( (freeWidth >= width) && (freeHeight >= height))
    {
      int leftoverHoriz = abs(freeWidth - width);
			int leftoverVert = abs(freeHeight - height);
			int shortSideFit = csMin(leftoverHoriz, leftoverVert);
			int longSideFit = csMax(leftoverHoriz, leftoverVert);

      if ((shortSideFit < bestShortSideFit) || (shortSideFit == bestShortSideFit && longSideFit < bestLongSideFit))
			{
        bestRect.xmin = freeRects[i].xmin;
        bestRect.ymin = freeRects[i].ymin;
        bestRect.SetSize(width,height);

				bestShortSideFit = shortSideFit;
				bestLongSideFit = longSideFit;
			}
    }
  }

  return bestRect;
}

csRect MaxRectangles::FindBestPositionBL(int width, int height, int &bestY, int &bestX) const
{
	csRect bestRect;

  bestY = 99999999;

	for (size_t i=0; i < freeRects.GetSize(); i++)
	{
    int freeWidth = freeRects[i].Width();
    int freeHeight = freeRects[i].Height();

		// Try to place the rectangle in upright (non-flipped) orientation.
		if ((freeWidth >= width) && (freeHeight >= height))
		{
			int topSideY = freeRects[i].ymin + height;
			if (topSideY < bestY || (topSideY == bestY && freeRects[i].xmin < bestX))
			{
				bestRect.xmin = freeRects[i].xmin;
        bestRect.ymin = freeRects[i].ymin;
        bestRect.SetSize(width,height);
        bestY = topSideY;
				bestX = freeRects[i].xmin;
			}
		}
	}
	return bestRect;
}

bool MaxRectangles::Grow(int newWidth, int newHeight)
{
  bool smaller = (newWidth < region.Width ()) || (newHeight < region.Height ());

  if (!smaller)
  {
    int oldWidth =  region.Width();
    int oldHeight =  region.Height();

    int numRectanglesToProcess = freeRects.GetSize();
    for (int i=0; i < numRectanglesToProcess; i++)
    {
      if (freeRects[i].xmax == oldWidth)
      {
        freeRects[i].xmax = newWidth;
      }
      if (freeRects[i].ymax == oldHeight)
      {
        freeRects[i].ymax = newHeight;
      }
    }

    csRect newRectWidth(0,oldHeight,newWidth,newHeight);
    if (!newRectWidth.IsEmpty())
      freeRects.Push(newRectWidth);

    csRect newRectHeight(oldWidth,0,newWidth,newHeight);
    if (!newRectHeight.IsEmpty() && (newRectHeight !=  newRectWidth))
      freeRects.Push(newRectHeight);

    region.SetSize(newWidth,newHeight);
    regionArea = region.Area();
  }

  return !smaller;
}

bool MaxRectangles::Shrink (int newWidth, int newHeight)
{
  size_t numRectanglesToProcess = freeRects.GetSize();
  
  for (size_t i =0; i < numRectanglesToProcess; i++)
  {
    if (freeRects[i].xmax > newWidth)
    {
      freeRects[i].xmax = newWidth;
    }
    if (freeRects[i].ymax > newHeight)
    {
      freeRects[i].ymax = newHeight;
    }

    if (freeRects[i].IsEmpty())
    {
      DeleteFreeRect(i);
      --i;
      --numRectanglesToProcess;
    }
  }
  region.SetSize(newWidth,newHeight);
  regionArea = region.Area();
  return true;
}

MaxRectangles::SubRect* MaxRectangles::Place(const csRect& rect)
{
  size_t numRectanglesToProcess = freeRects.GetSize();

  SubRect* resultRec = alloc.Alloc();
  resultRec->rect = rect;
  resultRec->allocIndex = allocatedRects.Push(resultRec);

  for (size_t i=0; i < numRectanglesToProcess; i++)
  {
    if (SplitFreeRect(freeRects[i], rect))
    {
      resultRec->splittedRect.Push(freeRects[i]);
      DeleteFreeRect(i);
      --i;
      --numRectanglesToProcess;
    }
  }

  usedArea -= rect.Area();
  PruneFreeList();

  minimumRectangle.Union(rect);

  return resultRec;
}

csRect MaxRectangles::GetMinimumRectangle ()
{
  if (minRectDirty)
  {
    minimumRectangle.SetSize(0,0);
    minRectDirty = false;

    int numRectanglesToProcess = allocatedRects.GetSize();
    for (int i=0; i < numRectanglesToProcess; i++)
    {
      minimumRectangle.Union(allocatedRects[i]->rect);
    }
  }
  return minimumRectangle;
}

float MaxRectangles::GetUsedArea() const
{
  return usedArea;
}

bool MaxRectangles::PlaceInto (const MaxRectangles* rectangles, 
  MaxRectangles::SubRect* subRect)
{
  int offsetX = subRect->rect.xmin;
  int offsetY = subRect->rect.xmin;

  // First we remove our subRect
  Reclaim(subRect);

  // Now we place our rect
  csArray<SubRect*>::ConstIterator allocRectIt = 
    rectangles->allocatedRects.GetIterator();

  while (allocRectIt.HasNext())
  {
    csRect newRect = allocRectIt.Next()->rect;
    newRect.Move(offsetX,offsetY);
    Place(newRect);
  }

  return true;
}


static void FillImgRect (uint8* data, uint8 color, int imgW, int imgH, 
			 const csRect& r)
{
  CS_ASSERT((r.xmax <= imgW) && (r.ymax <= imgH));
  int x, y;
  uint8* p = data + (r.ymin * imgW) + r.xmin;
  for (y = r.ymin; y < r.ymax; y++)
  {
    for (x = r.xmin; x < r.xmax; x++)
    {
      *p++ = color;
    }
    p += (imgW - r.Width ());
  }
}

// Debug dump: write some rect distribution into some images.
void MaxRectangles::Dump (iObjectRegistry* object_reg, const char* tag)
{
  csRef<iImageIO> imgsaver =
    csQueryRegistry<iImageIO> (object_reg);
  csRef<iVFS> vfs =
    csQueryRegistry<iVFS> (object_reg);

  if (!imgsaver || !vfs) return;
  
  csRGBpixel pal[256];
  int i;
  for (i = 0; i < 256; i++)
  {
    pal[i].red = csQint ((((i & 0x01) << 2) + ((i & 0x08) >> 2) +
      ((i & 0x40) >> 6)) * (255.0f / 7.0f));
    pal[i].green = csQint ((((i & 0x02) << 1) + ((i & 0x10) >> 3) +
      ((i & 0x80) >> 7)) * (255.0f / 7.0f));
    pal[i].blue = csQint ((((i & 0x04) >> 1) + ((i & 0x20) >> 5)) *
      (255.0f / 3.0f));
  }

  int w = region.Width (), h = region.Height ();

  if ((w == 0) || (h == 0)) return;

  csRGBpixel* newpal = new csRGBpixel[256];
  memcpy (newpal, pal, sizeof (pal));

  csImageMemory* img = 
    new csImageMemory (w, h, (new uint8[w * h]),
    true, CS_IMGFMT_PALETTED8, newpal);

  uint8* data = (uint8*)img->GetImageData ();
  memset (data, 0, w * h);

  newpal = new csRGBpixel[256];
  memcpy (newpal, pal, sizeof (pal));

  csImageMemory* img2 = 
    new csImageMemory (w, h, (new uint8[w * h]),
    true, CS_IMGFMT_PALETTED8, newpal);

  uint8* data2 = (uint8*)img2->GetImageData ();
  memset (data2, 0, w * h);

  //newpal = new csRGBpixel[256];
  //memcpy (newpal, pal, sizeof (pal));

  int c = 0;

  csArray<csRect>::Iterator freeRectIt =  freeRects.GetIterator();

  while(freeRectIt.HasNext())
  {
    csRect rect = freeRectIt.Next();
    rect.xmax = csMin(w,rect.xmax);
    rect.ymax = csMin(h,rect.ymax);
    FillImgRect (data, c + 1, w, h, rect);
    c = (c + 1) % 255;
  }
  
  csArray<SubRect*>::Iterator allocatedRectIt =  allocatedRects.GetIterator();
  while(allocatedRectIt.HasNext())
  {
    csRect& rect = allocatedRectIt.Next()->rect;
    FillImgRect (data2, c + 1, w, h, rect);;
    c = (c + 1) % 255;
  }

  csString tagStr (tag);
  if (tagStr.IsEmpty()) tagStr.Format ("%p", this);

  {
    csString outfn;
    csRef<iDataBuffer> buf = imgsaver->Save (img, "image/png");
    if (buf.IsValid())
    {
      outfn.Format ("/tmp/SubRectangles_dump_%s_r.png", tagStr.GetData());
      if (vfs->WriteFile (outfn, (char*)buf->GetInt8 (), buf->GetSize ()))
      {
        csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY,
          "crystalspace.lighter2.maxrects", "Successfully dumped to %s",
          outfn.GetData ());
      }
      else
      {
        csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY,
          "crystalspace.lighter2.maxrects", "Error dumping to %s",
          outfn.GetData ());
      }
    }
    delete img;

    buf = imgsaver->Save (img2, "image/png");
    if (buf.IsValid())
    {
      outfn.Format ("/tmp/SubRectangles_dump_%s_ar.png", tagStr.GetData());
      if (vfs->WriteFile (outfn, (char*)buf->GetInt8 (), buf->GetSize ()))
      {
        csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY,
          "crystalspace.lighter2.maxrects", "Successfully dumped to %s",
          outfn.GetData ());
      }
      else
      {
        csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY,
	        "crystalspace.lighter2.maxrects", "Error dumping to %s",
	        outfn.GetData ());
      }
    }
    delete img2;
  }
}

// Debug dump: write some rect distribution into some images.
void MaxRectangles::Dump (const char* tag)
{
#ifdef CS_DEBUG
  Dump (iSCF::SCF->object_reg, tag);
#endif
}


void MaxRectangles::PruneFreeList()
{
	/// Go through each pair and remove any rectangle that is redundant.
  for (size_t i = 0; i < freeRects.GetSize(); ++i)
	{
    if (freeRects[i].IsEmpty())
    {
      DeleteFreeRect(i);
      --i;
    }
    else
    {
      size_t j = i+1;
      if ((notPrunedIndex > 0) && (notPrunedIndex > j))
      {
        j = notPrunedIndex;
      }
      bool collapsed = false;
      while ( (!collapsed) && (j < freeRects.GetSize()) )
		  {
        csRect intersection = freeRects[i];
        intersection.Intersect(freeRects[j]);

        if (!intersection.IsEmpty())
        {
          if (intersection.Equal(freeRects[i]))
          {
            collapsed = true;
            DeleteFreeRect(i);
            --i;
          }
          else if (intersection.Equal(freeRects[j]))
          {
            DeleteFreeRect(j);
            --j;
          }
        }

        j++;
		  } // end  while
    }
	}

  // We know that all remaining rectangles don't contain each other
  notPrunedIndex = freeRects.GetSize();
  needPrune = false;
}

bool MaxRectangles::SplitFreeRect(const csRect& freeRect, const csRect& allocatedRect)
{
  // Test if the rectangles even intersect
  if (!freeRect.Intersects(allocatedRect))
    return false;

  if ((allocatedRect.xmin < freeRect.xmax) 
    && (allocatedRect.xmax > freeRect.xmin))
  {
    // New node at the top side of the used node.
    if (allocatedRect.ymin > freeRect.ymin && allocatedRect.ymin < freeRect.ymax)
    {
      csRect newNode = freeRect;
      newNode.ymax = allocatedRect.ymin;
      freeRects.Push(newNode);
    }

    // New node at the bottom side of the used node.
    if (allocatedRect.ymax < freeRect.ymax)
    {
      csRect newNode = freeRect;
      newNode.ymin = allocatedRect.ymax;
      freeRects.Push(newNode);
    }
  }

  if ((allocatedRect.ymin < freeRect.ymax) 
    && (allocatedRect.ymax > freeRect.ymin))
  {
    // New node at the left side of the used node.
    if (allocatedRect.xmin > freeRect.xmin && allocatedRect.xmin < freeRect.xmax)
    {
      csRect newNode = freeRect;
      newNode.xmax = allocatedRect.xmin;
      freeRects.Push(newNode);
    }

    // New node at the right side of the used node.
    if (allocatedRect.xmax < freeRect.xmax)
    {
      csRect newNode = freeRect;
      newNode.xmin = allocatedRect.xmax;
      freeRects.Push(newNode);
    }
  }

	return true;
}

//--------------------------------------------------------------------------

MaxRectanglesCompact::MaxRectanglesCompact(const csRect& maxRectSize)
  : MaxRectangles (csRect (0, 0, 0, 0)),
    maxRectSize (maxRectSize), maxRectArea(maxRectSize.Area()),
    growPO2 (false)
{
}

MaxRectanglesCompact::MaxRectanglesCompact(const MaxRectanglesCompact& other)
  : MaxRectangles (other),
    maxRectSize (other.maxRectSize), maxRectArea(other.maxRectArea),
    growPO2 (other.growPO2)
{
}

static inline int GetDimension (const csRect& r, int side)
{
  return (side == 0) ? r.Width() : r.Height();
}

static inline void SetDimension (csRect& r, int side, int v)
{
  int nw = r.Width();
  int nh = r.Height();
  if (side == 0) 
    nw = v;
  else
    nh = v;
  r.SetSize (nw, nh);
}

MaxRectangles::SubRect* MaxRectanglesCompact::Alloc (int w, int h, csRect& rect)
{
  // Try if rectangle fits already.
  MaxRectangles::SubRect* r = MaxRectangles::Alloc(w,h,rect);

  //If it's doesn't and the rectangle can be enlarged
  if ((r == 0)&&(region != maxRectSize))
  {
    const static int WIDTH = 0;
    const static int HEIGHT = 1;

    // Otherwise try with enlarging first one side, then the other
    int smallerSide = (region.Width() <= region.Height ()) ? WIDTH : HEIGHT;
    for (int s = 0; s < 2; s++)
    {
      csRect oldRect (region);
      int side = s ^ smallerSide;
      csRect newRect (region);
      // Enlarge one side
      int sideLength = ((side == WIDTH) ? w : h);
      // Enlarge one side
      SetDimension (newRect, side, 
        csMin (NewSize (GetDimension (region, side), sideLength),
        GetDimension (maxRectSize, side)));
      // Ensure other side is at least as large as requested
      int otherSideLength = ((side == 0) ? h : w);
      if (GetDimension (newRect, side ^ 1) < otherSideLength)
      {
        SetDimension (newRect, side ^ 1, NewSize (otherSideLength, 0));
      }

      Grow (newRect.Width(), newRect.Height());
      r = MaxRectangles::Alloc (w, h, rect);
      if (r != 0) break;
      // Restore old dimensions
      Shrink (oldRect.Width(), oldRect.Height());
    }
  }

  return r;
}

void MaxRectanglesCompact::Clear ()
{
  region.Set (0, 0, 0, 0);
  MaxRectangles::Clear ();
}

#endif

} // end of namespace lighter
