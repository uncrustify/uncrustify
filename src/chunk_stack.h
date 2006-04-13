/**
 * @file chunk_stack.h
 * Manages a simple stack of chunks
 *
 * $Id$
 */

#ifndef CHUNK_STACK_H_INCLUDED
#define CHUNK_STACK_H_INCLUDED

#include "uncrustify_types.h"

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
 */

void cs_reset(chunk_stack_t *cs);

void cs_push(chunk_stack_t *cs, chunk_t *pc);
void cs_push2(chunk_stack_t *cs, chunk_t *pc, int seqnum);

chunk_t *cs_pop(chunk_stack_t *cs);
chunk_t *cs_pop2(chunk_stack_t *cs, int *seqnum);

int cs_len(chunk_stack_t *cs);


#endif   /* CHUNK_STACK_H_INCLUDED */

