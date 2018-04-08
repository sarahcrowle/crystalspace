/*
  Copyright (C) 2014 Christian Van Brussel, Institute of Information
      and Communication Technologies, Electronics and Applied Mathematics
      at Universite catholique de Louvain, Belgium
      http://www.uclouvain.be/en-icteam.html

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include "cssysdef.h"
#include "csgfx/renderbuffer.h"
#include "cstool/genmeshbuilder.h"

#include "virtualscreentest.h"

VirtualScreenTest::VirtualScreenTest ()
  : DemoApplication ("Virtual Screen test"),
    screenWidth (1024), screenHeight (760), inScreen (false)
{
}

void VirtualScreenTest::Frame ()
{
  // Remap the position of the mouse cursor on the virtual screen then
  // update its display
  RemapMousePosition ();
  DrawVirtualScreen ();

  // Default behavior from DemoApplication
  DemoApplication::Frame ();
}

void VirtualScreenTest::DrawVirtualScreen ()
{
  // Set the virtual screen texture as the rendering target
  // and start rendering 2D operations
  // TODO: This would be cleaner to add the support for CEGUI 0.8.x then
  // use a specific CEGUI::RenderTarget for the new CEGUI:GUIContext objects
  g3d->SetRenderTarget (screenTexture);
  g3d->BeginDraw (CSDRAW_2DGRAPHICS);

  // Draw a background
  float value = 0.298f;
  csColor4 backgroundColor (value, value, value, 1.f);
  int color = g3d->GetDriver2D ()->FindRGB ((int) (backgroundColor[0] * 255.f),
					    (int) (backgroundColor[1] * 255.f),
					    (int) (backgroundColor[2] * 255.f),
					    (int) (backgroundColor[3] * 255.f));
  g3d->GetDriver2D ()->DrawBox (0, 0, screenWidth, screenHeight, color);

  // Render the CEGUI GUI
  cegui->Render ();

  // Finish the drawing and restore the screen as the rendering target
  g3d->FinishDraw ();
  g3d->SetRenderTarget (nullptr);
}

// Compute the barycentric coordinates of the given point on the given triangle
void ComputeBarycentricCoordinates
(const csVector3 &tr1, const csVector3 &tr2, const csVector3 &tr3,
 const csVector3 &point,
 float& a1, float& a2, float& a3)
{
  csVector3 f1 = tr1 - point;
  csVector3 f2 = tr2 - point;
  csVector3 f3 = tr3 - point;

  csVector3 vector;
  vector.Cross (tr1 - tr2, tr1 - tr3);
  float a = 1.f / vector.Norm ();
  vector.Cross (f2, f3);
  a1 = vector.Norm () * a;
  vector.Cross (f3, f1);
  a2 = vector.Norm () * a;
  vector.Cross (f1, f2);
  a3 = vector.Norm () * a;
}

void VirtualScreenTest::RemapMousePosition ()
{
  // Remap the mouse events within the context of the virtual screen
  int mouseX = mouse->GetLastX ();
  int mouseY = mouse->GetLastY ();

  // Compute the beam in object space
  iCamera* camera = view->GetCamera ();
  csOrthoTransform transform = camera->GetTransform ()
    / screenMesh->QuerySceneNode ()->GetMovable ()->GetTransform ();
  csVector2 v2d (mouseX, mouseY);
  csVector3 v3d = view->InvProject (v2d, 1000.0f);
  csVector3 start = transform.GetOrigin ();
  csVector3 end = transform.This2Other (v3d);

  // Get access to the buffers of the screen mesh
  iRenderBuffer* ibuffer = screenGenmesh->FindSubMesh ("screen")->GetIndices ();
  csTriangle* triangles = (csTriangle*) ibuffer->Lock (CS_BUF_LOCK_READ);

  iRenderBuffer* vbuffer = screenGenmesh->GetRenderBuffer (CS_BUFFER_POSITION);
  csVector3* vertices = (csVector3*) vbuffer->Lock (CS_BUF_LOCK_READ);

  iRenderBuffer* uvbuffer = screenGenmesh->GetRenderBuffer (CS_BUFFER_TEXCOORD0);
  csVector2* uvcoordinates = (csVector2*) uvbuffer->Lock (CS_BUF_LOCK_READ);

  // Search for a triangle being hit by the beam
  inScreen = false;
  csVector3 isect;
  for (size_t i = 0; i < ibuffer->GetElementCount () / 3; i++)
  {
    // Test for a hit
    const csTriangle& triangle = triangles[i];
    if (csIntersect3::SegmentTriangleBF
	(csSegment3 (start, end),
	 vertices[triangle.a], vertices[triangle.b], vertices[triangle.c],
	 isect))
    {
      // Compute the UV coordinates of the point hit
      float a1, a2, a3;
      ComputeBarycentricCoordinates
	(vertices[triangle.a], vertices[triangle.b], vertices[triangle.c],
	 isect, a1, a2, a3);
      csVector2 uv = a1 * uvcoordinates[triangle.a]
	+ a2 * uvcoordinates[triangle.b]
	+ a3 * uvcoordinates[triangle.c];

      // Convert the UV coordinates into virtual screen coordinates
      uv.x *= screenWidth;
      uv.y *= screenHeight;

      // Switch the mouse cursor to the one on the virtual screen
      g2d->SetMouseCursor (csmcNone);
      cegui->GetMouseCursorPtr ()->show ();

      // Inject the mouse motion event
      cegui->GetSystemPtr ()->injectMousePosition (uv.x, uv.y);

      // Mark that a hit beam has been found
      inScreen = true;
      break;
    }
  }

  // If no hit was found then restore the original mouse cursor
  if (!inScreen)
  {
    g2d->SetMouseCursor (csmcArrow);
    cegui->GetMouseCursorPtr ()->hide ();

    // Inject the mouse 'leave' event
    cegui->GetSystemPtr ()->injectMouseLeaves ();
  }

  // Release the buffers of the screen mesh
  ibuffer->Release ();
  vbuffer->Release ();
  uvbuffer->Release ();
}

bool VirtualScreenTest::OnMouseDown (iEvent &event)
{
  // TODO: real conversion of the mouse events
  if (!inScreen) return false;
  return CEGUI::System::getSingletonPtr()->injectMouseButtonDown (CEGUI::LeftButton);
}

bool VirtualScreenTest::OnMouseUp (iEvent &event)
{
  // TODO: real conversion of the mouse events
  return CEGUI::System::getSingletonPtr()->injectMouseButtonUp (CEGUI::LeftButton);
}

bool VirtualScreenTest::OnInitialize (int argc, char* argv[])
{
  // Default behavior from DemoApplication
  if (!DemoApplication::OnInitialize (argc, argv))
    return false;

  // Load the CEGUI plugin
  if (!csInitializer::RequestPlugins (GetObjectRegistry (),
	CS_REQUEST_PLUGIN ("crystalspace.cegui.wrapper", iCEGUI),
	CS_REQUEST_END))
  {
    ReportError ("Can't initialize plugins!");
    return false;
  }

  return true;
}

bool VirtualScreenTest::Application ()
{
  // Default behavior from DemoApplication
  if (!DemoApplication::Application ())
    return false;

  // Disable the motion of the camera through the mouse
  cameraManager->SetMouseMoveEnabled (false);

  // Initialize the CEGUI interface
  cegui = csQueryRegistry<iCEGUI> (GetObjectRegistry ());
  if (!cegui) return ReportError("Failed to locate the CEGUI plugin");
  cegui->Initialize ();
  cegui->GetLoggerPtr ()->setLoggingLevel(CEGUI::Informative);

  // Disable the CEGUI auto rendering and mouse capture since we will
  // virtualize them
  cegui->SetAutoRender (false);
  cegui->DisableMouseCapture ();

  // Setup the display size of CEGUI
  cegui->GetSystemPtr ()->getRenderer ()->setDisplaySize
    (CEGUI::Size (screenWidth, screenHeight));

  // Load the ice skin (which uses Falagard skinning system)
  csRef<iVFS> vfs (csQueryRegistry<iVFS> (object_reg));
  vfs->ChDir ("/cegui/");

  cegui->GetSchemeManagerPtr ()->create ("ice.scheme");
  cegui->GetSystemPtr ()->setDefaultMouseCursor ("ice", "MouseArrow");
  cegui->GetFontManagerPtr ()->createFreeTypeFont
    ("DejaVuSans", 10, true, "/fonts/dejavu/DejaVuSans.ttf");

  CEGUI::WindowManager* winMgr = cegui->GetWindowManagerPtr ();

  // Load the layout and set it as root
  vfs->ChDir ("/ceguitest/");
  cegui->GetSystemPtr ()->setGUISheet (winMgr->loadWindowLayout ("ice.layout"));

  // Subscribe to the exit button clicked event
  CEGUI::Window* button = winMgr->getWindow ("Demo7/Window1/Quit");
  button->subscribeEvent (CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber (&VirtualScreenTest::OnExitButtonClicked, this));

  // Create the scene
  if (!CreateRoom ()) return false;
  engine->Prepare ();

  // Run the application
  Run ();

  return true;
}

bool VirtualScreenTest::CreateRoom ()
{
  // Default behavior from DemoApplication
  if (!DemoApplication::CreateRoom ())
    return false;

  // Create the mesh of the virtual screen
  screenMesh = CS::Geometry::GeneralMeshBuilder::CreateFactoryAndMesh
    (engine, room, "screenMesh", "screenMeshFactory");
  iMeshFactoryWrapper* screenMeshFactory = screenMesh->GetFactory ();
  csRef<iGeneralFactoryState> screenMeshState = scfQueryInterface<iGeneralFactoryState>
    (screenMeshFactory->GetMeshObjectFactory ());
  screenGenmesh = scfQueryInterface<iGeneralMeshState> (screenMesh->GetMeshObject ());
  screenMesh->QuerySceneNode ()->GetMovable ()->SetPosition (csVector3 (0.f, 0.f, 10.f));

  float scale1 = 0.01f;
  float width1 = screenWidth * scale1;
  float height1 = screenHeight * scale1;

  float scale2 = 1.25f;
  float width2 = width1 * scale2;
  float height2 = height1 * scale2;

  csColor4 black (0, 0, 0);
  screenMeshState->AddVertex (csVector3 (-width2, -height2, -5), csVector2 (0, 0),
      csVector3 (0), black);
  screenMeshState->AddVertex (csVector3 (-width1, -height1, +8), csVector2 (1, 0),
      csVector3 (0), black);
  screenMeshState->AddVertex (csVector3 (+width1, -height1, +8), csVector2 (1, 0),
      csVector3 (0), black);
  screenMeshState->AddVertex (csVector3 (+width2, -height2, -5), csVector2 (0, 0),
      csVector3 (0), black);
  screenMeshState->AddVertex (csVector3 (-width2, +height2, -5), csVector2 (0, 1),
      csVector3 (0), black);
  screenMeshState->AddVertex (csVector3 (-width1, +height1, +8), csVector2 (1, 1),
      csVector3 (0), black);
  screenMeshState->AddVertex (csVector3 (+width1, +height1, +8), csVector2 (1, 1),
      csVector3 (0), black);
  screenMeshState->AddVertex (csVector3 (+width2, +height2, -5), csVector2 (0, 1),
      csVector3 (0), black);
  screenMeshState->AddVertex (csVector3 (-width1, -height1, +8), csVector2 (0, 1),
      csVector3 (0), black);
  screenMeshState->AddVertex (csVector3 (+width1, -height1, +8), csVector2 (1, 1),
      csVector3 (0), black);
  screenMeshState->AddVertex (csVector3 (-width1, +height1, +8), csVector2 (0, 0),
      csVector3 (0), black);
  screenMeshState->AddVertex (csVector3 (+width1, +height1, +8), csVector2 (1, 0),
      csVector3 (0), black);

  // Create the texture and material of the border of the screen
  if (!loader->LoadTexture ("stone", "/lib/std/stone4.gif"))
    ReportError ("Error loading %s texture!", CS::Quote::Single ("stone4"));
  iMaterialWrapper* borderMaterial = engine->GetMaterialList ()->FindByName ("stone");
  screenMesh->GetMeshObject ()->SetMaterialWrapper (borderMaterial);

  // Create the submesh of the border of the screen
  csRef<csRenderBuffer> indices;
  csTriangle* triangleData;

  indices = csRenderBuffer::CreateIndexRenderBuffer
    (30, CS_BUF_STATIC, CS_BUFCOMP_UNSIGNED_INT, 0, 29);
  triangleData = (csTriangle*) indices->Lock (CS_BUF_LOCK_READ);
  triangleData[0] = csTriangle (0, 1, 2);
  triangleData[1] = csTriangle (0, 2, 3);
  triangleData[2] = csTriangle (1, 0, 4);
  triangleData[3] = csTriangle (1, 4, 5);
  triangleData[4] = csTriangle (3, 2, 6);
  triangleData[5] = csTriangle (3, 6, 7);
  triangleData[6] = csTriangle (0, 3, 7);
  triangleData[7] = csTriangle (0, 7, 4);
  triangleData[8] = csTriangle (7, 6, 5);
  triangleData[9] = csTriangle (7, 5, 4);
  indices->Release ();
  screenMeshState->AddSubMesh (indices, borderMaterial, "border");

  // Create the virtual screen texture and material
  screenTexture = g3d->GetTextureManager()->CreateTexture
    (screenWidth, screenHeight, csimg2D, "rgb8", CS_TEXTURE_2D | CS_TEXTURE_NPOTS);
  csRef<iTextureWrapper> textureWrapper =
    engine->GetTextureList ()->CreateTexture (screenTexture);
  iMaterialWrapper* screenMaterial = engine->CreateMaterial ("screen", textureWrapper);

  // Create the submesh of the screen
  indices = csRenderBuffer::CreateIndexRenderBuffer
    (6, CS_BUF_STATIC, CS_BUFCOMP_UNSIGNED_INT, 0, 5);
  triangleData = (csTriangle*) indices->Lock (CS_BUF_LOCK_READ);
  triangleData[0] = csTriangle (9, 8, 10);
  triangleData[1] = csTriangle (9, 10, 11);
  indices->Release ();
  screenMeshState->AddSubMesh (indices, screenMaterial, "screen");

  screenMeshState->CalculateNormals ();

  // Add a light
  csRef<iLight> light = engine->CreateLight
    (0, csVector3 (0, 0, 0), 40, csColor (1, 1, 1));
  room->GetLights ()->Add (light);

  return true;
}

bool VirtualScreenTest::OnExitButtonClicked (const CEGUI::EventArgs&)
{
  csRef<iEventQueue> q =
    csQueryRegistry<iEventQueue> (GetObjectRegistry ());
  if (q.IsValid ()) q->GetEventOutlet()->Broadcast (csevQuit (GetObjectRegistry ()));
  return true;
}

//---------------------------------------------------------------------------

CS_IMPLEMENT_APPLICATION

int main (int argc, char* argv[])
{
  return VirtualScreenTest ().Main (argc, argv);
}
