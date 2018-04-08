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
#include "cssysdef.h"

#include "csgeom/plane3.h"
#include "csgeom/transfrm.h"
#include "csutil/stringquote.h"
#include "iengine/engine.h"
#include "iengine/mesh.h"
#include "iengine/movable.h"
#include "iengine/sector.h"
#include "imap/ldrctxt.h"
#include "imap/loader.h"
#include "imap/services.h"
#include "iutil/document.h"
#include "iutil/object.h"
#include "iutil/objreg.h"
#include "iutil/plugin.h"
#include "ivaria/reporter.h"
#include "imesh/terrain2.h"
#include "physldr2.h"

using namespace CS::Collisions;
using namespace CS::Physics;

SCF_IMPLEMENT_FACTORY (csPhysicsLoader2)

static const char* msgid = "crystalspace.physics.loader";

csPhysicsLoader2::csPhysicsLoader2 (iBase* pParent) :
  scfImplementationType (this, pParent)
{
}

csPhysicsLoader2::~csPhysicsLoader2 ()
{
}

bool csPhysicsLoader2::Initialize (iObjectRegistry* object_reg)
{
  csPhysicsLoader2::object_reg = object_reg;
  engine = csQueryRegistry<iEngine> (object_reg);
  reporter = csQueryRegistry<iReporter> (object_reg);
  synldr = csQueryRegistry<iSyntaxService> (object_reg);

  InitTokenTable (xmltokens);

  // Search for a collision and a physical system
  collisionSystem = csQueryRegistry<CS::Collisions::iCollisionSystem> (object_reg);
  if (collisionSystem)
  {
    physicalSystem = scfQueryInterface<CS::Physics::iPhysicalSystem> (collisionSystem);
    collisionHelper.Initialize (object_reg, collisionSystem);
  }

  return true;
}

csPtr<iBase> csPhysicsLoader2::Parse (iDocumentNode *node, 
                                      iStreamSource*, 
                                      iLoaderContext* loaderContext, 
                                      iBase* context)
{
  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    const char* value = child->GetValue ();
    csStringID id = xmltokens.Request (value);
    switch (id)
    {
    case XMLTOKEN_COLLISIONSYSTEM:
    {
      if (!ParseSystem (child, loaderContext))
	return csPtr<iBase> (nullptr);
      return csPtr<iBase> (collisionSystem);
    }
    // TODO: parse the groups & collision filtering properties
/*
  case XMLTOKEN_COLLISIONGROUP:
  {
  const char* name = child->GetAttributeValue ("name");
  CS::Collisions::iCollisionGroup* group = collisionSystem->CreateCollisionGroup (name);
  if (!group)
  synldr->Report (msgid, CS_REPORTER_SEVERITY_WARNING,
  child, "Could not create the collision group '%s' "
  "probably because the maximum count of groups has been reached",
  CS::Quote::Single (name));
  break;
  }
*/
    case XMLTOKEN_COLLISIONSECTOR:
    {
      if (!collisionSystem)
      {
	synldr->Report (msgid, CS_REPORTER_SEVERITY_WARNING,
			child, "No collision system while creating a collision sector");

	// Ignore this collision tag and return a dummy valid result
	csRef<iBase> base;
	base.AttachNew (new DummyBase ());
	return csPtr<iBase> (base);
      }

      csRef<iSector> sector = scfQueryInterface<iSector> (context);
      if (!sector) 
      {
	synldr->ReportError (msgid, child, "Could not create the collision sector"
			     " because the parent object is not an engine sector");
	break;
      }

      csRef<CS::Collisions::iCollisionSector> collisionSector =
	collisionSystem->FindCollisionSector (sector);

      const char* name = child->GetAttributeValue ("name");
      if (name && *name)
      {
	if (!collisionSector)
	  collisionSector = collisionSystem->FindCollisionSector (name);
	else
	  collisionSector->QueryObject ()->SetName (name);
      }

      if (!collisionSector)
      {
        collisionSector = collisionSystem->CreateCollisionSector (sector);
	if (name && *name)
	  collisionSector->QueryObject ()->SetName (name);
      }

      if (ParseCollisionSector (child, collisionSector, loaderContext))
        return csPtr<iBase> (collisionSector);
      else
      {
        synldr->ReportBadToken (child);
        return csPtr<iBase> (nullptr);
      }

      break;
    }
    case XMLTOKEN_COLLISIONOBJECT:
    {
      if (!collisionSystem)
      {
	// Ignore this collision tag and return a dummy valid result
	csRef<iBase> base;
	base.AttachNew (new DummyBase ());
	return csPtr<iBase> (base);
      }

      csRef<CS::Collisions::iCollisionObjectFactory> factory =
	collisionHelper.ParseCollisionObjectFactory (child, loaderContext, context);
      // TODO: check context OK
      return csPtr<iBase> (factory);
    }
    case XMLTOKEN_COLLISIONGHOST:
    {
      if (!collisionSystem)
      {
	// Ignore this collision tag and return a dummy valid result
	csRef<iBase> base;
	base.AttachNew (new DummyBase ());
	return csPtr<iBase> (base);
      }

      csRef<CS::Collisions::iCollisionObjectFactory> factory =
	collisionHelper.ParseCollisionObjectFactory (child, loaderContext, context);
      return csPtr<iBase> (factory);
    }
    case XMLTOKEN_RIGIDBODY:
    {
      if (!physicalSystem)
      {
	// Ignore this collision tag and return a dummy valid result
	csRef<iBase> base;
	base.AttachNew (new DummyBase ());
	return csPtr<iBase> (base);
      }

      csRef<CS::Physics::iRigidBodyFactory> factory =
	collisionHelper.ParseRigidBodyFactory (child, loaderContext, context);
      return csPtr<iBase> (factory);
    }
    case XMLTOKEN_SOFTBODY:
    {
      if (!physicalSystem)
      {
	// Ignore this collision tag and return a dummy valid result
	csRef<iBase> base;
	base.AttachNew (new DummyBase ());
	return csPtr<iBase> (base);
      }

      csRef<CS::Physics::iSoftBodyFactory> factory =
	collisionHelper.ParseSoftBodyFactory (child, loaderContext, context);
      return csPtr<iBase> (factory);
    }
    default:
      synldr->ReportBadToken (child);
      return csPtr<iBase> (nullptr);
    }
  }

  return csPtr<iBase> (nullptr);
}

