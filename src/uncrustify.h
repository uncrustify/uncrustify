/**
 * @file uncrustify.h
 * prototypes for uncrustify.c
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef UNCRUSTIFY_H_INCLUDED
#define UNCRUSTIFY_H_INCLUDED

#include <stdio.h>
#include "token_enum.h"
#include "log_levels.h"
#include "base_types.h"
#include "uncrustify_types.h"

int load_header_files(void);


void uncrustify_file(const file_mem &fm, FILE *pfout, const char *parsed_file, bool defer_uncrustify_end = false);


void uncrustify_end();


const char *get_token_name(c_token_t token);


/**
 * Gets the tag text for a language
 *
 * @param lang  The LANG_xxx enum
 *
 * @return A string
 */
const char *language_name_from_flags(size_t lang);


/**
 * Grab the token id for the text.
 *
 * @return token, will be CT_NONE on failure to match
 */
c_token_t find_token_name(const char *text);


void log_pcf_flags(log_sev_t sev, UINT64 flags);


/**
 * Replace the brain-dead and non-portable basename().
 * Returns a pointer to the character after the last '/'.
 * The returned value always points into path, unless path is nullptr.
 *
 * Input            Returns
 * nullptr       => ""
 * "/some/path/" => ""
 * "/some/path"  => "path"
 * "afile"       => "afile"
 *
 * @param path  The path to look at
 *
 * @return Pointer to the character after the last path separator
 */
const char *path_basename(const char *path);


/**
 * Returns the length of the directory part of the filename.
 *
 * @param filename  filename including full path
 *
 * @return character size of path
 */
int path_dirname_len(const char *filename);


void usage_exit(const char *msg, const char *argv0, int code);


/**
 * Set idx = 0 before the first call.
 * Done when returns nullptr
 */
const char *get_file_extension(int &idx);


//! Prints custom file extensions to the file
void print_extensions(FILE *pfile);


const char *extension_add(const char *ext_text, const char *lang_text);


#endif /* UNCRUSTIFY_H_INCLUDED */
