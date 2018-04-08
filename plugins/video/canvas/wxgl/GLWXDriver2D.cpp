/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
    Copyright (C) 2004 by Daniel Fryer and Peter Amstutz

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

#include "csutil/custom_new_disable.h"

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "csutil/custom_new_enable.h"

#include "csutil/cfgacc.h"
#include "csutil/csinput.h"
#include "csutil/csuctransform.h"
#include "csutil/eventnames.h"
#include "csutil/scf.h"
#include "csutil/sysfunc.h"
#include "csgeom/csrect.h"

#include "iengine/texture.h"
#include "iutil/cfgfile.h"
#include "iutil/comp.h"
#include "iutil/eventh.h"
#include "iutil/eventq.h"
#include "iutil/objreg.h"
#include "iutil/plugin.h"
#include "ivaria/reporter.h"
#include "ivideo/texture.h"

#ifdef WIN32
#include "csutil/win32/win32.h"
#include "csutil/win32/wintools.h"
#include <GL/gl.h>
#include "csplugincommon/opengl/glcommon2d.h"
#elif !defined(CS_PLATFORM_MACOSX)
#define USE_GLX
#endif

#ifdef USE_GLX
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>
#endif

#include "GLWXDriver2D.h"

SCF_IMPLEMENT_FACTORY (csGraphics2DWX)

// csGraphics2DWX function
csGraphics2DWX::csGraphics2DWX (iBase *iParent) :
    scfImplementationType (this, iParent), myParent (0), theCanvas (0)
{
#if WIN32
  cursorIsHidden = false;
#endif
}

void csGraphics2DWX::AlertV (int type, const char* title,
    const char* okMsg, const char* msg, va_list args)
{
  csString message;
  message.FormatV (msg, args);
  switch (type)
  {
    case CS_ALERT_ERROR:
      wxMessageBox (wxString::FromUTF8 (message), wxT("Error!"),
	  wxICON_ERROR | wxOK, myParent);
      break;
    case CS_ALERT_WARNING:
      wxMessageBox (wxString::FromUTF8 (message), wxT("Warning!"),
	  wxICON_EXCLAMATION | wxOK, myParent);
      break;
    default:
    case CS_ALERT_NOTE:
      wxMessageBox (wxString::FromUTF8 (message), wxT("Note!"),
	  wxICON_INFORMATION | wxOK, myParent);
      break;
  }
}

void csGraphics2DWX::Report (int severity, const char* msg, ...)
{
  va_list arg;
  va_start (arg, msg);
  csRef<iReporter> rep (csQueryRegistry<iReporter> (object_reg));
  if (rep)
    rep->ReportV (severity, "crystalspace.canvas.glwx2d", msg, arg);
  else
  {
    csPrintfV (msg, arg);
    csPrintf ("\n");
  }
  va_end (arg);
}

bool csGraphics2DWX::Initialize (iObjectRegistry *object_reg)
{
  if (!csGraphics2DGLCommon::Initialize (object_reg))
    return false;

  // Create the event outlet
  csRef<iEventQueue> q (csQueryRegistry<iEventQueue> (object_reg));
  if (q != 0)
    EventOutlet = q->CreateEventOutlet (this);

  return true;
}

bool csGraphics2DWX::SetMousePosition (int x, int y)
{
  myParent->WarpPointer (x, y);
  return true;
}

bool csGraphics2DWX::SetMouseCursor (csMouseCursorID shape)
{
  int cursorId;
  switch (shape)
  {
    case csmcNone:
      cursorId = wxCURSOR_BLANK;
#if WIN32
      // Workaround for windows where setting a blank cursor doesn't appear to work otherwise.
      {
        wxCursor cursor (cursorId);
        myParent->SetCursor (cursor);
	if (!cursorIsHidden)
	{
	  ShowCursor (0);
	  cursorIsHidden = true;
	}
	return true;
      }
#endif
      break;
    case csmcArrow: cursorId = wxCURSOR_ARROW; break;
    case csmcLens: cursorId = wxCURSOR_MAGNIFIER; break;
    case csmcCross: cursorId = wxCURSOR_CROSS; break;
    case csmcPen: cursorId = wxCURSOR_PENCIL; break;
    case csmcMove: cursorId = wxCURSOR_LEFT_BUTTON; break;
    case csmcSizeNWSE: cursorId = wxCURSOR_SIZENWSE; break;
    case csmcSizeNESW: cursorId = wxCURSOR_SIZENESW; break;
    case csmcSizeNS: cursorId = wxCURSOR_SIZENS; break;
    case csmcSizeEW: cursorId = wxCURSOR_SIZEWE; break;
    case csmcStop: cursorId = wxCURSOR_NO_ENTRY; break;
    case csmcWait: cursorId = wxCURSOR_WAIT; break;
    default: cursorId = wxCURSOR_ARROW; break;
  }
#if WIN32
  if (cursorIsHidden)
  {
    ShowCursor (1);
    cursorIsHidden = false;
  }
#endif
  wxCursor cursor (cursorId);
  //myParent->SetCursor (cursor);
  theCanvas->SetCursor (cursor);
  return true;
}

