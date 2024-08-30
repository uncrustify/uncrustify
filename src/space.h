/**
 * @file space.h
 * prototypes for space.cpp
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef SPACE_H_INCLUDED
#define SPACE_H_INCLUDED

#include "uncrustify_types.h"


/**
 * Marches through the whole file and checks to see how many spaces should be
 * between two chunks
 */
void space_text();


//! Marches through the whole file and adds spaces around nested parenthesis
void space_text_balance_nested_parens();


//! Determines if a space is required between two chunks
size_t space_needed(Chunk *first, Chunk *second);


/**
 * Calculates the column difference between two chunks.
 * The rules are bent a bit here, as IARF_IGNORE and IARF_ADD become IARF_FORCE.
 * So the column difference is either first->len or first->len + 1.
 *
 * @param first   The first chunk
 * @param second  The second chunk
 *
 * @return the column difference between the two chunks
 */
size_t space_col_align(Chunk *first, Chunk *second);


void space_add_after(Chunk *pc, size_t count);


const char *decode_IARF(uncrustify::iarf_e av);

#endif /* SPACE_H_INCLUDED */
