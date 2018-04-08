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

#ifndef __CS_THOGGAUDIOMEDIA_H__
#define __CS_THOGGAUDIOMEDIA_H__

#include "csutil/scf_implementation.h"
#include "isndsys/ss_listener.h"
#include "isndsys/ss_source.h"
#include "isndsys/ss_stream.h"
#include "iutil/comp.h"
#include "ivideo/videodecode.h"
#include "namespacedocs.h"

#include "csutil/custom_new_disable.h"
#include <vorbis/codec.h>
#include "csutil/custom_new_enable.h"

#include "media.h"

using namespace CS::Media;
using namespace CS::SndSys;

class SndSysTheoraStream;

class csTheoraAudioMedia
: public scfImplementation1<csTheoraAudioMedia, scfFakeInterface<iMedia> >,
  public csMedia
{
public:
  CS_LEAKGUARD_DECLARE (csTheoraAudioMedia);

  struct cachedData
  {
    int count;
    short* data;
  };

private:
  iObjectRegistry*      _object_reg;
  char*                 _name;
  float                 _length;

  ogg_stream_state  _streamState;
  vorbis_info       _streamInfo;
  vorbis_dsp_state  _dspState;
  vorbis_block      _vorbisBlock;
  vorbis_comment    _streamComments;
  ogg_packet        _oggPacket;
  ogg_page          *_oggPage;
  int               _vorbis_p;
  FILE              *_infile;
  FILE              *_log;
  bool              _decodersStarted;
  bool              _audiobuf_ready;

  csFIFO<cachedData>  _cache;
  size_t              _cacheSize;

  // The stream used to play audio
  csRef<iSndSysStream> _outputStream;

public:

  // Provide access to the Vorbis specific members
  // Inline because it's faster, although a bit slow
  inline ogg_stream_state*   StreamState ()    { return &_streamState; }
  inline vorbis_info*        StreamInfo ()     { return &_streamInfo; }
  inline vorbis_dsp_state*   DspState ()       { return &_dspState; }
  inline vorbis_block*       VorbisBlock ()    { return &_vorbisBlock; }
  inline vorbis_comment*     StreamComments () { return &_streamComments; }
  inline int&                Vorbis_p ()       { return _vorbis_p; }

public:
  csTheoraAudioMedia ();
  ~csTheoraAudioMedia ();

  //-- iComponent
  virtual bool Initialize (iObjectRegistry*);

  //-- iMedia
  virtual const char* GetName () const;
  virtual const char* GetType () const;
  virtual unsigned long GetFrameCount () const;
  virtual float GetDuration () const;
  virtual double GetPosition () const;
  virtual void CleanMedia ();
  virtual bool Update ();
  virtual void WriteData ();
  virtual void SwapBuffers ();
  virtual void SetCacheSize (size_t size);
  virtual bool HasDataReady () const;
  virtual bool IsCacheFull () const;
  virtual void DropFrame ();

  inline void SetLength (float length)  { this->_length=length; }
  void SetPosition (float time, ogg_sync_state *oy,ogg_page *op,ogg_stream_state *thState);
  void InitializeStream (const char* name, ogg_stream_state &state,
                         vorbis_info &info, vorbis_comment &comments,
                         FILE *source);
};

#endif // __CS_THOGGAUDIOMEDIA_H__
