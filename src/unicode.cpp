/**
 * @file unicode.cpp
 * Detects, read and writes characters in the proper format.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "unicode.h"

#include <cstdio>
#include <deque>
#include <vector>

using namespace std;


//! Check if all characters are ASCII (0-127)
static bool is_ascii(const vector<UINT8> &data, size_t &zero_cnt);


//! Convert the array of bytes into an array of ints
static bool decode_bytes(MemoryFile &fm);


/**
 * Decode UTF-8 sequences from the input data and put the chars in the output data.
 * If there is any decoding error, return false.
 */
static bool decode_utf8(MemoryFile &fm);


/**
 * Extract 2 bytes from the stream and increment idx by 2
 *
 * @param in   byte vector with input data
 * @param idx  index points to working position in vector
 */
static int get_word(const vector<UINT8> &in_data, size_t &idx, bool be);


/**
 * Decode a UTF-16 sequence.
 * Sets enc based on the BOM.
 * Must have the BOM as the first two bytes.
 */
static bool decode_utf16(MemoryFile &fm);


/**
 * Looks for the BOM of UTF-16 BE/LE and UTF-8.
 * If found, set the encoding type and return true.
 * Otherwise sets the encoding to E_CharEncoding::ASCII and returns false.
 */
static bool decode_bom(MemoryFile &fm);


//! Write for ASCII and BYTE encoding
static void write_byte(int ch);


//! Writes a single character to a file using UTF-8 encoding
static void write_utf8(int ch);


static void write_utf16(int ch, bool be);


static bool is_ascii(const vector<UINT8> &data, size_t &zero_cnt)
{
   size_t non_ascii_cnt = 0;

   zero_cnt = 0;

   for (UINT8 value : data)
   {
      if (value & 0x80)
      {
         non_ascii_cnt++;
      }

      if (!value)
      {
         zero_cnt++;
      }
   }

   return((non_ascii_cnt + zero_cnt) == 0);
}


static bool decode_bytes(MemoryFile &fm)
{
   fm.data.resize(fm.raw.size());
   fm.data.assign(fm.raw.begin(), fm.raw.end());
   return(true);
}


// ------------------------------------------------------------------------------------
// | UTF8                                | Code point            | Unicode range      |
// ------------------------------------------------------------------------------------
// | 0xxxxxxx                            | xxxxxxx               | U+0000  – U+007F   |
// | 110xxxxx 10yyyyyy                   | xxxxxyyyyyy           | U+0080  – U+07FF   |
// | 1110xxxx 10yyyyyy 10zzzzzz          | xxxxyyyyyyzzzzzz      | U+0800  – U+FFFF   |
// | 11110xxx 10yyyyyy 10zzzzzz 10wwwwww | xxxyyyyyyzzzzzzwwwwww | U+10000 – U+10FFFF |
// ------------------------------------------------------------------------------------
void encode_utf8(int ch, vector<UINT8> &res)
{
   if (ch < 0)
   {
      // illegal code - do not store
   }
   else if (ch < 0x80)
   {
      // 1-byte sequence
      res.push_back(ch);
   }
   else if (ch < 0x0800)
   {
      // 2-byte sequence
      res.push_back(0xC0 | (ch >> 6));
      res.push_back(0x80 | (ch & 0x3f));
   }
   else if (ch < 0x10000)
   {
      // 3-byte sequence
      res.push_back(0xE0 | (ch >> 12));
      res.push_back(0x80 | ((ch >> 6) & 0x3f));
      res.push_back(0x80 | (ch & 0x3f));
   }
   else if (ch < 0x110000)
   {
      // 4-byte sequence
      res.push_back(0xF0 | (ch >> 18));
      res.push_back(0x80 | ((ch >> 12) & 0x3f));
      res.push_back(0x80 | ((ch >> 6) & 0x3f));
      res.push_back(0x80 | (ch & 0x3f));
   }
   else
   {
      // illegal code - do not store
   }
} // encode_utf8


