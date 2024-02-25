/**
 * @file language_names.h
 *
 * @author  Guy Maurel
 * extract from uncrustify.cpp
 * @license GPL v2+
 */
#ifndef LANGUAGE_NAMES_H_INCLUDED
#define LANGUAGE_NAMES_H_INCLUDED

#include "base_types.h"

#include <cstddef>      // to get size_t
#include <cstdio>
#include <map>
#include <string>
#include <string.h>

struct lang_name_t
{
   const char *name;
   size_t     lang;
};

//! list of all programming languages Uncrustify supports
enum class lang_flag_e
{
   LANG_C          = 0x0001,
   LANG_CPP        = 0x0002,
   LANG_D          = 0x0004,
   LANG_CS         = 0x0008,     //! C# (C-Sharp)
   LANG_JAVA       = 0x0010,
   LANG_OC         = 0x0020,     //! Objective-C
   LANG_VALA       = 0x0040,
   LANG_PAWN       = 0x0080,
   LANG_ECMA       = 0x0100,     //! ECMA Script (JavaScript)

   LANG_ALLC_NOT_C = 0x017e,     /**             LANG_CPP | LANG_D    | LANG_CS   |            Issue # 4044
                                  *  LANG_JAVA | LANG_OC  | LANG_VALA | LANG_ECMA   */
   LANG_ALLC       = 0x017f,     /** LANG_C    | LANG_CPP | LANG_D    | LANG_CS   |
                                  *  LANG_JAVA | LANG_OC  | LANG_VALA | LANG_ECMA   */
   LANG_ALL        = 0x0fff,     //! applies to all languages

   FLAG_HDR        = 0x2000,     /*<< Header file for C family languages */
   FLAG_DIG        = 0x4000,     //! digraph/trigraph
   FLAG_PP         = 0x8000,     //! only appears in a preprocessor
};


size_t language_flags_from_name(const char *name);


/**
 * Gets the tag text for a language
 *
 * @param lang  The LANG_xxx enum
 *
 * @return A string
 */
const char *language_name_from_flags(size_t lang);

bool ends_with(const char *filename, const char *tag, bool case_sensitive);


//! type to map a programming language to a typically used filename extension
struct lang_ext_t
{
   const char *ext;  //! filename extension typically used for ...
   const char *name; //! a programming language
};


/**
 * Set idx = 0 before the first call.
 * Done when returns nullptr
 */
const char *get_file_extension(int &idx);


typedef std::map<std::string, std::string> extension_map_t;
/**
 * maps a file extension to a language flag.
 *
 * @note The "." need to be included, as in ".c". The file extensions
 *       ARE case sensitive.
 */
static extension_map_t g_ext_map;


const char *extension_add(const char *ext_text, const char *lang_text);


//! Prints custom file extensions to the file
void print_extensions(FILE *pfile);


// TODO: better use enum lang_t for source file language
/**
 * Find the language for the file extension
 * Defaults to C
 *
 * @param filename   The name of the file
 * @return           LANG_xxx
 */
size_t language_flags_from_filename(const char *filename);

#endif /* LANGUAGE_NAMES_H_INCLUDED */
