/*
    Copyright (C) 1998 by Jorrit Tyberghein

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "cssysdef.h"
#include "csutil/csinput.h"
#include "csutil/scf.h"
#include "csutil/setenv.h"
#include "csutil/sysfunc.h"
#include "csgeom/csrect.h"
#include "csutil/cfgacc.h"
#include "iutil/plugin.h"
#include "iutil/eventh.h"
#include "iutil/comp.h"
#include "ivideo/texture.h"
#include "iengine/texture.h"
#include "iutil/cfgfile.h"
#include "iutil/eventq.h"
#include "iutil/objreg.h"
#include "ivaria/reporter.h"

#include "plugins/video/canvas/openglegl/egl2d.h"

#ifdef CS_HAVE_XRENDER
#include <X11/extensions/Xrender.h>
#endif

SCF_IMPLEMENT_FACTORY (csGraphics2DEGL)

#define XWIN_SCF_ID "crystalspace.window.x"
#define USE_FULL_GL 0

// csGraphics2DEGL function
csGraphics2DEGL::csGraphics2DEGL (iBase *iParent) :
  scfImplementationType (this, iParent), cmap (0), hardwareaccelerated(false),
  transparencyRequested (false), transparencyAvailable (false)
{
}

void csGraphics2DEGL::Report (int severity, const char* msg, ...)
{
  va_list arg;
  va_start (arg, msg);
  csRef<iReporter> rep (csQueryRegistry<iReporter> (object_reg));
  if (rep)
    rep->ReportV (severity, "crystalspace.canvas.egl2d", msg, arg);
  else
  {
    csPrintfV (msg, arg);
    csPrintf ("\n");
  }
  va_end (arg);
}

bool csGraphics2DEGL::Initialize (iObjectRegistry *object_reg)
{
  xvis = 0;
  hardwareaccelerated = false;

  csRef<iPluginManager> plugin_mgr (
  	csQueryRegistry<iPluginManager> (object_reg));
  xwin = csLoadPlugin<iXWindow> (plugin_mgr, XWIN_SCF_ID);
  if (!xwin)
  {
    Report (CS_REPORTER_SEVERITY_WARNING,
      "Could not create an instance of " XWIN_SCF_ID "!");
    return false;
  }

  x_display = xwin->GetDisplay ();
  screen_num = xwin->GetScreen ();
  {
    /* According to http://standards.freedesktop.org/wm-spec/wm-spec-latest.html#id2552725
       (section "Compositing Managers") presence of a compositing window manager
       is signalled by the WM owning a selection "_NET_WM_CM_Sn", n being the screen
       number. */
    csString selname;
    selname.Format ("_NET_WM_CM_S%d", screen_num);
    compositingManagerPresenceSelection = XInternAtom (x_display, selname.GetData(), True);
  }
  
  dpy = eglGetDisplay((EGLNativeDisplayType) x_display);
  if(dpy == EGL_NO_DISPLAY)
  {
    Report (CS_REPORTER_SEVERITY_WARNING,
      "Could not create an EGL Display!");
    return false;
  }

  if ( !eglInitialize( dpy, &egl_major, &egl_minor ) ) {
    Report (CS_REPORTER_SEVERITY_WARNING,
      "Unable to initialize EGL!");
    return false;
  }
  
  // TODO: Do we reallly have to ?
  xwin->SetHWMouseMode (hwMouse);

  // Create the event outlet
  csRef<iEventQueue> q (csQueryRegistry<iEventQueue> (object_reg));
  if (q != 0)
    EventOutlet = q->CreateEventOutlet (this);

  return true;
}

csGraphics2DEGL::~csGraphics2DEGL ()
{
  // Destroy your graphic interface
  XFree ((void*)xvis);
  Close ();
}

