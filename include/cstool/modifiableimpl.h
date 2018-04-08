/*
  Copyright (C) 2012-2013 by Christian Van Brussel
  Copyright (C) 2012 by Andrei Barsan

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
#ifndef __CSUTIL_MODIFIABLE_IMPL_H
#define __CSUTIL_MODIFIABLE_IMPL_H

#include "iutil/modifiable.h"
#include "iutil/objreg.h"
#include "csutil/refarr.h"
#include "csutil/regexp.h"
#include "csutil/scfstringarray.h"
#include "csutil/stringarray.h"

using namespace CS::Utility;

namespace CS {
namespace Utility {

// TODO: using such macros is both tiresome and error-prone.
// Maybe use instead a XML description + a compiler for the code generation?
#define MODIF_DECLARE(count)						\
  virtual inline size_t GetTotalParameterCount () const { return count; } \
  csRefArray<iModifiableListener> listeners;				\
  virtual inline void AddListener (iModifiableListener* listener) {listeners.Push (listener);} \
  virtual inline void RemoveListener (iModifiableListener* listener) {listeners.Delete (listener);}
  
#define MODIF_GETDESCRIPTION_BEGIN(label, name)				\
  virtual csPtr<iModifiableDescription> GetDescription (iObjectRegistry* object_reg) const \
  {									\
    BaseModifiableDescription* description = new BaseModifiableDescription (label, name); \
    csRef<BaseModifiableParameter> parameter;				\
    csRef<iModifiableConstraint> constraint;				\
    csRef<iStringSet> strings =						\
      csQueryRegistryTagInterface<iStringSet> (object_reg, "crystalspace.shared.stringset"); \
    csStringID id;

#define MODIF_GETDESCRIPTION_END()		\
    return description;				\
}

#define MODIF_GETDESCRIPTION(type, label, name, desc)			\
  parameter.AttachNew (new BaseModifiableParameter (CSVAR_##type, strings->Request (label), label, name, desc)); \
  description->Push (parameter);

#define MODIF_GETDESCRIPTION_C(type, label, name, desc, constr)		\
  constraint.AttachNew (new constr);					\
  parameter.AttachNew (new BaseModifiableParameter (CSVAR_##type, strings->Request (label), label, name, desc, constraint)); \
  description->Push (parameter);

#define MODIF_GETDESCRIPTION_CENUM_DECLARE()	\
  {csRef<ModifiableConstraintEnum> constraint;		\
  constraint.AttachNew (new ModifiableConstraintEnum);

#define MODIF_GETDESCRIPTION_CENUM_PUSH(value, desc)	\
  constraint->PushValue (value, desc);

#define MODIF_GETDESCRIPTION_CENUM(type, label, name, desc)		\
  parameter.AttachNew (new BaseModifiableParameter (CSVAR_##type, strings->Request (label), label, name, desc, constraint)); \
  description->Push (parameter);}

#define MODIF_GETDESCRIPTION_PARENT(parent)				\
  {									\
    csRef<iModifiableDescription> child = parent::GetDescription (object_reg); \
    iStringArray* resources = child->GetResources ();			\
    for (size_t i = 0; i < resources->GetSize (); i++)			\
      description->Push (resources->Get (i));				\
    BaseModifiableDescription* csChild =				\
      static_cast<BaseModifiableDescription*> ((iModifiableDescription*) child); \
    description->Push (csChild);					\
  }

#define MODIF_GETDESCRIPTION_CHILD_BEGIN(label, name)			\
  {									\
    csRef<BaseModifiableDescription> child;				\
    child.AttachNew (new BaseModifiableDescription (label, name));	\
    description->Push (child);						\
    {									\
      BaseModifiableDescription* description = child;

#define MODIF_GETDESCRIPTION_CHILD_END()	\
  }}

#define MODIF_GETDESCRIPTION_RESOURCE(resource)	\
  description->Push (resource);

#define MODIF_GETPARAMETERVALUE_BEGIN()				\
  virtual void GetParameterValue (size_t index, csVariant& value) const	\
  {								\
    switch (index)						\
    {

#define MODIF_GETPARAMETERVALUE_END()		\
  default:					\
    break;					\
}						\
}

#define MODIF_GETPARAMETERVALUE(id, type, val)	\
  case id:					\
    value.Set##type (val);			\
    return;

#define MODIF_GETPARAMETERVALUE_F(id, type, func)	\
  case id:						\
    value.Set##type (func ());				\
    return;

#define MODIF_GETPARAMETERVALUE_PARENT_BEGIN()	\
  default:					\
    break;					\
}						\
  size_t offset = GetTotalParameterCount ();	\
  size_t count;

#define MODIF_GETPARAMETERVALUE_PARENT_END()	\
  }

#define MODIF_GETPARAMETERVALUE_PARENT(parent)		\
  count = parent::GetTotalParameterCount ();		\
  if (index - offset < count)				\
  {							\
    parent::GetParameterValue (index - offset, value);	\
    return;						\
  }							\
  offset += count;					\

#define MODIF_SETPARAMETERVALUE_BEGIN()				\
  virtual bool SetParameterValue (size_t index, const csVariant& value)	\
  {								\
    bool changed = true;					\
    switch (index)						\
    {

// TODO: Have a more memory efficient listener management, eg through a general
// manager accessible from the registry
#define MODIF_SETPARAMETERVALUE_END()					\
    default:								\
      changed = false;							\
  }									\
    if (changed) for (size_t i = 0; i < listeners.GetSize (); i++)	\
		   listeners[i]->ValueChanged (this, index);		\
    return changed;							\
  }

#define MODIF_SETPARAMETERVALUE(id, type, val)	\
    case id:					\
      val = value.Get##type ();			\
      break;

#define MODIF_SETPARAMETERVALUE_F(id, type, func)	\
    case id:						\
      func (value.Get##type ());			\
      break;

#define MODIF_SETPARAMETERVALUE_ENUM(id, type, val, enumt)	\
    case id:							\
      val = (enumt) value.Get##type ();				\
      break;

#define MODIF_SETPARAMETERVALUE_PARENT_BEGIN()	\
    default:					\
      changed = false;				\
  }						\
    size_t offset = GetTotalParameterCount ();	\
    size_t count;

#define MODIF_SETPARAMETERVALUE_PARENT_END()				\
    if (changed) for (size_t i = 0; i < listeners.GetSize (); i++)	\
		   listeners[i]->ValueChanged (this, index);		\
    return changed;							\
  }

#define MODIF_SETPARAMETERVALUE_PARENT(parent)			\
    if (!changed)						\
    {								\
      count = parent::GetTotalParameterCount ();		\
      if (index - offset < count)				\
      {								\
	parent::SetParameterValue (index - offset, value);	\
	changed = true;						\
      }								\
      offset += count;						\
    }

/**
 * Implementation of some of the most common CS::Utility::iModifiableParameter usage. 
 * Stores the parameter's name, description, type, ID and an optional constraint.
 */