void csGraphics2DWX::SetParent(wxWindow* wx)
{
  myParent = wx;
  if (!myParent) return;

  // Recreate the canvas if it has been deleted
  if (!theCanvas && is_open)
  {
    CreateCanvas();
    // TODO: reset previous config such as mouse cursor
  }

  if (theCanvas) theCanvas->Reparent(myParent);
}

wxWindow* csGraphics2DWX::GetWindow()
{
  return theCanvas;
}

#ifdef WIN32
HWND csGraphics2DWX::GetWindowHandle()
{
  return (HWND)myParent->GetHWND();
}
#endif

void* csGraphics2DWX::GetProcAddress (const char *funcname)
{
#ifdef WIN32
  return (void*)wglGetProcAddress (funcname);
#elif defined(USE_GLX)
    return (void*)glXGetProcAddressARB ((const GLubyte*) funcname);
#else
  return 0;
#endif
}

csGraphics2DWX::~csGraphics2DWX ()
{
  Close ();
  // All instances of theCanvas are always destroyed by wxWindows
}

#ifdef USE_GLX
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
#endif

void csGraphics2DWX::CreateCanvas ()
{
  /*
    GLPixelFormat format;

    const int colorBits = format[glpfvColorBits];
    const int colorComponentSize =
    ((colorBits % 32) == 0) ? colorBits / 4 : colorBits / 3;
    const int accumBits = format[glpfvAccumColorBits];
    const int accumComponentSize =
    ((accumBits % 32) == 0) ? accumBits / 4 : accumBits / 3;
  */

#ifdef WIN32

  csGLPixelFormatPicker picker (this);
  PIXELFORMATDESCRIPTOR pfd;
  FindPixelFormat (picker, pfd);

  currentFormat[glpfvColorBits] = pfd.cColorBits;
  currentFormat[glpfvAlphaBits] = pfd.cAlphaBits;
  currentFormat[glpfvDepthBits] = pfd.cDepthBits;
  currentFormat[glpfvStencilBits] = pfd.cStencilBits;
  currentFormat[glpfvAccumColorBits] = pfd.cAccumBits;
  currentFormat[glpfvAccumAlphaBits] = pfd.cAccumAlphaBits;

  Depth = pfd.cColorBits; 

  int desired_attributes[] =
  {
      WX_GL_RGBA,
      WX_GL_DOUBLEBUFFER,
      WX_GL_DEPTH_SIZE, currentFormat[glpfvDepthBits],
      WX_GL_MIN_RED, pfd.cRedBits,
      WX_GL_MIN_BLUE, pfd.cBlueBits,
      WX_GL_MIN_GREEN, pfd.cGreenBits,
      WX_GL_MIN_ALPHA, currentFormat[glpfvAlphaBits],
      WX_GL_STENCIL_SIZE, currentFormat[glpfvStencilBits],
      WX_GL_MIN_ACCUM_RED, pfd.cAccumRedBits,
      WX_GL_MIN_ACCUM_BLUE, pfd.cAccumBlueBits,
      WX_GL_MIN_ACCUM_GREEN, pfd.cAccumGreenBits,
      WX_GL_MIN_ACCUM_ALPHA, currentFormat[glpfvAccumAlphaBits],
      0
  };
#else
    int desired_attributes[] =
    {
      WX_GL_RGBA,
      WX_GL_DOUBLEBUFFER,
      0
    };
#endif

  /*WX_GL_DEPTH_SIZE, format[glpfvDepthBits],
    WX_GL_MIN_RED, colorComponentSize,
    WX_GL_MIN_BLUE, colorComponentSize,
    WX_GL_MIN_GREEN, colorComponentSize,*/
  /*WX_GL_MIN_ALPHA, format[glpfvAlphaBits],
    WX_GL_STENCIL_SIZE, format[glpfvStencilBits],
    WX_GL_MIN_ACCUM_RED, accumComponentSize,
    WX_GL_MIN_ACCUM_BLUE, accumComponentSize,
    WX_GL_MIN_ACCUM_GREEN, accumComponentSize,
    WX_GL_MIN_ACCUM_ALPHA, format[glpfvAccumAlphaBits],*/

  int w, h;
  myParent->GetClientSize(&w, &h);

  theCanvas = new csGLCanvas(this, myParent, wxID_ANY,
                             wxPoint(0, 0), wxSize(w, h), 0, wxT(""),
                             desired_attributes);
  if(!theCanvas) Report(CS_REPORTER_SEVERITY_ERROR, "Failed creating GL Canvas!");

  theCanvas->Show(true);
  theCanvas->SetCurrent();
}

