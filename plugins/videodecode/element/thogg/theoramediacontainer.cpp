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

#include "cssysdef.h"
#include "isndsys/ss_data.h"
#include "isndsys/ss_loader.h"
#include "isndsys/ss_renderer.h"
#include "iutil/objreg.h"
#include "iutil/plugin.h"
#include "iutil/vfs.h"

#include "theoramediacontainer.h"

SCF_IMPLEMENT_FACTORY (TheoraMediaContainer)

TheoraMediaContainer::TheoraMediaContainer (iBase* parent) :
scfImplementationType (this, parent),
_object_reg (0)
{
  _hasDataToBuffer=1;
}

TheoraMediaContainer::~TheoraMediaContainer ()
{
  // Close the media file
  fclose (_infile);

  // Tear everything down
  // Clean all the streams inside the container
  for (uint i=0;i<_media.GetSize ();i++)
  {
    _media[i]->CleanMedia ();
  }

  ogg_sync_clear (&_syncState);
}

bool TheoraMediaContainer::Initialize (iObjectRegistry* r)
{
  _object_reg=r;
  _endOfFile = false;
  _timeToSeekTo = -1;
  _waitToFillCache=true;
  _target = nullptr;
  _canWrite=false;

  return 0;
}

size_t TheoraMediaContainer::GetMediaCount () const
{
  return _media.GetSize ();
}

iMedia* TheoraMediaContainer::GetMedia (size_t index)
{
  if (index < _media.GetSize ())
    return _media[index];
  return nullptr;
}

const char* TheoraMediaContainer::GetDescription () const
{
  return _pDescription;
}

void TheoraMediaContainer::AddMedia (csMedia* media)
{
  this->_media.Push (media);
}

void TheoraMediaContainer::SetActiveStream (size_t index)
{
  if (index >= _media.GetSize ())
    return;

  bool found = false;
  for (uint i=0;i<_activeStreams.GetSize ();i++)
  {
    if (_media [_activeStreams [i]]->GetType () == _media [index]->GetType ())
    {
      found = true;
      _activeStreams[i] = index;
      _media[index]->SetCacheSize (_cacheSize);
    }
  }

  if (!found)
  {
    _activeStreams.Push (index);
    _media[index]->SetCacheSize (_cacheSize);
  }

  // Store the activated stream in our references, for fast, full access
  if ( strcmp (_media [index]->GetType (),"TheoraVideo") == 0)
  {
    _activeTheoraStream = static_cast<csTheoraVideoMedia*> (_media [index]);
  }

  else if ( strcmp (_media [index]->GetType (),"TheoraAudio") == 0)
  {
    _activeVorbisStream = static_cast<csTheoraAudioMedia*> (_media [index]);
  }
}

bool TheoraMediaContainer::RemoveActiveStream (size_t index)
{
  if ( strcmp (_media [_activeStreams [index]]->GetType (),"TheoraVideo") == 0)
  {
    _activeTheoraStream = nullptr;
  }
  if ( strcmp (_media [_activeStreams [index]]->GetType (),"TheoraAudio") == 0)
  {
    _activeVorbisStream = nullptr;
  }
  return _activeStreams.Delete (index);
}

void TheoraMediaContainer::DropFrame ()
{
  for (uint i=0;i<_activeStreams.GetSize ();i++)
  {
    _media [_activeStreams [i]]->DropFrame ();
  }
}

