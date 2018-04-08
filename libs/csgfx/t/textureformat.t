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

#include "csgfx/textureformatstrings.h"

/**
 * Test texture format descriptions.
 */
class TextureFormatTest : public CppUnit::TestFixture
{
public:
  void testConstructExplicit();
  void testConvertExplicitToString();
  void testConvertStringToCanonical();
  void testConvertStringToStructured();
  void testConvertRoundtrip();

  CPPUNIT_TEST_SUITE(TextureFormatTest);
    CPPUNIT_TEST(testConstructExplicit);
    CPPUNIT_TEST(testConvertExplicitToString);
    CPPUNIT_TEST(testConvertStringToCanonical);
    CPPUNIT_TEST(testConvertStringToStructured);
    CPPUNIT_TEST(testConvertRoundtrip);
  CPPUNIT_TEST_SUITE_END();
};

static const CS::StructuredTextureFormat format_r5g6b5 ('r', 5, 'g', 6, 'b', 5);

void TextureFormatTest::testConstructExplicit()
{
  {
    CS::StructuredTextureFormat format;
    CPPUNIT_ASSERT_EQUAL(CS::StructuredTextureFormat::Invalid, format.GetFormat());
    CPPUNIT_ASSERT_EQUAL(0, format.GetComponentCount());
    CPPUNIT_ASSERT_EQUAL(0u, format.GetComponentMask());
  }
  {
    CS::StructuredTextureFormat format ('a', 8);
    CPPUNIT_ASSERT_EQUAL(CS::StructuredTextureFormat::Integer, format.GetFormat());
    CPPUNIT_ASSERT_EQUAL(1, format.GetComponentCount());
    CPPUNIT_ASSERT_EQUAL('a', format.GetComponent (0));
    CPPUNIT_ASSERT_EQUAL(int (8), int (format.GetComponentSize (0)));
    CPPUNIT_ASSERT_EQUAL(uint (CS::StructuredTextureFormat::compA),
                         format.GetComponentMask());
  }
  {
    CS::StructuredTextureFormat format (format_r5g6b5/*'r', 5, 'g', 6, 'b', 5*/);
    CPPUNIT_ASSERT_EQUAL(CS::StructuredTextureFormat::Integer, format.GetFormat());
    CPPUNIT_ASSERT_EQUAL(3, format.GetComponentCount());
    CPPUNIT_ASSERT_EQUAL('r', format.GetComponent (0));
    CPPUNIT_ASSERT_EQUAL(int (5), int (format.GetComponentSize (0)));
    CPPUNIT_ASSERT_EQUAL('g', format.GetComponent (1));
    CPPUNIT_ASSERT_EQUAL(int (6), int (format.GetComponentSize (1)));
    CPPUNIT_ASSERT_EQUAL('b', format.GetComponent (2));
    CPPUNIT_ASSERT_EQUAL(int (5), int (format.GetComponentSize (2)));
    CPPUNIT_ASSERT_EQUAL(uint (CS::StructuredTextureFormat::compRGB),
                         format.GetComponentMask());
  }
  {
    CS::StructuredTextureFormat format ('.', 4, '.', 4, '.', 4, '.', 4);
    CPPUNIT_ASSERT_EQUAL(format.GetFormat(), CS::StructuredTextureFormat::Integer);
    CPPUNIT_ASSERT_EQUAL(4, format.GetComponentCount());
    CPPUNIT_ASSERT_EQUAL('.', format.GetComponent (0));
    CPPUNIT_ASSERT_EQUAL(int (4), int (format.GetComponentSize (0)));
    CPPUNIT_ASSERT_EQUAL('.', format.GetComponent (1));
    CPPUNIT_ASSERT_EQUAL(int (4), int (format.GetComponentSize (1)));
    CPPUNIT_ASSERT_EQUAL('.', format.GetComponent (2));
    CPPUNIT_ASSERT_EQUAL(int (4), int (format.GetComponentSize (2)));
    CPPUNIT_ASSERT_EQUAL('.', format.GetComponent (3));
    CPPUNIT_ASSERT_EQUAL(int (4), int (format.GetComponentSize (3)));
    CPPUNIT_ASSERT_EQUAL(uint (CS::StructuredTextureFormat::compUnknown),
                         format.GetComponentMask());
  }
  {
    CS::StructuredTextureFormat format ('r', 16, CS::StructuredTextureFormat::Float);
    CPPUNIT_ASSERT_EQUAL(format.GetFormat(), CS::StructuredTextureFormat::Float);
    CPPUNIT_ASSERT_EQUAL(1, format.GetComponentCount());
    CPPUNIT_ASSERT_EQUAL('r', format.GetComponent (0));
    CPPUNIT_ASSERT_EQUAL(int (16), int (format.GetComponentSize (0)));
    CPPUNIT_ASSERT_EQUAL(uint (CS::StructuredTextureFormat::compR),
                         format.GetComponentMask());
  }
  {
    // Filling with 0 components is a "legacy" way of setting a format when having less than 4 comps
    CS::StructuredTextureFormat format ('r', 16, 0, 0, 0, 0, 0, 0, CS::StructuredTextureFormat::Float);
    CPPUNIT_ASSERT_EQUAL(format.GetFormat(), CS::StructuredTextureFormat::Float);
    CPPUNIT_ASSERT_EQUAL(1, format.GetComponentCount());
    CPPUNIT_ASSERT_EQUAL('r', format.GetComponent (0));
    CPPUNIT_ASSERT_EQUAL(int (16), int (format.GetComponentSize (0)));
    CPPUNIT_ASSERT_EQUAL(uint (CS::StructuredTextureFormat::compR),
                         format.GetComponentMask());
  }
}

