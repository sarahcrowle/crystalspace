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

#ifndef __VPLPLAYER_H__
#define __VPLPLAYER_H__

#include "csutil/scf_implementation.h"
#include "csutil/threadmanager.h"
#include "iutil/comp.h"
#include "ivideo/videodecode.h"

struct iObjectRegistry;

#define QUALIFIED_PLUGIN_NAME "crystalspace.videodecode.player"

using namespace CS::Media;

/**
  * This is the implementation for our API and
  * also the implementation of the plugin.
  */
class csVplPlayer :  public ThreadedCallable<csVplPlayer>,
  public scfImplementation2 <csVplPlayer, iMediaPlayer, iComponent>
{
private:
  iObjectRegistry*        _object_reg;

  csRef<iMediaContainer>  _mediaFile;

  bool                    _playing;
  bool                    _shouldLoop;
  bool                    _shouldStop;
  bool                    _shouldUpdate;
  bool                    _shouldPlay;

  csRef<iThreadReturn>    _threadInfo;

public:
  csVplPlayer (iBase* parent);
  virtual ~csVplPlayer ();

  // From iComponent.
  virtual bool Initialize (iObjectRegistry*);

  // From iMediaPlayer
  virtual void InitializePlayer (iMediaContainer* media);
  virtual void SetCacheSize (size_t size);
  virtual void SetActiveStream (int index);
  virtual void RemoveActiveStream (int index);
  virtual iTextureHandle* GetTargetTexture ();
  virtual iSndSysStream* GetTargetAudio ();
  THREADED_CALLABLE_DECL (csVplPlayer, Update, csThreadReturn,
                          THREADED, false, false);
  virtual void UpdatePlayer ();
  virtual void Play ();
  virtual void Pause ();
  virtual void Stop ();
  virtual void SetPosition (float time);
  virtual float GetPosition () const;
  virtual float GetDuration () const;
  virtual void SetCyclic (bool shouldLoop);
  virtual bool GetCyclic () const;
  virtual bool IsPlaying ();
  virtual float GetAspectRatio ();
  virtual void SetCurrentLanguage (const char* identifier);

  void WriteData ();

  void SwapBuffers ();

  iObjectRegistry* GetObjectRegistry () const
    { return _object_reg; }

  /**
   * Embedded iEventHandler interface that handles the Frame event
   * in order to write data and swap buffers
   */
  class FrameEventHandler :
    public scfImplementation1<FrameEventHandler,
    iEventHandler>
  {
  private:
    csVplPlayer* parent;

  public:
    FrameEventHandler (csVplPlayer* parent) :
        scfImplementationType (this), parent (parent) { }
        virtual ~FrameEventHandler () { }
        virtual bool HandleEvent (iEvent& ev)
        {
          if (parent->IsPlaying ())
          {
            parent->WriteData ();
            parent->SwapBuffers ();
          }

          return false;
        }
        CS_EVENTHANDLER_PHASE_FRAME ("crystalspace.videodecode.frame")
  };
  csRef<FrameEventHandler> frameEventHandler;

  CS_DECLARE_FRAME_EVENT_SHORTCUTS;
};

#endif // __VPLPLAYER_H__