bool csGraphics2DWX::Open()
{
  if (is_open) return true;

#ifdef WIN32
  csRef<iWin32Assistant> w32(csQueryRegistry<iWin32Assistant> (object_reg));
  if(w32 != 0) w32->UseOwnMessageLoop(false);
#endif

  Report (CS_REPORTER_SEVERITY_NOTIFY, "Opening WX-GL canvas!\n");

  if(myParent == 0)
  {
    Report (CS_REPORTER_SEVERITY_ERROR,
            "Parent frame in wxGLCanvas not initialized!!");
    return false;
  }

  AllowResize(true);
  CreateCanvas();

#ifdef WIN32
#elif defined(USE_GLX)
  {
    Display* dpy = (Display*) wxGetDisplay ();
    GLXContext active_GLContext = glXGetCurrentContext();
    XVisualInfo *xvis = theCanvas->GetXVisualInfo();

    Report (CS_REPORTER_SEVERITY_NOTIFY, "Video driver GL/X version %s",
      glXIsDirect (dpy, active_GLContext) ? "(direct renderer)" :
      "(indirect renderer)");
    if (!glXIsDirect (dpy, active_GLContext))
    {
      Report (CS_REPORTER_SEVERITY_WARNING,
              "Indirect rendering may indicate a flawed OpenGL setup "
              "if you run on a local X server.");
    }

    Depth = xvis->depth;

    Report (CS_REPORTER_SEVERITY_NOTIFY, "Visual ID: %lx, %dbit %s",
      xvis->visualid, Depth, visual_class_name (xvis->c_class));

    int ctype, frame_buffer_depth, size_depth_buffer, level;
    glXGetConfig(dpy, xvis, GLX_RGBA, &ctype);
    //glXGetConfig(dpy, xvis, GLX_DOUBLEBUFFER, &double_buffer);
    glXGetConfig(dpy, xvis, GLX_BUFFER_SIZE, &frame_buffer_depth);
    glXGetConfig(dpy, xvis, GLX_DEPTH_SIZE, &size_depth_buffer);
    glXGetConfig(dpy, xvis, GLX_LEVEL, &level);

    int r_bits, g_bits, b_bits, color_bits = 0;
    int alpha_bits = 0;
    if (ctype)
    {
      glXGetConfig(dpy, xvis, GLX_RED_SIZE, &r_bits);
      color_bits += r_bits;
      glXGetConfig(dpy, xvis, GLX_GREEN_SIZE, &g_bits);
      color_bits += g_bits;
      glXGetConfig(dpy, xvis, GLX_BLUE_SIZE, &b_bits);
      color_bits += b_bits;
      glXGetConfig(dpy, xvis, GLX_ALPHA_SIZE, &alpha_bits);
    }

    // Report Info
    currentFormat[glpfvColorBits] = color_bits;
    currentFormat[glpfvAlphaBits] = alpha_bits;
    currentFormat[glpfvDepthBits] = size_depth_buffer;
    int stencilSize = 0;
    glXGetConfig(dpy, xvis, GLX_STENCIL_SIZE, &stencilSize);
    currentFormat[glpfvStencilBits] = stencilSize;
    int accumBits = 0;
    int accumAlpha = 0;
    {
      int dummy;
      glXGetConfig(dpy, xvis, GLX_RED_SIZE, &dummy);
      accumBits += dummy;
      glXGetConfig(dpy, xvis, GLX_GREEN_SIZE, &dummy);
      accumBits += dummy;
      glXGetConfig(dpy, xvis, GLX_BLUE_SIZE, &dummy);
      accumBits += dummy;
      glXGetConfig(dpy, xvis, GLX_ALPHA_SIZE, &accumAlpha);
    }
    currentFormat[glpfvAccumColorBits] = accumBits;
    currentFormat[glpfvAccumAlphaBits] = accumAlpha;

    if (ctype)
    {
      Report (CS_REPORTER_SEVERITY_NOTIFY, "R%d:G%d:B%d:A%d, ",
              r_bits, g_bits, b_bits, alpha_bits);
    }
  }
#endif

  // Open your graphic interface
  if (!csGraphics2DGLCommon::Open ())
  {
    Report(CS_REPORTER_SEVERITY_ERROR, "Graphics2DGLCommon failed to open");
    return false;
  }

  return true;
}