bool csGraphics2DEGL::Open ()
{
  if (is_open) return true;

  #if USE_FULL_GL
  static const EGLint ctx_attribs[] = {
    EGL_NONE
  };
#else
  static const EGLint ctx_attribs[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };
#endif

  Report (CS_REPORTER_SEVERITY_NOTIFY, "Opening EGL2D");
  // We now select the visual here as with a mesa bug it is not possible
  // to destroy double buffered contexts and then create a single buffered
  // one.

  // TODO: This is like a big dick in my ass right now, no clue how to take it out
  // I will just let the pain flow for now
  // ext.InitGLX_ARB_multisample (dpy, screen_num);

  if (!ChooseVisual ())
    return false;

  cmap = XCreateColormap (x_display, RootWindow (dpy, xvis->screen),
    xvis->visual, AllocNone);

  xwin->SetColormap (cmap);
  xwin->SetVisualInfo (xvis);
  xwin->SetCanvas (static_cast<iGraphics2D*> (this));
  if (!xwin->Open ())
  {
    Report (CS_REPORTER_SEVERITY_ERROR,
      "Failed to open the X-Window!");
    return false;
  }
  window = xwin->GetWindow ();

  #if USE_FULL_GL /* XXX fix this when eglBindAPI() works */
   eglBindAPI(EGL_OPENGL_API);
#else
   eglBindAPI(EGL_OPENGL_ES_API);
#endif
  
  active_GLContext = eglCreateContext (dpy, egl_config, EGL_NO_CONTEXT, ctx_attribs );
  if (!active_GLContext) {
      Report (CS_REPORTER_SEVERITY_ERROR,
      "Failed to create EGL Context!");
    return false;
   }
  
  egl_surface = eglCreateWindowSurface(dpy, egl_config, xwin->GetWindow(), NULL);
   if (!egl_surface) {
      Report (CS_REPORTER_SEVERITY_ERROR,
      "Failed to create EGL Surface!");
    return false;
   }
  
  //XMapWindow(x_display, xwin);
   if (!eglMakeCurrent(dpy, egl_surface, egl_surface, active_GLContext)) {
      Report (CS_REPORTER_SEVERITY_ERROR,
      "Failed to make EGL Current!");
    return false;
   }

  XSync (x_display, False);
  
  GetCurrentAttributes ();
  
  // Open your graphic interface
  if (!csGraphics2DGLCommon::Open ())
    return false;

  xwin->SetTitle (win_title);

  return true;
}

void csGraphics2DEGL::Close (void)
{
  if (!is_open) return;
    
  // Close your graphic interface
  csGraphics2DGLCommon::Close ();
  if (active_GLContext != 0)
  {
    eglDestroyContext(dpy, active_GLContext);
    eglDestroySurface(dpy, egl_surface);
    eglTerminate(dpy);
    active_GLContext = 0;
  }

  if (xwin)
    xwin->Close ();
}

const char* csGraphics2DEGL::GetVersionString (const char* ver)
{
  if (strcmp (ver, "mesa") == 0)
  {
    static const char needle[] = "Mesa ";
    const char* glVersion = (const char*)glGetString (GL_VERSION);
    const char* p = strstr (glVersion, needle);
    if (p != 0)
      return p + sizeof (needle) - 1;
    return 0;
  }
  else
    return csGraphics2DGLCommon::GetVersionString (ver);
}

static const char *visual_class_name (int cls)
{
  switch (cls)
  {
    case StaticColor:
      return "StaticColor";
    case PseudoColor:
      return "PseudoColor";
    case StaticGray:
      return "StaticGray";
    case GrayScale:
      return "GrayScale";
    case TrueColor:
      return "TrueColor";
    case DirectColor:
      return "DirectColor";
    default:
      return "";
  }
}

