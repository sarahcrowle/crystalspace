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

#ifndef __CS_THOGGMEDIACONT_H__
#define __CS_THOGGMEDIACONT_H__

#include "csutil/array.h"
#include "csutil/refarr.h"
#include "csutil/scf_implementation.h"
#include "iutil/comp.h"
#include "ivideo/videodecode.h"

#include "csutil/custom_new_disable.h"
#include "vorbis/codec.h"
#include "csutil/custom_new_enable.h"

#include "media.h"
#include "theoraaudiomedia.h"
#include "theoravideomedia.h"

#define QUALIFIED_PLUGIN_NAME "crystalspace.videodecode.element.thogg"

using namespace CS::Media;
using namespace CS::Threading;

/**
  * Container for the different streams inside a video file
  */
class TheoraMediaContainer : public scfImplementation2
    <TheoraMediaContainer, iMediaContainer, iComponent>
{
private:
  iObjectRegistry*    _object_reg;
  csRefArray<csMedia>  _media;
  csArray<size_t>     _activeStreams;
  const char*         _pDescription;
  bool                _endOfFile;
  unsigned long       _fileSize;
  float               _timeToSeekTo;
  bool                _waitToFillCache;

  csRef<csTheoraVideoMedia> _activeTheoraStream;
  csRef<csTheoraAudioMedia> _activeVorbisStream;

  size_t  _cacheSize;

  // audio languages
  csArray<MediaLanguage> _languages;

  // audio stream
  csRef<iSndSysStream> _sndstream;

  // audio stream length in seconds
  size_t _audioStreamLength;

  // Mutex
  Mutex     _swapMutex;
  Condition _isSeeking;

public:
  ogg_sync_state          _syncState;
  ogg_page                _oggPage;
  FILE*                   _infile;
  csRef<iTextureHandle>   _target;

private:
  bool  _canSwap;
  bool  _canWrite;
  int   _hasDataToBuffer;
  int   _updateState;

  // Helper for data buffering
  int BufferData (ogg_sync_state *oy);

public:
  TheoraMediaContainer (iBase* parent);
  virtual ~TheoraMediaContainer ();

  //-- iComponent
  virtual bool Initialize (iObjectRegistry*);

  //-- iMediaContainer
  virtual size_t GetMediaCount () const;
  virtual iMedia* GetMedia (size_t index);
  virtual const char* GetDescription () const;
  virtual void SetActiveStream (size_t index);
  virtual bool RemoveActiveStream (size_t index);
  virtual void AutoActivateStreams ();
  virtual iTextureHandle* GetTargetTexture ();
  virtual iSndSysStream* GetTargetAudio ();
  virtual void Update ();
  virtual bool Eof () const;
  virtual void SetPosition (float time);
  virtual float GetPosition () const;
  virtual float GetDuration () const;
  virtual void SwapBuffers ();
  virtual void WriteData ();
  virtual void SetCacheSize(size_t size);
  virtual float GetAspectRatio ();
  virtual void DropFrame ();
  virtual size_t GetLanguageCount () const;
  virtual const MediaLanguage& GetLanguage (size_t index) const;
  virtual void AddLanguage (const MediaLanguage& language);
  virtual void SetCurrentLanguage (const char* identifier);
  virtual void OnPause ();
  virtual void OnPlay ();
  virtual void OnStop ();

  // Add a media to the container
  void AddMedia (csMedia* media);

  // Execute a seek on the active media
  void DoSeek ();

  // Queue a page to the appropriate stream
  void QueuePage (ogg_page *page);

  inline void SetDescription (const char* pDescription)
    { this->_pDescription=pDescription; }

  inline void ClearMedia ()  
    { _media.Empty (); }

  inline unsigned long GetFileSize () const 
    { return _fileSize; }

  inline void SetFileSize (unsigned long size)  
    { _fileSize=size; }
};

/** @} */

#endif // __CS_THOGGMEDIACONT_H__
