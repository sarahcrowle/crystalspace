#define CS_IMPLEMENT_PLATFORM_APPLICATION

#include "PostEffectApp.h"
#include "cstool/wxappargconvert.h"

CS_IMPLEMENT_APPLICATION

#if defined(CS_PLATFORM_WIN32)
#ifndef SW_SHOWNORMAL
#define SW_SHOWNORMAL 1
#endif
int main (int argc, const char* const argv[])
{
  return WinMain (GetModuleHandle (0), 0, GetCommandLineA (), SW_SHOWNORMAL);
}
#endif

csFrame * csFrame::m_pInstance = NULL;

IMPLEMENT_APP(PostEffectApp)

iObjectRegistry*  reg;

bool PostEffectApp::OnInit()
{
  CS::WX::AppArgConvert args (argc, argv);
  reg = csInitializer::CreateEnvironment (args.csArgc(), args.csArgv());
  wxString title = wxT("Post Effect Demo");
  csFrame *pFrame = new csFrame(reg, title);
  pFrame->Start();
  return true;
}

int PostEffectApp::OnExit()
{
  csInitializer::DestroyApplication (reg);
  return 0;
}