bool csGraphics2DEGL::ChooseVisual ()
{
  EGLint vid;

  static const EGLint attribs[] = {
    EGL_RED_SIZE, 1,
    EGL_GREEN_SIZE, 1,
    EGL_BLUE_SIZE, 1,
    EGL_DEPTH_SIZE, 1,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
  };

  screen_num = xwin->GetScreen ();
  
  if (!eglChooseConfig( dpy, attribs, &egl_config, 1, &num_configs)) {
    Report (CS_REPORTER_SEVERITY_ERROR,
    "EGL Failed to choose a configuration!");
    return false;
  }

  assert(config);
  assert(num_configs > 0);

  if (!eglGetConfigAttrib(dpy, egl_config, EGL_NATIVE_VISUAL_ID, &vid)) {
    Report (CS_REPORTER_SEVERITY_ERROR,
    "EGL Failed to get Config Attributes!");
    return false;
  }

  /* The X window visual must match the EGL config */
  visual_template.visualid = vid;
  xvis = XGetVisualInfo(x_display, VisualIDMask, &visual_template, &num_visuals);
  if (!xvis) {
    printf("Error: couldn't get X visual\n");
    exit(1);
  }
  return true;
}

void csGraphics2DEGL::GetCurrentAttributes ()
{
  // TODO: Fix this. Fuck it right now, lets try to make it work somehow for now.
  hardwareaccelerated = true;
  Report (CS_REPORTER_SEVERITY_NOTIFY, "Video driver GL/X version %s",
    hardwareaccelerated ? "(direct renderer)" : "(indirect renderer)");
  if (!hardwareaccelerated)
  {
    Report (CS_REPORTER_SEVERITY_WARNING,
      "Indirect rendering may indicate a flawed OpenGL setup if you run on "
      "a local X server.");
  }

  Depth = xvis->depth;

  Report (CS_REPORTER_SEVERITY_NOTIFY, "Visual ID: %p, %dbit %s",
    xvis->visualid, Depth, visual_class_name (xvis->c_class));

  int ctype, frame_buffer_depth, size_depth_buffer, level;
  
  eglGetConfigAttrib (dpy, egl_config, EGL_BIND_TO_TEXTURE_RGBA, &ctype);
  eglGetConfigAttrib (dpy, egl_config, EGL_BUFFER_SIZE, &frame_buffer_depth);
  eglGetConfigAttrib (dpy, egl_config, EGL_DEPTH_SIZE, &size_depth_buffer);
  eglGetConfigAttrib (dpy, egl_config, EGL_LEVEL, &level);
  
  int r_bits, g_bits, b_bits, color_bits = 0;
  int alpha_bits = 0;
  if (ctype)
  {
    eglGetConfigAttrib (dpy, egl_config, EGL_RED_SIZE, &r_bits);
    color_bits += r_bits;
    eglGetConfigAttrib (dpy, egl_config, EGL_GREEN_SIZE, &g_bits);
    color_bits += g_bits;
    eglGetConfigAttrib (dpy, egl_config, EGL_BLUE_SIZE, &b_bits);
    color_bits += b_bits;
    eglGetConfigAttrib (dpy, egl_config, EGL_ALPHA_SIZE, &alpha_bits);
  }

  // Report Info
  currentFormat[glpfvColorBits] = color_bits;
  currentFormat[glpfvAlphaBits] = alpha_bits;
  currentFormat[glpfvDepthBits] = size_depth_buffer;
  int stencilSize = 0;
  eglGetConfigAttrib (dpy, egl_config, EGL_STENCIL_SIZE, &stencilSize);
  currentFormat[glpfvStencilBits] = stencilSize;
  /*int accumBits = 0;
  int accumAlpha = 0;
  {
    int dummy;
    glXGetConfig (dpy, xvis, GLX_ACCUM_RED_SIZE, &dummy);
    accumBits += dummy;
    glXGetConfig (dpy, xvis, GLX_ACCUM_GREEN_SIZE, &dummy);
    accumBits += dummy;
    glXGetConfig (dpy, xvis, GLX_ACCUM_BLUE_SIZE, &dummy);
    accumBits += dummy;
    glXGetConfig (dpy, xvis, GLX_ACCUM_ALPHA_SIZE, &accumAlpha);
  }
  currentFormat[glpfvAccumColorBits] = accumBits;
  currentFormat[glpfvAccumAlphaBits] = accumAlpha;
  
  if (ext.CS_GLX_ARB_multisample)
  {
    int v;
    glXGetConfig (dpy, xvis, GLX_SAMPLES_ARB, &v);
    currentFormat[glpfvMultiSamples] = v;
  }
  */
  if (ctype)
  {
    Report (CS_REPORTER_SEVERITY_NOTIFY, "R%d:G%d:B%d:A%d, ",
      r_bits, g_bits, b_bits, alpha_bits);
  }
    
  Report (CS_REPORTER_SEVERITY_NOTIFY, "level %d, double buffered", level);
}

