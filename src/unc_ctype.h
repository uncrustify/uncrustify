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

#include <cctype>                    // to get std::tolower

//! Test anything EOF (-1) to 0-255
int unc_fix_ctype(int ch);


//! check if a character is a space
int unc_isspace(int ch);


//! check if a character is a printing character
int unc_isprint(int ch);


//! check if a character is an alphabetic character (a letter).
int unc_isalpha(int ch);


//! check if a character is an alphanumeric character.
int unc_isalnum(int ch);


//! convert a character to upper case
int unc_toupper(int ch);


//! convert a character to lower case
int unc_tolower(int ch);


//! check if a character is a hexadecimal digit
int unc_isxdigit(int ch);


//! check if a character is a decimal digit
int unc_isdigit(int ch);


//! check if a character is upper case
int unc_isupper(int ch);

#endif /* UNC_CTYPE_H_INCLUDED */
