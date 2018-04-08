/*
Copyright (C) 2011 by Alin Baciu

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

#ifndef __CS_MEDIA_H__
#define __CS_MEDIA_H__

#include "ivideo/videodecode.h"

#include "csutil/custom_new_disable.h"
#include <vorbis/codec.h>
#include "csutil/custom_new_enable.h"

using namespace CS::Media;

class csMedia : public virtual iMedia
{
public:
  csMedia () {}
  virtual ~csMedia () {}

  virtual void CleanMedia () = 0;
  virtual bool Update () = 0;
  virtual void WriteData () = 0;
  virtual void SwapBuffers () = 0;
  virtual void SetCacheSize (size_t size) = 0;
  virtual bool HasDataReady () const = 0;
  virtual bool IsCacheFull () const = 0;
  virtual void DropFrame () = 0;
};

#endif // __CS_MEDIA_H__
