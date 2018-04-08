
#ifndef __CS_VIDEORECORDER_H__
#define __CS_VIDEORECORDER_H__

#include "iutil/comp.h"
#include "iutil/eventh.h"
#include "iutil/virtclk.h"
#include "ivaria/movierecorder.h"
#include "ivaria/reporter.h"
#include "ivideo/graph2d.h"
#include "isndsys/ss_renderer.h"
#include "csutil/eventhandlers.h"
#include "csutil/weakref.h"
#include "csutil/scf_implementation.h"
#include "csutil/cfgacc.h"
#include "cstool/numberedfilenamehelper.h"	

#include "video-encoder.h"

CS_PLUGIN_NAMESPACE_BEGIN(VideoRecorder)
{

/**
* Video recording plugin.
*/
class csVideoRecorder : public scfImplementation2<csVideoRecorder, iMovieRecorder, iComponent>
{
  CS_DECLARE_EVENT_SHORTCUTS;
  
  enum Mode
  { 
	/// Variable framerate. Engine runs at normal speed. Video recorder tries to record
    /// as many frames as possible but framerate will not exceed maximum framerate.
	MODE_VFR,
	
	/// Constant framerate. Engine runs at normal speed. Video recorder will
	/// drop (duplicate) frames if it can't record as many frames as necessary.
	MODE_CFR_DROP,
	
	/// Constant framerate, engine will be speeded up or slowed down as necessary.
	MODE_FORCED_CFR,
  };

  iObjectRegistry* object_reg;
  csConfigAccess config;
  csRef<iReporter> reporter;
  csRef<iGraphics2D> g2d;
  csRef<iVFS> VFS;
  csRef<iSndSysRenderer> soundRenderer;
  csRef<iSndSysRendererSoftware> softSoundRenderer;
  csRef<iVirtualClock> realClock;

  bool initialized;
  bool paused;
  bool recording;
  bool recordSound;
  Mode mode;

  // pseudo time used with forced-cfr mode
  csMicroTicks currentMicroTicks;
  csMicroTicks elapsedMicroTicks;
  csTicks elapsedTicks;

  csMicroTicks startTick;
  csMicroTicks pauseTick;
  csMicroTicks prevTick;
  
  csRef<VideoEncoder> encoder;
  
  /// format of the movie filename (e.g. "/this/crystal000")
  CS::NumberedFilenameHelper filenameHelper;
  csString filenameFormat;
  csString filename;
  /// If this is set then it will be used instead of filenameFormat
  csString forcedFilename;

  /// Encoding parameters
  int forcedWidth, forcedHeight;
  int width, height;
  int framerate;
  csString extension;
  
  csSndSysSoundFormat soundFormat;

  csMicroTicks lastSampleRead;
  csArray<uint16> sampleBuffer;
 
  struct KeyCode
  {
    utf32_char code;
    bool shift, alt, ctrl;

	bool operator==(KeyCode other)
	{
		return (code == other.code) && (shift == other.shift) && (alt == other.alt)  && (ctrl == other.ctrl);
	}
  };
  KeyCode keyRecord, keyPause;

  /// Handle keydown event
  bool OnKeyDown(KeyCode key);
  /// Handle frame end event
  void OnFrameEnd();
  
  /// Parse keyname and return key code + modifier status (from bugplug).
  KeyCode GetKeyCode (const char* keystring);

  bool InitPlugin();
  
  /// Report message
  void Report (int severity, const char* msg, ...);
  
  /// Called from our replacement of iVirtualClock
  void AdvanceClock();
  
  iObjectRegistry* GetObjectRegistry() const
  { return object_reg; }

  /**
   * Embedded iEventHandler interface that handles keyboard events
   */
  class KeyEventHandler : 
    public scfImplementation1<KeyEventHandler, iEventHandler>
  {
    csVideoRecorder* parent;

  public:
    KeyEventHandler (csVideoRecorder* parent) : scfImplementationType (this), parent (parent) {}
    virtual ~KeyEventHandler () {}
    virtual bool HandleEvent (iEvent& event);

    CS_EVENTHANDLER_NAMES("crystalspace.videorecorder.keyboard")
    CS_EVENTHANDLER_NIL_CONSTRAINTS
  };
  KeyEventHandler keyEventHandler;

  /**
  * Embedded iEventHandler interface that handles frame end events.
  */
  class FrameEventHandler : 
    public scfImplementation1<FrameEventHandler, iEventHandler>
  {
    csVideoRecorder* parent;

  public:
    FrameEventHandler (csVideoRecorder* parent) : scfImplementationType (this), parent (parent) {}
    virtual ~FrameEventHandler () {}
    virtual bool HandleEvent (iEvent& event)
    {
      if (event.Name == parent->Frame)
        parent->OnFrameEnd();
      return false; // we are just observing, we don't handle anything, so return false
    }
    CS_EVENTHANDLER_PHASE_FRAME("crystalspace.videorecorder.frame")
  };
  FrameEventHandler frameEventHandler;

   /**
   * Embedded iVirtualClock. Real clock will be replaced with this.
   * In forced cfr mode this allows us to alter time.
   */
  class FakeClock : public scfImplementation1<FakeClock, iVirtualClock>
  {
    csWeakRef<csVideoRecorder> parent;

  public:
    FakeClock (csVideoRecorder* parent) :
	  scfImplementationType (this),
      parent (parent)
	{}

    virtual ~FakeClock() {}

    virtual void Advance ()
    {
      if (parent) parent->AdvanceClock();
    }
    virtual void Suspend ()
    {
      if (parent) parent->realClock->Suspend();
    }
    virtual void Resume ()
    {
      if (parent) parent->realClock->Resume();
    }

    virtual csMicroTicks GetCurrentMicroTicks () const
    {
      return parent? parent->currentMicroTicks : 0;
    }
    virtual csTicks GetCurrentTicks () const
    {
      return parent? parent->currentMicroTicks/1000 : 0;
    }
    virtual csMicroTicks GetElapsedMicroTicks () const
    {
      return parent? parent->elapsedMicroTicks : 0;
    }
    virtual csTicks GetElapsedTicks () const
    {
      return parent? parent->elapsedTicks : 0;
    }
    virtual float GetElapsedSeconds ()
    {
      return parent? parent->elapsedMicroTicks*0.000001f : 0.0f;
    }
  };
  csRef<FakeClock> fakeClock;
  
  class SoundRecorder : 
    public scfImplementation1<SoundRecorder, iSndSysSoftwareOutputFilter>
  {
    csWeakRef<csVideoRecorder> parent;

  public:
    SoundRecorder (csVideoRecorder* parent) : scfImplementationType (this), parent (parent) {}
    virtual ~SoundRecorder () {}

	virtual void DeliverData (const csSoundSample *SampleBuffer, size_t Frames)
	{
		if (parent && parent->encoder && parent->recording && !parent->paused)
			parent->encoder->DeliverSoundData(SampleBuffer, Frames);
	}
	
	virtual bool FormatNotify (const csSndSysSoundFormat *pSoundFormat)
	{
		if (!parent || pSoundFormat->Bits != 16)
			return false;
		parent->soundFormat = *pSoundFormat;
		return true;
	}
  };
  SoundRecorder soundRecorder;

public:
  csVideoRecorder (iBase* parent);
  virtual ~csVideoRecorder ();
  
  //-- iComponent
  virtual bool Initialize(iObjectRegistry *iobject_reg);
  
  //-- iMovieRecorder
  virtual bool 	IsPaused () const;
  virtual bool 	IsRecording () const;
  virtual void 	Pause ();
  virtual void 	SetFilenameFormat (const char *format);
  virtual void 	SetRecordingFile (const char *filename);
  virtual void 	Start ();
  virtual void 	Stop ();
  virtual void 	UnPause ();
};

}
CS_PLUGIN_NAMESPACE_END(VideoRecorder)

#endif //__CS_VIDEORECORDER_H__