void TheoraMediaContainer::Update ()
{
  // If processingCache is true, that means we've reached the end
  // of the file, but there still is data in the caches of the
  // active streams which needs processing
  static bool processingCache=false;

  static csTicks frameTime = 0;
  static csTicks lastTime=csGetTicks ();

  if (frameTime==0 && _activeTheoraStream.IsValid ()
      && _activeTheoraStream->GetTargetFPS () != 0)
  {
    frameTime = 1000/_activeTheoraStream->GetTargetFPS ();
  }

  // If a seek is scheduled, do it
  if (_timeToSeekTo!=-1)
  {
    MutexScopedLock lock (_swapMutex);
    DoSeek ();
    _timeToSeekTo=-1;
    _endOfFile=false;

    _isSeeking.NotifyOne ();
  }

  if (!_endOfFile && _activeStreams.GetSize () > 0)
  {
    _updateState=0;
    size_t cacheFull = 0;
    size_t dataAvailable = 0;

    for (size_t i=0;i<_activeStreams.GetSize ();i++)
    {
      // First, we want to know if any stream needs more data.
      // If one stream needs data, updateState will be different from 0
      // If we're at the end of the file but there's still data
      // in the caches, we don't care if a stream needs more data
      if ( _media [_activeStreams [i]]->Update () && !processingCache)
        _updateState++;

      // Next, we want to know if all the active streams have
      // a full cache. if they do, we won't read more data until
      // there's space left in the cache
      if ( _media [_activeStreams [i]]->IsCacheFull ())
        cacheFull++;

      // Next, we want to know if there still is data available.
      // We don't want to stop updating until every frame is treated
      if ( _media [_activeStreams [i]]->HasDataReady ())
        dataAvailable++;
    }

    if (_waitToFillCache && dataAvailable == _activeStreams.GetSize ())
    {
      _waitToFillCache=false;
    }

    if (!_waitToFillCache && !_canWrite && dataAvailable)
    {
      if ( (csGetTicks () - lastTime) >= (frameTime) && ( (csGetTicks () - lastTime) < (frameTime+100)))
      {
        _canSwap=true;
        lastTime=csGetTicks ();
      }
      else if ((csGetTicks () - lastTime) >= (frameTime+100))
      {
        DropFrame ();
        lastTime=csGetTicks ();
      }
    }

    if (processingCache && dataAvailable==0)
      _updateState++;

    // Buffer compressed data on each loop
    if (_updateState>0 && cacheFull!=_activeStreams.GetSize ())
    {
      _hasDataToBuffer = BufferData (&_syncState);
      if (_hasDataToBuffer==0)
      {
        if (dataAvailable==0)
        {
          if (_sndstream.IsValid ())
          {
            _sndstream->Pause ();
          }
          _waitToFillCache=true;
          _endOfFile = true;
          processingCache=false;
        }
        else
        {
          processingCache=true;
        }
      }
      while (ogg_sync_pageout (&_syncState,&_oggPage)>0)
      {
        QueuePage (&_oggPage);
      }

    }
  }
}

void TheoraMediaContainer::QueuePage (ogg_page *page)
{
  // If there are no active stream (i.e. the video file is currently being loaded),
  // queue the page to every stream
  if (_activeStreams.GetSize () == 0)
  {
    for (uint i=0;i<_media.GetSize ();i++)
    {
      if (strcmp (_media[i]->GetType (),"TheoraVideo") == 0)
      {
	csTheoraVideoMedia* buff = static_cast<csTheoraVideoMedia*> (_media[i]);
	ogg_stream_pagein (buff->StreamState (), page);
      }

      else if (strcmp (_media[i]->GetType (),"TheoraAudio") == 0)
      {
	csTheoraAudioMedia* buff = static_cast<csTheoraAudioMedia*> (_media[i]);
	ogg_stream_pagein (buff->StreamState (), page);
      }
    }

  }
  // Otherwise, queue the page only to the active streams
  else
  {
    if (_activeTheoraStream.IsValid ())
    {
      ogg_stream_pagein (_activeTheoraStream->StreamState () ,page);
    }

    if (_activeVorbisStream.IsValid ())
    {
      ogg_stream_pagein (_activeVorbisStream->StreamState () ,page);
    }
  }
}

int TheoraMediaContainer::BufferData (ogg_sync_state *oy)
{
  char *buffer=ogg_sync_buffer (oy,1024);
  int bytes=fread (buffer,1,1024,_infile);
  ogg_sync_wrote (oy,bytes);
  return (bytes);
}

bool TheoraMediaContainer::Eof () const
{
  return _endOfFile;
}

void TheoraMediaContainer::SetPosition (float time)
{
  // Seeking is triggered and will be executed at the beginning of
  // the next update
  if (time<0)
    _timeToSeekTo=0;
  else
    _timeToSeekTo=time;
  _endOfFile=false;
}

