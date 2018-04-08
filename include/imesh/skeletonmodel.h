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
#ifndef __CS_IMESH_SKELETONMODEL_H__
#define __CS_IMESH_SKELETONMODEL_H__

/**\file
 * Physical description of the skeleton of a CS::Mesh::iAnimatedMesh.
 */

#include "csutil/scf_interface.h"

#include "imesh/animnode/skeleton2anim.h"
#include "ivaria/colliders.h"

/**\addtogroup meshplugins
 * @{ */

namespace CS {

namespace Mesh {

struct iAnimatedMeshFactory;

} // namespace Mesh

namespace Physics {

struct iJointFactory;
struct iRigidBodyFactory;

} // namespace Physics

namespace Animation {

struct iSkeletonBoneModel;
struct iSkeletonChain;
struct iSkeletonChainNode;
struct iSkeletonFactory;
struct iSkeletonModel;

/**
 * A class to manage the creation and deletion of skeleton models.
 */
struct iSkeletonModelManager : public virtual iBase
{
  SCF_INTERFACE (CS::Animation::iSkeletonModelManager, 1, 0, 0);

  /**
   * Create a new skeleton model for the given skeleton factory
   */
  virtual csPtr<iSkeletonModel> CreateModel (iSkeletonFactory* skeletonFactory) = 0;
};

/**
 * This iterator is returned by iSkeletonModel::GetBoneModels ().
 */
struct iSkeletonBoneModelIterator : public virtual iBase
{
  SCF_INTERFACE (iSkeletonBoneModelIterator, 1, 0, 0);

  virtual bool HasNext () const = 0;
  virtual iSkeletonBoneModel* Next () = 0;
};

/**
 * This iterator is returned by iSkeletonModel::GetSkeletonChains ().
 */
struct iSkeletonChainIterator : public virtual iBase
{
  SCF_INTERFACE (iSkeletonChainIterator, 1, 0, 0);

  virtual bool HasNext () const = 0;
  virtual iSkeletonChain* Next () = 0;
};

/**
 * This class holds the physical description of the skeleton of an CS::Mesh::iAnimatedMesh.
 * For each relevant bone of the skeleton, one has to define an CS::Animation::iSkeletonBoneModel that 
 * will hold the colliders, joint and properties of the bone.
 * Subtrees of the skeleton are defined through the CS::Animation::iSkeletonChain object.
 */
struct iSkeletonModel : public virtual iBase
{
  SCF_INTERFACE (CS::Animation::iSkeletonModel, 1, 0, 0);

  /// Query the iObject interface of this skeleton model.
  virtual iObject* QueryObject () = 0;

  /**
   * Get the skeleton factory associated with this skeleton model.
   */
  virtual iSkeletonFactory* GetSkeletonFactory () const = 0;

  /**
   * Delete all bone models and all skeleton chains.
   */
  virtual void ClearAll () = 0;

  /**
   * Create a new bone model.
   * \param boneID The ID of the animesh bone associated to the bone model.
   */
  virtual iSkeletonBoneModel* CreateBoneModel (BoneID boneID) = 0;

  /**
   * Find a bone model from the name of the associated animesh bone.
   */
  virtual iSkeletonBoneModel* FindBoneModel (const char *name) const = 0;

  /**
   * Find a bone model from the ID of the associated animesh bone.
   */
  virtual iSkeletonBoneModel* FindBoneModel (BoneID bone) const = 0;

  /**
   * Get an iterator over all the bone models
   */
  virtual csPtr<iSkeletonBoneModelIterator> GetBoneModels () const = 0;

  /**
   * Remove a bone model.
   */
  virtual void RemoveBoneModel (BoneID bone) = 0;

  /**
   * Delete all bone models.
   */
  virtual void ClearBoneModels () = 0;

  /**
   * Create a new skeleton chain, ie a subtree of the animesh skeleton.
   * \param name The name of the skeleton chain.
   * \param rootBone The root of the skeleton chain.
   * \return The skeleton chain upon success, nullptr if there was a problem.
   */
  virtual iSkeletonChain* CreateChain (const char *name, BoneID rootBone) = 0;

  /**
   * Find a skeleton chain from its name.
   */
  virtual iSkeletonChain* FindChain (const char *name) const = 0;

  /**
   * Get an iterator over all skeleton chains in this body.
   */
  virtual csPtr<iSkeletonChainIterator> GetChains () const = 0;

  /**
   * Remove a skeleton chain.
   */
  virtual void RemoveChain (const char* name) = 0;

  /**
   * Delete all skeleton chains.
   */
  virtual void ClearChains () = 0;

  /**
   * Populate this skeleton model with default colliders for all the bones of the
   * skeleton. For each bone, if there are no colliders already defined, then
   * this method will create a new collider based on the bounding box of the bone.
   *
   * If at least one collider exists for the parent bone, then a joint will also
   * be created between the two bones. By default, the joints are constrained in
   * translation and free in rotation.

   * \param animeshFactory The animesh factory associated with this skeleton.
   * \param colliderType The type of colliders to use
   */
  // TODO: tool class?
  virtual void PopulateDefaultModels
    (const CS::Mesh::iAnimatedMeshFactory* animeshFactory,
     CS::Collisions::ColliderType colliderType = CS::Collisions::COLLIDER_CAPSULE) = 0;

