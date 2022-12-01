/**
 * @file check_template.h
 * prototypes for check_template.cpp
 *
 * splitted from tokenize_cleanup.cpp
 *
 * @author  Guy Maurel 2022
 * @license GPL v2+
 */
#ifndef CHECK_TEMPLATE_H_INCLUDED
#define CHECK_TEMPLATE_H_INCLUDED

#include "uncrustify_types.h"


/**
 * Mark types in a single template argument.
 *
 * @param start  chunk to start check at
 * @param end    chunk to end check at
 */
void check_template_arg(Chunk *start, Chunk *end);


/**
 * Mark types in template argument(s).
 *
 * @param start  chunk to start check at
 * @param end    chunk to end check at
 */
void check_template_args(Chunk *start, Chunk *end);


/**
 * If there is nothing but CT_WORD and CT_MEMBER, then it's probably a
 * template thingy.  Otherwise, it's likely a comparison.
 *
 * @param start  chunk to start check at
 */
void check_template(Chunk *start, bool in_type_cast);


/**
 * Convert '>' + '>' into '>>'
 * If we only have a single '>', then change it to CT_COMPARE.
 *
 * @param pc  chunk to start at
 */
Chunk *handle_double_angle_close(Chunk *pc);


bool invalid_open_angle_template(Chunk *prev);


#endif /* CHECK_TEMPLATE_H_INCLUDED */
