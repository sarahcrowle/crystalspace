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

#include "cssysdef.h"
#include "iutil/objreg.h"
#include "iutil/plugin.h"

#include "vplplayer.h"

SCF_IMPLEMENT_FACTORY (csVplPlayer)

csVplPlayer::csVplPlayer (iBase* parent) :
scfImplementationType (this, parent),
  _object_reg (0), _playing (false), _shouldLoop (true), _shouldStop (false),
  _shouldUpdate (false), _shouldPlay (false)
{
}

csVplPlayer::~csVplPlayer ()
{
  // Unregister the Frame event listener
  if (frameEventHandler)
  {
    csRef<iEventQueue> q (csQueryRegistry<iEventQueue> (_object_reg));
    if (q)
      q->RemoveListener (frameEventHandler);
  }
}

bool csVplPlayer::Initialize (iObjectRegistry* r)
{
  _object_reg = r;
  return true;
}

void csVplPlayer::InitializePlayer (iMediaContainer* media)
{
  if (!media)
  {
    csReport (_object_reg, CS_REPORTER_SEVERITY_WARNING, QUALIFIED_PLUGIN_NAME,
              "Media container is not valid!");
    return;
  }

  _mediaFile = media;

  CS_INITIALIZE_FRAME_EVENT_SHORTCUTS (_object_reg);
  csRef<iEventQueue> q (csQueryRegistry<iEventQueue> (_object_reg));

  // Register the Frame event listener
  if (!frameEventHandler)
  {
    frameEventHandler.AttachNew (new FrameEventHandler (this));
  }
  if (q != 0)
  {
    csEventID events[2] = { Frame, CS_EVENTLIST_END };
    q->RegisterListener (frameEventHandler, events);
  }
  _shouldStop = false;
  _shouldUpdate=false;
  _shouldPlay=false;
}

void csVplPlayer::UpdatePlayer ()
{
  if (!_threadInfo.IsValid ())
  {
    _shouldUpdate = true;
    _threadInfo = Update ();
  }
}

void csVplPlayer::SetCacheSize (size_t size)
{
  if (_mediaFile.IsValid ())
  {
    if (size < 1)
      _mediaFile->SetCacheSize (1);
    else
      _mediaFile->SetCacheSize (size);
  }
}

void csVplPlayer::SetActiveStream (int index)
{
  if (_mediaFile.IsValid ())
  {
    if (index == -1)
      _mediaFile->AutoActivateStreams ();
    else
      _mediaFile->SetActiveStream (index);
  }
}

void csVplPlayer::RemoveActiveStream (int index)
{
  if (_mediaFile.IsValid ())
    _mediaFile->RemoveActiveStream (index);
}

iTextureHandle* csVplPlayer::GetTargetTexture ()
{
  return _mediaFile->GetTargetTexture ();
}

iSndSysStream* csVplPlayer::GetTargetAudio ()
{
  return _mediaFile->GetTargetAudio ();
}

void csVplPlayer::SetCurrentLanguage (const char* identifier)
{
  if (_mediaFile.IsValid ())
    _mediaFile->SetCurrentLanguage (identifier);
}

THREADED_CALLABLE_IMPL (csVplPlayer, Update)
{
  while (_shouldUpdate)
  {
    if (_shouldPlay)
    {
      Play ();
      _shouldPlay=false;
    }
    if (_playing)
    {
      if (_shouldStop)
      {
        // Stop playing
        _playing=false;
      }
      if (_mediaFile.IsValid ())
      {
        if (_mediaFile->Eof ())
        {
          if (_shouldLoop)
          {
            // Seek back to the beginning of the stream
            SetPosition (0.0f);
            _mediaFile->Update ();
            _playing=true;
          }
          else
            _playing=false;
        }

        _mediaFile->Update ();
      }

    }
    // If the media isn't playing, we don't want to use up the thread
    else
      csSleep (100);
  }

  return true;
}

void csVplPlayer::Play ()
{
  if (!_threadInfo.IsValid ())
  {
    _shouldPlay=true;
    return;
  }

  _playing=true;

  if (_mediaFile.IsValid ())
  {
    _mediaFile->OnPlay ();
  }
  if (_shouldStop)
  {
    _shouldStop=false;
    if (_mediaFile.IsValid ())
    {
      _mediaFile->SetPosition (0.0f);
      _mediaFile->OnPlay ();
    }
  }
}

void csVplPlayer::Pause ()
{
  _playing=false;
  _shouldUpdate = false;
  if (_mediaFile.IsValid ())
  {
    _mediaFile->OnPause ();
  }
}

void csVplPlayer::Stop ()
{
  _shouldStop = true;
  if (_mediaFile.IsValid ())
  {
    _mediaFile->OnStop ();
  }
}

bool csVplPlayer::IsPlaying ()
{
  return _playing;
}

void csVplPlayer::SetPosition (float time)
{
  if (_mediaFile.IsValid ())
  {
    _mediaFile->SetPosition (time);
    _playing = true;
  }
}

float csVplPlayer::GetPosition () const
{
  return _mediaFile->GetPosition ();
}

void csVplPlayer::SetCyclic (bool cyclic)
{
  _shouldLoop = cyclic;
}

bool csVplPlayer::GetCyclic () const
{
  return _shouldLoop;
}

float csVplPlayer::GetDuration () const
{
  return _mediaFile->GetDuration ();
}

void csVplPlayer::SwapBuffers ()
{
  _mediaFile->SwapBuffers ();
}

void csVplPlayer::WriteData ()
{
  _mediaFile->WriteData ();
}

float csVplPlayer::GetAspectRatio ()
{
  return _mediaFile->GetAspectRatio ();
}
