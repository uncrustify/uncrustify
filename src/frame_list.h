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
