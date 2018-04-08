#include "csFrame.h"


BEGIN_EVENT_TABLE(csFrame, wxFrame)
  EVT_SIZE(csFrame::OnSize)

  EVT_COMBOBOX(XRCID("cbEffects"), csFrame::OnEffectChange)

  EVT_CHECKBOX(XRCID("SSDOcbEnableAO"), csFrame::OnCheckSSDO_AO)
  EVT_CHECKBOX(XRCID("SSDOcbIndirectLight"), csFrame::OnCheckSSDO_IL)
  EVT_CHECKBOX(XRCID("HBAOcbEnable"), csFrame::OnCheckHBAO)
  EVT_CHECKBOX(XRCID("DOFcbEnable"), csFrame::OnCheckDDOF)

  EVT_SLIDER(XRCID("sliderCoCScale"), csFrame::OnDDOFCoCSlider)

  EVT_SLIDER(XRCID("sliderAngleBias"), csFrame::OnHBAOAngleBiasSlider)
  EVT_SLIDER(XRCID("sliderHBAORadius"), csFrame::OnHBAORadiusSlider)
  EVT_SLIDER(XRCID("sliderHBAOSteps"), csFrame::OnHBAONumStepsSlider)
  EVT_SLIDER(XRCID("sliderHBAODirs"), csFrame::OnHBAONumDirSlider)
  EVT_SLIDER(XRCID("sliderHBAOContrast"), csFrame::OnHBAOContrastSlider)
  EVT_SLIDER(XRCID("sliderHBAOAttenuation"), csFrame::OnHBAOAttenuationSlider)
  EVT_SLIDER(XRCID("sliderHBAOBlurRadius"), csFrame::OnHBAOBlurRadiusSlider)
  EVT_SLIDER(XRCID("sliderHBAOBlurSharpness"), csFrame::OnHBAOBlurSharpnessSlider)
  EVT_CHECKBOX(XRCID("HBAOcbDebugView"), csFrame::OnCheckHBAODebug)


  EVT_SLIDER(XRCID("sliderSSDOsampleRadius"), csFrame::OnSSDORadiusSlider)
  EVT_SLIDER(XRCID("sliderSSDOdetailSampleRadius"), csFrame::OnSSDODetailRadiusSlider)
  EVT_SLIDER(XRCID("sliderSSDOnumPasses"), csFrame::OnSSDONumPassesSlider)
  EVT_SLIDER(XRCID("sliderSSDOselfOcclusion"), csFrame::OnSSDOSelfOcclusionSlider)
  EVT_SLIDER(XRCID("sliderSSDOocclusionStrength"), csFrame::OnSSDOOcclusionStrengthSlider)
  EVT_SLIDER(XRCID("sliderSSDOmaxOccluderDist"), csFrame::OnSSDOMaxOcclusionDistSlider)
  EVT_SLIDER(XRCID("sliderSSDObounceStrength"), csFrame::OnSSDOBounceStrengthSlider)
  EVT_RADIOBUTTON(XRCID("rbNoDebug"), csFrame::OnClickSSDONoDebug)
  EVT_RADIOBUTTON(XRCID("rbDebugAo"), csFrame::OnClickSSDODebugAO)
  EVT_RADIOBUTTON(XRCID("rbDebugIL"), csFrame::OnClickSSDODebugIL)

END_EVENT_TABLE()

csFrame::csFrame(iObjectRegistry* reg, wxString& title) : wxFrame(NULL, -1, title), registry(reg), drawTick(this)
{
  rotX = rotY = 0.0f;
  hbaoParams = NULL;
  ddofParams = NULL;
  ssdoParams = NULL;
  OnInitialize();
  m_pInstance = this;
}


csFrame::~csFrame()
{
  if (hbaoParams) delete hbaoParams; hbaoParams = NULL;
  if (ddofParams) delete ddofParams; ddofParams = NULL;
  if (ssdoParams) delete ssdoParams; ssdoParams = NULL;
  if (m_pInstance == this) m_pInstance = NULL;
}

