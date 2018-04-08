/*
Copyright (C) 2010 by Alin Baciu

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free
Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __VPLLOADER_H__
#define __VPLLOADER_H__

#define QUALIFIED_PLUGIN_NAME "crystalspace.videodecode.loader"

#include "csutil/scf_implementation.h"
#include "iutil/comp.h"
#include "ivideo/videodecode.h"

struct iObjectRegistry;

using namespace CS::Media;

/**
  * This is the implementation for our API and
  * also the implementation of the plugin.
  */
class csVplLoader : public scfImplementation2
    <csVplLoader, iMediaLoader, iComponent>
{
private:
  iObjectRegistry*  _object_reg;

public:
  csVplLoader (iBase* parent);
  virtual ~csVplLoader ();

  // From iComponent.
  virtual bool Initialize (iObjectRegistry*);

  // From iMediaLoader
  virtual csPtr<iMediaContainer> LoadMedia (const char * pFileName,
                                            const char *pDescription=0);
};

#endif // __VPLLOADER_H__