  /**
   * Populate this skeleton model with default skeleton chains. This method will try to
   * create as less skeleton chains as possible, covering all the bones that contains
   * a rigid body factory.
   *
   * The name of the chains that are created are of the format 'default_' + \a root_bone_name.
   *
   * \note This method should work well when called after PopulateDefaultModels ().
   */
  // TODO: tool class?
  virtual void PopulateDefaultChains () = 0;
};

/**
 * A bone model holds the physical description of the bone of an CS::Mesh::iAnimatedMesh.
 */
struct iSkeletonBoneModel : public virtual iBase
{
  SCF_INTERFACE (CS::Animation::iSkeletonBoneModel, 1, 0, 0);

  /**
   * Return the ID of the bone of the CS::Mesh::iAnimatedMesh associated with this bone model.
   */
  virtual BoneID GetAnimeshBone () const = 0;

  /**
   * Set the rigid body factory describing the physical properties of this bone model.
   */
  virtual void SetRigidBodyFactory (CS::Physics::iRigidBodyFactory* factory) = 0;

  /**
   * Get the physical properties of this bone model.
   * \return nullptr if no properties were defined. 
   */
  virtual CS::Physics::iRigidBodyFactory* GetRigidBodyFactory () const = 0;

  /**
   * Set the joint factory of this bone model. It describes the properties of the joint
   * linking the physical body of this bone to the one of the parent bone.
   */
  virtual void SetJointFactory (CS::Physics::iJointFactory* factory) = 0;

  /**
   * Get the description of the joint of this bone model.
   * \return 0 if no joint was defined. 
   */
  virtual CS::Physics::iJointFactory* GetJointFactory () const = 0;

  /**
   * Set the local transform of the joint of this bone model, relatively to the
   * transform of the animesh's bone.
   */
  virtual void SetJointTransform (const csOrthoTransform &transform) = 0;

  /**
   * Get the local transform of the joint of this bone model, relatively to the
   * transform of the animesh's bone.
   */
  virtual const csOrthoTransform& GetJointTransform () const = 0;
};

/**
 * A skeleton chain is a subtree of the skeleton of an CS::Mesh::iAnimatedMesh. It is used
 * to apply varying animation controllers on different parts of the skeleton.
 */
struct iSkeletonChain : public virtual iBase
{
  SCF_INTERFACE (CS::Animation::iSkeletonChain, 1, 0, 0);

  /**
   * Get the name of this skeleton chain.
   */
  virtual const char* GetName () const = 0;

  /**
   * Get the associated physical description of the skeleton
   */
  virtual iSkeletonModel* GetSkeletonModel () = 0;

  /**
   * Get the root node of this chain.
   */
  virtual iSkeletonChainNode* GetRootNode () = 0;

  /**
   * Add a sub-chain to this chain, ie all nodes from the root of the chain to the given subBone.
   *
   * For example, if you want to create a chain for the two legs of a human 
   * body, you should set the pelvis as the root node, and call this method once for each foot.
   * \return True upon success, false otherwise (ie the subBone has not been found as a sub-child
   * of the root of this chain)
   */
  virtual bool AddSubChain (CS::Animation::BoneID subBone) = 0;

  /**
   * Add all sub-child of the root node of this chain.
   * \return True upon success, false otherwise (this should never happen although)
   */
  virtual bool AddAllSubChains () = 0;

  /**
   * Print the hierarchical structure of this bone chain to the standard output.
   */
  virtual csString Description () const = 0;

  /**
   * Populate the given bone mask with the bones of this chain. The bit mask will
   * not be reset before populating it, but its size will be grown if needed to the
   * count of bones in the skeleton.
   */
  // TODO: tool class?
  virtual void PopulateBoneMask (csBitArray& boneMask) const = 0;
};

/**
 * A node in a skeleton chain tree. A node is directly associated to a 
 * bone model.
 */
struct iSkeletonChainNode : public virtual iBase
{
  SCF_INTERFACE (CS::Animation::iSkeletonChainNode, 1, 0, 0);

  /**
   * Return the ID of the bone of the CS::Mesh::iAnimatedMesh associated with
   * this node.
   */
  virtual BoneID GetAnimeshBone () const = 0;

  /**
   * Get the count of children of this node.
   */
  virtual size_t GetChildCount () const = 0;

  /**
   * Get the given child of this node.
   */
  virtual iSkeletonChainNode* GetChild (size_t index) const = 0;

  /**
   * Get the parent node of this node. Returns 0 if the node 
   * is the root of the skeleton chain.
   */
  virtual iSkeletonChainNode* GetParent () const = 0;

  /**
   * Find the node of the bone with the given BoneID in the descendent children of this node.
   * Return 0 if the given bone was not found.
   */
  virtual iSkeletonChainNode* FindSubChild (CS::Animation::BoneID child) const = 0;

  /**
   * Print the hierarchical structure of this node and its children to the standard output.
   */
  virtual csString Description () const = 0;
};

} // namespace Animation
} // namespace CS

/** @} */

#endif // __CS_IMESH_SKELETONMODEL_H__
