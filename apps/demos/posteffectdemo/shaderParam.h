#ifndef SHADER_PARAM_H
#define SHADER_PARAM_H

#include "crystalspace.h"
#include <wx/wx.h>

using namespace CS::RenderManager;


class DDOFParams
{
  CS::ShaderVarStringID svCoCStrID;
  csRef<iPostEffect> effect;
  wxSlider * pCoCSlider;
public:
  DDOFParams(iShaderVarStringSet * svStrings, iPostEffect * effect);
  void SetCoCScale(float f);
  void SetCoCScale(int i, bool updateCtrl = false);
  bool SetPanel(wxPanel * panel);
};



class HBAOParams
{
  // hbao variables
  csRef<csShaderVariable> angleBias;
  csRef<csShaderVariable> radius;
  csRef<csShaderVariable> sqrRadius;
  csRef<csShaderVariable> invRadius;
  csRef<csShaderVariable> steps;
  csRef<csShaderVariable> directions;
  csRef<csShaderVariable> contrast;
  csRef<csShaderVariable> attenuation;
  csRef<csShaderVariable> svDebugMode;

  // blur variables
  csRef<csShaderVariable> blurRadius;
  csRef<csShaderVariable> blurFalloff;
  csRef<csShaderVariable> blurSharpness;

  //wx Controls
  wxSlider * sliderAngleBias;
  wxSlider * sliderRadius;
  wxSlider * sliderNumSteps;
  wxSlider * sliderNumDirs;
  wxSlider * sliderContrast;
  wxSlider * sliderAttenuation;
  wxSlider * sliderBlurRadius;
  wxSlider * sliderBlurSharpness;
public:
  HBAOParams(iShaderVarStringSet * svStrings, iPostEffect * effect);

  bool SetPanel(wxPanel * panel);

  void SetDebugMode(bool b);

  void SetAngleBias(float f);
  void SetAngleBias(int i, bool updateCtrl = false);

  void SetRadius(float f);
  void SetRadius(int i, bool updateCtrl = false);

  void SetNumSteps(float f);
  void SetNumSteps(int i, bool updateCtrl = false);

  void SetNumDirections(float f);
  void SetNumDirections(int i, bool updateCtrl = false);

  void SetContrast(float f);
  void SetContrast(int i, bool updateCtrl = false);

  void SetAttenuation(float f);
  void SetAttenuation(int i, bool updateCtrl = false);

  void SetBlurRadius(float f);
  void SetBlurRadius(int i, bool updateCtrl = false);

  void SetBlurSharpness(float f);
  void SetBlurSharpness(int i, bool updateCtrl = false);
};


class SSDOParams
{

  csRef<csShaderVariable> enableAO;
  csRef<csShaderVariable> enableIL;
  csRef<csShaderVariable> sampleRadius;
  csRef<csShaderVariable> datailSampleRadius;
  csRef<csShaderVariable> passes;
  csRef<csShaderVariable> selfOcclusion;
  csRef<csShaderVariable> occlusionStrength;
  csRef<csShaderVariable> maxDist;
  csRef<csShaderVariable> bounceStrength;
  csRef<csShaderVariable> svDebugAO;
  csRef<csShaderVariable> svDebugIL;

  wxSlider * sliderRadius;
  wxSlider * sliderDetailRadius;
  wxSlider * sliderNumPasses;
  wxSlider * sliderSelfOcclusion;
  wxSlider * sliderOcclusionStrength;
  wxSlider * sliderMaxDist;
  wxSlider * sliderBounceStrength;

public:

  enum ssdoDbgMode {SSDO_NONE, SSDO_AO, SSDO_IL};

  SSDOParams(iShaderVarStringSet * svStrings, iPostEffect * effect);

  bool SetPanel(wxPanel * panel);

  bool SetAOEnable(bool b);
  bool SetILEnable(bool b);

  void SetDebugMode(ssdoDbgMode mode);

  void SetRadius(float f);
  void SetRadius(int i, bool updateCtrl = false);

  void SetDetailRadius(float f);
  void SetDetailRadius(int i, bool updateCtrl = false);

  void SetNumPasses(float f);
  void SetNumPasses(int i, bool updateCtrl = false);

  void SetSelfOcclusion(float f);
  void SetSelfOcclusion(int i, bool updateCtrl = false);

  void SetOcclusionStrength(float f);
  void SetOcclusionStrength(int i, bool updateCtrl = false);

  void SetMaxOcclusionDist(float f);
  void SetMaxOcclusionDist(int i, bool updateCtrl = false);

  void SetBounceStrength(float f);
  void SetBounceStrength(int i, bool updateCtrl = false);

};


#endif