/**
 * @file keywords.h
 * prototypes for keywords.cpp
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef KEYWORDS_H_INCLUDED
#define KEYWORDS_H_INCLUDED

#include "language_names.h"
#include "uncrustify_types.h"

/**
 * Initializes keywords table for a given language.
 *
 * Let us have a look on the problem is caused by the tokens "in" and "out",
 * used in the file "Issue_3353.h" under.
 * The strings representing the tokens ("in" and "out"), are found in the
 * original table: static chunk_tag_t keywords[] because they are used by
 * other languages. They are tokenized as CT_IN and CT_OUT.
 * The correct tokenization is CT_FUNC_VAR.
 *
 * It is necessary to create (at run time) a new table with all the keywords
 * proper to the used language.
 *
 */

/**
 * The file
 * "Issue_3353.h"
 *    struct A {
 *        void (*in)(
 *            void);
 *        void (*out)(
 *            void);
 *    };
 */
void init_keywords_for_language();

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
 *
 * @return CT_WORD (no match) or the keyword token
 */
E_Token find_keyword_type(const char *word, size_t len);


/**
 * Adds a keyword to the list of dynamic keywords
 *
 * @param tag   The tag (string) must be zero terminated
 * @param type  The type, usually CT_TYPE
 */
void add_keyword(const std::string &tag, E_Token type);


void print_custom_keywords(FILE *pfile);


void clear_keyword_file();


//! Returns the pattern that the keyword needs based on the token
pattern_class_e get_token_pattern_class(E_Token tok);


bool keywords_are_sorted();


static constexpr size_t e_LANG_C          = (size_t)lang_flag_e::LANG_C;
static constexpr size_t e_LANG_CPP        = (size_t)lang_flag_e::LANG_CPP;
static constexpr size_t e_LANG_D          = (size_t)lang_flag_e::LANG_D;
static constexpr size_t e_LANG_CS         = (size_t)lang_flag_e::LANG_CS;
static constexpr size_t e_LANG_JAVA       = (size_t)lang_flag_e::LANG_JAVA;
static constexpr size_t e_LANG_OC         = (size_t)lang_flag_e::LANG_OC;
static constexpr size_t e_LANG_VALA       = (size_t)lang_flag_e::LANG_VALA;
static constexpr size_t e_LANG_PAWN       = (size_t)lang_flag_e::LANG_PAWN;
static constexpr size_t e_LANG_ECMA       = (size_t)lang_flag_e::LANG_ECMA;
static constexpr size_t e_LANG_ALLC_NOT_C = (size_t)lang_flag_e::LANG_ALLC_NOT_C;
static constexpr size_t e_LANG_ALLC       = (size_t)lang_flag_e::LANG_ALLC;
static constexpr size_t e_LANG_ALL        = (size_t)lang_flag_e::LANG_ALL;
static constexpr size_t e_FLAG_HDR        = (size_t)lang_flag_e::FLAG_HDR;
static constexpr size_t e_FLAG_DIG        = (size_t)lang_flag_e::FLAG_DIG;
static constexpr size_t e_FLAG_PP         = (size_t)lang_flag_e::FLAG_PP;


#endif /* KEYWORDS_H_INCLUDED */
