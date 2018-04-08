/*
    Copyright (C) 2012-2013 by Christian Van Brussel
    Copyright (C) 2012 by Andrei Barsan

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
#ifndef __IUTIL_MODIFIABLE_H__
#define __IUTIL_MODIFIABLE_H__

#include "csutil/scf_interface.h"
#include "csutil/variant.h"
#include "iutil/strset.h"

struct iObjectRegistry;

namespace CS {
namespace Utility {

struct iModifiable;
struct iModifiableConstraint;
struct iModifiableDescription;

//----------------- iModifiableParameter ---------------------

/**
 * Description of a specific parameter of a CS::Utility::iMovable.
 * \sa BaseModifiableParameter for a default implementation
 */
struct iModifiableParameter : public virtual iBase
{
  SCF_INTERFACE (iModifiableParameter, 1, 0, 0);

  /**
   * Get the ID of this parameter. This ID is built from the string returned by
   * GetLabel()
   */
  virtual csStringID GetID () const = 0;

  /**
   * Get the label of this parameter. A label identifies (supposedly uniquely)
   * this parameter.
   */
  virtual const char* GetLabel () const = 0;

  /**
   * Get the parameter's name
   * \note You might want to process this string by the translator.
   */
  virtual const char* GetName () const = 0;

  /**
   * Get the parameter's textual description
   * \note You might want to process this string by the translator.
   */
  virtual const char* GetDescription () const = 0;

  /**
   * Get the type of this parameter
   */
  virtual csVariantType GetType () const = 0;

  /**
   * Get this parameter's constraint, or nullptr if there are none.
   */
  virtual iModifiableConstraint* GetConstraint () const = 0;
};

//----------------- iModifiableDescription ---------------------

/**
 * The description of an CS::Utility::iModifiable object. It contains all the
 * information needed in order to expose and access the properties of a iModifiable.
 *
 * It can be used fo an automated access to an object, eg in order to generate a
 * graphical user interface (see the CSEditing library), to animate it over time, or
 * to save and load it.
 * 
 * A iModifiableDescription is created through iModifiable::GetDescription() if a user
 * application would like to access the object without knowing its external interface.
 * The iModifiableDescription will then allow to know how the object can be accessed
 * or represented in a graphical user environment.
 *
 * A iModifiableDescription can be structured hierachically (see GetChild() and
 * GetChildrenCount()) in order to gather logical subsets of parameters.
 *
 * \sa BaseModifiableDescription for a default implementation
 */
struct iModifiableDescription : public virtual iBase
{
  SCF_INTERFACE (iModifiableDescription, 1, 0, 0);

  /**
   * Get the label of this modifiable. A label identifies (supposedly uniquely) the
   * description of the modifiable.
   */
  virtual const char* GetLabel () const = 0;

  /**
   * Get the name of this modifiable
   * \note You might want to process this string by the translator.
   */
  virtual const char* GetName () const = 0;

  /**
   * Get the number of editable parameters of this description. This won't include
   * the parameters of the child descriptions.
   * \sa GetTotalParameterCount(), GetChild()
   */
  virtual size_t GetParameterCount () const = 0;

  /**
   * Get the total number of editable parameters of this description, that is
   * including the parameters of all children and grand-children.
   * \sa GetParameterCount(), GetChild()
   */
  virtual size_t GetTotalParameterCount () const = 0;

  /**
   * Get a parameter based on its ID.
   */
  virtual iModifiableParameter* GetParameter (csStringID id) const = 0;

  /**
   * Get a parameter based on its index.
   */
  virtual iModifiableParameter* GetParameter (size_t index) const = 0;

  /**
   * Find the index of a parameter based on its ID.
   * \return The index of the parameter, or (size_t) ~0 if it was not found.
   */
  virtual size_t FindParameter (csStringID id) const = 0;

  /**
   * Get the count of child descriptions. Child descriptions forms a hierarchical
   * structure of sets of parameters.
   */
  virtual size_t GetChildrenCount () const = 0;

