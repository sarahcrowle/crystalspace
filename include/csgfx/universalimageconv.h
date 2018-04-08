/*
    Copyright(C) 2011 by Frank Richter

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

#ifndef __CSGFX_UNIVERSALIMAGECONV_H__
#define __CSGFX_UNIVERSALIMAGECONV_H__

namespace CS
{
  class StructuredTextureFormat;

  namespace Graphics
  {
    /**
     * Convert image/texture data between different image/texture formats.
     */
    struct CS_CRYSTALSPACE_EXPORT UniversalImageConverter
    {
      /// Conversion result
      enum Result
      {
        /// The conversion was successful.
        Success = 0,

        /**
         * The components in the input and output formats are incompatible
         * (e.g. a depth format is attempted to be converted to a color format).
         */
        ErrorIncompatibleComponents = -1,
        /// The input or output format was invalid
        ErrorInvalidFormat = -2,
        /// The input or output format is not supported
        ErrorFormatUnsupported = -3
      };
      /**
       * Convert image/texture data between different basic image/texture formats.
       *
       * Abilities:
       * * Conversion between formats of the same basic component set
       *   (RGBA, LA, DS) and conversion from L(A) to RGB(A).
       * * Changing the component sizes (eg convert from \c rgb8 to \c r5g6b5
       *   and vice versa).
       * * Converting between float and integer data (eg \c rgba8 to
       *   \c rgba32_f).
       *   If converting from float to integer, values will be clamped to the
       *   range 0.0...1.0, and the produced output will be a normalized value
       *   (eg when converting to an 8-bit component,  the result will be in
       *   the component's full range 0...255).
       *   Conversely, if converting from integer to float, the integer values
       *   are treated as normalized values.
       * * Removing components (eg converting from \c abgr8 to \c bgr8,
       *   \c bgr8 to \c r8) or "adding" them (eg converting from \c bgr8 to
       *   \c abgr8). Read the documentation to \a outputData for information
       *   about the output values in that case.
       *
       * Inabilities:
       * * Converting between different component sets (e.g. \c rgba8 to \c d32),
       *   with the exception of LA to RGBA (e.g. \c l8 to \c rgb8).
       * * Replicating components (something like \c rrr8).
       * * Component sizes larger than 64 bits.
       * * Float widths other than 16, 32 or 64.
       * * Converting to or from any special format whatsoever.
       *
       * \param inputData Input pixels buffer.
       * \param inputFormat Description of the input data output format.
       * \param outputData Output data buffer. Make sure it is large enough to
       *   receive the converted output data!
       *   Bits for components that appear in the output format, but not the
       *   input format, are unchanged; however, they can be set to some known
       *   value by providing \a defaultPixel data.
       * \param numPixels Number of pixels to convert.
       * \param defaultPixel Optional default output pixel data. Must be in the
       *   same format as the outputFormat. Can be used to ensure all
       *   components in the output pixels are initialized to a default value.
       * \returns Conversion result.
       */
      static Result ConvertBasic (const void* inputData, const StructuredTextureFormat& inputFormat,
                                  void* outputData, const StructuredTextureFormat& outputFormat,
                                  size_t numPixels,
                                  const void* defaultPixel = nullptr);
    };
  } // namespace Graphics
} // namespace CS

#endif // __CSGFX_UNIVERSALIMAGECONV_H__
