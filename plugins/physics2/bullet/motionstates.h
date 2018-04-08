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

#ifndef __CS_BULLET_MOTIONSTATES_H__
#define __CS_BULLET_MOTIONSTATES_H__

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


CS_PLUGIN_NAMESPACE_BEGIN(Bullet2)
{
//------------------------ csBulletMotionState ----------------------

class csBulletMotionState : public btDefaultMotionState
{
public:
  csBulletCollisionObject* body;
  // we save the inverse of the principal axis for performance reasons
  btTransform inversePrincipalAxis;

public:
  csBulletMotionState (csBulletCollisionObject* body,
		       const btTransform& initialTransform,
		       const btTransform& principalAxis);

  virtual void setWorldTransform (const btTransform& trans);
};


//------------------------ csBulletKinematicMotionState ----------------------

class csBulletKinematicMotionState : public csBulletMotionState
{
  csOrthoTransform principalAxis;

public:
  csBulletKinematicMotionState (csBulletCollisionObject* body,
		       const btTransform& initialTransform,
				const btTransform& principalAxis);

  virtual void getWorldTransform (btTransform& trans) const;
};

}
CS_PLUGIN_NAMESPACE_END(Bullet2)

#endif
