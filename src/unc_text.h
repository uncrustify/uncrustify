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
   unc_text()
      : m_logok(false)
   {
   }


   ~unc_text()
   {
   }


   unc_text(const unc_text &ref)
   {
      set(ref);
   }


   unc_text(const unc_text &ref, size_t idx, size_t len = 0)
   {
      set(ref, idx, len);
   }


   unc_text(const char *ascii_text)
   {
      set(ascii_text);
   }


   unc_text(const string &ascii_text)
   {
      set(ascii_text);
   }


   unc_text(const value_type &data, size_t idx = 0, size_t len = 0)
   {
      set(data, idx, len);
   }

   void resize(size_t new_size);
   void clear();


   /* grab the number of characters */
   size_t size() const
   {
      return(m_chars.size());
   }

   void set(int ch);
   void set(const unc_text &ref);
   void set(const unc_text &ref, size_t idx, size_t len = 0);
   void set(const string &ascii_text);
   void set(const char *ascii_text);
   void set(const value_type &data, size_t idx = 0, size_t len = 0);

   unc_text &operator =(int ch)
   {
      set(ch);
      return(*this);
   }

   unc_text &operator =(const unc_text &ref)
   {
      set(ref);
      return(*this);
   }

   unc_text &operator =(const string &ascii_text)
   {
      set(ascii_text);
      return(*this);
   }

   unc_text &operator =(const char *ascii_text)
   {
      set(ascii_text);
      return(*this);
   }

   void insert(size_t idx, int ch);
   void insert(size_t idx, const unc_text &ref);

   void erase(size_t idx, size_t len = 1);

   void append(int ch);
   void append(const unc_text &ref);
   void append(const string &ascii_text);
   void append(const char *ascii_text);
   void append(const value_type &data, size_t idx = 0, size_t len = 0);

   unc_text &operator +=(int ch)
   {
      append(ch);
      return(*this);
   }

   unc_text &operator +=(const unc_text &ref)
   {
      append(ref);
      return(*this);
   }

   unc_text &operator +=(const string &ascii_text)
   {
      append(ascii_text);
      return(*this);
   }

   unc_text &operator +=(const char *ascii_text)
   {
      append(ascii_text);
      return(*this);
   }

   /* get the UTF-8 string for logging */
   const char *c_str();

   static int compare(const unc_text &ref1, const unc_text &ref2, size_t len = 0);
   bool equals(const unc_text &ref) const;

   /* grab the data as a series of ints for outputting to a file */
   value_type &get()
   {
      m_logok = false;
      return(m_chars);
   }

   const value_type &get() const
   {
      return(m_chars);
   }

   int operator[](size_t idx) const
   {
      return((idx < m_chars.size()) ? m_chars[idx] : 0);
   }

   /* throws an exception if out of bounds */
   int &at(size_t idx)
   {
      return(m_chars.at(idx));
   }

   const int &at(size_t idx) const
   {
      return(m_chars.at(idx));
   }

   const int &back() const
   {
      return(m_chars.back());
   }

   int &back()
   {
      /* \todo returning a temporary via a reference
       * this has to be checked and probably changed */
      return(m_chars.back());
   }


   void push_back(int ch)
   {
      append(ch);
   }


   void pop_back()
   {
      if (size() > 0)
      {
         m_chars.pop_back();
         m_logok = false;
      }
   }


   void pop_front()
   {
      if (size() > 0)
      {
         m_chars.pop_front();
         m_logok = false;
      }
   }

   bool startswith(const unc_text &text, size_t idx = 0) const;
   bool startswith(const char *text, size_t idx = 0) const;

   /*
    * look for 'text', beginning with position 'sidx'
    * return value:
    *           -1: if not found
    * the position: if found
    */
   int find(const char *text, size_t idx = 0) const;

   int rfind(const char *text, size_t idx = 0) const;
   int replace(const char *oldtext, const unc_text &newtext);

protected:
   void update_logtext();

   /* this contains the non-encoded 31-bit chars */
   value_type m_chars;

   /* logging text, utf8 encoded - updated in c_str() */
   vector<UINT8> m_logtext;
   bool          m_logok;
};

#endif /* UNC_TEXT_H_INCLUDED */