class BaseModifiableParameter
: public scfImplementation1<BaseModifiableParameter, iModifiableParameter> 
{
public:
  BaseModifiableParameter (csVariantType type, csStringID id, const char* label,
			      const char* name, const char* description,
			      iModifiableConstraint* constraint = nullptr)
    : scfImplementationType (this),
    id (id),
    label (label),
    name (name),
    description (description),
    type (type),
    constraint (constraint)
    {}

  /// Default implementation for CS::Utility::iModifiableParameter::GetID()
  virtual csStringID GetID () const
  { return id; }

  /// Default implementation for CS::Utility::iModifiableParameter::GetLabel()
  virtual const char* GetLabel () const
  { return label; }

  /// Default implementation for CS::Utility::iModifiableParameter::GetName()
  virtual const char* GetName () const
  { return name; }

  /// Default implementation for CS::Utility::iModifiableParameter::GetDescription()
  virtual const char* GetDescription () const
  { return description; }

  /// Default implementation for CS::Utility::iModifiableParameter::GetType()
  virtual csVariantType GetType () const 
  { return type; }

  /// Set the constraint on this parameter
  virtual void SetConstraint (iModifiableConstraint* constraint)
  { this->constraint = constraint; }

  /// Default implementation for CS::Utility::iModifiableParameter::GetConstraint()
  virtual iModifiableConstraint* GetConstraint () const 
  { return constraint; }

protected:
  csStringID id;
  csString label;
  csString name;
  csString description;
  csVariantType type;
  csRef<iModifiableConstraint> constraint;
};