  /**
   * Get the child description with the given index.
   */
  virtual iModifiableDescription* GetChild (size_t index) const = 0;

  /**
   * Get the list of resource entries of this description. Each entry is a path
   * containing additional data such as the translation files of this description.
   */
  virtual iStringArray* GetResources () const = 0;
};

//----------------- iModifiableListener ---------------------

/**
 * Listener for the notification of the state changes of a CS::Utility::iModifiable
 */
struct iModifiableListener : public virtual iBase
{
  SCF_INTERFACE (iModifiableListener, 1, 0, 0);

  /**
   * The value of the parameter of the given CS::Utility::iModifiable has changed.
   * \param modifiable The modifiable who's value has changed.
   * \param parameterIndex The index of the parameter that has changed.
   *
   * \note The new value of the parameter can be found eg through
   * CS::Utility::iModifiable::GetParameterValue().
   */
  // TODO: return bool to veto?
  virtual void ValueChanged (iModifiable* modifiable, size_t parameterIndex) = 0;
};

// ----------------- iModifiable ---------------------

/**
 * This interface is to be implemented by all objects willing to be modified automatically.
 * It provide access to the parameters of the CS objects, and to their type, description and
 * constraints. 
 *
 * A iModifiableDescription can be accessed through GetDescription() in order to obtain the
 * description of the list of parameters of this iModifiable.
 *
 * This mechanism allows for example programs such as cseditor to generate automatically
 * Graphical User Interfaces in order to modify their attributes. It can also be used by any
 * automated process such as an animation or a persistance system.
 *
 * \see iModifiableDescription
 * \see iModifiableParameter
 */
struct iModifiable : public virtual iBase 
{
  SCF_INTERFACE (iModifiable, 1, 0 ,0);

  /**
   * Returns this object's description.
   */
  virtual csPtr<iModifiableDescription> GetDescription (iObjectRegistry* object_reg) const = 0;

  /**
   * Returns the value of one of this object's parameters. 
   */
  virtual void GetParameterValue (size_t parameterIndex, csVariant& value) const = 0;

  /**
   * Sets a value for the parameter with the unique identifier id. 
   *
   * \return true if the value can be set, false if a property with
   * that index couldn't be found
   */
  virtual bool SetParameterValue (size_t parameterIndex, const csVariant& value) = 0;

  /**
   * Add a listener to the list.
   */
  virtual void AddListener (iModifiableListener* listener) = 0;

  /**
   * Remove the given listener from the list.
   */
  virtual void RemoveListener (iModifiableListener* listener) = 0;

  /**
   * This method is there only to help securing the current code generation. This method
   * should therefore be removed once a better code generation system would be implemented.
   */
  virtual size_t GetTotalParameterCount () const = 0;
};

//----------------- iModifiableConstraintType ---------------------

/**
 * List of types that a CS::Utility::iModifiableConstraint can be.
 */
enum iModifiableConstraintType
{
  /**
   * This constraint is a bounded constraint. It can be upcast to a
   * CS::Utility::iModifiableConstraintBounded.
   *
   * This constraint can only be set for parameters of type CSVAR_LONG,
   * CSVAR_FLOAT, CSVAR_VECTOR2, CSVAR_VECTOR3, or CSVAR_VECTOR4.
   */
  MODIFIABLE_CONSTRAINT_BOUNDED = 0,

  /**
   * This constraint is an enum. It can be upcast to a
   * CS::Utility::iModifiableConstraintEnum.
   *
   * This constraint can be set on all type of parameters.
   */
  MODIFIABLE_CONSTRAINT_ENUM,

  /**
   * This constraint defines a string referencing a VFS path to a file.
   *
   * This constraint can only be set for parameters of type CSVAR_STRING.
   *
   * \sa CS::Utility::ModifiableConstraintVFSFile for a default implementation
   */
  MODIFIABLE_CONSTRAINT_VFS_FILE,