#ifdef WIN32

struct DummyWndInfo
{
  int pixelFormat;
  csGraphics2DWX* this_;
  csGraphics2DGLCommon::GLPixelFormat* chosenFormat;
};

int csGraphics2DWX::FindPixelFormatGDI (HDC hDC, 
                                        csGLPixelFormatPicker& picker)
{
  int newPixelFormat = 0;
  GLPixelFormat& format = currentFormat;
  while (picker.GetNextFormat (format))
  {
    PIXELFORMATDESCRIPTOR pfd = {
      sizeof(PIXELFORMATDESCRIPTOR),  /* size */
      1,                              /* version */
      PFD_SUPPORT_OPENGL |
      PFD_DOUBLEBUFFER |              /* support double-buffering */
      PFD_DRAW_TO_WINDOW,
      PFD_TYPE_RGBA,                  /* color type */
      format[glpfvColorBits],         /* prefered color depth */
      0, 0, 0, 0, 0, 0,               /* color bits (ignored) */
      format[glpfvAlphaBits],         /* no alpha buffer */
      0,                              /* alpha bits (ignored) */
      format[glpfvAccumColorBits],	/* accumulation buffer */
      0, 0, 0,			/* accum bits (ignored) */
      format[glpfvAccumAlphaBits],	/* accum alpha bits */
      format[glpfvDepthBits],         /* depth buffer */
      format[glpfvStencilBits],	/* stencil buffer */
      0,                              /* no auxiliary buffers */
      PFD_MAIN_PLANE,                 /* main layer */
      0,                              /* reserved */
      0, 0, 0                         /* no layer, visible, damage masks */
    };

    newPixelFormat = ChoosePixelFormat (hDC, &pfd);

    if (newPixelFormat == 0)
    {
      DWORD error = GetLastError();
      char* msg = cswinGetErrorMessage (error);
      Report (CS_REPORTER_SEVERITY_ERROR,
        "ChoosePixelFormat failed: %s [%" PRId32 "]",
        msg, error);
      delete[] msg;
    }

    if (DescribePixelFormat (hDC, newPixelFormat, 
      sizeof(PIXELFORMATDESCRIPTOR), &pfd) == 0)
    {
      DWORD error = GetLastError();
      char* msg = cswinGetErrorMessage (error);
      Report (CS_REPORTER_SEVERITY_ERROR,
        "DescribePixelFormat failed: %s [%" PRId32 "]",
        msg, error);
      delete[] msg;
    }

    if (!(pfd.dwFlags & PFD_GENERIC_FORMAT) ||
      (pfd.dwFlags & PFD_GENERIC_ACCELERATED))
    {
      return newPixelFormat;
    }
  }

  return newPixelFormat;
}

