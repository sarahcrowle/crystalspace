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
#include "csgeom/matrix3.h"
#include "csgeom/transfrm.h"
#include "csgeom/quaternion.h"
#include "csgeom/vector3.h"
#include "iengine/scenenode.h"
#include "iutil/strset.h"

// Bullet includes.
#include "csutil/custom_new_disable.h"
#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"
#include "BulletSoftBody/btSoftBody.h"
#include "BulletSoftBody/btSoftRigidDynamicsWorld.h"
#include "BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"
#include "csutil/custom_new_enable.h"

#include "softbody2.h"
#include "rigidbody2.h"

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

void csBulletSoftBody::CreateSoftBodyObject (BulletSoftBodyFactory* factory)
{
  CreatePhysicalBodyObject (factory);
  SetLinearStiffness (factory->linearStiffness);
  SetAngularStiffness (factory->angularStiffness);
  SetVolumeStiffness (factory->volumeStiffness);
  SetSRHardness (factory->SRHhardness);
  SetSKHardness (factory->SKHhardness);
  SetSSHardness (factory->SSHhardness);
  SetSRImpulse (factory->SRimpulse);
  SetSKImpulse (factory->SKimpulse);
  SetSSImpulse (factory->SSimpulse);
  SetDamping (factory->damping);
  SetDrag (factory->drag);
  SetLift (factory->lift);
  SetPressure (factory->pressure);
  SetVolumeConversationCoefficient (factory->conversation);
  SetShapeMatchThreshold (factory->matching);
  SetRContactsHardness (factory->Rhardness);
  SetKContactsHardness (factory->Khardness);
  SetSContactsHardness (factory->Shardness);
  SetAnchorsHardness (factory->anchorHardness);
  if (factory->match)
    SetShapeMatching (true);
  if (factory->generateBending)
    GenerateBendingConstraints (factory->bendingDistance);
  // TODO: mass?
}

csBulletSoftBody::csBulletSoftBody (csBulletSystem* phySys, btSoftBody* body)
  :scfImplementationType (this, phySys), btBody (body), anchorCount (0), gravityEnabled (true)
{
  btObject = body;
  btBody->setUserPointer (dynamic_cast<CS::Collisions::iCollisionObject*> (this));
}

csBulletSoftBody::~csBulletSoftBody ()
{
}

void csBulletSoftBody::RebuildObject ()
{
  btBody->setCollisionFlags (0);
}

CS::Collisions::HitBeamResult csBulletSoftBody::HitBeam (const csVector3& start, const csVector3& end)
{
  CS::Collisions::HitBeamResult result;
  btVector3 rayFrom = CSToBullet (start, system->GetInternalScale ());
  btVector3 rayTo = CSToBullet (end, system->GetInternalScale ());
  btSoftBody::sRayCast ray;

  btCollisionWorld::ClosestRayResultCallback rayCallback (rayFrom, rayTo);
  if (btBody->rayTest (rayFrom, rayTo, ray))
  {
    result.hasHit = true;
    result.object = this;
    result.isect = BulletToCS (rayCallback.m_hitPointWorld,
			       system->GetInverseInternalScale ());
    result.normal = BulletToCS (rayCallback.m_hitNormalWorld,
				system->GetInverseInternalScale ());	
    
    btVector3 impact = rayFrom + (rayTo - rayFrom) * ray.fraction;
    switch (ray.feature)
    {
    case btSoftBody::eFeature::Face:
    {
      btSoftBody::Face& face = btBody->m_faces[ray.index];
      btSoftBody::Node* node = face.m_n[0];
      float distance = (node->m_x - impact).length2 ();

        for (int i = 1; i < 3; i++)
        {
          float nodeDistance = (face.m_n[i]->m_x - impact).length2 ();
          if (nodeDistance < distance)
          {
            node = face.m_n[i];
            distance = nodeDistance;
          }
        }
        result.vertexIndex = (size_t) (node - &btBody->m_nodes[0]);
        break;
      }
    default:
      break;
    }
  }
  return result;
}

