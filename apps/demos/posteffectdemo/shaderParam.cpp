#include "shaderParam.h"

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

template <typename T>
T clamp(T val, T min, T max)
{
  return val < min ? min : (val > max ? max : val);
}

DDOFParams::DDOFParams (iShaderVarStringSet * svStrings, iPostEffect * effect): effect(effect)
{
  svCoCStrID = svStrings->Request ("coc scale");
  //effect->GetLayer (0)->GetSVContext ()->AddVariable(CoCScale);
}


bool DDOFParams::SetPanel (wxPanel * panel)
{
  pCoCSlider = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderCoCScale")));
  if (!pCoCSlider) return false;
  pCoCSlider->SetMin (0);
  pCoCSlider->SetMax (30);
  SetCoCScale (15, true);
  return true;
}

void DDOFParams::SetCoCScale (float f)
{
  f = clamp<float>(f, 0.f, 30.f);
  if (effect.IsValid())
  {
    iPostEffectLayer * build_x = effect->GetLayer ("build_abc_x");
    iPostEffectLayer * build_y = effect->GetLayer ("build_abc_y");
    if (build_x && build_y)
    {
      build_x->GetSVContext ()->GetVariableAdd (svCoCStrID)->SetValue(f);
      build_y->GetSVContext ()->GetVariableAdd (svCoCStrID)->SetValue(f);
    }
  }
}

void DDOFParams::SetCoCScale (int i, bool updateCtrl)
{
  i = clamp<int>(i, 0, 30);
  SetCoCScale ((float)i);
  if (updateCtrl)
    pCoCSlider->SetValue ((int)i);
}



HBAOParams::HBAOParams(iShaderVarStringSet * svStrings, iPostEffect * effect)
{
  angleBias.AttachNew (new csShaderVariable(svStrings->Request ("angle bias")));
  radius.AttachNew (new csShaderVariable(svStrings->Request ("radius")));
  sqrRadius.AttachNew (new csShaderVariable(svStrings->Request ("sqr radius")));
  invRadius.AttachNew (new csShaderVariable(svStrings->Request ("inv radius")));
  steps.AttachNew (new csShaderVariable(svStrings->Request ("num steps")));
  directions.AttachNew (new csShaderVariable(svStrings->Request ("num directions")));
  contrast.AttachNew (new csShaderVariable(svStrings->Request ("contrast")));
  attenuation.AttachNew (new csShaderVariable(svStrings->Request ("attenuation")));

  blurRadius.AttachNew (new csShaderVariable(svStrings->Request ("blur radius")));
  blurFalloff.AttachNew (new csShaderVariable(svStrings->Request ("blur falloff")));
  blurSharpness.AttachNew (new csShaderVariable(svStrings->Request ("sharpness")));
  svDebugMode.AttachNew (new csShaderVariable(svStrings->Request ("debug show ao")));



  effect->GetLayer (0)->GetSVContext ()->AddVariable(angleBias);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(radius);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(sqrRadius);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(invRadius);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(steps);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(directions);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(contrast);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(attenuation);


  effect->GetLayer ("blurX")->GetSVContext ()->AddVariable(blurRadius);
  effect->GetLayer ("blurX")->GetSVContext ()->AddVariable(blurFalloff);
  effect->GetLayer ("blurX")->GetSVContext ()->AddVariable(blurSharpness);
  effect->GetLayer ("blurY")->GetSVContext ()->AddVariable(blurRadius);
  effect->GetLayer ("blurY")->GetSVContext ()->AddVariable(blurFalloff);
  effect->GetLayer ("blurY")->GetSVContext ()->AddVariable(blurSharpness);

  effect->GetLayer ("combine")->GetSVContext ()->AddVariable(svDebugMode);

  SetDebugMode(false);
}

