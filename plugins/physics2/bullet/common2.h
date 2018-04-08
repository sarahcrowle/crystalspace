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
#ifndef __CS_BULLET_COMMON2_H__
#define __CS_BULLET_COMMON2_H__

#include "bulletsystem.h"
#include "bulletsector.h"
#include "csgeom/matrix3.h"
#include "csgeom/transfrm.h"
#include "csgeom/vector3.h"
#include "ivideo/graph2d.h"
#include "ivideo/graph3d.h"
#include "ivaria/view.h"
#include "iengine/camera.h"
#include "csutil/cscolor.h"
#include "btBulletCollisionCommon.h"
#include "motionstates.h"

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

class csBulletCollisionObject;

//----------------------- Bullet-CS matrices and vectors conversion ----------------------------

// TODO: move those conversion methods within csBulletSystem in order to avoid problems
// with the inverse scale being used correctly or not
static inline csReversibleTransform BulletToCS (const btTransform& trans,
						float inverseInternalScale)
{
  const btVector3& trans_o = trans.getOrigin ();
  csVector3 origin (trans_o.getX () * inverseInternalScale,
		    trans_o.getY () * inverseInternalScale,
		    trans_o.getZ () * inverseInternalScale);
  const btMatrix3x3& trans_m = trans.getBasis ();
  const btVector3& row0 = trans_m.getRow (0);
  const btVector3& row1 = trans_m.getRow (1);
  const btVector3& row2 = trans_m.getRow (2);
  csMatrix3 m (row0.getX (), row1.getX (), row2.getX (),
	       row0.getY (), row1.getY (), row2.getY (),
	       row0.getZ (), row1.getZ (), row2.getZ ());
  return csReversibleTransform (m, origin);
}

static inline btTransform CSToBullet (const csReversibleTransform& tr,
				      float internalScale)
{
  const csVector3& origin = tr.GetOrigin ();
  btVector3 trans_o (origin.x * internalScale,
		     origin.y * internalScale,
		     origin.z * internalScale);
  const csMatrix3& m = tr.GetO2T ();
  btMatrix3x3 trans_m (m.m11, m.m21, m.m31,
		       m.m12, m.m22, m.m32,
		       m.m13, m.m23, m.m33);
  return btTransform (trans_m, trans_o);
}

static inline btMatrix3x3 CSToBullet (const csMatrix3& m)
{
  return btMatrix3x3 (m.m11, m.m21, m.m31,
		      m.m12, m.m22, m.m32,
		      m.m13, m.m23, m.m33);
}

static inline csMatrix3 BulletToCS (const btMatrix3x3& m)
{
  const btVector3& row0 = m.getRow (0);
  const btVector3& row1 = m.getRow (1);
  const btVector3& row2 = m.getRow (2);
  return csMatrix3 (row0.getX (), row1.getX (), row2.getX (),
		    row0.getY (), row1.getY (), row2.getY (),
		    row0.getZ (), row1.getZ (), row2.getZ ());
}

static inline csVector3 BulletToCS (const btVector3& v,
				    float inverseInternalScale)
{
  return csVector3 (v.getX () * inverseInternalScale,
		    v.getY () * inverseInternalScale,
		    v.getZ () * inverseInternalScale);
}

static inline btVector3 CSToBullet (const csVector3& v,
				    float internalScale)
{
  return btVector3 (v.x * internalScale,
		    v.y * internalScale,
		    v.z * internalScale);
}

//----------------------- csBulletDebugDraw ----------------------------

struct csBulletDebugLine
{
  csVector3 p1, p2;
  csColor color;
};

class csBulletDebugDraw : public btIDebugDraw
{
private:
  csArray<csBulletDebugLine> lines;
  int mode;
  float inverseInternalScale;

public:
  csBulletDebugDraw (float inverseInternalScale);
  virtual ~csBulletDebugDraw () { }

  virtual void drawLine (const btVector3& from, const btVector3& to,
			 const btVector3& color);
  virtual void drawContactPoint (const btVector3 &PointOnB,
				 const btVector3 &normalOnB,
				 btScalar distance, int lifeTime,
				 const btVector3 &color);
  virtual void reportErrorWarning (const char *warningString);
  virtual void draw3dText (const btVector3 &location, const char *textString);

  virtual void SetDebugMode (CS::Physics::DebugMode mode);
  virtual CS::Physics::DebugMode GetDebugMode ();

  virtual void StartProfile ();
  virtual void StopProfile ();

  virtual void setDebugMode (int m);
  virtual int getDebugMode () const;

  void DebugDraw (iGraphics3D* g3d, iCamera* camera);
};

}
CS_PLUGIN_NAMESPACE_END (Bullet2)

#endif //__CS_BULLET_COMMON2_H__
