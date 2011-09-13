/**
 * @file unicode.cpp
 * Detects, read and writes characters in the proper format.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "uncrustify_types.h"
#include "prototypes.h"
#include "unc_ctype.h"
#include <cstring>
#include <cstdlib>


/**
 * See if all characters are ASCII (0-127)
 */
bool is_ascii(const vector<UINT8>& data, int& non_ascii_cnt, int& zero_cnt)
{
   non_ascii_cnt = zero_cnt = 0;
   for (int idx = 0; idx < (int)data.size(); idx++)
   {
      if (data[idx] & 0x80)
      {
         non_ascii_cnt++;
      }
      if (!data[idx])
      {
         zero_cnt++;
      }
   }
   return((non_ascii_cnt + zero_cnt) == 0);
}


/**
 * Convert the array of bytes into an array of ints
 */
bool decode_bytes(const vector<UINT8>& in_data, deque<int>& out_data)
{
   out_data.resize(in_data.size());
   for (int idx = 0; idx < (int)in_data.size(); idx++)
   {
      out_data[idx] = in_data[idx];
   }
   return true;
}


void encode_utf8(int ch, vector<UINT8>& res)
{
   if (ch < 0)
   {
      /* illegal code - do not store */
   }
   else if (ch < 0x80)
   {
      /* 0xxxxxxx */
      res.push_back(ch);
   }
   else if (ch < 0x0800)
   {
      /* 110xxxxx 10xxxxxx */
      res.push_back(0xC0 | (ch >> 6));
      res.push_back(0x80 | (ch & 0x3f));
   }
   else if (ch < 0x10000)
   {
      /* 1110xxxx 10xxxxxx 10xxxxxx */
      res.push_back(0xE0 | (ch >> 12));
      res.push_back(0x80 | ((ch >> 6) & 0x3f));
      res.push_back(0x80 | (ch & 0x3f));
   }
   else if (ch < 0x200000)
   {
      /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
      res.push_back(0xF0 | (ch >> 18));
      res.push_back(0x80 | ((ch >> 12) & 0x3f));
      res.push_back(0x80 | ((ch >> 6) & 0x3f));
      res.push_back(0x80 | (ch & 0x3f));
   }
   else if (ch < 0x4000000)
   {
      /* 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
      res.push_back(0xF8 | (ch >> 24));
      res.push_back(0x80 | ((ch >> 18) & 0x3f));
      res.push_back(0x80 | ((ch >> 12) & 0x3f));
      res.push_back(0x80 | ((ch >> 6) & 0x3f));
      res.push_back(0x80 | (ch & 0x3f));
   }
   else /* (ch <= 0x7fffffff) */
   {
      /* 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
      res.push_back(0xFC | (ch >> 30));
      res.push_back(0x80 | ((ch >> 24) & 0x3f));
      res.push_back(0x80 | ((ch >> 18) & 0x3f));
      res.push_back(0x80 | ((ch >> 12) & 0x3f));
      res.push_back(0x80 | ((ch >> 6) & 0x3f));
      res.push_back(0x80 | (ch & 0x3f));
   }
}


/**
 * Decode UTF-8 sequences from in_data and put the chars in out_data.
 * If there are any decoding errors, then return false.
 */
bool decode_utf8(const vector<UINT8>& in_data, deque<int>& out_data)
{
   int idx = 0;
   int ch, tmp, cnt;

   out_data.clear();

   /* check for UTF-8 BOM silliness and skip */
   if (in_data.size() >= 3)
   {
      if ((in_data[0] == 0xef) &&
          (in_data[1] == 0xbb) &&
          (in_data[2] == 0xbf))
      {
         /* skip it */
         idx = 3;
      }
   }

   while (idx < (int)in_data.size())
   {
      ch = in_data[idx++];
      if (ch < 0x80)                   /* 1-byte sequence */
      {
         out_data.push_back(ch);
         continue;
      }
      else if ((ch & 0xE0) == 0xC0)    /* 2-byte sequence */
      {
         ch &= 0x1F;
         cnt = 1;
      }
      else if ((ch & 0xF0) == 0xE0)    /* 3-byte sequence */
      {
         ch &= 0x0F;
         cnt = 2;
      }
      else if ((ch & 0xF8) == 0xF0)    /* 4-byte sequence */
      {
         ch &= 0x07;
         cnt = 3;
      }
      else if ((ch & 0xFC) == 0xF8)    /* 5-byte sequence */
      {
         ch &= 0x03;
         cnt = 4;
      }
      else if ((ch & 0xFE) == 0xFC)    /* 6-byte sequence */
      {
         ch &= 0x01;
         cnt = 5;
      }
      else
      {
         /* invalid UTF-8 sequence */
         return false;
      }

      while ((cnt-- > 0) && (idx < (int)in_data.size()))
      {
         tmp = in_data[idx++];
         if ((tmp & 0xC0) != 0x80)
         {
            /* invalid UTF-8 sequence */
            return false;
         }
         ch = (ch << 6) | (tmp & 0x3f);
      }
      if (cnt >= 0)
      {
         /* short UTF-8 sequence */
         return false;
      }
      out_data.push_back(ch);
   }
   return true;
}


/**
 * Extract 2 bytes from the stream and increment idx by 2
 */
static int get_word(const vector<UINT8>& in_data, int& idx, bool be)
{
   int ch;

   if ((idx + 2) > (int)in_data.size())
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
   return ch;
}


/**
 * Deocde a UTF-16 sequence.
 * Sets enc based on the BOM.
 * Must have the BOM as the first two bytes.
 */
bool decode_utf16(const vector<UINT8>& in_data, deque<int>& out_data, CharEncoding& enc)
{
   out_data.clear();

   if (in_data.size() & 1)
   {
      /* can't have and odd length */
      return false;
   }

   if (in_data.size() < 2)
   {
      /* we require the BOM or at least 1 char */
      return false;
   }

   int idx = 2;
   if ((in_data[0] == 0xfe) && (in_data[1] == 0xff))
   {
      enc = ENC_UTF16_BE;
   }
   else if ((in_data[0] == 0xff) && (in_data[1] == 0xfe))
   {
      enc = ENC_UTF16_LE;
   }
   else
   {
      /* If we have a few words, we can take a guess, assuming the first few
       * chars are ASCII */
      enc = ENC_ASCII;
      idx = 0;
      if (in_data.size() >= 6)
      {
         if ((in_data[0] == 0) && (in_data[2] == 0) && (in_data[4] == 0))
         {
            enc = ENC_UTF16_BE;
         }
         else if ((in_data[1] == 0) && (in_data[3] == 0) && (in_data[5] == 0))
         {
            enc = ENC_UTF16_LE;
         }
      }
      if (enc == ENC_ASCII)
      {
         return false;
      }
   }

   bool be = (enc == ENC_UTF16_BE);

   while (idx < (int)in_data.size())
   {
      int ch = get_word(in_data, idx, be);
      if ((ch & 0xfc00) == 0xd800)
      {
         ch  &= 0x3ff;
         ch <<= 10;
         int tmp = get_word(in_data, idx, be);
         if ((tmp & 0xfc00) != 0xdc00)
         {
            return false;
         }
         ch |= (tmp & 0x3ff);
         ch += 0x10000;
         out_data.push_back(ch);
      }
      else if (((ch >= 0) && (ch < 0xD800)) || (ch >= 0xE000))
      {
         out_data.push_back(ch);
      }
      else
      {
         /* invalid character */
         return false;
      }
   }
   return true;
}


/**
 * Looks for the BOM of UTF-16 BE/LE and UTF-8.
 * If found, set enc and return true.
 * Sets enc to ENC_ASCII and returns false if not found.
 */
bool decode_bom(const vector<UINT8>& in_data, CharEncoding& enc)
{
   enc = ENC_ASCII;
   if (in_data.size() >= 2)
   {
      if ((in_data[0] == 0xfe) && (in_data[1] == 0xff))
      {
         enc = ENC_UTF16_BE;
         return true;
      }
      else if ((in_data[0] == 0xff) && (in_data[1] == 0xfe))
      {
         enc = ENC_UTF16_LE;
         return true;
      }
      else if ((in_data.size() >= 3) &&
               (in_data[0] == 0xef) &&
               (in_data[1] == 0xbb) &&
               (in_data[2] == 0xbf))
      {
         enc = ENC_UTF8;
         return true;
      }
   }
   return false;
}


/**
 * Figure out the encoding and convert to an int sequence
 */
bool decode_unicode(const vector<UINT8>& in_data, deque<int>& out_data, CharEncoding& enc, bool& has_bom)
{
   /* check for a BOM */
   if (decode_bom(in_data, enc))
   {
      has_bom = true;
      if (enc == ENC_UTF8)
      {
         return decode_utf8(in_data, out_data);
      }
      else
      {
         return decode_utf16(in_data, out_data, enc);
      }
   }
   has_bom = false;

   /* Check for simple ASCII */
   int non_ascii_cnt;
   int zero_cnt;
   if (is_ascii(in_data, non_ascii_cnt, zero_cnt))
   {
      enc = ENC_ASCII;
      return decode_bytes(in_data, out_data);
   }

   /* There are alot of 0's in UTF-16 (~50%) */
   if ((zero_cnt > ((int)in_data.size() / 4)) &&
       (zero_cnt <= ((int)in_data.size() / 2)))
   {
      /* likely is UTF-16 */
      if (decode_utf16(in_data, out_data, enc))
      {
         return true;
      }
   }

   if (decode_utf8(in_data, out_data))
   {
      enc = ENC_UTF8;
      return true;
   }

   /* it is an unrecognized byte sequence */
   enc = ENC_BYTE;
   return decode_bytes(in_data, out_data);
}


/**
 * Write for ASCII and BYTE encoding
 */
void write_byte(int ch, FILE *pf)
{
   if (ch < 0x100)
   {
      fputc(ch, pf);
   }
   else
   {
      /* illegal code - do not store */
   }
}


/**
 * Writes a single character to a file using UTF-8 encoding
 */
void write_utf8(int ch, FILE *pf)
{
   vector<UINT8> dq;
   dq.reserve(6);

   encode_utf8(ch, dq);
   fwrite(&dq[0], dq.size(), 1, pf);
}


void write_utf16(int ch, bool be, FILE *pf)
{
   /* U+0000 to U+D7FF and U+E000 to U+FFFF */
   if (((ch >= 0) && (ch < 0xD800)) || ((ch >= 0xE000) && (ch < 0x10000)))
   {
      if (be)
      {
         fputc((ch >> 8), pf);
         fputc((ch & 0xff), pf);
      }
      else
      {
         fputc((ch & 0xff), pf);
         fputc((ch >> 8), pf);
      }
   }
   else if ((ch >= 0x10000) && (ch < 0x110000))
   {
      int v1 = ch - 0x10000;
      int w1 = 0xD800 + (v1 >> 10);
      int w2 = 0xDC00 + (v1 & 0x3ff);
      if (be)
      {
         fputc((w1 >> 8), pf);
         fputc((w1 & 0xff), pf);
         fputc((w2 >> 8), pf);
         fputc((w2 & 0xff), pf);
      }
      else
      {
         fputc((w1 & 0xff), pf);
         fputc((w1 >> 8), pf);
         fputc((w2 & 0xff), pf);
         fputc((w2 >> 8), pf);
      }
   }
   else
   {
      /* illegal code - do not store */
   }
}


void write_bom(FILE *pf, CharEncoding enc)
{
   switch (enc)
   {
   case ENC_UTF8:
      write_byte(0xef, pf);
      write_byte(0xbb, pf);
      write_byte(0xbf, pf);
      break;

   case ENC_UTF16_LE:
      write_utf16(0xfeff, false, pf);
      break;

   case ENC_UTF16_BE:
      write_utf16(0xfeff, true, pf);
      break;

   default:
      break;
   }
}


/**
 * @param ch the 31-bit char value
 */
void write_char(FILE *pf, int ch, CharEncoding enc)
{
   if (ch >= 0)
   {
      switch (enc)
      {
      case ENC_BYTE:
         write_byte(ch & 0xff, pf);
         break;

      case ENC_ASCII:
      default:
         write_byte(ch, pf);
         break;

      case ENC_UTF8:
         write_utf8(ch, pf);
         break;

      case ENC_UTF16_LE:
         write_utf16(ch, false, pf);
         break;

      case ENC_UTF16_BE:
         write_utf16(ch, true, pf);
         break;
      }
   }
}


void write_string(FILE *pf, const char *ascii_text, CharEncoding enc)
{
   while (*ascii_text)
   {
      write_char(pf, *ascii_text, enc);
      ascii_text++;
   }
}


void write_string(FILE *pf, const deque<int>& text, CharEncoding enc)
{
   for (int idx = 0; idx < (int)text.size(); idx++)
   {
      write_char(pf, text[idx], enc);
   }
}
