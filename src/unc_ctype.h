/**
 * @file unc_ctype.h
 * The ctype function are only required to handle values 0-255 and EOF.
 * A char is sign-extended when cast to an int.
 * With some C libraries, these values cause a crash.
 * These wrappers will properly handle all char values.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef UNC_CTYPE_H_INCLUDED
#define UNC_CTYPE_H_INCLUDED

#include "options.h"

#include <cctype>

// TODO: better avoid inline and move implementation to cpp file


//! Truncate anything except EOF (-1) to 0-255
static inline int unc_fix_ctype(int ch)
{
   if (ch == -1)
   {
      return(ch);
   }

   if (ch > 255)
   {
      return(0);                                  // Issue #3025
   }

   if (ch < -1)
   {
      return(0);                                  // Issue #3025
   }
   return(ch);

   //return((ch == -1) ? -1 : (ch & 0xff));
   //return(ch);
}


//! check if a character is a space
static inline int unc_isspace(int ch)
{
   if (  (ch == 12)                          // Issue #2386
      && uncrustify::options::use_form_feed_no_more_as_whitespace_character())
   {
      return(0);
   }
   else
   {
      return(isspace(unc_fix_ctype(ch)));
      //return(isspace(ch));                   // Issue #3025
   }
}


//! check if a character is a printing character
static inline int unc_isprint(int ch)
{
   return(isprint(unc_fix_ctype(ch)));
   //return(isprint(ch));
}


//! check if a character is an alphabetic character (a letter).
static inline int unc_isalpha(int ch)
{
   return(isalpha(unc_fix_ctype(ch)));
   //return(isalpha(ch));
}


//! check if a character is an alphanumeric character.
static inline int unc_isalnum(int ch)
{
   return(isalnum(unc_fix_ctype(ch)));
   //return(isalnum(ch));
}


//! convert a character to upper case
static inline int unc_toupper(int ch)
{
   return(toupper(unc_fix_ctype(ch)));
   //return(toupper(ch));
}


//! convert a character to lower case
static inline int unc_tolower(int ch)
{
   return(tolower(unc_fix_ctype(ch)));
   //return(tolower(ch));
}


//! check if a character is a hexadecimal digit
static inline int unc_isxdigit(int ch)
{
   return(isxdigit(unc_fix_ctype(ch)));
   //return(isxdigit(ch));
}


//! check if a character is a decimal digit
static inline int unc_isdigit(int ch)
{
   return(isdigit(unc_fix_ctype(ch)));
   //return(isdigit(ch));
}


//! check if a character is upper case
static inline int unc_isupper(int ch)
{
   return(  isalpha(unc_fix_ctype(ch))
         && (unc_toupper(unc_fix_ctype(ch)) == ch));
   //return(  isalpha(ch)
   //      && (unc_toupper(ch) == ch));
}


#endif /* UNC_CTYPE_H_INCLUDED */
