/*
  Copyright (C) 2010-2013 Christian Van Brussel, Institute of Information
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
#ifndef __CS_SOFTBODYANIM_H__
#define __CS_SOFTBODYANIM_H__

#include "csutil/dirtyaccessarray.h"
#include "csutil/scf_implementation.h"
#include "csutil/weakref.h"
#include "iengine/movable.h"
#include "imesh/animesh.h"
#include "iutil/comp.h"
#include "imesh/softanim.h"

CS_PLUGIN_NAMESPACE_BEGIN (SoftAnim2)
{

class SoftBody2ControlType : public scfImplementation2<SoftBody2ControlType,
    CS::Animation::iSoftBodyAnimationControl2Type, iComponent>
{
  public:
    CS_LEAKGUARD_DECLARE(SoftBody2ControlType);

    SoftBody2ControlType (iBase* parent);

    //-- iGenMeshAnimationControlType
    virtual csPtr<iGenMeshAnimationControlFactory> CreateAnimationControlFactory ();

    //-- iComponent
    virtual bool Initialize (iObjectRegistry*);

    // error reporting
    //void Report (int severity, const char* msg, ...) const;

  private:
    iObjectRegistry* object_reg;

    friend class SoftBodyControlFactory;
    friend class SoftBodyControl;
};

class SoftBodyControlFactory : public scfImplementation2<SoftBodyControlFactory, 
    scfFakeInterface<iGenMeshAnimationControlFactory>,
    CS::Animation::iSoftBodyAnimationControl2Factory>
{
  public:
    CS_LEAKGUARD_DECLARE(CS::Animation::iSoftBodyAnimationControl2Factory);

    SoftBodyControlFactory (SoftBody2ControlType* type);

    //-- iGenMeshAnimationControlFactory
    virtual csPtr<iGenMeshAnimationControl> CreateAnimationControl (iMeshObject* mesh);
    virtual const char* Load (iDocumentNode* node);
    virtual const char* Save (iDocumentNode* parent);

 private:
    csRef<SoftBody2ControlType> type;

    friend class SoftBodyControl;
};

class SoftBodyControl : public scfImplementation2<SoftBodyControl, 
    scfFakeInterface<iGenMeshAnimationControl>, CS::Animation::iSoftBodyAnimationControl2>
{
  public:
    CS_LEAKGUARD_DECLARE(CS::Animation::iSoftBodyAnimationControl2);

    SoftBodyControl (SoftBodyControlFactory* factory, iMeshObject* mesh);

    //-- CS::Animation::iSoftBodyAnimationControl2
    virtual void SetSoftBody (CS::Physics::iSoftBody* body,
			      CS::Physics::MeshDuplicationMode duplicationMode
			      = CS::Physics::MESH_DUPLICATION_NONE);
    virtual CS::Physics::iSoftBody* GetSoftBody ();

    virtual void CreateAnimatedMeshAnchor (CS::Mesh::iAnimatedMesh* animesh,
					   CS::Physics::iRigidBody* body,
					   size_t bodyVertexIndex,
					   size_t animeshVertexIndex = (size_t) ~0);
    virtual size_t GetAnimatedMeshAnchorVertex (size_t bodyVertexIndex);
    virtual void RemoveAnimatedMeshAnchor (size_t bodyVertexIndex);

    //-- iGenMeshAnimationControl
    virtual bool AnimatesColors () const;
    virtual bool AnimatesNormals () const;
    virtual bool AnimatesTexels () const;
    virtual bool AnimatesVertices () const;
    virtual bool AnimatesBBoxRadius () const;
    virtual void Update (csTicks current, int num_verts, uint32 version_id);
    virtual const csColor4* UpdateColors (csTicks current, const csColor4* colors,
					  int num_colors, uint32 version_id);
    virtual const csVector3* UpdateNormals (csTicks current, const csVector3* normals,
					    int num_normals, uint32 version_id);
    virtual const csVector2* UpdateTexels (csTicks current, const csVector2* texels,
					   int num_texels, uint32 version_id);
    virtual const csVector3* UpdateVertices (csTicks current, const csVector3* verts,
					     int num_verts, uint32 version_id);
    virtual const csBox3& UpdateBoundingBox (csTicks current, uint32 version_id,
					     const csBox3& factoryBbox);
    virtual const float UpdateRadius (csTicks current, uint32 version_id,
				      const float factoryRadius);
    virtual const csBox3* UpdateBoundingBoxes (csTicks current, uint32 version_id);

  private:
    csRef<SoftBodyControlFactory> factory;
    csWeakRef<iMeshObject> mesh;
    csRef<CS::Physics::iSoftBody> softBody;
    CS::Physics::MeshDuplicationMode duplicationMode;
    csDirtyAccessArray<csVector3> vertices;
    csDirtyAccessArray<csVector3> normals;
    csBox3 bbox;
    csDirtyAccessArray<csBox3> bboxes;
    csTicks lastTicks;
    csVector3 meshPosition;

    struct Anchor
    {
      csRef<CS::Mesh::iAnimatedMesh> animesh;
      csRef<CS::Physics::iRigidBody> body;
      size_t bodyVertexIndex;
      size_t animeshVertexIndex;
    };
    csArray<Anchor> anchors;
};

}
CS_PLUGIN_NAMESPACE_END (SoftAnim2)

#endif //__CS_SOFTBODYANIM_H__