bool HBAOParams::SetPanel(wxPanel * panel)
{
  sliderAngleBias = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderAngleBias")));
  sliderRadius = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderHBAORadius")));
  sliderNumSteps = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderHBAOSteps")));
  sliderNumDirs = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderHBAODirs")));
  sliderContrast = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderHBAOContrast")));
  sliderAttenuation = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderHBAOAttenuation")));
  sliderBlurRadius = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderHBAOBlurRadius")));
  sliderBlurSharpness = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderHBAOBlurSharpness")));


  if (!sliderAngleBias || !sliderRadius || !sliderNumSteps ||
      !sliderNumDirs || !sliderContrast || !sliderAttenuation ||
      !sliderBlurRadius || !sliderBlurSharpness)
      return false;

  sliderAngleBias->SetMin (0);
  sliderAngleBias->SetMax (90);

  sliderRadius->SetMin (0);
  sliderRadius->SetMax (100);

  sliderNumSteps->SetMin (0);
  sliderNumSteps->SetMax (31);

  sliderNumDirs->SetMin (0);
  sliderNumDirs->SetMax (31);

  sliderContrast->SetMin (0);
  sliderContrast->SetMax (30);

  sliderAttenuation->SetMin (0);
  sliderAttenuation->SetMax (30);

  sliderBlurRadius->SetMin (0);
  sliderBlurRadius->SetMax (15);

  sliderBlurSharpness->SetMin (0);
  sliderBlurSharpness->SetMax (100);

  SetAngleBias(10, true);
  SetRadius(5, true);
  SetNumSteps(8, true);
  SetNumDirections(16, true);
  SetContrast(15, true);
  SetAttenuation(30, true);
  SetBlurRadius(3, true);
  SetBlurSharpness(50, true);
  return true;
}

void HBAOParams::SetDebugMode(bool b)
{
  if (b)
    svDebugMode->SetValue(1.0f);
  else
    svDebugMode->SetValue(0.0f);
}

void HBAOParams::SetAngleBias(float f)
{
  f = clamp<float>(f, 0.f, M_PI/2);
  angleBias->SetValue(f);
}

void HBAOParams::SetRadius(float f)
{
  f = clamp<float>(f, 0.01f, 5);
  radius->SetValue(f);
  sqrRadius->SetValue(f * f);
  invRadius->SetValue(1 / f);
}

void HBAOParams::SetNumSteps(float f)
{
  f = (float)clamp<int>((int)f, 1, 32);
  steps->SetValue(f);
}

void HBAOParams::SetNumDirections(float f)
{
  f = (float)clamp<int>((int)f, 1, 32);
  directions->SetValue(f);
}

void HBAOParams::SetContrast(float f)
{
  f = clamp<float>(f, 0.f, 3.f);
  contrast->SetValue(f);
}

void HBAOParams::SetAttenuation(float f)
{
  f = clamp<float>(f, 0.f, 1.f);
  attenuation->SetValue(f);
}

void HBAOParams::SetBlurRadius(float f)
{
  int i = clamp<int>((int)f, 1, 32);
  if (!(i&1)) i += 1;
  blurRadius->SetValue((float)i);
  blurFalloff->SetValue( 1.f / (float)(2*i*i));
}

void HBAOParams::SetBlurSharpness(float f)
{
  f = clamp<float>(f, 0.f, 10.f);
  f = pow(10.f, f) - 1.f;
  blurSharpness->SetValue(f);
}


void HBAOParams::SetAngleBias(int i, bool updateCtrl)
{
  i = clamp<int>(i, 0, 90);
  SetAngleBias((float)(i*M_PI / 180.f));
  if (updateCtrl)
    sliderAngleBias->SetValue(i);
}

void HBAOParams::SetRadius(int i, bool updateCtrl)
{
  i = clamp<int>(i, 0, 100);
  SetRadius(i * 0.05f);
  if (updateCtrl)
    sliderRadius->SetValue(i);
}

void HBAOParams::SetNumSteps(int i, bool updateCtrl)
{
  i = clamp<int>(i, 0, 31);
  SetNumSteps(1.f + i);
  if (updateCtrl)
    sliderNumSteps->SetValue(i);
}

void HBAOParams::SetNumDirections(int i, bool updateCtrl)
{
  i = clamp<int>(i, 0, 31);
  SetNumDirections(1.f + i);
  if (updateCtrl)
    sliderNumDirs->SetValue(i);
}

void HBAOParams::SetContrast(int i, bool updateCtrl)
{
  i = clamp<int>(i, 0, 30);
  SetContrast(0.1f*i);
  if (updateCtrl)
    sliderContrast->SetValue(i);
}

