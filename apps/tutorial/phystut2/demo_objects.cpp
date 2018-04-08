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

/**
 * Create dynamic (interactive) objects
 */

// TODO: use actually the factories in order to create them only once

#include "cssysdef.h"
#include "csgeom/poly3d.h"
#include "csgeom/sphere.h"
#include "iengine/portal.h"
#include "imesh/emit.h"
#include "imesh/genmesh.h"
#include "imesh/particles.h"
#include "imesh/terrain2.h"
#include "cstool/genmeshbuilder.h"
#include "cstool/materialbuilder.h"
#include "csutil/floatrand.h"

#include "physdemo.h"

using namespace CS::Collisions;
using namespace CS::Physics;
using namespace CS::Geometry;

csPtr<CS::Physics::iRigidBody> RenderMeshColliderPair::SpawnRigidBody (const csOrthoTransform& trans,
  float friction, float density)
{ 
  csRef<iRigidBodyFactory> factory = physDemo.physicalSystem->CreateRigidBodyFactory (Collider);
  factory->SetDensity (density);
  factory->SetElasticity (DefaultElasticity);
  factory->SetFriction (friction);

  csRef<CS::Physics::iRigidBody> body = factory->CreateRigidBody ();
  body->SetTransform (trans);
  
  iMaterialWrapper* mat = physDemo.engine->GetMaterialList ()->FindByName ("stone");
  csRef<iMeshWrapper> mesh = MeshFactory->CreateMeshWrapper ();
  mesh->GetMeshObject ()->SetMaterialWrapper (mat);

  body->SetAttachedSceneNode (mesh->QuerySceneNode ());
  body->QueryObject ()->SetObjectParent (mesh->QueryObject ());

  // Add to world
  physDemo.GetCurrentSector ()->AddCollisionObject (body);

  return csPtr<CS::Physics::iRigidBody>(body);
}

void PhysDemo::CreateBoxMeshColliderPair (RenderMeshColliderPair& pair, const csVector3& extents)
{
  DensityTextureMapper mapper (0.3f);
  TesselatedBox tbox (-extents/2, extents/2);
  tbox.SetLevel (3);
  tbox.SetMapper (&mapper);
  
  pair.MeshFactory = GeneralMeshBuilder::CreateFactory (engine, "box", &tbox);
  pair.Collider = csRef<iColliderBox>(physicalSystem->CreateColliderBox (extents));
}

csRef<CS::Physics::iRigidBody> PhysDemo::SpawnBox (bool setVelocity /* = true */)
{
  csVector3 extents (0.4f, 0.8f, 0.4f);
  csVector3 pos = view->GetCamera ()->GetTransform ().GetOrigin () + 1.5f * GetCameraDirection ();
  return SpawnBox (extents, pos, setVelocity);
}

csRef<CS::Physics::iRigidBody> PhysDemo::SpawnBox (const csVector3& extents, const csVector3& pos, bool setVelocity /* = true */)
{
  // Create a box collider & mesh and then the actual box object
  RenderMeshColliderPair pair;
  CreateBoxMeshColliderPair (pair, extents);
  return SpawnRigidBody (pair, pos, setVelocity);
}

csRef<CS::Physics::iRigidBody> PhysDemo::SpawnRigidBody (RenderMeshColliderPair& pair, const csVector3& pos,
  float friction, float density, bool setVelocity)
{
  //static csRandomFloatGen randGen;

  //trans.RotateThis (UpVector, randGen.GetAngle ());

  // Create body
  

  // Set transform
  csOrthoTransform trans; trans.SetOrigin (pos);
  csRef<CS::Physics::iRigidBody> body = pair.SpawnRigidBody (trans, friction, density);

  if (setVelocity)
  {
    // Fling the body.
    const csOrthoTransform& tc = view->GetCamera ()->GetTransform ();
    body->SetLinearVelocity (tc.GetT2O () * csVector3 (0, 0, 5));
    body->SetAngularVelocity (tc.GetT2O () * csVector3 (5, 0, 0));
  }

  return body;
}

void PhysDemo::CreateGhostCylinder ()
{
  // Create the cylinder mesh factory.
  csRef<iMeshFactoryWrapper> cylinderFact = engine->CreateMeshFactory (
    "crystalspace.mesh.object.genmesh", "cylinderFact");

  if (!cylinderFact)
  {
    ReportError ("Error creating mesh object factory!");
    return;
  }

  csRef<iGeneralFactoryState> gmstate = scfQueryInterface<
    iGeneralFactoryState> (cylinderFact->GetMeshObjectFactory ());

  const float radius (1.5f);
  const float length (4.0f);
  gmstate->GenerateCylinder (length, radius, 10);

  // Create the mesh.
  csRef<iMeshWrapper> mesh (engine->CreateMeshWrapper (cylinderFact, "cylinder"));

  iMaterialWrapper* mat = engine->GetMaterialList ()->FindByName ("objtexture");
  mesh->GetMeshObject ()->SetMaterialWrapper (mat);

  // Create a body and attach the mesh
  csRef<CS::Collisions::iColliderCylinder> cylinder = physicalSystem->CreateColliderCylinder (length, radius);
  csRef<iCollisionObjectFactory> factory = physicalSystem->CreateGhostCollisionObjectFactory (cylinder);

  ghostObject = factory->CreateCollisionObject ();
  
  csYRotMatrix3 m (PI/2.0);
  csOrthoTransform trans (m, csVector3 (0, -3, 5));

  // look into: Find fitting location in an arbitrary level
  ghostObject->SetTransform (trans);
  ghostObject->SetAttachedSceneNode (mesh->QuerySceneNode ());

  // It won't work for ghost and actor.
  //ghostObject->Rotate (csVector3 (0, 1, 0), PI/2.0);
  //ghostObject->AddCollider (cylinder, trans)

  GetCurrentSector ()->AddCollisionObject (ghostObject);
}

CS::Physics::iRigidBody* PhysDemo::SpawnSphere ()
{
  // Use the camera transform.
  const csOrthoTransform& tc = view->GetCamera ()->GetTransform ();

  // Create the ball mesh factory.
  csRef<iMeshFactoryWrapper> ballFact =
    engine->CreateMeshFactory ("crystalspace.mesh.object.genmesh", "ballFact");
  if (!ballFact)
  {
    ReportError ("Error creating mesh object factory!");
    return 0;
  }

  csRef<iGeneralFactoryState> gmstate = scfQueryInterface<
    iGeneralFactoryState> (ballFact->GetMeshObjectFactory ());

  float radiusf = rand () %5 / 10.f + .2f;
  csVector3 radius (radiusf, radiusf, radiusf);
  csEllipsoid ellips (csVector3 (0), radius);
  gmstate->GenerateSphere (ellips, 16);

  // We do a hardtransform here to make sure our sphere has an artificial
  // offset over its center of gravity. That way we can test if the physics
  // engine supports that.
  csReversibleTransform artificialTransform =
    csReversibleTransform (csMatrix3 (), csVector3 (1, 1, 1));
  ballFact->HardTransform (artificialTransform);

  // Create the mesh.
  csRef<iMeshWrapper> mesh (engine->CreateMeshWrapper (ballFact, "ball"));

  iMaterialWrapper* mat = engine->GetMaterialList ()->FindByName ("objtexture");
  if (!mat)
  {
    // something went wrong
    ReportWarning ("Could not find material: objtexture - Alternatives are:");
    for (int i = 0; i < engine->GetMaterialList ()->GetCount (); ++i)
    {
      iMaterialWrapper* wrap =  engine->GetMaterialList ()->Get (i);
      csString str = wrap->QueryObject ()->GetName ();
      ReportWarning (" %s", str.GetData ());
    }
  }
  mesh->GetMeshObject ()->SetMaterialWrapper (mat);

  // Create a body and attach the mesh and attach a sphere collider.
  csRef<CS::Collisions::iColliderSphere> sphere = physicalSystem->CreateColliderSphere (1.0);
  sphere->SetLocalScale (radius);
  csRef<iRigidBodyFactory> factory = physicalSystem->CreateRigidBodyFactory (sphere);
  factory->SetDensity (DefaultDensity);
  factory->SetElasticity (DefaultElasticity);
  factory->SetFriction (DefaultFriction);
  factory->SetColliderTransform (artificialTransform);
  csRef<CS::Physics::iRigidBody> rb = factory->CreateRigidBody ();

  iSceneNode* meshNode = mesh->QuerySceneNode ();
  rb->SetAttachedSceneNode (meshNode);
  rb->QueryObject ()->SetObjectParent (mesh->QueryObject ());

  csOrthoTransform trans = tc;
  trans.SetOrigin (view->GetCamera ()->GetTransform ().GetOrigin () + GetCameraDirection ());
  rb->SetTransform (artificialTransform.GetInverse () * trans);
  
  // Fling the body.
  rb->SetLinearVelocity (tc.GetT2O () * csVector3 (0, 0, 5));
  rb->SetAngularVelocity (tc.GetT2O () * csVector3 (5, 0, 0));

  GetCurrentSector ()->AddCollisionObject (rb);

  return rb;
}