int csGraphics2DWX::FindPixelFormat (csGLPixelFormatPicker& picker,
                                     PIXELFORMATDESCRIPTOR& pfd)
{
  static const char* dummyClassName = "CSGL_DummyWindow";

  HINSTANCE ModuleHandle = GetModuleHandle(0);

  WNDCLASSA wc;
  wc.hCursor        = 0;
  wc.hIcon	    = 0;
  wc.lpszMenuName   = 0;
  wc.lpszClassName  = dummyClassName;
  wc.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.hInstance      = ModuleHandle;
  wc.style          = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  wc.lpfnWndProc    = DefWindowProc;
  wc.cbClsExtra     = 0;
  wc.cbWndExtra     = 0;

  if (!RegisterClassA (&wc)) return false;

  HWND wnd = CreateWindowA (dummyClassName, 0, 0, CW_USEDEFAULT, 
    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,
    ModuleHandle, 0);

  int pixel_format = FindPixelFormatGDI (GetDC (wnd), picker);

  if (DescribePixelFormat (GetDC (wnd), pixel_format,
    sizeof(PIXELFORMATDESCRIPTOR), &pfd) == 0)
  {
    DWORD error = GetLastError();
    char* msg = cswinGetErrorMessage (error);
    Report (CS_REPORTER_SEVERITY_ERROR,
      "DescribePixelFormat failed: %s [%" PRId32 "]",
      msg, error);
    delete[] msg;
  }

  DestroyWindow (wnd);

  UnregisterClassA (dummyClassName, ModuleHandle);

  ext.Reset();

  return 0;
}

#endif

void csGraphics2DWX::Close(void)
{
  if (!is_open) return;

  // Close your graphic interface
  csGraphics2DGLCommon::Close ();
}

bool csGraphics2DWX::BeginDraw(void)
{
  if (!theCanvas) return false;

  theCanvas->SetCurrent();

  //glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT);
  if (csGraphics2DGLCommon::BeginDraw())
  {
    return true;
  }
  else
  {
    Report(CS_REPORTER_SEVERITY_ERROR,"Common BeginDraw fails");
    return false;
  }
}

void csGraphics2DWX::FinishDraw(void)
{
  csGraphics2DGLCommon::FinishDraw();
}

bool csGraphics2DWX::PerformExtensionV (char const* command, va_list args)
{
  if (!strcasecmp (command, "hardware_accelerated"))
  {
    return true;
  }
  if (!strcasecmp (command, "fullscreen"))
  {
    return false;
  }
  if (!strcasecmp (command, "setglcontext"))
  {
    theCanvas->SetCurrent();
    return true;
  }
  return csGraphics2DGLCommon::PerformExtensionV (command, args);
}

void csGraphics2DWX::Print (csRect const* /*area*/)
{
  //Make some sort of swapbuffers call
  glFlush();
  theCanvas->SwapBuffers();
}


void csGraphics2DWX::SetFullScreen (bool yesno)
{
  //Do nothing here, not valid for this type of canvas
}

void csGraphics2DWX::AllowResize (bool iAllow)
{
  AllowResizing = iAllow;
  // TODO: do something?
}

// -------------------------------------------------------------------

BEGIN_EVENT_TABLE(csGLCanvas, wxGLCanvas)
  EVT_SIZE(csGLCanvas::OnSize)
  EVT_PAINT(csGLCanvas::OnPaint)
  EVT_ERASE_BACKGROUND(csGLCanvas::OnEraseBackground)
  EVT_KEY_DOWN(csGLCanvas::OnKeyDown)
  EVT_KEY_UP(csGLCanvas::OnKeyUp)
  EVT_CHAR(csGLCanvas::OnKeyChar)
  EVT_MOUSE_EVENTS(csGLCanvas::OnMouseEvent)
  EVT_MOUSE_CAPTURE_LOST(csGLCanvas::OnMouseCaptureLost)
  EVT_SET_FOCUS(csGLCanvas::OnSetFocus)
  EVT_KILL_FOCUS(csGLCanvas::OnKillFocus)
END_EVENT_TABLE()