bool csFrame::OnInitialize()
{
  if (!csInitializer::RequestPlugins (registry,
    CS_REQUEST_VFS,
    CS_REQUEST_PLUGIN( "crystalspace.graphics2d.wxgl", iGraphics2D ),
    CS_REQUEST_OPENGL3D,
    CS_REQUEST_ENGINE,
    CS_REQUEST_FONTSERVER,
    CS_REQUEST_IMAGELOADER,
    CS_REQUEST_LEVELLOADER,
    CS_REQUEST_REPORTER,
    CS_REQUEST_REPORTERLISTENER,
    CS_REQUEST_END))
  {
    csReport (registry, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.posteffectdemo",
      "Can't initialize plugins!");
    return false;
  }

  if (!csInitializer::SetupEventHandler (registry, EventHandler))
  {
    csReport (registry, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.posteffectdemo",
      "Can't initialize event handler!");
    return false;
  }
  CS_INITIALIZE_EVENT_SHORTCUTS (registry);

  keyboardDown = csevKeyboardDown (registry);
  mouseMove = csevMouseMove (registry, 0);
  mouseUp = csevMouseUp (registry, 0);
  mouseDown = csevMouseDown (registry, 0);


  if (csCommandLineHelper::CheckHelp (registry))
  {
    csCommandLineHelper::Help (registry);
    return false;
  }

  vfs = csQueryRegistry<iVFS> (registry);
  if (vfs == 0)
  {
    csReport (registry, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.posteffectdemo",
      "Failed to locate virtual file system!");
    return false;
  }

  vc = csQueryRegistry<iVirtualClock> (registry);
  if (vc == 0)
  {
    csReport (registry, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.posteffectdemo",
      "Can't find the virtual clock!");
    return false;
  }

  engine = csQueryRegistry<iEngine> (registry);
  if (engine == 0)
  {
    csReport (registry, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.posteffectdemo",
      "No iEngine plugin!");
    return false;
  }

  loader = csQueryRegistry<iLoader> (registry);
  if (loader == 0)
  {
    csReport (registry, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.posteffectdemo",
      "No iLoader plugin!");
    return false;
  }

  g3d = csQueryRegistry<iGraphics3D> (registry);
  if (g3d == 0)
  {
    csReport (registry, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.posteffectdemo",
      "No iGraphics3D plugin!");
    return false;
  }

  kbd = csQueryRegistry<iKeyboardDriver> (registry);
  if (kbd == 0)
  {
    csReport (registry, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.posteffectdemo",
      "No iKeyboardDriver plugin!");
    return false;
  }


  view.AttachNew(new csView (engine, g3d));

  iGraphics2D* g2d = g3d->GetDriver2D ();
  g2d->AllowResize (true);
  wxwindow = scfQueryInterface<iWxWindow> (g2d);
  if (!wxwindow)
  {
    csReport (registry, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.posteffectdemo",
      "Canvas is no iWxWindow plugin!");
    return false;
  }

  if (!LoadInterface ())
  {
    csReport (registry, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.posteffectdemo",
      "Could not load the interface!");
    return false;
  }

  Show (true);


  if (!csInitializer::OpenApplication (registry))
  {
    csReport (registry, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.posteffectdemo",
      "Error opening system!");
    return false;
  }

  wxwindow->GetWindow ()->SetFocus ();

  if (!LoadWorld ())
  {
    csReport (registry, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.posteffectdemo",
      "Could not load the world.");
    return false;
  }

  csRef<iConfigManager> cfg = csQueryRegistry<iConfigManager> (registry);
  cfg->AddDomain ("/config/engine.cfg", vfs, iConfigManager::ConfigPriorityPlugin);
  csRef<iRenderManager> deferred_rm = csLoadPlugin<iRenderManager> (registry, "crystalspace.rendermanager.deferred");
  if (deferred_rm == 0)
  {
    csReport (registry, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.posteffectdemo",
      "Failed to load deferred rm!");
    return false;
  }
  engine->SetRenderManager(deferred_rm);
  
  if (!LoadEffects ())
  {
    csReport (registry, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.posteffectdemo",
      "Could not load the effects!");
    return false;
  }

  int x, y;
  x = canvasPanel->GetSize ().GetWidth ();
  y = canvasPanel->GetSize ().GetHeight ();

  engine->Prepare ();
  view->SetAutoResize (false);
  view->GetPerspectiveCamera ()->SetAspectRatio ((float) (x) / (float) (y));
  view->SetRectangle (0, 0, x, y);

  printer.AttachNew (new FramePrinter (registry));

  return true;
}

bool csFrame::EventHandler (iEvent& ev)
{
  if (m_pInstance)
  {
    if (ev.Name == m_pInstance->Frame)
      m_pInstance->OnFrame();
    else if (CS_IS_KEYBOARD_EVENT(m_pInstance->registry, ev))
      m_pInstance->OnKey(ev);
    else if (CS_IS_MOUSE_EVENT(m_pInstance->registry, ev))
      m_pInstance->OnMouse(ev);
  }
  return true;
}

