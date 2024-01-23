/**
 * @file unc_text.h
 * A simple class that handles the chunk text.
 * At the start of processing, the entire file is decoded into a std::vector of ints.
 * This class is intended to hold sections of that large std::vector.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef UNC_TEXT_H_INCLUDED
#define UNC_TEXT_H_INCLUDED

#include "base_types.h"

#include <deque>
#include <string>
#include <vector>


/**
 *  abbreviations used:
 * - unc_text - uncrustify text
 */

class UncText
{
public:
   typedef std::deque<int>      value_type; // double encoded list of int values
   typedef std::vector<UINT8>   log_type;


   UncText();

   UncText(const UncText &ref);

   UncText(const UncText &ref, size_t idx, size_t len = 0);

   UncText(const char *ascii_text);

   UncText(const std::string &ascii_text);

   UncText(const value_type &data, size_t idx = 0, size_t len = 0);


   ~UncText() = default;


   void resize(size_t new_size);


   void clear();


   //! grab the number of characters
   size_t size() const;


   void set(int ch);

   void set(const UncText &ref);

   void set(const UncText &ref, size_t idx, size_t len = 0);

   void set(const std::string &ascii_text);

   void set(const char *ascii_text);

   void set(const value_type &data, size_t idx = 0, size_t len = 0);


   UncText &operator=(int ch);

   UncText &operator=(const UncText &ref);

   UncText &operator=(const std::string &ascii_text);

   UncText &operator=(const char *ascii_text);


   void insert(size_t idx, int ch);

   void insert(size_t idx, const UncText &ref);


   void erase(size_t idx, size_t len = 1);


   //! Add a unc_text character to an unc_text
   void append(int ch);

   void append(const UncText &ref);

   //! Add a string to an unc_text
   void append(const std::string &ascii_text);

   /**
    * Add a variable length string to an unc_text.
    * The variable length string format is similar as for printf
    *
    * @note the overall length of the string must not exceed 256 characters
    *
    *	@param ascii_text  a variable length string
    */
   void append(const char *ascii_text);

   void append(const value_type &data, size_t idx = 0, size_t len = 0);


   UncText &operator+=(int ch);

   UncText &operator+=(const UncText &ref);

   UncText &operator+=(const std::string &ascii_text);

   UncText &operator+=(const char *ascii_text);


   //! Returns the UTF-8 string for logging
   const char *c_str() const;


   /**
    * compares the content of two unc_text instances
    *
    * @param ref1  first  instance to compare
    * @param ref2  second instance to compare
    * @param len   number of character to compare
    * @param tcare take care of case (lower case/ upper case)                  Issue #2091
    *
    * @retval == 0  both text elements are equal
    * @retval  > 0
    * @retval  < 0
    */
   static int compare(const UncText &ref1, const UncText &ref2, size_t len = 0, bool tcare = false);


   bool equals(const UncText &ref) const;


   //! grab the data as a series of ints for outputting to a file
   const value_type &get() const;


   int operator[](size_t idx) const;


   // throws an exception if out of bounds
   const int &at(size_t idx) const;
   int &at(size_t idx);


   //! returns the last element of the character list
   const int &back() const;


   void push_back(int ch);


   void pop_back();


   void pop_front();


   bool startswith(const UncText &text, size_t idx = 0) const;

   bool startswith(const char *text, size_t idx = 0) const;


   /**
    * look for 'text', beginning with position 'idx'
    *
    * @param text  text to search for
    * @param idx   position to start search
    *
    * @return == -1 if not found
    * @return >=  0 the position
    */
   int find(const char *text, size_t idx = 0) const;


   int rfind(const char *text, size_t idx = 0) const;


   int replace(const char *oldtext, const UncText &newtext);


protected:
   void update_logtext();

   value_type m_chars;           //! this contains the non-encoded 31-bit chars
   log_type   m_logtext;         //! logging text, utf8 encoded - updated in c_str()
};

#endif /* UNC_TEXT_H_INCLUDED */