CS::Physics::iRigidBody* PhysDemo::SpawnCone (bool setVelocity /* = true */)
{
  // Use the camera transform.
  const csOrthoTransform& tc = view->GetCamera ()->GetTransform ();

  const float radius (0.4f);
  const float length (0.8f);

  // Create the cone mesh factory.
  csRef<iMeshFactoryWrapper> coneFact =
    engine->CreateMeshFactory ("crystalspace.mesh.object.genmesh", "coneFact");
  if (!coneFact)
  {
    ReportError ("Error creating mesh object factory!");
    return 0;
  }

  csRef<iGeneralFactoryState> gmstate = scfQueryInterface<
    iGeneralFactoryState> (coneFact->GetMeshObjectFactory ());
  gmstate->GenerateCone (length, radius, 10);
  coneFact->HardTransform
    (csReversibleTransform (csXRotMatrix3 (PI * -0.5f), csVector3 (0.0f, 0.0f, length * -0.5f)));

  // Create the mesh.
  csRef<iMeshWrapper> mesh (engine->CreateMeshWrapper (
    coneFact, "cone"));

  iMaterialWrapper* mat = engine->GetMaterialList ()->FindByName ("objtexture");
  mesh->GetMeshObject ()->SetMaterialWrapper (mat);

  // Create a body and attach the mesh.
  csRef<CS::Collisions::iColliderCone> cone = physicalSystem->CreateColliderCone (length, radius);
  //cone->SetLocalScale (csVector3 (rand ()%5/10. + .2, rand ()%5/10. + .2, rand ()%5/10. + .2));

  // Create object
  csRef<iRigidBodyFactory> factory = physicalSystem->CreateRigidBodyFactory (cone);
  factory->SetDensity (DefaultDensity);
  factory->SetElasticity (DefaultElasticity);
  factory->SetFriction (DefaultFriction);

  csRef<CS::Physics::iRigidBody> rb = factory->CreateRigidBody ();

  csOrthoTransform trans = tc;
  trans.SetOrigin (tc.GetOrigin () + tc.GetT2O () * csVector3 (0, 0, 1));
  trans.RotateThis (csXRotMatrix3 (PI / 5.0));
  rb->SetTransform (trans);
  rb->SetAttachedSceneNode (mesh->QuerySceneNode ());
  rb->QueryObject ()->SetObjectParent (mesh->QueryObject ());

  if (setVelocity)
  {
    // Fling the body.
    rb->SetLinearVelocity (tc.GetT2O () * csVector3 (0, 0, 5));
    rb->SetAngularVelocity (tc.GetT2O () * csVector3 (5, 0, 0));
  }

  // TODO: Attach a mesh
  GetCurrentSector ()->AddCollisionObject (rb);

  return rb;
}

CS::Physics::iRigidBody* PhysDemo::SpawnCylinder (bool setVelocity /* = true */)
{
  // Use the camera transform.
  const csOrthoTransform& tc = view->GetCamera ()->GetTransform ();

  // Create the cylinder mesh factory.
  csRef<iMeshFactoryWrapper> cylinderFact =
    engine->CreateMeshFactory ("crystalspace.mesh.object.genmesh", "cylinderFact");
  if (!cylinderFact)
  {
    ReportError ("Error creating mesh object factory!");
    return 0;
  }

  csRef<iGeneralFactoryState> gmstate = scfQueryInterface<
    iGeneralFactoryState> (cylinderFact->GetMeshObjectFactory ());
  const float radius (rand () % 10 / 50. + .2);
  const float length (rand () % 3 / 50. + .7);
  gmstate->GenerateCylinder (length, radius, 10);

  // We do a hardtransform here to make sure our cylinder has an artificial
  // offset. That way we can test if the physics engine supports that.
  //csVector3 artificialOffset (3, 3, 3);
  csVector3 artificialOffset (0, 0, 0);
  csReversibleTransform hardTransform (csZRotMatrix3 (PI/2.0), artificialOffset);
  cylinderFact->HardTransform (hardTransform);

  // Create the mesh.
  csRef<iMeshWrapper> mesh (engine->CreateMeshWrapper (
    cylinderFact, "cylinder"));

  iMaterialWrapper* mat = engine->GetMaterialList ()->FindByName ("objtexture");
  mesh->GetMeshObject ()->SetMaterialWrapper (mat);

  // Create a body and attach the mesh.
  csRef<CS::Collisions::iColliderCylinder> cylinder =
    physicalSystem->CreateColliderCylinder (length, radius);
  csMatrix3 m;

  csRef<iRigidBodyFactory> factory = physicalSystem->CreateRigidBodyFactory (cylinder);
  factory->SetDensity (DefaultDensity);
  factory->SetElasticity (DefaultElasticity);
  factory->SetFriction (DefaultFriction);
  csRef<CS::Physics::iRigidBody> rb = factory->CreateRigidBody ();

  csOrthoTransform trans = tc;
  trans.SetOrigin (tc.GetOrigin () + tc.GetT2O () * csVector3 (0, 0, 1));
  trans.RotateThis (csXRotMatrix3 (PI / 5.0));
  rb->SetTransform (trans);
  rb->SetAttachedSceneNode (mesh->QuerySceneNode ());
  rb->QueryObject ()->SetObjectParent (mesh->QueryObject ());

  if (setVelocity)
  {
    // Fling the body.
    rb->SetLinearVelocity (tc.GetT2O () * csVector3 (0, 0, 5));
    rb->SetAngularVelocity (tc.GetT2O () * csVector3 (5, 0, 0));
  }
  
  //rb->RebuildObject ();
  GetCurrentSector ()->AddCollisionObject (rb);

  return rb;
}

CS::Physics::iRigidBody* PhysDemo::SpawnCapsule (float length, float radius, bool setVelocity /* = true */)
{
  // Use the camera transform.
  const csOrthoTransform& tc = view->GetCamera ()->GetTransform ();

  // Create the capsule mesh factory.
  csRef<iMeshFactoryWrapper> capsuleFact =
    engine->CreateMeshFactory ("crystalspace.mesh.object.genmesh", "capsuleFact");
  if (!capsuleFact)
  {
    ReportError ("Error creating mesh object factory!");
    return 0;
  }

  csRef<iGeneralFactoryState> gmstate =
    scfQueryInterface<iGeneralFactoryState> (capsuleFact->GetMeshObjectFactory ());
  gmstate->GenerateCapsule (length, radius, 10);
  capsuleFact->HardTransform
    (csReversibleTransform (csYRotMatrix3 (PI * 0.5f), csVector3 (0)));

  // Create the mesh.
  csRef<iMeshWrapper> mesh (engine->CreateMeshWrapper (
    capsuleFact, "capsule"));
  iMaterialWrapper* mat = engine->GetMaterialList ()->FindByName ("objtexture");
  mesh->GetMeshObject ()->SetMaterialWrapper (mat);

  // Create a body
  csRef<CS::Collisions::iColliderCapsule> capsule =
    physicalSystem->CreateColliderCapsule (length, radius);
  csRef<iRigidBodyFactory> factory = physicalSystem->CreateRigidBodyFactory (capsule);
  factory->SetDensity (DefaultDensity);
  factory->SetElasticity (DefaultElasticity);
  factory->SetFriction (DefaultFriction);
  csRef<CS::Physics::iRigidBody> rb = factory->CreateRigidBody ();

  // set transform
  csOrthoTransform trans = tc;
  trans.SetOrigin (tc.GetOrigin () + tc.GetT2O () * csVector3 (0, 0, 1));
  trans.RotateThis (csXRotMatrix3 (PI / 5.0));
  rb->SetTransform (trans);

  // attach the mesh
  rb->SetAttachedSceneNode (mesh->QuerySceneNode ());
  rb->QueryObject ()->SetObjectParent (mesh->QueryObject ());

  // Add to world
  GetCurrentSector ()->AddCollisionObject (rb);

  if (setVelocity)
  {
    // Fling the body.
    rb->SetLinearVelocity (tc.GetT2O () * csVector3 (0, 0, 5));
    rb->SetAngularVelocity (tc.GetT2O () * csVector3 (5, 0, 0));
  }

  return rb;
}

