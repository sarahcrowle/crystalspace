/*
    Copyright (C) 1998 by Jorrit Tyberghein

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
#ifndef __CS_IUTIL_PLUGINCONFIG_H__
#define __CS_IUTIL_PLUGINCONFIG_H__

/**\file
 * Plugin configuration interface.
 */

/**\addtogroup util
 * @{ */

#include "csutil/variant.h"

/// Description of a configuration option, to be used by the iPluginConfig interfaces
struct csOptionDescription
{
  /// Description index (or ID)
  int id;
  /// Short name of this option.
  csString name;
  /// Description for this option.
  csString description;
  /// Type to use for this option.
  csVariantType type;

  /// Constructor
  csOptionDescription () {}

  /**
   * Constructor
   * \param id Description index (or ID)
   * \param name Short name of this option.
   * \param description Description for this option.
   * \param type Type to use for this option.
   */
  csOptionDescription (int id, const char* name, const char* description, csVariantType type)
  : id (id), name (name), description (description), type (type) {}

  /**
   * Constructor
   * \param name Short name of this option.
   * \param description Description for this option.
   * \param type Type to use for this option.
   * \warning The \a id is initialized to 0 in this constructor!
   */
  csOptionDescription (const char* name, const char* description, csVariantType type)
  : id (0), name (name), description (description), type (type) {}

  ~csOptionDescription () {}
};

/**
 * Interface to a configurator object. If a SCF module
 * has an object implementing this interface then this can
 * be used to query or set configuration options.
 *
 * Main creators of instances implementing this interface:
 * - Some plugins implement this.
 * 
 * Main ways to get pointers to this interface:
 * - scfQueryInterface() from a plugin instance.
 * 
 * Main users of this interface:
 * - csCommandLineHelper
 * - csPluginManager
 */
struct iPluginConfig : public virtual iBase
{
  SCF_INTERFACE (iPluginConfig,2,1,0);

  /**
   * Get the description of the option of index \a idx. Return \a false if this
   * option does not exist, true otherwise.
   * \param index The index of the option
   * \param option The returned description of the option
   */
  virtual bool GetOptionDescription (int index, csOptionDescription* option) = 0;

  /**
   * Set the value of the option of index \a idx. Return \a false if this
   * option does not exist, true otherwise.
   * \param index The index of the option
   * \param value The new value to be set for the option
   */
  virtual bool SetOption (int index, csVariant* value) = 0;

  /**
   * Get the value of the option of index \a idx. Return \a false if this
   * option does not exist, true otherwise.
   * \param index The index of the option
   * \param value A variant where to store the value of the option
   */
  virtual bool GetOption (int index, csVariant* value) = 0;
};

/** @} */

#endif // __CS_IUTIL_PLUGINCONFIG_H__
