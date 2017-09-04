/**
 * @file space.h
 * prototypes for space.c
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
void space_text(void);


//! Marches through the whole file and adds spaces around nested parenthesis
void space_text_balance_nested_parens(void);


//! Determines if a space is required between two chunks
size_t space_needed(chunk_t *first, chunk_t *second);


/**
 * Calculates the column difference between two chunks.
 * The rules are bent a bit here, as AV_IGNORE and AV_ADD become AV_FORCE.
 * So the column difference is either first->len or first->len + 1.
 *
 * @param first   The first chunk
 * @param second  The second chunk
 *
 * @return the column difference between the two chunks
 */
size_t space_col_align(chunk_t *first, chunk_t *second);


void space_add_after(chunk_t *pc, size_t count);


#endif /* SPACE_H_INCLUDED */
