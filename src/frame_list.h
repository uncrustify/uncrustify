/**
 * @file frame_list.h
 * mainly used to handle preprocessor stuff
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef PARSE_FRAME_H_INCLUDED
#define PARSE_FRAME_H_INCLUDED

#include "ParseFrame.h"
#include "uncrustify_types.h"


/**
 * Push a copy of a ParseFrame onto the frames list.
 * This is called on #if and #ifdef.
 */
void fl_push(std::vector<ParseFrame> &frames, ParseFrame &frm);


/**
 * Pop the top element off the frame list and copy it into the ParseFrame.
 *
 * Does nothing if the frame list is empty.
 *
 * This is called on #endif
 */
void fl_pop(std::vector<ParseFrame> &frames, ParseFrame &pf);


// TODO: this name is dumb:
// - what is it checking?
// - why does is much more than simple checks, it allters kinds of stuff
//! Returns the pp_indent to use for this line
int fl_check(std::vector<ParseFrame> &frames, ParseFrame &frm, int &pp_level, chunk_t *pc);


#endif /* PARSE_FRAME_H_INCLUDED */
