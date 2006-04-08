/**
 * @file chunk_stack.c
 * Manages a chunk stack
 *
 * $Id$
 */

#include "chunk_stack.h"
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
#include <stdlib.h>
#endif


chunk_stack_t *cs_new(void)
{
   return(calloc(1, sizeof(chunk_stack_t)));
}

void cs_delete(chunk_stack_t *cs)
{
   if (cs->pc != NULL)
   {
      free(cs->pc);
      memset(cs, 0, sizeof(*cs));
   }
   free(cs);
}

void cs_reset(chunk_stack_t *cs)
{
   cs->len = 0;
}

void cs_push(chunk_stack_t *cs, chunk_t *pc)
{
   chunk_t **tmp;

   if (cs->len >= cs->size)
   {
      /* double the size */
      if (cs->pc != NULL)
      {
         tmp = realloc(cs->pc, (cs->size + 256) * sizeof(chunk_t *));
      }
      else
      {
         tmp = malloc(256 * sizeof(chunk_t *));
      }
      if (tmp != NULL)
      {
         cs->pc    = tmp;
         cs->size += 256;
      }
   }

   if (cs->len < cs->size)
   {
      cs->pc[cs->len++] = pc;
   }
}

chunk_t *cs_pop(chunk_stack_t *cs)
{
   if (cs->len > 0)
   {
      cs->len--;
      return(cs->pc[cs->len]);
   }
   return(NULL);
}

int cs_len(chunk_stack_t *cs)
{
   return(cs->len);
}