bool csFrame::LoadInterface ()
{
  wxXmlResource::Get ()->InitAllHandlers ();


  csPathsList* installPaths = csInstallationPathsHelper::GetPlatformInstallationPaths ();
  wxString searchPath;
  for (size_t i = 0; i < installPaths->GetSize(); i++)
  {
    if (!searchPath.IsEmpty ()) searchPath.Append (wxPATH_SEP);
    searchPath.Append (wxString::FromUTF8 ((*installPaths)[i].path));
  }
  delete installPaths;

  wxString resourceLocation;
  wxFileSystem wxfs;
  if (!wxfs.FindFileInPath (&resourceLocation, searchPath, wxT ("data/posteffectdemo/layout.xrc"))
    || !wxXmlResource::Get ()->Load (resourceLocation))
  {
    csReport (registry, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.posteffectdemo",
      "Could not load XRC ressource file!");
    return false;
  }

  SetSize(1024, 768);
  canvasPanel = wxXmlResource::Get ()->LoadPanel (this, wxT ("Canvas"));
  optionsPanel = wxXmlResource::Get ()->LoadPanel (this, wxT ("effectUI"));
  if (!canvasPanel || !optionsPanel)
  {
    csReport (registry, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.posteffectdemo",
      "Could not find main panel in XRC ressource file!");
    return false;
  }


  // Load Effect's UI
  SSDOPanel = wxXmlResource::Get ()->LoadPanel (optionsPanel, wxT ("SSDOPanel"));
  HBAOPanel = wxXmlResource::Get ()->LoadPanel (optionsPanel, wxT ("HBAOPanel"));
  DOFPanel = wxXmlResource::Get ()->LoadPanel (optionsPanel, wxT ("DOFPanel"));
  if (!SSDOPanel || !HBAOPanel || !DOFPanel)
  {
    csReport (registry, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.posteffectdemo",
      "Could not find the effect panel in XRC ressource file!");
    return false;
  }

  SSDOPanel->Show (false);
  HBAOPanel->Show (false);
  DOFPanel->Show (false);


  wxSize size = GetSize ();

  canvasPanel->SetSize (size.x - optionsPanel->GetSize ().GetWidth (), size.y);
  optionsPanel->SetSize (optionsPanel->GetSize ().GetWidth (), size.y);

  wxwindow->SetParent (canvasPanel);

  return true;
}

bool csFrame::LoadWorld ()
{
  bool b = vfs->ChDir ("lev/castle");
  if (!b)
    return false;

  if (!loader->LoadMapFile ("world"))
  {
    csReport (registry, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.posteffectdemo",
      "Error couldn't load level!");
    return false;
  }


  csVector3 pos (0, 0, 0);
  if (engine->GetCameraPositions ()->GetCount () > 0)
  {
    iCameraPosition* campos = engine->GetCameraPositions ()->Get (0);
    room = engine->GetSectors ()->FindByName (campos->GetSector ());
    pos = campos->GetPosition ();
  }
  else
  {
    room = engine->GetSectors ()->FindByName ("room");
    pos = csVector3 (0, 0, 0);
  }

  view->GetCamera ()->SetSector (room);
  view->GetCamera ()->GetTransform ().SetOrigin (pos);

  view->GetPerspectiveCamera()->SetNearClipDistance(1);


  return true;
}

bool csFrame::LoadEffects ()
{
  postMgr = scfQueryInterface<iRenderManagerPostEffects>(engine->GetRenderManager ());
  if (!postMgr)
  {
    csReport (registry, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.posteffectdemo",
      "Error couldn't load post effect manager!");
    return false;
  }

  dofHelper.AttachNew (new CS::RenderManager::DDOFHelper (registry));

  ddof = postMgr->CreatePostEffect (DDOF_EFFECT_NAME);
  dofHelper->Setup (ddof);

  hbao = postMgr->CreatePostEffect (HBAO_EFFECT_NAME);
  if (!hbao->LoadFromFile ("/data/posteffects/HBAO.xml"))
  {
    csReport (registry, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.posteffectdemo",
      "Error couldn't load HBAO!");
    return false;
  }
  ssdo = postMgr->CreatePostEffect (SSDO_EFFECT_NAME);
  if (!ssdo->LoadFromFile ("/data/posteffects/SSDO.xml"))
  {
    csReport (registry, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.posteffectdemo",
      "Error couldn't load SSDO!");
    return false;
  }

  csRef<iShaderVarStringSet> svStrings = csQueryRegistryTagInterface<iShaderVarStringSet> (registry, "crystalspace.shader.variablenameset");
  hbaoParams = new HBAOParams (svStrings, hbao);
  if (!hbaoParams->SetPanel (HBAOPanel))
  {
    csReport (registry, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.posteffectdemo",
      "Error couldn't  setup hbao parameters!");
    return false;
  }
  ddofParams = new DDOFParams (svStrings, ddof);
  if (!ddofParams->SetPanel (DOFPanel))
  {
    csReport (registry, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.posteffectdemo",
      "Error couldn't  setup ddof parameters!");
    return false;
  }

  ssdoParams = new SSDOParams(svStrings, ssdo);
  if (!ssdoParams->SetPanel (SSDOPanel))
  {
    csReport (registry, CS_REPORTER_SEVERITY_ERROR,
      "crystalspace.application.posteffectdemo",
      "Error couldn't  setup ssdo parameters!");
    return false;
  }

  return true;
}

