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

#include "videotest.h"

#include "csutil/custom_new_disable.h"
#include <CEGUI.h>
#include <CEGUILogger.h>
#include <CEGUIWindowManager.h>
#include "csutil/custom_new_enable.h"

#include "cstool/genmeshbuilder.h"
#include "cstool/materialbuilder.h"
#include "csutil/array.h"
#include "csutil/nobjvec.h"
#include "imap/reader.h"
#include "isndsys/ss_listener.h"
#include "isndsys/ss_source.h"
#include "isndsys/ss_stream.h"
#include "iutil/document.h"

VideoTest::VideoTest ()
: DemoApplication ("CrystalSpace.VideoTest"), inWater (false)
{
}

void VideoTest::PrintHelp ()
{
  csCommandLineHelper commandLineHelper;

  // Printing help
  commandLineHelper.PrintApplicationHelp
    (GetObjectRegistry (), "csvid", "csvid", "Crystal Space's video player demo.");
}

void VideoTest::Frame ()
{
  // Update the media player
  // TODO: do this automatically
  mediaPlayer->UpdatePlayer ();

  if (updateSeeker)
  {
    float videoPos = mediaPlayer->GetPosition ();
    CEGUI::Scrollbar * seeker = static_cast<CEGUI::Scrollbar*> 
      (CEGUI::WindowManager::getSingleton ().getWindow ("Video/Window1/Seek"));
    seeker->setScrollPosition (videoPos);
  }

  // Draw the 3D scene
  DemoApplication::Frame ();

  // Display the video image on top of the 3D scene
  int w, h;
  logoTex->GetOriginalDimensions (w, h);
  int screenW = g2d->GetWidth ();

  // Width of the image, as a fraction of the screen width
  const float widthFraction = 0.3f;
  const int width = (int)screenW * widthFraction;
  const int height = width * h / w;

  // Draw 2D window displaying the video on-screen
  g3d->BeginDraw (CSDRAW_2DGRAPHICS);
  g3d->DrawPixmap (logoTex, 
    10, 
    10,
    width * mediaPlayer->GetAspectRatio (),
    height,
    0,
    0,
    w,
    h,
    0);
}

bool VideoTest::Application ()
{
  if (!csInitializer::RequestPlugins (object_reg,
    CS_REQUEST_VFS,
    CS_REQUEST_PLUGIN ("crystalspace.videodecode.loader", iMediaLoader),
    CS_REQUEST_PLUGIN ("crystalspace.videodecode.player", iMediaPlayer),
    CS_REQUEST_PLUGIN ("crystalspace.videodecode.parser", iLoaderPlugin),
    CS_REQUEST_PLUGIN ("crystalspace.cegui.wrapper", iCEGUI),
    CS_REQUEST_PLUGIN ("crystalspace.documentsystem.multiplexer", iDocumentSystem),
    CS_REQUEST_PLUGIN_TAG ("crystalspace.documentsystem.tinyxml", iDocumentSystem, 
                           "iDocumentSystem.1"),
    CS_REQUEST_PLUGIN ("crystalspace.sndsys.renderer.openal", iSndSysRenderer),
    CS_REQUEST_END))
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.videotest",
      "Can't initialize plugins!");
    return false;
  }

  // Default behavior from DemoApplication
  if (!DemoApplication::Application ())
    return false;

  vfs = csQueryRegistry<iVFS> (GetObjectRegistry ());
  if (!vfs) return ReportError ("Failed to locate VFS!");

  cegui = csQueryRegistry<iCEGUI> (GetObjectRegistry ());
  if (!cegui) return ReportError ("Failed to locate CEGUI plugin");

  sndrenderer = csQueryRegistry<iSndSysRenderer> (GetObjectRegistry ());
  if (!sndrenderer) 
    ReportWarning ("Failed to locate sound renderer!");
  else
  {
    csRef<iSndSysListener> listener = sndrenderer->GetListener ();
    if (!listener) ReportWarning ("Failed to locate sound listener!"); 
    else listener->SetPosition (csVector3 (0, 0, 0));
  }

  // Set the working directory
  vfs->ChDir ("/videodecode/");

  // Get the loader and load the video
  csString videoName = "vid422.xml";
  csRef<iMediaLoader> mediaLoader = csQueryRegistry<iMediaLoader> (object_reg);
  csRef<iMediaContainer> video = mediaLoader->LoadMedia (videoName);
  if (!video) return ReportError ("Failed to load the video file %s!",
				  CS::Quote::Single (videoName.GetData ()));

  // Display the content of the media container
  if (video.IsValid ())
  {
    printf ("%d stream(s) in media container:\n", (int) video->GetMediaCount ());
    for (size_t i = 0; i < video->GetMediaCount (); i++)
    {
      csRef<iMedia> media = video->GetMedia (i);
      printf("--> %i. '%s' of type '%s'\n", (int) i+1, media->GetName (), media->GetType ());
    }
    printf ("%d language(s) in media container:\n", (int) video->GetLanguageCount ());
    for (size_t i = 0; i < video->GetLanguageCount (); i++)
    {
      const MediaLanguage& lang = video->GetLanguage (i);
      printf("--> %i. language '%s' from file '%s'\n", (int) i+1,
	     lang.name.GetData (), lang.path.GetData ());
    }
    printf("\n");
  }

  // Initialize the player
  mediaPlayer = csQueryRegistry<iMediaPlayer> (object_reg);
  mediaPlayer->InitializePlayer (video);
  mediaPlayer->SetCacheSize (5);
  mediaPlayer->SetActiveStream (0);  // Specifying -1 as index triggers auto stream activation

  // Get the video texture
  logoTex = mediaPlayer->GetTargetTexture ();

  // Get the audio stream
  mediaPlayer->SetCurrentLanguage ("enUS");
  audioStream = mediaPlayer->GetTargetAudio ();

  // Start the player
  if (audioStream.IsValid () && sndrenderer)
  {
    sndsource = sndrenderer->CreateSource (audioStream);

    if (sndsource.IsValid () )
    {
      sndsource->SetVolume (1.0f);
    }
  }

  mediaPlayer->Play ();

  InitializeCEGUI ();

  updateSeeker = true;

  // Create the scene
  if (!CreateScene ())
    return false;

  // Run the application
  Run ();

  return true;
}

