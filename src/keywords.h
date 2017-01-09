/**
 * @file keywords.h
 * prototypes for keywords.c
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef KEYWORDS_H_INCLUDED
#define KEYWORDS_H_INCLUDED

#include "uncrustify_types.h"


/**
 * Loads the dynamic keywords from a file
 *
 * @param filename   The path to the file to load
 * @return           SUCCESS or FAILURE
 */
int load_keyword_file(const char *filename);


/**
 * Search first the dynamic and then the static table for a matching keyword
 *
 * @param word    Pointer to the text -- NOT zero terminated
 * @param len     The length of the text
 * @return        CT_WORD (no match) or the keyword token
 */
c_token_t find_keyword_type(const char *word, int len);


/**
 * Adds a keyword to the list of dynamic keywords
 *
 * @param tag        The tag (string) must be zero terminated
 * @param type       The type, usually CT_TYPE
 */
void add_keyword(const char *tag, c_token_t type);


void print_keywords(FILE *pfile);


void clear_keyword_file(void);


/**
 * Returns the pattern that the keyword needs based on the token
 */
pattern_class get_token_pattern_class(c_token_t tok);


bool keywords_are_sorted(void);

#endif /* KEYWORDS_H_INCLUDED */
