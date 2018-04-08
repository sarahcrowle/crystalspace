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

#ifndef __THOGGLOADER_H__
#define __THOGGLOADER_H__

#include "csutil/scf_implementation.h"
#include "csutil/nobjvec.h"
#include "iutil/comp.h"
#include "ivideo/videodecode.h"

#include "csutil/custom_new_disable.h"
#include <vorbis/codec.h>
#include "csutil/custom_new_enable.h"

#include "theoramediacontainer.h"

#define QUALIFIED_PLUGIN_NAME "crystalspace.videodecode.element.thogg"

struct iObjectRegistry;

using namespace CS::Media;

/*#ifdef WIN32
  #pragma comment (lib,"libtheora_static.lib")
  #pragma comment (lib,"ogg.lib")
  #pragma comment (lib,"vorbis.lib")
#endif*/

class csThOggLoader : public scfImplementation2
<csThOggLoader, iMediaLoader, iComponent>
{
private:

  iObjectRegistry* _object_reg;
  csRef<iTextureManager> _texManager;
  csRef<iGraphics3D>  _g3d;

  // Ogg stuff
  ogg_sync_state   _oy;
  ogg_page         _og;

  // Theora stuff
  th_info      ti;
  th_comment   tc;

  // Vorbis stuff
  vorbis_info      _vi;
  vorbis_comment   _vc;

  // Media data
  csString            _path;        // video file path
  FILE *              _infile;      // video file
  csArray<MediaLanguage>   _languages;   // array of languages
                                    // (audio file paths and names)

public:
  csThOggLoader (iBase* parent);
  virtual ~csThOggLoader ();

  //-- iComponent
  virtual bool Initialize (iObjectRegistry*);

  virtual csPtr<iMediaContainer> LoadMedia
    (const char * pFileName, const char *pDescription=0);

private:

  /* Helper; just grab some more compressed bitstream
     and sync it for page extraction */
  int BufferData (ogg_sync_state *oy);

  bool StartParsing (TheoraMediaContainer* container);
  bool ParseHeaders (TheoraMediaContainer* container);
  void ComputeStreamLength (TheoraMediaContainer* container);
};

#endif // __THOGGLOADER_H__