bool VideoTest::CreateScene ()
{
  printf ("Creating level...\n");

  // Create a MaterialWrapper from the video texture
  iTextureList* texList = engine->GetTextureList ();
  iTextureWrapper* texWrapper = texList->NewTexture (logoTex);
  iMaterialWrapper* vidMaterial = engine->CreateMaterial ("vidMaterial",texWrapper);

  // We create a new sector called "room".
  room = engine->CreateSector ("room");

  // Creating the walls for our room.

  // First we make a primitive for our geometry.
  using namespace CS::Geometry;
  DensityTextureMapper mapper (0.3f);
  TesselatedBox box (csVector3 (-5, 0, -5), csVector3 (5, 20, 5));
  box.SetLevel (3);
  box.SetMapper (&mapper);
  box.SetFlags (Primitives::CS_PRIMBOX_INSIDE);

  // Now we make a factory and a mesh at once.
  csRef<iMeshWrapper> walls = GeneralMeshBuilder::CreateFactoryAndMesh (
    engine, room, "walls", "walls_factory", &box);

  // Set the material to each wall of the room
  walls->GetMeshObject ()->SetMaterialWrapper (vidMaterial);

  // Now we need light to see something.
  csRef<iLight> light;
  iLightList* ll = room->GetLights ();

  light = engine->CreateLight (0, csVector3 (-3, 5, 0), 10, csColor (1, 0, 0));
  ll->Add (light);

  light = engine->CreateLight (0, csVector3 (3, 5,  0), 10, csColor (0, 0, 1));
  ll->Add (light);

  light = engine->CreateLight (0, csVector3 (0, 5, -3), 10, csColor (0, 1, 0));
  ll->Add (light);

  // Setup the sector and the camera
  view->GetCamera ()->SetSector (room);
  cameraManager->SetStartPosition (csVector3 (0,3,-4.5));
  cameraManager->SetCamera (view->GetCamera ());
  cameraManager->SetCameraMode (CS::Utility::CAMERA_MOVE_FREE);
  cameraManager->SetMotionSpeed (10.0f);

  // Set the video texture as the background as well
  int w, h;
  logoTex->GetOriginalDimensions (w, h);
  view->SetBackgroundTexture
    (logoTex, 0, 0, view->GetWidth (), view->GetHeight (),
     0, 0, w, h, 1.f, false);

  printf ("Precaching data...\n");
  engine->PrecacheDraw ();

  printf ("Ready!\n");

  cegui->EnableMouseCapture ();

  return true;
}

//--------------------------- CEGUI management ---------------------------