bool csBulletSoftBody::RemoveBulletObject ()
{
  if (insideWorld)
  {
    for (size_t i = 0; i < joints.GetSize (); i++)
    {
      sector->RemoveJoint (joints[i]);
    }
    btSoftRigidDynamicsWorld* softWorld = static_cast<btSoftRigidDynamicsWorld*> (sector->bulletWorld);
    softWorld->removeSoftBody (btBody);
    insideWorld = false;
    anchorCount = 0;
    animatedAnchors.DeleteAll ();
    sector->anchoredSoftBodies.Delete (this);
    for (int i = 0; i < btBody->m_anchors.size (); i++)
    {
      btRigidBody* btRB = btBody->m_anchors[i].m_body;
      CS::Collisions::iCollisionObject* collObject = static_cast<CS::Collisions::iCollisionObject*> (btRB->getUserPointer ());
      csBulletRigidBody* rb = dynamic_cast<csBulletRigidBody*> (collObject);
      rb->anchorCount -- ;
    }
    return true;
  }
  return false;
}

bool csBulletSoftBody::AddBulletObject ()
{
  if (!insideWorld)
  {
    btBody->m_worldInfo = sector->softWorldInfo;
    btSoftRigidDynamicsWorld* softWorld =
      static_cast<btSoftRigidDynamicsWorld*> (sector->bulletWorld);

    softWorld->addSoftBody (btBody, group->value, group->mask);
    btBody->setUserPointer (dynamic_cast<CS::Collisions::iCollisionObject*> (this));
    insideWorld = true;
  }
  return true;
}

void csBulletSoftBody::SetTransform (const csOrthoTransform& trans)
{
  CS_ASSERT (btObject);

  btTransform btTrans = CSToBullet (trans, system->GetInternalScale ());
  
  iSceneNode* sceneNode = GetAttachedSceneNode ();
  if (sceneNode)
  {
    sceneNode->GetMovable ()->SetFullTransform (trans);
    sceneNode->GetMovable ()->UpdateMove ();
  }

  if (camera)
  {
    camera->SetTransform (trans);
  }

  btTransform& worldTrans = btObject->getWorldTransform ();
  btBody->transform (worldTrans.inverse () * btTrans);           // transform relative to last
}

void csBulletSoftBody::SetMass (float mass)
{
  CS_ASSERT (btBody);
  btBody->setTotalMass (mass);
}

float csBulletSoftBody::GetMass () const
{
  CS_ASSERT (btBody);
  return btBody->getTotalMass ();
}

void csBulletSoftBody::SetDensity (float density)
{
  CS_ASSERT (btBody);
  this->density = density;
  btBody->setTotalDensity (density);
}

float csBulletSoftBody::GetVolume () const
{
  CS_ASSERT (btBody);
  return btBody->getVolume ();
}

void csBulletSoftBody::AddForce (const csVector3& force)
{
  CS_ASSERT (btBody);
  btBody->addForce (CSToBullet (force, system->GetInternalScale ()));
}

void csBulletSoftBody::AddForce (const csVector3& force, size_t vertexIndex)
{
  CS_ASSERT (vertexIndex < (size_t) btBody->m_nodes.size ());
  // TODO: Why a correction factor on the force?
  btVector3 correctedForce = CSToBullet (force / btBody->m_nodes[vertexIndex].m_im * 1000.f,
					 system->GetInternalScale ());
  btBody->addForce (correctedForce, int (vertexIndex));
}

void csBulletSoftBody::ApplyImpulse (const csVector3& impulse)
{
  CS_ASSERT (btBody);
  btBody->addForce (CSToBullet (impulse, system->GetInternalScale ()));
}

void csBulletSoftBody::ApplyImpulse (const csVector3& impulse, size_t vertexIndex)
{
  CS_ASSERT (vertexIndex < (size_t) btBody->m_nodes.size ());
  // TODO: Why a correction factor on the impulse?
  btVector3 correctedImpulse = CSToBullet (impulse / btBody->m_nodes[vertexIndex].m_im * 1000.f,
					   system->GetInternalScale ());
  btBody->addForce (correctedImpulse, int (vertexIndex));
}

