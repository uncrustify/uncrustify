/**
 * @file frame_list.h
 * Functions for the cpd.frames var, mainly used to handle preprocessor stuff
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef PARSE_FRAME_H_INCLUDED
#define PARSE_FRAME_H_INCLUDED

#include "uncrustify_types.h"
#include "ParseFrame.h"


/**
 * Push a copy of a ParseFrame onto the frames list.
 * This is called on #if and #ifdef.
 */
void fl_push(ParseFrame &pf);


/**
 * Push a copy of a ParseFrame before the last element on the frames list.
 * If this were a linked list, just add before the last item.
 * This is called on the first #else and #elif.
 */
void fl_push_under(ParseFrame &pf);


/**
 * Copy the top element of the frame list into the ParseFrame.
 *
 * If the frame list is empty nothing happens.
 *
 * This is called on #else and #elif.
 */
void fl_copy_tos(ParseFrame &pf);


/**
 * Copy the 2nd top element off the list into the ParseFrame.
 * This is called on #else and #elif.
 * The stack contains [...] [base] [if] at this point.
 * We want to copy [base].
 */
void fl_copy_2nd_tos(ParseFrame &pf);


//! Deletes the top element from the list.
void fl_trash_tos(void);


/**
 * Pop the top element off the frame list and copy it into the ParseFrame.
 *
 * Does nothing if the frame list is empty.
 *
 * This is called on #endif
 */
void fl_pop(ParseFrame &pf);


//! Returns the pp_indent to use for this line
int fl_check(ParseFrame &frm, chunk_t *pc);


#endif /* PARSE_FRAME_H_INCLUDED */
