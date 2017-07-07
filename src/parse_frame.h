/**
 * @file parse_frame.h
 * prototypes for parse_frame.c
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef PARSE_FRAME_H_INCLUDED
#define PARSE_FRAME_H_INCLUDED

#include "uncrustify_types.h"


//! Copies src to dst
void pf_copy(parse_frame_t *dst, const parse_frame_t *src);


/**
 * Push a copy of the parse frame onto the stack.
 * This is called on #if and #ifdef.
 */
void pf_push(parse_frame_t *pf);


/**
 * Push a copy of the parse frame onto the stack, under the tos.
 * If this were a linked list, just add before the last item.
 * This is called on the first #else and #elif.
 */
void pf_push_under(parse_frame_t *pf);


/**
 * Copy the top item off the stack into pf.
 * This is called on #else and #elif.
 */
void pf_copy_tos(parse_frame_t *pf);


/**
 * Copy the 2nd top item off the stack into pf.
 * This is called on #else and #elif.
 * The stack contains [...] [base] [if] at this point.
 * We want to copy [base].
 */
void pf_copy_2nd_tos(struct parse_frame *pf);


//! Deletes the top frame from the stack.
void pf_trash_tos(void);


/**
 * Pop the top item off the stack and copy into pf.
 * This is called on #endif
 */
void pf_pop(parse_frame_t *pf);


//! Returns the pp_indent to use for this line
int pf_check(parse_frame_t *frm, chunk_t *pc);


#endif /* PARSE_FRAME_H_INCLUDED */
