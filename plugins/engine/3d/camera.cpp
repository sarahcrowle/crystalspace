/*
    Copyright (C) 1998-2001 by Jorrit Tyberghein
    Camera code written by Ivan Avramovic <ivan@avramovic.com>

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
#include "csqint.h"
#include "csqsqrt.h"
#include "csgeom/projections.h"
#include "plugins/engine/3d/camera.h"
#include "plugins/engine/3d/sector.h"
#include "plugins/engine/3d/engine.h"

CS_PLUGIN_NAMESPACE_BEGIN(Engine)
{

long csCameraBase:: cur_cameranr = 0;

csCameraBase::csCameraBase () : scfImplementationType (this), fp (0)
{
  mirror = false;
  sector = 0;
  BumpCamera();
  only_portals = true;
}

csCameraBase::csCameraBase (const csCameraBase* other)
  : scfImplementationType (this)
{
  *this = *other;
  if (fp)
  {
    // Make a copy of the plane.
    fp = new csPlane3 (*fp);
  }
  // Listeners are not supposed to be cloned
  listeners.DeleteAll();
  BumpCamera();
}

csCameraBase::~csCameraBase ()
{
  delete fp;
}

void csCameraBase::FireCameraSectorListeners (iSector* sector)
{
  size_t i;
  for (i = 0 ; i < listeners.GetSize () ; i++)
    listeners[i]->NewSector ((iCamera*)this, sector);
}

void csCameraBase::FireCameraMovedListeners ()
{
  size_t i;
  for (i = 0 ; i < listeners.GetSize () ; i++)
    listeners[i]->CameraMoved ((iCamera*)this);
}

void csCameraBase::SetFarPlane (csPlane3 *farplane)
{
  delete fp;
  if (farplane)
    fp = new csPlane3 (*farplane);
  else
    fp = 0;
}

void csCameraBase::MoveWorld (const csVector3 &v, bool cd)
{
  csVector3 new_position = GetOrigin () + v;

  if (sector)
  {
    // Test if the motion crosses a portal
    iSector *new_sector = sector->FollowSegment (
        *this,
        new_position,
        mirror,
        only_portals);

    if (new_sector != sector)
    {
      sector = new_sector;
      cameranr = cur_cameranr++;
      FireCameraSectorListeners (sector);
    }
  }

  FireCameraMovedListeners ();

  SetOrigin (new_position);
  cameranr = cur_cameranr++;
}

void csCameraBase::Correct (int n)
{
  if (n == 0) return;

  csVector3 w1, w2, w3;
  float *vals[5];

  w3 = m_t2o.Col3 ();
  vals[0] = &w3.x;
  vals[1] = &w3.y;
  vals[2] = &w3.z;
  vals[4] = 0;
  Correct (n, vals);  /* perform the snap-to operation on the forward vector */

  /* Maybe w3 should be normalized.  Only necessary if there is
   significant roundoff error: */

  //  w3 = csVector3::unit(w3);

  /* perhaps a snap-to should be performed on one of the other vectors as well */
  w1 = m_t2o.Col2 ();
  w2 = csVector3::Unit (w3 % w1);
  w1 = w2 % w3;

  SetT2O (csMatrix3 (w1.x, w2.x, w3.x, w1.y, w2.y, w3.y, w1.z, w2.z, w3.z));
}

void csCameraBase::Correct (int n, float *vals[])
{
  if (vals == 0) return;
  if (vals[0] == 0) return;
  if (vals[1] == 0) return;
  
  float r;
  if (vals[2] != 0)
  {
    if (*vals[0] < *vals[1])
    {
      r = *vals[2];
      *vals[2] = *vals[0];
      *vals[0] = r;
    }
    else
    {
      r = *vals[2];
      *vals[2] = *vals[1];
      *vals[1] = r;
    }
  }

  float angle;
  angle = (float)atan2 (*vals[1], *vals[0]);
  angle = (TWO_PI / n) * csQround (n * angle / TWO_PI);
  *vals[1] = csQsqrt ((*vals[0]) * (*vals[0]) + (*vals[1]) * (*vals[1]));
  Correct (n, vals + 1);
  r = *vals[1];
  *vals[0] = r * (float)cos (angle);
  *vals[1] = r * (float)sin (angle);
  cameranr = cur_cameranr++;
}

//---------------------------------------------------------------------------

float PerspectiveImpl:: default_aspect_ratio = 1.f;
float PerspectiveImpl:: default_inv_aspect_ratio = 1.f;
float PerspectiveImpl:: default_fov_ratio = 1.f;
float PerspectiveImpl:: default_inv_fov_ratio = 1.f;