void TheoraMediaContainer::DoSeek ()
{
  // In order to seek, an active video stream has to be defined.
  // This is because we first have to seek the video stream and
  // sync the rest of the streams to that frame.
  // This is important, because of the nature of seeking in Theora.

  if (!_activeTheoraStream.IsValid ())
  {
    csReport (_object_reg, CS_REPORTER_SEVERITY_WARNING, QUALIFIED_PLUGIN_NAME,
      "There isn't an active video stream in the media container. Seeking not available.\n");
    return;
  }

  if (_activeTheoraStream->GetDuration () == 0)
  {
    csReport (_object_reg, CS_REPORTER_SEVERITY_WARNING, QUALIFIED_PLUGIN_NAME,
      "The active video stream has a zero length. Seeking not available.\n");
    return;
  }

  // If a video stream is present, seek
  long frame;
  unsigned long frameCount = _activeTheoraStream->GetFrameCount ();
  unsigned long targetFrame = (unsigned long)
    (frameCount * _timeToSeekTo / _activeTheoraStream->GetDuration ());

  // Check if we're seeking outside the video
  if (targetFrame >= _activeTheoraStream->GetFrameCount ())
    targetFrame = _activeTheoraStream->GetFrameCount () - 1;
  else if (targetFrame < 0)
    targetFrame = 0;

  frame = _activeTheoraStream->SeekPage (targetFrame,frameCount,
                                         true,&_syncState,_fileSize);
  if (frame != -1)
    _activeTheoraStream->SeekPage (std::max ((long)0,frame),frameCount,
                                   false,&_syncState,_fileSize);

  // TODO: In case audio from video file is implemented later on, seek to this time
  //float time = ( (float) targetFrame/_activeTheoraStream->GetFrameCount () )
  //                * _activeTheoraStream->GetDuration ();


  // Skip to the frame we need
  while (_activeTheoraStream->Update ()!=0)
  {
    _hasDataToBuffer=BufferData (&_syncState);
    if (_hasDataToBuffer==0)
    {
      _waitToFillCache=true;
      _endOfFile = true;
    }
    while (ogg_sync_pageout (&_syncState,&_oggPage)>0)
    {
      QueuePage (&_oggPage);
    }
  }

  // If a sound stream is present, seek
  if (_sndstream.IsValid ())
  {
    // We want to know if we seek past the end of the audio stream.
    // If we do seek past the end, seek to the end of the stream.
    if (_timeToSeekTo > _audioStreamLength)
      _sndstream->SetPosition (_sndstream->GetFrameCount ());
    // Ortherwise, seek to the required position
    else
    {
      if (_sndstream->GetPauseState () == CS_SNDSYS_STREAM_PAUSED)
        _sndstream->Unpause ();
      _sndstream->SetPosition (_timeToSeekTo*_sndstream->GetRenderedFormat ()->Freq);
    }
  }
}

void TheoraMediaContainer::AutoActivateStreams ()
{
  if (_activeStreams.GetSize () == 0)
  {
    for (size_t i=0;i<_media.GetSize ();i++)
    {
      bool found = false;

      for (size_t j=0;j<_activeStreams.GetSize ();j++)
      {
        if (strcmp (_media[i]->GetType (), _media[_activeStreams[j]]->GetType ())==0)
        {
          found = true;
          break;
        }
      }

      if (!found)
      {
        SetActiveStream (i);
        _media[i]->SetCacheSize (_cacheSize);
      }
    }
  }
}

iTextureHandle* TheoraMediaContainer::GetTargetTexture ()
{
  if (_activeTheoraStream.IsValid ())
    return _activeTheoraStream->GetVideoTarget ();
  else
    return nullptr;
}
iSndSysStream* TheoraMediaContainer::GetTargetAudio ()
{
  if (_sndstream.IsValid ())
    return _sndstream;
  else
    return nullptr;
}

float TheoraMediaContainer::GetPosition () const
{
  if (_activeTheoraStream.IsValid ())
    return _activeTheoraStream->GetPosition ();

  return 0;
}

float TheoraMediaContainer::GetDuration () const
{
  if (_activeTheoraStream.IsValid ())
    return _activeTheoraStream->GetDuration ();

  return 0;
}

void TheoraMediaContainer::OnPause ()
{
  if (_sndstream.IsValid ())
  {
    _sndstream->Pause ();
  }
}

