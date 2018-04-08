/*
Copyright (C) 2011 by Alin Baciu

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

#include "cssysdef.h"
#include "csutil/xmltiny.h"
#include "imap/reader.h"
#include "iutil/document.h"
#include "iutil/objreg.h"
#include "iutil/plugin.h"
#include "iutil/vfs.h"

#include "vplloader.h"

SCF_IMPLEMENT_FACTORY (csVplLoader)

csVplLoader::csVplLoader (iBase* parent) :
scfImplementationType (this, parent),
_object_reg (0)
{
}

csVplLoader::~csVplLoader ()
{
}

bool csVplLoader::Initialize (iObjectRegistry* r)
{
  _object_reg = r;

  return true;
}

csPtr<iMediaContainer> csVplLoader::LoadMedia (const char * pFileName, const char *pDescription)
{
  // Get the generic media xml parser
  csRef<iLoaderPlugin> parser = csQueryRegistry<iLoaderPlugin> (_object_reg);

  // Parse XML

  // Read the xml file and create the document
  csRef<iVFS> vfs = csQueryRegistry<iVFS> (_object_reg);
  csRef<iDocumentSystem> docSys = csPtr<iDocumentSystem> (new csTinyDocumentSystem ());
  csRef<iDocument> xmlDoc = docSys->CreateDocument ();
  csRef<iDataBuffer> xmlData = vfs->ReadFile (pFileName);

  if (!xmlData)
  {
    csReport (_object_reg, CS_REPORTER_SEVERITY_ERROR, QUALIFIED_PLUGIN_NAME,
      "Can't parse media: File '%s' not found.\n", pFileName);
    return csPtr<iMediaContainer> (nullptr);
  }

  // Start parsing
  if (xmlDoc->Parse (xmlData) == 0)
  {
    // Get the root
    csRef<iDocumentNode> node = xmlDoc->GetRoot ();

    // Tell the parser to parse the xml file
    csRef<iBase> result = parser->Parse (node,0,0,0);

    // Get the media container
    csRef<iMediaContainer> container = scfQueryInterface<iMediaContainer> (result);
    return csPtr<iMediaContainer> (container);
  }
  else
  {
    csReport (_object_reg, CS_REPORTER_SEVERITY_ERROR, QUALIFIED_PLUGIN_NAME,
      "Failed to parse '%s'.\n", pFileName);
    return csPtr<iMediaContainer> (nullptr);
  }

  return csPtr<iMediaContainer> (nullptr);
}