PerspectiveImpl::PerspectiveImpl (csEngine* engine)
  : nearClip (engine->csEngine::GetDefaultNearClipDistance()),
    matrixDirty (true), invMatrixDirty (true)
{
  aspect_ratio = default_aspect_ratio;
  inv_aspect_ratio = default_inv_aspect_ratio;
  fov_ratio = default_fov_ratio;
  inv_fov_ratio = default_inv_fov_ratio;
  shift_x = 0.5f;
  shift_y = 0.5f;
}

void PerspectiveImpl::SetDefaultFOV (float fov, float aspect)
{
  // Make sure we have valid angles
  if (fov < SMALL_EPSILON)
  {
    fov = SMALL_EPSILON;
  }

  default_fov_ratio = fov;
  default_inv_fov_ratio = 1.0f / default_fov_ratio;
  default_aspect_ratio = aspect;
  default_inv_aspect_ratio = 1.0f / aspect;
}

void PerspectiveImpl::SetDefaultFOVAngle (float fov, float aspect)
{
  // Make sure we have valid angles
  if (fov >= 180.f)
  {
    fov = 180.f - SMALL_EPSILON;
  }
  else if (fov < SMALL_EPSILON)
  {
    fov = SMALL_EPSILON;
  }

  // The FOV ratio is the tangent of the half fov angle (in radiant).
  default_fov_ratio = tan ((fov * 0.5f * PI) / 180.f);

  // Set the other necessary variables
  default_inv_fov_ratio = 1.0f / default_fov_ratio;
  default_aspect_ratio = aspect;
  default_inv_aspect_ratio = 1.0f / aspect;
}

void PerspectiveImpl::SetFOV (float a, float width)
{
  inv_fov_ratio = 1.0f;
  fov_ratio = 1.0f;
  aspect_ratio = 1.0f / a;
  inv_aspect_ratio = a;
}

void PerspectiveImpl::SetFOVAngle (float a, float width)
{
  // make sure we have valid angles
  if (a >= 180)
  {
    a = 180 - SMALL_EPSILON;
  }
  else if (a <= 0)
  {
    a = SMALL_EPSILON;
  }

  // We calculate the new aspect relative to the reference height of 0.5
  inv_fov_ratio = (0.5f * width) / tan ((a * 0.5f) / 180.f * PI);

  // Set the other necessary variables
  fov_ratio = 1.0f / fov_ratio;
  aspect_ratio = width;
  inv_aspect_ratio = 1.0f / aspect_ratio;

  Dirtify();
}

void PerspectiveImpl::SetVerticalFOV (float fov)
{
  // Make sure we have valid angles
  if (fov < SMALL_EPSILON)
  {
    fov = SMALL_EPSILON;
  }

  fov_ratio = fov;
  inv_fov_ratio = 1.0f / fov_ratio;

  Dirtify();
}

void PerspectiveImpl::SetVerticalFOVAngle (float fov)
{
  // Make sure we have valid angles
  if (fov >= 180.f)
  {
    fov = 180.f - SMALL_EPSILON;
  }
  else if (fov < SMALL_EPSILON)
  {
    fov = SMALL_EPSILON;
  }

  // The FOV ratio is the tangent of the half FOV angle (in radiant).
  fov_ratio = tan ((fov * 0.5f * PI) / 180.f);
  inv_fov_ratio = 1.0f / fov_ratio;

  Dirtify ();
}

float PerspectiveImpl::GetFOVAngle () const
{
  return atan (fov_ratio) * 360.f / PI;
}

float PerspectiveImpl::GetVerticalFOVAngle () const
{
  return atan (fov_ratio) * 360.f / PI;
}

void PerspectiveImpl::SetAspectRatio (float aspect)
{
  aspect_ratio = aspect;
  inv_aspect_ratio = 1.0f / aspect;

  Dirtify ();
}

void PerspectiveImpl::UpdateMatrix ()
{
  if (!matrixDirty) return;

  matrix = CS::Math::Projections::CSPerspective
    (aspect_ratio, 1.0f, shift_x * aspect_ratio, shift_y, fov_ratio, nearClip);

  matrixDirty = false;
  invMatrixDirty = true;
}

void PerspectiveImpl::UpdateInvMatrix ()
{
  if (!invMatrixDirty) return;
  
  invMatrix = matrix.GetInverse();
  
  invMatrixDirty = false;
}

csVector2 PerspectiveImpl::Perspective (const csVector3& v) const
{
  csVector2 p;
  float iz = inv_fov_ratio / v.z;
  p.x = v.x * inv_aspect_ratio * iz + shift_x;
  p.y = v.y * iz + shift_y;
  return p;
}

csVector3 PerspectiveImpl::InvPerspective (const csVector2& p, float z) const
{
  csVector3 v;
  v.z = z;
  v.x = (p.x - shift_x) * z * fov_ratio * aspect_ratio;
  v.y = (p.y - shift_y) * z * fov_ratio;
  return v;
}

