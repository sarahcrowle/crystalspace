#ifndef __CSFRAME_H__
#define __CSFRAME_H__



#include <crystalspace.h>
#include "csplugincommon/rendermanager/posteffects.h"
#include "csplugincommon/rendermanager/ddofhelper.h"
#include "csutil/ref.h"
#include "ivideo/wxwin.h"
#include "shaderParam.h"

#include "csutil/custom_new_disable.h"
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include "csutil/custom_new_enable.h"

#define DDOF_EFFECT_NAME "Diffusion DOF"
#define HBAO_EFFECT_NAME "HBAO"
#define SSDO_EFFECT_NAME "SSDO"

class csFrame : public wxFrame, public CS::Utility::WeakReferenced
{
protected:
  iObjectRegistry* registry;
  csRef<iVFS> vfs;
  csRef<iEngine> engine;
  csRef<iLoader> loader;
  csRef<iGraphics3D> g3d;
  csRef<iKeyboardDriver> kbd;
  csRef<iVirtualClock> vc;
  csRef<iView> view;
  csRef<iWxWindow> wxwindow;
  csRef<iSector> room;
  csRef<FramePrinter> printer;
  csRef<iRenderManagerPostEffects> postMgr;

  CS_DECLARE_EVENT_SHORTCUTS;
  csEventID mouseDown;
  csEventID mouseUp;
  csEventID mouseMove;
  csEventID keyboardDown;
  float rotX, rotY;
  static csFrame * m_pInstance;

  //------ Main UI -----------

  wxPanel * canvasPanel;
  wxPanel * optionsPanel;

  //------ Effect UI ---------

  wxPanel * SSDOPanel;
  wxPanel * HBAOPanel;
  wxPanel * DOFPanel;

  //------ PostEffects -------

  csRef<CS::RenderManager::iPostEffect> hbao;
  csRef<CS::RenderManager::iPostEffect> ssdo;
  csRef<CS::RenderManager::iPostEffect> ddof;

  csRef<CS::RenderManager::DDOFHelper> dofHelper;
  //--------------------------
  HBAOParams * hbaoParams;
  DDOFParams * ddofParams;
  SSDOParams * ssdoParams;

  bool LoadWorld();
  bool LoadInterface();
  bool LoadEffects();

  class DrawTick : public wxTimer
  {
    csWeakRef<csFrame> frame;
  public:
    DrawTick(csFrame* f) : frame(f) {if (IsRunning()) Stop();}
    virtual void Notify()
    {
      if (frame.IsValid())
        frame->PushFrame();
    }
  };

  DrawTick drawTick;
public:
  csFrame(iObjectRegistry* reg, wxString& title);
  ~csFrame();

  bool OnInitialize();

  void OnFrame();

  void OnKey(iEvent& ev) {}

  void OnMouse(iEvent& ev) {}

  static bool EventHandler(iEvent& ev);

  void Start() {drawTick.Start(10);}

  void Stop() {drawTick.Stop();}

  void PushFrame ();

  virtual void OnSize (wxSizeEvent& ev);


  //-------------Events----------------
  void OnEffectChange(wxCommandEvent& ev);

  void OnCheckHBAO(wxCommandEvent& ev);
  void OnCheckSSDO_AO(wxCommandEvent& ev);
  void OnCheckSSDO_IL(wxCommandEvent& ev);
  void OnCheckDDOF(wxCommandEvent& ev);


  void OnDDOFCoCSlider(wxCommandEvent& ev);

  void OnHBAOAngleBiasSlider(wxCommandEvent& ev);
  void OnHBAORadiusSlider(wxCommandEvent& ev);
  void OnHBAONumStepsSlider(wxCommandEvent& ev);
  void OnHBAONumDirSlider(wxCommandEvent& ev);
  void OnHBAOContrastSlider(wxCommandEvent& ev);
  void OnHBAOAttenuationSlider(wxCommandEvent& ev);
  void OnHBAOBlurRadiusSlider(wxCommandEvent& ev);
  void OnHBAOBlurSharpnessSlider(wxCommandEvent& ev);
  void OnCheckHBAODebug(wxCommandEvent& ev);

  void OnSSDORadiusSlider(wxCommandEvent& ev);
  void OnSSDODetailRadiusSlider(wxCommandEvent& ev);
  void OnSSDONumPassesSlider(wxCommandEvent& ev);
  void OnSSDOSelfOcclusionSlider(wxCommandEvent& ev);
  void OnSSDOOcclusionStrengthSlider(wxCommandEvent& ev);
  void OnSSDOMaxOcclusionDistSlider(wxCommandEvent& ev);
  void OnSSDOBounceStrengthSlider(wxCommandEvent& ev);
  void OnClickSSDONoDebug(wxCommandEvent& ev);
  void OnClickSSDODebugAO(wxCommandEvent& ev);
  void OnClickSSDODebugIL(wxCommandEvent& ev);
  //-----------------------------------

  DECLARE_EVENT_TABLE()
};

#endif