bool csGraphics2DEGL::GetWorkspaceDimensions (int& width, int& height)
{
  return xwin->GetWorkspaceDimensions (width, height);
}

bool csGraphics2DEGL::AddWindowFrameDimensions (int& width, int& height)
{
  return xwin->AddWindowFrameDimensions (width, height);
}

bool csGraphics2DEGL::PerformExtensionV (char const* command, va_list args)
{
  if (!strcasecmp (command, "hardware_accelerated"))
  {
    bool* hasaccel = (bool*)va_arg (args, bool*);
    *hasaccel = hardwareaccelerated;
    return true;
  }
  if (!strcasecmp (command, "fullscreen"))
  {
    xwin->SetFullScreen (!xwin->GetFullScreen ());
    return true;
  }
  if (!strcasecmp (command, "setglcontext"))
  {
    eglMakeCurrent(dpy, egl_surface, egl_surface, active_GLContext);
    return true;
  }
  return csGraphics2DGLCommon::PerformExtensionV (command, args);
}

void csGraphics2DEGL::Print (csRect const* /*area*/)
{
  eglSwapBuffers (dpy,egl_surface);
}

bool csGraphics2DEGL::IsWindowTransparencyAvailable()
{
#ifdef CS_HAVE_XRENDER
  if (compositingManagerPresenceSelection == None) return false;
  /* According to http://standards.freedesktop.org/wm-spec/wm-spec-latest.html#id2552725
     (section "Compositing Managers") presence of a compositing window manager
     is signalled by the WM owning a selection "_NET_WM_CM_Sn", n being the screen
     number. Check that. */
  return (XGetSelectionOwner (x_display, compositingManagerPresenceSelection) != None);
#else
  return false;
#endif
}

bool csGraphics2DEGL::SetWindowTransparent (bool transparent)
{
#ifdef CS_HAVE_XRENDER
  if (!is_open)
  {
    transparencyRequested = transparent;
    return true;
  }
  else
  {
    // This train has left the station.
    return false;
  }
#else
  return false;
#endif
}

bool csGraphics2DEGL::GetWindowTransparent ()
{
  return is_open ? transparencyAvailable : transparencyRequested;
}

bool csGraphics2DEGL::SetWindowDecoration (WindowDecoration decoration, bool flag)
{
  return xwin->SetWindowDecoration (decoration, flag);
}

bool csGraphics2DEGL::GetWindowDecoration (WindowDecoration decoration)
{
  bool flag;
  if (xwin->GetWindowDecoration (decoration, flag))
    return flag;
  return csGraphics2D::GetWindowDecoration (decoration);
}

void csGraphics2DEGL::SetFullScreen (bool yesno)
{
  csGraphics2D::SetFullScreen (yesno);
  xwin->SetFullScreen (yesno);
}

void csGraphics2DEGL::AllowResize (bool iAllow)
{
  AllowResizing = iAllow;
  xwin->AllowResize (iAllow);
}

bool csGraphics2DEGL::ForceCanvasResize (int w, int h)
{
  if (!csGraphics2DGLCommon::ForceCanvasResize (w, h)) return false;
  xwin->Resize (w, h);
  return true;
}

#undef XWIN_SCF_ID
