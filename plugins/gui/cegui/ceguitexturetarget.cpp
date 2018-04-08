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

#include "ceguitexturetarget.h"
#include "ceguitexture.h"

#include "ivideo/txtmgr.h"

CS_PLUGIN_NAMESPACE_BEGIN(cegui)
{
  //----------------------------------------------------------------------------//
  TextureTarget::TextureTarget(Renderer& owner, iObjectRegistry* reg, Texture* texture) :
    RenderTarget(owner, reg),
    d_CEGUITexture(texture)
  {
    d_renderTarget = texture->GetTexHandle();
    int w, h;
    d_renderTarget->GetRendererDimensions(w, h);
    setArea(CEGUI::Rect(0, 0, w, h));
  }

  //----------------------------------------------------------------------------//
  TextureTarget::~TextureTarget()
  {
    d_owner.destroyTexture(*d_CEGUITexture);
  }

  //----------------------------------------------------------------------------//
  bool TextureTarget::isImageryCache() const
  {
    return true;
  }

  //----------------------------------------------------------------------------//
  void TextureTarget::clear()
  {
  }

  //----------------------------------------------------------------------------//
  CEGUI::Texture& TextureTarget::getTexture() const
  {
    return *d_CEGUITexture;
  }

  //----------------------------------------------------------------------------//
  void TextureTarget::declareRenderSize(const CEGUI::Size& sz)
  {
    // exit if current size is enough
    if ((d_area.getWidth() >= sz.d_width) && (d_area.getHeight() >=sz.d_height))
        return;

    d_renderTarget = g3d->GetTextureManager()->CreateTexture
      (sz.d_width, sz.d_height, csimg2D, "rgb8", CS_TEXTURE_2D | CS_TEXTURE_NPOTS);
    d_CEGUITexture->SetTexHandle(d_renderTarget);

    int w, h;
    d_renderTarget->GetRendererDimensions(w, h);
    setArea(CEGUI::Rect(0, 0, w, h));
  }

  //----------------------------------------------------------------------------//
  bool TextureTarget::isRenderingInverted() const
  {
    return false;
  }

//----------------------------------------------------------------------------//

} CS_PLUGIN_NAMESPACE_END(cegui)