csGLCanvas::csGLCanvas(csGraphics2DWX* g, wxWindow *parent,
                       wxWindowID id,
                       const wxPoint& pos,
                       const wxSize& size, long style,
                       const wxString& name, int* attr)
  : wxGLCanvas (parent, id, pos, size, style | wxWANTS_CHARS, name, attr),
  g2d (g), mouseState (0), wheelPosition (0), lastKeyCode (-1)
{
  int w, h;
  GetClientSize(&w, &h);
  if(w < 0 || h < 0)
  {
    w = 0;
    h = 0;
  }

  // Canvas on creation not necessarily visible
  bool visible = IsShown();
  wxWindow* p = parent;
  while (visible && p)
  {
    visible &= p->IsShown();
    p = p->GetParent();
  }
  if (visible) SetCurrent();
  g2d->ResizeNotify (w, h);
}


csGLCanvas::~csGLCanvas()
{
  // The canvas may have been deleted by wxWidgets due to a parent wxWindow getting
  // deleted too. Hence notify the parent csGraphics2DWX that the canvas is no longer valid.
  g2d->theCanvas = nullptr;
}

void csGLCanvas::OnPaint(wxPaintEvent& WXUNUSED(event))
{
  wxPaintDC dc(this);
}

void csGLCanvas::OnSize(wxSizeEvent& event)
{
  // this is also necessary to update the context on some platforms
  wxGLCanvas::OnSize(event);

  int w, h;
  GetClientSize(&w, &h);
  if(w < 0 || h < 0)
  {
    w = 0;
    h = 0;
  }

  SetCurrent();
  g2d->Resize(w, h);
}

void csGLCanvas::OnEraseBackground(wxEraseEvent& WXUNUSED(event))
{
  // Do nothing, to avoid flashing.
}

void csGLCanvas::OnMouseEvent(wxMouseEvent& event)
{
  if(event.GetEventType() == wxEVT_MOTION)
  {
    g2d->EventOutlet->Mouse(csmbNone, false, event.GetX(), event.GetY());
  }
  else if(event.GetEventType() == wxEVT_LEFT_DOWN)
  {
    // Start capturing the mouse
    if (!mouseState) CaptureMouse ();
    mouseState |= MOUSE_LEFT;

    SetFocus ();
    g2d->EventOutlet->Mouse(csmbLeft, true, event.GetX(), event.GetY());
  }
  else if(event.GetEventType() == wxEVT_LEFT_UP)
  {
    // Release the mouse capturing
    mouseState &= ~MOUSE_LEFT;
    if (!mouseState) ReleaseMouse ();

    g2d->EventOutlet->Mouse(csmbLeft, false, event.GetX(), event.GetY());
  }
  else if(event.GetEventType() == wxEVT_MIDDLE_DOWN)
  {
    // Start capturing the mouse
    if (!mouseState) CaptureMouse ();
    mouseState |= MOUSE_MIDDLE;

    SetFocus ();
    g2d->EventOutlet->Mouse(csmbMiddle, true, event.GetX(), event.GetY());
  }
  else if(event.GetEventType() == wxEVT_MIDDLE_UP)
  {
    // Release the mouse capturing
    mouseState &= ~MOUSE_MIDDLE;
    if (!mouseState) ReleaseMouse ();

    g2d->EventOutlet->Mouse(csmbMiddle, false, event.GetX(), event.GetY());
  }
  else if(event.GetEventType() == wxEVT_RIGHT_DOWN)
  {
    // Start capturing the mouse
    if (!mouseState) CaptureMouse ();
    mouseState |= MOUSE_RIGHT;

    SetFocus ();
    g2d->EventOutlet->Mouse(csmbRight, true, event.GetX(), event.GetY());
  }
  else if(event.GetEventType() == wxEVT_RIGHT_UP)
  {
    // Release the mouse capturing
    mouseState &= ~MOUSE_RIGHT;
    if (!mouseState) ReleaseMouse ();

    g2d->EventOutlet->Mouse(csmbRight, false, event.GetX(), event.GetY());
  }
  else if(event.GetEventType() == wxEVT_MOUSEWHEEL)
  {
    // Accumulate the rotation of the wheel
    wheelPosition += event.GetWheelRotation();

    // Generate one event per wheel delta
    while (wheelPosition > event.GetWheelDelta() / 2)
    {
      g2d->EventOutlet->Mouse(csmbWheelUp, true, event.GetX(), event.GetY());
      wheelPosition -= event.GetWheelDelta();
    }

    while (wheelPosition < -event.GetWheelDelta() / 2)
    {
      g2d->EventOutlet->Mouse(csmbWheelDown, true, event.GetX(), event.GetY());
      wheelPosition += event.GetWheelDelta();
    }
  }
}