void VideoTest::InitializeCEGUI ()
{
  // Initialize CEGUI wrapper
  cegui->Initialize ();

  /* Let CEGUI plugin install an event handler that takes care of rendering
     every frame */
  cegui->SetAutoRender (true);
  
  // Set the logging level 
  cegui->GetLoggerPtr ()->setLoggingLevel (CEGUI::Informative);

  vfs->ChDir ("/cegui/");

  // Load the ice skin (which uses Falagard skinning system)
  cegui->GetSchemeManagerPtr ()->create ("ice.scheme");

  cegui->GetSystemPtr ()->setDefaultMouseCursor ("ice", "MouseArrow");

  cegui->GetFontManagerPtr ()->createFreeTypeFont ("DejaVuSans", 10, true, "/fonts/dejavu/DejaVuSans.ttf");

  CEGUI::WindowManager* winMgr = cegui->GetWindowManagerPtr ();

  // Load layout and set as root
  vfs->ChDir ("/videodecode/");
  cegui->GetSystemPtr ()->setGUISheet (winMgr->loadWindowLayout ("ice.layout"));

  // Subscribe to the events that we need
  CEGUI::Window* btn = winMgr->getWindow ("Video/Window1/Quit");

  btn->subscribeEvent (CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber (&VideoTest::OnExitButtonClicked, this));

  CEGUI::Window* btn2 = winMgr->getWindow ("Video/Window1/Play");

  btn2->subscribeEvent (CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber (&VideoTest::OnPlayButtonClicked, this));

  CEGUI::Window* btn3 = winMgr->getWindow ("Video/Window1/Pause");

  btn3->subscribeEvent (CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber (&VideoTest::OnPauseButtonClicked, this));

  CEGUI::Window* btn4 = winMgr->getWindow ("Video/Window1/Stop");

  btn4->subscribeEvent (CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber (&VideoTest::OnStopButtonClicked, this));

  CEGUI::Window* btn5 = winMgr->getWindow ("Video/Window1/Loop");

  btn5->subscribeEvent (CEGUI::Checkbox::EventCheckStateChanged,
    CEGUI::Event::Subscriber (&VideoTest::OnLoopToggle, this));

  CEGUI::Scrollbar* slider = (CEGUI::Scrollbar*)winMgr->getWindow ("Video/Window1/Seek");

  slider->subscribeEvent (CEGUI::Scrollbar::EventThumbTrackStarted ,
    CEGUI::Event::Subscriber (&VideoTest::OnSeekingStart, this));
  slider->subscribeEvent (CEGUI::Scrollbar::EventThumbTrackEnded ,
    CEGUI::Event::Subscriber (&VideoTest::OnSeekingEnd, this));
  slider->setDocumentSize (mediaPlayer->GetDuration ());
  slider->setStepSize (0.1f);
  slider->setTooltipText ("Seek");

  CEGUI::RadioButton * radioButton1 = static_cast<CEGUI::RadioButton*> 
    (CEGUI::WindowManager::getSingleton ().getWindow ("Video/Window1/Loop"));
  radioButton1->setSelected (mediaPlayer->GetCyclic ());
}

bool VideoTest::OnExitButtonClicked (const CEGUI::EventArgs&)
{
  csRef<iEventQueue> q =
    csQueryRegistry<iEventQueue> (GetObjectRegistry ());
  if (q.IsValid ())
  {
    q->GetEventOutlet ()->Broadcast (csevQuit (GetObjectRegistry ()));
  }
  return true;
}
bool VideoTest::OnPlayButtonClicked (const CEGUI::EventArgs&)
{
  mediaPlayer->Play ();
  return true;
}
bool VideoTest::OnPauseButtonClicked (const CEGUI::EventArgs&)
{
  mediaPlayer->Pause ();
  return true;
}
bool VideoTest::OnStopButtonClicked (const CEGUI::EventArgs&)
{
  mediaPlayer->Stop ();
  return false;
}
bool VideoTest::OnLoopToggle (const CEGUI::EventArgs& e)
{
  CEGUI::RadioButton * radioButton1 = static_cast<CEGUI::RadioButton*> 
    (CEGUI::WindowManager::getSingleton ().getWindow ("Video/Window1/Loop"));
  mediaPlayer->SetCyclic (radioButton1->isSelected ());
  return true;
}
bool VideoTest::OnSeekingStart (const CEGUI::EventArgs&)
{
  updateSeeker=false;
  return true;
}
bool VideoTest::OnSeekingEnd (const CEGUI::EventArgs&)
{

  CEGUI::Scrollbar * seeker = 
    static_cast<CEGUI::Scrollbar*> (CEGUI::WindowManager::getSingleton ().getWindow ("Video/Window1/Seek"));

  mediaPlayer->SetPosition (seeker->getScrollPosition ());
  updateSeeker=true;
  return true;
}

//---------------------------------------------------------------------------