void TheoraMediaContainer::OnPlay ()
{
  if (_sndstream.IsValid ())
  {
    _sndstream->Unpause ();
  }
}

void TheoraMediaContainer::OnStop ()
{
  if (_sndstream.IsValid ())
  {
    _sndstream->Pause ();
    _sndstream->SetPosition (0);
  }
}

void TheoraMediaContainer::SwapBuffers ()
{
  static csTicks time =0;
  static csTicks total=0;

  csTicks timeTaken = csGetTicks ()-time;
  total+=timeTaken;
  MutexScopedLock lock (_swapMutex);

  // Wait until we have an item
  while (_timeToSeekTo != -1)
    _isSeeking.Wait (_swapMutex);

  time = csGetTicks ();

  if (_canSwap)
  {
    total = 0;
    _canSwap=false;
    _canWrite=true;
    for (size_t i =0;i<_activeStreams.GetSize ();i++)
    {
      _media[_activeStreams[i]]->SwapBuffers ();
    }
  }
}

void TheoraMediaContainer::WriteData ()
{
  if (_waitToFillCache)
    return;
  if (!_canSwap && _canWrite)
  {
    if (_updateState==0)
    {
      _canWrite=false;
      if (_activeTheoraStream.IsValid ())
        _activeTheoraStream->WriteData ();
      if (_activeVorbisStream.IsValid ())
        _activeVorbisStream->WriteData ();
    }
  }
}

void TheoraMediaContainer::SetCacheSize (size_t size)
{
  _cacheSize = size;
}

float TheoraMediaContainer::GetAspectRatio ()
{
  if (_activeTheoraStream.IsValid ())
    return _activeTheoraStream->GetAspectRatio ();
  return 1;
}

size_t TheoraMediaContainer::GetLanguageCount () const
{
  return _languages.GetSize ();
}

const MediaLanguage& TheoraMediaContainer::GetLanguage (size_t index) const
{
  return _languages[index];
}

void TheoraMediaContainer::AddLanguage (const MediaLanguage& language)
{
  //_languages.Push (language);
  _languages.Push (MediaLanguage (language.name, language.path));
}

void TheoraMediaContainer::SetCurrentLanguage (const char* identifier)
{
  bool found = false;

  // Search for the selected language
  for (size_t i = 0; i < _languages.GetSize (); i++)
  {
    if (strcmp (_languages[i].name,identifier) == 0)
    {
      csRef<iVFS> vfs = csQueryRegistry<iVFS> (_object_reg);
      csRef<iSndSysLoader> sndloader = csQueryRegistry<iSndSysLoader> (_object_reg);
      csRef<iSndSysRenderer> sndrenderer = csQueryRegistry<iSndSysRenderer> (_object_reg);

      csRef<iDataBuffer> soundbuf = vfs->ReadFile (_languages[i].path);
      if (!soundbuf)
      {
        csReport (_object_reg, CS_REPORTER_SEVERITY_ERROR, QUALIFIED_PLUGIN_NAME,
          "Can't load file %s!\n", _languages[i].path.GetData ());
        return;
      }

      csRef<iSndSysData> snddata = sndloader->LoadSound (soundbuf);
      if (!snddata)
      {
        csReport (_object_reg, CS_REPORTER_SEVERITY_ERROR, QUALIFIED_PLUGIN_NAME,
          "Can't load sound %s!\n", _languages[i].path.GetData ());
        return;
      }

      _sndstream = sndrenderer->CreateStream (snddata,CS_SND3D_DISABLE );
      if (!_sndstream)
      {
        csReport (_object_reg, CS_REPORTER_SEVERITY_ERROR, QUALIFIED_PLUGIN_NAME,
          "Can't create stream for %s!\n", _languages[i].path.GetData ());
        return;
      }
      _sndstream->SetLoopState (CS_SNDSYS_STREAM_DONTLOOP);

      // Store the audio stream length
      found = true;
      _audioStreamLength = _sndstream->GetFrameCount ()/_sndstream->GetRenderedFormat ()->Freq;
      break;
    }
  }

  if (!found)
    csReport (_object_reg, CS_REPORTER_SEVERITY_ERROR, QUALIFIED_PLUGIN_NAME,
              "Can't set '%s': language not found.\n", identifier);
}
