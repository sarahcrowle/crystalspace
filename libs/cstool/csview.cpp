/*
    CrystalSpace 3D renderer view
    Copyright (C) 1998 by Jorrit Tyberghein

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
#include "csqint.h"
#include "csgeom/polyclip.h"
#include "csgeom/poly2d.h"
#include "ivideo/graph2d.h"
#include "ivideo/graph3d.h"
#include "cstool/csview.h"
#include "cstool/enginetools.h"
#include "iengine/camera.h"
#include "iengine/engine.h"
#include "iengine/rendermanager.h"


csView::csView (iEngine *e, iGraphics3D* ig3d) 
  : scfImplementationType (this),
    Engine (e), G3D (ig3d), RectView (nullptr), PolyView (nullptr),
    normalizedRectView (nullptr), normalizedPolyView (nullptr),
    AutoResize (true), hasBackgroundColor (true),
    backgroundColor (0.f, 0.f, 0.f, 1.f)
{
  csRef<iPerspectiveCamera> pcam = e->CreatePerspectiveCamera ();
  SetPerspectiveCamera(pcam);

  viewWidth = G3D->GetWidth ();
  viewHeight = G3D->GetHeight ();
}

csView::~csView ()
{
  delete RectView;
  delete PolyView;
  delete normalizedRectView;
  delete normalizedPolyView;
}

iEngine* csView::GetEngine ()
{
  return Engine;
}

void csView::SetEngine (iEngine* e)
{
  Engine = e;
}

iCamera *csView::GetCamera ()
{
  return Camera;
}

void csView::SetCamera (iCamera* c)
{
  CS_ASSERT_MSG("Null camera not allowed.", c != NULL); 
  Camera = c;
}

iPerspectiveCamera *csView::GetPerspectiveCamera ()
{
  csRef<iPerspectiveCamera> pcam = scfQueryInterfaceSafe<iPerspectiveCamera>(Camera);
  return pcam;
}

void csView::SetPerspectiveCamera (iPerspectiveCamera* c)
{
  CS_ASSERT_MSG("Null camera not allowed.", c != NULL); 
  Camera = c->GetCamera();
}

iCustomMatrixCamera* csView::GetCustomMatrixCamera ()
{
  csRef<iCustomMatrixCamera> cmcam = scfQueryInterfaceSafe<iCustomMatrixCamera>(Camera);
  return cmcam;
}

void csView::SetCustomMatrixCamera (iCustomMatrixCamera* c)
{
  CS_ASSERT_MSG("Null camera not allowed.", c != nullptr); 
  Camera = c->GetCamera();
}

iGraphics3D* csView::GetContext ()
{
  return G3D;
}

void csView::SetContext (iGraphics3D *ig3d)
{
  G3D = ig3d;
}

void csView::SetRectangle (int x, int y, int w, int h, bool restrictToScreen)
{
  delete PolyView; PolyView = nullptr;
  delete normalizedPolyView; normalizedPolyView = nullptr;
  Clipper = nullptr;

  if (restrictToScreen)
  {
    // Do not allow the rectangle to go out of the screen
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > viewWidth) { w = viewWidth - x; }
    if (y + h > viewHeight) { h = viewHeight - y; }
  }

  if (RectView)
  {
    RectView->Set (x, y, x + w, y + h);
    normalizedRectView->Set (((float) x) / ((float) viewWidth),
			     ((float) y) / ((float) viewHeight),
			     ((float) x + w) / ((float) viewWidth),
			     ((float) y + h) / ((float) viewHeight));
  }
  else
  {
    RectView = new csBox2 (x, y, x + w, y + h);
    normalizedRectView = new csBox2 (((float) x) / ((float) viewWidth),
				     ((float) y) / ((float) viewHeight),
				     ((float) x + w) / ((float) viewWidth),
				     ((float) y + h) / ((float) viewHeight));
  }
}

void csView::ClearView ()
{
  Clipper = nullptr;
  delete RectView; RectView = nullptr;
  delete normalizedRectView; normalizedRectView = nullptr;

  if (PolyView)
  {
    PolyView->MakeEmpty ();
    normalizedPolyView->MakeEmpty ();
  }
}

void csView::AddViewVertex (int x, int y)
{
  if (!PolyView)
  {
    PolyView = new csPoly2D ();
    normalizedPolyView = new csPoly2D ();
  }

  PolyView->AddVertex (x, y);
  normalizedPolyView->AddVertex (((float) x) / ((float) viewWidth),
				 ((float) y) / ((float) viewHeight));

  Clipper = nullptr;
  delete RectView; RectView = nullptr;
  delete normalizedRectView; normalizedRectView = nullptr;
}

void csView::UpdateView ()
{
  if (viewWidth == G3D->GetWidth () && viewHeight == G3D->GetHeight ())
    return;

  viewWidth = G3D->GetWidth ();
  viewHeight = G3D->GetHeight ();

  iPerspectiveCamera* pcamera = GetPerspectiveCamera ();
  if (pcamera)
    pcamera->SetAspectRatio ((float) viewWidth / (float) viewHeight);

  if (PolyView)
  {
    size_t i;
    csVector2 *pverts = PolyView->GetVertices ();
    csVector2 *npverts = normalizedPolyView->GetVertices ();
    size_t InCount = PolyView->GetVertexCount ();

    // Scale the poly
    for (i = 0; i < InCount; i++)
    {
      pverts[i].x = npverts[i].x * viewWidth;
      pverts[i].y = npverts[i].y * viewHeight;
    }
  }
  else if (RectView)
  {
    RectView->Set (csQround (normalizedRectView->MinX () * viewWidth),
		   csQround (normalizedRectView->MinY () * viewHeight),
		   csQround (normalizedRectView->MaxX () * viewWidth),
		   csQround (normalizedRectView->MaxY () * viewHeight));
  }

  Clipper = nullptr;
}

void csView::Draw (iMeshWrapper* mesh)
{
  Engine->GetRenderManager()->RenderView (this);
}

void csView::UpdateClipper ()
{
  if (AutoResize) UpdateView ();

  if (!Clipper)
  {
    if (PolyView)
      Clipper.AttachNew (new csPolygonClipper (PolyView));
    else
    {
      if (!RectView)
      {
        RectView = new csBox2 (0, 0, viewWidth, viewHeight);
        normalizedRectView = new csBox2 (0.f, 0.f, 1.f, 1.f);
      }
      Clipper.AttachNew (new csBoxClipper (*RectView));
    }
  }
}

void csView::RestrictClipperToScreen ()
{
  // rectangular views are automatically restricted to screen borders,
  // so we only have to update polygon-based views
  if (PolyView)
  {
    // Clip the normalized polygon
    size_t InCount = normalizedPolyView->GetVertexCount (), OutCount;
    csBoxClipper bc (0.f, 0.f, 1.f, 1.f);
    csVector2 *TempPoly = new csVector2[InCount + 5];
    uint8 rc = bc.Clip (normalizedPolyView->GetVertices (), InCount , TempPoly, OutCount);
    if (rc != CS_CLIP_OUTSIDE)
    {
      normalizedPolyView->MakeRoom (OutCount);
      normalizedPolyView->SetVertices (TempPoly, OutCount);
      //@@@normalizedPolyView->UpdateBoundingBox ();
    }
    delete [] TempPoly;

    // Copy and scale into the actual polygon
    InCount = normalizedPolyView->GetVertexCount ();
    PolyView->SetVertexCount (InCount);
    csVector2 *pverts = PolyView->GetVertices ();
    csVector2 *npverts = normalizedPolyView->GetVertices ();
     for (size_t i = 0; i < InCount; i++)
    {
      pverts[i].x = npverts[i].x * viewWidth;
      pverts[i].y = npverts[i].y * viewHeight;
    }
  }
}

iClipper2D* csView::GetClipper ()
{
  UpdateClipper ();
  return Clipper;
}

csVector2 csView::Project (const csVector3& v) const
{
  return csEngineTools::NormalizedToScreen
    (Camera->Project (v), viewWidth, viewHeight);
}

csVector3 csView::InvProject (const csVector2& p, float z) const
{
  return Camera->InvProject
    (csEngineTools::ScreenToNormalized (p, viewWidth, viewHeight), z);
}

void csView::SetBackgroundColor (csColor4* color)
{
  hasBackgroundColor = color;
  if (color) backgroundColor = *color;
}

const csColor4* csView::GetBackgroundColor () const
{
  if (!hasBackgroundColor) return nullptr;
  return &backgroundColor;
}

void csView::SetBackgroundTexture
    (iTextureHandle* texture,
     int sx, int sy, int sw, int sh,
     int tx, int ty, int tw, int th,
     uint8 alpha, bool tiled)
{
  backgroundTexture = texture;
  this->sx = sx;
  this->sy = sy;
  this->sw = sw;
  this->sh = sh;
  this->tx = tx;
  this->ty = ty;
  this->tw = tw;
  this->th = th;
  this->alpha = alpha;
  this->tiled = tiled;
}

iTextureHandle* csView::GetBackgroundTexture
    (int& sx, int& sy, int& sw, int& sh,
     int& tx, int& ty, int& tw, int& th,
     uint8& alpha, bool& tiled) const
{
  sx = this->sx;
  sy = this->sy;
  sw = this->sw;
  sh = this->sh;
  tx = this->tx;
  ty = this->ty;
  tw = this->tw;
  th = this->th;
  alpha = this->alpha;
  tiled = this->tiled;
  return backgroundTexture;
}

/// Clip a texture before drawing it
inline void DrawClippedTexture (iGraphics3D* g3d, iTextureHandle* texture,
				int vx, int vy, int vw, int vh,
				int sx, int sy, int sw, int sh,
				int tx, int ty, int tw, int th,
				uint8 alpha)
{
  // Clip on the left
  if (sx < vx)
  {
    int delta = vx - sx;
    float ratio = (float) delta / (float) sw;
    sx = vx;
    sw -= delta;
    int tdelta = csQround (ratio * (float) tw);
    tx += tdelta;
    tw -= tdelta;
  }

  // Clip on the right
  if (sx + sw > vx + vw)
  {
    int delta = sx + sw - vx - vw;
    float ratio = (float) delta / (float) sw;
    sw -= delta;
    int tdelta = csQround (ratio * (float) tw);
    tw -= tdelta;
  }

  // Clip on the top
  if (sy < vy)
  {
    int delta = vy - sy;
    float ratio = (float) delta / (float) sh;
    sy = vy;
    sh -= delta;
    int tdelta = csQround (ratio * (float) th);
    tx += tdelta;
    th -= tdelta;
  }

  // Clip on the bottom
  if (sy + sh > vy + vh)
  {
    int delta = sy + sh - vy - vh;
    float ratio = (float) delta / (float) sh;
    sh -= delta;
    int tdelta = csQround (ratio * (float) th);
    th -= tdelta;
  }

  // Draw the pixmap
  g3d->DrawPixmap (texture, sx, sy, sw, sh, tx, ty, tw, th, alpha);
}

void csView::DrawBackground (iGraphics3D* g3d)
{
  if (!hasBackgroundColor && !backgroundTexture) return;

  // Compute the coordinates of the background area
  int vx, vy, vw, vh;
  csBox2 box = GetClipper ()->GetBoundingBox ();

  vx = box.MinX ();
  vy = viewHeight - box.MaxY ();
  vw = box.MaxX () - box.MinX ();
  vh = box.MaxY () - box.MinY ();

  // Draw the background color
  if (hasBackgroundColor)
  {
    g3d->BeginDraw (CSDRAW_2DGRAPHICS);
    int color = g3d->GetDriver2D ()->FindRGB ((int) (backgroundColor[0] * 255.f),
					      (int) (backgroundColor[1] * 255.f),
					      (int) (backgroundColor[2] * 255.f),
					      (int) (backgroundColor[3] * 255.f));
    g3d->GetDriver2D ()->DrawBox (vx, vy, vw, vh, color);
  }

  // Draw the background texture
  if (backgroundTexture)
  {
    g3d->BeginDraw (CSDRAW_3DGRAPHICS);

    // Non-tiled mode
    if (!tiled) DrawClippedTexture (g3d, backgroundTexture,
				    vx, vy, vw, vh,
				    sx, sy, sw, sh,
				    tx, ty, tw, th, alpha);

    // Tiled mode
    else
    {
      int startx = sx;
      int starty = sy;

      while (startx + sw < vx) startx += sw;
      while (starty + sh < vy) starty += sh;

      while (startx < vx + vw)
      {
	int x = startx;
	int y = starty;

	while (y < vy + vh)
	{
	  DrawClippedTexture (g3d, backgroundTexture,
			      vx, vy, vw, vh,
			      x, y, sw, sh,
			      tx, ty, tw, th, alpha);
	  y += sh;
	}

	startx += sw;
      }
    }
  }
}
