/**
 * @file chunk_pos.h
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef NEWLINES_CHUNK_POS_H_INCLUDED
#define NEWLINES_CHUNK_POS_H_INCLUDED

#include "option.h"
#include "token_enum.h"

/**
 * Searches for a chunk of type chunk_type and moves them, if needed.
 * Will not move tokens that are on their own line or have other than
 * exactly 1 newline before (options::pos_comma() == TRAIL) or after (options::pos_comma() == LEAD).
 * We can't remove a newline if it is right before a preprocessor.
 */
void newlines_chunk_pos(E_Token chunk_type, uncrustify::token_pos_e mode);

#endif /* NEWLINES_CHUNK_POS_H_INCLUDED */