void TextureFormatTest::testConvertExplicitToString()
{
  {
    CS::StructuredTextureFormat format;
    CPPUNIT_ASSERT_EQUAL(csString (), format.GetCanonical());
  }
  {
    CS::StructuredTextureFormat format ('a', 8);
    CPPUNIT_ASSERT_EQUAL(csString ("a8_i"), format.GetCanonical());
  }
  {
    CS::StructuredTextureFormat format ('r', 5, 'g', 6, 'b', 5);
    CPPUNIT_ASSERT_EQUAL(csString ("r5g6b5_i"), format.GetCanonical());
  }
  {
    CS::StructuredTextureFormat format ('.', 4, '.', 4, '.', 4, '.', 4);
    CPPUNIT_ASSERT_EQUAL(csString (".4.4.4.4_i"), format.GetCanonical());
  }
  {
    CS::StructuredTextureFormat format ('r', 16, 0, 0, 0, 0, 0, 0, CS::StructuredTextureFormat::Float);
    CPPUNIT_ASSERT_EQUAL(csString ("r16_f"), format.GetCanonical());
  }
}

void TextureFormatTest::testConvertStringToCanonical()
{
  CPPUNIT_ASSERT_EQUAL(csString (),
                       CS::TextureFormatStrings::ConvertCanonical (nullptr));
  CPPUNIT_ASSERT_EQUAL(csString ("a8_i"),
                       CS::TextureFormatStrings::ConvertCanonical ("a8"));
  CPPUNIT_ASSERT_EQUAL(csString ("r5g6b5_i"),
                       CS::TextureFormatStrings::ConvertCanonical ("r5g6b5"));
  CPPUNIT_ASSERT_EQUAL(csString ("r8g8b8_i"),
                       CS::TextureFormatStrings::ConvertCanonical ("rgb8"));
  CPPUNIT_ASSERT_EQUAL(csString ("r3g3b2_i"),
                       CS::TextureFormatStrings::ConvertCanonical ("rg3b2"));
  CPPUNIT_ASSERT_EQUAL(csString ("a1r5g5b5_i"),
                       CS::TextureFormatStrings::ConvertCanonical ("a1rgb5"));
  CPPUNIT_ASSERT_EQUAL(csString ("r16_f"),
                       CS::TextureFormatStrings::ConvertCanonical ("r16_f"));
  // Weird case
  CPPUNIT_ASSERT_EQUAL(csString ("r8g8b8_i"),
                       CS::TextureFormatStrings::ConvertCanonical ("r8gb"));
  // Special formats (unchanged)
  CPPUNIT_ASSERT_EQUAL(csString ("*dxt1"),
                       CS::TextureFormatStrings::ConvertCanonical ("*dxt1"));
  CPPUNIT_ASSERT_EQUAL(csString ("*SpecialFormatWithQuiteLongName"),
                       CS::TextureFormatStrings::ConvertCanonical ("*SpecialFormatWithQuiteLongName"));
}

