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
#ifndef __CS_RAGDOLL_H__
#define __CS_RAGDOLL_H__

#include "csutil/csstring.h"
#include "csutil/leakguard.h"
#include "csutil/scf_implementation.h"
#include "csutil/weakref.h"
#include "cstool/animnodetmpl.h"
#include "imesh/animnode/ragdoll2.h"
#include "imesh/skeletonmodel.h"
#include "iutil/comp.h"
#include "ivaria/physics.h"

CS_PLUGIN_NAMESPACE_BEGIN (Ragdoll2)
{

  class Ragdoll2NodeManager;

  struct ChainData
  {
    csRef<CS::Animation::iSkeletonChain> chain;
    CS::Animation::RagdollState state;
  };

  class RagdollNodeFactory
    : public scfImplementation2<RagdollNodeFactory,
    scfFakeInterface<CS::Animation::iSkeletonAnimNodeFactory>,
    CS::Animation::iSkeletonRagdollNodeFactory2>,
    public CS::Animation::SkeletonAnimNodeFactorySingle
  {
  public:
    CS_LEAKGUARD_DECLARE (RagdollNodeFactory);

    RagdollNodeFactory (Ragdoll2NodeManager* manager, const char *name);

    //-- CS::Animation::iSkeletonRagdollNodeFactory2
    virtual void AddChain (CS::Animation::iSkeletonChain* chain,
			   CS::Animation::RagdollState state
			   = CS::Animation::STATE_INACTIVE);
    virtual void RemoveChain (CS::Animation::iSkeletonChain* chain);

    inline virtual void SetChildNode (CS::Animation::iSkeletonAnimNodeFactory* factory)
    { CS::Animation::SkeletonAnimNodeFactorySingle::SetChildNode (factory); }
    inline virtual iSkeletonAnimNodeFactory* GetChildNode () const
    { return CS::Animation::SkeletonAnimNodeFactorySingle::GetChildNode (); }

    //-- CS::Animation::iSkeletonAnimNodeFactory2
    csPtr<CS::Animation::SkeletonAnimNodeSingleBase> ActualCreateInstance (
      CS::Animation::iSkeletonAnimPacket* packet, CS::Animation::iSkeleton* skeleton);

  protected:
    csRef<Ragdoll2NodeManager> manager;
    csWeakRef<CS::Physics::iPhysicalSystem> physicalSystem;
    csArray<ChainData> chains;
    CS::Animation::BoneID ragdollRoot;

    friend class RagdollNode;
  };

  class RagdollNode
    : public scfImplementation2<RagdollNode,
				scfFakeInterface<CS::Animation::iSkeletonAnimNode>,
				CS::Animation::iSkeletonRagdollNode2>,
      CS::Animation::SkeletonAnimNodeSingle<RagdollNodeFactory>
  {
  public:
    CS_LEAKGUARD_DECLARE (RagdollNode);

    RagdollNode (RagdollNodeFactory* factory, CS::Animation::iSkeleton* skeleton);
    ~RagdollNode ();

    //-- CS::Animation::iSkeletonRagdollNode2
    virtual void SetChainState (CS::Animation::iSkeletonChain* chain, CS::Animation::RagdollState state);
    virtual CS::Animation::RagdollState GetChainState (CS::Animation::iSkeletonChain* chain) const;

    virtual CS::Physics::iRigidBody* GetBoneRigidBody (CS::Animation::BoneID bone);
    virtual CS::Physics::iJoint* GetBoneJoint (const CS::Animation::BoneID bone);

    virtual uint GetBoneCount (CS::Animation::RagdollState state) const;
    virtual CS::Animation::BoneID GetBone (CS::Animation::RagdollState state, uint index) const;
    virtual CS::Animation::BoneID FindBone (CS::Physics::iRigidBody* body) const;

    virtual void ResetChainTransform (CS::Animation::iSkeletonChain* chain);

    //-- CS::Animation::iSkeletonAnimNode
    virtual void Play ();
    virtual void Stop ();

    virtual void BlendState (CS::Animation::AnimatedMeshState* state,
			     float baseWeight = 1.0f);
    virtual void TickAnimation (float dt);

  private:
    struct BoneData
    {
      CS::Animation::BoneID boneID;
      CS::Animation::RagdollState state;
      csRef<CS::Physics::iRigidBody> rigidBody;
      csRef<CS::Physics::iJoint> joint;
    };

    struct ResetChainData
    {
      csRef<CS::Animation::iSkeletonChain> chain;
      int frameCount;
    };

    void CreateBoneData (CS::Animation::iSkeletonChainNode* chainNode,
			 CS::Animation::RagdollState state);
    void SetChainNodeState (CS::Animation::iSkeletonChainNode* chainNode,
			    CS::Animation::RagdollState state);
    void InitBoneStates ();
    void UpdateBoneState (BoneData* boneData);
    void ResetChainNodeTransform (CS::Animation::iSkeletonChainNode* chainNode);

  private:
    csRef<CS::Animation::iSkeletonModel> skeletonModel;
    csRef<CS::Physics::iPhysicalSector> physicalSector;
    csWeakRef<iSceneNode> sceneNode;
    csArray<ChainData> chains;
    csHash<BoneData, CS::Animation::BoneID> bones;
    csArray<ResetChainData> resetChains;
    CS::Animation::BoneID maxBoneID;

    friend class RagdollNodeFactory;
    friend class BoneKinematicCallback;
  };

  class BoneKinematicCallback : public scfImplementation1
    <BoneKinematicCallback, CS::Physics::iKinematicCallback>
  {
  public:
    BoneKinematicCallback (RagdollNode* ragdollNode, CS::Animation::BoneID boneID);
    ~BoneKinematicCallback ();

    void GetBodyTransform (CS::Physics::iRigidBody* body, csOrthoTransform& transform) const;

  private:
    RagdollNode* ragdollNode;
    CS::Animation::BoneID boneID;
  };

  class Ragdoll2NodeManager
    : public CS::Animation::AnimNodeManagerCommon<Ragdoll2NodeManager,
						  CS::Animation::iSkeletonRagdollNodeManager2,
						  RagdollNodeFactory>
  {
    friend class RagdollNode;
    friend class RagdollNodeFactory;

  public:
    Ragdoll2NodeManager (iBase* parent)
     : AnimNodeManagerCommonType (parent) {}
     
    void Report (int severity, const char* msg, ...) const;
  };

}
CS_PLUGIN_NAMESPACE_END (Ragdoll)

#endif //__CS_RAGDOLL_H__
