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
 * Initializes keywords table
 */
void init_keywords(void);

/**
 * Loads the dynamic keywords from a file
 *
 * @param filename  The path to the file to load
 *
 * @retval EX_OK     successfully read keywords from file
 * @retval EX_IOERR  reading keywords file failed
 */
int load_keyword_file(const char *filename);


/**
 * Search first the dynamic and then the static table for a matching keyword
 *
 * @param word  Pointer to the text -- NOT zero terminated
 * @param len   The length of the text
 * @param TODO: (doc enableDynamicSubstitution)
 *
 * @return CT_WORD (no match) or the keyword token
 */
c_token_t find_keyword_type(const char *word, size_t len, bool enableDynamicSubstitution = true);


/**
 * Adds a keyword to the list of dynamic keywords
 *
 * @param tag   The tag (string) must be zero terminated
 * @param type  The type, usually CT_TYPE
 */
void add_keyword(const char *tag, c_token_t type);


/*
 * Removes a keyword from the list of dynamic keywords
 *
 * @param tag        The tag (string)
 */
void remove_keyword(const std::string &tag);


void print_keywords(FILE *pfile);


void clear_keyword_file(void);


//! Returns the pattern that the keyword needs based on the token
pattern_class_e get_token_pattern_class(c_token_t tok);


bool keywords_are_sorted(void);


#endif /* KEYWORDS_H_INCLUDED */