/**
 * Basic implementation of CS::Utility::iModifiableDescription, suitable for most uses. 
 * Simply holds a csRefArray of CS::Utility::iModifiableParameter and implements 
 * GetParameterCount() and GetParameter().
 */
class BaseModifiableDescription
: public scfImplementation1<BaseModifiableDescription, iModifiableDescription>
{
public:
  BaseModifiableDescription (const char* label, const char* name) :
  scfImplementationType (this), label (label), name (name)
  {
    resources.AttachNew (new scfStringArray ());
  }

  /// Default implementation for CS::Utility::iModifiableDescription::GetLabel()
  virtual const char* GetLabel () const
  { return label; }

  /// Default implementation for CS::Utility::iModifiableDescription::GetName()
  virtual const char* GetName () const
  { return name; }

  /// Default implementation for CS::Utility::iModifiableDescription::GetParameterCount()
  virtual size_t GetParameterCount () const
  { return parameters.GetSize (); }

  /// Default implementation for CS::Utility::iModifiableDescription::GetTotalParameterCount()
  virtual size_t GetTotalParameterCount () const
  {
    size_t total = parameters.GetSize ();
    for (size_t i = 0; i < children.GetSize (); i++)
      total += children[i]->GetTotalParameterCount ();
    return total;
  }

  /// Default implementation for CS::Utility::iModifiableDescription::GetParameter(csStringID)
  virtual iModifiableParameter* GetParameter (csStringID id) const 
  {
    for (size_t i = 0; i < parameters.GetSize (); i++)
      if (parameters.Get (i)->GetID () == id)
        return parameters.Get (i);

    for (size_t i = 0; i < children.GetSize (); i++)
    {
      iModifiableParameter* parameter = children[i]->GetParameter (id);
      if (parameter) return parameter;
    }

    return nullptr;
  }

  /// Default implementation for CS::Utility::iModifiableDescription::GetParameter(size_t)
  virtual iModifiableParameter* GetParameter (size_t index) const
  {
    return GetParameterInternal (index);
  }

  /// Default implementation for CS::Utility::iModifiableDescription::FindParameter()
  virtual size_t FindParameter (csStringID id) const
  {
    for (size_t i = 0; i < parameters.GetSize (); i++)
      if (parameters.Get (i)->GetID () == id)
        return i;

    for (size_t i = 0; i < children.GetSize (); i++)
    {
      size_t index = children[i]->FindParameter (id);
      if (index != (size_t) ~0) return index;
    }

    return (size_t) ~0;
  }

  /// Add the given parameter to the list
  inline void Push (iModifiableParameter* param)
  {
    parameters.Push (param);
  }

  /// Default implementation for CS::Utility::iModifiableDescription::GetChildrenCount()
  virtual size_t GetChildrenCount () const
  {
    return children.GetSize ();
  }

  /// Default implementation for CS::Utility::iModifiableDescription::GetChild()
  virtual iModifiableDescription* GetChild (size_t index) const
  {
    return children[index];
  }

  /// Add the given description to the list of children
  inline void Push (BaseModifiableDescription* child)
  {
    children.Push (child);
  }

  virtual iStringArray* GetResources () const
  { return resources; }

  /// Add the given resource to the list
  inline void Push (const char* resource)
  {
    resources->Push (resource);
  }

private:
  virtual iModifiableParameter* GetParameterInternal (size_t& index) const
  {
    if (index < parameters.GetSize ())
      return parameters[index];

    index -= parameters.GetSize ();
    for (size_t i = 0; i < children.GetSize (); i++)
    {
      iModifiableParameter* parameter = children[i]->GetParameterInternal (index);
      if (parameter) return parameter;
    }

    return nullptr;
  }

protected:
  csString label;
  csString name;
  csRefArray<iModifiableParameter> parameters;
  csRefArray<BaseModifiableDescription> children;
  csRef<iStringArray> resources;
};


