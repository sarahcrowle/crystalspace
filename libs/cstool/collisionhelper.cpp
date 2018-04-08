/*
    Copyright (C) 2012 Christian Van Brussel, Institute of Information
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

#include "csutil/stringquote.h"
#include "iengine/engine.h"
#include "iengine/movable.h"
#include "iengine/mesh.h"
#include "iengine/portal.h"
#include "iengine/portalcontainer.h"
#include "iengine/scenenode.h"
#include "iengine/sector.h"
#include "igeom/trimesh.h"
#include "imap/ldrctxt.h"
#include "imap/loader.h"
#include "imap/services.h"
#include "imesh/objmodel.h"
#include "imesh/softanim.h"
#include "imesh/terrain2.h"
#include "iutil/document.h"
#include "iutil/plugin.h"
#include "ivaria/convexdecompose.h"
#include "ivaria/collisions.h"
#include "ivaria/physics.h"
#include "ivaria/reporter.h"

#include "cstool/collisionhelper.h"

using namespace CS::Collisions;

void CollisionHelper::ReportError (const char* msg, ...) const
{
  va_list arg;
  va_start (arg, msg);
  csReportV (objectRegistry, CS_REPORTER_SEVERITY_ERROR,
	     "crystalspace.collisions.helper",
	     msg, arg);
  va_end (arg);
}

void CollisionHelper::ReportWarning (const char* msg, ...) const
{
  va_list arg;
  va_start (arg, msg);
  csReportV (objectRegistry, CS_REPORTER_SEVERITY_WARNING,
	     "crystalspace.collisions.helper",
	     msg, arg);
  va_end (arg);
}

enum
{
  XMLTOKEN_MOVE,
  XMLTOKEN_ROTATE,
  XMLTOKEN_COLLIDER,
  XMLTOKEN_BOX,
  XMLTOKEN_BOUNCE,
  XMLTOKEN_CAPSULE,
  XMLTOKEN_CONCAVEMESH,
  XMLTOKEN_CONE,
  XMLTOKEN_CONVEXMESH,
  XMLTOKEN_CYLINDER,
  XMLTOKEN_PLANE,
  XMLTOKEN_SPHERE,
  XMLTOKEN_TERRAIN,
  XMLTOKEN_COLLISIONOBJECT,
  XMLTOKEN_COLLISIONGHOST,
  XMLTOKEN_COLLISIONACTOR,
  XMLTOKEN_RIGIDBODY,
  XMLTOKEN_SOFTBODY,
  XMLTOKEN_MESHFACT,
  XMLTOKEN_GROUP,
  XMLTOKEN_CONSTRAINTS,
  XMLTOKEN_MIN,
  XMLTOKEN_MAX,
  XMLTOKEN_DISTANCE,
  XMLTOKEN_ANGLE,
  XMLTOKEN_SPRING,
  XMLTOKEN_STIFFNESS,
  XMLTOKEN_LINEARSTIFFNESS,
  XMLTOKEN_ANGULARSTIFFNESS,
  XMLTOKEN_LINEARDAMPING,
  XMLTOKEN_ANGULARDAMPING,
  XMLTOKEN_DESIREDVELOCITY
};

bool CollisionHelper::Initialize
(iObjectRegistry* objectRegistry, CS::Collisions::iCollisionSystem* collisionSystem,
 CS::Collisions::iConvexDecomposer* decomposer)
{
  CS_ASSERT (objectRegistry);

  this->objectRegistry = objectRegistry;
  this->collisionSystem = collisionSystem;
  this->decomposer = decomposer;

  // Create the collision system if none are provided or found
  if (!this->collisionSystem)
    this->collisionSystem = csQueryRegistry<CS::Collisions::iCollisionSystem> (objectRegistry);

  if (!this->collisionSystem)
  {
    csRef<iPluginManager> pluginManager = csQueryRegistry<iPluginManager> (objectRegistry);
    this->collisionSystem = csLoadPlugin<CS::Collisions::iCollisionSystem>
      (pluginManager, "crystalspace.physics.bullet");
  }

  if (!this->collisionSystem)
  {
    ReportError ("Unable to find or load a collision system plugin");
    return false;
  }

  engine = csQueryRegistry<iEngine> (objectRegistry);

  // Initialize the ID's for the collision models of the meshes
  csRef<iStringSet> strings = csQueryRegistryTagInterface<iStringSet>
    (objectRegistry, "crystalspace.shared.stringset");
  baseID = strings->Request ("base");
  collisionID = strings->Request ("colldet");

  // Initialize parsing
  synldr = csQueryRegistry<iSyntaxService> (objectRegistry);

  xmltokens.Register ("move", XMLTOKEN_MOVE);
  xmltokens.Register ("rotate", XMLTOKEN_ROTATE);
  xmltokens.Register ("collider", XMLTOKEN_COLLIDER);
  xmltokens.Register ("concavemesh", XMLTOKEN_CONCAVEMESH);
  xmltokens.Register ("convexmesh", XMLTOKEN_CONVEXMESH);
  xmltokens.Register ("sphere", XMLTOKEN_SPHERE);
  xmltokens.Register ("cylinder", XMLTOKEN_CYLINDER);
  xmltokens.Register ("capsule", XMLTOKEN_CAPSULE);
  xmltokens.Register ("cone", XMLTOKEN_CONE);
  xmltokens.Register ("box", XMLTOKEN_BOX);
  xmltokens.Register ("plane", XMLTOKEN_PLANE);
  xmltokens.Register ("terrain", XMLTOKEN_TERRAIN);
  xmltokens.Register ("collisionobject", XMLTOKEN_COLLISIONOBJECT);
  xmltokens.Register ("collisionghost", XMLTOKEN_COLLISIONGHOST);
  xmltokens.Register ("collisionactor", XMLTOKEN_COLLISIONACTOR);
  xmltokens.Register ("rigidbody", XMLTOKEN_RIGIDBODY);
  xmltokens.Register ("softbody", XMLTOKEN_SOFTBODY);
  xmltokens.Register ("meshfact", XMLTOKEN_MESHFACT);
  xmltokens.Register ("group", XMLTOKEN_GROUP);
  xmltokens.Register ("constraints", XMLTOKEN_CONSTRAINTS);
  xmltokens.Register ("min", XMLTOKEN_MIN);
  xmltokens.Register ("max", XMLTOKEN_MAX);
  xmltokens.Register ("distance", XMLTOKEN_DISTANCE);
  xmltokens.Register ("angle", XMLTOKEN_ANGLE);
  xmltokens.Register ("bounce", XMLTOKEN_BOUNCE);
  xmltokens.Register ("spring", XMLTOKEN_SPRING);
  xmltokens.Register ("stiffness", XMLTOKEN_STIFFNESS);
  xmltokens.Register ("linearstiffness", XMLTOKEN_LINEARSTIFFNESS);
  xmltokens.Register ("angularstiffness", XMLTOKEN_ANGULARSTIFFNESS);
  xmltokens.Register ("lineardamping", XMLTOKEN_LINEARDAMPING);
  xmltokens.Register ("angulardamping", XMLTOKEN_ANGULARDAMPING);
  xmltokens.Register ("desiredvelocity", XMLTOKEN_DESIREDVELOCITY);

  return true;
}

void CollisionHelper::InitializeCollisionObjects
(iEngine* engine, iCollection* collection) const
{
  // Initialize all mesh objects for collision detection.
  int i;
  iMeshList* meshes = engine->GetMeshes ();
  for (i = 0 ; i < meshes->GetCount () ; i++)
  {
    iMeshWrapper* mesh = meshes->Get (i);
    if (collection && !collection->IsParentOf (mesh->QueryObject ())) continue;
    InitializeCollisionObjects (mesh->GetMovable ()->GetSectors ()->Get (0), mesh);
  }
}

void CollisionHelper::InitializeCollisionObjects
(iSector* sector, iCollection* collection) const
{
  // Initialize all mesh objects for collision detection.
  int i;
  iMeshList* meshes = sector->GetMeshes ();
  for (i = 0 ; i < meshes->GetCount () ; i++)
  {
    iMeshWrapper* mesh = meshes->Get (i);
    if (collection && !collection->IsParentOf (mesh->QueryObject ())) continue;
    InitializeCollisionObjects (sector, mesh);
  }
}

void CollisionHelper::InitializeCollisionObjects
(iSector* sector, iMeshWrapper* mesh) const
{
  // Create or find the iCollisionSector associated with the iSector
  iCollisionSector* collisionSector = collisionSystem->CreateCollisionSector (sector);

  // TODO: search first for a collision factory, then fallback on the object model only if not found
  iObjectModel* objectModel = mesh->GetMeshObject ()->GetObjectModel ();

  // Check if we have a terrain mesh
  iTerrainSystem* terrainSys = objectModel->GetTerrainColldet ();
  if (terrainSys)
  {
    // Create and add a collision terrain
    csRef<iTerrainFactory> factory = scfQueryInterface<iTerrainFactory>
      (mesh->GetFactory ()->GetMeshObjectFactory ());
    csRef<iCollisionTerrainFactory> terrainFactory =
      collisionSystem->CreateCollisionTerrainFactory (factory);
    csRef<iCollisionTerrain> terrain = terrainFactory->CreateTerrain (terrainSys);
    collisionSector->AddCollisionObject (terrain);

    terrain->SetAttachedSceneNode (mesh->QuerySceneNode ());
    terrain->QueryObject ()->SetObjectParent (mesh->QueryObject ());
  }

  // Check if we have a portal mesh
  iPortalContainer* portalContainer = mesh->GetPortalContainer ();
  if (portalContainer)
  {
    for (int i = 0; i < portalContainer->GetPortalCount (); i++)
    {
      iPortal* portal = portalContainer->GetPortal (i);

      // Exclude portals with the collision detection flag set
      if (portal->GetFlags ().Check (CS_PORTAL_COLLDET))
      {
	// TODO: create a collision object instead
	continue;
      }

      // Add the collision portal to its sector
      collisionSector->AddPortal (portal, mesh->GetMovable ()->GetFullTransform ());
    }
  }

  // Create the collision object of the mesh
  csRef<iCollisionObject> collisionObject;
  
  // Get mesh factory
  iMeshFactoryWrapper* meshFactory = mesh->GetFactory ();
  if (meshFactory)
  {
    ::iObject* meshFactoryObj = meshFactory->QueryObject ();
    if (meshFactoryObj)
    {
      // Search for a collision object factory for this mesh
      for (csRef<iObjectIterator> it = meshFactoryObj->GetIterator (); it->HasNext (); )
      {
        ::iObject* next = it->Next ();
        csRef<iCollisionObjectFactory> nextFactory = scfQueryInterface<iCollisionObjectFactory> (next);
        if (nextFactory)
        {
          collisionObject = nextFactory->CreateCollisionObject ();
          break;
        }
      }
    }
  }

  if (!terrainSys && !portalContainer && !collisionObject)
  {
    // did not find a specific physical factory and its not a placeholder
    // -> Create a static collision object from the mesh, using default values for physical values (if available)

    csRef<iTriangleMesh> triMesh = FindCollisionMesh (mesh);
    if (triMesh)
    {
      csRef<CS::Collisions::iCollider> collider;
      if (decomposer)
      {
	collider = csRef<CS::Collisions::iCollider> (collisionSystem->CreateCollider ());
	DecomposeConcaveMesh (triMesh, collider, decomposer);
      }
      else
      {
        collider = csRef<CS::Collisions::iColliderConcaveMesh>(collisionSystem->CreateColliderConcaveMesh (triMesh));
      }

      // Colliders cannot be created for meshes that have no triangle data
      if (collider)
      {
        csRef<iCollisionObjectFactory> collisionObjectFact = collisionSystem->CreateCollisionObjectFactory (collider);
        collisionObject = collisionObjectFact->CreateCollisionObject ();
        mesh->QueryObject ()->ObjAdd (collisionObject->QueryObject ());
      }
    }
  }

  if (collisionObject)
  {
    // Attach the scene node of the mesh to the collision object
    collisionObject->SetTransform (mesh->GetMovable ()->GetFullTransform ());
    collisionObject->SetAttachedSceneNode (mesh->QuerySceneNode ());

    // Set the iObject of the mesh as the parent of the one of the collision object.
    // This allows to find the reference from a collision object to its associated mesh,
    // and conversely.
    collisionObject->QueryObject ()->SetObjectParent (mesh->QueryObject ());

#ifdef CS_DEBUG
    // Set the name of the collision object
    collisionObject->QueryObject ()->SetName (mesh->QueryObject ()->GetName ());
#endif

    // Add the object to the sector
    collisionSector->AddCollisionObject (collisionObject);
  }

  // Recurse on the children scene nodes
  const csRef<iSceneNodeArray> ml = mesh->QuerySceneNode ()->GetChildrenArray ();
  size_t i;
  for (i = 0 ; i < ml->GetSize () ; i++)
  {
    iMeshWrapper* child = ml->Get (i)->QueryMesh ();
    
    // @@@ What if we have a light containing another mesh?
    if (child)
    {
      InitializeCollisionObjects (sector, child);
    }
  }
}

iTriangleMesh* CollisionHelper::FindCollisionMesh
(iMeshWrapper* mesh) const
{
  iObjectModel* objectModel = mesh->GetMeshObject ()->GetObjectModel ();
  iTriangleMesh* triMesh;
  if (objectModel->IsTriangleDataSet (collisionID))
    triMesh = objectModel->GetTriangleData (collisionID);
  else
    triMesh = objectModel->GetTriangleData (baseID);

  if (!triMesh || triMesh->GetVertexCount () == 0
      || triMesh->GetTriangleCount () == 0)
    return nullptr;

  return triMesh;
}

iTriangleMesh* CollisionHelper::FindCollisionMesh
(iMeshFactoryWrapper* meshFactory) const
{
  iObjectModel* objectModel = meshFactory->GetMeshObjectFactory ()->GetObjectModel ();
  iTriangleMesh* triMesh;
  if (objectModel->IsTriangleDataSet (collisionID))
    triMesh = objectModel->GetTriangleData (collisionID);
  else
    triMesh = objectModel->GetTriangleData (baseID);

  if (!triMesh || triMesh->GetVertexCount () == 0
      || triMesh->GetTriangleCount () == 0)
    return nullptr;

  return triMesh;
}

void CollisionHelper::DecomposeConcaveMesh
(iTriangleMesh* mesh, CS::Collisions::iCollider* collider,
 CS::Collisions::iConvexDecomposer* decomposer) const
{
  // Decompose the mesh in its convex parts
  csRefArray<iTriangleMesh> convexParts;
  decomposer->DecomposeMesh (mesh, convexParts);

  // Merge the convex parts into the given collider
  for (size_t i = 0; i < convexParts.GetSize (); i++)
  {
    csRef<iColliderConvexMesh> convexCollider =
      collisionSystem->CreateColliderConvexMesh (convexParts[i]);
    collider->AddChild (convexCollider);
  }    
}

void CollisionHelper::DecomposeConcaveMesh
(iMeshWrapper* mesh, CS::Collisions::iCollider* collider,
 CS::Collisions::iConvexDecomposer* decomposer) const
{
  iTriangleMesh* triangleMesh = FindCollisionMesh (mesh);
  if (triangleMesh)
    DecomposeConcaveMesh (triangleMesh, collider, decomposer);
}

//--------------------------------- Parsing ---------------------------------

void CollisionHelper::ParseTransform (iDocumentNode* node, csTransform& transform) const
{
  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    const char *value = child->GetValue ();
    csStringID id = xmltokens.Request (value);
    switch (id)
    {
      case XMLTOKEN_MOVE:
      {
        csVector3 v;
        synldr->ParseVector (child, v);
        transform.SetOrigin (v);
        break;
      }
      case XMLTOKEN_ROTATE:
      {
        csMatrix3 m;
        synldr->ParseMatrix (child, m);
        transform.SetO2T (m);
        break;
      }
    }
  }
}

csPtr<CS::Collisions::iCollider> CollisionHelper::ParseCollider
(iDocumentNode* node, csTransform& transform, iLoaderContext* loaderContext,
 iBase* context) const
{
  transform.Identity ();
  csRef<CS::Collisions::iCollider> collider;

  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;

    ParseTransform (child, transform);

    const char *value = child->GetValue ();
    csStringID id = xmltokens.Request (value);
    switch (id)
    {
    case XMLTOKEN_SPHERE:
    {
      float r = child->GetAttributeValueAsFloat ("radius", 1.0f);
      csRef<CS::Collisions::iColliderSphere> sphere =
	collisionSystem->CreateColliderSphere (r);
      collider = sphere;
      break;
    }
    case XMLTOKEN_BOX:
    {
      csVector3 v;
      if (!synldr->ParseVector (child, v))
      {
	synldr->ReportError ("crystalspace.collisions.helper", child,
			     "Error processing box parameters");
	continue;
      }
      csRef<CS::Collisions::iColliderBox> box = collisionSystem->CreateColliderBox (v);
      collider = box;
      break;
    }
    case XMLTOKEN_CYLINDER:
    {
      float l = child->GetAttributeValueAsFloat ("length", 1.0f);
      float r = child->GetAttributeValueAsFloat ("radius", 1.0f);
      csRef<CS::Collisions::iColliderCylinder> cylinder =
	collisionSystem->CreateColliderCylinder (l, r);
      collider = cylinder;
      break;
    }
    case XMLTOKEN_CAPSULE:
    {
      float l = child->GetAttributeValueAsFloat ("length", 1.0f);
      float r = child->GetAttributeValueAsFloat ("radius", 1.0f);
      csRef<CS::Collisions::iColliderCapsule> capsule =
	collisionSystem->CreateColliderCapsule (l, r);
      collider = capsule;
      break;
    }
    case XMLTOKEN_CONE:
    {
      float l = child->GetAttributeValueAsFloat ("length", 1.0f);
      float r = child->GetAttributeValueAsFloat ("radius", 1.0f);
      csRef<CS::Collisions::iColliderCone> cone =
	collisionSystem->CreateColliderCone (l, r);
      collider = cone;
      break;
    }
    case XMLTOKEN_PLANE:
    {
      csPlane3 planeParameters;
      if (synldr->ParsePlane (child, planeParameters))
      {
	synldr->ReportError ("crystalspace.collisions.helper", child,
			     "Error processing plane parameters");
	continue;
      }

      csRef<CS::Collisions::iColliderPlane> plane =
	collisionSystem->CreateColliderPlane (planeParameters);
      collider = plane;
    }
    case XMLTOKEN_CONVEXMESH:
    {
      // Search for the mesh factory holding the definition of the collider
      csRef<iMeshFactoryWrapper> meshFactory;

      // Search for a mesh specified through the dedicated tag
      csString meshName = child->GetAttributeValue ("mesh");
      if (!meshName.IsEmpty ())
      {
	meshFactory = loaderContext->FindMeshFactory (meshName);
	if (!meshFactory)
	  meshFactory = engine->FindMeshFactory (meshName);
	if (!meshFactory)
	{
	  synldr->ReportError ("crystalspace.collisions.helper", child,
			       "Could not find the mesh factory %s for the convex collider",
			       CS::Quote::Single (child->GetAttributeValue ("mesh")));
	  continue;
	}
      }

      // Search for a mesh in the context
      if (!meshFactory)
	meshFactory = scfQueryInterface<iMeshFactoryWrapper> (context);

      if (!meshFactory)
      {
	synldr->ReportError ("crystalspace.collisions.helper", child,
			     "No valid mesh factory defined for the convex collider");
	continue;
      }

      // Search for the triangle mesh associated to the factory
      iTriangleMesh* triangles = FindCollisionMesh (meshFactory);
      if (!triangles)
      {
	synldr->ReportError ("crystalspace.collisions.helper", child,
			     "Could not find the triangle mesh for the convex collider");
	continue;
      }

      // Create the collider
      csRef<CS::Collisions::iColliderConvexMesh> meshCollider =
	collisionSystem->CreateColliderConvexMesh (triangles);
      collider = meshCollider;
      break;
    }
    case XMLTOKEN_CONCAVEMESH:
    {
      // Search for the mesh factory holding the definition of the collider
      csRef<iMeshFactoryWrapper> meshFactory;

      // Search for a mesh specified through the dedicated tag
      csString meshName = child->GetAttributeValue ("mesh");
      if (!meshName.IsEmpty ())
      {
	meshFactory = loaderContext->FindMeshFactory (meshName);
	if (!meshFactory)
	  meshFactory = engine->FindMeshFactory (meshName);
	if (!meshFactory)
	{
	  synldr->ReportError ("crystalspace.collisions.helper", child,
			       "Could not find the mesh factory %s for the concave collider",
			       CS::Quote::Single (child->GetAttributeValue ("mesh")));
	  continue;
	}
      }

      // Search for a mesh in the context
      if (!meshFactory)
	meshFactory = scfQueryInterface<iMeshFactoryWrapper> (context);

      if (!meshFactory)
      {
	synldr->ReportError ("crystalspace.collisions.helper", child,
			     "No valid mesh factory defined for the concave collider");
	continue;
      }

      // Search for the triangle mesh associated to the factory
      iTriangleMesh* triangles = FindCollisionMesh (meshFactory);
      if (!triangles)
      {
	synldr->ReportError ("crystalspace.collisions.helper", child,
			     "Could not find the triangle mesh for the concave collider");
	continue;
      }

      // Create the collider
      csRef<CS::Collisions::iColliderConcaveMesh> meshCollider =
	collisionSystem->CreateColliderConcaveMesh (triangles);
      collider = meshCollider;
      break;
    }
    case XMLTOKEN_TERRAIN:
    {
      // TODO?
      break;
    }
    case XMLTOKEN_COLLIDER:
    {
      csOrthoTransform childTransform;
      csRef<CS::Collisions::iCollider> childCollider =
	ParseCollider (child, childTransform, loaderContext, context);
      // TODO: let the user define the children before the collider
      if (childCollider && collider)
	collider->AddChild (childCollider, childTransform);
      break;
    }
    default:
      synldr->ReportBadToken (child);
      break;
    }
  }

  return csPtr<CS::Collisions::iCollider> (collider);
}

csPtr<CS::Collisions::iCollisionObjectFactory>
CollisionHelper::ParseCollisionObjectFactory
(iDocumentNode* node, iLoaderContext* loaderContext, iBase* context) const
{
  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    const char *value = child->GetValue ();
    csStringID id = xmltokens.Request (value);
    switch (id)
    {
    case XMLTOKEN_COLLISIONOBJECT:
      return ParseCollisionObjectSimpleFactory (child, loaderContext, context);
    case XMLTOKEN_COLLISIONGHOST:
      return ParseCollisionObjectGhostFactory (child, loaderContext, context);
    case XMLTOKEN_COLLISIONACTOR:
      // TODO
      return ParseCollisionObjectSimpleFactory (child, loaderContext, context);
    case XMLTOKEN_RIGIDBODY:
    {
      if (!collisionSystem->QueryPhysicalSystem ())
	return ParseCollisionObjectSimpleFactory (child, loaderContext, context);
      csRef<CS::Physics::iRigidBodyFactory> body =
	ParseRigidBodyFactory (child, loaderContext, context);
      return body;
    }
    case XMLTOKEN_SOFTBODY:
      // TODO
      return ParseCollisionObjectSimpleFactory (child, loaderContext, context);
    default:
      break;
    }
  }

  return csPtr<CS::Collisions::iCollisionObjectFactory> (nullptr);
}

void CollisionHelper::ParseCollisionObjectProperties
(iDocumentNode* node, CS::Collisions::iCollisionObjectFactory* objectFactory,
 iLoaderContext* loaderContext, iBase* context) const
{
  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    const char *value = child->GetValue ();
    csStringID id = xmltokens.Request (value);
    switch (id)
    {
    case XMLTOKEN_GROUP:
    {
      const char* name = node->GetAttributeValue ("name");
      CS::Collisions::iCollisionGroup* group = collisionSystem->FindCollisionGroup (name);
      if (!group)
      {
	synldr->ReportError ("crystalspace.dynamics.loader", child,
			     "Could not find the collision group '%s'",
			     CS::Quote::Single (name));
	continue;
      }
      objectFactory->SetCollisionGroup (group);
      break;
    }

    case XMLTOKEN_COLLIDER:
    {
      csTransform transform;
      csRef<CS::Collisions::iCollider> collider =
	ParseCollider (child, transform, loaderContext, context);
      if (collider) objectFactory->SetCollider (collider, transform);
    }

    default:
      break;
    }
  }
}

csPtr<CS::Collisions::iCollisionObjectFactory> CollisionHelper::ParseCollisionObjectSimpleFactory
(iDocumentNode* node, iLoaderContext* loaderContext, iBase* context) const
{
  csRef<CS::Collisions::iCollisionObjectFactory> objectFactory =
    collisionSystem->CreateCollisionObjectFactory ();
  ParseCollisionObjectProperties (node, objectFactory, loaderContext, context);
  return objectFactory;
}

csPtr<CS::Collisions::iCollisionObjectFactory> CollisionHelper::ParseCollisionObjectGhostFactory
(iDocumentNode* node, iLoaderContext* loaderContext, iBase* context) const
{
  csRef<CS::Collisions::iCollisionObjectFactory> objectFactory =
    collisionSystem->CreateGhostCollisionObjectFactory ();
  ParseCollisionObjectProperties (node, objectFactory, loaderContext, context);
  return objectFactory;
}

void CollisionHelper::ParsePhysicalObjectProperties
(iDocumentNode* node, CS::Physics::iPhysicalObjectFactory* factory,
 iLoaderContext* loaderContext, iBase* context) const
{
  if (node->GetAttributeValue ("mass"))
    factory->SetMass (node->GetAttributeValueAsFloat ("mass"));
  if (node->GetAttributeValue ("density"))
    factory->SetDensity (node->GetAttributeValueAsFloat ("density"));
  if (node->GetAttributeValue ("friction"))
    factory->SetFriction (node->GetAttributeValueAsFloat ("friction"));

  // If the context object is a mesh factory then add this factory as a child of
  // the iObject of the mesh factory.
  if (context)
  {
    csRef<iMeshFactoryWrapper> meshFactory = scfQueryInterface<iMeshFactoryWrapper> (context);
    if (meshFactory) meshFactory->QueryObject ()->ObjAdd (factory->QueryObject ());
  }
}

csPtr<CS::Physics::iRigidBodyFactory> CollisionHelper::ParseRigidBodyFactory
(iDocumentNode* node, iLoaderContext* loaderContext, iBase* context) const
{
  CS_ASSERT (collisionSystem->QueryPhysicalSystem ());

  csRef<CS::Physics::iRigidBodyFactory> factory =
    collisionSystem->QueryPhysicalSystem ()->CreateRigidBodyFactory ();
  ParseCollisionObjectProperties (node, factory, loaderContext, context);
  ParsePhysicalObjectProperties (node, factory, loaderContext, context);

  if (node->GetAttributeValue ("elasticity"))
    factory->SetElasticity (node->GetAttributeValueAsFloat ("elasticity"));
  if (node->GetAttributeValue ("angulardamping"))
    factory->SetAngularDamping (node->GetAttributeValueAsFloat ("angulardamping"));
  if (node->GetAttributeValue ("lineardamping"))
    factory->SetLinearDamping (node->GetAttributeValueAsFloat ("lineardamping"));

  if (node->GetAttributeValue ("state"))
  {
    csString state = node->GetAttributeValue ("state");
    if (state == "dynamic") factory->SetState (CS::Physics::STATE_DYNAMIC);
    else if (state == "static") factory->SetState (CS::Physics::STATE_STATIC);
    else if (state == "kinematic") factory->SetState (CS::Physics::STATE_KINEMATIC);
  }

  return factory;
}

csPtr<CS::Physics::iSoftBodyFactory> CollisionHelper::ParseSoftBodyFactory
(iDocumentNode* node, iLoaderContext* loaderContext, iBase* context) const
{
  CS_ASSERT (collisionSystem->QueryPhysicalSystem ());

  csRef<CS::Physics::iSoftMeshFactory> factory =
      collisionSystem->QueryPhysicalSystem ()->CreateSoftMeshFactory ();
  //ParseCollisionObjectProperties (node, factory, loaderContext, context);
  ParsePhysicalObjectProperties (node, factory, loaderContext, context);

  csRef<iMeshFactoryWrapper> meshFactory; 

  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    const char *value = child->GetValue ();
    csStringID id = xmltokens.Request (value);
    switch (id)
    {
      case XMLTOKEN_MESHFACT:
      {
	const char* name = child->GetAttributeValue ("name");

	// Search for the mesh specified in the context then in the engine
	if (name && *name)
	{
	  meshFactory = loaderContext->FindMeshFactory (name);
	  if (!meshFactory)
	    meshFactory = engine->FindMeshFactory (name);
	  if (!meshFactory)
	  {
	    synldr->ReportError ("crystalspace.collisions.helper", child,
				 "Could not find the mesh factory %s for the soft body factory",
				 CS::Quote::Single (name));
	    continue;
	  }
	}

	// Parse the duplication mode
	const char* duplication = child->GetAttributeValue ("duplication");
	if (duplication && *duplication)
	{
	  if (!strcmp (duplication, "none")) {}
	  else if (!strcmp (duplication, "interleaved"))
	    factory->SetDuplicationMode (CS::Physics::MESH_DUPLICATION_INTERLEAVED);
	  else if (!strcmp (duplication, "contiguous"))
	    factory->SetDuplicationMode (CS::Physics::MESH_DUPLICATION_CONTIGUOUS);
	  else
	    synldr->ReportError ("crystalspace.collisions.helper", child,
				 "Wrong mesh duplication mode for the soft body factory: %s",
				 CS::Quote::Single (duplication));
	}

	break;
      }

    case XMLTOKEN_STIFFNESS:
    {
      if (child->GetAttributeValue ("linear"))
	factory->SetLinearStiffness (child->GetAttributeValueAsFloat ("linear", 1.f));
      if (child->GetAttributeValue ("angular"))
	factory->SetAngularStiffness (child->GetAttributeValueAsFloat ("angular", 1.f));
      if (child->GetAttributeValue ("volume"))
	factory->SetVolumeStiffness (child->GetAttributeValueAsFloat ("volume", 1.f));

      break;
    }

      // TODO: Parse the remaining soft body parameters
    default:
      synldr->ReportBadToken (child);
      continue;
    }
  }

  // Search for a mesh in the context
  if (!meshFactory)
    meshFactory = scfQueryInterface<iMeshFactoryWrapper> (context);

  if (!meshFactory)
  {
    synldr->ReportError ("crystalspace.collisions.helper", node,
			 "No valid mesh factory defined for the soft body factory");
    return csPtr<CS::Physics::iSoftBodyFactory> (nullptr);
  }

  // Search for the triangle mesh associated to the factory
  iTriangleMesh* triangles = FindCollisionMesh (meshFactory);
  if (!triangles)
  {
    synldr->ReportError ("crystalspace.collisions.helper", node,
			 "Could not find the triangle mesh for the soft body factory");
    return csPtr<CS::Physics::iSoftBodyFactory> (nullptr);
  }

  factory->SetMesh (triangles);

  // If the context is a genmesh factory, then add automatically a soft body animation controller
  meshFactory = scfQueryInterface<iMeshFactoryWrapper> (context);
  if (meshFactory)
  {
    csRef<iGeneralFactoryState> genmeshFactory =
      scfQueryInterface<iGeneralFactoryState> (meshFactory->GetMeshObjectFactory ());

    if (genmeshFactory && !genmeshFactory->GetAnimationControlFactory ())
    {
      // Search for the animation control plugin
      csRef<iPluginManager> pluginManager = csQueryRegistry<iPluginManager> (objectRegistry);
      csRef<CS::Animation::iSoftBodyAnimationControl2Type> softBodyAnimationType =
	csLoadPlugin<CS::Animation::iSoftBodyAnimationControl2Type>
	(pluginManager, "crystalspace.physics.softanim2");
      if (!softBodyAnimationType)
	ReportError ("Could not load the soft body animation controller");

      else
      {
	// Set up the animation control factory
	csRef<iGenMeshAnimationControlFactory> animationControl = 
	  softBodyAnimationType->CreateAnimationControlFactory ();
	genmeshFactory->SetAnimationControlFactory (animationControl);
      }
    }
  }

  return factory;
}

bool CollisionHelper::ParseJointConstraint
(iDocumentNode *node, bool &x, bool &y, bool &z, csVector3 &min, csVector3 &max) const
{
  x = strcmp (node->GetAttributeValue ("x"), "true") == 0;
  y = strcmp (node->GetAttributeValue ("y"), "true") == 0;
  z = strcmp (node->GetAttributeValue ("z"), "true") == 0;
  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    const char *value = child->GetValue ();
    csStringID id = xmltokens.Request (value);
    switch (id)
    {
    case XMLTOKEN_MIN:
      synldr->ParseVector (child, min);
      break;
    case XMLTOKEN_MAX:
      synldr->ParseVector (child, max);
      break;
    default:
      synldr->ReportBadToken (child);
      return false;
    }
  }
  return true;
}

csPtr<CS::Physics::iJointFactory> CollisionHelper::ParseJointFactory
(iDocumentNode* node, csTransform& transform, iLoaderContext* loaderContext,
 iBase* context) const
{
  CS_ASSERT (collisionSystem->QueryPhysicalSystem ());

  csRef<CS::Physics::iJointFactory> factory =
    collisionSystem->QueryPhysicalSystem ()->CreateJointFactory ();

  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    const char *value = child->GetValue ();
    csStringID id = xmltokens.Request (value);
    switch (id)
    {
    case XMLTOKEN_CONSTRAINTS:
    {
      csRef<iDocumentNodeIterator> cit = child->GetNodes ();
      while (cit->HasNext ())
      {
	csRef<iDocumentNode> cchild = cit->Next ();
	if (cchild->GetType () != CS_NODE_ELEMENT) continue;
	const char *cvalue = cchild->GetValue ();
	csStringID cid = xmltokens.Request (cvalue);
	switch (cid)
	{
	case XMLTOKEN_DISTANCE:
	{
	  bool x, y, z;
	  csVector3 min, max;
	  ParseJointConstraint (cchild, x, y, z, min, max);
	  factory->SetTransConstraints (x, y, z);
	  factory->SetMinimumDistance (min);
	  factory->SetMaximumDistance (max);
	  break;
	}
	case XMLTOKEN_ANGLE:
	{
	  bool x, y, z;
	  csVector3 min, max;
	  ParseJointConstraint (cchild, x, y, z, min, max);
	  factory->SetRotConstraints (x, y, z);
	  factory->SetMinimumAngle (min);
	  factory->SetMaximumAngle (max);
	  break;
	}
	default:
	  synldr->ReportBadToken (cchild);
	  break;
	}
      }
      break;
    }
    // TODO: use attribute values instead of nodes
    case XMLTOKEN_BOUNCE:
    {
      csVector3 bounce;
      synldr->ParseVector (child, bounce);
      factory->SetBounce (bounce);
      break;
    }
    case XMLTOKEN_SPRING:
    {
      factory->SetSpring (true);
      break;
    }
    case XMLTOKEN_LINEARSTIFFNESS:
    {
      csVector3 v;
      synldr->ParseVector (child, v);
      factory->SetLinearStiffness (v);
      break;
    }
    case XMLTOKEN_ANGULARSTIFFNESS:
    {
      csVector3 v;
      synldr->ParseVector (child, v);
      factory->SetAngularStiffness (v);
      break;
    }
    case XMLTOKEN_LINEARDAMPING:
    {
      csVector3 v;
      synldr->ParseVector (child, v);
      factory->SetLinearDamping (v);
      break;
    }
    case XMLTOKEN_ANGULARDAMPING:
    {
      csVector3 v;
      synldr->ParseVector (child, v);
      factory->SetAngularDamping (v);
      break;
    }
    case XMLTOKEN_DESIREDVELOCITY:
    {
      csVector3 v;
      synldr->ParseVector (child, v);
      factory->SetDesiredVelocity (v);
      break;
    }
    case XMLTOKEN_MOVE:
    {
      csVector3 v;
      synldr->ParseVector (child, v);
      transform.SetOrigin (v);
      break;
    }
    case XMLTOKEN_ROTATE:
    {
      csMatrix3 m;
      synldr->ParseMatrix (child, m);
      transform.SetO2T (m);
      break;
    }
    default:
      synldr->ReportBadToken (child);
      break;
    }
  }

  return factory;
}
