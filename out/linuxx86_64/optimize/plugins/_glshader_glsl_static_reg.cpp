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
static char const metainfo_glshader_glsl[] =
"<?xml version=\"1.0\"?>"
"<!-- glshader_glsl.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.graphics3d.shader.glsl</name>"
"        <implementation>csGLShader_GLSL</implementation>"
"        <description>OpenGL Shading Language shaderprogram provider for Graphics3D</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef csGLShader_GLSL_FACTORY_REGISTER_DEFINED 
  #define csGLShader_GLSL_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(csGLShader_GLSL) 
  #endif

class glshader_glsl
{
SCF_REGISTER_STATIC_LIBRARY(glshader_glsl,metainfo_glshader_glsl)
  #ifndef csGLShader_GLSL_FACTORY_REGISTERED 
  #define csGLShader_GLSL_FACTORY_REGISTERED 
    csGLShader_GLSL_StaticInit csGLShader_GLSL_static_init__; 
  #endif
public:
 glshader_glsl();
};
glshader_glsl::glshader_glsl() {}

}