void HBAOParams::SetAttenuation(int i, bool updateCtrl)
{
  i = clamp<int>(i, 0, 30);
  SetAttenuation( (1.f/30.f)*i );
  if (updateCtrl)
    sliderAttenuation->SetValue(i);
}

void HBAOParams::SetBlurRadius(int i, bool updateCtrl)
{
  i = clamp<int>(i, 0, 8);
  i = i*2 + 1;
  SetBlurRadius((float)i);
  if (updateCtrl)
    sliderBlurRadius->SetValue(i);
}

void HBAOParams::SetBlurSharpness(int i, bool updateCtrl)
{
  i = clamp<int>(i, 0, 100);
  SetBlurSharpness((float)i / 10.f);
  if (updateCtrl)
    sliderBlurSharpness->SetValue(i);
}

SSDOParams::SSDOParams(iShaderVarStringSet * svStrings, iPostEffect * effect)
{
  enableAO.AttachNew (new csShaderVariable(svStrings->Request ("enable ambient occlusion")));
  enableIL.AttachNew (new csShaderVariable(svStrings->Request ("enable indirect light")));
  sampleRadius.AttachNew (new csShaderVariable(svStrings->Request ("sample radius")));
  datailSampleRadius.AttachNew (new csShaderVariable(svStrings->Request ("detail sample radius")));
  passes.AttachNew (new csShaderVariable(svStrings->Request ("num passes")));
  selfOcclusion.AttachNew (new csShaderVariable(svStrings->Request ("self occlusion")));
  occlusionStrength.AttachNew (new csShaderVariable(svStrings->Request ("occlusion strength")));
  maxDist.AttachNew (new csShaderVariable(svStrings->Request ("max occluder distance")));
  bounceStrength.AttachNew (new csShaderVariable(svStrings->Request ("bounce strength")));
  svDebugAO.AttachNew (new csShaderVariable(svStrings->Request ("debug show ambocc")));
  svDebugIL.AttachNew (new csShaderVariable(svStrings->Request ("debug show indirect light")));

  effect->GetLayer (0)->GetSVContext ()->AddVariable(enableAO);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(enableIL);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(sampleRadius);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(datailSampleRadius);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(passes);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(selfOcclusion);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(occlusionStrength);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(maxDist);
  effect->GetLayer (0)->GetSVContext ()->AddVariable(bounceStrength);

  effect->GetLayer ("composition")->GetSVContext ()->AddVariable(svDebugAO);
  effect->GetLayer ("composition")->GetSVContext ()->AddVariable(svDebugIL);

  SetDebugMode(SSDO_NONE);
}

bool SSDOParams::SetPanel(wxPanel * panel)
{
  sliderRadius = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderSSDOsampleRadius")));
  sliderDetailRadius = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderSSDOdetailSampleRadius")));
  sliderNumPasses = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderSSDOnumPasses")));
  sliderSelfOcclusion = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderSSDOselfOcclusion")));
  sliderOcclusionStrength = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderSSDOocclusionStrength")));
  sliderMaxDist = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderSSDOmaxOccluderDist")));
  sliderBounceStrength = dynamic_cast<wxSlider*> (panel->FindWindow(wxT("sliderSSDObounceStrength")));

  if (!sliderRadius || !sliderDetailRadius || !sliderNumPasses || !sliderSelfOcclusion ||
      !sliderOcclusionStrength || !sliderMaxDist ||  !sliderBounceStrength)
    return false;

  sliderRadius->SetMin(0);
  sliderDetailRadius->SetMin(0);
  sliderNumPasses->SetMin(0);
  sliderSelfOcclusion->SetMin(0);
  sliderOcclusionStrength->SetMin(0);
  sliderMaxDist->SetMin(0);
  sliderBounceStrength->SetMin(0);

  sliderRadius->SetMax(100);
  sliderDetailRadius->SetMax(100);
  sliderNumPasses->SetMax(6);
  sliderSelfOcclusion->SetMax(100);
  sliderOcclusionStrength->SetMax(100);
  sliderMaxDist->SetMax(100);
  sliderBounceStrength->SetMax(100);

  SetAOEnable(false);
  SetILEnable(false);

  SetRadius(15, true);

  SetDetailRadius(30, true);
  SetNumPasses(3, true);

  SetSelfOcclusion(0, true);
  SetOcclusionStrength(80, true);
  SetMaxOcclusionDist(100, true);
  SetBounceStrength(60, true);

  return true;
}