void csBulletSoftBody::SetLinearVelocity (const csVector3& vel)
{
  CS_ASSERT (btBody);
  btBody->setVelocity (CSToBullet (vel, system->GetInternalScale ()));
}

csVector3 csBulletSoftBody::GetLinearVelocity () const
{
  CS_ASSERT (btBody);

  // Weighted sum of of all node velocities
  btVector3 vel = btVector3 (0, 0, 0);
  for (int i = 0; i < btBody->m_nodes.size (); ++i)
  {
    vel += btBody->m_nodes[i].m_v * btBody->getMass (i);
  }
  return BulletToCS (vel / GetMass (), system->GetInverseInternalScale ());
}

csVector3 csBulletSoftBody::GetLinearVelocity (size_t index /* = 0 */) const
{
  CS_ASSERT (btBody && index < (size_t) btBody->m_nodes.size ());

  return BulletToCS (btBody->m_nodes[int (index)].m_v, system->GetInverseInternalScale ());
}

void csBulletSoftBody::SetFriction (float friction)
{
  CS_ASSERT (btBody);
  CS_ASSERT (friction >= 0.0f && friction <= 1.0f);
  
  // dynamic friction
  btBody->m_cfg.kDF = friction;
}

void csBulletSoftBody::SetVertexMass (float mass, size_t index)
{
  CS_ASSERT (btBody);
  btBody->setMass (int (index), mass);
}

float csBulletSoftBody::GetVertexMass (size_t index)
{
  CS_ASSERT (btBody);
  return btBody->getMass (int (index));
}

size_t csBulletSoftBody::GetVertexCount ()
{
  CS_ASSERT (btBody);
  return btBody->m_nodes.size ();
}

csVector3 csBulletSoftBody::GetVertexPosition (size_t index) const
{
  CS_ASSERT (btBody && index < (size_t) btBody->m_nodes.size ());
  return BulletToCS (btBody->m_nodes[int (index)].m_x, system->GetInverseInternalScale ());
}

void csBulletSoftBody::AnchorVertex (size_t vertexIndex)
{
  CS_ASSERT (vertexIndex < (size_t) btBody->m_nodes.size ());
  btBody->setMass (int (vertexIndex), 0.0f);
  anchorCount ++;
}

void csBulletSoftBody::AnchorVertex (size_t vertexIndex, iRigidBody* body)
{
  csBulletRigidBody* rigidBody = dynamic_cast<csBulletRigidBody*> (body);
  CS_ASSERT (rigidBody
    && vertexIndex < (size_t) this->btBody->m_nodes.size ()
    && rigidBody->btBody);
  this->btBody->appendAnchor (int (vertexIndex), rigidBody->btBody);
  anchorCount ++;
  rigidBody->anchorCount ++;
}

void csBulletSoftBody::AnchorVertex (size_t vertexIndex,
                                     CS::Physics::iAnchorAnimationControl* controller)
{
  AnimatedAnchor anchor (vertexIndex, controller);
  animatedAnchors.Push (anchor);
  (sector->anchoredSoftBodies).Push (this);
  anchorCount ++;
}

void csBulletSoftBody::UpdateAnchor (size_t vertexIndex, csVector3& position)
{
  CS_ASSERT (vertexIndex < (size_t) btBody->m_nodes.size ());

  // Update the local position of the anchor
  for (int i = 0; i < this->btBody->m_anchors.size (); i++)
    if (this->btBody->m_anchors[i].m_node == &this->btBody->m_nodes[int (vertexIndex)])
    {
      this->btBody->m_anchors[i].m_local =
        this->btBody->m_anchors[i].m_body->getInterpolationWorldTransform ().inverse ()
        * CSToBullet (position, system->GetInternalScale ());
      return;
    }
}

