/*
    Copyright (C) 2003 by Jorrit Tyberghein

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

#ifndef __CS_MESHLOD_H__
#define __CS_MESHLOD_H__

#include "csutil/array.h"
#include "csutil/refcount.h"
#include "csutil/scf_implementation.h"
#include "iengine/lod.h"
#include "iengine/mesh.h"
#include "iengine/sharevar.h"

struct iMeshWrapper;
struct iMeshFactoryWrapper;

/**
 * A listener to listen to the variables.
 */
class csLODListener : public scfImplementation1<csLODListener,
                                                iSharedVariableListener>
{
public:
  csLODListener (float* variable)
    : scfImplementationType (this), variable (variable)
  {
  }

  virtual ~csLODListener() {}

  virtual void VariableChanged (iSharedVariable* var)
  {
    *variable = var->Get ();
  }
private:
  float* variable;
};

/**
 * This class is used to represent the static lod levels of a
 * hierarchical mesh.
 */
class csStaticLODMesh : public scfImplementation1<csStaticLODMesh,
                                                  iLODControl>
{
private:
  /// Static lod's. New-style version.
  csRefArray<iMeshObject> objects_for_lod;

  /// Function for lod.
  float lod_m, lod_a, lod_f;
  /// Or using variables.
  csRef<iSharedVariable> lod_varm;
  csRef<iSharedVariable> lod_vara;
  csRef<iSharedVariable> lod_varf;
  csRef<csLODListener> lod_varm_listener;
  csRef<csLODListener> lod_vara_listener;
  csRef<csLODListener> lod_varf_listener;

  void ClearLODListeners ();
  void ClearLODFListeners ();

public:
  /// constructor
  csStaticLODMesh ();
  virtual ~csStaticLODMesh ();

  virtual void SetLOD (float m, float a);
  virtual void GetLOD (float& m, float& a) const;
  virtual void SetLOD (iSharedVariable* varm, iSharedVariable* vara);
  virtual void GetLOD (iSharedVariable*& varm, iSharedVariable*& vara) const
  {
    varm = lod_varm;
    vara = lod_vara;
  }
  virtual int GetLODPolygonCount (float lod) const;

  void SetLODFade (float f);
  void GetLODFade (float& f) const;
  void SetLODFade (iSharedVariable* varf);
  void GetLODFade (iSharedVariable*& varf) const
  { varf = lod_varf; }

  float GetLODValue (float distance) const
  {
    return lod_m * distance + lod_a;
  }

  /**
   * Add a new-style static lod mesh object.
   */
  void AddMeshObjectForLOD (iMeshObject* object)
  {
    objects_for_lod.Push (object);
  }

  /**
   * Get the mesh object for the numerical lod (new-style). Returns 0 for
   * highest detail level (0) as that is not kept in this structure.
   */
  iMeshObject* GetMeshObjectForLOD (int lod)
  {
    if (lod == 0) return 0;
    lod--;
    if (lod >= (int)objects_for_lod.GetSize ())
    {
      objects_for_lod.SetSize (lod+1);
    }
    return objects_for_lod[lod];
  }

  /**
   * Get the mesh array for a lod between 0 and 1 (new-style).
   * Returns 0 for highest detail level (0) as that is not kept in this structure.
   */
  iMeshObject* GetMeshObjectForLOD (float lod)
  {
    int l = (int)objects_for_lod.GetSize ()+1; // One more because highest detail is in wrapper.
    int idx = int (lod * l);
    if (idx < 0) idx = 0;
    else if (idx >= l) idx = l-1;
    if (idx == 0) return 0;
    idx--;
    return objects_for_lod[idx];
  }

  /// Get number of lod levels we have.
  int GetLODCount ()
  {
    return (int)objects_for_lod.GetSize ()+1;
  }
};

/**
 * This class is used to represent the static lod levels of a
 * hierarchical mesh factory. It is used as a template to create
 * a csStaticLODMesh instance.
 */
class csStaticLODFactoryMesh : public scfImplementation1<csStaticLODFactoryMesh,
                                                         iLODControl>
{
private:
  /// Function for lod.
  float lod_m, lod_a, lod_f;
  /// Or using variables.
  csRef<iSharedVariable> lod_varm;
  csRef<iSharedVariable> lod_vara;
  csRef<iSharedVariable> lod_varf;
  csRef<csLODListener> lod_varm_listener;
  csRef<csLODListener> lod_vara_listener;
  csRef<csLODListener> lod_varf_listener;

public:
  /// constructor
  csStaticLODFactoryMesh ();
  virtual ~csStaticLODFactoryMesh ();

  virtual void SetLOD (float m, float a);
  virtual void GetLOD (float& m, float& a) const;
  virtual void SetLOD (iSharedVariable* varm, iSharedVariable* vara);
  virtual void GetLOD (iSharedVariable*& varm, iSharedVariable*& vara) const
  {
    varm = lod_varm;
    vara = lod_vara;
  }
  virtual int GetLODPolygonCount (float /*lod*/) const { return 0; }

  void SetLODFade (float f);
  void GetLODFade (float& f) const;
  void SetLODFade (iSharedVariable* varf);
  void GetLODFade (iSharedVariable*& varf) const
  { varf = lod_varf; }
};

#endif // __CS_MESHLOD_H__

