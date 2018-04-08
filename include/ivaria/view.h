/*
    Copyright (C) 2000 by Jorrit Tyberghein
    Authored by Brandon Ehle

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

#ifndef __CS_IVARIA_VIEW_H__
#define __CS_IVARIA_VIEW_H__

/**\file
 * Renderer view interface
 */

#include "csutil/scf_interface.h"
#include "cstool/meshfilter.h"

struct iCamera;
struct iClipper2D;
struct iCustomMatrixCamera;
struct iEngine;
struct iGraphics3D;
struct iMeshWrapper;
struct iPerspectiveCamera;
struct iTextureHandle;

/**
 * The iView class represents a viewport and encapsulates the top-level renderer
 * interface.
 *
 * It is basically a camera and a clipper, and is used to define how the view
 * from the camera must be rendered into the render target.
 *
 * The clipper is used to speed up the rendering process by excluding the objects
 * that are not visible. It is returned by GetClipper() and can be defined either
 * through a rectangle (methods SetWidth(), SetHeight() and SetRectangle())
 * or through a custom polygon (methods ClearView(), AddViewVertex() and
 * RestrictClipperToScreen()).
 *
 * The clipper and view dimensions doesn't need to be updated if
 * SetAutoResize() is set to \a true. If no clipper is provided and SetAutoResize()
 * is set to \a true, then a rectangular one will be used with the size of the full
 * screen.
 *
 * The list of meshes to be rendered from the scene can also be restricted
 * using the method GetMeshFilter().
 *
 * A background can be defined for the view, it will be displayed before any other
 * graphics when the view is rendered. The background can be a combination of a
 * color (SetBackgroundColor()) and a texture that can be tiled
 * (SetBackgroundTexture()). The background is clipped against the current clipper,
 * or against the rectangular bounding box of the clipper if it is a custom polygon.
 *
 * If you don't want the current 2D canvas to be overwritten when rendering this
 * view, eg if you want to superpose the rendering of several views, then you can
 * set a null background by calling SetBackgroundColor() with a null pointer.
 *
 * \remarks The view coordinates are vertically mirrored in comparison to screen
 * space, i.e. y=0 is at the bottom of the viewport, y=GetHeight() at the top.
 *
 * Main creators of instances implementing this interface:
 * - Applications using csView.
 *   
 * Main ways to get pointers to this interface:
 * - Application stores it.
 *   
 * Main users of this interface:
 * - Application uses it.
 *   
 */
struct iView : public virtual iBase
{
  SCF_INTERFACE(iView, 3,0,3);

  /// Get the engine handle.
  virtual iEngine* GetEngine () = 0;
  /// Set the engine handle.
  virtual void SetEngine (iEngine* e) = 0;

  /// Get the current camera.
  virtual iCamera* GetCamera () = 0;
  /// Set the current camera.
  virtual void SetCamera (iCamera* c) = 0;

  /**
   * Get the current perspective camera.
   * Can return nullptr if the current camera is not a perspective camera.
   */
  virtual iPerspectiveCamera* GetPerspectiveCamera () = 0;
  /// Set the current camera as a perspective camera.
  virtual void SetPerspectiveCamera (iPerspectiveCamera* c) = 0;

  /// Get the iGraphics3d context of this view
  virtual iGraphics3D* GetContext () = 0;
  /// Set the iGraphics3d context of this view
  virtual void SetContext (iGraphics3D *ig3d) = 0;

  /**
   * Set the clipper as a clipping rectangle.
   * \param x The left position of the rectangle, in pixels.
   * \param y The bottom position of the rectangle, in pixels.
   * \param w The width of the rectangle, in pixels.
   * \param h The height of the rectangle, in pixels.
   * \param restrictToScreen Restrict the rectangle to not be bigger
   * than the screen size.
   */
  virtual void SetRectangle (int x, int y, int w, int h, bool restrictToScreen = true) = 0;
  /// Clear the clipper in order to start building a new polygon-based clipper.
  virtual void ClearView () = 0;
  /**
   * Add a vertex to the clipping polygon (non-rectangular clipping).
   * If the current clipper was a rectangle, then it will be deleted.
   * \param x The X coordinate of the vertex, in pixels.
   * \param y The Y coordinate of the vertex, in pixels.
   */
  virtual void AddViewVertex (int x, int y) = 0;
  /// Clip the polygon clipper to the screen boundaries
  virtual void RestrictClipperToScreen () = 0;

  /// Update the Clipper. This is called automatically by GetClipper().
  virtual void UpdateClipper () = 0;
  /// Return the current clipper. This function may call UpdateClipper().
  virtual iClipper2D* GetClipper () = 0;

  /**
   * Render the scene as seen from the camera.
   * \param mesh This parameter was previously used to restrict the
   * rendering of the view to a single mesh, but it is not used anymore.
   * You should use GetMeshFilter() instead.
   */
  virtual void Draw (iMeshWrapper* mesh = 0) = 0;
  
  /**
   * Enable or disable automatic resizing. When this
   * is true (default) then the view will detect automatically
   * when the canvas size changes and adapt the view
   * and camera automatically (i.e. it will change the view
   * rectangle, the clipper coordinates and the camera aspect ratio).
   * If you don't want that then you can disable this.
   */
  virtual void SetAutoResize (bool state) = 0;

  /**
   * Get the mesh filter used to restrict the meshes that are visible in this
   * view.
   */
  virtual CS::Utility::MeshFilter& GetMeshFilter () = 0;