bool csPhysicsLoader2::ParseSystem (iDocumentNode *node, iLoaderContext* loaderContext)
{
  // Load the collision system
  const char* plugin = node->GetAttributeValue ("plugin");
  if (!plugin)
  {
    synldr->ReportError (msgid, node, "No plugin specified for the system!");
    return false;
  }

  collisionSystem = csQueryRegistry<CS::Collisions::iCollisionSystem> (object_reg);
  if (!collisionSystem)
  {
    csRef<iPluginManager> pluginManager = csQueryRegistry<iPluginManager> (object_reg);
    collisionSystem = csLoadPlugin<CS::Collisions::iCollisionSystem> (pluginManager, plugin);
  }

  if (!collisionSystem)
  {
    synldr->ReportError (msgid, node, "Unable to load the collision system plugin %s",
			 CS::Quote::Single (plugin));
    return false;
  }

  physicalSystem = scfQueryInterface<CS::Physics::iPhysicalSystem> (collisionSystem);
  collisionHelper.Initialize (object_reg, collisionSystem);

  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT)
      continue;
    const char* value = child->GetValue ();
    csStringID id = xmltokens.Request (value);
    switch (id)
    {
    case XMLTOKEN_SIMULATIONSPEED:
    {
      float speed = child->GetAttributeValueAsFloat ("speed");
      collisionSystem->SetSimulationSpeed (speed);
      break;
    }
    case XMLTOKEN_STEPPARAMS:
    {
      if (!physicalSystem)
      {
	synldr->ReportError (msgid, node, "Invalid attribute for a pure collision system plugin");
	break;
      }

      float timeStep = child->GetAttributeValueAsFloat ("timestep", 0.0166f);
      int maxStep = child->GetAttributeValueAsInt ("maxstep", 1);
      int iterations = child->GetAttributeValueAsInt ("iterations", 10);
      physicalSystem->SetStepParameters (timeStep, maxStep, iterations);

      break;
    }
    case XMLTOKEN_INTERNALSCALE:
    {
      if (!physicalSystem)
      {
	synldr->ReportError (msgid, node, "Invalid attribute for a pure collision system plugin");
	break;
      }

      physicalSystem->SetInternalScale (child->GetAttributeValueAsFloat ("scale", 1.0f));
      break;
    }
    case XMLTOKEN_SOFTBODYENABLED:
    {
      if (!physicalSystem)
      {
	synldr->ReportError (msgid, node, "Invalid attribute for a pure collision system plugin");
	break;
      }

      physicalSystem->SetSoftBodyEnabled (child->GetAttributeValueAsBool ("enabled", true));
      break;
    }
    case XMLTOKEN_DAMPING:
    {
      if (!physicalSystem)
      {
	synldr->ReportError (msgid, node, "Invalid attribute for a pure collision system plugin");
	break;
      }

      float linear = child->GetAttributeValueAsFloat ("linear", 0.1f);
      float angular = child->GetAttributeValueAsFloat ("angular", 0.1f);
      physicalSystem->SetLinearDamping (linear);
      physicalSystem->SetAngularDamping (angular);

      break;
    }
    default:
      synldr->ReportBadToken (child);
      return false;
    }
  }

  return true;
}

bool csPhysicsLoader2::ParseCollisionSector (iDocumentNode *node, 
                                             CS::Collisions::iCollisionSector* collisionSector,
                                             iLoaderContext* loaderContext)
{
  csRef<CS::Physics::iPhysicalSector> physicalSector = 
    scfQueryInterface<CS::Physics::iPhysicalSector> (collisionSector);

  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT)
      continue;
    const char* value = child->GetValue ();
    csStringID id = xmltokens.Request (value);
    switch (id)
    {
    case XMLTOKEN_GRAVITY:
    {
      csVector3 v;
      if (!synldr->ParseVector (child, v))
      {
	synldr->ReportError (msgid,
			     child, "Error processing gravity token");
	return false;
      }
      collisionSector->SetGravity (v);

      break;
    }
    case XMLTOKEN_DAMPING:
    {
      if (!physicalSector)
      {
	synldr->ReportError (msgid, node, "Invalid attribute for a pure collision system plugin");
	break;
      }

      float linear = child->GetAttributeValueAsFloat ("linear", 0.1f);
      float angular = child->GetAttributeValueAsFloat ("angular", 0.1f);
      physicalSector->SetLinearDamping (linear);
      physicalSector->SetAngularDamping (angular);

      break;
    }
    case XMLTOKEN_AUTODISABLING:
    {
      if (!physicalSector)
      {
	synldr->ReportError (msgid, node, "Invalid attribute for a pure collision system plugin");
	break;
      }

      float linear = child->GetAttributeValueAsFloat ("linear", 0.8f);
      float angular = child->GetAttributeValueAsFloat ("angular", 1.0f);
      float time = child->GetAttributeValueAsFloat ("time", 0.0f);
      physicalSector->SetAutoDisableParams (linear, angular, time);

      break;
    }
    default:
      synldr->ReportBadToken (child);
      return false;
    }
  }

  return true;
}