void csGLCanvas::OnMouseCaptureLost(wxMouseCaptureLostEvent& event)
{
  wxMouseState wxmouseState = wxGetMouseState();
  wxPoint position = ScreenToClient(wxPoint (wxmouseState.GetX(), wxmouseState.GetY()));

  if (mouseState & MOUSE_LEFT)
    g2d->EventOutlet->Mouse(csmbLeft, false, position.x, position.y);
  if (mouseState & MOUSE_MIDDLE)
    g2d->EventOutlet->Mouse(csmbMiddle, false, position.x, position.y);
  if (mouseState & MOUSE_RIGHT)
    g2d->EventOutlet->Mouse(csmbRight, false, position.x, position.y);

  mouseState = 0;
}

void csGLCanvas::OnSetFocus(wxFocusEvent& WXUNUSED(event))
{
  csRef<iEventNameRegistry> enr = csQueryRegistry<iEventNameRegistry> (g2d->object_reg);
  g2d->EventOutlet->Broadcast(csevFocusGained(enr));
}

void csGLCanvas::OnKillFocus(wxFocusEvent& WXUNUSED(event))
{
  csRef<iEventNameRegistry> enr = csQueryRegistry<iEventNameRegistry> (g2d->object_reg);
  g2d->EventOutlet->Broadcast(csevFocusLost(enr));
}

static bool wxCodeToCSCode(int wxkey, utf32_char& raw, utf32_char& cooked)
{
#define MAP(wx, r, c) \
    case WXK_ ## wx: { raw = CSKEY_ ## r; cooked = CSKEY_ ## c; return true; }
#define MAPC(wx, r, c) \
    case WXK_ ## wx: { raw = CSKEY_ ## r; cooked = c; return true; }
  switch(wxkey)
  {
    MAP (BACK,            BACKSPACE,    BACKSPACE)
    MAP (TAB,             TAB,          TAB)
    MAP (RETURN,          ENTER,        ENTER)
    MAP (ESCAPE,          ESC,          ESC)
    MAP (SPACE,           SPACE,        SPACE)
    MAP (DELETE,          DEL,          DEL)
    MAP (CLEAR,           DEL,          DEL)
    MAP (SHIFT,           SHIFT,        SHIFT)
    MAP (ALT,             ALT,          ALT)
    MAP (CONTROL,         CTRL,         CTRL)
    MAP (MENU,            CONTEXT,      CONTEXT)
    MAP (PAUSE,           PAUSE,        PAUSE)
    MAP (CAPITAL,         CAPSLOCK,     CAPSLOCK)
    MAP (PAGEUP,          PGUP,         PGUP)
    MAP (PAGEDOWN,        PGDN,         PGDN)
    MAP (END,             END,          END)
    MAP (HOME,            HOME,         HOME)
    MAP (LEFT,            LEFT,         LEFT)
    MAP (UP,              UP,           UP)
    MAP (RIGHT,           RIGHT,        RIGHT)
    MAP (DOWN,            DOWN,         DOWN)
    MAP (PRINT,           PRINTSCREEN,  PRINTSCREEN)
    MAP (INSERT,          INS,          INS)
    MAPC (NUMPAD0,        PAD0,         '0')
    MAP (NUMPAD_INSERT,   PAD0,         INS)
    MAPC (NUMPAD1,        PAD1,         '1')
    MAP (NUMPAD_END,      PAD1,         END)
    MAPC (NUMPAD2,        PAD2,         '2')
    MAP (NUMPAD_DOWN,     PAD2,         DOWN)
    MAPC (NUMPAD3,        PAD3,         '3')
    MAP (NUMPAD_PAGEDOWN, PAD3,         PGDN)
    MAPC (NUMPAD4,        PAD4,         '4')
    MAP (NUMPAD_LEFT,     PAD4,         LEFT)
    MAPC (NUMPAD5,        PAD5,         '5')
    MAPC (NUMPAD6,        PAD6,         '6')
    MAP (NUMPAD_RIGHT,    PAD6,         RIGHT)
    MAPC (NUMPAD7,        PAD7,         '7')
    MAP (NUMPAD_HOME,     PAD7,         HOME)
    MAPC (NUMPAD8,        PAD8,         '8')
    MAP (NUMPAD_UP,       PAD8,         UP)
    MAPC (NUMPAD9,        PAD9,         '9')
    MAP (NUMPAD_PAGEUP,   PAD9,         PGUP)
    MAPC (MULTIPLY,       PADMULT,      '*')
    MAPC (NUMPAD_MULTIPLY,PADMULT,      '*')
    MAPC (ADD,            PADPLUS,      '+')
    MAPC (SUBTRACT,       PADMINUS,     '-')
    MAPC (NUMPAD_SUBTRACT,PADMINUS,     '-')
    MAPC (NUMPAD_ADD,     PADPLUS,      '+')
    MAPC (DECIMAL,        PADDECIMAL,   '.')
    MAPC (NUMPAD_DECIMAL, PADDECIMAL,   '.')
    MAP (NUMPAD_DELETE,   PADDECIMAL,   DEL)
    MAPC (DIVIDE,         PADDIV,       '/')
    MAPC (NUMPAD_DIVIDE,  PADDIV,       '/')
    MAP (F1,              F1,           F1)
    MAP (F2,              F2,           F2)
    MAP (F3,              F3,           F3)
    MAP (F4,              F4,           F4)
    MAP (F5,              F5,           F5)
    MAP (F6,              F6,           F6)
    MAP (F7,              F7,           F7)
    MAP (F8,              F8,           F8)
    MAP (F9,              F9,           F9)
    MAP (F10,             F10,          F10)
    MAP (F11,             F11,          F11)
    MAP (F12,             F12,          F12)
    MAP (NUMLOCK,         PADNUM,       PADNUM)
    MAP (SCROLL,          SCROLLLOCK,   SCROLLLOCK)
    MAP (NUMPAD_ENTER,    PADENTER,     ENTER)
    default: return false;
  }
#undef MAP
#undef MAPC
}

