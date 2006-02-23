/**
 * @file chunk_stack.h
 * Manages a simple stack of chunks
 *
 * $Id: chunk_stack.h,v 1.2 2006/02/13 03:30:20 bengardner Exp $
 */

#ifndef CHUNK_STACK_H_INCLUDED
#define CHUNK_STACK_H_INCLUDED

#include "cparse_types.h"

/*TODO: redo this so that multiple stacks can co-exists.
 * example:
 *
 * chunk_stack_t  *cs_new(void);
 * void            cs_delete(chunk_stack_t *cs);
 * void            cs_reset(chunk_stack_t *cs);
 * void            cs_push(chunk_stack_t *cs, chunk_t *pc);
 * chunk_t *cs_pop(chunk_stack_t *cs);
 * int             cs_len(chunk_stack_t *cs);
 *
 * Would be even better to make this a class that can be put on the stack.
 * Maybe this should be my first D project?
 *
 */

void           cs_reset(void);

void           cs_push(chunk_t *pc);

chunk_t *cs_pop(void);

int            cs_len(void);


#endif   /* CHUNK_STACK_H_INCLUDED */