/**
 * Implements an enum constraint for a CSVAR_LONG CS::Utility::iModifiable field. Contains a list of
 * long values that are members of the respective enum, as well as their string labels,
 * for displaying in a combo box.
 */
class ModifiableConstraintEnum
  : public scfImplementation1<ModifiableConstraintEnum, iModifiableConstraintEnum>
{
public:
  /**
   * Constructor
   */
  ModifiableConstraintEnum ()
    : scfImplementationType (this)
  {
  }  

  virtual bool Validate (const csVariant* variant) const
  {
    // No point performing the check - the value was selected from
    // a combo box. And nobody is trying to hack the editor either.
    return true;
  }

  //-- CS::Utility::iModifiableConstraintEnum
  virtual size_t GetValueCount () const 
  {
    return labels.GetSize ();
  }

  virtual const csVariant& GetValue (size_t index) const
  {
    return values.Get (index);
  }

  virtual iModifiableConstraintType GetType () const
  {
    return MODIFIABLE_CONSTRAINT_ENUM;
  }

  //-- ModifiableConstraintEnum
  virtual void PushValue (const csVariant& value, const char* label)
  {
    values.Push (value);
    labels.Push (label);
  }

  virtual const char* GetLabel (size_t index) const
  {
    return labels[index];
  }

private:
  csStringArray labels;
  csArray<csVariant> values;
};

/**
 * Implementation of CS::Utility::iModifiableConstraintBounded. See its documentation for a bit more help.
 * Currently works for the following value types:
 * CSVAR_FLOAT, CSVAR_LONG, CSVAR_VECTOR2, CSVAR_VECTOR3, CSVAR_VECTOR4
 */
