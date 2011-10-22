/**
 * @file unc_text.cpp
 * A simple class that handles the chunk text.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "unc_text.h"
#include "prototypes.h" /* encode_utf8() */

static void fix_len_idx(int size, int& idx, int& len)
{
   if (len < 0)
   {
      len = size;
   }
   if (len > 0)
   {
      if (idx < 0)
      {
         idx += size;
         if (idx < 0)
         {
            idx = 0;
            len = 0;
            return;
         }
      }
      if (idx >= size)
      {
         len = 0;
      }
      else
      {
         int left = size - idx;
         if (len > left)
         {
            len = left;
         }
      }
   }
}

void unc_text::update_logtext()
{
   if (!m_logok)
   {
      /* make a pessimistic guess at the size */
      m_logtext.clear();
      m_logtext.reserve(m_chars.size() * 3);
      for (value_type::iterator it = m_chars.begin(); it != m_chars.end(); ++it)
      {
         encode_utf8(*it, m_logtext);
      }
      m_logtext.push_back(0);
      m_logok = true;
   }
}

int unc_text::compare(const unc_text& ref1, const unc_text& ref2, int len)
{
   int idx, len1, len2;
   len1 = ref1.size();
   len2 = ref2.size();

   if (len > 0)
   {
      for (idx = 0; (idx < len1) && (idx < len2) && (idx < len); idx++)
      {
         if (ref1.m_chars[idx] != ref2.m_chars[idx])
         {
            return(ref1.m_chars[idx] - ref2.m_chars[idx]);
         }
      }
      if (idx == len)
      {
         return 0;
      }
      return(len1 - len2);
   }

   for (idx = 0; (idx < len1) && (idx < len2); idx++)
   {
      if (ref1.m_chars[idx] != ref2.m_chars[idx])
      {
         return(ref1.m_chars[idx] - ref2.m_chars[idx]);
      }
   }
   return (len1 - len2);
}

bool unc_text::equals(const unc_text& ref) const
{
   int len = size();
   if (ref.size() != len)
   {
      return false;
   }
   for (int idx = 0; idx < len; idx++)
   {
      if (m_chars[idx] != ref.m_chars[idx])
      {
         return false;
      }
   }
   return true;
}

const char *unc_text::c_str()
{
   update_logtext();
   return (const char *)&m_logtext[0];
}

void unc_text::set(int ch)
{
   m_chars.clear();
   m_chars.push_back(ch);
   m_logok = false;
}

void unc_text::set(const unc_text& ref)
{
   m_chars = ref.m_chars;
   m_logok = false;
}

void unc_text::set(const unc_text& ref, int idx, int len)
{
   int size = ref.size();
   fix_len_idx(size, idx, len);
   m_logok = false;
   if ((idx == 0) && (len == size))
   {
      m_chars = ref.m_chars;
   }
   else
   {
      m_chars.resize(len);
      int di = 0;
      while (len-- > 0)
      {
         m_chars[di++] = ref.m_chars[idx++];
      }
   }
}

void unc_text::set(const string& ascii_text)
{
   int len = ascii_text.size();

   m_chars.resize(len);
   for (int idx = 0; idx < len; idx++)
   {
      m_chars[idx] = ascii_text[idx];
   }
   m_logok = false;
}

void unc_text::set(const char *ascii_text)
{
   int len = strlen(ascii_text);

   m_chars.resize(len);
   for (int idx = 0; idx < len; idx++)
   {
      m_chars[idx] = *ascii_text++;
   }
   m_logok = false;
}

void unc_text::set(const value_type& data, int idx, int len)
{
   fix_len_idx(data.size(), idx, len);
   m_chars.resize(len);
   int di = 0;
   while (len-- > 0)
   {
      m_chars[di++] = data[idx++];
   }
   m_logok = false;
}

void unc_text::resize(size_t new_size)
{
   if (size() != (int)new_size)
   {
      m_chars.resize(new_size);
      m_logok = false;
   }
}

void unc_text::clear()
{
   m_chars.clear();
   m_logok = false;
}

void unc_text::insert(int idx, int ch)
{
   if (idx >= 0)
   {
      m_chars.insert(m_chars.begin() + idx, ch);
      m_logok = false;
   }
}

void unc_text::append(int ch)
{
   m_chars.push_back(ch);
   m_logok = false;
}

void unc_text::append(const unc_text& ref)
{
   m_chars.insert(m_chars.end(), ref.m_chars.begin(), ref.m_chars.end());
   m_logok = false;
}

void unc_text::append(const string& ascii_text)
{
   unc_text tmp(ascii_text);
   append(tmp);
}

void unc_text::append(const char *ascii_text)
{
   unc_text tmp(ascii_text);
   append(tmp);
}

void unc_text::append(const value_type& data, int idx, int len)
{
   unc_text tmp(data, idx, len);
   append(tmp);
}

bool unc_text::startswith(const char *text, int idx) const
{
   bool match = false;
   while ((idx < size()) && *text)
   {
      if (*text != m_chars[idx])
      {
         return false;
      }
      idx++;
      text++;
      match = true;
   }
   return(match && (*text == 0));
}

bool unc_text::startswith(const unc_text& text, int idx) const
{
   bool match = false;
   int  si = 0;
   while ((idx < size()) && (si < text.size()))
   {
      if (text.m_chars[si] != m_chars[idx])
      {
         return false;
      }
      idx++;
      si++;
      match = true;
   }
   return(match && (si == text.size()));
}

int unc_text::find(const char *text, int sidx) const
{
   int len  = strlen(text);
   int midx = size() - len;
   int idx, ii;

   for (idx = sidx; idx < midx; idx++)
   {
      bool match = true;
      for (ii = 0; ii < len; ii++)
      {
         if (m_chars[idx + ii] != text[ii])
         {
            match = false;
            break;
         }
      }
      if (match)
      {
         return idx;
      }
   }
   return -1;
}