static const char static_dxt1_str[] = "*dxt1";
static const CS::StructuredTextureFormat format_dxt1 (
  CS::TextureFormatStrings::ConvertStructured (static_dxt1_str));
static const CS::StructuredTextureFormat format_longname (
  CS::TextureFormatStrings::ConvertStructured ("*SpecialFormatWithQuiteLongName"));

void TextureFormatTest::testConvertStringToStructured()
{
  {
    CS::StructuredTextureFormat format (CS::TextureFormatStrings::ConvertStructured (nullptr));
    CPPUNIT_ASSERT_EQUAL(CS::StructuredTextureFormat::Invalid, format.GetFormat());
    CPPUNIT_ASSERT_EQUAL(0, format.GetComponentCount());
    CPPUNIT_ASSERT_EQUAL(0u, format.GetComponentMask());
  }
  {
    CS::StructuredTextureFormat format (CS::TextureFormatStrings::ConvertStructured ("a8"));
    CPPUNIT_ASSERT_EQUAL(CS::StructuredTextureFormat::Integer, format.GetFormat());
    CPPUNIT_ASSERT_EQUAL(1, format.GetComponentCount());
    CPPUNIT_ASSERT_EQUAL('a', format.GetComponent (0));
    CPPUNIT_ASSERT_EQUAL(int (8), int (format.GetComponentSize (0)));
    CPPUNIT_ASSERT_EQUAL(uint (CS::StructuredTextureFormat::compA),
                         format.GetComponentMask());
  }
  {
    CS::StructuredTextureFormat format (CS::TextureFormatStrings::ConvertStructured ("r5g6b5"));
    CPPUNIT_ASSERT_EQUAL(CS::StructuredTextureFormat::Integer, format.GetFormat());
    CPPUNIT_ASSERT_EQUAL(3, format.GetComponentCount());
    CPPUNIT_ASSERT_EQUAL('r', format.GetComponent (0));
    CPPUNIT_ASSERT_EQUAL(int (5), int (format.GetComponentSize (0)));
    CPPUNIT_ASSERT_EQUAL('g', format.GetComponent (1));
    CPPUNIT_ASSERT_EQUAL(int (6), int (format.GetComponentSize (1)));
    CPPUNIT_ASSERT_EQUAL('b', format.GetComponent (2));
    CPPUNIT_ASSERT_EQUAL(int (5), int (format.GetComponentSize (2)));
    CPPUNIT_ASSERT_EQUAL(uint (CS::StructuredTextureFormat::compRGB),
                         format.GetComponentMask());
  }
  {
    CS::StructuredTextureFormat format (CS::TextureFormatStrings::ConvertStructured ("rgb8"));
    CPPUNIT_ASSERT_EQUAL(CS::StructuredTextureFormat::Integer, format.GetFormat());
    CPPUNIT_ASSERT_EQUAL(3, format.GetComponentCount());
    CPPUNIT_ASSERT_EQUAL('r', format.GetComponent (0));
    CPPUNIT_ASSERT_EQUAL(int (8), int (format.GetComponentSize (0)));
    CPPUNIT_ASSERT_EQUAL('g', format.GetComponent (1));
    CPPUNIT_ASSERT_EQUAL(int (8), int (format.GetComponentSize (1)));
    CPPUNIT_ASSERT_EQUAL('b', format.GetComponent (2));
    CPPUNIT_ASSERT_EQUAL(int (8), int (format.GetComponentSize (2)));
    CPPUNIT_ASSERT_EQUAL(uint (CS::StructuredTextureFormat::compRGB),
                         format.GetComponentMask());
  }
  {
    CS::StructuredTextureFormat format (CS::TextureFormatStrings::ConvertStructured ("rg3b2"));
    CPPUNIT_ASSERT_EQUAL(CS::StructuredTextureFormat::Integer, format.GetFormat());
    CPPUNIT_ASSERT_EQUAL(3, format.GetComponentCount());
    CPPUNIT_ASSERT_EQUAL('r', format.GetComponent (0));
    CPPUNIT_ASSERT_EQUAL(int (3), int (format.GetComponentSize (0)));
    CPPUNIT_ASSERT_EQUAL('g', format.GetComponent (1));
    CPPUNIT_ASSERT_EQUAL(int (3), int (format.GetComponentSize (1)));
    CPPUNIT_ASSERT_EQUAL('b', format.GetComponent (2));
    CPPUNIT_ASSERT_EQUAL(int (2), int (format.GetComponentSize (2)));
    CPPUNIT_ASSERT_EQUAL(uint (CS::StructuredTextureFormat::compRGB),
                         format.GetComponentMask());
  }
  {
    CS::StructuredTextureFormat format (CS::TextureFormatStrings::ConvertStructured ("a1rgb5"));
    CPPUNIT_ASSERT_EQUAL(CS::StructuredTextureFormat::Integer, format.GetFormat());
    CPPUNIT_ASSERT_EQUAL(4, format.GetComponentCount());
    CPPUNIT_ASSERT_EQUAL('a', format.GetComponent (0));
    CPPUNIT_ASSERT_EQUAL(int (1), int (format.GetComponentSize (0)));
    CPPUNIT_ASSERT_EQUAL('r', format.GetComponent (1));
    CPPUNIT_ASSERT_EQUAL(int (5), int (format.GetComponentSize (1)));
    CPPUNIT_ASSERT_EQUAL('g', format.GetComponent (2));
    CPPUNIT_ASSERT_EQUAL(int (5), int (format.GetComponentSize (2)));
    CPPUNIT_ASSERT_EQUAL('b', format.GetComponent (3));
    CPPUNIT_ASSERT_EQUAL(int (5), int (format.GetComponentSize (3)));
    CPPUNIT_ASSERT_EQUAL(uint (CS::StructuredTextureFormat::compRGBA),
                         format.GetComponentMask());
  }
  {
    CS::StructuredTextureFormat format (CS::TextureFormatStrings::ConvertStructured ("r16_f"));
    CPPUNIT_ASSERT_EQUAL(format.GetFormat(), CS::StructuredTextureFormat::Float);
    CPPUNIT_ASSERT_EQUAL(1, format.GetComponentCount());
    CPPUNIT_ASSERT_EQUAL('r', format.GetComponent (0));
    CPPUNIT_ASSERT_EQUAL(int (16), int (format.GetComponentSize (0)));
    CPPUNIT_ASSERT_EQUAL(uint (CS::StructuredTextureFormat::compR),
                         format.GetComponentMask());
  }
  // Weird case
  {
    CS::StructuredTextureFormat format (CS::TextureFormatStrings::ConvertStructured ("r8gb"));
    CPPUNIT_ASSERT_EQUAL(CS::StructuredTextureFormat::Integer, format.GetFormat());
    CPPUNIT_ASSERT_EQUAL(3, format.GetComponentCount());
    CPPUNIT_ASSERT_EQUAL('r', format.GetComponent (0));
    CPPUNIT_ASSERT_EQUAL(int (8), int (format.GetComponentSize (0)));
    CPPUNIT_ASSERT_EQUAL('g', format.GetComponent (1));
    CPPUNIT_ASSERT_EQUAL(int (8), int (format.GetComponentSize (1)));
    CPPUNIT_ASSERT_EQUAL('b', format.GetComponent (2));
    CPPUNIT_ASSERT_EQUAL(int (8), int (format.GetComponentSize (2)));
    CPPUNIT_ASSERT_EQUAL(uint (CS::StructuredTextureFormat::compRGB),
                         format.GetComponentMask());
  }
  // Special formats
  {
    CS::StructuredTextureFormat format (format_dxt1/*CS::TextureFormatStrings::ConvertStructured ("*dxt1")*/);
    CPPUNIT_ASSERT_EQUAL(CS::StructuredTextureFormat::Special, format.GetFormat());
    CPPUNIT_ASSERT_EQUAL(csString ("*dxt1"), csString (format.GetSpecial()));
  }
  {
    CS::StructuredTextureFormat format (format_longname/*CS::TextureFormatStrings::ConvertStructured ("*SpecialFormatWithQuiteLongName")*/);
    CPPUNIT_ASSERT_EQUAL(CS::StructuredTextureFormat::Special, format.GetFormat());
    CPPUNIT_ASSERT_EQUAL(csString ("*SpecialFormatWithQuiteLongName"), csString (format.GetSpecial()));
  }
}

