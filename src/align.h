/**
 * @file align.h
 * prototypes for align.c
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef ALIGN_H_INCLUDED
#define ALIGN_H_INCLUDED

#include "uncrustify_types.h"


void quick_align_again(void);


void align_all(void);


/**
 * Aligns all backslash-newline combos in the file.
 * This should be done LAST.
 */
void align_backslash_newline(void);


void align_right_comments(void);


//! Scans the whole file for #defines. Aligns all within X lines of each other
void align_preprocessor(void);


//! Aligns stuff inside a multi-line "= { ... }" sequence.
void align_struct_initializers(void);


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
 * Aligns all assignment operators on the same level as first, starting with
 * first.
 * For variable definitions, only consider the '=' for the first variable.
 * Otherwise, only look at the first '=' on the line.
 *
 * @param first  chunk pointing to the first assignment
 */
chunk_t *align_assign(chunk_t *first, size_t span, size_t thresh, size_t *p_nl_count);


chunk_t *step_back_over_member(chunk_t *pc);


/**
 * Shifts out all columns by a certain amount.
 *
 * @param idx  The index to start shifting
 * @param num  The number of columns to shift
 */
void ib_shift_out(size_t idx, size_t num);


#endif /* ALIGN_H_INCLUDED */
