/*
    Copyright (C) 2006 by Jorrit Tyberghein

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
#include "csgeom/math3d.h"
#include "csutil/array.h"
#include "cstool/procmesh.h"
#include <math.h>

#include "iutil/objreg.h"
#include "ivideo/graph3d.h"
#include "ivideo/graph2d.h"
#include "ivideo/texture.h"
#include "iengine/engine.h"
#include "iengine/movable.h"
#include "iengine/sector.h"
#include "iengine/mesh.h"
#include "iengine/camera.h"
#include "iengine/rendermanager.h"

csMeshOnTexture::csMeshOnTexture (iObjectRegistry* object_reg)
{
  engine = csQueryRegistry<iEngine> (object_reg);
  g3d = csQueryRegistry<iGraphics3D> (object_reg);
  view.AttachNew (new csView (engine, g3d));
  view->SetAutoResize (false);
  view->GetMeshFilter().SetFilterMode(CS::Utility::MESH_FILTER_INCLUDE);
  cur_w = cur_h = -1;
}

csMeshOnTexture::~csMeshOnTexture ()
{
}

void csMeshOnTexture::ScaleCamera (iMeshWrapper* mesh, int txtw, int txth)
{
  UpdateView (txtw, txth);
  const csBox3 mesh_box = mesh->GetWorldBoundingBox ();
  const csVector3 mesh_center = mesh_box.GetCenter ();
  const CS::Math::Matrix4& invProjection = view->GetCamera ()->GetInvProjectionMatrix ();

  float maxz = 0.f;

#if 0
  // This code works supposedly for all types of projections but is much
  // slower
  for (int i = 0 ; i < 8 ; i++)
  {
    csVector3 corner = mesh_box.GetCorner (i) - mesh_center;

    csVector4 cornerProjected = invProjection * csVector4 (1.f, 0.f, 0.f, 1.f);
    float z = corner.x * cornerProjected.z / cornerProjected.x;
    z -= corner.z;
    if (z > maxz) maxz = z;

    cornerProjected = invProjection * csVector4 (-1.f, 0.f, 0.f, 1.f);
    z = corner.x * cornerProjected.z / cornerProjected.x;
    z -= corner.z;
    if (z > maxz) maxz = z;

    cornerProjected = invProjection * csVector4 (0.f, 1.f, 0.f, 1.f);
    z = corner.y * cornerProjected.z / cornerProjected.y;
    z -= corner.z;
    if (z > maxz) maxz = z;

    cornerProjected = invProjection * csVector4 (0.f, -1.f, 0.f, 1.f);
    z = corner.y * cornerProjected.z / cornerProjected.y;
    z -= corner.z;
    if (z > maxz) maxz = z;
  }
#endif

  // This code works well for regular camera projections and is much faster
  const csVector3 mesh_size = mesh_box.GetSize () * 0.5f;

  csVector4 point = invProjection * csVector4 (1.f, 0.f, 0.f, 1.f);
  float z = mesh_size.x * point.z / point.x;
  if (z > maxz) maxz = z;

  point = invProjection * csVector4 (-1.f, 0.f, 0.f, 1.f);
  z = mesh_size.x * point.z / point.x;
  if (z > maxz) maxz = z;

  point = invProjection * csVector4 (0.f, 1.f, 0.f, 1.f);
  z = mesh_size.y * point.z / point.y;
  if (z > maxz) maxz = z;

  point = invProjection * csVector4 (0.f, -1.f, 0.f, 1.f);
  z = mesh_size.y * point.z / point.y;
  if (z > maxz) maxz = z;

  maxz += mesh_size.z;

  csVector3 cam_pos = mesh_center;
  cam_pos.z -= maxz;

  view->GetCamera()->GetTransform ().Identity ();
  view->GetCamera()->GetTransform ().SetOrigin (cam_pos);
}

void csMeshOnTexture::ScaleCamera (iMeshWrapper* mesh, float distance)
{
  iMovable* movable = mesh->GetMovable ();
  csVector3 mesh_pos = movable->GetFullPosition ();
  iCamera* camera = view->GetCamera ();
  const csVector3& cam_pos = camera->GetTransform ().GetOrigin ();

  camera->GetTransform ().LookAt (mesh_pos-cam_pos, csVector3 (0, 1, 0));

  csVector3 new_cam_pos = mesh_pos + distance * (cam_pos-mesh_pos).Unit ();
  camera->GetTransform ().SetOrigin (new_cam_pos);
}

void csMeshOnTexture::UpdateView (int w, int h)
{
  if (cur_w != w || cur_h != h)
  {
    view->SetAutoResize(false);
    view->SetWidth(w);
    view->SetHeight(h);
    view->SetRectangle (0, 0, w, h, false);
    view->GetPerspectiveCamera ()->SetAspectRatio ((float) w / (float) h);
    cur_w = w;
    cur_h = h;
  }
}

void csMeshOnTexture::PrepareRender (iTextureHandle* handle)
{
  int w, h;
  handle->GetRendererDimensions (w, h);
  UpdateView (w, h);
  view->GetMeshFilter().Clear();
}

bool csMeshOnTexture::DoRender (iTextureHandle* handle,
    bool persistent, int color)
{
  csRef<iRenderManagerTargets> rmTargets = scfQueryInterface<iRenderManagerTargets>(engine->GetRenderManager());
  rmTargets->RegisterRenderTarget(handle, 
                                  view, 
                                  0, 
                                  iRenderManagerTargets::updateOnce 
                                  | ((persistent || color != -1) ? 0 : iRenderManagerTargets::clearScreen));

  ///@TODO Color doesn't work at the moment.
  /// The next line should probably move to SimpleTreeRenderer::RenderContextStack() somewhere.
  //if ((!persistent) && color != -1) g3d->GetDriver2D()->Clear (color);

  rmTargets->MarkAsUsed(handle);

  return true;
}

bool csMeshOnTexture::Render (iMeshWrapper* mesh, iTextureHandle* handle,
    bool persistent, int color)
{
  PrepareRender (handle);

  view->GetMeshFilter().AddFilterMesh(mesh, true);
  view->GetCamera()->SetSector(mesh->GetMovable()->GetSectors()->Get(0));

  return DoRender (handle, persistent, color);
}

