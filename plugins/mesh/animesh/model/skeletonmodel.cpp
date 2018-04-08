/*
  Copyright (C) 2009-10 Christian Van Brussel, Institute of Information
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

#include "csutil/scf.h"
#include "csutil/stringquote.h"
#include "iengine/mesh.h"
#include "ivaria/physics.h"
#include "ivaria/reporter.h"

#include "skeletonmodel.h"

CS_PLUGIN_NAMESPACE_BEGIN (SkeletonModel)
{

  /********************
   *  ModelManager
   ********************/

  SCF_IMPLEMENT_FACTORY (ModelManager);

  CS_LEAKGUARD_IMPLEMENT (ModelManager);

  ModelManager::ModelManager (iBase* parent)
    : scfImplementationType (this, parent)
  {}

  bool ModelManager::Initialize (iObjectRegistry* object_reg)
  {
    this->object_reg = object_reg;
    return true;
  }

  csPtr<CS::Animation::iSkeletonModel> ModelManager::CreateModel
    (CS::Animation::iSkeletonFactory* skeletonFactory)
  {
    CS_ASSERT (skeletonFactory);
    return new SkeletonModel (this, skeletonFactory);
  }

  void ModelManager::Report (int severity, const char* msg, ...)
  {
    va_list arg;
    va_start (arg, msg);
    csRef<iReporter> rep (csQueryRegistry<iReporter> (object_reg));
    if (rep)
      rep->ReportV (severity, "crystalspace.mesh.animesh.model",
		    msg, arg);
    else
      {
	csPrintfV (msg, arg);
	csPrintf ("\n");
      }
    va_end (arg);
  }


  /********************
   *  SkeletonModel
   ********************/

  CS_LEAKGUARD_IMPLEMENT (SkeletonModel);

  SkeletonModel::SkeletonModel (ModelManager* manager,
				CS::Animation::iSkeletonFactory* skeletonFactory)
    : scfImplementationType (this), manager (manager), skeletonFactory (skeletonFactory)
  {}

  CS::Animation::iSkeletonFactory* SkeletonModel::GetSkeletonFactory () const
  {
    return skeletonFactory;
  }

  void SkeletonModel::ClearAll ()
  {
    boneHash.DeleteAll ();
    chainHash.DeleteAll ();
  }

  CS::Animation::iSkeletonBoneModel* SkeletonModel::CreateBoneModel (CS::Animation::BoneID boneID)
  {
    if (boneID == CS::Animation::InvalidBoneID)
    {
      manager->Report (CS_REPORTER_SEVERITY_ERROR,
		       "Invalid bone ID while creating bone model");
      return nullptr;
    }

    // Check that the bone id exists in the skeleton
    if (!skeletonFactory)
    {
      manager->Report (CS_REPORTER_SEVERITY_ERROR,
	       "No skeleton factory defined while creating bone model");
      return nullptr;
    }    

    if (!skeletonFactory->HasBone (boneID))
    {
      manager->Report (CS_REPORTER_SEVERITY_ERROR,
       "Bone %i doesn't exist in skeleton factory while creating bone model",
		       boneID);
      return nullptr;
    }    

    csRef<BoneModel> newFact;
    newFact.AttachNew (new BoneModel (boneID));

    return boneHash.PutUnique (boneID, newFact);
  }

  CS::Animation::iSkeletonBoneModel* SkeletonModel::FindBoneModel (const char *name) const
  {
    CS::Animation::BoneID boneID = skeletonFactory->FindBone (name);
    return boneHash.Get (boneID, 0);
  }

  CS::Animation::iSkeletonBoneModel* SkeletonModel::FindBoneModel (CS::Animation::BoneID bone) const
  {
    return boneHash.Get (bone, 0);
  }

  class BoneModelIterator : public scfImplementation1<BoneModelIterator, CS::Animation::iSkeletonBoneModelIterator>
  {
  private:
    csHash<csRef<BoneModel>, CS::Animation::BoneID>::ConstGlobalIterator it;

  public:
    BoneModelIterator (const csHash<csRef<BoneModel>, CS::Animation::BoneID>::ConstGlobalIterator& it) :
      scfImplementationType (this), it (it) { }
    virtual ~BoneModelIterator () { }
    virtual bool HasNext () const { return it.HasNext (); }
    virtual CS::Animation::iSkeletonBoneModel* Next () { return it.Next (); }
  };

  csPtr<CS::Animation::iSkeletonBoneModelIterator> SkeletonModel::GetBoneModels () const
  {
    return new BoneModelIterator (boneHash.GetIterator ());
  }

  void SkeletonModel::RemoveBoneModel (CS::Animation::BoneID bone)
  {
    boneHash.DeleteAll (bone);
  }

  void SkeletonModel::ClearBoneModels ()
  {
    boneHash.DeleteAll ();
  }

  CS::Animation::iSkeletonChain* SkeletonModel::CreateChain (const char *name,
							    CS::Animation::BoneID rootBone)
  {
    // check that the chain name has not already be given
    if (chainHash.Contains (name))
    {
      manager->Report (CS_REPORTER_SEVERITY_ERROR,
		       "Chain %s has already been defined.", name);
      return nullptr;
    }

    // check the validity of the root bone
    if (!skeletonFactory->HasBone (rootBone))
    {
      manager->Report (CS_REPORTER_SEVERITY_ERROR,
       "The root bone %i is not in the skeleton factory of the animesh",
		       rootBone);
      return nullptr;
    }

    // chain is OK, create it
    csRef<SkeletonChain> chain;
    chain.AttachNew (new SkeletonChain (this, name, rootBone));

    return chainHash.PutUnique (name, chain);
  }

  CS::Animation::iSkeletonChain* SkeletonModel::FindChain (const char *name) const
  {
    return chainHash.Get (name, 0);
  }

  class ChainIterator : public scfImplementation1<ChainIterator, CS::Animation::iSkeletonChainIterator>
  {
  private:
    csHash<csRef<CS::Animation::iSkeletonChain>, csString>::ConstGlobalIterator it;

  public:
    ChainIterator (const csHash<csRef<CS::Animation::iSkeletonChain>, csString>::ConstGlobalIterator& it) :
      scfImplementationType (this), it (it) { }
    virtual ~ChainIterator () { }
    virtual bool HasNext () const { return it.HasNext (); }
    virtual CS::Animation::iSkeletonChain* Next () { return it.Next (); }
  };

  csPtr<CS::Animation::iSkeletonChainIterator> SkeletonModel::GetChains () const
  {
    ChainIterator* iterator = new ChainIterator (chainHash.GetIterator ());
    return iterator;
  }

  void SkeletonModel::RemoveChain (const char* name)
  {
    chainHash.DeleteAll (name);
  }

  void SkeletonModel::ClearChains ()
  {
    chainHash.DeleteAll ();
  }

  void SkeletonModel::PopulateDefaultModels
    (const CS::Mesh::iAnimatedMeshFactory* animeshFactory,
     CS::Collisions::ColliderType colliderType)
  {
    // Check that the animesh uses the same skeleton factory
    if (skeletonFactory != animeshFactory->GetSkeletonFactory ())
    {
      manager->Report (CS_REPORTER_SEVERITY_ERROR,
		       "Creation of the default chains failed: the skeleton factory of "
		       "this chain is different than the one of the animesh");
      return;
    }

    // Find the physical system
    csRef<CS::Collisions::iCollisionSystem> collisionSystem =
      csQueryRegistry<CS::Collisions::iCollisionSystem> (manager->object_reg);
    if (!collisionSystem)
    {
      manager->Report (CS_REPORTER_SEVERITY_ERROR,
		       "Could not find any collision system for the creation of the "
		       "physical factories");
      return;
    }

    csRef<CS::Physics::iPhysicalSystem> physicalSystem =
      scfQueryInterface<CS::Physics::iPhysicalSystem> (collisionSystem);
    if (!physicalSystem)
    {
      manager->Report (CS_REPORTER_SEVERITY_ERROR,
		       "Could not find any physical system for the creation of the "
		       "physical factories");
      return;
    }

    // Check the type of the collider
#ifdef CS_DEBUG
    switch (colliderType)
    {
    case CS::Collisions::COLLIDER_BOX:
    case CS::Collisions::COLLIDER_SPHERE:
    case CS::Collisions::COLLIDER_CYLINDER:
    case CS::Collisions::COLLIDER_CAPSULE:
      break;
    default:
      manager->Report (CS_REPORTER_SEVERITY_WARNING,
		       "Unsupported collision type");
      return;
    }
#endif

    const csArray<CS::Animation::BoneID> &bones = skeletonFactory->GetBoneOrderList ();

    // Check that there are any bones in the skeleton
    if (!bones.GetSize ())
      return;

    // Iterate on all bones
    for (size_t i = 0; i < bones.GetSize (); i++)
    {
      const CS::Animation::BoneID &boneID = bones.Get (i);

      // Check that the bounding box is not empty
      const csBox3 &bbox = animeshFactory->GetBoneBoundingBox (boneID);
      if (bbox.Empty ()) continue;

      // Find or create the bone model
      BoneModel* bone;
      if (!boneHash.Contains (boneID))
      {
	csRef<BoneModel> newFact;
	newFact.AttachNew (new BoneModel (boneID));
	bone = boneHash.PutUnique (boneID, newFact);
      }

      else bone = *boneHash.GetElementPointer (boneID);

      // Create a default physical model if there are none previously defined
      if (!bone->rigidBody)
      {
	csRef<CS::Collisions::iCollider> collider;
	csOrthoTransform transform;

	switch (colliderType)
	{
	case CS::Collisions::COLLIDER_BOX:
	{
	  csRef<CS::Collisions::iColliderBox> box =
	    physicalSystem->CreateColliderBox (bbox.GetSize ());
	  collider = box;
	  transform.SetOrigin (bbox.GetCenter ());
	}
	  break;
	case CS::Collisions::COLLIDER_SPHERE:
	{
	  csRef<CS::Collisions::iColliderSphere> sphere =
	    physicalSystem->CreateColliderSphere (bbox.GetSize ().Norm () * 0.4f);
	  collider = sphere;
	  transform.SetOrigin (bbox.GetCenter ());
	}
	  break;
	case CS::Collisions::COLLIDER_CYLINDER:
	{
	  const csVector3 size = bbox.GetSize ();
	  csRef<CS::Collisions::iColliderCylinder> cylinder =
	    physicalSystem->CreateColliderCylinder
	    (size[2], csQsqrt (size[0] * size[0] + size[1] * size[1]) * 0.4f);
	  collider = cylinder;
	  transform.SetOrigin (bbox.GetCenter ());
	}
	  break;
	case CS::Collisions::COLLIDER_CAPSULE:
	{
	  const csVector3 size = bbox.GetSize ();
	  float radius = csQsqrt (size[0] * size[0] + size[1] * size[1]) * 0.4f;
	  csRef<CS::Collisions::iColliderCapsule> capsule =
	    physicalSystem->CreateColliderCapsule (size[2] - radius, radius);
	  collider = capsule;
	  transform.SetOrigin (bbox.GetCenter ());
	}
	  break;
	default:
	  break;
	}

	bone->rigidBody = physicalSystem->CreateRigidBodyFactory (collider);
      }

      // Create a default joint if it has not been previously defined
      // and if there is a valid collider for the parent bone
      if (!bone->joint)
      {
	CS::Animation::BoneID parentBoneID = skeletonFactory->GetBoneParent (boneID);
	if (parentBoneID == CS::Animation::InvalidBoneID)
	  continue;

	if (!boneHash.Contains (parentBoneID))
	  continue;

	BoneModel* parentBone = *boneHash.GetElementPointer (parentBoneID);
	if (!parentBone->rigidBody)
	  continue;

	bone->joint = physicalSystem->CreateP2PJointFactory ();
	bone->joint->SetMinimumAngle (csVector3 (-PI * 0.25f));
	bone->joint->SetMaximumAngle (csVector3 (PI * 0.25f));
	//bone->joint->SetAngularDamping (csVector3 (0.1f));
      }
    }
  }

  void SkeletonModel::PopulateDefaultChains ()
  {
    const csArray<CS::Animation::BoneID> &bones = skeletonFactory->GetBoneOrderList ();

    // Check that there are any bones in the skeleton
    if (!bones.GetSize ())
      return;

    // Start the recursion on all root bones
    for (size_t i = 0; i < bones.GetSize (); i++)
    {
      const CS::Animation::BoneID &boneID = bones.Get (i);

      if (skeletonFactory->GetBoneParent (boneID) == CS::Animation::InvalidBoneID)
	PopulateDefaultChainNode (nullptr, bones, i);
    }
  }

  void SkeletonModel::PopulateDefaultChainNode
    (SkeletonChainNode* parentNode,
     const csArray<CS::Animation::BoneID> &bones,
     size_t index)
  {
    const CS::Animation::BoneID &boneID = bones.Get (index);

    // Check if there is a rigid body defined for this bone
    csRef<SkeletonChainNode> node;
    if (boneHash.Contains (boneID)
	&& (*boneHash.GetElementPointer (boneID))->rigidBody)
    {
      // If there are no parent node then create a new chain
      if (!parentNode)
      {
	csString name = "default_";
	name += skeletonFactory->GetBoneName (boneID);
	CS::Animation::iSkeletonChain* chain = CreateChain (name, boneID);
	node = dynamic_cast<SkeletonChainNode*> (chain->GetRootNode ());
      }

      // Else create a new chain node
      else
      {
	node.AttachNew (new SkeletonChainNode (boneID));
	parentNode->AddChild (node);
      }
    }

    // Recurse on all children bones
    for (size_t i = index + 1; i < bones.GetSize (); i++)
    {
      const CS::Animation::BoneID &boneIDIt = bones.Get (i);

      // Check that this bone is a child of the parent one
      if (skeletonFactory->GetBoneParent (boneIDIt) != boneID)
	continue;

      PopulateDefaultChainNode (node, bones, i);
    }
  }

  /********************
   *  BoneModel
   ********************/

  CS_LEAKGUARD_IMPLEMENT (BoneModel);

  BoneModel::BoneModel (CS::Animation::BoneID boneID)
    : scfImplementationType (this), animeshBone (boneID)
  {
  }

  /********************
   *  SkeletonChain
   ********************/

  CS_LEAKGUARD_IMPLEMENT (SkeletonChain);

  SkeletonChain::SkeletonChain (SkeletonModel* skeletonModel, const char *name,
			CS::Animation::BoneID rootBone)
    : scfImplementationType (this), name (name), skeletonModel (skeletonModel), rootNode (rootBone)
  {}

  const char* SkeletonChain::GetName () const
  {
    return name;
  }

  CS::Animation::iSkeletonModel* SkeletonChain::GetSkeletonModel ()
  {
    return skeletonModel;
  }

  CS::Animation::iSkeletonChainNode* SkeletonChain::GetRootNode ()
  {
    return &rootNode;
  }

  void CollectAllNodes (csHash<csRef<SkeletonChainNode>, CS::Animation::BoneID>& nodeHash,
			SkeletonChainNode* rootNode)
  {
    nodeHash.Put (rootNode->GetAnimeshBone (), rootNode);

    for (size_t i = 0; i < rootNode->GetChildCount (); i++)
      CollectAllNodes (nodeHash, (SkeletonChainNode*) rootNode->GetChild (i));
  }

  bool SkeletonChain::AddSubChain (CS::Animation::BoneID subBone)
  {
#ifdef CS_DEBUG
    // check if the bone is valid
    if (!skeletonModel->skeletonFactory->HasBone (subBone))
    {
      skeletonModel->manager->Report (CS_REPORTER_SEVERITY_ERROR,
				     "The child bone %i is not in the skeleton factory of the animesh",
				     subBone);
      return false;
    }
#endif

    // collect all nodes of this chain
    csHash<csRef<SkeletonChainNode>, CS::Animation::BoneID> currentHash;
    CollectAllNodes (currentHash, &rootNode);

    // check that the sub bone is not already in the chain
    if (currentHash.Contains (subBone))
      return true;

    // store the new nodes so that they are deleted in case of error
    csHash<csRef<SkeletonChainNode>, CS::Animation::BoneID> nodeHash;

    // OK, create a chain node
    csRef<SkeletonChainNode> currentNode;
    currentNode.AttachNew (new SkeletonChainNode (subBone));
    nodeHash.Put (subBone, currentNode);

    // iterate on the parent nodes until we find a connection to the chain
    CS::Animation::BoneID parent = skeletonModel->skeletonFactory->GetBoneParent (subBone);
    while (1)
    {
      // check that the root bone of the skeleton is not reached
      if (parent == CS::Animation::InvalidBoneID)
      {
	skeletonModel->manager->Report (CS_REPORTER_SEVERITY_ERROR,
			 "The specified child bone %i (%s) is not really a child of root bone %i (%s)",
			 subBone, skeletonModel->skeletonFactory->GetBoneName (subBone),
			 rootNode.boneID, skeletonModel->skeletonFactory->GetBoneName (rootNode.boneID));
	return false;
      }

      // check if we already got this bone
      if (currentHash.Contains (parent))
      {
	// add the current node as a child of the node we already got
	csRef<SkeletonChainNode> node = currentHash.Get (parent, 0);
	node->AddChild (currentNode);
	break;
      }

      // OK, create a new node
      csRef<SkeletonChainNode> node;
      node.AttachNew (new SkeletonChainNode (parent));
      nodeHash.Put (parent, node);
      node->AddChild (currentNode);
      currentNode = node;

      parent = skeletonModel->skeletonFactory->GetBoneParent (parent);
    }

    return true;
  }

  bool SkeletonChain::AddAllSubChains ()
  {
#ifdef CS_DEBUG
    // check if the chain is really empty
    if (rootNode.children.GetSize ())
    {
      skeletonModel->manager->Report (CS_REPORTER_SEVERITY_ERROR,
				     "The chain %s is not empty while trying to add all sub chains",
				     name.GetData ());
      return false;
    }
#endif

    // create a structure holding all the bones of the skeleton.
    // When this method will return, all the bones that are not sub child of this chain will
    // be decref'd and therefore deleted.
    csHash<csRef<SkeletonChainNode>, CS::Animation::BoneID> nodeHash;
    nodeHash.Put (rootNode.boneID, &rootNode);
    CS::Animation::BoneID maxBoneID = skeletonModel->skeletonFactory->GetTopBoneID ();
    //CS::Animation::BoneID rootBoneID = 0;

    for (CS::Animation::BoneID boneIt = 0; boneIt < maxBoneID; boneIt++)
    {
      // check if this is the root of this chain
      if (rootNode.boneID == boneIt)
	continue;

      // find or create an entry in the node hash
      csRef<SkeletonChainNode> node;
      if (nodeHash.Contains (boneIt))
	node = *nodeHash[boneIt];
      else
      {
	node.AttachNew (new SkeletonChainNode (boneIt));
	nodeHash.Put (boneIt, node);
      }

      // check if this is the root bone
      CS::Animation::BoneID parentBone = skeletonModel->skeletonFactory->GetBoneParent (boneIt);
      if (parentBone == CS::Animation::InvalidBoneID)
      {
	//rootBoneID = boneIt;
	continue;
      }

      // find or create an entry in the node hash for the parent of this node
      csRef<SkeletonChainNode> parentNode;
      if (nodeHash.Contains (parentBone))
	parentNode = *nodeHash[parentBone];
      else
      {
	parentNode.AttachNew (new SkeletonChainNode (parentBone));
	nodeHash.Put (parentBone, parentNode);
      }
      parentNode->AddChild (node);
    }

    return true;
  }

  csString SkeletonChain::Description () const
  {
    csString txt;
    txt.Format ("Bone chain %s:\n", name.GetData ());
    Describe (txt, &rootNode);
    return txt;
  }

  void SkeletonChain::Describe (csString& txt, const SkeletonChainNode* node, size_t level) const
  {
    for (size_t i = 0; i < level; i++)
      txt += ' ';
    csString txt2;
    txt2.Format ("+ bone %zu: %s\n", node->boneID,
		 skeletonModel->skeletonFactory->GetBoneName (node->boneID));
    txt += txt2;

    for (csRefArray<SkeletonChainNode>::ConstIterator it = node->children.GetIterator (); it.HasNext (); )
    {
      csRef<SkeletonChainNode> node = it.Next ();
      Describe (txt, node, level + 1);
    }
  }

  void SkeletonChain::PopulateBoneMask (csBitArray& boneMask) const
  {
    boneMask.SetSize (skeletonModel->skeletonFactory->GetTopBoneID () + 1);
    PopulateMask (&rootNode, boneMask);
  }

  void SkeletonChain::PopulateMask (const SkeletonChainNode* node, csBitArray& boneMask) const
  {
    boneMask.SetBit (node->GetAnimeshBone ());
    for (size_t i = 0; i < node->GetChildCount (); i++)
      PopulateMask (node->children[i], boneMask);
  }

  /********************
   *  SkeletonChainNode
   ********************/

  CS_LEAKGUARD_IMPLEMENT (SkeletonChainNode);

  SkeletonChainNode::SkeletonChainNode (CS::Animation::BoneID boneID)
    : scfImplementationType (this), boneID (boneID), parent (0)
  {
  }

  CS::Animation::BoneID SkeletonChainNode::GetAnimeshBone () const
  {
    return boneID;
  }

  size_t SkeletonChainNode::GetChildCount () const
  {
    return (size_t)children.GetSize ();
  }

  CS::Animation::iSkeletonChainNode* SkeletonChainNode::GetChild (size_t index) const
  {
    return children.Get (index);
  }

  CS::Animation::iSkeletonChainNode* SkeletonChainNode::GetParent () const
  {
    return parent;
  }

  void SkeletonChainNode::AddChild (SkeletonChainNode* node)
  {
    node->parent = this;
    children.Push (node);
  }

  CS::Animation::iSkeletonChainNode* SkeletonChainNode::FindSubChild (CS::Animation::BoneID child) const
  {
    if (child == boneID)
      return (CS::Animation::iSkeletonChainNode*) this;

    for (csRefArray<SkeletonChainNode>::ConstIterator it = children.GetIterator (); it.HasNext (); )
    {
      csRef<SkeletonChainNode> node = it.Next ();
      iSkeletonChainNode* result = node->FindSubChild (child);
      if (result)
	return result;
    }

    return nullptr;
  }

  csString SkeletonChainNode::Description () const
  {
    csString txt;
    Describe (txt);
    return txt;
  }

  void SkeletonChainNode::Describe (csString& txt, size_t level) const
  {
    for (size_t i = 0; i < level; i++)
      txt += ' ';
    csString txt2;
    txt2.Format ("+ node %zu\n", boneID);
    txt += txt2;

    for (csRefArray<SkeletonChainNode>::ConstIterator it = children.GetIterator (); it.HasNext (); )
      it.Next ()->Describe (txt, level + 1);
  }

}
CS_PLUGIN_NAMESPACE_END (SkeletonModel)