  /**
   * This constraint defines a string referencing a VFS path to a directory.
   *
   * This constraint can only be set for parameters of type CSVAR_STRING.
   *
   * \sa CS::Utility::ModifiableConstraintVFSDir for a default implementation
   */
  MODIFIABLE_CONSTRAINT_VFS_DIR,

  /**
   * This constraint defines a string referencing a VFS path to either a file or a
   * directory.
   *
   * This constraint can only be set for parameters of type CSVAR_STRING.
   *
   * \sa CS::Utility::ModifiableConstraintVFSPath for a default implementation
   */
  MODIFIABLE_CONSTRAINT_VFS_PATH,

  /**
   * This constraint defines a single line string.
   *
   * This constraint can only be set for parameters of type CSVAR_STRING.
   */
  MODIFIABLE_CONSTRAINT_TEXT_ENTRY,

  /**
   * This constraint defines a string that uses to have a great size. It is
   * usually represented in more than one line of text.
   *
   * This constraint can only be set for parameters of type CSVAR_STRING.
   */
  MODIFIABLE_CONSTRAINT_TEXT_BLOB,

  /**
   * This constraint defines a mask of bits than can be set separately.
   *
   * This constraint can only be set for parameters of type CSVAR_LONG.
   */
  MODIFIABLE_CONSTRAINT_BITMASK
  //MODIFIABLE_CONSTRAINT_ARRAY_STATIC
};

//----------------- iModifiableConstraint ---------------------

/**
 * A constraint allows to limit the range of values that a parameter of a
 * CS::Utility::iMovable can take.
 *
 * Main ways to get pointers to this interface:
 * - CS::Utility::iModifiableParameter::GetConstraint()
 *
 * \sa iModifiable
 */
struct iModifiableConstraint : public virtual iBase
{
  SCF_INTERFACE (iModifiableConstraint, 1, 0, 0);

  /// Get the type of this constraint.
  virtual iModifiableConstraintType GetType () const = 0;
  
  /**
   * Return whether or not the value of this variant is valid according to this constraint.
   * The variant is supposed to be of a csVariantType compatible with this constraint.
   */
  virtual bool Validate (const csVariant* variant) const = 0;
};

//----------------- iModifiableConstraintBounded ---------------------

/**
 * A bounded constraint forces the value of a parameter to either stay under
 * a certain value, over a certain value, or between two values. 
 * \sa CS::Utility::ModifiableConstraintBounded for a default implementation
 */
struct iModifiableConstraintBounded : public virtual iModifiableConstraint
{
  /// Get whether or not the value of this parameter has a minimum value
  virtual bool HasMinimum () const = 0;

  /**
   * Get the minimum value of this parameter. The behavior is undefined if
   * HasMinimum() doesn't return true.
   */
  virtual csVariant& GetMinimum () const = 0;

  /// Get whether or not the value of this parameter has a maximum value
  virtual bool HasMaximum () const = 0;

  /**
   * Get the maximum value of this parameter. The behavior is undefined if
   * HasMaximum() doesn't return true.
   */
  virtual csVariant& GetMaximum () const = 0;
};

//----------------- iModifiableConstraintEnum ---------------------

/**
 * An enum constraint defines a list of values that a parameter can take.
 * No other values are allowed for this parameter.
 * \sa CS::Utility::ModifiableConstraintEnum for a default implementation
 */
struct iModifiableConstraintEnum : public virtual iModifiableConstraint
{
  /// Get the count of different values that are allowed
  virtual size_t GetValueCount () const = 0;

  /// Get the value of the given index.
  virtual const csVariant& GetValue (size_t index) const = 0;

  /**
   * Get the textual description of the value of the given index.
   * \note You might want to process this string by the translator.
   */
  virtual const char* GetLabel (size_t index) const = 0;
};

} // namespace Utility
} // namespace CS

#endif // __IUTIL_MODIFIABLE_H__
