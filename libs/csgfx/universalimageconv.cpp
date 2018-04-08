/*
    Copyright(C) 2012 by Frank Richter

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

#include "csgfx/universalimageconv.h"

#include "csgfx/textureformatstrings.h"
#include "csutil/csendian.h"

namespace CS
{
  namespace Graphics
  {
    static bool IgnoreComponent (char comp)
    {
      return comp == 'x';
    }

    static inline bool IsFormatRGBA (const StructuredTextureFormat& format)
    {
      uint fmtComps (format.GetComponentMask());
      return ((fmtComps & StructuredTextureFormat::compRGBA) != 0)
        && ((fmtComps & ~(StructuredTextureFormat::compRGBA | StructuredTextureFormat::compX)) == 0);
    }

    static inline bool IsFormatLA (const StructuredTextureFormat& format)
    {
      uint fmtComps (format.GetComponentMask());
      return ((fmtComps & StructuredTextureFormat::compLumA) != 0)
        && ((fmtComps & ~(StructuredTextureFormat::compLumA | StructuredTextureFormat::compX)) == 0);
    }

    static inline bool IsFormatDS (const StructuredTextureFormat& format)
    {
      uint fmtComps (format.GetComponentMask());
      return ((fmtComps & StructuredTextureFormat::compDepthStencil) != 0)
        && ((fmtComps & ~(StructuredTextureFormat::compDepthStencil | StructuredTextureFormat::compX)) == 0);
    }

    namespace
    {
      struct ComponentExtractorUInt
      {
        ComponentExtractorUInt (const StructuredTextureFormat& format)
          : format (format), pixelSize (0)
        {
          for (int c = 0; c < format.GetComponentCount(); c++)
            pixelSize += format.GetComponentSize (c);
          pixelSize = (pixelSize + 7) / 8;
        }

        template<typename DestType>
        void Extract (int comp, const void* data, DestType& dest)
        {
          dest = 0;
          const uint8* data_u8 (reinterpret_cast<const uint8*> (data));
          int componentOffset (pixelSize*8);
          for (int c = 0; c <= comp; c++)
            componentOffset -= format.GetComponentSize (c);
          int destBitOffset = 0;
          int remainingBits (format.GetComponentSize (comp));
          while (remainingBits > 0)
          {
            int srcByteOffset = (componentOffset / 8);
            int srcBitOffset = componentOffset % 8;
            int srcBitNum = 8-srcBitOffset;
            srcBitNum = csMin (srcBitNum, remainingBits);
            uint srcBitsMask = ((1 << srcBitNum) - 1) << srcBitOffset;
            dest |= (DestType ((*(data_u8 + srcByteOffset)) & srcBitsMask) >> srcBitOffset) << destBitOffset;
            componentOffset += srcBitNum;
            destBitOffset += srcBitNum;
            remainingBits -= srcBitNum;
          }
        }
      public:
        const StructuredTextureFormat& format;
        int pixelSize;
      };

      struct ComponentExtractorFloat
      {
        ComponentExtractorFloat (const StructuredTextureFormat& format)
          : uintExtractor (format)
        {
        }

        template<typename DestType>
        void Extract (int comp, const void* data, DestType& dest)
        {
          char compSize (uintExtractor.format.GetComponentSize (comp));
          if (compSize == 16)
          {
            uint16 ieeeValue;
            uintExtractor.Extract (comp, data, ieeeValue);
            dest = csIEEEfloat::ToNative (ieeeValue);
          }
          else if (compSize == 32)
          {
            uint32 ieeeValue;
            uintExtractor.Extract (comp, data, ieeeValue);
            dest = csIEEEfloat::ToNative (ieeeValue);
          }
          else if (compSize == 64)
          {
            uint64 ieeeValue;
            uintExtractor.Extract (comp, data, ieeeValue);
            dest = csIEEEfloat::ToNative (ieeeValue);
          }
        }
      private:
        ComponentExtractorUInt uintExtractor;
      };

      template<typename SrcType>
      struct ComponentWriterUInt
      {
        typedef SrcType WriterInputType;

        ComponentWriterUInt (const StructuredTextureFormat& format)
          : format (format), pixelSize (0)
        {
          for (int c = 0; c < format.GetComponentCount(); c++)
            pixelSize += format.GetComponentSize (c);
          pixelSize = (pixelSize + 7) / 8;
        }

        void Write (int comp, const SrcType src, void* data)
        {
          uint8* data_u8 (reinterpret_cast<uint8*> (data));
          int componentOffset (pixelSize*8);
          for (int c = 0; c <= comp; c++)
            componentOffset -= format.GetComponentSize (c);
          int srcBitOffset = 0;
          int remainingBits (format.GetComponentSize (comp));
          while (remainingBits > 0)
          {
            int destByteOffset = (componentOffset / 8);
            int destBitOffset = componentOffset % 8;
            int destBitNum = 8-destBitOffset;
            destBitNum = csMin (destBitNum, remainingBits);
            uint destBitsMask = ((1 << destBitNum) - 1) << destBitOffset;
            uint8* dest_byte = data_u8 + destByteOffset;
            *dest_byte &= ~destBitsMask;
            uint8 dest_data = ((src >> srcBitOffset) << destBitOffset) & destBitsMask;
            *dest_byte |= dest_data;
            componentOffset += destBitNum;
            srcBitOffset += destBitNum;
            remainingBits -= destBitNum;
          }
        }
      public:
        const StructuredTextureFormat& format;
        int pixelSize;
      };

      template<typename SrcType>
      struct ComponentWriterFloat
      {
        typedef SrcType WriterInputType;

        ComponentWriterFloat (const StructuredTextureFormat& format)
          : format (format)
        {
        }

        void Write (int comp, const SrcType src, void* data)
        {
          char compSize (format.GetComponentSize (comp));
          if (compSize == 16)
          {
            ComponentWriterUInt<uint16> uintWriter (format);
            uintWriter.Write (comp, csIEEEfloat::FromNativeRTZ (float (src)), data);
          }
          else if (compSize == 32)
          {
            ComponentWriterUInt<uint32> uintWriter (format);
            uintWriter.Write (comp, csIEEEfloat::FromNative (float (src)), data);
          }
          else if (compSize == 64)
          {
            ComponentWriterUInt<uint64> uintWriter (format);
            uintWriter.Write (comp, csIEEEfloat::FromNative (double (src)), data);
          }
        }
      private:
        const StructuredTextureFormat& format;
      };

      struct ConversionParams
      {
        const void* inputData;
        const StructuredTextureFormat& inputFormat;
        void* outputData;
        const StructuredTextureFormat& outputFormat;
        size_t numPixels;
        const void* defaultBytes;

        ConversionParams (const void* inputData, const StructuredTextureFormat& inputFormat,
                          void* outputData, const StructuredTextureFormat& outputFormat,
                          size_t numPixels, const void* defaultBytes)
          : inputData (inputData), inputFormat (inputFormat),
            outputData (outputData), outputFormat (outputFormat), numPixels (numPixels),
            defaultBytes (defaultBytes) {}
      };

      // Convert from integer to integer
      template<typename OutputType>
      struct ConverterUInt
      {
        ConverterUInt (int inSize, int outSize) : inSize (inSize), outSize (outSize)
        {
        }

        template<typename InputType>
        inline OutputType operator() (const InputType& comp)
        {
          if (inSize > outSize)
          {
            // Input component is larger: just cut off right bits
            return comp >> (inSize - outSize);
          }
          else if (inSize < outSize)
          {
            // Output component is larger: fill up with input component
            OutputType outComp (0);
            int shift = outSize;
            while (shift > 0)
            {
              shift -= inSize;
              if (shift >= 0)
                outComp |= comp << shift;
              else
                outComp |= comp >> -shift;
            }
            return outComp;
          }
          else
            // No adjustment needed
            return comp;
        }
      private:
        int inSize;
        int outSize;
      };

      // Convert an (normalized float) input to an integer
      template<typename OutputType>
      struct ConverterUIntNorm
      {
        ConverterUIntNorm (int inSize, int outSize) : outSize (outSize)
        {
        }

        template<typename InputType>
        inline OutputType operator() (const InputType& comp)
        {
          OutputType outMax (~(OutputType (~0) << outSize));
          return OutputType (csClamp (comp, InputType (1), InputType (0)) * outMax);
        }
      private:
        int outSize;
      };

      // Convert an (integer) input to a float (normalized)
      template<typename OutputType>
      struct ConverterFloatNorm
      {
        ConverterFloatNorm (int inSize, int outSize) : inSize (inSize)
        {
        }

        template<typename InputType>
        inline OutputType operator() (const InputType& comp)
        {
          InputType inMax (~(InputType (~0) << inSize));
          return OutputType (comp) / OutputType (inMax);
        }
      private:
        int inSize;
      };

      // Convert a float to a float
      template<typename OutputType>
      struct ConverterFloatCast
      {
        ConverterFloatCast (int inSize, int outSize)
        {
        }

        template<typename InputType>
        inline OutputType operator() (const InputType& comp)
        {
          return comp;
        }
      };

      struct CompMapDefault
      {
        char operator() (char outComp) const
        {
          return IgnoreComponent (outComp) ? 0 : outComp;
        };
      };

      struct CompMapLAtoRGB : public CompMapDefault
      {
        char operator() (char outComp) const
        {
          switch (outComp)
          {
          case 'r':
          case 'g':
          case 'b':
            return 'l';
          }
          return CompMapDefault::operator() (outComp);
        };
      };
    } // namespace

    template<typename ComponentMapper,
             typename Extractor,
             typename IntermediateType,
             typename Converter,
             typename Writer>
    static inline void ConvertOnePixel (ConversionParams& params)
    {
      int numInComps (params.inputFormat.GetComponentCount());
      IntermediateType compValues[4];
      // Grab input components, expand to intermediate values
      {
        Extractor extract (params.inputFormat);
        for (int c = 0; c < numInComps; c++)
        {
          extract.Extract (c, params.inputData, compValues[c]);
        }
      }

      // Construct output components
      Writer write (params.outputFormat);
      int numOutComps (params.outputFormat.GetComponentCount());
      for (int o = 0; o < numOutComps; o++)
      {
        ComponentMapper compmap;
        char compID (compmap (params.outputFormat.GetComponent (o)));
        // Don't write unmapped components
        if (!compID) continue;
        int inputComp (-1);
        for (int i = 0; i < numInComps; i++)
        {
          if (params.inputFormat.GetComponent (i) == compID)
          {
            inputComp = i;
            break;
          }
        }
        if (inputComp != -1)
        {
          Converter convert (params.inputFormat.GetComponentSize (inputComp),
                             params.outputFormat.GetComponentSize (o));

          // Write output component
          write.Write (o, convert (compValues[inputComp]), params.outputData);
        }
      }
    }

    template<typename PtrType>
    static void SkipPixel (PtrType& ptr, int pixelSize)
    {
      ptr = (PtrType)((const uint8*)ptr + pixelSize);
    }

    template<typename ComponentMapper,
             typename Extractor,
             typename IntermediateType,
             typename Converter,
             typename Writer>
    static UniversalImageConverter::Result ConvertWithIntermediateMapper (ConversionParams params)
    {
      int in_pixelSize (0);
      for (int c = 0; c < params.inputFormat.GetComponentCount(); c++)
        in_pixelSize += params.inputFormat.GetComponentSize (c);
      in_pixelSize = (in_pixelSize + 7) / 8;
      int out_pixelSize (0);
      for (int c = 0; c < params.outputFormat.GetComponentCount(); c++)
        out_pixelSize += params.outputFormat.GetComponentSize (c);
      out_pixelSize = (out_pixelSize + 7) / 8;
      while (params.numPixels-- > 0)
      {
        if (params.defaultBytes)
          memcpy (params.outputData, params.defaultBytes, out_pixelSize);
        // Actual conversion
        ConvertOnePixel<ComponentMapper, Extractor, IntermediateType, Converter, Writer> (params);
        // Go to next pixels
        SkipPixel (params.inputData, in_pixelSize);
        SkipPixel (params.outputData, out_pixelSize);
      }
      return UniversalImageConverter::Success;
    }

    template<typename Extractor,
             typename IntermediateType,
             typename Converter,
             typename Writer>
    static UniversalImageConverter::Result ConvertWithIntermediate (ConversionParams params)
    {
      if (IsFormatLA (params.inputFormat) && IsFormatRGBA (params.outputFormat))
        return ConvertWithIntermediateMapper<CompMapLAtoRGB, Extractor, IntermediateType, Converter, Writer> (params);
      else
        return ConvertWithIntermediateMapper<CompMapDefault, Extractor, IntermediateType, Converter, Writer> (params);
    }

    static int MaxCompSize (const StructuredTextureFormat& format)
    {
      int maxCompSize (0);
      for (int c = 0; c < format.GetComponentCount(); c++)
      {
        // Junk doesn't need to figure into component size
        if (IgnoreComponent (format.GetComponent (c))) continue;
        maxCompSize = csMax (maxCompSize, int (format.GetComponentSize (c)));
      }
      return maxCompSize;
    }

    namespace
    {
      template<template <typename> class Converter,
               typename Writer>
      struct ExtractFromInt
      {
        UniversalImageConverter::Result operator()(const ConversionParams& params)
        {
          int maxCompSize (MaxCompSize (params.inputFormat));
          if (maxCompSize <= 8)
          {
            // converter input is uint8
            return ConvertWithIntermediate<ComponentExtractorUInt, uint8, Converter<typename Writer::WriterInputType>, Writer> (params);
          }
          else if (maxCompSize <= 16)
          {
            // converter input is uint16
            return ConvertWithIntermediate<ComponentExtractorUInt, uint16, Converter<typename Writer::WriterInputType>, Writer> (params);
          }
          else if (maxCompSize <= 32)
          {
            // converter input is uint32
            return ConvertWithIntermediate<ComponentExtractorUInt, uint32, Converter<typename Writer::WriterInputType>, Writer> (params);
          }
          else if (maxCompSize <= 64)
          {
            // converter input is uint64
            return ConvertWithIntermediate<ComponentExtractorUInt, uint64, Converter<typename Writer::WriterInputType>, Writer> (params);
          }
          else
          {
            // not supported
            return UniversalImageConverter::ErrorFormatUnsupported;
          }
        }
      };

      template<template <typename> class Converter,
               typename Writer>
      struct ExtractFromFloat
      {
        UniversalImageConverter::Result operator()(const ConversionParams& params)
        {
          int maxCompSize (MaxCompSize (params.inputFormat));
          if (maxCompSize <= 32)
          {
            // converter input is float
            return ConvertWithIntermediate<ComponentExtractorFloat, float, Converter<typename Writer::WriterInputType>, Writer> (params);
          }
          else if (maxCompSize <= 64)
          {
            // converter input is double
            return ConvertWithIntermediate<ComponentExtractorFloat, double, Converter<typename Writer::WriterInputType>, Writer> (params);
          }
          else
          {
            // not supported
            return UniversalImageConverter::ErrorFormatUnsupported;
          }
        }
      };

      template<template <template <typename> class Converter, typename> class ExtractFrom,
               template <typename> class Converter>
      struct WriteToInt
      {
        UniversalImageConverter::Result operator()(const ConversionParams& params)
        {
          int maxCompSize (MaxCompSize (params.outputFormat));
          if (maxCompSize <= 8)
          {
            // writer input is uint8
            ExtractFrom<Converter, ComponentWriterUInt<uint8> > extractor;
            return extractor (params);
          }
          else if (maxCompSize <= 16)
          {
            // writer input is uint16
            ExtractFrom<Converter, ComponentWriterUInt<uint16> > extractor;
            return extractor (params);
          }
          else if (maxCompSize <= 32)
          {
            // writer input is uint32
            ExtractFrom<Converter, ComponentWriterUInt<uint32> > extractor;
            return extractor (params);
          }
          else if (maxCompSize <= 64)
          {
            // writer input is uint64
            ExtractFrom<Converter, ComponentWriterUInt<uint64> > extractor;
            return extractor (params);
          }
          else
          {
            // not supported
            return UniversalImageConverter::ErrorFormatUnsupported;
          }
        }
      };

      template<template <template <typename> class Converter, typename> class ExtractFrom,
               template <typename> class Converter>
      struct WriteToFloat
      {
        UniversalImageConverter::Result operator()(const ConversionParams& params)
        {
          int maxCompSize (MaxCompSize (params.outputFormat));
          if (maxCompSize <= 32)
          {
            // writer input is float
            ExtractFrom<Converter, ComponentWriterFloat<float> > extractor;
            return extractor (params);
          }
          else if (maxCompSize <= 64)
          {
            // writer input is double
            ExtractFrom<Converter, ComponentWriterFloat<double> > extractor;
            return extractor (params);
          }
          else
          {
            // not supported
            return UniversalImageConverter::ErrorFormatUnsupported;
          }
        }
      };
    } // namespace

    static UniversalImageConverter::Result ConvertIntInt (const ConversionParams& params)
    {
      WriteToInt<ExtractFromInt, ConverterUInt> writer;
      return writer (params);
    }

    static UniversalImageConverter::Result ConvertIntFloat (const ConversionParams& params)
    {
      WriteToFloat<ExtractFromInt, ConverterFloatNorm> writer;
      return writer (params);
    }

    static UniversalImageConverter::Result ConvertFloatFloat (const ConversionParams& params)
    {
      WriteToFloat<ExtractFromFloat, ConverterFloatCast> writer;
      return writer (params);
    }

    static UniversalImageConverter::Result ConvertFloatInt (const ConversionParams& params)
    {
      WriteToInt<ExtractFromFloat, ConverterUIntNorm> writer;
      return writer (params);
    }

    static inline bool ComponentsUnique (const StructuredTextureFormat& format)
    {
      bool seen[256];
      memset (seen, 0, sizeof (seen));
      for (int c = 0; c < format.GetComponentCount(); c++)
      {
        uint8 comp = format.GetComponent (c);
        // Junk is special (can have multiple ones)
        if (IgnoreComponent (comp)) continue;

        if (seen[comp]) return false;
        seen[comp] = true;
      }
      return true;
    }

    static inline bool CheckFloatCompSize (const StructuredTextureFormat& format)
    {
      for (int c = 0; c < format.GetComponentCount(); c++)
      {
        uint8 comp = format.GetComponent (c);
        // Junk doesn't need to figure into component size
        if (IgnoreComponent (comp)) continue;

        char compSize (format.GetComponentSize (c));
        // We only support certain float sizes
        if ((compSize != 16) && (compSize != 32) && (compSize != 64))
          return false;
      }
      return true;
    }

    UniversalImageConverter::Result
    UniversalImageConverter::ConvertBasic (const void* inputData, const StructuredTextureFormat& inputFormat,
                                           void* outputData, const StructuredTextureFormat& outputFormat,
                                           size_t numPixels,
                                           const void* defaultBytes)
    {
      if (!inputFormat.IsValid() || !outputFormat.IsValid())
        return ErrorInvalidFormat;

      /* Check inputFormat/outputFormat compatibility:
       * Can only convert between RGBA/RGBA formats, LA/LA, or DS/DS */
      if ((!IsFormatRGBA (inputFormat)
          && !IsFormatLA (inputFormat)
          && !IsFormatDS (inputFormat))
        || (!IsFormatRGBA (outputFormat)
          && !IsFormatLA (outputFormat)
          && !IsFormatDS (outputFormat)))
      {
        return ErrorFormatUnsupported;
      }
      // Don't allow mixed formats (except L(A) -> RGB(A))
      if ((IsFormatRGBA (inputFormat) && !IsFormatRGBA (outputFormat))
        || (IsFormatLA (inputFormat) && !(IsFormatLA (outputFormat) || IsFormatRGBA (outputFormat)))
        || (IsFormatDS (inputFormat) && !IsFormatDS (outputFormat)))
      {
        return ErrorIncompatibleComponents;
      }
      // We don't support replicating components (yet?)
      if (!ComponentsUnique (inputFormat)
        || !ComponentsUnique (outputFormat))
      {
        return ErrorInvalidFormat;
      }

      ConversionParams params (inputData, inputFormat, outputData, outputFormat, numPixels, defaultBytes);

      // TODO: Optimized conversions for frequent in/out format combos

      // float -> float: intermediate is float or double
      // float -> int: intermediate is float or double, conversion is done before writing
      // int -> float: intermediate is largest type all components fit in, conversion is done before writing
      // int -> int: intermediate is largest type all components fit in

      if (inputFormat.GetFormat() == StructuredTextureFormat::Float)
      {
        if (!CheckFloatCompSize (inputFormat))
          return ErrorInvalidFormat; 
        if (outputFormat.GetFormat() == StructuredTextureFormat::Float)
        {
          if (!CheckFloatCompSize (outputFormat))
            return ErrorInvalidFormat;
          return ConvertFloatFloat (params);
        }
        else if (outputFormat.GetFormat() == StructuredTextureFormat::Integer)
        {
          return ConvertFloatInt (params);
        }
        else if (outputFormat.GetFormat() == StructuredTextureFormat::Special)
          return ErrorFormatUnsupported;
        else
          return ErrorInvalidFormat; // Not that we should get here.
      }
      else if (inputFormat.GetFormat() == StructuredTextureFormat::Integer)
      {
        if (outputFormat.GetFormat() == StructuredTextureFormat::Float)
        {
          if (!CheckFloatCompSize (outputFormat))
            return ErrorInvalidFormat;
          return ConvertIntFloat (params);
        }
        else if (outputFormat.GetFormat() == StructuredTextureFormat::Integer)
        {
          return ConvertIntInt (params);
        }
        else if (outputFormat.GetFormat() == StructuredTextureFormat::Special)
          return ErrorFormatUnsupported;
        else
          return ErrorInvalidFormat; // Not that we should get here.
      }
      else if (inputFormat.GetFormat() == StructuredTextureFormat::Special)
        return ErrorFormatUnsupported;
      else
        return ErrorInvalidFormat; // Not that we should get here.
    }
  } // namespace Graphics
} // namespace CS