CS::Collisions::iCollisionObject* PhysDemo::SpawnConcaveMesh ()
{
  // Find the 'star' mesh factory
  csRef<iMeshFactoryWrapper> starFact = engine->FindMeshFactory ("genstar");
  if (!starFact)
  {
    loader->Load ("/lib/std/star.xml");
    starFact = engine->FindMeshFactory ("genstar");
    if (!starFact)
    {
      ReportError ("Error loading %s!", CS::Quote::Single ("star.xml"));
      return nullptr;
    }
  }

  // Use the camera transform.
  csOrthoTransform tc = view->GetCamera ()->GetTransform ();
  tc.SetOrigin (tc.This2Other (csVector3 (0, 0, 3)));

  // Create the mesh.
  csRef<iMeshWrapper> star = engine->CreateMeshWrapper (starFact, "star");
  star->GetMovable ()->SetTransform (tc);
  star->GetMovable ()->UpdateMove ();

  // The first time that this method is called, we create a simple 'concave mesh' collider
  csRef<CS::Collisions::iCollider> starCollider;
  float scale = 0.f;
  if (!mainCollider)
  {
    mainCollider = physicalSystem->CreateColliderConcaveMesh
      (collisionHelper.FindCollisionMesh (star), true);
    starCollider = mainCollider;
  }

  // If this method has already been called, then we create a 'scaled concave mesh' static
  // collider instead (the only purpose of this being to test them)
  else
  {
    scale = 2.f;
    starCollider = csRef<CS::Collisions::iColliderConcaveMeshScaled>
      (physicalSystem->CreateColliderConcaveMeshScaled (mainCollider, scale));
  }  

  // Create the rigid body
  csRef<iRigidBodyFactory> factory = physicalSystem->CreateRigidBodyFactory (starCollider);
  factory->SetDensity (DefaultDensity);
  factory->SetElasticity (DefaultElasticity);
  factory->SetFriction (DefaultFriction);

  csRef<CS::Physics::iRigidBody> body = factory->CreateRigidBody ();

  // Attach the star mesh and set its transform
  csOrthoTransform trans = tc;
  trans.SetOrigin (tc.GetOrigin () + tc.GetT2O () * csVector3 (0, 0, 2));
  body->SetAttachedSceneNode (star->QuerySceneNode ());
  body->QueryObject ()->SetObjectParent (star->QueryObject ());
  body->SetTransform (trans);

  // Add the rigid body to the collision sector
  GetCurrentSector ()->AddCollisionObject (body);

  // If we spawn a scaled concave collider, then scale the mesh too.
  if (scale > EPSILON)
  {
    csMatrix3 scaling;
    scaling.Identity ();
    scaling *= scale;

    csOrthoTransform transform = star->GetMovable ()->GetFullTransform ();
    transform.SetT2O (scaling * transform.GetT2O ());

    star->GetMovable ()->SetFullTransform (transform);
    star->GetMovable ()->UpdateMove ();
  }

  return body;
}

CS::Physics::iRigidBody* PhysDemo::SpawnConvexMesh (bool setVelocity /* = true */)
{
  // Use the camera transform.
  const csOrthoTransform& tc = view->GetCamera ()->GetTransform ();

  // Create the mesh factory (a capsule in this example)
  csRef<iMeshFactoryWrapper> capsuleFact = engine->CreateMeshFactory (
    "crystalspace.mesh.object.genmesh", "capsuleFact");
  if (!capsuleFact)
  {
    ReportError ("Error creating mesh object factory!");
    return 0;
  }

  csRef<iGeneralFactoryState> gmstate = scfQueryInterface<
    iGeneralFactoryState> (capsuleFact->GetMeshObjectFactory ());
  const float radius (rand () % 10 / 50. + .2);
  const float length (rand () % 3 / 50. + .7);
  gmstate->GenerateCapsule (length, radius, 10);
  capsuleFact->HardTransform (
    csReversibleTransform (csYRotMatrix3 (PI/2), csVector3 (0)));

  // Create the mesh.
  csRef<iMeshWrapper> mesh (engine->CreateMeshWrapper (
    capsuleFact, "capsule"));
  iMaterialWrapper* mat = engine->GetMaterialList ()->FindByName ("objtexture");
  mesh->GetMeshObject ()->SetMaterialWrapper (mat);

  // Create a body and attach the mesh.
  csRef<CS::Collisions::iColliderConvexMesh> collider =
    physicalSystem->CreateColliderConvexMesh (collisionHelper.FindCollisionMesh (mesh));
  csRef<iRigidBodyFactory> factory = physicalSystem->CreateRigidBodyFactory (collider);
  factory->SetDensity (DefaultDensity);
  factory->SetElasticity (DefaultElasticity);
  factory->SetFriction (DefaultFriction);

  csRef<CS::Physics::iRigidBody> rb = factory->CreateRigidBody ();

  // Set transform
  csOrthoTransform trans = tc;
  trans.SetOrigin (tc.GetOrigin () + tc.GetT2O () * csVector3 (0, 0, 1));
  rb->SetTransform (trans);
  
  // Attach mesh
  rb->SetAttachedSceneNode (mesh->QuerySceneNode ());
  rb->QueryObject ()->SetObjectParent (mesh->QueryObject ());
  
  if (setVelocity)
  {
    // Fling the body.
    rb->SetLinearVelocity (tc.GetT2O () * csVector3 (0, 0, 5));
    rb->SetAngularVelocity (tc.GetT2O () * csVector3 (5, 0, 0));
  }

  // Add to world
  GetCurrentSector ()->AddCollisionObject (rb);

  return rb;
}

CS::Physics::iRigidBody* PhysDemo::SpawnCompound (bool setVelocity /* = true */)
{
  // Use the camera transform.
  const csOrthoTransform& tc = view->GetCamera ()->GetTransform ();

  // Create the mesh.
  csRef<iMeshFactoryWrapper> meshFact = loader->LoadMeshObjectFactory ("/varia/physmesh");
  if (!meshFact)
  { 
    ReportError ("Error loading mesh object factory!"); 
    meshFact = engine->CreateMeshFactory ("crystalspace.mesh.object.genmesh", "ballFact");
  }

  csRef<iMeshWrapper> mesh (engine->CreateMeshWrapper (meshFact, "mesh"));

  // Perform the convex decomposition of the mesh (or fallback to a concave mesh if
  // there are no decomposer)
  csRef<CS::Collisions::iCollider> rootCollider;
  if (convexDecomposer)
  {
    rootCollider = csRef<CS::Collisions::iCollider> (physicalSystem->CreateCollider ());
    collisionHelper.DecomposeConcaveMesh (mesh, rootCollider, convexDecomposer);
  }
  else rootCollider = csRef<CS::Collisions::iColliderConcaveMesh>
	 (physicalSystem->CreateColliderConcaveMesh
	  (collisionHelper.FindCollisionMesh (mesh)));

  // Create a body
  csRef<iRigidBodyFactory> factory = physicalSystem->CreateRigidBodyFactory (rootCollider);
  factory->SetDensity (DefaultDensity);
  factory->SetElasticity (DefaultElasticity);
  factory->SetFriction (DefaultFriction);

  csRef<CS::Physics::iRigidBody> rb = factory->CreateRigidBody ();

  // Set transform
  csOrthoTransform trans = tc;
  trans.SetOrigin (tc.GetOrigin () + tc.GetT2O () * csVector3 (0, 0, 2));
  rb->SetTransform (trans);
  rb->SetAttachedSceneNode (mesh->QuerySceneNode ());
  rb->QueryObject ()->SetObjectParent (mesh->QueryObject ());

  // Add to world
  GetCurrentSector ()->AddCollisionObject (rb);

  if (setVelocity)
  {
    // Fling the body.
    rb->SetLinearVelocity (tc.GetT2O () * csVector3 (0, 0, 5));
    rb->SetAngularVelocity (tc.GetT2O () * csVector3 (5, 0, 0));
  }

  return rb;
}