void csGLCanvas::EmitKeyEvent(wxKeyEvent& event, bool down)
{
  utf32_char cskey_raw = 0, cskey_cooked = 0, cskey_cooked_new = 0;
  wxCodeToCSCode (event.GetKeyCode(), cskey_raw, cskey_cooked);

  // Argh! Seems there is no way to get the character code for non-ASCII keys
  // in non-Unicode builds... not even a character in the local charset...
  if (event.GetKeyCode() <= 127)
  {
    cskey_cooked_new = event.GetKeyCode();
  }
#if wxUSE_UNICODE
  else
  {
    cskey_cooked_new = event.GetUnicodeKey();
  }
#endif

  if (down)
  {
    if (cskey_raw == 0)
      cskey_raw = csUnicodeTransform::MapToLower (cskey_cooked_new);
    if (cskey_cooked == 0) cskey_cooked = cskey_cooked_new;
    if ((cskey_raw != 0) && (lastKeyCode >= 0))
    {
      // Store away the key codes so we restore them in the KEY_UP event
      keyCodeToCS.PutUnique (lastKeyCode, KeyEventCodes (cskey_raw, cskey_cooked));
      lastKeyCode = -1;
    }
  }
  else
  {
    const KeyEventCodes* codes = keyCodeToCS.GetElementPointer (event.GetKeyCode());
    if (codes)
    {
      cskey_raw = codes->raw;
      cskey_cooked = codes->cooked;
    }
    keyCodeToCS.DeleteAll (event.GetKeyCode());
  }
  if (cskey_raw != 0)
  {
    g2d->EventOutlet->Key (cskey_raw, cskey_cooked, down);
  }
}

void csGLCanvas::OnKeyDown( wxKeyEvent& event )
{
  if ((event.GetKeyCode() >= 32) && (event.GetKeyCode() < WXK_START))
  {
    // Non-special characters: defer to EVT_CHAR handling to produce event
    lastKeyCode = event.GetKeyCode();
    event.Skip ();
    return;
  }

  EmitKeyEvent(event, true);
}

void csGLCanvas::OnKeyUp( wxKeyEvent& event )
{
  EmitKeyEvent(event, false);
}

void csGLCanvas::OnKeyChar( wxKeyEvent& event )
{
  EmitKeyEvent(event, true);
}