void csBulletSoftBody::RemoveAnchor (size_t vertexIndex)
{
  CS_ASSERT (vertexIndex < (size_t) btBody->m_nodes.size ());

  // Check if it is a fixed anchor
  if (btBody->getMass (int (vertexIndex)) < SMALL_EPSILON)
  {
    btBody->setMass (int (vertexIndex), btBody->getTotalMass () / btBody->m_nodes.size ());
    anchorCount --;
    return;
  }

  // Check if it is an animated anchor
  size_t index = 0;
  for (csArray<AnimatedAnchor>::Iterator it = animatedAnchors.GetIterator (); it.HasNext (); index++)
  {
    AnimatedAnchor& anchor = it.Next ();
    if (anchor.vertexIndex == vertexIndex)
    {
      animatedAnchors.DeleteIndex (index);
      (sector->anchoredSoftBodies).Delete (this);
      anchorCount --;
      return;
    }
  }

  // Check if it is a simple 'rigid body' anchor
  for (int i = 0; i < this->btBody->m_anchors.size (); i++)
    if (this->btBody->m_anchors[i].m_node == &this->btBody->m_nodes[int (vertexIndex)])
    {
      // TODO: this is not possible within Bullet
      system->ReportWarning
	("Removal of anchor %zu is not possible within the Bullet API. Expect wrong behavior.", vertexIndex);
      //btSoftBody::Anchor* anchor = this->body->m_anchors[i];
      //this->body->m_anchors.remove (i);
      return;
    }
}

size_t csBulletSoftBody::GetTriangleCount ()
{
  CS_ASSERT (btBody);
  return btBody->m_faces.size ();
}

csTriangle csBulletSoftBody::GetTriangle (size_t index) const
{
  CS_ASSERT (index < (size_t) btBody->m_faces.size ());
  btSoftBody::Face& face = btBody->m_faces[int (index)];
  return csTriangle (face.m_n[0] - &btBody->m_nodes[0],
    face.m_n[1] - &btBody->m_nodes[0],
    face.m_n[2] - &btBody->m_nodes[0]);
}

csVector3 csBulletSoftBody::GetVertexNormal (size_t index) const
{
  CS_ASSERT (index < (size_t) btBody->m_nodes.size ());
  csVector3 normal (btBody->m_nodes[int (index)].m_n.getX (),
    btBody->m_nodes[int (index)].m_n.getY (),
    btBody->m_nodes[int (index)].m_n.getZ ());
  normal.Normalize ();
  return normal;
}

void csBulletSoftBody::SetLinearVelocity (size_t vertexIndex, const csVector3& velocity)
{
  CS_ASSERT (vertexIndex < (size_t) btBody->m_nodes.size ());
  btBody->addVelocity (CSToBullet (velocity, system->GetInternalScale ())
		       - btBody->m_nodes[int (vertexIndex)].m_v, int (vertexIndex));
}

void csBulletSoftBody::SetWindVelocity (const csVector3& velocity)
{
  CS_ASSERT (btBody);
  btVector3 velo = CSToBullet (velocity, system->GetInternalScale ());
  btBody->setWindVelocity (velo);
}

const csVector3 csBulletSoftBody::GetWindVelocity () const
{
  CS_ASSERT (btBody);
  csVector3 velo = BulletToCS (btBody->getWindVelocity (), system->GetInternalScale ());
  return velo;
}

void csBulletSoftBody::SetLinearStiffness (float stiffness)
{
  if (stiffness >= 0.0f && stiffness <= 1.0f)
  {
    btSoftBody::Material* pm = btBody->m_materials[0];
    pm->m_kLST = stiffness;
  }
}

void csBulletSoftBody::SetAngularStiffness (float stiffness)
{
  CS_ASSERT (btBody);
  if (stiffness >= 0.0f && stiffness <= 1.0f)
  {
    btSoftBody::Material* pm = btBody->m_materials[0];
    pm->m_kAST = stiffness;
  }
}

void csBulletSoftBody::SetVolumeStiffness (float stiffness)
{
  CS_ASSERT (btBody);
  if (stiffness >= 0.0f && stiffness <= 1.0f)
  {
    btSoftBody::Material* pm = btBody->m_materials[0];
    pm->m_kVST = stiffness;
  }
}

void csBulletSoftBody::ResetCollisionFlag ()
{
  CS_ASSERT (btBody);
  btBody->m_cfg.collisions = 0;
}

