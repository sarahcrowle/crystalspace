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

#ifndef __CS_IVARIA_TRANSLATOR_H__
#define __CS_IVARIA_TRANSLATOR_H__

/**\file
 * Translator utility plugin interface
 */

#include "csutil/scf.h"

struct iLoaderPlugin;

/**
 * Translator interface. It can be used to translate one message to another using a
 * predefined dictionary and the current system language. The dictionary can be initialized
 * eg with successive calls to the method LoadTranslation().
 *
 * The configuration of the iTranslator is defined in the VFS file '/config/translator.cfg'.
 */
struct iTranslator : public virtual iBase
{
  SCF_INTERFACE (iTranslator, 1, 0, 0);

  /**
   * Get the current language. Languages are represented by their two letters
   * language code. The default language is 'en' (for English), but may be overwritten
   * within the configuration file.
   */
  virtual const char* GetCurrentLanguage () const = 0;

  /**
   * Set the current language. Languages are represented by their two letters
   * language code.
   */
  virtual void SetCurrentLanguage (const char* language) = 0;

  /**
   * Get the list of fallback languages defined for the given language. If no dictionary
   * is found when searching for the current language, then the fallback languages should
   * be used instead, in the given priority order.
   */
  virtual csPtr<iStringArray> GetFallbacks (const char* language) const = 0;

  /**
   * Utility method to load the translation data from the given file. Only the translation
   * data of the language returned by GetCurrentLanguage() will be loaded.
   * \param file The VFS path of the file to be loaded.
   * \param loader The translator loader plugin to be used. If none are provided
   * then the default one will be used.
   */
  virtual bool LoadTranslation (const char* file, iLoaderPlugin* loader = nullptr) = 0;

  /**
   * Get the translation of the given source message.
   *
   * There are two main fallback policies that you can use when the given message is not found
   * in the dictionary: either use the source message as the fallback, or implement by yourself
   * a more complex behavior. The first policy is the simplest to implement but has several
   * drawbacks:
   * - It won't allow the use of synonyms, that is, one sentence that can have several
   * different translations depending on the context.
   * - It is not memory efficient since there are no limits on the size of the key messages,
   * and they can therefore turn pretty long. 
   * - It is not flexible since every time you tweak a source message, you would need to
   * report the exact same changes in all translation dictionaries.
   *
   * \param src The source message to be used as the key of translation.
   * \param fallback Whether the value returned should fallback to \a src or to
   * \a nullptr if the source message is not found in the translation dictionary.
   */
  virtual const char* GetMsg (const char* src, bool fallback = true) const = 0;
};

#endif // __CS_IVARIA_TRANSLATOR_H__
