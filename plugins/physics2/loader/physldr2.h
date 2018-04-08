/*
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
#ifndef __CS_PHYSLDR2_H__
#define __CS_PHYSLDR2_H__

#include "imap/reader.h"
#include "ivaria/collisions.h"
#include "ivaria/convexdecompose.h"
#include "ivaria/physics.h"
#include "iutil/eventh.h"
#include "iutil/comp.h"
#include "cstool/collisionhelper.h"
#include "csutil/strhash.h"
#include "csutil/scf_implementation.h"

struct iObjectRegistry;
struct iReporter;
struct iSyntaxService;
struct iEngine;

class DummyBase : public scfImplementation0<DummyBase>
{
public:
  DummyBase () : scfImplementationType (this) {}
};

class csPhysicsLoader2 :
  public scfImplementation2<csPhysicsLoader2, iLoaderPlugin, iComponent>
{
public:
  csPhysicsLoader2 (iBase*);
  virtual ~csPhysicsLoader2 ();

  bool Initialize (iObjectRegistry*);

  virtual bool IsThreadSafe() { return true; }

  virtual csPtr<iBase> Parse (iDocumentNode *node,
    iStreamSource*, iLoaderContext* loaderContext, iBase* context);

  virtual bool ParseSystem (iDocumentNode *node, iLoaderContext* loaderContext);
  virtual bool ParseCollisionSector
    (iDocumentNode *node, CS::Collisions::iCollisionSector* collSector,
     iLoaderContext* loaderContext);

private:
  iObjectRegistry* object_reg;
  csRef<iReporter> reporter;
  csRef<iSyntaxService> synldr;
  csRef<iEngine> engine;
  csRef<CS::Collisions::iCollisionSystem> collisionSystem;
  csRef<CS::Physics::iPhysicalSystem> physicalSystem;
  CS::Collisions::CollisionHelper collisionHelper;

  csStringHash xmltokens;
#define CS_TOKEN_ITEM_FILE \
  "plugins/physics2/loader/physldr2.tok"
#include "cstool/tokenlist.h"
#undef CS_TOKEN_ITEM_FILE
};

#endif // __CS_PHYSLDR_H__