void csBulletSoftBody::SetClusterCollisionRS (bool cluster)
{
  CS_ASSERT (btBody);
  if (cluster)
    btBody->m_cfg.collisions += btSoftBody::fCollision::CL_RS;
  else
    btBody->m_cfg.collisions += btSoftBody::fCollision::SDF_RS;
}

bool csBulletSoftBody::GetClusterCollisionRS ()
{
  CS_ASSERT (btBody);
  if (btBody->m_cfg.collisions & btSoftBody::fCollision::CL_RS)
    return true;
  return false;
}

void csBulletSoftBody::SetClusterCollisionSS (bool cluster)
{
  CS_ASSERT (btBody);
  if (cluster)
    btBody->m_cfg.collisions += btSoftBody::fCollision::CL_SS;
  else
    btBody->m_cfg.collisions += btSoftBody::fCollision::VF_SS;
}

bool csBulletSoftBody::GetClusterCollisionSS ()
{
  CS_ASSERT (btBody);
  if (btBody->m_cfg.collisions & btSoftBody::fCollision::CL_SS)
    return true;
  return false;
}

void csBulletSoftBody::SetSRHardness (float hardness)
{
  CS_ASSERT (btBody);
  if (hardness >= 0.0f && hardness <= 1.0f)
    btBody->m_cfg.kSRHR_CL = hardness;
}

void csBulletSoftBody::SetSKHardness (float hardness)
{
  CS_ASSERT (btBody);
  if (hardness >= 0.0f && hardness <= 1.0f)
    btBody->m_cfg.kSKHR_CL = hardness;
}

void csBulletSoftBody::SetSSHardness (float hardness)
{
  CS_ASSERT (btBody);
  if (hardness >= 0.0f && hardness <= 1.0f)
    btBody->m_cfg.kSSHR_CL = hardness;
}

void csBulletSoftBody::SetSRImpulse (float impulse)
{
  CS_ASSERT (btBody);
  if (impulse >= 0.0f && impulse <= 1.0f)
    btBody->m_cfg.kSR_SPLT_CL = impulse;
}

void csBulletSoftBody::SetSKImpulse (float impulse)
{
  CS_ASSERT (btBody);
  if (impulse >= 0.0f && impulse <= 1.0f)
    btBody->m_cfg.kSK_SPLT_CL = impulse;
}

void csBulletSoftBody::SetSSImpulse (float impulse)
{
  CS_ASSERT (btBody);
  if (impulse >= 0.0f && impulse <= 1.0f)
    btBody->m_cfg.kSS_SPLT_CL = impulse;
}

void csBulletSoftBody::SetVeloCorrectionFactor (float factor)
{
  CS_ASSERT (btBody);
  btBody->m_cfg.kVCF = factor;
}

void csBulletSoftBody::SetDamping (float damping)
{
  CS_ASSERT (btBody);
  if (damping >= 0.0f && damping <= 1.0f)
    btBody->m_cfg.kDP = damping;
}

void csBulletSoftBody::SetDrag (float drag)
{
  CS_ASSERT (btBody);
  if (drag >= 0.0f)
    btBody->m_cfg.kDG = drag;
}

void csBulletSoftBody::SetLift (float lift)
{
  CS_ASSERT (btBody);
  if (lift >= 0.0f)
    btBody->m_cfg.kLF = lift;
}

void csBulletSoftBody::SetPressure (float pressure)
{
  CS_ASSERT (btBody);
  if (pressure >= 0.0f && pressure <= 1.0f)
    btBody->m_cfg.kPR = pressure;
}

void csBulletSoftBody::SetVolumeConversationCoefficient (float conversation)
{
  CS_ASSERT (btBody);
  btBody->m_cfg.kVC = conversation;
}

void csBulletSoftBody::SetShapeMatchThreshold (float matching)
{
  CS_ASSERT (btBody);
  if (matching >= 0.0f && matching <= 1.0f)
    btBody->m_cfg.kMT = matching;
}

void csBulletSoftBody::SetRContactsHardness (float hardness)
{
  CS_ASSERT (btBody);
  if (hardness >= 0.0f && hardness <= 1.0f)
    btBody->m_cfg.kCHR = hardness;
}