class ModifiableConstraintBounded : public scfImplementation1<ModifiableConstraintBounded, 
  iModifiableConstraintBounded>
{
public:
  /// Initializes this constraint with both a min and a max value
  ModifiableConstraintBounded (const csVariant& min, const csVariant& max)
    : scfImplementationType (this),
    min (new csVariant (min)),
    max (new csVariant (max))
    {
      CheckTypes ();
    }

  /// Initializes the constraint to have just a maximum value
  ModifiableConstraintBounded (const csVariant& max)
    : scfImplementationType (this),
    min (nullptr),
    max (new csVariant (max))
    {
      CheckTypes ();
    }

  ~ModifiableConstraintBounded ()
  {
    delete min;
    delete max;
  }

  virtual void SetMinimum (csVariant* min)
  {
    delete this->min;
    this->min = min;
    CheckTypes ();
  }

  virtual void SetMaximum (csVariant* max)
  {
    delete this->max;
    this->max = max;
    CheckTypes ();
  }

  //-- CS::Utility::iModifiableConstraint

  virtual iModifiableConstraintType GetType () const 
  {
    return MODIFIABLE_CONSTRAINT_BOUNDED;
  }

  bool Validate (const csVariant* variant) const 
  {
    if (min != nullptr)
      CS_ASSERT_MSG ("Bounds must be of the same type as the variant", 
		     variant->GetType () == min->GetType ());

    if (max != nullptr)
      CS_ASSERT_MSG ("Bounds must be of the same type as the variant", 
		     variant->GetType () == max->GetType ());

    switch (variant->GetType ())
    {
    case CSVAR_FLOAT:
      if (max != nullptr) {
        if (variant->GetFloat () > max->GetFloat ()) return false;
      }
      if (min != nullptr) {
        if (variant->GetFloat () < min->GetFloat ()) return false;
      }
      break;

    case CSVAR_LONG:
      if (max != nullptr) {
        if (variant->GetLong () > max->GetLong ()) return false;
      }
      if (min != nullptr) {
        if (variant->GetLong () < min->GetLong ()) return false;
      }
      break;

    case CSVAR_VECTOR2:
      if (max != nullptr) {
        if (variant->GetVector2 ().x > max->GetVector2 ().x
            || variant->GetVector2 ().y > max->GetVector2 ().y
	  )
          return false;
      }
      if (min != nullptr) {
        if (variant->GetVector2 ().x < min->GetVector2 ().x
            || variant->GetVector2 ().y < min->GetVector2 ().y
	  )
          return false;
      }
      break;

    case CSVAR_VECTOR3:
      if (max != nullptr) {
        if (variant->GetVector3 ().x > max->GetVector3 ().x
	    || variant->GetVector3 ().y > max->GetVector3 ().y
	    || variant->GetVector3 ().z > max->GetVector3 ().z
	    || variant->GetVector3 ().z > max->GetVector3 ().z
	  )
          return false;
      }
      if (min != nullptr) {
        if (variant->GetVector3 ().x < min->GetVector3 ().x
	    || variant->GetVector3 ().y < min->GetVector3 ().y
	    || variant->GetVector3 ().z < min->GetVector3 ().z
	  )
          return false;
      }
      break;

    case CSVAR_VECTOR4:
      if (max != nullptr) {
        if (variant->GetVector4 ().x > max->GetVector4 ().x
	    || variant->GetVector4 ().y > max->GetVector4 ().y
	    || variant->GetVector4 ().z > max->GetVector4 ().z
	    || variant->GetVector4 ().w > max->GetVector4 ().w
	  )
          return false;
      }
      if (min != nullptr) {
        if (variant->GetVector4 ().x < min->GetVector4 ().x
	    || variant->GetVector4 ().y < min->GetVector4 ().y
	    || variant->GetVector4 ().z < min->GetVector4 ().z
	    || variant->GetVector4 ().w > max->GetVector4 ().w
          )
          return false;
      }

    default:
      // TODO
      break;
    }

    return true;
  }

  //-- CS::Utility::iModifiableConstraintBounded

  bool HasMinimum () const 
  {
    return min != nullptr;
  }

  bool HasMaximum () const 
  {
    return max != nullptr;
  }

  csVariant& GetMinimum () const 
  {
    return *min;
  }

  csVariant& GetMaximum () const
  {
    return *max;
  }

private:
  csVariant *min, *max;

  /// Some helpful assertions to make sure no funny stuff is going on
  void CheckTypes () {
    if (min != nullptr && max != nullptr)
      CS_ASSERT_MSG ("Bounds must be of the same type", min->GetType () == max->GetType ());

    csVariantType t;
    if (min != nullptr) {
      t = min->GetType ();
    } else if (max != nullptr) {
      t = max->GetType ();
    } else {
      CS_ASSERT_MSG ("Both constraints can't be null!", false);
      return;
    }

    CS_ASSERT_MSG ("Invalid type for comparing...",
		   t == CSVAR_FLOAT || t == CSVAR_LONG || t == CSVAR_VECTOR2 
		   ||  t == CSVAR_VECTOR3 || t == CSVAR_VECTOR4);
    (void)t;
  }
};

/**
 * Attached to an CS::Utility::iModifiable parameters, verifies that the value entered within
 * is always a VFS file, not a path or a directory.
 */
class ModifiableConstraintVFSFile : public scfImplementation1<ModifiableConstraintVFSFile, iModifiableConstraint>
{
  // TODO: a single class for all VFS constraints
public:
  ModifiableConstraintVFSFile () 
    : scfImplementationType (this)
  {
    // Should match anything that's got a special delimiter in it
    matcher = new csRegExpMatcher ("[^][[:alnum:]_ ,~!@#%.{}$-]");
  }

  virtual ~ModifiableConstraintVFSFile () 
  {
    delete matcher;
  }

  iModifiableConstraintType GetType () const
  {
    return MODIFIABLE_CONSTRAINT_VFS_FILE;
  }

  bool Validate (const csVariant* variant) const
  {
    CS_ASSERT (variant->GetType () == CSVAR_STRING);
    csRegExpMatchError result = matcher->Match (variant->GetString ());
    // It's ok as long as not special delimiters are found
    return result == csrxNoMatch;
  }

private:
  csRegExpMatcher* matcher;
};

/**
 * Attached to an CS::Utility::iModifiable parameters, verifies that the value entered within
 * is always a VFS directory, relative or absolute.
 */
