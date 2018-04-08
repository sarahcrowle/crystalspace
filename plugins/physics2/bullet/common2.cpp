/*
    Copyright (C) 2011-2012 Christian Van Brussel, Institute of Information
      and Communication Technologies, Electronics and Applied Mathematics
      at Universite catholique de Louvain, Belgium
      http://www.uclouvain.be/en-icteam.html
    Copyright (C) 2012 by Dominik Seifert
    Copyright (C) 2011 by Liu Lu

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
#include "common2.h"

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

//----------------------- Error reporting ----------------------------

bool csBulletSystem::ReportError (const char* msg, ...)
{
  va_list arg;
  va_start (arg, msg);
  csReportV (object_reg, CS_REPORTER_SEVERITY_ERROR,
	     "crystalspace.physics.bullet",
	     msg, arg);
  va_end (arg);
  return false;
}

void csBulletSystem::ReportWarning (const char* msg, ...)
{
  va_list arg;
  va_start (arg, msg);
  csReportV (object_reg, CS_REPORTER_SEVERITY_WARNING,
	     "crystalspace.physics.bullet",
	     msg, arg);
  va_end (arg);
}

//----------------------- csBulletDebugDraw ----------------------------

csBulletDebugDraw::csBulletDebugDraw (float inverseInternalScale)
  : mode (DBG_DrawWireframe | DBG_DrawConstraints | DBG_DrawConstraintLimits),
  inverseInternalScale (inverseInternalScale)
  {
  }

void csBulletDebugDraw::drawLine (const btVector3& from, const btVector3& to,
				  const btVector3& color)
{
  csBulletDebugLine l;
  l.p1.Set (BulletToCS (from, inverseInternalScale));
  l.p2.Set (BulletToCS (to, inverseInternalScale));
  l.color.Set (color.getX (), color.getY (), color.getZ ());
  lines.Push (l);
}

void csBulletDebugDraw::drawContactPoint (const btVector3 &PointOnB,
					  const btVector3 &normalOnB,
					  btScalar distance, int lifeTime,
					  const btVector3 &color)
{
  btVector3 to = PointOnB+normalOnB*1;//distance;
  const btVector3& from = PointOnB;
  csBulletDebugLine l;
  l.p1.Set (BulletToCS (from, inverseInternalScale));
  l.p2.Set (BulletToCS (to, inverseInternalScale));
  l.color.Set (color.getX (), color.getY (), color.getZ ());
  lines.Push (l);
}

void csBulletDebugDraw::reportErrorWarning (const char *warningString)
{
  printf ("%s", warningString);
}

void csBulletDebugDraw::draw3dText (const btVector3 &location, const char *textString)
{}

void csBulletDebugDraw::SetDebugMode (CS::Physics::DebugMode mode)
{
  this->mode = 0;
  if (mode & CS::Physics::DEBUG_COLLIDERS)
    this->mode |= DBG_DrawWireframe;
  if (mode & CS::Physics::DEBUG_AABB)
    this->mode |= DBG_DrawAabb;
  if (mode & CS::Physics::DEBUG_JOINTS)
    this->mode |= DBG_DrawConstraints | DBG_DrawConstraintLimits;
}

CS::Physics::DebugMode csBulletDebugDraw::GetDebugMode ()
{
  CS::Physics::DebugMode mode = CS::Physics::DEBUG_NOTHING;
  if (this->mode & DBG_DrawWireframe)
    mode = (CS::Physics::DebugMode) (mode | CS::Physics::DEBUG_COLLIDERS);
  if (this->mode & DBG_DrawAabb)
    mode = (CS::Physics::DebugMode) (mode | CS::Physics::DEBUG_AABB);
  if (this->mode & DBG_DrawConstraints)
    mode = (CS::Physics::DebugMode) (mode | CS::Physics::DEBUG_JOINTS);
  return mode;
}

void csBulletDebugDraw::StartProfile ()
{
  this->mode |= DBG_ProfileTimings;
}

void csBulletDebugDraw::StopProfile ()
{
  this->mode &= this->mode & ~DBG_ProfileTimings;
}

void csBulletDebugDraw::setDebugMode (int m)
{
  mode = m;
}

int csBulletDebugDraw::getDebugMode () const
{
  return mode;
}

void csBulletDebugDraw::DebugDraw (iGraphics3D* g3d, iCamera* camera)
{
  iGraphics2D* g2d = g3d->GetDriver2D ();
  csTransform tr_w2c = camera->GetTransform ();
  const CS::Math::Matrix4& projection (camera->GetProjectionMatrix ());

  if (!g3d->BeginDraw (CSDRAW_2DGRAPHICS))
    return;

  for (size_t i = 0 ; i < lines.GetSize () ; i++)
  {
    csBulletDebugLine& l = lines[i];
    int color = g2d->FindRGB (int (l.color.red * 255),
			      int (l.color.green * 255),
			      int (l.color.blue * 255));
    g2d->DrawLineProjected (tr_w2c * l.p1, tr_w2c * l.p2, projection, color);
  }

  lines.Empty ();
}

}
CS_PLUGIN_NAMESPACE_END (Bullet2)
