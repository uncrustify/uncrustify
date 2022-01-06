/**
 * @file align_trailing_comments.h
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef ALIGN_TRAILING_COMMENTS_H_INCLUDED
#define ALIGN_TRAILING_COMMENTS_H_INCLUDED

#include "chunk.h"
#include "ChunkStack.h"

enum class comment_align_e : unsigned int
{
   REGULAR,
   BRACE,
   ENDIF,
};

/**
 * For a series of lines ending in a comment, align them.
 * The series ends when more than align_right_cmt_span newlines are found.
 *
 * Interesting info:
 *  - least physically allowed column
 *  - intended column
 *  - least original cmt column
 *
 * min_col is the minimum allowed column (based on prev token col/size)
 * cmt_col less than
 *
 * @param start   Start point
 * @return        pointer the last item looked at
 */
Chunk *align_trailing_comments(Chunk *start);

comment_align_e get_comment_align_type(Chunk *cmt);

void align_stack(ChunkStack &cs, size_t col, bool align_single, log_sev_t sev);

void align_right_comments(void);

#endif /* ALIGN_TRAILING_COMMENTS_H_INCLUDED */