void csFrame::PushFrame ()
{
  csRef<iEventQueue> q (csQueryRegistry<iEventQueue> (registry));
  if (!q)
    return ;
  csRef<iVirtualClock> vc (csQueryRegistry<iVirtualClock> (registry));

  if (vc)
    vc->Advance();
  q->Process();
}

void csFrame::OnFrame ()
{
  csTicks elapsed_time = vc->GetElapsedTicks ();
  float speed = (elapsed_time / 1000.0) * (0.06 * 20);

  iCamera* c = view->GetCamera ();

  if (kbd->GetKeyState (CSKEY_SHIFT))
  {
    if (kbd->GetKeyState (CSKEY_RIGHT))
      c->Move (CS_VEC_RIGHT * 4 * speed);
    if (kbd->GetKeyState (CSKEY_LEFT))
      c->Move (CS_VEC_LEFT * 4 * speed);
    if (kbd->GetKeyState (CSKEY_UP))
      c->Move (CS_VEC_UP * 4 * speed);
    if (kbd->GetKeyState (CSKEY_DOWN))
      c->Move (CS_VEC_DOWN * 4 * speed);
  }
  else
  {
    if (kbd->GetKeyState (CSKEY_RIGHT))
      rotY += speed;
    if (kbd->GetKeyState (CSKEY_LEFT))
      rotY -= speed;
    if (kbd->GetKeyState (CSKEY_PGUP))
      rotX += speed;
    if (kbd->GetKeyState (CSKEY_PGDN))
      rotX -= speed;
    if (kbd->GetKeyState (CSKEY_UP))
      c->Move (CS_VEC_FORWARD * 4 * speed);
    if (kbd->GetKeyState (CSKEY_DOWN))
      c->Move (CS_VEC_BACKWARD * 4 * speed);
  }

  csMatrix3 rot = csXRotMatrix3 (rotX) * csYRotMatrix3 (rotY);
  csOrthoTransform ot (rot, c->GetTransform().GetOrigin ());
  c->SetTransform (ot);

  engine->GetRenderManager ()->RenderView (view);
}


void csFrame::OnSize (wxSizeEvent& ev)
{
  wxSize size = ev.GetSize ();
  if (!wxwindow.IsValid ()) return;

  if (!wxwindow->GetWindow ()) return;

  size.x -= optionsPanel->GetSize ().GetWidth ();
  optionsPanel->SetSize(optionsPanel->GetSize ().GetWidth (), size.y);

  wxwindow->GetWindow ()->SetSize (size);
  canvasPanel->SetSize (size);

  view->GetPerspectiveCamera ()->SetAspectRatio ((float) (size.x) / (float) (size.y));
  view->SetRectangle (0, 0, size.x, size.y, false);
}


//------------------------------

void csFrame::OnEffectChange (wxCommandEvent& ev)
{
  int i = ev.GetSelection ();

  DOFPanel->Show (false);
  HBAOPanel->Show (false);
  SSDOPanel->Show (false);

  switch (i)
  {
  case 0:
    DOFPanel->Show (true);
    break;
  case 1:
    HBAOPanel->Show (true);
    break;
  case 2:
    SSDOPanel->Show (true);
    break;
  default:
    ;
  }
}


