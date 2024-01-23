/**
 * @file uncrustify.h
 * prototypes for uncrustify.cpp
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef UNCRUSTIFY_H_INCLUDED
#define UNCRUSTIFY_H_INCLUDED

#include "uncrustify_types.h"

#include <stdio.h>


int load_header_files();


void uncrustify_file(const file_mem &fm, FILE *pfout, const char *parsed_file, const char *dump_filename, bool is_quiet, bool defer_uncrustify_end = false);


void uncrustify_end();


const char *get_token_name(E_Token token);


/**
 * Grab the token id for the text.
 *
 * @return token, will be CT_NONE on failure to match
 */
E_Token find_token_name(const char *text);


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


void usage(const char *argv0);


void usage_error(const char *msg = nullptr);

#endif /* UNCRUSTIFY_H_INCLUDED */
