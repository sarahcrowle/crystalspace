/*
    Copyright (C) 2012 by Dominik Seifert

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

#ifndef __CS_CONVEXDECOMPOSE_H__
#define __CS_CONVEXDECOMPOSE_H__

#include "csutil/scf.h"
#include "csutil/scf_implementation.h"
#include "iutil/comp.h"
#include "iutil/objreg.h"

#include "ivaria/convexdecompose.h"

struct iTriangleMesh;

CS_PLUGIN_NAMESPACE_BEGIN (ConvexDecompose)
{

using namespace CS::Collisions;

class ConvexDecomposer :
public scfImplementation2<ConvexDecomposer, iComponent, iConvexDecomposer>
{
private:
  iObjectRegistry* object_reg;

public:
  ConvexDecomposer (iBase* parent);
  virtual ~ConvexDecomposer ();

  bool Initialize (iObjectRegistry* object_reg);

  virtual void DecomposeMesh (iTriangleMesh* triMesh, csRefArray<iTriangleMesh>& result) const;
};

}
CS_PLUGIN_NAMESPACE_END (ConvexDecompose)

#endif // __CS_CONVEXDECOMPOSE_H__