CS::Physics::iJoint* PhysDemo::SpawnJointed ()
{
  // TODO: provide some runtime options about that
#define P2P
//#define CONETWIST
//#define HINGE
//#define SLIDE
//#define SPRING
//#define SOFT_LINEAR
//#define SOFT_ANGULAR

#ifdef P2P
  // Create and position two rigid bodies
  // Already added to sector.
  CS::Physics::iRigidBody* rb1 = SpawnBox (false);
  csOrthoTransform trans = rb1->GetTransform ();
  trans.SetOrigin (trans.GetOrigin () + trans.GetT2O () * csVector3 (0.5f, 0.5f, 0.0f));
  rb1->SetTransform (trans);
  csVector3 jointPosition = trans.This2Other (csVector3 (-0.2f, 0.4f, 0.2f));

  CS::Physics::iRigidBody* rb2 = SpawnBox (false);
  trans = rb2->GetTransform ();
  trans.SetOrigin (trans.GetOrigin () + trans.GetT2O () * csVector3 (0.0f, 0.0f, 0.0f));
  rb2->SetTransform (trans);
  rb2->SetState (CS::Physics::STATE_STATIC);

  csRef<CS::Physics::iJointFactory> jointFactory = physicalSystem->CreateP2PJointFactory ();
  csRef<CS::Physics::iJoint> joint = jointFactory->CreateJoint ();
  joint->SetPosition (jointPosition);
  joint->SetMinimumAngle (csVector3 (-HALF_PI, 0.0f, 0.0f));
  joint->SetMaximumAngle (csVector3 (HALF_PI, 0.0f, 0.0f));
  joint->Attach (rb2, rb1);
#endif

#ifdef CONETWIST
  // Create and position two rigid bodies
  // Already added to sector.
  CS::Physics::iRigidBody* rb1 = SpawnBox (false);
  csOrthoTransform trans = rb1->GetTransform ();
  trans.SetOrigin (trans.GetOrigin () + trans.GetT2O () * csVector3 (0.0f, 1.0f, 0.0f));
  rb1->SetTransform (trans);
  //csVector3 jointPosition = trans.This2Other (csVector3 (-0.2f, 0.4f, 0.2f));

  CS::Physics::iRigidBody* rb2 = SpawnBox (false);
  trans = rb2->GetTransform ();
  csOrthoTransform jointTrans = trans;
  jointTrans.SetO2T (csZRotMatrix3 (PI / 2.0f));
  trans.SetOrigin (trans.GetOrigin () + trans.GetT2O () * csVector3 (0.0f, -1.0f, 0.0f));
  rb2->SetTransform (trans);
  rb2->SetState (CS::Physics::STATE_STATIC);

  csRef<CS::Physics::iJointFactory> jointFactory =
    physicalSystem->CreateConeTwistJointFactory (PI * 0.15f, PI * 0.25f, PI * 0.8f);
  csRef<CS::Physics::iJoint> joint = jointFactory->CreateJoint ();
  joint->SetTransform (jointTrans);
  joint->Attach (rb1, rb2);
#endif

#ifdef HINGE
  CS::Physics::iRigidBody* rb1 = SpawnBox (false);
  csOrthoTransform trans = rb1->GetTransform ();
  trans.SetOrigin (trans.GetOrigin () + csVector3 (1.0f, 2.0f, 0.0f));
  rb1->SetTransform (trans);
  rb1->SetLinearVelocity (csVector3 (0.0f));
  rb1->SetAngularVelocity (csVector3 (0.0f));
  csVector3 jointPosition = trans.This2Other (csVector3 (0.2f, -0.2f, 0.2f));
  // Create a joint and attach the two bodies to it.

  csRef<CS::Physics::iJointFactory> jointFactory =
    physicalSystem->CreateHingeJointFactory (PI, -PI, 1);
  csRef<CS::Physics::iJoint> joint = jointFactory->CreateJoint ();
  joint->SetPosition (jointPosition);

  // Add a motor to the joint
  joint->SetDesiredVelocity (csVector3 (0.0f, -0.5f, 0.0f));
  joint->SetMaxForce (csVector3 (0.0f, 1.0f, 0.0f));
  joint->Attach (rb1, nullptr/* rb2, true*/);
#endif

#ifdef SLIDE
  // Create and position two rigid bodies
  // Already added to sector.
  CS::Physics::iRigidBody* rb1 = SpawnBox (false);
  csOrthoTransform trans = rb1->GetTransform ();
  csOrthoTransform jointTrans = trans;

  CS::Physics::iRigidBody* rb2 = SpawnBox (false);
  trans = rb2->GetTransform ();
  trans.SetOrigin (trans.GetOrigin () + trans.GetT2O () * csVector3 (-0.5f, 0.0f, 0.0f));
  rb2->SetTransform (trans);
  rb2->SetState (CS::Physics::STATE_STATIC);
  csRef<CS::Physics::iJointFactory> jointFactory =
    physicalSystem->CreateSlideJointFactory (-1.f, 1.f, 1.f, -1.f, 0);
  csRef<CS::Physics::iJoint> joint = jointFactory->CreateJoint ();
  joint->SetTransform (jointTrans);
  joint->Attach (rb2, rb1);
#endif

#ifdef SPRING
  CS::Physics::iRigidBody* rb1 = SpawnBox (false);
  csOrthoTransform jointTrans = rb1->GetTransform ();
  jointTrans.SetOrigin (jointTrans.GetOrigin () + jointTrans.GetT2O () * csVector3 (0.0f, 0.0f, 0.0f));

  CS::Physics::iRigidBody* rb2 = SpawnBox (false);
  csOrthoTransform trans = rb2->GetTransform ();
  trans.SetOrigin (trans.GetOrigin () + trans.GetT2O () * csVector3 (-1.0f, 0.0f, 0.0f));
  rb2->SetTransform (trans);
  rb2->SetState (CS::Physics::STATE_STATIC);

  csRef<CS::Physics::iJointFactory> jointFactory = physicalSystem->CreateJointFactory ();
  csRef<CS::Physics::iJoint> joint = jointFactory->CreateJoint ();
  joint->SetTransform (jointTrans);
  joint->SetSpring (true);
  joint->SetTransConstraints (true, true, true);
  joint->SetMinimumDistance (csVector3 (-1.0f, 0.0f, 0.0f));
  joint->SetMaximumDistance (csVector3 (1.0f, 0.0f, 0.0f));
  joint->SetLinearStiffness (csVector3 (9.478f, 0.0f, 0.0f));
  joint->SetLinearDamping (csVector3 (0.5f, 0.0f, 0.0f));
  joint->SetRotConstraints (true, true, true);
  joint->SetMinimumAngle (csVector3 (-PI/2.0f, 0.0f, 0.0f));
  joint->SetMaximumAngle (csVector3 (PI/2.0f, 0.0f, 0.0f));
  joint->SetAngularStiffness (csVector3 (9.478f, 0.0f, 0.0f));
  joint->SetAngularDamping (csVector3 (0.5f, 0.0f, 0.0f));
  joint->Attach (rb2, rb1);
#endif

#ifdef SOFT_LINEAR
  csRef<CS::Physics::iSoftBody> sb1 = SpawnSoftBody (false);
  csRef<CS::Physics::iSoftBody> bulletSoftBody = scfQueryInterface<CS::Physics::iSoftBody> (sb1);
  bulletSoftBody->ResetCollisionFlag ();
  // The soft body need to use cluster collision.
  bulletSoftBody->SetClusterCollisionRS (true);
  bulletSoftBody->SetClusterCollisionSS (true);
  bulletSoftBody->GenerateCluster (32);
  
  //CS::Physics::iRigidBody* rb2 = SpawnBox (false);
  //csOrthoTransform trans = rb2->GetTransform ();
  //trans.SetOrigin (trans.GetOrigin () + trans.GetT2O () * csVector3 (0.0f,2.0f,0.0f));
  //rb2->SetTransform (trans);
  //rb2->SetLinearDamping (0.8f);
  //rb2->SetAngularDamping (0.8f);

  //Or you can attach it with a softbody.
  CS::Physics::iSoftBody* rb2 = SpawnSoftBody (false);
  // SetTransform can only be called once in iSoftBody. And this is a local transform.
  csOrthoTransform trans = rb2->GetTransform ();
  trans.SetOrigin (trans.GetOrigin () + csVector3 (0.0f,1.0f,-0.5f));
  rb2->SetTransform (trans);
  bulletSoftBody = scfQueryInterface<CS::Physics::iSoftBody> (rb2);
  bulletSoftBody->ResetCollisionFlag ();
  bulletSoftBody->SetClusterCollisionRS (true);
  bulletSoftBody->SetClusterCollisionSS (true);
  bulletSoftBody->GenerateCluster (32);

  //Use local position when it's soft joint.
  csRef<CS::Physics::iJointFactory> jointFactory = 
    physicalSystem->CreateSoftLinearJointFactory ();
  csRef<CS::Physics::iJoint> joint = jointFactory->CreateJoint ();
  joint->SetPosition (rb2->GetVertexPosition (0));
  joint->Attach (sb1, rb2);
#endif

#ifdef SOFT_ANGULAR
  csRef<CS::Physics::iSoftBody> sb1 = SpawnSoftBody (false);
  csRef<CS::Physics::iSoftBody> bulletSoftBody = scfQueryInterface<CS::Physics::iSoftBody> (sb1);
  bulletSoftBody->ResetCollisionFlag ();
  bulletSoftBody->SetClusterCollisionRS (true);
  bulletSoftBody->SetClusterCollisionSS (true);
  bulletSoftBody->GenerateCluster (16);

  CS::Physics::iRigidBody* rb2 = SpawnBox (false);
  csOrthoTransform trans = rb2->GetTransform ();
  trans.SetOrigin (trans.GetOrigin () + trans.GetT2O () * csVector3 (0.0f,2.0f,0.0f));
  rb2->SetTransform (trans);
  rb2->SetLinearDamping (0.8f);
  rb2->SetAngularDamping (0.8f);

  csRef<CS::Physics::iJointFactory> jointFactory =
    physicalSystem->CreateSoftAngularJointFactory (1);
  csRef<CS::Physics::iJoint> joint = jointFactory->CreateJoint ();
  joint->Attach (sb1, rb2);
#endif

  //joint->RebuildJoint ();
  GetCurrentSector ()->AddJoint (joint);
  return joint;
}