void csBulletSoftBody::SetKContactsHardness (float hardness)
{
  CS_ASSERT (btBody);
  if (hardness >= 0.0f && hardness <= 1.0f)
    btBody->m_cfg.kKHR = hardness;
}

void csBulletSoftBody::SetSContactsHardness (float hardness)
{
  CS_ASSERT (btBody);
  if (hardness >= 0.0f && hardness <= 1.0f)
    btBody->m_cfg.kSHR = hardness;
}

void csBulletSoftBody::SetAnchorsHardness (float hardness)
{
  CS_ASSERT (btBody);
  if (hardness >= 0.0f && hardness <= 1.0f)
    btBody->m_cfg.kAHR = hardness;
}

void csBulletSoftBody::SetVeloSolverIterations (int iter)
{
  CS_ASSERT (btBody);
  btBody->m_cfg.piterations = iter;
}

void csBulletSoftBody::SetPositionIterations (int iter)
{
  CS_ASSERT (btBody);
  btBody->m_cfg.viterations = iter;
}

void csBulletSoftBody::SetDriftIterations (int iter)
{
  CS_ASSERT (btBody);
  btBody->m_cfg.diterations = iter;
}

void csBulletSoftBody::SetClusterIterations (int iter)
{
  CS_ASSERT (btBody);
  btBody->m_cfg.citerations = iter;
}

void csBulletSoftBody::SetShapeMatching (bool match)
{
  CS_ASSERT (btBody);
  if (match)
    btBody->setPose (false,true);
  else
    btBody->setPose (true,false);
}

void csBulletSoftBody::GenerateBendingConstraints (size_t distance)
{
  CS_ASSERT (btBody);
  btBody->generateBendingConstraints (distance);
  btBody->randomizeConstraints ();
}

void csBulletSoftBody::GenerateCluster (int iter)
{
  CS_ASSERT (btBody);
  if (btBody->m_cfg.collisions & (btSoftBody::fCollision::CL_RS + btSoftBody::fCollision::CL_SS))
    btBody->generateClusters (iter);
}

size_t csBulletSoftBody::GetNodeCount () const 
{ 
  return size_t (btBody->m_nodes.size ()); 
}

void csBulletSoftBody::PreStep (float dt)
{
  CS_ASSERT (btBody);

  // Counter gravity effects (that will be applied during simulation)
  if (!gravityEnabled)
  {
    if (sector->GetGravity ().SquaredNorm () > 0)
    {
      // apply the exact opposite of gravity (since it cannot simply be disabled or set)
      // this is how gravity is applied in btSoftBody.cpp
      btVector3 minusGravity = -(btBody->m_cfg.timescale * system->GetWorldTimeStep ()) * btBody->m_worldInfo->m_gravity;
      btBody->addVelocity (minusGravity);
    }
  }

  // Update anchor positions
  for (csArray<AnimatedAnchor>::Iterator it = animatedAnchors.GetIterator (); it.HasNext (); )
  {
    AnimatedAnchor& anchor = it.Next ();
    anchor.position = CSToBullet (anchor.controller->GetAnchorPosition (), system->GetInternalScale ());
  }
}

void csBulletSoftBody::UpdateAnchorInternalTick (btScalar timeStep)
{
  CS_ASSERT (btBody);
  for (csArray<AnimatedAnchor>::Iterator it = animatedAnchors.GetIterator (); it.HasNext (); )
  {
    AnimatedAnchor& anchor = it.Next ();

    btVector3 delta = anchor.position - btBody->m_nodes[int (anchor.vertexIndex)].m_x;
    static const btScalar maxdrag = 10;
    if (delta.length2 () > maxdrag * maxdrag)
      delta = delta.normalized () * maxdrag;
    btBody->m_nodes[int (anchor.vertexIndex)].m_v += delta / timeStep;
  }  
}

void csBulletSoftBody::DebugDraw (iView* view)
{
  system->InitDebugDraw ();
  btSoftBodyHelpers::Draw (btBody, system->debugDraw);
  system->debugDraw->DebugDraw (view->GetContext (), view->GetCamera ());
}

}
CS_PLUGIN_NAMESPACE_END (Bullet2)
