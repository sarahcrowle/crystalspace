/*
    Copyright (C) 2006 by Dariusz Dawidowski

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
#ifndef __CS_TRANS_H__
#define __CS_TRANS_H__

#include "iutil/comp.h"
#include "csutil/csstring.h"
#include "csutil/hash.h"
#include "csutil/scf_implementation.h"

#include "ivaria/translator.h"

struct iObjectRegistry;

CS_PLUGIN_NAMESPACE_BEGIN(TransStd)
{

/**
 * Implementation of the iTranslator API.
 */

class csTranslator : public scfImplementation2<csTranslator, iTranslator,
	iComponent>
{
private:
  iObjectRegistry* object_reg;
  csString language;
  csHash<csString, csString> messages;

public:
  csTranslator (iBase* parent);
  virtual ~csTranslator ();

  //-- iComponent
  virtual bool Initialize (iObjectRegistry *object_reg);

  //-- iTranslator
  virtual const char* GetCurrentLanguage () const;
  virtual void SetCurrentLanguage (const char* language);
  virtual csPtr<iStringArray> GetFallbacks (const char* language) const;

  virtual bool LoadTranslation (const char* file, iLoaderPlugin* loader = nullptr);

  virtual const char* GetMsg (const char* src, bool fallback = true) const;
  void SetMsg (const char* src, const char* dst);

private:
  bool ReportError (const char* description, ...);
};

}
CS_PLUGIN_NAMESPACE_END(TransStd)

#endif // __CS_TRANS_H__