static bool decode_utf8(MemoryFile &fm)
{
   size_t idx = 0;

   fm.data.clear();

   // skip UTF-8 BOM if present
   if (  (fm.hasBom && fm.encoding == E_CharEncoding::UTF8)
      || (fm.raw[0] == 0xef && fm.raw[1] == 0xbb && fm.raw[2] == 0xbf))
   {
      idx = 3;
   }
   int char_cnt;

   while (idx < fm.raw.size())
   {
      int ch = fm.raw[idx++];

      if ((ch & 0x80) == 0x00)         // 1-byte sequence
      {
         char_cnt = 1;
      }
      else if ((ch & 0xE0) == 0xC0)    // 2-byte sequence
      {
         ch      &= 0x1F;
         char_cnt = 2;
      }
      else if ((ch & 0xF0) == 0xE0)    // 3-byte sequence
      {
         ch      &= 0x0F;
         char_cnt = 3;
      }
      else if ((ch & 0xF8) == 0xF0)    // 4-byte sequence
      {
         ch      &= 0x07;
         char_cnt = 4;
      }
      else
      {
         // invalid UTF-8 sequence
         return(false);
      }

      while (  --char_cnt > 0
            && idx < fm.raw.size())
      {
         int tmp = fm.raw[idx++];

         if ((tmp & 0xC0) != 0x80)
         {
            // invalid UTF-8 sequence
            return(false);
         }
         ch = (ch << 6) | (tmp & 0x3f);
      }

      if (char_cnt > 0)
      {
         // invalid UTF-8 sequence
         return(false);
      }
      fm.data.push_back(ch);
   }
   return(true);
} // decode_utf8


static int get_word(const vector<UINT8> &in_data, size_t &idx, bool be)
{
   int ch;

   if ((idx + 2) > in_data.size())
   {
      ch = -1;
   }
   else if (be)
   {
      ch = (in_data[idx] << 8) | in_data[idx + 1];
   }
   else
   {
      ch = in_data[idx] | (in_data[idx + 1] << 8);
   }
   idx += 2;
   return(ch);
}


static bool decode_utf16(MemoryFile &fm)
{
   fm.data.clear();

   if (fm.raw.size() & 1)
   {
      // can't have and odd length
      return(false);
   }

   if (fm.raw.size() < 2)
   {
      // we require the BOM or at least 1 char
      return(false);
   }
   size_t idx = 2;

   if (  (fm.raw[0] == 0xfe)
      && (fm.raw[1] == 0xff))
   {
      fm.encoding = E_CharEncoding::UTF16_BE;
   }
   else if (  (fm.raw[0] == 0xff)
           && (fm.raw[1] == 0xfe))
   {
      fm.encoding = E_CharEncoding::UTF16_LE;
   }
   else
   {
      /*
       * If we have a few words, we can take a guess, assuming the first few
       * chars are ASCII
       */
      fm.encoding = E_CharEncoding::ASCII;
      idx         = 0;

      if (fm.raw.size() >= 6)
      {
         if (  (fm.raw[0] == 0)
            && (fm.raw[2] == 0)
            && (fm.raw[4] == 0))
         {
            fm.encoding = E_CharEncoding::UTF16_BE;
         }
         else if (  (fm.raw[1] == 0)
                 && (fm.raw[3] == 0)
                 && (fm.raw[5] == 0))
         {
            fm.encoding = E_CharEncoding::UTF16_LE;
         }
      }

      if (fm.encoding == E_CharEncoding::ASCII)
      {
         return(false);
      }
   }
   bool be = (fm.encoding == E_CharEncoding::UTF16_BE);

   while (idx < fm.raw.size())
   {
      int ch = get_word(fm.raw, idx, be);

      if ((ch & 0xfc00) == 0xd800)
      {
         ch  &= 0x3ff;
         ch <<= 10;
         int tmp = get_word(fm.raw, idx, be);

         if ((tmp & 0xfc00) != 0xdc00)
         {
            return(false);
         }
         ch |= (tmp & 0x3ff);
         ch += 0x10000;
         fm.data.push_back(ch);
      }
      else if (  (  ch >= 0
                 && ch < 0xD800)
              || ch >= 0xE000)
      {
         fm.data.push_back(ch);
      }
      else
      {
         // invalid character
         return(false);
      }
   }
   return(true);
} // decode_utf16


static bool decode_bom(MemoryFile &fm)
{
   fm.encoding = E_CharEncoding::ASCII;
   fm.hasBom   = false;

   if (fm.raw.size() >= 2)
   {
      if (  (fm.raw[0] == 0xfe)
         && (fm.raw[1] == 0xff))
      {
         fm.encoding = E_CharEncoding::UTF16_BE;
         fm.hasBom   = true;
      }
      else if (  (fm.raw[0] == 0xff)
              && (fm.raw[1] == 0xfe))
      {
         fm.encoding = E_CharEncoding::UTF16_LE;
         fm.hasBom   = true;
      }
      else if (  (fm.raw.size() >= 3)
              && (fm.raw[0] == 0xef)
              && (fm.raw[1] == 0xbb)
              && (fm.raw[2] == 0xbf))
      {
         fm.encoding = E_CharEncoding::UTF8;
         fm.hasBom   = true;
      }
   }
   return(fm.hasBom);
}


