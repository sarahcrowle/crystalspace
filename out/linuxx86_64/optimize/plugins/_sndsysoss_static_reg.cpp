/ This file is automatically generated.
#include "cssysdef.h"
#include "csutil/scf.h"

/ Put static linking stuff into own section.
/ The idea is that this allows the section to be swapped out but not
// swapped in again b/c something else in it was needed.
#if !defined(CS_DEBUG) && defined(CS_COMPILER_MSVC)
#pragma const_seg(".CSmetai")
#pragma comment(linker, "/section:.CSmetai,r")
#pragma code_seg(".CSmeta")
#pragma comment(linker, "/section:.CSmeta,er")
#pragma comment(linker, "/merge:.CSmetai=.CSmeta")
#endif

namespace csStaticPluginInit
{
static char const metainfo_sndsysoss[] =
"<?xml version=\"1.0\"?>"
"<!-- sndsysoss.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.sndsys.software.driver.oss</name>"
"        <implementation>SndSysDriverOSS</implementation>"
"        <description>Sound System OSS Software Driver</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
""
;
  #ifndef SndSysDriverOSS_FACTORY_REGISTER_DEFINED 
  #define SndSysDriverOSS_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(SndSysDriverOSS) 
  #endif

class sndsysoss
{
SCF_REGISTER_STATIC_LIBRARY(sndsysoss,metainfo_sndsysoss)
  #ifndef SndSysDriverOSS_FACTORY_REGISTERED 
  #define SndSysDriverOSS_FACTORY_REGISTERED 
    SndSysDriverOSS_StaticInit SndSysDriverOSS_static_init__; 
  #endif
public:
 sndsysoss();
};
sndsysoss::sndsysoss() {}

}