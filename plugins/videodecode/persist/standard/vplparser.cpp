/*
Copyright (C) 2010 by Alin Baciu

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

#include "cssysdef.h"

//#include <ctype.h>

#include "imap/ldrctxt.h"
#include "imap/services.h"
#include "imesh/genmesh.h"
#include "imesh/object.h"
#include "iutil/comp.h"
#include "iutil/document.h"
#include "iutil/eventh.h"
#include "iutil/object.h"
#include "iutil/objreg.h"
#include "iutil/plugin.h"
#include "iutil/stringarray.h"
#include "ivaria/reporter.h"

#include "vplparser.h"

SCF_IMPLEMENT_FACTORY (csVplParser)

static const char* msgidFactory = "crystalspace.videodecode.parser";

csVplParser::csVplParser (iBase* parent) :
scfImplementationType (this, parent),
_object_reg (0)
{
}

csVplParser::~csVplParser ()
{
}

bool csVplParser::Initialize (iObjectRegistry* r)
{
  csVplParser::_object_reg = r;
  _reporter = csQueryRegistry<iReporter> (_object_reg);
  _synldr = csQueryRegistry<iSyntaxService> (_object_reg);

  // Clear the internal language table
  _languages.DeleteAll ();

  // Initialize the xml tokens
  InitTokenTable (xmltokens);
  return true;
}

csPtr<iBase> csVplParser::Parse (iDocumentNode* node, iStreamSource*,
                                 iLoaderContext* ldr_context, iBase* context)
{
  csRef<iDocumentNodeIterator> it = node->GetNodes ();

  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    const char* value = child->GetValue ();
    csStringID id = xmltokens.Request (value);
    switch (id)
    {
    case XMLTOKEN_MEDIA:
      {
        const char* type = child->GetAttributeValue ("type");
        if (type == 0)
        {
          _synldr->ReportError (msgidFactory, child,
                               "No type defined while loading video");
          return 0;
        }

        if (strcmp (type,"theoraVideo")==0)
        {
          csRef<iPluginManager> mgr = csQueryRegistry<iPluginManager> (_object_reg);
          csRef<iMediaLoader> m_pThOggLoader = csLoadPlugin<iMediaLoader>
            (mgr, "crystalspace.videodecode.element.thogg");

          // Get the type of the media
          _mediaType = csString (child->GetAttributeValue ("type"));

          // Iterate through the rest of the nodes in the media file
          csRef<iDocumentNodeIterator> it2 = child->GetNodes ();

          while (it2->HasNext ())
          {
            csRef<iDocumentNode> child2 = it2->Next ();
            if (child2->GetType () != CS_NODE_ELEMENT) continue;
            const char* value2 = child2->GetValue ();
            csStringID id2 = xmltokens.Request (value2);
            switch (id2)
            {
            case XMLTOKEN_VIDEOSTREAM:
              {
                // Get the path for the media file
                const char* vidPath = child2->GetAttributeValue ("path");
                if (vidPath == 0)
                {
                  _synldr->ReportError (msgidFactory, child2,
                                       "No path defined while loading videostream");
                  return 0;
                }

                _mediaPath = csString (vidPath);
              }
            case XMLTOKEN_AUDIOSTREAM:
              {
                // Get the info about the different audio streams available
                csRef<iDocumentNodeIterator> it3 = child2->GetNodes ();

                while (it3->HasNext ())
                {
                  csRef<iDocumentNode> child3 = it3->Next ();
                  if (child3->GetType () != CS_NODE_ELEMENT) continue;
                  const char* value3 = child3->GetValue ();
                  csStringID id3 = xmltokens.Request (value3);
                  switch (id3)
                  {
                  case XMLTOKEN_LANGUAGE:
                    {
                      // Store the name
                      const char* name = child3->GetAttributeValue ("name");
                      if (name == 0)
                      {
                        _synldr->ReportError (msgidFactory, child3,
                                             "No language name defined while loading audiostream");
                        return 0;
                      }
                      // and the path
                      const char* langPath = child3->GetAttributeValue ("path");
                      if (langPath == 0)
                      {
                        _synldr->ReportError (msgidFactory, child3,
                                             "No language path defined while loading audiostream");
                        return 0;
                      }

		      MediaLanguage buff (name, langPath);
                      _languages.Push (buff);
                    }
                  }
                }
              }
            }
          }

	  csRef<iMediaContainer> container = m_pThOggLoader->LoadMedia (_mediaPath);
	  for (size_t i = 0; i < _languages.GetSize (); i++)
	    container->AddLanguage (_languages[i]);

          return csPtr<iBase> (container);
        }
      }
    }
  }

  return 0;
}