CS::Physics::iRigidBody* PhysDemo::SpawnChainLink ()
{
  csVector3 extents (0.1f, 0.105f, 0.02f);
  RenderMeshColliderPair pair;
  pair.MeshFactory = engine->FindMeshFactory ("chainlinkFact");
  pair.Collider = csRef<iColliderBox> (physicalSystem->CreateColliderBox (extents));

  // Create the mesh.
  csRef<iMeshWrapper> mesh = engine->CreateMeshWrapper (pair.MeshFactory, "chainlink");

  // Create the body
  csRef<iRigidBodyFactory> factory = physicalSystem->CreateRigidBodyFactory (pair.Collider);
  factory->SetDensity (DefaultDensity);
  factory->SetElasticity (DefaultElasticity);
  factory->SetFriction (DefaultFriction);

  csRef<CS::Physics::iRigidBody> body = factory->CreateRigidBody ();
  body->SetAttachedSceneNode (mesh->QuerySceneNode ());
  body->QueryObject ()->SetObjectParent (mesh->QueryObject ());
  
  GetCurrentSector ()->AddCollisionObject (body);
  
  return body;
}

void PhysDemo::SpawnChain ()
{
  csVector3 initPos = view->GetCamera ()->GetTransform ().GetOrigin () + 1.5f * GetCameraDirection ();
  CS::Physics::iRigidBody* rb1 = SpawnChainLink ();
  
  csOrthoTransform trans = rb1->GetTransform ();
  trans.SetOrigin (initPos);
  rb1->SetTransform (trans);
  rb1->SetState (CS::Physics::STATE_STATIC);

  csVector3 offset (0.0f, 0.095f, 0.0f);

  CS::Physics::iRigidBody* rb2 = SpawnChainLink ();
  trans.SetO2T (csYRotMatrix3 (PI / 2.0f));
  trans.SetOrigin (initPos - offset);
  rb2->SetTransform (trans);

  CS::Physics::iRigidBody* rb3 = SpawnChainLink ();
  trans.Identity ();
  trans.SetOrigin (initPos - 2.0f * offset);
  rb3->SetTransform (trans);

  CS::Physics::iRigidBody* rb4 = SpawnChainLink ();
  trans.SetO2T (csYRotMatrix3 (PI / 2.0f));
  trans.SetOrigin (initPos - 3.0f * offset);
  rb4->SetTransform (trans);

  CS::Physics::iRigidBody* rb5 = SpawnChainLink ();
  trans.Identity ();
  trans.SetOrigin (initPos - 4.0f * offset);
  rb5->SetTransform (trans);

  // Create the joint factory and its constraints
  csRef<CS::Physics::iJointFactory> jointFactory = physicalSystem->CreateJointFactory ();

  // If the axis is constrained. You should set it with axis and pass min and max value to it.
  // Min = max means full constraint; min < max means move in the range. 
  // The translations are fully constrained.
  jointFactory->SetTransConstraints (true, true, true);
  jointFactory->SetMinimumDistance (csVector3 (0, 0, 0));
  jointFactory->SetMaximumDistance (csVector3 (0, 0, 0));
    jointFactory->SetSpring (false);

  // The rotations are bounded
  jointFactory->SetRotConstraints (true, true, true);
  jointFactory->SetMinimumAngle (csVector3 (-PI/4.0f, -PI/6.0f, -PI/6.0f));
  jointFactory->SetMaximumAngle (csVector3 (PI/4.0f, PI/6.0f, PI/6.0f));

  // Create the joints and attach the bodies to them.
  csOrthoTransform jointTransform;
  csRef<CS::Physics::iJoint> joint;
  
  csVector3 jointOffset (0.0f, 0.04f, 0.0f);

  joint = jointFactory->CreateJoint ();
  jointTransform.SetO2T (csZRotMatrix3 (PI * .5f));
  jointTransform.SetOrigin (initPos - jointOffset);
  joint->SetTransform (jointTransform);
  joint->Attach (rb1, rb2, false);
  joint->RebuildJoint ();
  GetCurrentSector ()->AddJoint (joint);

  joint = jointFactory->CreateJoint ();
  jointTransform.SetO2T (csZRotMatrix3 (PI * .5f));
  jointTransform.SetOrigin (initPos - jointOffset - offset);
  joint->SetTransform (jointTransform);
  joint->Attach (rb2, rb3, false);
  joint->RebuildJoint ();
  GetCurrentSector ()->AddJoint (joint);

  joint = jointFactory->CreateJoint ();
  jointTransform.SetO2T (csZRotMatrix3 (PI * .5f));
  jointTransform.SetOrigin (initPos - jointOffset - 2.0f * offset);
  joint->SetTransform (jointTransform);
  joint->Attach (rb3, rb4, false);
  joint->RebuildJoint ();
  GetCurrentSector ()->AddJoint (joint);

  joint = jointFactory->CreateJoint ();
  jointTransform.SetO2T (csZRotMatrix3 (PI * .5f));
  jointTransform.SetOrigin (initPos - jointOffset - 3.0f * offset);
  joint->SetTransform (jointTransform);
  joint->Attach (rb4, rb5, false);
  joint->RebuildJoint ();
  GetCurrentSector ()->AddJoint (joint);
}

