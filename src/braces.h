/**
 * @file braces.h
 * prototypes for braces.cpp
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef BRACES_H_INCLUDED
#define BRACES_H_INCLUDED

#include "uncrustify_types.h"


//! Change virtual braces into real braces
void do_braces();

/**
 * See the preprocessor counterpart:
 *   add_long_preprocessor_conditional_block_comment
 * in output.cpp
 */
void add_long_closebrace_comment();


/**
 * Adds a comment after the ref chunk
 * Returns the added chunk or Chunk::NullChunkPtr
 */
Chunk *insert_comment_after(Chunk *ref, E_Token cmt_type, const UncText &cmt_text);


#endif /* BRACES_H_INCLUDED */
