
#include "cssysdef.h"
#include "iutil/eventq.h"
#include "iutil/cfgfile.h"
#include "igraphic/image.h"
#include "csutil/event.h"
#include "csutil/eventnames.h"
#include "csgfx/imagememory.h"

#include "video-recorder.h"

CS_PLUGIN_NAMESPACE_BEGIN(VideoRecorder)
{
SCF_IMPLEMENT_FACTORY(csVideoRecorder);

csVideoRecorder::csVideoRecorder (iBase* parent) : 
  scfImplementationType (this, parent),
  keyEventHandler(this),
  frameEventHandler(this),
  soundRecorder(this)
{
  object_reg = NULL;

  initialized = false;
  paused = false;
  recording = false;
  recordSound = false;

  currentMicroTicks = 0;
  elapsedMicroTicks = 0;
  elapsedTicks = 0;

  lastSampleRead = 0;

  encoder = NULL;
  fakeClock = NULL;
}

bool csVideoRecorder::Initialize (iObjectRegistry* object_reg)
{
  this->object_reg = object_reg;
  CS_INITIALIZE_EVENT_SHORTCUTS (object_reg);

  // Register keyboard and frame events handler.
  csRef<iEventQueue> q (csQueryRegistry<iEventQueue> (object_reg));
  if (q)
  {
    csEventID events1[2] = { KeyboardEvent, CS_EVENTLIST_END };
    q->RegisterListener (&keyEventHandler, events1);

    csEventID events2[2] = { Frame, CS_EVENTLIST_END };
    q->RegisterListener (&frameEventHandler, events2);
  }

  soundRenderer = csQueryRegistry<iSndSysRenderer> (object_reg);
  if (soundRenderer)
  {
	if (!soundRenderer->IsLoopback())
	{
	  softSoundRenderer = scfQueryInterface<iSndSysRendererSoftware>(soundRenderer);
	  if (!softSoundRenderer)
        Report(CS_REPORTER_SEVERITY_NOTIFY, "Sound renderer is not software, sound will not be recorded");
	}
  }
  else
	Report(CS_REPORTER_SEVERITY_NOTIFY, "No sound renderer, sound will not be recorded");

  // Replace clock with our own
  fakeClock.AttachNew (new FakeClock (this));
  realClock = csQueryRegistry<iVirtualClock> (object_reg);
  object_reg->Unregister(realClock, "iVirtualClock");
  object_reg->Register(fakeClock, "iVirtualClock");

  return true;
}

csVideoRecorder::~csVideoRecorder ()
{
  Stop();

  csRef<iEventQueue> q (csQueryRegistry<iEventQueue> (object_reg));
  if (q)
  {
    q->RemoveListener (&keyEventHandler);
    q->RemoveListener (&frameEventHandler);
  }

  // Restore original clock
  object_reg->Unregister(fakeClock, "iVirtualClock");
  object_reg->Register(realClock, "iVirtualClock");
}

void csVideoRecorder::Report (int severity, const char* msg, ...)
{
  if (!reporter)
	  reporter = csQueryRegistry<iReporter> (object_reg);
  
  va_list args;
  va_start (args, msg);
  if (reporter)
    reporter->ReportV (severity, "crystalspace.videorecorder", msg, args);
  else
  {
    csPrintfV (msg, args);
    csPrintf ("\n");
  }
  va_end (args);
}

bool csVideoRecorder::InitPlugin()
{
  if (initialized)
	  return true;
 
  if (!g2d)
	  g2d = csQueryRegistry<iGraphics2D> (object_reg);
  if (!g2d)
  {
    Report (CS_REPORTER_SEVERITY_ERROR, "No Graphics2D!");
    return false;
  }
  
  VFS = csQueryRegistry<iVFS> (object_reg);
  if (!VFS)
  {
    Report (CS_REPORTER_SEVERITY_ERROR, "No VFS!");
    return false;
  }

  config.AddConfig (object_reg, "/config/videorecorder.cfg");

  // read hotkeys
  keyRecord = GetKeyCode(config->GetStr("VideoRecorder.Keys.Record", "alt-r"));
  keyPause = GetKeyCode(config->GetStr("VideoRecorder.Keys.Pause", "alt-p"));
 
  if (filenameFormat == "")
	  filenameFormat = config->GetStr("VideoRecorder.FilenameFormat", "/this/crystal000");

  // read recording settings
  forcedWidth = config->GetInt("VideoRecorder.Width", 0);
  forcedHeight = config->GetInt("VideoRecorder.Height", 0);
  framerate = config->GetInt("VideoRecorder.FPS", 30);
  extension = config->GetStr("VideoRecorder.Extension", "mp4");
  const char* Mode = config->GetStr("VideoRecorder.Mode", "vfr");
  if (strcmp(Mode, "vfr") == 0)
	  mode = MODE_VFR;
  else if (strcmp(Mode, "cfr-drop") == 0)
	  mode = MODE_CFR_DROP;
  else if (strcmp(Mode, "forced-cfr") == 0)
	  mode = MODE_FORCED_CFR;
  else
	  mode = MODE_VFR;

  recordSound = strcmp(config->GetStr("VideoRecorder.AudioCodec", "no"), "no") != 0;
  if (!soundRenderer)
    recordSound = false;
  if (recordSound)
  {
	bool IsLoopback = soundRenderer->IsLoopback();
	if (IsLoopback)
	  soundRenderer->GetLoopbackFormat(&soundFormat);
	if (mode == MODE_FORCED_CFR && !IsLoopback)
	{
	  Report (CS_REPORTER_SEVERITY_WARNING, "Sound recording in forced-cfr mode requires SndSys.Loopback to be true!");
	  recordSound = false;
    }
	if (!IsLoopback && !softSoundRenderer)
      recordSound = false;
  }

  initialized = true;
  return true;
}

bool csVideoRecorder::KeyEventHandler::HandleEvent(iEvent& event)
{
  if (!CS_IS_KEYBOARD_EVENT(parent->object_reg, event))
    return false;
  if (csKeyEventHelper::GetEventType (&event) != csKeyEventTypeDown)
    return false;

  csKeyModifiers m;
  csKeyEventHelper::GetModifiers (&event, m);

  KeyCode key;
  key.code = csKeyEventHelper::GetCookedCode (&event);
  key.alt = !!m.modifiers[csKeyModifierTypeAlt];
  key.ctrl = !!m.modifiers[csKeyModifierTypeCtrl];
  key.shift = !!m.modifiers[csKeyModifierTypeShift];
  return parent->OnKeyDown(key);
}

bool csVideoRecorder::OnKeyDown(KeyCode key)
{
  if (!InitPlugin())
	  return false;

  if (key == keyRecord) 
  {
    IsRecording()? Stop() : Start();
    return true;
  }

  if (key == keyPause) 
  {
    IsPaused()? UnPause() : Pause();
    return true;
  }

  return false;
}

bool csVideoRecorder::IsPaused () const
{
  return paused;
}

bool csVideoRecorder::IsRecording () const
{
  return recording;
}

void csVideoRecorder::Pause ()
{
  if (!recording)
    return;
  Report (CS_REPORTER_SEVERITY_NOTIFY, "Video recording paused");
  pauseTick = currentMicroTicks;
  paused = true;
}

void csVideoRecorder::UnPause()
{
  if (!recording)
    return;
  Report (CS_REPORTER_SEVERITY_NOTIFY, "Video recording resumed");
  startTick += currentMicroTicks - pauseTick;
  paused = false;
}

void csVideoRecorder::SetFilenameFormat(const char *format)
{
	filenameFormat = format;
}

void csVideoRecorder::SetRecordingFile(const char *filename)
{
	forcedFilename = filename;
}

void csVideoRecorder::Start()
{
  if (!InitPlugin())
	  return;

  if (IsPaused())
  {
    UnPause();
    return;
  }
  if (IsRecording())
    Stop();
 
  width  = forcedWidth ? forcedWidth  : g2d->GetWidth ();
  height = forcedHeight? forcedHeight : g2d->GetHeight();
  
  // construct filename
  if (forcedFilename) // exact filename is specified, just append extension
	  filename = forcedFilename + "." + extension;
  else // otherwise construct numbered filename
  {
	  csString filenameMask = filenameFormat + "." + extension;
	  filenameHelper.SetMask(filenameMask);
	  filename = filenameHelper.FindNextFilename (VFS);
  }

  // add sound recording filter
  if (recordSound && softSoundRenderer && !softSoundRenderer->AddOutputFilter(SS_FILTER_LOC_RENDEROUT, &soundRecorder))
	recordSound = false;

  // Create encoder. It is asynchronous and we will not wait for it to perform full initialization.
  // We will handle any initialization errors later (in OnFrameEnd()).
  encoder.AttachNew(new VideoEncoder(VFS, config, filename,
	                                 width, height, (mode == MODE_VFR)? 1000 : framerate,
							         recordSound? soundFormat.Channels : 0, soundFormat.Freq));
  recording = true;
  startTick = 0;

  Report(CS_REPORTER_SEVERITY_NOTIFY, "Starting video recording to %s", filename.GetData());
}

void csVideoRecorder::Stop()
{
  if (!recording)
	return;
  
  encoder->Stop();
  encoder.Invalidate();
  recording = false;
  if (softSoundRenderer && recordSound)
    softSoundRenderer->RemoveOutputFilter(SS_FILTER_LOC_RENDEROUT, &soundRecorder);

  Report(CS_REPORTER_SEVERITY_NOTIFY, "Stop video recording to %s", filename.GetData());
}

void csVideoRecorder::OnFrameEnd()
{
  if (soundRenderer && soundRenderer->IsLoopback())
  {
	soundRenderer->GetLoopbackFormat(&soundFormat);
	if (sampleBuffer.IsEmpty())
      sampleBuffer.SetSize(500*soundFormat.Channels);

	csMicroTicks BufferLength = csMicroTicks(1000)*1000*sampleBuffer.GetSize()/(soundFormat.Freq*soundFormat.Channels);
	while (currentMicroTicks > lastSampleRead + BufferLength)
	{
	  soundRenderer->FillDriverBuffer(&sampleBuffer[0], sampleBuffer.GetSize()/soundFormat.Channels, NULL, 0);
	  lastSampleRead += BufferLength;

	  if (recording && !paused)
		encoder->DeliverSoundData(&sampleBuffer[0], sampleBuffer.GetSize()/soundFormat.Channels);
	}
  }
  
  if (!recording || paused)
    return;

  // encoder is asynchronous so initialization error may only appear now
  csString Error = encoder->GetError();
  if (Error != "")
  {
	Report(CS_REPORTER_SEVERITY_ERROR, "Error during video encoding: %s", Error.GetData());
	Stop();
	return;
  }

  csMicroTicks time = currentMicroTicks;
  if (startTick == 0) // this is first frame
  {
	startTick = time;
	time = 0;
  }
  else
  {
	time -= startTick;

	int prevFrame = prevTick*framerate/1000000;
	int currentFrame = time*framerate/1000000;

	// we don't need so much frames
	if (currentFrame == prevFrame)
	  return;
  }
  prevTick = time;

  if (mode == MODE_FORCED_CFR)
	  encoder->Wait();
  else if(!encoder->NeedFrame()) // encoder is to slow, ignore this frame
  {
	  if (mode == MODE_CFR_DROP)
         Report (CS_REPORTER_SEVERITY_NOTIFY, "Drop frame");
	  return;
  }
 
  csRef<csImageMemory> screenshot;
  screenshot.AttachNew(new csImageMemory (g2d->GetWidth(), g2d->GetHeight(), CS_IMGFMT_TRUECOLOR));
  screenshot->SetImageType (csimg2D);

  if (!g2d->ScreenShot ((unsigned char*)screenshot->GetImagePtr()))
  {
      Report (CS_REPORTER_SEVERITY_ERROR, "Couldn't get screenshot");
      Stop();
      return;
  }

  encoder->AddFrame(time, screenshot);
}

void csVideoRecorder::AdvanceClock()
{
  realClock->Advance();

  if (mode == MODE_FORCED_CFR && IsRecording() && !IsPaused())
    elapsedMicroTicks = 1000000/framerate; // FIXME: this is imprecise
  else
    elapsedMicroTicks = realClock->GetElapsedMicroTicks();
  csTicks OldTicks = currentMicroTicks / 1000;
  currentMicroTicks += elapsedMicroTicks;
  csTicks NewTicks = currentMicroTicks / 1000;
  // We can't do just "elapsedTicks = elapsedMicroTicks / 1000" because this will introduce error that can accumulate.
  // This way we ensure that sum of elapsed ticks in each frame equals current ticks.
  elapsedTicks = NewTicks - OldTicks;
}

/// Parse keyname and return key code + modifier status (from bugplug).
csVideoRecorder::KeyCode csVideoRecorder::GetKeyCode (const char* keystring)
{  
  KeyCode key = {(utf32_char)-1, false, false, false};
  char const* dash = strchr (keystring, '-');
  while (dash)
  {
    if (!strncmp (keystring, "shift", int (dash-keystring))) key.shift = true;
    else if (!strncmp (keystring, "alt", int (dash-keystring))) key.alt = true;
    else if (!strncmp (keystring, "ctrl", int (dash-keystring))) key.ctrl = true;
    keystring = dash+1;
    dash = strchr (keystring, '-');
  }

  if (!strcmp (keystring, "tab")) key.code = CSKEY_TAB;
  else if (!strcmp (keystring, "space")) key.code = ' ';
  else if (!strcmp (keystring, "esc")) key.code = CSKEY_ESC;
  else if (!strcmp (keystring, "enter")) key.code = CSKEY_ENTER;
  else if (!strcmp (keystring, "bs")) key.code = CSKEY_BACKSPACE;
  else if (!strcmp (keystring, "up")) key.code = CSKEY_UP;
  else if (!strcmp (keystring, "down")) key.code = CSKEY_DOWN;
  else if (!strcmp (keystring, "right")) key.code = CSKEY_RIGHT;
  else if (!strcmp (keystring, "left")) key.code = CSKEY_LEFT;
  else if (!strcmp (keystring, "pgup")) key.code = CSKEY_PGUP;
  else if (!strcmp (keystring, "pgdn")) key.code = CSKEY_PGDN;
  else if (!strcmp (keystring, "home")) key.code = CSKEY_HOME;
  else if (!strcmp (keystring, "end")) key.code = CSKEY_END;
  else if (!strcmp (keystring, "ins")) key.code = CSKEY_INS;
  else if (!strcmp (keystring, "del")) key.code = CSKEY_DEL;
  else if (!strcmp (keystring, "f1")) key.code = CSKEY_F1;
  else if (!strcmp (keystring, "f2")) key.code = CSKEY_F2;
  else if (!strcmp (keystring, "f3")) key.code = CSKEY_F3;
  else if (!strcmp (keystring, "f4")) key.code = CSKEY_F4;
  else if (!strcmp (keystring, "f5")) key.code = CSKEY_F5;
  else if (!strcmp (keystring, "f6")) key.code = CSKEY_F6;
  else if (!strcmp (keystring, "f7")) key.code = CSKEY_F7;
  else if (!strcmp (keystring, "f8")) key.code = CSKEY_F8;
  else if (!strcmp (keystring, "f9")) key.code = CSKEY_F9;
  else if (!strcmp (keystring, "f10")) key.code = CSKEY_F10;
  else if (!strcmp (keystring, "f11")) key.code = CSKEY_F11;
  else if (!strcmp (keystring, "f12")) key.code = CSKEY_F12;
  else if (*(keystring+1) != 0) return key;
  else if ((*keystring >= 'A' && *keystring <= 'Z')
  	|| strchr ("!@#$%^&*()_+", *keystring))
  {
    key.shift = 1;
    key.code = *keystring;
  }
  else
    key.code = *keystring;

  return key;
}

}
CS_PLUGIN_NAMESPACE_END(VideoRecorder)