void PhysDemo::SpawnParticles ()
{
  // Compute the position of the particles
  iSector* sector = view->GetCamera ()->GetSector ();
  float height = ActorDimensions.y;

  csVector3 pos = GetPointInFrontOfFeetXZ (2 * height);
  csVector3 origin = pos + csVector3 (0, height, 0);
  //csVector3 origin = cam->GetTransform ().GetOrigin () + csVector3 (0, ActorDimensions.y, 2);

  // Load the texture
  const char* materialName = "fire";
  iMaterialWrapper* mat = engine->GetMaterialList ()->FindByName (materialName);
  if (!mat)
  {
    ReportError ("Could not find the material %s!", CS::Quote::Single (materialName));
    return;
  }

  // Create the particle mesh
  csRef<iMeshFactoryWrapper> meshFactory =
    engine->CreateMeshFactory ("crystalspace.mesh.object.particles", "physical");
  if (!meshFactory)
  {
    ReportError ("Could not create the particle factory!");
    return;
  }

  csRef<iMeshWrapper> mesh = engine->CreateMeshWrapper (meshFactory, "physical", sector, origin);
  mesh->SetZBufMode (CS_ZBUF_TEST);
  mesh->SetRenderPriority (engine->GetRenderPriority ("transp"));
  mesh->GetMeshObject ()->SetMaterialWrapper (mat);

  // Setup the particle system
  csRef<iParticleSystem> partstate = scfQueryInterface<iParticleSystem> (mesh->GetMeshObject ());
  partstate->SetParticleSize (csVector2 (0.04f, 0.08f));

  // Create the emitters and effectors
  csRef<iParticleBuiltinEmitterFactory> emitterFactory = 
      csLoadPluginCheck<iParticleBuiltinEmitterFactory> (
        GetObjectRegistry (), "crystalspace.mesh.object.particles.emitter", false);
  csRef<iParticleBuiltinEffectorFactory> effectorFactory = 
      csLoadPluginCheck<iParticleBuiltinEffectorFactory> (
        GetObjectRegistry (), "crystalspace.mesh.object.particles.effector", false);

  csRef<iParticleBuiltinEmitterSphere> sphemit = emitterFactory->CreateSphere ();
  float velocity = 0.4f;
  float secondsToLive = 8.0f;
  float verticalFactor = 2.0f;
  sphemit->SetRadius (.2f);
  sphemit->SetParticlePlacement (CS_PARTICLE_BUILTIN_VOLUME);
  sphemit->SetEmissionRate (256.0f / secondsToLive);
  sphemit->SetInitialMass (5.0f, 7.5f);
  sphemit->SetUniformVelocity (true);
  sphemit->SetInitialTTL (secondsToLive, secondsToLive);
  sphemit->SetInitialVelocity (csVector3 (0, verticalFactor * velocity, 0), csVector3 (0));
  partstate->AddEmitter (sphemit);

  csRef<iParticleBuiltinEffectorPhysical> physicalEffector = effectorFactory->CreatePhysical ();
  physicalEffector->SetRandomAcceleration (csVector3 (1.5f, verticalFactor * 1.5f, 1.5f));
  partstate->AddEffector (physicalEffector);
}

void PhysDemo::LoadFrankieRagdoll ()
{
  // Load animesh factory
  printf ("Loading the Frankie model... "); fflush (stdout);
  csLoadResult rc = loader->Load ("/lib/frankie/frankie.xml");
  if (!rc.success)
  {
    ReportError ("Can't load Frankie!");
    return;
  }
  printf ("Done\n");

  csRef<iMeshFactoryWrapper> meshfact = engine->FindMeshFactory ("franky_frankie");
  if (!meshfact)
  {
    ReportError ("Can't find Frankie's mesh factory!");
    return;
  }

  csRef<CS::Mesh::iAnimatedMeshFactory> animeshFactory =
    scfQueryInterface<CS::Mesh::iAnimatedMeshFactory>
    (meshfact->GetMeshObjectFactory ());
  if (!animeshFactory)
  {
    ReportError ("Can't find Frankie's animesh factory!");
    return;
  }

  // Load the skeleton model (i.e. the animesh's physical properties)
  rc = loader->Load ("/lib/frankie/skelfrankie_model.xml");
  if (!rc.success)
  {
    ReportError ("Can't load Frankie's skeleton model description!");
    return;
  }

  CS::Animation::iSkeletonModel* skeletonModel =
    animeshFactory->GetSkeletonFactory ()->GetSkeletonModel ();
  if (!skeletonModel)
  {
    ReportError ("Can't find Frankie's skeleton model description!");
    return;
  }

  // Create the bone chain
  CS::Animation::iSkeletonChain* chain = skeletonModel->CreateChain
    ("body_chain", animeshFactory->GetSkeletonFactory ()->FindBone ("Frankie_Main"));
  chain->AddSubChain (animeshFactory->GetSkeletonFactory ()->FindBone ("CTRL_Head"));
  chain->AddSubChain (animeshFactory->GetSkeletonFactory ()->FindBone ("Tail_8"));

  // Create the ragdoll animation node factory
  csRef<CS::Animation::iSkeletonRagdollNodeFactory2> ragdollFactory =
    ragdollManager->CreateAnimNodeFactory ("frankie_ragdoll");
  ragdollFactory->AddChain (chain, CS::Animation::STATE_DYNAMIC);

  // Set the ragdoll animation node as the only node of the animation tree
  animeshFactory->GetSkeletonFactory ()->GetAnimationPacket ()
    ->SetAnimationRoot (ragdollFactory);
}

void PhysDemo::SpawnFrankieRagdoll ()
{
  // Load Frankie's factory if not yet done
  csRef<iMeshFactoryWrapper> meshfact =
    engine->FindMeshFactory ("franky_frankie");
  if (!meshfact)
  {
    LoadFrankieRagdoll ();
    meshfact = engine->FindMeshFactory ("franky_frankie");
  }

  if (!meshfact)
    return;

  // Create the animesh
  const csOrthoTransform& tc = view->GetCamera ()->GetTransform ();
  csOrthoTransform position =
    csOrthoTransform (csMatrix3 (), csVector3 (0.0f, 0.0f, 1.0f)) * tc;
  csRef<iMeshWrapper> ragdollMesh =
    engine->CreateMeshWrapper (meshfact, "Frankie", room, position.GetOrigin ());

  // Set the initial position of the body
  ragdollMesh->QuerySceneNode ()->GetMovable ()->SetFullTransform (position);

  // Close the eyes of Frankie as he is dead
  csRef<CS::Mesh::iAnimatedMeshFactory> animeshFactory =
    scfQueryInterface<CS::Mesh::iAnimatedMeshFactory>
    (meshfact->GetMeshObjectFactory ());
  csRef<CS::Mesh::iAnimatedMesh> animesh =
    scfQueryInterface<CS::Mesh::iAnimatedMesh> (ragdollMesh->GetMeshObject ());
  animesh->SetMorphTargetWeight (animeshFactory->FindMorphTarget ("eyelids_closed"), 0.7f);

  // Start the ragdoll animation node so that the rigid bodies of the bones are created
  // directly.
  CS::Animation::iSkeletonAnimNode* root = animesh->GetSkeleton ()->GetAnimationPacket ()->
    GetAnimationRoot ();
  csRef<CS::Animation::iSkeletonRagdollNode2> ragdoll =
    scfQueryInterfaceSafe<CS::Animation::iSkeletonRagdollNode2> (root);
  ragdoll->Play ();

  // Fling the body (that's why we needed the rigid bodies to be created directly).
  for (uint i = 0; i < ragdoll->GetBoneCount (CS::Animation::STATE_DYNAMIC); i++)
  {
    CS::Animation::BoneID boneID = ragdoll->GetBone (CS::Animation::STATE_DYNAMIC, i);
    CS::Physics::iRigidBody* rb = ragdoll->GetBoneRigidBody (boneID);
    rb->SetLinearVelocity (tc.GetT2O () * csVector3 (0, 0, 5));
    rb->SetAngularVelocity (tc.GetT2O () * csVector3 (5, 5, 0));
  }
}