void csFrame::OnCheckHBAO (wxCommandEvent& ev)
{
  size_t pos = postMgr->FindPostEffect (HBAO_EFFECT_NAME);
  if (ev.IsChecked ())
  {
    if (pos == (size_t)-1)
      postMgr->InsertPostEffect (hbao, 0);
  } else
    postMgr->RemovePostEffect (pos);
}
void csFrame::OnCheckSSDO_AO (wxCommandEvent& ev)
{
  size_t pos = postMgr->FindPostEffect(SSDO_EFFECT_NAME);
  bool b = ssdoParams->SetAOEnable (ev.IsChecked ());
  if (ev.IsChecked())
  {
    if (pos == (size_t)-1)
      postMgr->InsertPostEffect(ssdo, 0);
  } else
  {
    if (b)
      postMgr->RemovePostEffect(pos);
  }
    
}
void csFrame::OnCheckSSDO_IL (wxCommandEvent& ev)
{
  size_t pos = postMgr->FindPostEffect(SSDO_EFFECT_NAME);
  bool b = ssdoParams->SetILEnable (ev.IsChecked ());
  if (ev.IsChecked())
  {
    if (pos == (size_t)-1)
      postMgr->InsertPostEffect(ssdo, 0);
  } else
  {
    if (b)
      postMgr->RemovePostEffect(pos);
  }
}
void csFrame::OnCheckDDOF (wxCommandEvent& ev)
{
  size_t pos = postMgr->FindPostEffect (DDOF_EFFECT_NAME);
  if (ev.IsChecked ())
  {
    if (pos == (size_t)-1)
      postMgr->AddPostEffect (ddof);
  } else
    postMgr->RemovePostEffect (pos);
}

void csFrame::OnDDOFCoCSlider (wxCommandEvent& ev)
{
  ddofParams->SetCoCScale (ev.GetInt ());
}
void csFrame::OnHBAOAngleBiasSlider (wxCommandEvent& ev)
{
  hbaoParams->SetAngleBias (ev.GetInt ());
}
void csFrame::OnHBAORadiusSlider (wxCommandEvent& ev)
{
  hbaoParams->SetRadius (ev.GetInt ());
}
void csFrame::OnHBAONumStepsSlider (wxCommandEvent& ev)
{
  hbaoParams->SetNumSteps (ev.GetInt ());
}
void csFrame::OnHBAONumDirSlider (wxCommandEvent& ev)
{
  hbaoParams->SetNumDirections (ev.GetInt ());
}
void csFrame::OnHBAOContrastSlider (wxCommandEvent& ev)
{
  hbaoParams->SetContrast (ev.GetInt ());
}
void csFrame::OnHBAOAttenuationSlider (wxCommandEvent& ev)
{
  hbaoParams->SetAttenuation (ev.GetInt ());
}
void csFrame::OnHBAOBlurRadiusSlider (wxCommandEvent& ev)
{
  hbaoParams->SetBlurRadius (ev.GetInt ());
}
void csFrame::OnHBAOBlurSharpnessSlider (wxCommandEvent& ev)
{
  hbaoParams->SetBlurSharpness (ev.GetInt ());
}

void csFrame::OnSSDORadiusSlider (wxCommandEvent& ev)
{
  ssdoParams->SetRadius (ev.GetInt ());
}
void csFrame::OnSSDODetailRadiusSlider (wxCommandEvent& ev)
{
  ssdoParams->SetDetailRadius(ev.GetInt ());
}
void csFrame::OnSSDONumPassesSlider (wxCommandEvent& ev)
{
  ssdoParams->SetNumPasses (ev.GetInt ());
}
void csFrame::OnSSDOSelfOcclusionSlider (wxCommandEvent& ev)
{
  ssdoParams->SetSelfOcclusion (ev.GetInt ());
}
void csFrame::OnSSDOOcclusionStrengthSlider (wxCommandEvent& ev)
{
  ssdoParams->SetOcclusionStrength (ev.GetInt ());
}
void csFrame::OnSSDOMaxOcclusionDistSlider (wxCommandEvent& ev)
{
  ssdoParams->SetMaxOcclusionDist (ev.GetInt ());
}
void csFrame::OnSSDOBounceStrengthSlider (wxCommandEvent& ev)
{
  ssdoParams->SetBounceStrength (ev.GetInt ());
}

void csFrame::OnCheckHBAODebug(wxCommandEvent& ev)
{
  hbaoParams->SetDebugMode(ev.IsChecked());
}

void csFrame::OnClickSSDONoDebug(wxCommandEvent& ev)
{
  ssdoParams->SetDebugMode(SSDOParams::SSDO_NONE);
}
void csFrame::OnClickSSDODebugAO(wxCommandEvent& ev)
{
  ssdoParams->SetDebugMode(SSDOParams::SSDO_AO);
}
void csFrame::OnClickSSDODebugIL(wxCommandEvent& ev)
{
  ssdoParams->SetDebugMode(SSDOParams::SSDO_IL);
}
