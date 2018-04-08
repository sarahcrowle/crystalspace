/*
    CrystalSpace 3D renderer view
    Copyright (C) 1998-2001 by Jorrit Tyberghein

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

#ifndef __CS_CSVIEW_H__
#define __CS_CSVIEW_H__

/**\file
 * Crystal Space 3D renderer view
 */

#include "csextern.h"

#include "csutil/ref.h"
#include "csutil/scf_implementation.h"
#include "csutil/set.h"
#include "igeom/clip2d.h"
#include "iengine/mesh.h"
#include "ivaria/view.h"

class csBox2;
class csPoly2D;

/**
 * Default implementation of a iView.
 */
class CS_CRYSTALSPACE_EXPORT csView : 
  public scfImplementation1<csView, iView>
{
private:
  /// the engine
  csRef<iEngine> Engine;
  /// rendering context
  csRef<iGraphics3D> G3D;

  /// the camera
  csRef<iCamera> Camera;

  /// Rect clipping region (0 if this is a polygon-based clipper)
  csBox2* RectView;
  /// Poly clipping region (0 if this is a rectangular clipper)
  csPoly2D* PolyView;
  /// Normalized rect clipping region (0 if this is a polygon-based clipper)
  csBox2* normalizedRectView;
  /// Normalized poly clipping region (0 if this is a rectangular clipper)
  csPoly2D* normalizedPolyView;
  /// The prepared clipper
  csRef<iClipper2D> Clipper;

  /// Rescale the clipper to deal with a context resize
  void UpdateView ();

  /// State of the automatic resizing
  bool AutoResize;

  CS::Utility::MeshFilter meshFilter;

  // View width.
  int viewWidth;

  // View height.
  int viewHeight;

  // Background
  bool hasBackgroundColor;
  csColor4 backgroundColor;
  csRef<iTextureHandle> backgroundTexture;
  int sx, sy, sw, sh;
  int tx, ty, tw, th;
  uint8 alpha;
  bool tiled;

public:
  /// Constructor.
  csView (iEngine *iEngine, iGraphics3D* ig3d);
  /// Destructor.
  virtual ~csView ();

  virtual iEngine* GetEngine ();
  virtual void SetEngine (iEngine* e);

  virtual iCamera* GetCamera ();
  virtual void SetCamera (iCamera* c);

  virtual iPerspectiveCamera* GetPerspectiveCamera ();
  virtual void SetPerspectiveCamera (iPerspectiveCamera* c);

  virtual iCustomMatrixCamera* GetCustomMatrixCamera ();
  virtual void SetCustomMatrixCamera (iCustomMatrixCamera* c);

  virtual iGraphics3D* GetContext ();
  virtual void SetContext (iGraphics3D *ig3d);

  virtual void SetRectangle (int x, int y, int w, int h, bool restrictToScreen = true);
  virtual void ClearView ();
  virtual void AddViewVertex (int x, int y);
  virtual void RestrictClipperToScreen ();

  virtual void SetAutoResize (bool state) { AutoResize = state; }

  virtual void UpdateClipper();
  virtual iClipper2D* GetClipper ();

  virtual void Draw (iMeshWrapper* mesh = 0);

  virtual CS::Utility::MeshFilter& GetMeshFilter ()
  {
    return meshFilter;
  }

  virtual int GetWidth () const
  {
    return viewWidth;
  }

  virtual int GetHeight () const
  {
    return viewHeight;
  }

  virtual void SetWidth (int w)
  {
    viewWidth = w;
  }

  virtual void SetHeight (int h)
  {
    viewHeight = h;
  }

  virtual csVector2 NormalizedToScreen (const csVector2& pos)
  {
    return csVector2 (
        (pos.x + 1.0f) * float (viewWidth) * 0.5f,
        (pos.y + 1.0f) * float (viewHeight) * 0.5f
	);
  }

  virtual csVector2 ScreenToNormalized (const csVector2& pos)
  {
    return csVector2 (
        pos.x * 2.0f / float (viewWidth) - 1.0f,
        pos.y * 2.0f / float (viewHeight) - 1.0f
	);
  }

  virtual csVector2 Project (const csVector3& v) const;
  virtual csVector3 InvProject (const csVector2& p, float z) const;

  virtual void SetBackgroundColor (csColor4* color);
  virtual const csColor4* GetBackgroundColor () const;

  virtual void SetBackgroundTexture
    (iTextureHandle* texture,
     int sx, int sy, int sw, int sh,
     int tx, int ty, int tw, int th,
     uint8 alpha, bool tiled);
  virtual iTextureHandle* GetBackgroundTexture
    (int& sx, int& sy, int& sw, int& sh,
     int& tx, int& ty, int& tw, int& th,
     uint8& alpha, bool& tiled) const;

  virtual void DrawBackground (iGraphics3D* g3d);
};

#endif // __CS_CSVIEW_H__