void PhysDemo::LoadKrystalRagdoll ()
{
  // Load animesh factory
  printf ("Loading the Krystal model... "); fflush (stdout);
  csLoadResult rc = loader->Load ("/lib/krystal/krystal.xml");
  if (!rc.success)
  {
    ReportError ("Can't load Krystal library file!");
    return;
  }
  printf ("Done\n");

  csRef<iMeshFactoryWrapper> meshfact = engine->FindMeshFactory ("krystal");
  if (!meshfact)
  {
    ReportError ("Can't find Krystal's mesh factory!");
    return;
  }

  csRef<CS::Mesh::iAnimatedMeshFactory> animeshFactory =
    scfQueryInterface<CS::Mesh::iAnimatedMeshFactory>
    (meshfact->GetMeshObjectFactory ());
  if (!animeshFactory)
  {
    ReportError ("Can't find Krystal's animesh factory!");
    return;
  }

  // Load the skeleton model (i.e. the animesh's physical properties)
  rc = loader->Load ("/lib/krystal/skelkrystal_model.xml");
  if (!rc.success)
  {
    ReportError ("Can't load Krystal's skeleton model description!");
    return;
  }

  CS::Animation::iSkeletonModel* skeletonModel =
    animeshFactory->GetSkeletonFactory ()->GetSkeletonModel ();
  if (!skeletonModel)
  {
    ReportError ("Can't find Krystal's skeleton model description!");
    return;
  }
/*
  // Generate automatically a skeleton model
  csRef<CS::Animation::iSkeletonModel> skeletonModel =
    modelManager->CreateModel (animeshFactory->GetSkeletonFactory ());
  skeletonModel->PopulateDefaultModels (animeshFactory);
  animeshFactory->GetSkeletonFactory ()->SetSkeletonModel (skeletonModel);
*/
  // Create the bone chain
  CS::Animation::iSkeletonChain* chain = skeletonModel->CreateChain
    ("skeleton_chain", animeshFactory->GetSkeletonFactory ()->FindBone ("Hips"));
  chain->AddSubChain (animeshFactory->GetSkeletonFactory ()->FindBone ("Head"));
  chain->AddSubChain (animeshFactory->GetSkeletonFactory ()->FindBone ("RightFoot"));
  chain->AddSubChain (animeshFactory->GetSkeletonFactory ()->FindBone ("LeftFoot"));
  chain->AddSubChain (animeshFactory->GetSkeletonFactory ()->FindBone ("RightHand"));
  chain->AddSubChain (animeshFactory->GetSkeletonFactory ()->FindBone ("LeftHand"));

  // Create the ragdoll animation node factory
  csRef<CS::Animation::iSkeletonRagdollNodeFactory2> ragdollFactory =
    ragdollManager->CreateAnimNodeFactory ("krystal_ragdoll");
  ragdollFactory->AddChain (chain, CS::Animation::STATE_DYNAMIC);

  // Set the ragdoll animation node as the only node of the animation tree
  animeshFactory->GetSkeletonFactory ()->GetAnimationPacket ()
    ->SetAnimationRoot (ragdollFactory);
}


void PhysDemo::SpawnKrystalRagdoll ()
{
  // Load Krystal's factory if not yet done
  csRef<iMeshFactoryWrapper> meshfact = engine->FindMeshFactory ("krystal");
  if (!meshfact)
  {
    LoadKrystalRagdoll ();
    meshfact = engine->FindMeshFactory ("krystal");
  }

  if (!meshfact)
    return;

  // Create the animesh
  const csOrthoTransform& tc = view->GetCamera ()->GetTransform ();
  csOrthoTransform position =
    csOrthoTransform (csMatrix3 (), csVector3 (0.0f, -1.0f, 1.0f)) * tc;
  csRef<iMeshWrapper> ragdollMesh =
    engine->CreateMeshWrapper (meshfact, "Krystal", room, position.GetOrigin ());

  // Set the initial position of the body
  ragdollMesh->QuerySceneNode ()->GetMovable ()->SetFullTransform (position);

  // Start the ragdoll animation node so that the rigid bodies of the bones are created
  // directly.
  csRef<CS::Mesh::iAnimatedMesh> animesh =
    scfQueryInterface<CS::Mesh::iAnimatedMesh> (ragdollMesh->GetMeshObject ());
  CS::Animation::iSkeletonAnimNode* root = animesh->GetSkeleton ()->GetAnimationPacket ()->
    GetAnimationRoot ();
  csRef<CS::Animation::iSkeletonRagdollNode2> ragdoll =
    scfQueryInterfaceSafe<CS::Animation::iSkeletonRagdollNode2> (root);
  ragdoll->Play ();

  // Fling the body (that's why we needed the rigid bodies to be created directly).
  for (size_t i = 0; i < ragdoll->GetBoneCount (CS::Animation::STATE_DYNAMIC); i++)
  {
    CS::Animation::BoneID boneID = ragdoll->GetBone (CS::Animation::STATE_DYNAMIC, i);
    CS::Physics::iRigidBody* rb = ragdoll->GetBoneRigidBody (boneID);
    rb->SetLinearVelocity (tc.GetT2O () * csVector3 (0, 0, 5));
    rb->SetAngularVelocity (tc.GetT2O () * csVector3 (5, 5, 0));
  }
}

void PhysDemo::SpawnRope ()
{
  // Use the camera transform.
  const csOrthoTransform& tc = view->GetCamera ()->GetTransform ();

  // Spawn a box
  csRef<CS::Physics::iRigidBody> box = SpawnBox ();

  // Spawn a first rope and attach it to the box
  csRef<CS::Physics::iSoftRopeFactory> factory = physicalSystem->CreateSoftRopeFactory ();
  factory->SetStart (csVector3 (-2.f, 2.f, 0.f));
  factory->SetEnd (csVector3 (-0.15f, 0.f, 0.f));
  factory->SetNodeCount (20);
  factory->SetMass (2.0f);
  factory->SetLinearStiffness (0.95f);

  csRef<CS::Physics::iSoftBody> body = factory->CreateSoftBody ();

  csOrthoTransform trans;
  trans.SetO2T (tc.GetO2T ());
  trans.SetOrigin (tc.GetOrigin () + tc.GetT2O () * csVector3 (0.f, 0.f, 1.6f));
  body->SetTransform (trans);
  GetCurrentSector ()->AddCollisionObject (body);

  // Anchoring must be done once inside a collision sector
  body->AnchorVertex (0);
  body->AnchorVertex (body->GetVertexCount () - 1, box);

  // Spawn a second rope and attach it to the box
  body = factory->CreateSoftBody ();
  body->SetLinearStiffness (0.95f);

  trans.SetO2T (csYRotMatrix3 (PI) * tc.GetO2T ());
  body->SetTransform (trans);
  GetCurrentSector ()->AddCollisionObject (body);

  // Anchoring must be done once inside a collision sector
  body->AnchorVertex (0);
  body->AnchorVertex (body->GetVertexCount () - 1, box);
}

