/*
  Copyright (C) 2005-2006 by Marten Svanfeldt

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

#ifndef __SCENE_H__
#define __SCENE_H__

#include "object.h"
#include "kdtree.h"
#include "light.h"
#include "material.h"

#include "lightcalculator.h"

namespace lighter
{
  class KDTree;
  class Scene;
  class Sector;
  class PhotonMap;
  class IrradianceCache;
  class SectorGroup;

  class Portal : public csRefCount
  {
  public:
    Portal ()
    {}

    void computePrimitives()
    {
      const size_t verticesCount = vertexData.positions.GetSize();

      // First compute the center of the polygon
      csVector3 center = vertexData.positions[0];
      for (size_t i=1; i<verticesCount;i++)
      {
        center += vertexData.positions[i];
      }

      center *= 1/verticesCount;
      vertexData.positions.Push(center);

      for (size_t i=0; i<verticesCount;i++)
      {
        Primitive prim(vertexData,0);

        Primitive::TriangleType t;
        t.a = i;
        t.b = verticesCount;
        t.c = (i+1)%verticesCount;

        prim.SetTriangle(t);
        prim.ComputePlane();
        prim.SetPortal(this);
        portalPrimitives.Push(prim);
      }
      
    }

    Sector* sourceSector;
    Sector* destSector;

    csReversibleTransform wrapTransform;
    PrimitiveArray portalPrimitives;
    ObjectVertexData vertexData;
    Vector3DArray worldVertices;
    csPlane3 portalPlane;
  };
  typedef csRefArray<Portal> PortalRefArray;



  class SectorProcessor : ThreadedCallable<SectorProcessor>,
                     public scfImplementation0<SectorProcessor>
  {
  public:
    SectorProcessor(iObjectRegistry *objReg);

    iObjectRegistry* GetObjectRegistry() const
    { return objReg; }

    THREADED_CALLABLE_DECL2(SectorProcessor,BuildKDTree,csThreadReturn,
      csRef<Sector>,sector,Statistics::Progress*, progress,THREADED,false,true);

    THREADED_CALLABLE_DECL2(SectorProcessor,BuildPhotonMaps,csThreadReturn,
      csRef<Sector>,sector,Statistics::Progress*, progress,THREADED,false,true);

    THREADED_CALLABLE_DECL2(SectorProcessor,BalancePhotonMaps,csThreadReturn,
      csRef<Sector>,sector,Statistics::Progress*, progress,THREADED,false,true)

    THREADED_CALLABLE_DECL7(SectorProcessor,ComputeObjectGroupLighting,csThreadReturn,
      csArray<csRef<Object> >*,objectsBatch, bool, enableRaytracer, bool, enablePhotonMapper,int,pass,
      Statistics::Progress*, progress,size_t, totalPrimitives, float, progressFactor,THREADED,false,true);

    THREADED_CALLABLE_DECL1(SectorProcessor,SaveLightmaps,csThreadReturn,
      csRef<SectorGroup>,sectorGroup,THREADED,false,true);

  private :
    iObjectRegistry* objReg;
  };

  // Representation of sector in our local setup
  class Sector : public csRefCount
  {
  public:
    Sector (Scene* scene)
        : kdTree (nullptr), scene (scene), sectorGroup(nullptr),
          numPhotonsToEmit(globalConfig.GetIndirectProperties().numPhotons),
          photonMap (nullptr), causticPhotonMap (nullptr),
          irradianceCache (nullptr)
    {}

    ~Sector ();

    // Initialize any extra data in the sector
    void Initialize (Statistics::Progress& progress);

    void PrepareLighting (Statistics::Progress& progress);

    // Build kd tree for Sector
    void BuildKDTree (Statistics::Progress& progress);

    void SavePhotonMap(const char* filename);

    void SaveCausticPhotonMap(const char* filename);

    void InitPhotonMap();

    void InitCausticPhotonMap();

    void InitPhotonNumber();

    // Release the photonmapping data this mean photonmap,
    // caustic map and irradiance cache
    void FreePhotonMappingData();

    void AddPhoton(const csColor power, const csVector3 pos,
      const csVector3 dir );

    void AddCausticPhoton(const csColor power, const csVector3 pos,
      const csVector3 dir );

    void ScalePhotons(const float scale);

    void ScaleCausticPhotons(const float scale);

    size_t GetPhotonCount();
    
    void BalancePhotons(Statistics::ProgressState& prog);

    bool SampleIRCache(const csVector3 point, const csVector3 normal,
                       csColor &irrad);

    csColor SamplePhoton(const csVector3 point, const csVector3 normal,
                         const float searchRad);

    void AddToIRCache(const csVector3 point, const csVector3 normal,
                      const csColor irrad, const float mean);

    // Return 2.0 * numThread group of object to be lighted
    csArray<csArray<csRef<Object> > *> getObjectBatches(size_t& primitivesCount);

    // Hash of all mesh names and materials

    //csHash <csString,csRef<RadMaterial>> materialHash;
    // All objects in sector
    ObjectHash allObjects;

    // All light sources (no PD lights)
    LightRefArray allNonPDLights;

    // All PD light sources
    LightRefArray allPDLights;

    // All portals in sector
    PortalRefArray allPortals;

    // KD-tree of all primitives in sector
    KDTree *kdTree;

    // Sector-name
    csString sectorName;

    Scene* scene;

    SectorGroup* sectorGroup;

    int numPhotonsToEmit;

  protected:

    // Compute the number of photons require for an average accuracy
    size_t getLightPhotonsNumber(Light* light, csBox3& sectorBBox);

    // Photon map for indirect lighting
    PhotonMap *photonMap;

    // Caustic Photon Map

    PhotonMap *causticPhotonMap;

    // Irradiance cache to speed up photon mapping
    IrradianceCache *irradianceCache;
  };
  typedef csHash<csRef<Sector>, csString> SectorHash;

  class SectorGroup : public csRefCount
  {
    public :
      SectorGroup(Sector* sector, SectorProcessor* processor);
      
      void addSector(csRef<Sector> sector);

      void Process(bool enableRaytracer, bool enablePhotonMapper,
        Statistics::Progress& progress);

      void SaveLightmaps();

      int computeNeededPhotonNumber();

      csRefArray<iThreadReturn> lightComputations;

    private :

      inline void BuildKDTreeAndPhotonMaps(bool buildPhotonMaps,Statistics::Progress& progress);

      inline void ComputeLighting(bool enableRaytracer, bool enablePhotonMapper,Statistics::Progress& progress);

      SectorHash sectors;
      csRef<SectorProcessor> sectorProcessor;
  };

  typedef csArray< csRef<SectorGroup> > SectorGroupRefArray;

  class Scene
  {
  public:
    Scene ();
    ~Scene ();
    
    void CleanUp (Statistics::Progress& progress);

    // Add a file for later loading
    void AddFile (const char* directory);

    // Load all files, and parse the loaded info
    bool LoadFiles (Statistics::Progress& progress);

    /* Save all files we've loaded. Will save any changed factory. */
    bool SaveWorldFactories (Statistics::Progress& progress);
    /* Save all files we've loaded. Will save any changed mesh. */
    bool SaveWorldMeshes (Statistics::Progress& progress);
    /* Save all files we've loaded. Writes out document to a temporary file. */
    bool FinishWorldSaving (Statistics::Progress& progress);

    // Copy the temporary file created in SaveWorld() over the actual world file.
    bool ApplyWorldChanges (Statistics::Progress& progress);
    // Generate specular direction maps
    bool GenerateSpecularDirectionMaps (Statistics::Progress& progress);
    // Write the generated lightmaps out.
    bool SaveLightmaps (Statistics::Progress& progress);
    // Save any mesh data that can only be saved after lighting.
    bool SaveMeshesPostLighting (Statistics::Progress& progress);
    // Clean up all data not needed after lighting.
    void CleanLightingData (Statistics::Progress& progress);

    // Data access
    inline ObjectFactoryHash& GetFactories () 
    { return radFactories; }

    inline const ObjectFactoryHash& GetFactories () const
    { return radFactories; }

    inline SectorHash& GetSectors () { return sectors; }

    inline const SectorHash& GetSectors () const { return sectors; }

    inline SectorGroupRefArray& GetSectorGroups () { return sectorGroups; }

    inline const SectorGroupRefArray& GetSectorGroups () const { return sectorGroups; }

    const LightmapPtrDelArray& GetLightmaps () const 
    { return lightmaps; }

    LightmapPtrDelArray& GetLightmaps () 
    { return lightmaps; }

    Lightmap* GetLightmap (uint lightmapID, size_t subLightmapNum, 
      Light* light = 0);

    csArray<LightmapPtrDelArray*> GetAllLightmaps ();
    
    enum { specDirectionMapCount = 3 };
    iTextureWrapper* GetSpecDirectionMapTexture (uint ID, int subNum);
    
    const RadMaterial* GetRadMaterial (iMaterialWrapper* matWrap) const
    {
      if (matWrap == 0) return 0;
      return *radMaterials.GetElementPointer (
        matWrap->QueryObject()->GetName());
    }

    /**
     * Helper class to perform some lightmap postprocessing
     * (exposure + ambient term).
     */
    class LightingPostProcessor
    {
    private:
      friend class Scene;
      Scene* scene;

      LightingPostProcessor (Scene* scene);
    public:
      //@{
      /// Apply exposure function
      void ApplyExposure (Lightmap* lightmap);
      void ApplyExposure (csColor* colors, size_t numColors);
      //@}

      
      //@{
      /**
       * Apply ambient term.
       * Ambient may be a hack to approximate indirect lighting, but then, 
       * as long as that is not supported, or disabled by the user later on, 
       * ambient can still serve a purpose.
       */
      void ApplyAmbient (Lightmap* lightmap);
      void ApplyAmbient (csColor* colors, size_t numColors);
      //@}
    };
    LightingPostProcessor lightmapPostProc;
  
    struct PropageState
    {
      csSet<csPtrKey<Portal> > seenPortals;
    };
    void PropagateLights (Sector* sector);

    // Materials
    MaterialHash radMaterials;

  protected:
    
    //  factories
    ObjectFactoryHash radFactories;
    
    // All sectors
    SectorHash sectors;
    SectorGroupRefArray sectorGroups;
    typedef csHash<Sector*, csPtrKey<iSector> > SectorOrigSectorHash;
    SectorOrigSectorHash originalSectorHash;

    CS::Threading::Mutex lightmapMutex;
    LightmapPtrDelArray lightmaps;
    typedef csHash<LightmapPtrDelArray*, csPtrKey<Light> > PDLightmapsHash;
    PDLightmapsHash pdLightmaps;
    DirectionMapPtrDelArray directionMaps[2];
    csStringArray directionMapBaseNames;
    csString GetDirectionMapFilename (uint ID, int subNum) const;
    
    struct DirectionMapTextures
    {
      iTextureWrapper* t[specDirectionMapCount];
      
      DirectionMapTextures() { memset (t, 0, sizeof (t)); }
    };
    csArray<DirectionMapTextures> directionMapTextures;

    struct LoadedFile
    {
    private:
      csRef<iDocument> document;
      bool docChangeable;
      bool changed;
    public:
      Configuration sceneConfig;
      csString levelName;
      csString fileName;
      csString directory; //VFS name, full path
      csSet<csString> texturesToClean;
      csSet<csString> texFileNamesToDelete;
      csArray<Object*> fileObjects;

    LoadedFile() : changed(false), sceneConfig (globalConfig) {}

      void SetDocument (iDocument* doc)
      {
        document = doc;
        docChangeable = doc && (doc->Changeable () == CS_CHANGEABLE_YES);
      }
      iDocument* GetDocument() const { return document; }
      iDocument* GetDocumentChangeable();

      bool IsChanged() const { return changed; }
      void SetChanged (bool c) { changed = c; }
    };

    // All files loaded into scene
    csArray<LoadedFile> sceneFiles;

    // Save functions
    void CollectDeleteTextures (iDocumentNode* textureNode,
                                csSet<csString>& filesToDelete);
    void BuildLightmapTextureList (LoadedFile* fileInfo, 
      csStringArray& texturesToSave);
    void CleanOldLightmaps (LoadedFile* fileInfo);
    void SaveSceneFactoriesToDom (LoadedFile* fileInfo, 
                                 Statistics::Progress& progress);
    void SaveSceneMeshesToDom (LoadedFile* fileInfo,
                               Statistics::Progress& progress);
    bool SaveSceneLibrary (csSet<csString>& savedFactories, 
                           const char* libFile, LoadedFile* fileInfo,
                           Statistics::Progress& progress, bool noModify);
    void HandleLibraryNode (csSet<csString>& savedFactories, 
                            iDocumentNode* node, LoadedFile* fileInfo,
                            Statistics::Progress& progress, bool noModify);

    enum SaveResult
    {
      svFailure, svSuccess, svRemoveItem
    };
    SaveResult SaveMeshFactoryToDom (csSet<csString>& savedObjects, 
                               iDocumentNode* factNode, LoadedFile* fileInfo);
    void SaveSectorToDom (iDocumentNode* sectorNode, LoadedFile* fileInfo,
                          Statistics::Progress& progress);
    SaveResult SaveMeshObjectToDom (csSet<csString>& savedObjects, 
                                    iDocumentNode *objNode, 
                                    Sector* sect, LoadedFile* fileInfo);

    void GenerateSpecularDirectionMaps (LoadedFile* fileInfo,
      Statistics::Progress& progress);
    void SaveSpecularDirectionMaps (LoadedFile* fileInfo,
      csStringArray& filenames, csStringArray& textureNames);

    csStringHash solidColorFiles;
    const char* GetSolidColorFile (LoadedFile* fileInfo, const csColor& col);
    void SaveLightmapsToDom (iDocumentNode* root, LoadedFile* fileInfo,
                             Statistics::Progress& progress);

    csPtr<iDataBuffer> SaveDebugData (LoadedFile& fileInfo, 
      iDataBuffer* sourceData, Statistics::Progress& progress);
    
    // Load functions
    bool ParseEngine (LoadedFile* fileInfo, Statistics::Progress& progress);
    bool ParseEngineAll (Statistics::Progress& progress);
    void ParseSector (LoadedFile* fileInfo, iSector *sector, 
      Statistics::Progress& progress);
    void ParsePortals (iSector *srcSect, Sector* sector);
    enum MeshParseResult
    {
      mpFailure, mpSuccess, mpNotAGenMesh
    };
    MeshParseResult ParseMesh (LoadedFile* fileInfo, Sector *sector,  
      iMeshWrapper *mesh, csRef<Object>& obj);
    MeshParseResult ParseMeshFactory (LoadedFile* fileInfo, 
      iMeshFactoryWrapper *factory, csRef<ObjectFactory>& radFact);
    bool ParseMaterial (iMaterialWrapper* material);
    void PropagateLight (Light* light, const csFrustum& lightFrustum, 
      PropageState& state);
    void PropagateLight (Light* light, const csFrustum& lightFrustum)
    { 
      PropageState state;
      PropagateLight (light, lightFrustum, state);
    }
    
    iCollection* GetCollection (iObject* obj);
    bool IsObjectFromBaseDir (iObject* obj, const char* baseDir);
    bool IsFilenameFromBaseDir (const char* filename, const char* baseDir);

    static csRef<iDocument> EnsureChangeable (iDocument* doc);
  };
}

#endif