void SSDOParams::SetDebugMode(SSDOParams::ssdoDbgMode mode)
{
  float f1, f2;
  f1 = f2 = 0.0f;
  switch(mode)
  {
  case SSDO_AO:
    f1 = 1.0f;
    break;
  case SSDO_IL:
    f2 = 1.0f;
    break;
  default:
    ;
  }
  svDebugAO->SetValue(f1);
  svDebugIL->SetValue(f2);
}

void SSDOParams::SetRadius(float f)
{
 f = clamp<float>(f, 0.f, 3.f);
 sampleRadius->SetValue(f);

  
}
void SSDOParams::SetRadius(int i, bool updateCtrl)
{
  i = clamp<int>(i, 0, 100);
  SetRadius((float)i * 0.03f);
  if (updateCtrl)
    sliderRadius->SetValue(i);
}

void SSDOParams::SetDetailRadius(float f)
{
  f = clamp<float>(f, 0.f, 3.f);
  datailSampleRadius->SetValue(f);
}
void SSDOParams::SetDetailRadius(int i, bool updateCtrl)
{
  i = clamp<int>(i, 0, 100);
  SetDetailRadius((float)i * 0.03f);
  if (updateCtrl)
    sliderDetailRadius->SetValue(i);
}

void SSDOParams::SetNumPasses(float f)
{
  f = clamp<float>(f, 0.f, 6.f);
  passes->SetValue(f);
}
void SSDOParams::SetNumPasses(int i, bool updateCtrl)
{
  i = clamp<int>(i, 0, 6);
  SetNumPasses((float)i);
  if (updateCtrl)
    sliderNumPasses->SetValue(i);
}

void SSDOParams::SetSelfOcclusion(float f)
{
  f = clamp<float>(f, 0.f, 10.f);
  selfOcclusion->SetValue(f);
}
void SSDOParams::SetSelfOcclusion(int i, bool updateCtrl)
{
  i = clamp<int>(i, 0, 100);
  SetSelfOcclusion((float)i / 10.f);
  if (updateCtrl)
    sliderSelfOcclusion->SetValue(i);
}

void SSDOParams::SetOcclusionStrength(float f)
{
  f = clamp<float>(f, 0.f, 1.f);
  occlusionStrength->SetValue(f);
}
void SSDOParams::SetOcclusionStrength(int i, bool updateCtrl)
{
  i = clamp<int>(i, 0, 100);
  SetOcclusionStrength((float)i / 100.f);
  if (updateCtrl)
    sliderOcclusionStrength->SetValue(i);
}

void SSDOParams::SetMaxOcclusionDist(float f)
{
  f = clamp<float>(f, 0.f, 10.f);
  maxDist->SetValue(f);
}
void SSDOParams::SetMaxOcclusionDist(int i, bool updateCtrl)
{
  i = clamp<int>(i, 0, 100);
  SetMaxOcclusionDist((float)i / 10.f);
  if (updateCtrl)
    sliderMaxDist->SetValue(i);
}

void SSDOParams::SetBounceStrength(float f)
{
  f = clamp<float>(f, 0.f, 10.f);
  bounceStrength->SetValue(f);
}
void SSDOParams::SetBounceStrength(int i, bool updateCtrl)
{
  i = clamp<int>(i, 0, 100);
  SetBounceStrength((float)i / 10.f);
  if (updateCtrl)
    sliderBounceStrength->SetValue(i);
}

bool SSDOParams::SetAOEnable(bool b)
{
  if (b)
    enableAO->SetValue(1.0f);
  else
    enableAO->SetValue(0.0f);

  float f;
  enableIL->GetValue(f);
  return (f == 0.0f && !b);
}

bool SSDOParams::SetILEnable(bool b)
{
  if (b)
    enableIL->SetValue(1.0f);
  else
    enableIL->SetValue(0.0f);

  float f;
  enableAO->GetValue(f);
  return (f == 0.0f && !b);
}
