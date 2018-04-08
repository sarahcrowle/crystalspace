/*
  Copyright (C) 2009-2012 Christian Van Brussel, Institute of Information
      and Communication Technologies, Electronics and Applied Mathematics
      at Universite catholique de Louvain, Belgium
      http://www.uclouvain.be/en-icteam.html

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
#include "csgeom/transfrm.h"
#include "csgeom/plane3.h"
#include "csutil/stringquote.h"
#include "iengine/mesh.h"
#include "iengine/movable.h"
#include "iengine/scenenode.h"
#include "iengine/sector.h"
#include "imesh/animesh.h"
#include "imesh/object.h"
#include "ivaria/reporter.h"

#include "ragdoll2.h"

CS_PLUGIN_NAMESPACE_BEGIN (Ragdoll2)
{
  SCF_IMPLEMENT_FACTORY (Ragdoll2NodeManager);

  void Ragdoll2NodeManager::Report (int severity, const char* msg, ...) const
  {
    va_list arg;
    va_start (arg, msg);
    csReportV (object_reg, severity, 
	       "crystalspace.mesh.animesh.animnode.ragdoll2",
	       msg, arg);
    va_end (arg);
  }

  // ------------------------------------------------------------------------

  size_t GetChainIndex (csArray<ChainData>& chains, CS::Animation::iSkeletonChain*& chain)
  {
    size_t index = 0;
    for (csArray<ChainData>::Iterator it = chains.GetIterator (); it.HasNext (); index++)
    {
      ChainData& chainData = it.Next ();
      if (chainData.chain == chain)
	return index;
    }
    return (size_t) ~0;
  }

  size_t GetChainIndex (const csArray<ChainData>& chains, CS::Animation::iSkeletonChain*& chain)
  {
    size_t index = 0;
    for (csArray<ChainData>::ConstIterator it = chains.GetIterator (); it.HasNext (); index++)
    {
      const ChainData& chainData = it.Next ();
      if (chainData.chain == chain)
	return index;
    }
    return (size_t) ~0;
  }

  // --------------------------  RagdollNodeFactory  --------------------------

  CS_LEAKGUARD_IMPLEMENT (RagdollNodeFactory);

  RagdollNodeFactory::RagdollNodeFactory (Ragdoll2NodeManager* manager, const char *name)
    : scfImplementationType (this), CS::Animation::SkeletonAnimNodeFactorySingle (name),
    manager (manager)
  {
    // Find the physical system from the object registry
    csRef<CS::Collisions::iCollisionSystem> collisionSystem =
      csQueryRegistry<CS::Collisions::iCollisionSystem> (manager->object_reg);
    if (!collisionSystem)
    {
      manager->Report (CS_REPORTER_SEVERITY_ERROR,
		       "Could not find any collision system for the creation of the "
		       "physical factories");
      return;
    }

    physicalSystem = scfQueryInterface<CS::Physics::iPhysicalSystem> (collisionSystem);
    if (!physicalSystem)
    {
      manager->Report (CS_REPORTER_SEVERITY_ERROR,
		       "Could not find any physical system for the creation of the "
		       "physical factories");
      return;
    }
  }

  void RagdollNodeFactory::AddChain (CS::Animation::iSkeletonChain* chain,
				     CS::Animation::RagdollState state)
  {
    ChainData data;
    data.chain = chain;
    data.state = state;
    chains.Push (data);
  }

  void RagdollNodeFactory::RemoveChain (CS::Animation::iSkeletonChain* chain)
  {
    size_t index = GetChainIndex (chains, chain);
    if (index != (size_t) ~0)
      chains.DeleteIndexFast (index);
  }

  csPtr<CS::Animation::SkeletonAnimNodeSingleBase> RagdollNodeFactory::ActualCreateInstance (
    CS::Animation::iSkeletonAnimPacket* packet,
    CS::Animation::iSkeleton* skeleton)
  {
    return csPtr<CS::Animation::SkeletonAnimNodeSingleBase>
      (new RagdollNode (this, skeleton));
  }

  // --------------------------  RagdollNode  --------------------------

  CS_LEAKGUARD_IMPLEMENT (RagdollNode);
/*
  CS::Animation::iSkeletonModel* FindSkeletonModel (CS::Mesh::iAnimatedMesh* mesh)// const
  {
    csRef<iMeshFactoryWrapper> factory = scfQueryInterface<iMeshFactoryWrapper>
      (mesh->GetAnimatedMeshFactory ());
    for (csRef<iObjectIterator> it = factory->QueryObject ()->GetIterator (); it->HasNext (); )
    {
      iObject* object = it->Next ();
      csRef<CS::Animation::iSkeletonModel> model =
	scfQueryInterface<CS::Animation::iSkeletonModel> (object);
      if (model) return model;
    }

    return nullptr;
  }
*/
  RagdollNode::RagdollNode (RagdollNodeFactory* factory, 
			    CS::Animation::iSkeleton* skeleton)
    : scfImplementationType (this),
    CS::Animation::SkeletonAnimNodeSingle<RagdollNodeFactory> (factory, skeleton),
    sceneNode (nullptr), maxBoneID (0)
  {
    // Find the physical model of the skeleton
    //skeletonModel = FindSkeletonModel (skeleton->GetAnimatedMesh ());
    skeletonModel = skeleton->GetFactory ()->GetSkeletonModel ();
#ifdef CS_DEBUG
    if (!skeletonModel)
    {
      factory->manager->Report (CS_REPORTER_SEVERITY_ERROR,
		       "Could not find any physical model for the skeleton");
      return;
    }
#endif

    // Copy the skeleton chains from the factory
    for (csArray<ChainData>::Iterator it = factory->chains.GetIterator (); it.HasNext (); )
    {
      ChainData& chainData = it.Next ();
      chains.Push (chainData);

      // Create an entry for each bones of the chain
      CreateBoneData (chainData.chain->GetRootNode (), chainData.state);
    }
  }

  RagdollNode::~RagdollNode ()
  {
    Stop ();
  }

  void RagdollNode::CreateBoneData (CS::Animation::iSkeletonChainNode* chainNode,
				    CS::Animation::RagdollState state)
  {
    // TODO: Set the iObject of the mesh as the parent of the one of the collision object.
    //collisionObject->QueryObject ()->SetObjectParent (mesh->QueryObject ());

    CS::Animation::iSkeletonBoneModel* boneModel =
      skeletonModel->FindBoneModel (chainNode->GetAnimeshBone ());
#ifdef CS_DEBUG
    if (!boneModel)
    {
      factory->manager->Report
	(CS_REPORTER_SEVERITY_ERROR,
	 "Could not find the bone model for the bone %s. "
	 "Adding child bones to this one may potentially segfault!",
	 CS::Quote::Single (skeleton->GetFactory ()->GetBoneName (chainNode->GetAnimeshBone ())));
      return;
    }

    if (!boneModel->GetRigidBodyFactory ()
	|| !boneModel->GetRigidBodyFactory ()->GetCollider ())
    {
      factory->manager->Report
	(CS_REPORTER_SEVERITY_ERROR,
	 "Could not find any valid rigid body factory for the bone %s! "
	 "Activating this bone may potentially segfault!",
	 CS::Quote::Single (skeleton->GetFactory ()->GetBoneName (chainNode->GetAnimeshBone ())));
    }
#endif

    // Check if the bone is already defined
    if (!bones.Contains (boneModel->GetAnimeshBone ()))
    {
      // Create the bone reference
      BoneData boneData;
      boneData.boneID = boneModel->GetAnimeshBone ();
      boneData.state = state;
      bones.Put (boneData.boneID, boneData);

      // Update the maximum bone ID
      maxBoneID = csMax (maxBoneID, boneData.boneID);
    }

    // Update the state of the children nodes
    for (uint i = 0; i < chainNode->GetChildCount (); i++)
      CreateBoneData (chainNode->GetChild (i), state);
  }

  void RagdollNode::SetChainState (CS::Animation::iSkeletonChain* chain,
				   CS::Animation::RagdollState state)
  {
    size_t index = GetChainIndex (chains, chain);
#ifdef CS_DEBUG
    // Check that the chain is registered
    if (index == (size_t) ~0)
    {
      factory->manager->Report (CS_REPORTER_SEVERITY_WARNING,
       "Chain %s was not registered in the ragdoll plugin while trying to set new state",
				chain->GetName ());
      return;
    }
#endif

    // TODO: if dynamic then check that there is sth to link to

    // Set the new chain state
    chains[index].state = state;
    SetChainNodeState (chain->GetRootNode (), state);
  }

  void RagdollNode::SetChainNodeState (CS::Animation::iSkeletonChainNode* node,
				       CS::Animation::RagdollState state)
  {
    // Find the associated bone data
    BoneData nullBone;
    BoneData& boneData = bones.Get (node->GetAnimeshBone (), nullBone);
    boneData.state = state;

    // Update the state of the bone if this node is playing
    if (isPlaying)
      UpdateBoneState (&boneData);

    // Update the state of the children nodes
    for (uint i = 0; i < node->GetChildCount (); i++)
      SetChainNodeState (node->GetChild (i), state);
  }

  CS::Animation::RagdollState RagdollNode::GetChainState
    (CS::Animation::iSkeletonChain* chain) const
  {
    size_t index = GetChainIndex (chains, chain);
    if (index == (size_t) ~0)
      return CS::Animation::STATE_INACTIVE;

    return chains[index].state;
  }

  CS::Physics::iRigidBody* RagdollNode::GetBoneRigidBody (CS::Animation::BoneID bone)
  {
    if (!bones.Contains (bone))
      return nullptr;

    return bones[bone]->rigidBody;
  }

  CS::Physics::iJoint* RagdollNode::GetBoneJoint (const CS::Animation::BoneID bone)
  {
    if (!bones.Contains (bone))
      return nullptr;

    return bones[bone]->joint;
  }

  uint RagdollNode::GetBoneCount (CS::Animation::RagdollState state) const
  {
    uint count = 0;
    for (csHash<BoneData, CS::Animation::BoneID>::ConstGlobalIterator it = bones.GetIterator ();
	 it.HasNext (); )
    {
      const BoneData& boneData = it.Next ();

      if (boneData.state == state)
	count++;
    }

    return count;
  }

  CS::Animation::BoneID RagdollNode::GetBone (CS::Animation::RagdollState state, uint index) const
  {
    uint count = 0;
    for (csHash<BoneData, CS::Animation::BoneID>::ConstGlobalIterator it = bones.GetIterator ();
	 it.HasNext (); )
    {
      const BoneData& boneData = it.Next ();

      if (boneData.state == state)
      {
	if (count == index)
	  return boneData.boneID;

	count++;
      }
    }

    return CS::Animation::InvalidBoneID;
  }

  CS::Animation::BoneID RagdollNode::FindBone (CS::Physics::iRigidBody* body) const
  {
    for (csHash<BoneData, CS::Animation::BoneID>::ConstGlobalIterator it = bones.GetIterator ();
	 it.HasNext (); )
    {
      const BoneData& boneData = it.Next ();

      if (boneData.rigidBody == body)
	return boneData.boneID;
    }

    return CS::Animation::InvalidBoneID;
  }

  void RagdollNode::ResetChainTransform (CS::Animation::iSkeletonChain* chain)
  {
#ifdef CS_DEBUG
    // Check that the chain is registered
    size_t index = GetChainIndex (chains, chain);
    if (index == (size_t) ~0)
    {
      factory->manager->Report (CS_REPORTER_SEVERITY_WARNING,
       "Chain %s was not registered in the ragdoll plugin while trying to reset the chain transform",
				chain->GetName ());
      return;
    }

    // Check that the chain is in dynamic state
    if (chains[index].state != CS::Animation::STATE_DYNAMIC)
    {
      factory->manager->Report (CS_REPORTER_SEVERITY_WARNING,
       "Chain %s was not in dynamic state while trying to reset the chain transform",
				chain->GetName ());
      return;
    }
#endif

    // Schedule for the reset (it cannot be made right now since the skeleton state
    // may not yet be in a new state, eg if the user has just changed a chain to dynamic
    // state)
    ResetChainData chainData;
    chainData.chain = chain;
    chainData.frameCount = 2; // The skeleton may not be in a good state before 2 frames
    resetChains.Put (0, chainData);
  }

  void RagdollNode::Play ()
  {
    if (isPlaying)
      return;
    isPlaying = true;

    // Find the scene node of the animesh
    if (!sceneNode)
      sceneNode = skeleton->GetSceneNode ();

    if (factory->physicalSystem)
    {
      // Find the physical sector associated to the sector where the animesh is
      iSector* sector = sceneNode->GetMovable ()->GetSectors ()->Get (0);
      CS::Collisions::iCollisionSector* collisionSector =
	factory->physicalSystem->FindCollisionSector (sector);
      if (!collisionSector)
	collisionSector = factory->physicalSystem->CreateCollisionSector (sector);
      physicalSector = collisionSector->QueryPhysicalSector ();
      // TODO: let the user add the mesh to a sector later

      // Init the bone states
      if (physicalSector)
	InitBoneStates ();
    }

    // Start the child node
    if (childNode)
      childNode->Play ();
  }

  void RagdollNode::Stop ()
  {
    if (!isPlaying)
      return;

    isPlaying = false;

    // Reset the reference to the skeleton model
    skeletonModel.Invalidate ();

    // Put back all bones in the 'inactive' state
    for (csHash<BoneData, CS::Animation::BoneID>::GlobalIterator it = bones.GetIterator ();
	 it.HasNext (); )
    {
      BoneData& bone = it.Next ();
      UpdateBoneState (&bone);
    }

    // Stop the child node
    if (childNode)
      childNode->Stop ();
  }

  void RagdollNode::BlendState (CS::Animation::AnimatedMeshState* state, float baseWeight)
  {
    // TODO: use baseWeight

    // Check that this node is active
    if (!isPlaying || !physicalSector)
      return;

    // Make the child node blend the state
    if (childNode)
      childNode->BlendState (state, baseWeight);

    // Reset the chains that have been asked for
    for (int i = ((int) resetChains.GetSize ()) - 1; i >= 0; i--)
    {
      ResetChainData &chainData = resetChains.Get (i);
      chainData.frameCount--;

      if (chainData.frameCount == 0)
      {
        ResetChainNodeTransform (chainData.chain->GetRootNode ());
        resetChains.DeleteIndex (i);
      }
    }

    // Update each bones
    bool hasNaN = false;
    for (csHash<BoneData, CS::Animation::BoneID>::GlobalIterator it = bones.GetIterator ();
      it.HasNext (); )
    {
      BoneData& boneData = it.Next ();

      // TODO: test for deactivation of rigid body

      // Check if the bone is in dynamic state
      if (boneData.state != CS::Animation::STATE_DYNAMIC)
        continue;

      // Get the bind transform of the bone
      csQuaternion skeletonRotation;
      csVector3 skeletonOffset;
      skeleton->GetFactory ()->GetTransformBoneSpace (boneData.boneID, skeletonRotation,
						      skeletonOffset);

      csOrthoTransform bodyTransform = boneData.rigidBody->GetTransform ();
      if (CS::IsNaN (bodyTransform.GetOrigin ()[0]))
      {
	hasNaN = true;
	continue;
      }

      CS::Animation::BoneID parentBoneID = skeleton->GetFactory ()->GetBoneParent (boneData.boneID);

      // If this bone is the root of the skeleton
      if (parentBoneID == CS::Animation::InvalidBoneID)
      {
	// Compute the new bone transform
	csReversibleTransform relativeTransform =
	  bodyTransform * sceneNode->GetMovable ()->GetFullTransform ().GetInverse ();

	// Apply the new transform to the CS::Animation::AnimatedMeshState
	state->SetBoneUsed (boneData.boneID);
	state->GetVector (boneData.boneID) = relativeTransform.GetOrigin () - skeletonOffset;
	csQuaternion quaternion;
	quaternion.SetMatrix (relativeTransform.GetT2O ());
	state->GetQuaternion (boneData.boneID) = quaternion * skeletonRotation.GetConjugate ();

	continue;
      }
      
      // If this bone is inside the ragdoll chain
      if (bones.Contains (parentBoneID))
      {
	// Compute the new bone transform
	BoneData nullBone;
	csOrthoTransform parentTransform =
	  bones.Get (parentBoneID, nullBone).rigidBody->GetTransform ();
	csReversibleTransform relativeTransform =
	  bodyTransform * parentTransform.GetInverse ();

	// Apply the new transform to the CS::Animation::AnimatedMeshState
	state->SetBoneUsed (boneData.boneID);
	state->GetVector (boneData.boneID) = relativeTransform.GetOrigin () - skeletonOffset;
	csQuaternion quaternion;
	quaternion.SetMatrix (relativeTransform.GetT2O ());
	state->GetQuaternion (boneData.boneID) = quaternion * skeletonRotation.GetConjugate ();

	continue;
      }

      // Else this bone is the root of the ragdoll chain, but not of the skeleton
      else
      {
	// TODO: the transform should be read from the AnimatedMeshState
	csQuaternion boneRotation;
	csVector3 boneOffset;
	skeleton->GetTransformAbsSpace (parentBoneID, boneRotation, boneOffset);
	csOrthoTransform parentTransform (csMatrix3 (boneRotation.GetConjugate ()), boneOffset);
	parentTransform = parentTransform * sceneNode->GetMovable ()->GetFullTransform ();

	// Compute the new bone transform
	csReversibleTransform relativeTransform =
	  bodyTransform * parentTransform.GetInverse ();

	// Apply the new transform to the CS::Animation::AnimatedMeshState
	state->SetBoneUsed (boneData.boneID);
	state->GetVector (boneData.boneID) = relativeTransform.GetOrigin () - skeletonOffset;
	csQuaternion quaternion;
	quaternion.SetMatrix (relativeTransform.GetT2O ());
	state->GetQuaternion (boneData.boneID) = quaternion * skeletonRotation.GetConjugate ();

	continue;
      }
    }

    // Report any NaN values found
#ifdef CS_DEBUG
    if (hasNaN)
      factory->manager->Report
	(CS_REPORTER_SEVERITY_WARNING,
	 "NaN values found while updating the ragdoll's rigid bodies");
#endif
  }

  void RagdollNode::TickAnimation (float dt)
  {
    // Tick the child node
    if (childNode)
      childNode->TickAnimation (dt);

    // Update of the position of the mesh

    // Search for the list of bones that have a direct influence on the position
    // of the mesh (ie the bones that have no colliders or bounding boxes in any of
    // their parents).
    csArray<BoneData*> rootBones;
    for (size_t i = 0; i < chains.GetSize (); i++)
    {
      const ChainData& chainData = chains.Get (i);
      if (chainData.state != CS::Animation::STATE_DYNAMIC)
	continue;

      // Check if there is any collider or bounding box in one of the parent
      CS::Animation::BoneID boneID = skeleton->GetFactory ()->GetBoneParent
	(chainData.chain->GetRootNode ()->GetAnimeshBone ());
      while (boneID != CS::Animation::InvalidBoneID)
      {
	if (!skeleton->GetAnimatedMesh ()->GetBoneBoundingBox (boneID).Empty ()
	    || bones.Contains (boneID))
	  break;

	boneID = skeleton->GetFactory ()->GetBoneParent (boneID);
      }

      // Add this bone to the list
      if (boneID == CS::Animation::InvalidBoneID)
	rootBones.Push (bones.GetElementPointer
			(chainData.chain->GetRootNode ()->GetAnimeshBone ()));
    }

    // If no bone influence the position of the mesh then don't do anything
    if (!rootBones.GetSize ())
      return;

    // If there is only one bone influencing the position of the mesh
    if (rootBones.GetSize () == 1)
    {
      // Compute the new transform of the mesh
      csOrthoTransform bodyTransform = rootBones[0]->rigidBody->GetTransform ();
      csQuaternion boneRotation;
      csVector3 boneOffset;
      skeleton->GetTransformAbsSpace (rootBones[0]->boneID, boneRotation, boneOffset);
      csOrthoTransform boneTransform (csMatrix3 (boneRotation.GetConjugate ()), boneOffset);
      csOrthoTransform newTransform = boneTransform.GetInverse () * bodyTransform;
      
      // Apply the new transform to the iMovable of the animesh
      iMovable* movable = sceneNode->GetMovable ();
      movable->SetFullTransform (newTransform);
      movable->UpdateMove ();
    }

    // If there are more than one bone influencing the position of the mesh
    else
    {
      // Compute the mean origin of all bones
      csVector3 origin (0.0f);
      for (size_t i = 0; i < rootBones.GetSize (); i++)
      {
	csOrthoTransform bodyTransform = rootBones[i]->rigidBody->GetTransform ();
	csQuaternion boneRotation;
	csVector3 boneOffset;
	skeleton->GetTransformAbsSpace (rootBones[i]->boneID, boneRotation, boneOffset);
	csOrthoTransform boneTransform (csMatrix3 (boneRotation.GetConjugate ()), boneOffset);
	csOrthoTransform newTransform = boneTransform.GetInverse () * bodyTransform;
	origin += newTransform.GetOrigin ();
      }
      origin /= rootBones.GetSize ();

      // Apply the new origin to the iMovable of the animesh
      iMovable* movable = sceneNode->GetMovable ();
      csTransform transform (csMatrix3 (), origin);
      movable->SetFullTransform (transform);
      movable->UpdateMove ();
    }
  }

  void RagdollNode::UpdateBoneState (BoneData* boneData)
  {
    // Check if this node has been stopped or if the bone is inactive
    if (!isPlaying
	|| boneData->state == CS::Animation::STATE_INACTIVE)
    {
      if (boneData->joint)
      {
	physicalSector->RemoveJoint (boneData->joint);
	boneData->joint = nullptr;
      }

      if (boneData->rigidBody)
      {
	physicalSector->RemoveCollisionObject (boneData->rigidBody);
	boneData->rigidBody = nullptr;
      }

      return;
    }

    CS::Animation::iSkeletonBoneModel* boneModel = skeletonModel->FindBoneModel (boneData->boneID);

    // Create the rigid body if not yet done
    bool previousBody = true;
    if (!boneData->rigidBody)
    {
      previousBody = false;

      // Check the availability of the physical properties
      if (!boneModel->GetRigidBodyFactory ())
      {
	factory->manager->Report
	  (CS_REPORTER_SEVERITY_ERROR,
	   "No rigid body factory defined for bone %i while creating the body.",
	   boneModel->GetAnimeshBone ());
	return;
      }

      // Create the rigid body
      // TODO: force dynamic state?
      boneData->rigidBody = boneModel->GetRigidBodyFactory ()->CreateRigidBody ();

      // Set the transform of the body
      csQuaternion rotation;
      csVector3 offset;
      skeleton->GetTransformAbsSpace (boneData->boneID, rotation, offset);
      // TODO: we shouldn't have to use the conjugate of the quaternion, should we?
      csOrthoTransform boneTransform (csMatrix3 (rotation.GetConjugate ()), offset);
      csOrthoTransform animeshTransform = sceneNode->GetMovable ()->GetFullTransform ();
      csOrthoTransform bodyTransform = boneTransform * animeshTransform;
      boneData->rigidBody->SetTransform (bodyTransform);

      //boneData->rigidBody->RebuildObject ();
      physicalSector->AddCollisionObject (boneData->rigidBody);
    }

    // If the bone is in dynamic state
    if (boneData->state == CS::Animation::STATE_DYNAMIC)
    {
      // Set the rigid body in dynamic state
      boneData->rigidBody->SetState (CS::Physics::STATE_DYNAMIC);

      // If there was already a rigid body then update its position
      if (previousBody)
      {
	csQuaternion rotation;
	csVector3 offset;
	skeleton->GetTransformAbsSpace (boneData->boneID, rotation, offset);
	csOrthoTransform boneTransform (csMatrix3 (rotation.GetConjugate ()), offset);
	csOrthoTransform animeshTransform = sceneNode->GetMovable ()->GetFullTransform ();
	// TODO: remove RigidBodyTransform?
	csOrthoTransform bodyTransform = boneTransform * animeshTransform;
	boneData->rigidBody->SetTransform (bodyTransform);
      }

      // Prepare for adding a joint
      // Check if the joint has already been defined
      if (boneData->joint)
	return;

      // Check that a joint definition is present
      if (!boneModel->GetJointFactory ())
	return;

      // Check if there is a parent bone to attach a joint to
      CS::Animation::BoneID parentBoneID = skeleton->GetFactory ()->GetBoneParent (boneData->boneID);
      if (parentBoneID == CS::Animation::InvalidBoneID)
	return;

      // Check if a CS::Animation::iBodyBone has been defined for the parent bone
      const BoneData* parentBoneData = bones.GetElementPointer (parentBoneID);
      if (!parentBoneData)
	return;

      // Create the dynamic joint
      boneData->joint = boneModel->GetJointFactory ()->CreateJoint ();

      // Setup the transform of the joint 
      csQuaternion rotation; 
      csVector3 offset; 

      skeleton->GetTransformAbsSpace (boneData->boneID, rotation, offset); 
      csOrthoTransform boneTransform (csMatrix3 (rotation.GetConjugate ()), offset); 
      boneData->joint->SetTransform
	(boneModel->GetJointTransform () * boneTransform
	 * sceneNode->GetMovable ()->GetFullTransform ()); 

      // Attach the rigid bodies to the joint
      boneData->joint->Attach (parentBoneData->rigidBody, boneData->rigidBody, false);
      boneData->joint->RebuildJoint ();
      physicalSector->AddJoint (boneData->joint);

      return;
    }

    // If the bone is in kinematic state
    else if (boneData->state == CS::Animation::STATE_KINEMATIC)
    {
      // Create the kinematic callback controlling the motion of the bone
      csRef<BoneKinematicCallback> ref;
      ref.AttachNew (new BoneKinematicCallback (this, boneData->boneID));
      boneData->rigidBody->SetKinematicCallback (ref);

      // Set the rigid body in kinematic state
      boneData->rigidBody->SetState (CS::Physics::STATE_KINEMATIC);

      // Remove the joint
      if (boneData->joint)
      {
	physicalSector->RemoveJoint (boneData->joint);
	boneData->joint = nullptr;
      }
    }
  }

  void RagdollNode::InitBoneStates ()
  {
    // Update the state of all bones (iterate in increasing order of the bone ID's
    // so that the parent bones are always updated before their children)
    for (size_t i = 0; i <= maxBoneID; i++)
    {
      if (!bones.Contains ((CS::Animation::BoneID)i))
        continue;

      BoneData nullBone;
      BoneData& boneData = bones.Get ((CS::Animation::BoneID)i, nullBone);
      UpdateBoneState (&boneData);
    }
  }

  void RagdollNode::ResetChainNodeTransform (CS::Animation::iSkeletonChainNode* node)
  {
    // TODO: simply re-call UpdateSkeleton ()?

    // Find the associated bone data
    BoneData nullBone;
    BoneData& boneData = bones.Get (node->GetAnimeshBone (), nullBone);
    
    // Compute the bind transform of the bone
    csQuaternion boneRotation;
    csVector3 boneOffset;
    skeleton->GetFactory ()->GetTransformBoneSpace (boneData.boneID, boneRotation,
						   boneOffset);
    csOrthoTransform bodyTransform (csMatrix3 (boneRotation.GetConjugate ()),
				    boneOffset);

    CS::Animation::BoneID parentBoneID = skeleton->GetFactory ()->GetBoneParent (boneData.boneID);

    // If the parent bone is a rigid body then take the parent transform from it
    if (bones.Contains (parentBoneID))
    {
      csOrthoTransform parentTransform =
      bones.Get (parentBoneID, nullBone).rigidBody->GetTransform ();
      bodyTransform = bodyTransform * parentTransform;
    }

    // Else take the parent transform from the skeleton state
    else if (parentBoneID != CS::Animation::InvalidBoneID)
    {
      skeleton->GetTransformAbsSpace (parentBoneID, boneRotation,
				      boneOffset);
      csOrthoTransform parentTransform (csMatrix3 (boneRotation.GetConjugate ()),
					boneOffset);

      bodyTransform = bodyTransform * parentTransform
	* sceneNode->GetMovable ()->GetFullTransform ();
    }

    // Apply the transform to the rigid body
    boneData.rigidBody->SetTransform (bodyTransform);
    boneData.rigidBody->SetLinearVelocity (csVector3 (0.0f));
    boneData.rigidBody->SetAngularVelocity (csVector3 (0.0f));

    // Update the transform of the children nodes
    for (uint i = 0; i < node->GetChildCount (); i++)
      ResetChainNodeTransform (node->GetChild (i));
  }

  /********************
   *  BoneKinematicCallback
   ********************/

  BoneKinematicCallback::BoneKinematicCallback (RagdollNode* ragdollNode,
						CS::Animation::BoneID boneID)
    : scfImplementationType (this), ragdollNode (ragdollNode), boneID (boneID)
  {
  }

  BoneKinematicCallback::~BoneKinematicCallback ()
  {
  }

  void BoneKinematicCallback::GetBodyTransform
    (CS::Physics::iRigidBody* body, csOrthoTransform& transform) const
  {
    // Read the position of the kinematic body from the skeleton state
    csQuaternion boneRotation;
    csVector3 boneOffset;
    ragdollNode->skeleton->GetTransformAbsSpace (boneID, boneRotation, boneOffset);
    transform.SetO2T (csMatrix3 (boneRotation.GetConjugate ()));
    transform.SetOrigin (boneOffset);
    csOrthoTransform animeshTransform =
      ragdollNode->sceneNode->GetMovable ()->GetFullTransform ();
    transform = transform * animeshTransform;
  }

}
CS_PLUGIN_NAMESPACE_END (Ragdoll)
