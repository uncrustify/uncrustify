/**
 * @file unc_text.cpp
 * A simple class that handles the chunk text.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "unc_text.h"
#include "prototypes.h" /* encode_utf8() */


void unc_text::update_logtext()
{
   if (!m_logok)
   {
      /* make a pessimistic guess at the size */
      m_logtext.reserve(m_chars.size() * 3);
      for (value_type::iterator it = m_chars.begin(); it != m_chars.end(); ++it)
      {
         encode_utf8(*it, m_logtext);
      }
      m_logtext.push_back(0);
      m_logok = true;
   }
}

const char *unc_text::c_str()
{
   update_logtext();
   return (const char *)&m_logtext[0];
}

void unc_text::set(const unc_text& ref)
{
   m_chars = ref.m_chars;
   m_logok = false;
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

void fix_len_idx(int size, int& idx, int& len)
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

void unc_text::set(const value_type& data, int idx, int len)
{
   fix_len_idx(data.size(), idx, len);
   m_chars.resize(len);
   while (len-- > 0)
   {
      m_chars[idx] = data[idx];
      idx++;
   }
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
