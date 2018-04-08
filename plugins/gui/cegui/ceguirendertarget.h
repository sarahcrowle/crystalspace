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

#ifndef _CEGUIRENDERTARGET_H_
#define _CEGUIRENDERTARGET_H_

#include "ceguiimports.h"
#include "ceguirenderer.h"


CS_PLUGIN_NAMESPACE_BEGIN(cegui)
{
  /// Intermediate RenderTarget implementing common parts for CS engine.
  class RenderTarget : public virtual CEGUI::RenderTarget
  {
  public:
    /// Constructor
    RenderTarget(Renderer& owner, iObjectRegistry* reg);

    /// Destructor
    virtual ~RenderTarget();

    // implement parts of CEGUI::RenderTarget interface
    void draw(const CEGUI::GeometryBuffer& buffer);
    void draw(const CEGUI::RenderQueue& queue);
    void setArea(const CEGUI::Rect& area);
    const CEGUI::Rect& getArea() const;
    void activate();
    void deactivate();
    void unprojectPoint(const CEGUI::GeometryBuffer& buff,
      const CEGUI::Vector2& p_in, CEGUI::Vector2& p_out) const;

  protected:
    iObjectRegistry* obj_reg;
    csRef<iGraphics3D> g3d;
    /// Renderer object that owns this RenderTarget
    Renderer& d_owner;
    /// holds defined area for the RenderTarget
    CEGUI::Rect d_area;
    /// CS render target that we are effectively wrapping
    csRef<iTextureHandle> d_renderTarget;
    /// The render target that was active before we start drawing
    csRef<iTextureHandle> previousTarget;
  };

} CS_PLUGIN_NAMESPACE_END(cegui)

#endif  // end of guard _CEGUIRENDERTARGET_H_
