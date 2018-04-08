/*
    Copyright (C) 2012 by Frank Richter

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

#ifndef __CSUTIL_WXAPPARGCONVERT_H__
#define __CSUTIL_WXAPPARGCONVERT_H__

/**\file
 * Helper to convert wxApp command line arguments into CS' encoding
 */

#include <wx/string.h>

namespace CS
{
  namespace WX
  {
    /**
     * Helper to convert wxApp command line arguments into CS' encoding.
     * Usage:
     * \code
     * // Construct with wxApp argc/argc members
     * CS:WX::AppArgConvert argHelper (app->argc, app->argv);
     * // Pass converted arguments to CS.
     * csInitializer::CreateEnvironment (argHelper.csArgc(), argHelper.csArgv());
     * \endcode
     */
    class AppArgConvert
    {
      int argc;
      const char* const* argv;
    public:
      /**
       * Construct argument helper.
       * \param argc \c argc member of the \c wxApp instance.
       * \param argv \c argv member of the \c wxApp instance.
       */
      AppArgConvert (int argc, const wxChar* const* argv)
        : argc (argc)
      {
      #if defined(wxUSE_UNICODE) && wxUSE_UNICODE
        char** csargv;
        csargv = (char**)cs_malloc(sizeof(char*) * argc);
        for(int i = 0; i < argc; i++) 
        {
          csargv[i] = strdup (wxString(argv[i]).mb_str().data());
        }
        this->argv = csargv;
      #else
        this->argv = argv;
      #endif
      }
      ~AppArgConvert ()
      {
      #if defined(wxUSE_UNICODE) && wxUSE_UNICODE
        for(int i = 0; i < argc; i++) 
        {
          free (const_cast<char*> (argv[i]));
        }
        cs_free (const_cast<const char**> (argv));
      #endif
      }

      /// Return the \c argc value for passing to CS.
      int csArgc() const
      { return argc; }
      /// Return the \c argv value for passing to CS.
      const char* const* csArgv() const
      { return argv; }
    };
  } // namespace WX
} // namespace CS

#endif // __CSUTIL_WXAPPARGCONVERT_H__

