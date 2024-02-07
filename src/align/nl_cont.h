/**
 * @file nl_cont.h
 *
 * @author  Guy Maurel
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef ALIGN_NL_COUNT_H_INCLUDED
#define ALIGN_NL_COUNT_H_INCLUDED

class Chunk;

/**
 * For a series of lines ending in backslash-newline, align them.
 * The series ends when a newline or multi-line C comment is encountered.
 *
 * @param start   Start point
 *
 * @return pointer the last item looked at (null chunk/newline/comment)
 */
Chunk *align_nl_cont(Chunk *start);

/**
 * Aligns all backslash-newline combos in the file.
 * This should be done LAST.
 */
void align_backslash_newline();

#endif /* ALIGN_NL_COUNT_H_INCLUDED */