CS::Physics::iSoftBody* PhysDemo::SpawnCloth ()
{
  // Use the camera transform.
  const csOrthoTransform& tc = view->GetCamera ()->GetTransform ();

  // Create the cloth
  size_t segmentsH = 10;
  size_t segmentsV = 10;
  csRef<CS::Physics::iSoftClothFactory> factory = physicalSystem->CreateSoftClothFactory ();
  factory->SetCorners (csVector3 (0, 3, 1),
		       csVector3 (2, 3, 1),
		       csVector3 (0, 0, 1),
		       csVector3 (2, 0, 1));
  factory->SetSegmentCounts (segmentsH, segmentsV);
  factory->SetDiagonals (true);
  factory->SetMass (5.0f);

  csRef<iSoftBody> body = factory->CreateSoftBody ();

  // Attach the two top corners
  body->AnchorVertex (0);
  body->AnchorVertex (segmentsH - 1);

  // Create the cloth mesh factory
  csRef<iMeshFactoryWrapper> clothFact =
    CS::Physics::SoftBodyHelper::CreateClothGenMeshFactory
    (GetObjectRegistry (), "clothFact", body, csVector2 (0.3f, 0.5f));
  csRef<iGeneralFactoryState> gmstate = scfQueryInterface<iGeneralFactoryState>
    (clothFact->GetMeshObjectFactory ());

  // Create the mesh
  gmstate->SetAnimationControlFactory (softBodyAnimationFactory);
  csRef<iMeshWrapper> mesh (engine->CreateMeshWrapper (
    clothFact, "cloth_body"));
  iMaterialWrapper* mat = engine->GetMaterialList ()->FindByName ("flag");  
  if (!mat)
  {
    ReportError ("Could not find the material %s!", CS::Quote::Single ("flag"));
  }
  mesh->GetMeshObject ()->SetMaterialWrapper (mat);

  body->SetAttachedSceneNode (mesh->QuerySceneNode ());
  body->QueryObject ()->SetObjectParent (mesh->QueryObject ());

  csOrthoTransform trans;
  trans.SetO2T (tc.GetO2T ());
  trans.SetOrigin (tc.GetOrigin () + tc.GetT2O () * csVector3 (0, 0, 1));
  body->SetTransform (trans);
  GetCurrentSector ()->AddCollisionObject (body);

  // Init the animation control for the animation of the genmesh
  // Since this mesh has been created through a iSoftClothFactory, then this is
  // a double-sided mesh, with the vertices duplicated contiguously. We hence need
  // to setup the animation controller with the correct vertex duplication mode.
  csRef<iGeneralMeshState> meshState =
    scfQueryInterface<iGeneralMeshState> (mesh->GetMeshObject ());
  csRef<CS::Animation::iSoftBodyAnimationControl2> animationControl =
    scfQueryInterface<CS::Animation::iSoftBodyAnimationControl2> (meshState->GetAnimationControl ());
  animationControl->SetSoftBody (body, CS::Physics::MESH_DUPLICATION_CONTIGUOUS);

  return body;
}

CS::Physics::iSoftBody* PhysDemo::SpawnSoftBody (bool setVelocity /* = true */)
{
  // Create a ball mesh factory.
  csRef<iMeshFactoryWrapper> ballFact = engine->CreateMeshFactory (
    "crystalspace.mesh.object.genmesh", "ballFact");
  if (!ballFact)
  {
    ReportError ("Error creating mesh object factory!");
    return nullptr;
  }

  csRef<iGeneralFactoryState> gmstate = scfQueryInterface<
    iGeneralFactoryState> (ballFact->GetMeshObjectFactory ());
  const float r = 0.4f;
  csVector3 radius (r, r, r);
  csEllipsoid ellips (csVector3 (0), radius);
  gmstate->GenerateSphere (ellips, 16);

  // Use the camera transform.
  const csOrthoTransform& tc = view->GetCamera ()->GetTransform ();

  // Create the soft body from the shape of the ball mesh
  csRef<iSoftMeshFactory> factory = physicalSystem->CreateSoftMeshFactory ();
  factory->SetMesh (collisionHelper.FindCollisionMesh (ballFact));
  factory->SetMass (20.0f);
  factory->SetLinearStiffness (0.8f);
  factory->GenerateBendingConstraints (2.0f);

  csRef<CS::Physics::iSoftBody> body = factory->CreateSoftBody ();
  
  if (setVelocity)
  {
    // Fling the body.
    body->QueryPhysicalBody ()->SetLinearVelocity (tc.GetT2O () * csVector3 (0, 0, 5));

    // This would have worked too
    //for (size_t i = 0; i < body->GetVertexCount (); i++)
    //  body->SetLinearVelocity (tc.GetT2O () * csVector3 (0, 0, 5), i);
  }

  // Create the mesh and its 'soft body' animation controller
  gmstate->SetAnimationControlFactory (softBodyAnimationFactory);
  csRef<iMeshWrapper> mesh (engine->CreateMeshWrapper (ballFact, "soft_body"));
  iMaterialWrapper* mat = engine->GetMaterialList ()->FindByName ("objtexture");
  mesh->GetMeshObject ()->SetMaterialWrapper (mat);

  body->SetAttachedSceneNode (mesh->QuerySceneNode ());
  body->QueryObject ()->SetObjectParent (mesh->QueryObject ());
  //body->RebuildObject ();
  
  csOrthoTransform trans;
  trans.SetOrigin (tc.GetOrigin () + tc.GetT2O () * csVector3 (0, 0, 1));
  body->SetTransform (trans);

  GetCurrentSector ()->AddCollisionObject (body);

  return body;
}

void PhysDemo::SpawnBoxStacks (int stackNum, int stackHeight, float boxLen, float mass)
{
  // Place stacks of boxes
  // Stacks are horizontally aligned with the viewing direction

  static const float anchorDist = 2;                  // distance from pos to stack area
  static const float hSpacingFactor = 0.9f;           // how much of the box length is to be left as space between boxes horizontally
  static const float vSpacingFactor = 0.01f;          // how much of the box length is to be left as space between boxes vertically

  // position & direction
  csVector3 pos = GetCameraPosition (); GetPointOnGroundBeneathPos (pos, pos);    // move to ground
  csVector3 dir = GetCameraDirection ();
  
  csVector2 pos2 (pos.x, pos.z);
  csVector2 dir2 (dir.x, dir.z);
  csVector2 dirOrth2 = dir2;
  dirOrth2.Rotate (HALF_PI);
  
  float hspace = hSpacingFactor * boxLen;                          // horizontal spacing between boxes
  float dist = boxLen + hspace;                                   // horizontal distance between two neighboring stacks
  csVector2 hdistDir = dist * dir2;                               // horizontal stack distance in dir
  csVector2 hdistOrth = dist * dirOrth2;                          // horizontal stack distance orthogonal to dir
  
  int numDir = int (sqrt (float (stackNum))) + 0.99999f;             // amount of stacks in dir direction
  int numOrth = int (stackNum / numDir + 1);                       // amount of stacks in orth direction

  float halfWidth = .5f * (numOrth - 1) * (boxLen + hspace);      // half the width of the OBB that covers all box centers
  
  csVector3 extents (boxLen);                                      // box size
  csVector2 anchor = pos2 + anchorDist * dir2;                    // the closest point of the stack area from pos
  csVector2 boxPos2 (anchor - halfWidth * dirOrth2);               // position of the first box

  int n = 0;

  // prepare collider and render mesh
  csRef<CS::Collisions::iCollider> collider = stackBoxMeshPair.Collider;
  if (!collider || 
    fabs (dynamic_cast<iColliderBox*>(&*collider)->GetBoxGeometry ().x - boxLen) > EPSILON)
  {
    CreateBoxMeshColliderPair (stackBoxMeshPair, extents);
  }
  
  for (int x = 0; x < numOrth; ++x)
  {
    for (int z = 0; z < numDir && n < stackNum; ++z)
    {
      csVector3 boxPos (boxPos2.x, pos[UpAxis], boxPos2.y);
      boxPos += (.5f * (1 + vSpacingFactor) * boxLen) * UpVector;
      for (int i = 0; i < stackHeight; ++i)
      {
        SpawnRigidBody (stackBoxMeshPair, boxPos, DefaultFriction, DefaultDensity, false);
        boxPos += ((1 + vSpacingFactor) * boxLen) * UpVector;
      }
      ++n;
      boxPos2 += hdistDir;
    }
    boxPos2 += hdistOrth - numDir * hdistDir;
  }  
}