class ModifiableConstraintVFSDir : public scfImplementation1<ModifiableConstraintVFSDir, iModifiableConstraint>
{
public:
  ModifiableConstraintVFSDir ()
    : scfImplementationType (this)
  {
    // Just like the file matcher, only allows colons and forward slashes
    matcher = new csRegExpMatcher ("[^][[:alnum:]_ ,~!@#%.{}$/-]");
  }

  virtual ~ModifiableConstraintVFSDir () 
  {
    delete matcher;
  }

  iModifiableConstraintType GetType () const
  {
    return MODIFIABLE_CONSTRAINT_VFS_DIR;
  }

  bool Validate (const csVariant* variant) const
  {
    CS_ASSERT (variant->GetType () == CSVAR_STRING);
    return matcher->Match (variant->GetString ()) == csrxNoMatch;
  }

private:
  csRegExpMatcher* matcher;
};


/**
 * Attached to an CS::Utility::iModifiable parameters, verifies that the value entered within
 * is always a full VFS path - a directory and a file, relative or absolute.
 */
class ModifiableConstraintVFSPath : public scfImplementation1<ModifiableConstraintVFSPath, iModifiableConstraint>
{
public:
  ModifiableConstraintVFSPath ()
    : scfImplementationType (this)
  {
    // Just like the dir regex
    matcher = new csRegExpMatcher ("[^][[:alnum:]_ ,~!@#%.{}$/-]");
  }

  virtual ~ModifiableConstraintVFSPath () 
  {
    delete matcher;
  }

  iModifiableConstraintType GetType () const
  {
    return MODIFIABLE_CONSTRAINT_VFS_PATH;
  }

  bool Validate (const csVariant* variant) const
  {
    CS_ASSERT (variant->GetType () == CSVAR_STRING);
    return matcher->Match (variant->GetString ()) == csrxNoMatch;
  }

private:
  csRegExpMatcher* matcher;
};

/**
 * Can validate a text entry, using minimum/ maximum length and/or a regular expression.
 */
class ModifiableConstraintTextEntry : public scfImplementation1<ModifiableConstraintTextEntry, iModifiableConstraint>
{
public:
  ModifiableConstraintTextEntry (long maxLength = -1, long minLength = -1, const char* regex = 0)
    : scfImplementationType (this),
    minLength (minLength),
    maxLength (maxLength)
    {
      if (regex)
	matcher = new csRegExpMatcher (regex);
      else
	matcher = nullptr;
    }

  virtual ~ModifiableConstraintTextEntry () 
  {
    delete matcher;
  }

  iModifiableConstraintType GetType () const
  {
    return MODIFIABLE_CONSTRAINT_TEXT_ENTRY;
  }

  bool Validate (const csVariant* variant) const
  {
    CS_ASSERT (variant->GetType () == CSVAR_STRING);
    csString val = variant->GetString ();
/*
  if (minLength >= 0 && (val.Length () < minLength || val.Length () > maxLength))
  return false;

  if (maxLength >= 0 && (matcher != nullptr && matcher->Match (val) == csrxNoMatch))
  return false;
*/
    return true;
  }

private:
  csRegExpMatcher* matcher;
  long minLength, maxLength;
};

/**
 * Validates a CSVAR_LONG value, checking that its bits satisfy a given mask. When a bit
 * that's not part of the mask is set, the validation fails.
 */
class ModifiableConstraintBitMask : public scfImplementation1<ModifiableConstraintBitMask, iModifiableConstraint>
{
  ModifiableConstraintBitMask (long mask)
    : scfImplementationType (this),
    mask (mask)
    {
    }

  iModifiableConstraintType GetType () const
  {
    return MODIFIABLE_CONSTRAINT_BITMASK;
  }

  bool Validate (const csVariant* variant) const
  {
    CS_ASSERT_MSG ("Can only apply bitmasks on integer-type values.", variant->GetType () == CSVAR_LONG);
    return (variant->GetLong () & (~mask)) == 0;
  }

private:
  long mask;
};

} // namespace Utility
} // namespace CS

#endif // __CSUTIL_MODIFIABLE_IMPL_H
