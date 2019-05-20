/**
 * @file align_nl_cont.h
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef ALIGN_NL_COUNT_H_INCLUDED
#define ALIGN_NL_COUNT_H_INCLUDED

#include "uncrustify_types.h"

#include "chunk_list.h"

/**
 * For a series of lines ending in backslash-newline, align them.
 * The series ends when a newline or multi-line C comment is encountered.
 *
 * @param start   Start point
 *
 * @return pointer the last item looked at (nullptr/newline/comment)
 */
chunk_t *align_nl_cont(chunk_t *start);

/**
 * Aligns all backslash-newline combos in the file.
 * This should be done LAST.
 */
void align_backslash_newline(void);

#endif /* ALIGN_NL_COUNT_H_INCLUDED */
