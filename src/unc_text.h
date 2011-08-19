/**
 * @file unc_text.h
 * A simple class that handles the chunk text.
 * At the start of processing, the entire file is decoded into a vector of ints.
 * This class is intended to hold sections of that large vector.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef UNC_TEXT_H_INCLUDED
#define UNC_TEXT_H_INCLUDED

#include "base_types.h"
#include <vector>
#include <deque>
#include <string>
using namespace std;

class unc_text
{
public:
   typedef deque<int> value_type;

public:
   unc_text() : m_logok(false)
   {
   }
   ~unc_text()
   {
   }
   unc_text(const unc_text& ref)
   {
      set(ref);
   }
   unc_text(const char *ascii_text)
   {
      set(ascii_text);
   }
   unc_text(const string& ascii_text)
   {
      set(ascii_text);
   }
   unc_text(const value_type& data, int idx = 0, int len = -1)
   {
      set(data, idx, len);
   }

   void set(const unc_text& ref);
   void set(const string& ascii_text);
   void set(const char *ascii_text);
   void set(const value_type& data, int idx = 0, int len = -1);

   void append(const unc_text& ref);
   void append(const string& ascii_text);
   void append(const char *ascii_text);
   void append(const value_type& data, int idx = 0, int len = -1);

   unc_text& operator +=(const unc_text& ref)
   {
      append(ref);
      return(*this);
   }
   unc_text& operator +=(const string& ascii_text)
   {
      append(ascii_text);
      return(*this);
   }
   unc_text& operator +=(const char *ascii_text)
   {
      append(ascii_text);
      return(*this);
   }

   /* grab the number of characters */
   int size() const
   {
      return m_chars.size();
   }

   /* get the UTF-8 string for logging */
   const char *c_str();

   /* grab the data as a series of ints for outputting to a file */
   value_type& get()
   {
      m_logok = false;
      return m_chars;
   }

   const value_type& get() const
   {
      return m_chars;
   }

   int operator[](size_t idx) const
   {
      return(((idx >= 0) && (idx < m_chars.size())) ? m_chars[idx] : 0);
   }

   /* throws an exception if out of bounds */
   int& at(int idx)
   {
      return m_chars.at(idx);
   }

protected:
   void update_logtext();

   /* this contains the non-encoded 31-bit chars */
   value_type    m_chars;

   /* logging text, utf8 encoded - updated in c_str() */
   vector<UINT8> m_logtext;
   bool          m_logok;
};


#endif /* UNC_TEXT_H_INCLUDED */