  /**
   * Get the current custom matrix camera.
   * Can return nullptr if the current camera is not a custom matrix camera.
   */
  virtual iCustomMatrixCamera* GetCustomMatrixCamera () = 0;
  /// Set the current camera as a custom matrix camera.
  virtual void SetCustomMatrixCamera (iCustomMatrixCamera* c) = 0;

  /// Get the view width, in pixels.
  virtual int GetWidth () const = 0;

  /// Get the view height, in pixels.
  virtual int GetHeight () const = 0;

  /// Set the view width, in pixels.
  virtual void SetWidth (int w) = 0;

  /// Set the view height, in pixels.
  virtual void SetHeight (int h) = 0;

  /**
   * Transform a normalized screen space coordinate (-1 to 1) to real pixels in this
   * viewport.
   *
   * \remarks Unlike csEngineTools::NormalizedToScreen(), this method works in
   * view space, not in screen space. That is, the view coordinates are
   * vertically mirrored in comparison to screen space, i.e. y=0 is at the
   * bottom of the viewport, and y=GetHeight() at the top.
   */
  virtual csVector2 NormalizedToScreen (const csVector2& pos) = 0;

  /**
   * Transform a screen space coordinate in pixels to a normalized screen
   * coordinate (-1 to 1).
   *
   * \remarks Unlike csEngineTools::NormalizedToScreen(), this method works in
   * view space, not in screen space. That is, the view coordinates are
   * vertically mirrored in comparison to screen space, i.e. y=0 is at the
   * bottom of the viewport, and y=GetHeight() at the top.
   */
  virtual csVector2 ScreenToNormalized (const csVector2& pos) = 0;

  /**
   * Calculate a projection corrected point for this view, that is the
   * projection of a 3D point expressed in camera space into the 2D
   * screen space.
   * \param v The 3D point to be projected, in camera space coordinates.
   * \return The 2D projection into the screen, in pixels.
   * \sa InvProject() iCamera::Project()
   */
  virtual csVector2 Project (const csVector3& v) const = 0;

  /**
   * Calculate an inverse projection corrected point for this view,
   * that is the inverse projection of a 2D point expressed in screen space
   * into the 3D camera space.
   * \param p The 2D point on the screen, in pixels.
   * \param z The Z component of the projection point, that is the
   * distance between the camera and the plane where the point is projected.
   * \return The 3D projection point, in camera space coordinates.
   * \sa Project() iCamera::InvProject()
   */
  virtual csVector3 InvProject (const csVector2& p, float z) const = 0;

  /**
   * Set the background color of this view. This will be displayed before
   * any other graphics when this view is rendered.
   *
   * If it is set to \a nullptr, then no background color will be displayed
   * and the 2D canvas won't be cleared before the view is rendered. This
   * can be used e.g. in order to superpose the rendering of several views.
   *
   * The default is a black, non transparent, background color.
   */
  virtual void SetBackgroundColor (csColor4* color) = 0;

  /**
   * Get the background color of this view. This will be displayed before
   * any other graphics when this view is rendered.
   */
  virtual const csColor4* GetBackgroundColor () const = 0;

  /**
   * Set the background texture of this view. This will be displayed before
   * any other graphics when this view is rendered (but just after the
   * background color).
   *
   * \remark Unlike the other methods of iView that uses flipped vertical
   * coordinates, this method uses the same logic as iGraphics2D::DrawPixmap(),
   * that is, y=0 is at the top of the screen while y=GetHeight() is at the
   * bottom.
   *
   * \param texture The background texture.
   * \param sx The X coordinate of the left of the texture on screen.
   * \param sy The Y coordinate of the top of the texture on screen.
   * \param sw The width of the texture on screen.
   * \param sh The height of the texture on screen.
   * \param tx The X coordinate of the left of the displayed texture area.
   * \param ty The Y coordinate of the top of the displayed texture area.
   * \param tw The width of the displayed texture area.
   * \param th The height of the displayed texture area.
   * \param alpha The transparency of the texture.
   * \param tiled Whether the texture should be displayed once or repeated
   * in order to cover the whole screen.
   */
  virtual void SetBackgroundTexture
    (iTextureHandle* texture,
     int sx, int sy, int sw, int sh,
     int tx, int ty, int tw, int th,
     uint8 alpha, bool tiled) = 0;

  /**
   * Get the background texture of this view. This will be displayed before
   * any other graphics when this view is rendered (but just after the
   * background color).
   *
   * \param sx The X coordinate of the left of the texture on screen.
   * \param sy The Y coordinate of the top of the texture on screen.
   * \param sw The width of the texture on screen.
   * \param sh The height of the texture on screen.
   * \param tx The X coordinate of the left of the displayed texture area.
   * \param ty The Y coordinate of the top of the displayed texture area.
   * \param tw The width of the displayed texture area.
   * \param th The height of the displayed texture area.
   * \param alpha The transparency of the texture.
   * \param tiled Whether the texture should be displayed once or repeated
   * in order to cover the whole screen.
   * \return The background texture.
   */
  virtual iTextureHandle* GetBackgroundTexture
    (int& sx, int& sy, int& sw, int& sh,
     int& tx, int& ty, int& tw, int& th,
     uint8& alpha, bool& tiled) const = 0;

  /**
   * Draw the background of this view. This is usually the responsability of
   * the iRenderManager to call this method, hence you shouldn't need to call
   * it manually.
   * \param g3d The 3D canvas where to render the background.
   */
  virtual void DrawBackground (iGraphics3D* g3d) = 0;
};

#endif // __CS_IVARIA_VIEW_H__
