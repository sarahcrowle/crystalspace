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
#ifndef __CS_BODYMESH_H__
#define __CS_BODYMESH_H__

#include "csgeom/matrix3.h"
#include "csgeom/plane3.h"
#include "csutil/csobject.h"
#include "csutil/leakguard.h"
#include "csutil/hash.h"
#include "csutil/csstring.h"
#include "csutil/refarr.h"
#include "csutil/scf_implementation.h"
#include "csutil/weakref.h"
#include "imesh/animesh.h"
#include "imesh/skeletonmodel.h"
#include "iutil/comp.h"
#include "ivaria/physics.h"

CS_PLUGIN_NAMESPACE_BEGIN (SkeletonModel)
{
  using namespace CS::Animation;

  class BoneModel;
  class SkeletonChainNode;

  class ModelManager
    : public scfImplementation2<ModelManager, CS::Animation::iSkeletonModelManager, iComponent>
  {
  public:
    CS_LEAKGUARD_DECLARE (ModelManager);

    ModelManager (iBase* parent);

    //-- iComponent
    virtual bool Initialize (iObjectRegistry* object_reg);

    //-- CS::Animation::iSkeletonModelManager
    virtual csPtr<CS::Animation::iSkeletonModel> CreateModel
      (CS::Animation::iSkeletonFactory* skeletonFactory);

    // Internal
    void Report (int severity, const char* msg, ...);
  
  public:
    iObjectRegistry* object_reg;
  };

  class SkeletonModel
    : public scfImplementationExt1<SkeletonModel, csObject, CS::Animation::iSkeletonModel>
  {
  public:
    CS_LEAKGUARD_DECLARE (SkeletonModel);

    SkeletonModel (ModelManager* manager,
		  CS::Animation::iSkeletonFactory* skeletonFactory);

    virtual iObject* QueryObject () { return this; }
    virtual CS::Animation::iSkeletonFactory* GetSkeletonFactory () const;

    virtual void ClearAll ();

    virtual CS::Animation::iSkeletonBoneModel* CreateBoneModel (CS::Animation::BoneID boneID);
    virtual CS::Animation::iSkeletonBoneModel* FindBoneModel (const char *name) const;
    virtual CS::Animation::iSkeletonBoneModel* FindBoneModel (CS::Animation::BoneID bone) const;
    virtual csPtr<CS::Animation::iSkeletonBoneModelIterator> GetBoneModels () const;
    virtual void RemoveBoneModel (CS::Animation::BoneID bone);
    virtual void ClearBoneModels ();

    virtual CS::Animation::iSkeletonChain* CreateChain
      (const char *name, CS::Animation::BoneID rootBone);
    virtual CS::Animation::iSkeletonChain* FindChain (const char *name) const;
    virtual csPtr<CS::Animation::iSkeletonChainIterator> GetChains () const;
    virtual void RemoveChain (const char* name);
    virtual void ClearChains ();

    virtual void PopulateDefaultModels
      (const CS::Mesh::iAnimatedMeshFactory* animeshFactory,
       CS::Collisions::ColliderType colliderType = CS::Collisions::COLLIDER_CAPSULE);
    virtual void PopulateDefaultChains ();

  private:
    void PopulateDefaultChainNode (SkeletonChainNode* parentNode,
				   const csArray<CS::Animation::BoneID> &bones,
				   size_t index);

    csRef<ModelManager> manager;
    // This is a weakref in order to avoid circular references between animesh,
    // skeleton model and animation nodes
    csWeakRef<CS::Animation::iSkeletonFactory> skeletonFactory;
    csHash<csRef<BoneModel>, CS::Animation::BoneID> boneHash;
    csHash<csRef<CS::Animation::iSkeletonChain>, csString> chainHash;

    friend class SkeletonChain;
  };

  class BoneModel : public scfImplementation1<BoneModel, CS::Animation::iSkeletonBoneModel>
  {
  public:
    CS_LEAKGUARD_DECLARE (BoneModel);

    BoneModel (CS::Animation::BoneID boneID);

    virtual CS::Animation::BoneID GetAnimeshBone () const
    { return animeshBone; }
    virtual void SetRigidBodyFactory (CS::Physics::iRigidBodyFactory* factory)
    { rigidBody = factory; }
    virtual CS::Physics::iRigidBodyFactory* GetRigidBodyFactory () const
    { return rigidBody; }
    virtual void SetJointFactory (CS::Physics::iJointFactory* factory)
    { joint = factory; }
    virtual CS::Physics::iJointFactory* GetJointFactory () const
    { return joint; }
    virtual void SetJointTransform (const csOrthoTransform &transform)
    { jointTransform = transform; }
    virtual const csOrthoTransform& GetJointTransform () const
    { return jointTransform; }

  private:
    CS::Animation::BoneID animeshBone;
    csRef<CS::Physics::iRigidBodyFactory> rigidBody;
    csRef<CS::Physics::iJointFactory> joint;
    csOrthoTransform jointTransform;

    friend class SkeletonChainNode;
    friend class SkeletonModel;
  };

  class SkeletonChainNode : public scfImplementation1<SkeletonChainNode,
    CS::Animation::iSkeletonChainNode>
  {
  public:
    CS_LEAKGUARD_DECLARE (SkeletonChainNode);

    SkeletonChainNode (CS::Animation::BoneID boneID);

    virtual CS::Animation::BoneID GetAnimeshBone () const;

    virtual size_t GetChildCount () const;
    virtual CS::Animation::iSkeletonChainNode* GetChild (size_t index) const;
    virtual CS::Animation::iSkeletonChainNode* GetParent () const;
    virtual iSkeletonChainNode* FindSubChild (CS::Animation::BoneID child) const;

    void AddChild (SkeletonChainNode* node);

    virtual csString Description () const;

  private:
    void Describe (csString& txt, size_t level = 0) const;

    CS::Animation::BoneID boneID;
    SkeletonChainNode* parent;
    csRefArray<SkeletonChainNode> children;

    friend class SkeletonChain;
  };

  class SkeletonChain : public scfImplementation1<SkeletonChain, CS::Animation::iSkeletonChain>
  {
  public:
    CS_LEAKGUARD_DECLARE (SkeletonChain);

    SkeletonChain (SkeletonModel* bodySkeleton, const char *name,
	       CS::Animation::BoneID rootBone);

    virtual const char* GetName () const;
    virtual CS::Animation::iSkeletonModel* GetSkeletonModel ();
    virtual CS::Animation::iSkeletonChainNode* GetRootNode ();

    virtual bool AddSubChain (CS::Animation::BoneID subBone);
    virtual bool AddAllSubChains ();

    virtual csString Description () const;

    virtual void PopulateBoneMask (csBitArray& boneMask) const;

  private:
    void Describe (csString& txt, const SkeletonChainNode* node, size_t level = 0) const;
    void PopulateMask (const SkeletonChainNode* node, csBitArray& boneMask) const;

    csString name;
    SkeletonModel* skeletonModel;
    SkeletonChainNode rootNode;
  };

}
CS_PLUGIN_NAMESPACE_END (SkeletonModel)

#endif //__CS_BODYMESH_H__