bool decode_unicode(MemoryFile &fm)
{
   // check for a BOM
   if (decode_bom(fm))
   {
      if (fm.encoding == E_CharEncoding::UTF8)
      {
         return(decode_utf8(fm));
      }
      return(decode_utf16(fm));
   }
   // Check for simple ASCII or UTF8/16
   size_t zero_cnt = 0;

   if (is_ascii(fm.raw, zero_cnt))
   {
      // ASCII encoding
      fm.encoding = E_CharEncoding::ASCII;
      return(decode_bytes(fm));
   }

   if (  (zero_cnt > (fm.raw.size() / 4))
      && (zero_cnt <= (fm.raw.size() / 2)))
   {
      // There are a lot of 0's in the input data
      // so it is likely to be UTF-16
      if (decode_utf16(fm))
      {
         return(true);
      }
   }

   if (decode_utf8(fm))
   {
      fm.encoding = E_CharEncoding::UTF8;
      return(true);
   }
   // it is an unrecognized byte sequence
   fm.encoding = E_CharEncoding::BYTE;
   return(decode_bytes(fm));
} // decode_unicode


static void write_byte(int ch)
{
   if ((ch & 0xff) == ch)
   {
      if (cpd.fout)
      {
         fputc(ch, cpd.fout);
      }

      if (cpd.bout)
      {
         cpd.bout->push_back(static_cast<UINT8>(ch));
      }
   }
   else
   {
      // illegal code - do not store
   }
}


static void write_utf8(int ch)
{
   vector<UINT8> vv;

   vv.reserve(6);

   encode_utf8(ch, vv);

   for (UINT8 char_val : vv)
   {
      write_byte(char_val);
   }
}


static void write_utf16(int ch, bool be)
{
   // U+0000 to U+D7FF and U+E000 to U+FFFF
   if (  (  ch >= 0
         && ch < 0xD800)
      || (  ch >= 0xE000
         && ch < 0x10000))
   {
      if (be)
      {
         write_byte(ch >> 8);
         write_byte(ch & 0xff);
      }
      else
      {
         write_byte(ch & 0xff);
         write_byte(ch >> 8);
      }
   }
   else if (  ch >= 0x10000
           && ch < 0x110000)
   {
      int v1 = ch - 0x10000;
      int w1 = 0xD800 + (v1 >> 10);
      int w2 = 0xDC00 + (v1 & 0x3ff);

      if (be)
      {
         write_byte(w1 >> 8);
         write_byte(w1 & 0xff);
         write_byte(w2 >> 8);
         write_byte(w2 & 0xff);
      }
      else
      {
         write_byte(w1 & 0xff);
         write_byte(w1 >> 8);
         write_byte(w2 & 0xff);
         write_byte(w2 >> 8);
      }
   }
   else
   {
      // illegal code - do not store
   }
} // write_utf16


void write_bom()
{
   switch (cpd.enc)
   {
   case E_CharEncoding::UTF8:
      write_byte(0xef);
      write_byte(0xbb);
      write_byte(0xbf);
      break;

   case E_CharEncoding::UTF16_LE:
      write_utf16(0xfeff, false);
      break;

   case E_CharEncoding::UTF16_BE:
      write_utf16(0xfeff, true);
      break;

   default:
      // E_CharEncoding::ASCII
      // E_CharEncoding::BYTE
      // do nothing
      // Coveralls will complain
      break;
   }
}


void write_char(int ch)
{
   if (ch >= 0)
   {
      switch (cpd.enc)
      {
      case E_CharEncoding::BYTE:
         write_byte(ch & 0xff);
         break;

      case E_CharEncoding::ASCII:
      default:
         write_byte(ch);
         break;

      case E_CharEncoding::UTF8:
         write_utf8(ch);
         break;

      case E_CharEncoding::UTF16_LE:
         write_utf16(ch, false);
         break;

      case E_CharEncoding::UTF16_BE:
         write_utf16(ch, true);
         break;
      }
   }
}


void write_string(const UncText &text)
{
   for (size_t idx = 0; idx < text.size(); idx++)
   {
      write_char(text[idx]);
   }
}