csVector2 PerspectiveImpl::Project (const csVector3& v) const
{
  csVector2 p;
  float iz = inv_fov_ratio / v.z;
  p.x = (v.x * inv_aspect_ratio * iz + shift_x) * 2.f - 1.f;
  p.y = (v.y * iz + shift_y) * 2.f - 1.f;
  return p;
}

csVector3 PerspectiveImpl::InvProject (const csVector2& p, float z) const
{
  csVector3 v;
  v.z = z;
  v.x = (p.x * 0.5f + 0.5f - shift_x) * z * fov_ratio * aspect_ratio;
  v.y = (p.y * 0.5f + 0.5f - shift_y) * z * fov_ratio;
  return v;
}  

//---------------------------------------------------------------------------

void csCameraPerspective::UpdateClipPlanes()
{
  if (!clipPlanesDirty) return;
  
  float lx, rx, ty, by;
  lx = -shift_x * inv_fov_ratio * aspect_ratio;
  rx = (1.0f - shift_x) * inv_fov_ratio * aspect_ratio;
  ty = -shift_y * inv_fov_ratio;
  by = (1.0f - shift_y) * inv_fov_ratio;
  
  csPlane3* frust = clipPlanes;
  csVector3 v1 (lx, ty, 1);
  csVector3 v2 (rx, ty, 1);
  frust[0].Set (v1 % v2, 0);
  frust[0].norm.Normalize ();

  csVector3 v3 (rx, by, 1);
  frust[1].Set (v2 % v3, 0);
  frust[1].norm.Normalize ();
  v2.Set (lx, by, 1);
  frust[2].Set (v3 % v2, 0);
  frust[2].norm.Normalize ();
  frust[3].Set (v2 % v1, 0);
  frust[3].norm.Normalize ();
  
  csPlane3 pz0 (0, 0, 1, 0);	// Inverted!!!.
  clipPlanes[4] = pz0;
  clipPlanesMask = 0x1f;

  if (fp)
  {
    clipPlanes[5] = *fp;
    clipPlanesMask |= 0x20;
  }
  
  clipPlanesDirty = false;
}
//---------------------------------------------------------------------------

csCameraCustomMatrix::csCameraCustomMatrix (csCameraBase* other)
  : scfImplementationType (this, other), clipPlanesDirty (true)
{
  // csCameraBase copy ctor already bumped the cam
  matrix = other->GetProjectionMatrix();
  invMatrix = other->GetInvProjectionMatrix();
}

const csPlane3* csCameraCustomMatrix::GetVisibleVolume (uint32& mask)
{
  if (clipPlanesDirty)
  {
    CS::Math::Matrix4 invMatrix_inv_t = matrix.GetTranspose();
      
    int n = 0;
    csPlane3 p;
    // Back plane
    p.Set (0, 0, 1, 1);
    clipPlanes[n] = invMatrix_inv_t * p;
    clipPlanes[n].Normalize();
    n++;
    // Far plane
    p.Set (0, 0, -1, 1);
    clipPlanes[n] = invMatrix_inv_t * p;
    clipPlanes[n].Normalize();
    n++;
    // Left plane
    p.Set (1, 0, 0, 1);
    clipPlanes[n] = invMatrix_inv_t * p;
    clipPlanes[n].Normalize();
    n++;
    // Right plane
    p.Set (-1, 0, 0, 1);
    clipPlanes[n] = invMatrix_inv_t * p;
    clipPlanes[n].Normalize();
    n++;
    // Bottom plane
    p.Set (0, 1, 0, 1);
    clipPlanes[n] = invMatrix_inv_t * p;
    clipPlanes[n].Normalize();
    n++;
    // Top plane
    p.Set (0, -1, 0, 1);
    clipPlanes[n] = invMatrix_inv_t * p;
    clipPlanes[n].Normalize();
    n++;
    
    clipPlanesMask = (1 << n) - 1;
    
    clipPlanesDirty = false;
  }
    
  mask = clipPlanesMask;
  return clipPlanes;
}

csVector2 csCameraCustomMatrix::Project (const csVector3& v) const
{
  csVector4 v_proj (matrix * csVector4 (v, 1.f));
  float inv_w = 1.f / v_proj.w;
  csVector2 p;
  p.x = v_proj.x * inv_w;
  p.y = v_proj.y * inv_w;
  return p;
}

csVector3 csCameraCustomMatrix::InvProject (const csVector2& p, float z) const
{
  csVector4 v_proj =
    invMatrix * csVector4 (p.x, p.y, 0.f, 1.f);
  float scale = z / v_proj[2];
  csVector3 v (scale * v_proj[0], scale * v_proj[1], z);
  return v;
}

}
CS_PLUGIN_NAMESPACE_END(Engine)

