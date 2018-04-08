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
#include "cssysdef.h"
#include "csutil/cfgacc.h"
#include "csutil/scfstringarray.h"
#include "iutil/cfgfile.h"
#include "iutil/document.h"
#include "iutil/objreg.h"
#include "iutil/plugin.h"
#include "iutil/stringarray.h"
#include "iutil/vfs.h"

#include "trans.h"
#include "transldr_xml.h"

CS_PLUGIN_NAMESPACE_BEGIN(TransStd)
{

SCF_IMPLEMENT_FACTORY (csTranslator)

bool csTranslator::ReportError (const char* description, ...)
{
  va_list arg;
  va_start (arg, description);
  csReportV (object_reg, CS_REPORTER_SEVERITY_ERROR,
	     "crystalspace.translator.standard",
	     description, arg);
  va_end (arg);
  return false;
}

csTranslator::csTranslator (iBase* parent)
  : scfImplementationType (this, parent), object_reg (0)
{
}

csTranslator::~csTranslator ()
{
}

bool csTranslator::Initialize (iObjectRegistry *object_reg)
{
  csTranslator::object_reg = object_reg;

  // Load the configuration file and default language
  csConfigAccess cfg;
  cfg.AddConfig (object_reg, "/config/translator.cfg");
  language = cfg->GetStr ("Translator.Language", "en");

/*
  // If no language is found then try to find one from the system locale
  if (language.Empty ())
  {
    const char* syslang = 0;
    // Unix
    syslang = getenv ("LANG");
    // Windows
    //WCHAR wcBuffer [32];
    //if (GetSystemDefaultLocaleName (wcBuffer, 32) > 0)
    //{
    //  syslang = wcBuffer;
    //}
    csRef<iConfigIterator> it_alias (cfg->Enumerate ("Translator.Alias."));
    if (it_alias)
      while (it_alias->Next ())
      {
        csString keystr = it_alias->GetStr ();
        start = 0;
        pos = 0;
        do
        {
          pos = keystr.Find (" ", pos + 1);
          csString keyslice = keystr.Slice (start, pos - start);
          if (!strcmp (syslang, keyslice.GetData ()))
          {
            language = it_alias->GetKey (true);
            break;
          }
          start = pos + 1;
        }
        while (pos != (size_t)-1);
        if (pos != (size_t)-1)
          break;
      }
  }
*/

  return true;
}

const char* csTranslator::GetCurrentLanguage () const
{
  return language;
}

void csTranslator::SetCurrentLanguage (const char* language)
{
  this->language = language;
}

csPtr<iStringArray> csTranslator::GetFallbacks (const char* language) const
{
  csRef<iStringArray> fallbacks;
  fallbacks.AttachNew (new scfStringArray ());

  // Load the list of fallbacks from the configuration file
  csConfigAccess cfg;
  cfg.AddConfig (object_reg, "/config/translator.cfg");

  char fallbackname [24];
  strcpy (fallbackname, "Translator.Fallback.");
  strcat (fallbackname, language);
  csString fallback = cfg->GetStr (fallbackname);
  size_t start = 0;
  size_t pos = 0;

  do
  {
    pos = fallback.Find (" ", pos + 1);
    csString keyslice = fallback.Slice (start, pos - start);
    fallbacks->Push (keyslice.GetData ());
    start = pos + 1;
  }
  while (pos != (size_t)-1);

  return csPtr<iStringArray> (fallbacks);
}

bool csTranslator::LoadTranslation (const char* filepath, iLoaderPlugin* loader)
{
  // Open the file and parse its XML content
  csRef<iVFS> vfs = csQueryRegistry<iVFS> (object_reg);
  if (!vfs.IsValid ()) return ReportError ("Could not find a iVFS");
  csRef<iFile> file (vfs->Open (filepath , VFS_FILE_READ));
  if (!file.IsValid ()) return ReportError ("The VFS file %s coult not be open",
					    CS::Quote::Single (filepath));

  csRef<iDataBuffer> data (file->GetAllData ());
  csRef<iDocumentSystem> documentSystem =
    csQueryRegistryOrLoad<iDocumentSystem> (object_reg, "crystalspace.documentsystem.xmlread");
  if (!documentSystem) return ReportError ("Could not find the document system plugin");
  csRef<iDocument> document = documentSystem->CreateDocument ();
  document->Parse (data->GetData ());
  csRef<iDocumentNode> root = document->GetRoot ();

  // Load the loader plugin if needed
  csRef<csTranslatorLoaderXml> loaderRef;
  if (!loader)
  {
    loaderRef.AttachNew (new csTranslatorLoaderXml (this));
    loaderRef->Initialize (object_reg);
    if (!loaderRef) return ReportError ("Could not locate the translator loader plugin");
    loader = loaderRef;
  }

  // Parse the data
  csRef<iBase> result = loader->Parse (root, nullptr, nullptr, this);
  return result.IsValid ();
}

const char* csTranslator::GetMsg (const char* src, bool fallback) const
{
  const csString* dst = messages.GetElementPointer (src);
  if (dst)
    return *dst;
  else
    return fallback ? src : nullptr;
}

void csTranslator::SetMsg (const char* src, const char* dst)
{
  messages.Put (src, dst);
}

}
CS_PLUGIN_NAMESPACE_END(TransStd)
