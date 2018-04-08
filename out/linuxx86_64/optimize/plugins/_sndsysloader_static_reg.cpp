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
static char const metainfo_sndsysloader[] =
"<?xml version=\"1.0\"?>"
"<!-- sndsysloader.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.sndsys.element.loader</name>"
"        <implementation>SndSysLoader</implementation>"
"        <description>Sound System Element Loader</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
""
;
  #ifndef SndSysLoader_FACTORY_REGISTER_DEFINED 
  #define SndSysLoader_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(SndSysLoader) 
  #endif

class sndsysloader
{
SCF_REGISTER_STATIC_LIBRARY(sndsysloader,metainfo_sndsysloader)
  #ifndef SndSysLoader_FACTORY_REGISTERED 
  #define SndSysLoader_FACTORY_REGISTERED 
    SndSysLoader_StaticInit SndSysLoader_static_init__; 
  #endif
public:
 sndsysloader();
};
sndsysloader::sndsysloader() {}

}