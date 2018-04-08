/*
    Copyright (C) 2007 by Marten Svanfeldt

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

#ifndef __CSTOOL_MESHFILTER_H__
#define __CSTOOL_MESHFILTER_H__

#include "csutil/set.h"
#include "csutil/ref.h"
#include "iengine/mesh.h"

namespace CS
{
namespace Utility
{
  /// Filtering modes for a CS::Utility::MeshFilter
  enum MeshFilterMode
  {
    /**
     * The meshes in this filter must be excluded from the scene
     * when rendering
     */
    MESH_FILTER_EXCLUDE,  
    /**
     * Only the meshes in this filter must be included when rendering
     */
    MESH_FILTER_INCLUDE
  };

  /**
   * Structure holding filtering data in order to restrict the
   * list of meshes from a scene that must be rendered.
   *
   * This is basically a list of iMeshWrapper's that can be either 
   * included or excluded from rendering (depending on the selected
   * MeshFilterMode).
   *
   * Main ways to get pointers to this interface:
   * - iView::GetMeshFilter()
   */
  class CS_CRYSTALSPACE_EXPORT MeshFilter
  {
  public:
    MeshFilter();
    ~MeshFilter();

    /**
     * Add a mesh to this filter.
     * \param mesh The mesh to be added
     * \param addChildren Whether or not the children of this mesh
     * must be considered too.
     */
    void AddFilterMesh (iMeshWrapper* mesh, bool addChildren=false);
    /// Remove a mesh from this filter.
    void RemoveFilterMesh (iMeshWrapper* mesh);

    /**
     * Return wheter or not this mesh should be rendered.
     * \return \a true if the mesh should be excluded, \a false
     * otherwise.
     */
    bool IsMeshFiltered (iMeshWrapper* mesh) const;

    /**
     * Set the filtering mode for this filter. The default value is
     * CS::Utility::MESH_FILTER_EXCLUDE.
     */
    void SetFilterMode (MeshFilterMode mode)
    {
      filterMode = mode;
    }

    /// Get the filtering mode for this filter
    MeshFilterMode GetFilterMode () const
    {
      return filterMode;
    }

    /// Clear all meshes in this list
    void Clear ();

  private:
    MeshFilterMode filterMode;
    // FIXME: csRef<> prolly not a good idea
    csSet<csRef<iMeshWrapper> > filteredMeshes;
  };

}
}

#endif
