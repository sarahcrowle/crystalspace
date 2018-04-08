/*
  Copyright (C) 2010-2012 Christian Van Brussel, Institute of Information
      and Communication Technologies, Electronics and Applied Mathematics
      at Universite catholique de Louvain, Belgium
      http://www.uclouvain.be/en-icteam.html

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include "cssysdef.h"

#include "csutil/ref.h"
#include "csgeom/plane3.h"
#include "iengine/engine.h"
#include "iengine/mesh.h"
#include "imap/ldrctxt.h"
#include "imap/services.h"
#include "iutil/document.h"
#include "iutil/plugin.h"
#include "ivaria/collisions.h"
#include "ivaria/convexdecompose.h"
#include "ivaria/physics.h"

#include "modelldr.h"

CS_PLUGIN_NAMESPACE_BEGIN (ModelLoader)
{
  SCF_IMPLEMENT_FACTORY (ModelLoader);

  static const char* msgid = "crystalspace.mesh.loader.skeleton.model";

  ModelLoader::ModelLoader (iBase* parent)
    : scfImplementationType (this, parent)
  {
  }

  bool ModelLoader::Initialize (iObjectRegistry* objReg)
  {
    object_reg = objReg;

    synldr = csQueryRegistry<iSyntaxService> (object_reg);

    modelManager = csQueryRegistryOrLoad<CS::Animation::iSkeletonModelManager>
      (object_reg, "crystalspace.mesh.animesh.model");

    if (!collisionHelper.Initialize (object_reg)) return false;

    InitTokenTable (xmltokens);
    return true;
  }

  csPtr<iBase> ModelLoader::Parse (iDocumentNode* node,
    iStreamSource* ssource, iLoaderContext* ldr_context,
    iBase* context)
  {
    if (!modelManager)
    {
      synldr->ReportError (msgid, node, "Couldn't get any body mesh system");
      return (iBase*) nullptr;
    }

    csRef<iDocumentNodeIterator> it = node->GetNodes ();
    while (it->HasNext ())
    {
      csRef<iDocumentNode> child = it->Next ();
      if (child->GetType () != CS_NODE_ELEMENT) continue;
      const char* value = child->GetValue ();
      csStringID id = xmltokens.Request (value);
      switch (id)
      {
      case XMLTOKEN_MODEL:
        if (!ParseModel (child, ldr_context))
          return (iBase*) nullptr;
        break;

      default:
        synldr->ReportBadToken (child);
        return (iBase*) nullptr;
      }

      break;
    }

    return csPtr<iBase> (model);
  }

  bool ModelLoader::ParseModel (iDocumentNode* node,
				iLoaderContext* ldr_context)
  {
    // Parse the animesh factory
    const char* factoryName = node->GetAttributeValue ("skeletonfact");
    if (!factoryName)
    {
      synldr->ReportError (msgid, node, "No animesh factory specified");
      return false;
    }

    csRef<CS::Animation::iSkeletonManager> skeletonManager =
      csQueryRegistry<CS::Animation::iSkeletonManager> (object_reg);
    if (!skeletonManager)
    {
      synldr->ReportError (msgid, node, "No animesh skeleton manager");
      return false;
    }

    CS::Animation::iSkeletonFactory* skeletonFactory =
      skeletonManager->FindSkeletonFactory (factoryName);
    if (!skeletonFactory)
    {
      synldr->ReportError (msgid, node, "No animesh skeleton factory named %s",
			   factoryName);
      return false;
    }

    // Create the skeleton model
    model = modelManager->CreateModel (skeletonFactory);
    skeletonFactory->SetSkeletonModel (model);

    // Parse the child nodes
    csRef<iDocumentNodeIterator> it = node->GetNodes ();
    while (it->HasNext ())
    {
      csRef<iDocumentNode> child = it->Next ();
      if (child->GetType () != CS_NODE_ELEMENT) continue;
      const char* value = child->GetValue ();
      csStringID id = xmltokens.Request (value);
      switch (id)
      {
      case XMLTOKEN_BONE:
        if (!ParseBone (child, ldr_context))
          return false;
        break;

      case XMLTOKEN_CHAIN:
        if (!ParseChain (child))
          return false;
        break;

      default:
        synldr->ReportBadToken (child);
        return false;
      }
    }

    return true;
  }

  bool ModelLoader::ParseBone (iDocumentNode* node, iLoaderContext* loaderContext)
  {
    // Parse the bone name
    const char* name = node->GetAttributeValue ("name");
    if (!name)
    {
      synldr->ReportError (msgid, node, "No name set for bone");
      return false;
    }

    CS::Animation::BoneID boneID = model->GetSkeletonFactory ()->FindBone (name);
    if (boneID == CS::Animation::InvalidBoneID)
    {
      synldr->ReportError (msgid, node, "No bone with name %s in skeleton factory",
			   name);
      return false;
    }

    // Create the bone model
    CS::Animation::iSkeletonBoneModel* boneModel = model->CreateBoneModel (boneID);

    // Parse the child nodes
    csRef<iDocumentNodeIterator> it = node->GetNodes ();
    while (it->HasNext ())
    {
      csRef<iDocumentNode> child = it->Next ();
      if (child->GetType () != CS_NODE_ELEMENT) continue;
      const char* value = child->GetValue ();
      csStringID id = xmltokens.Request (value);
      switch (id)
      {
      case XMLTOKEN_RIGIDBODY:
      {
	csRef<CS::Physics::iRigidBodyFactory> body =
	  collisionHelper.ParseRigidBodyFactory (child, loaderContext, nullptr);
	if (!body) return false;
	boneModel->SetRigidBodyFactory (body);
        break;
      }

      case XMLTOKEN_JOINT:
      {
	csOrthoTransform transform;
	csRef<CS::Physics::iJointFactory> joint =
	  collisionHelper.ParseJointFactory (child, transform, loaderContext, nullptr);
	if (!joint) return false;
	boneModel->SetJointFactory (joint);
	boneModel->SetJointTransform (transform);
        break;
      }

      default:
        synldr->ReportBadToken (child);
        return false;
      }
    }

    return true;
  }

  bool ModelLoader::ParseChain (iDocumentNode* node)
  {
    const char* name = node->GetAttributeValue ("name");

    const char* root = node->GetAttributeValue ("root");
    CS::Animation::BoneID boneID = model->GetSkeletonFactory ()->FindBone (root);
    if (boneID == CS::Animation::InvalidBoneID)
    {
      synldr->ReportError (msgid, node, "Wrong root bone of chain: no bone with name %s in skeleton factory",
			   root);
      return false;
    }

    CS::Animation::iSkeletonChain* chain = model->CreateChain (name, boneID);

    csRef<iDocumentNodeIterator> it = node->GetNodes ();
    while (it->HasNext ())
    {
      csRef<iDocumentNode> child = it->Next ();
      if (child->GetType () != CS_NODE_ELEMENT) continue;
      const char *value = child->GetValue ();
      csStringID id = xmltokens.Request (value);
      switch (id)
	{
	case XMLTOKEN_CHILDALL:
	  chain->AddAllSubChains ();
	  break;

	case XMLTOKEN_CHILD:
	{
	  const char* name = child->GetAttributeValue ("name");
	  CS::Animation::BoneID boneID = model->GetSkeletonFactory ()->FindBone (name);
	  if (boneID == CS::Animation::InvalidBoneID)
	  {
	    synldr->ReportError (msgid, node, "Wrong child bone of chain: no bone with name %s in skeleton factory",
				 name);
	    return false;
	  }

	  chain->AddSubChain (boneID);
	}
	  break;

	default:
	  synldr->ReportBadToken (child);
	  return false;
	}
    }

    return true;
  }

}
CS_PLUGIN_NAMESPACE_END (ModelLoader)