void TextureFormatTest::testConvertRoundtrip()
{
  {
    csString formatStr ((char*)nullptr);
    CS::StructuredTextureFormat format (
      CS::TextureFormatStrings::ConvertStructured (formatStr));
    CPPUNIT_ASSERT_EQUAL(formatStr,
                         format.GetCanonical());
  }
  {
    csString formatStr ("a8_i");
    CS::StructuredTextureFormat format (
      CS::TextureFormatStrings::ConvertStructured (formatStr));
    CPPUNIT_ASSERT_EQUAL(formatStr,
                         format.GetCanonical());
  }
  {
    csString formatStr ("r5g6b5_i");
    CS::StructuredTextureFormat format (
      CS::TextureFormatStrings::ConvertStructured (formatStr));
    CPPUNIT_ASSERT_EQUAL(formatStr,
                         format.GetCanonical());
  }
  {
    csString formatStr ("r8g8b8_i");
    CS::StructuredTextureFormat format (
      CS::TextureFormatStrings::ConvertStructured (formatStr));
    CPPUNIT_ASSERT_EQUAL(formatStr,
                         format.GetCanonical());
  }
  {
    csString formatStr ("r3g3b2_i");
    CS::StructuredTextureFormat format (
      CS::TextureFormatStrings::ConvertStructured (formatStr));
    CPPUNIT_ASSERT_EQUAL(formatStr,
                         format.GetCanonical());
  }
  {
    csString formatStr ("a1r5g5b5_i");
    CS::StructuredTextureFormat format (
      CS::TextureFormatStrings::ConvertStructured (formatStr));
    CPPUNIT_ASSERT_EQUAL(formatStr,
                         format.GetCanonical());
  }
  {
    csString formatStr ("r16_f");
    CS::StructuredTextureFormat format (
      CS::TextureFormatStrings::ConvertStructured (formatStr));
    CPPUNIT_ASSERT_EQUAL(formatStr,
                         format.GetCanonical());
  }
  // Special formats (unchanged)
  {
    csString formatStr ("*dxt1");
    CS::StructuredTextureFormat format (
      CS::TextureFormatStrings::ConvertStructured (formatStr));
    CPPUNIT_ASSERT_EQUAL(formatStr,
                         format.GetCanonical());
  }
  {
    csString formatStr ("*SpecialFormatWithQuiteLongName");
    CS::StructuredTextureFormat format (
      CS::TextureFormatStrings::ConvertStructured (formatStr));
    CPPUNIT_ASSERT_EQUAL(formatStr,
                         format.GetCanonical());
  }
}
