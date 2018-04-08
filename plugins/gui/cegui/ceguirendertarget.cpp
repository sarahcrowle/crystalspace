/*
    Copyright (C) 2005 Dan Hardfeldt, Seth Yastrov and Jelle Hellemans

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

#include "ceguiimports.h"
#include "ceguirendertarget.h"
#include "ceguigeometrybuffer.h"

CS_PLUGIN_NAMESPACE_BEGIN(cegui)
{
  //----------------------------------------------------------------------------//
  RenderTarget::RenderTarget(Renderer& owner, iObjectRegistry* reg) :
    obj_reg (reg),
    d_owner(owner),
    d_area(0, 0, 0, 0),
    d_renderTarget(0)
  {
    g3d = csQueryRegistry<iGraphics3D> (obj_reg);
  }

  //----------------------------------------------------------------------------//
  RenderTarget::~RenderTarget()
  {
  }

  //----------------------------------------------------------------------------//
  void RenderTarget::draw(const CEGUI::GeometryBuffer& buffer)
  {
    buffer.draw();
  }

  //----------------------------------------------------------------------------//
  void RenderTarget::draw(const CEGUI::RenderQueue& queue)
  {
    queue.draw();
  }

  //----------------------------------------------------------------------------//
  void RenderTarget::setArea(const CEGUI::Rect& area)
  {
    d_area = area;
  }

  //----------------------------------------------------------------------------//
  const CEGUI::Rect& RenderTarget::getArea() const
  {
    return d_area;
  }

  //----------------------------------------------------------------------------//
  void RenderTarget::activate()
  {
    if (!d_renderTarget) return;

    // Activate the rendering to target
    previousTarget = g3d->GetRenderTarget ();
    g3d->SetRenderTarget (d_renderTarget);
    g3d->BeginDraw (CSDRAW_2DGRAPHICS | CSDRAW_CLEARSCREEN);
  }

  //----------------------------------------------------------------------------//
  void RenderTarget::deactivate()
  {
    if (!d_renderTarget) return;

    // Deactivate the rendering to target and restore the previous target
    g3d->FinishDraw ();
    g3d->SetRenderTarget (previousTarget);
  }

  //----------------------------------------------------------------------------//
  void RenderTarget::unprojectPoint(const CEGUI::GeometryBuffer& buff,
                                    const CEGUI::Vector2& p_in, CEGUI::Vector2& p_out) const
  {
  }

  //----------------------------------------------------------------------------//

} CS_PLUGIN_NAMESPACE_END(cegui)
