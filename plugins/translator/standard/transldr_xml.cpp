/*
    Copyright (C) 2006, 2007 by Dariusz Dawidowski
    Copyright (C) 2007 by Amir Taaki

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
#include "iutil/objreg.h"
#include "iutil/plugin.h"
#include "iutil/document.h"
#include "iutil/object.h"
#include "iutil/cfgmgr.h"
#include "iutil/stringarray.h"
#include "ivaria/translator.h"
#include "transldr_xml.h"
#include "trans.h"

CS_PLUGIN_NAMESPACE_BEGIN(TransStd)
{

SCF_IMPLEMENT_FACTORY (csTranslatorLoaderXml)

csTranslatorLoaderXml::csTranslatorLoaderXml (iBase* parent) :
	scfImplementationType (this, parent), object_reg (0)
{
  InitTokenTable (tokens);
}

csTranslatorLoaderXml::~csTranslatorLoaderXml ()
{
}

bool csTranslatorLoaderXml::Initialize (iObjectRegistry *object_reg)
{
  csTranslatorLoaderXml::object_reg = object_reg;
  synldr = csQueryRegistry<iSyntaxService> (object_reg);
  return true;
}

void csTranslatorLoaderXml::Report (int severity, const char* msg, ...)
{
  va_list arg;
  va_start (arg, msg);
  csRef<iReporter> rep = csQueryRegistry<iReporter> (object_reg);
  if (rep)
    rep->ReportV (severity, "crystalspace.translation.loader.xml", msg, arg);
  else
  {
    csPrintfV (msg, arg);
    csPrintf ("\n");
  }
  va_end (arg);
}

bool csTranslatorLoaderXml::Process (iDocumentNode* node, const char* lang)
{
  bool found_language = false;
  csRef<iDocumentNodeIterator> it1 = node->GetNodes ();
  while (lang && it1->HasNext ())
  {
    csRef<iDocumentNode> ch1 = it1->Next ();
    if (ch1->GetType () != CS_NODE_ELEMENT) continue;
    csStringID id1 = tokens.Request (ch1->GetValue ());
    switch (id1)
    {
      case XMLTOKEN_LANGUAGES:
	Process (ch1, lang);
	break;

      case XMLTOKEN_LANGUAGE:
      {
        const char* language = ch1->GetAttributeValue ("name");
        if (!language)
        {
          synldr->ReportError (
          	"crystalspace.translation.loader.xml",
          	node, "Translator: Missing %s attribute!",
		CS::Quote::Single ("name"));
          return false;
        }

        if (!strcmp (lang, language))
        {
          found_language = true;
          csRef<iDocumentNodeIterator> it2 = ch1->GetNodes ();
          while (it2->HasNext ())
          {
            csRef<iDocumentNode> ch2 = it2->Next ();
            if (ch2->GetType () != CS_NODE_ELEMENT) continue;
            csStringID id2 = tokens.Request (ch2->GetValue ());
            switch (id2)
            {
              case XMLTOKEN_MSG:
              {
                csRef<iDocumentNodeIterator> it3 = ch2->GetNodes ();
                const char* src = 0;
                const char* dst = 0;
                while (it3->HasNext ())
                {
                  csRef<iDocumentNode> ch3 = it3->Next ();
                  if (ch3->GetType () != CS_NODE_ELEMENT) continue;
                  csStringID id3 = tokens.Request (ch3->GetValue ());
                  switch (id3)
                  {
                    case XMLTOKEN_SRC:
                      src = ch3->GetContentsValue ();
                    break;
                    case XMLTOKEN_DST:
                      dst = ch3->GetContentsValue ();
                    break;
                    default:
                      if (synldr) synldr->ReportBadToken (ch3);
                      return false;
                  }
                }
                if ((src && dst) && (strcmp (src, dst)))
                	trans->SetMsg (src, dst);
              }
              break;
              default:
                if (synldr) synldr->ReportBadToken (ch2);
                return false;
            }
          }
        }
      }
      break;

      default:
        if (synldr) synldr->ReportBadToken (ch1);
        return false;
    }
  }

  return found_language;
}

csPtr<iBase> csTranslatorLoaderXml::Parse (iDocumentNode* node,
	iStreamSource*, iLoaderContext* ldr_context, iBase* context)
{
  if (!node) return csPtr<iBase> (nullptr);

  // Attempt to find a translator in the loading context
  csRef<iTranslator> translator;
  if (context)
  {
    translator = scfQueryInterface<iTranslator> (context);
    if (translator)
      trans = static_cast<csTranslator*> ((iTranslator*) translator);
  }

  // Attempt to find a translator in the global registry
  if (!trans)
  {
    translator = csQueryRegistry<iTranslator> (object_reg);
    if (translator)
      trans = static_cast<csTranslator*> ((iTranslator*) translator);
  }

  // Attempt to create a translator from scratch
  if (!trans)
  {
    trans.AttachNew (new csTranslator (context));

    if (!trans)
    {
      Report (CS_REPORTER_SEVERITY_ERROR, "Couldn't find any valid iTranslator instance!");
      return csPtr<iBase> (nullptr);
    }
/*
    // Unregister any previous translator before registering this one
    csRef<iTranslator> old = csQueryRegistry<iTranslator> (object_reg);
    if (old) object_reg->Unregister (old, "iTranslator");

    // TODO: who unregisters?
    if (!object_reg->Register (trans, "iTranslator"))
      Report (CS_REPORTER_SEVERITY_ERROR, "Couldn't register iTranslator!");
*/
  }

  // Process the file until some data is found for the default language
  // or for the fallback ones
  if (Process (node, trans->GetCurrentLanguage ()))
    return csPtr<iBase> (trans);

  csRef<iStringArray> fallbacks = trans->GetFallbacks (trans->GetCurrentLanguage ());
  for (size_t i = 0; i < fallbacks->GetSize (); i++)
    if (Process (node, fallbacks->Get (i)))
      return csPtr<iBase> (trans);

  return csPtr<iBase> (nullptr);
}

}
CS_PLUGIN_NAMESPACE_END(TransStd)
