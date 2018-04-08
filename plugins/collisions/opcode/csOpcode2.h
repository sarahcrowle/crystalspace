

CS_PLUGIN_NAMESPACE_BEGIN (Opcode2)
{
class csOpcodeCollisionSystem;
class csOpcodeCollisionObject;

class csOpcodeCollisionSystem : public scfVirtImplementation2<
  csOpcodeCollisionSystem, CS::Collisions::iCollisionSystem,
  iComponent>
{
friend class csOpcodeCollider;
private:
  iObjectRegistry* object_reg;
  csRefArrayObject<csOpcodeCollisionSector> collSectors;
  csStringID baseID;
  csStringID colldetID;

public:
  csOpcodeCollisionSystem (iBase* iParent);
  virtual ~csOpcodeCollisionSystem ();

  // iComponent
  virtual bool Initialize (iObjectRegistry* object_reg);

  // iCollisionSystem
  virtual void SetInternalScale (float scale);
  virtual csPtr<CS::Collisions::iColliderCompound> CreateColliderCompound() { return nullptr; }
  virtual csPtr<CS::Collisions::iColliderConvexMesh> CreateColliderConvexMesh (
    iMeshWrapper* mesh, bool simplify = false);
  virtual csPtr<CS::Collisions::iColliderConcaveMesh> CreateColliderConcaveMesh (iMeshWrapper* mesh);
  virtual csPtr<CS::Collisions::iColliderConcaveMeshScaled> CreateColliderConcaveMeshScaled
    (CS::Collisions::iColliderConcaveMesh* collider, csVector3 scale);
  virtual csPtr<CS::Collisions::iColliderCylinder> CreateColliderCylinder (float length, float radius);
  virtual csPtr<CS::Collisions::iColliderBox> CreateColliderBox (const csVector3& size);
  virtual csPtr<CS::Collisions::iColliderSphere> CreateColliderSphere (float radius);
  virtual csPtr<CS::Collisions::iColliderCapsule> CreateColliderCapsule (float length, float radius);
  virtual csPtr<CS::Collisions::iColliderCone> CreateColliderCone (float length, float radius);
  virtual csPtr<CS::Collisions::iColliderPlane> CreateColliderPlane (const csPlane3& plane);
  virtual csPtr<CS::Collisions::iCollisionTerrain> CreateCollisionTerrain (iTerrainSystem* terrain,
    float minHeight = 0, float maxHeight = 0);
  
  virtual csPtr<CS::Collisions::iCollisionObject> CreateCollisionObject ();
  virtual csPtr<CS::Collisions::iCollisionGhostObject> CreateGhostCollisionObject ();
  virtual csPtr<CS::Collisions::iCollisionActor> CreateCollisionActor (CS::Collisions::iCollider* collider);
  virtual csPtr<CS::Collisions::iCollisionSector> CreateCollisionSector ();
  virtual CS::Collisions::iCollisionSector* FindCollisionSector (const char* name);
  virtual CS::Collisions::iCollisionSector* GetCollisionSector (const iSector* sceneSector);

  virtual void DecomposeConcaveMesh (CS::Collisions::iCollider* object,
    iMeshWrapper* mesh, bool simplify = false); 

  static void OpcodeReportV (int severity, const char* message, 
    va_list args);

public:
  static iObjectRegistry* rep_object_reg;
};
}
CS_PLUGIN_NAMESPACE_END (Opcode2)
#endif
