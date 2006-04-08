/**
 * @file ChunkList.d
 * Manages and navigates the list of chunks.
 *
 * $Id: chunk_list.c 13 2006-02-24 03:21:49Z bengardner $
 */

#include "chunk_list.h"
#include <string.h>
#include <stdlib.h>


/**
 * Create some dummy entries at the head and tail.
 * This makes is so the ops below never hit the end.
 */
void chunk_list_init(void)
{
   cpd.list_chunks.str  = "-= LIST =-";
   cpd.list_chunks.len  = 10;
   cpd.list_chunks.next = &cpd.list_chunks;
   cpd.list_chunks.prev = &cpd.list_chunks;
}

static chunk_t *chunk_dup(const chunk_t *pc_in)
{
   chunk_t *pc;
   char    *text;

   /* allocate some memory - 1 extra char for labels */
   pc = malloc(sizeof(chunk_t) + pc_in->len + 2);
   if (pc == NULL)
   {
      exit(1);
   }

   /* Text goes after the structure */
   text = (char *)&pc[1];

   /* Set all fields */
   *pc      = *pc_in;
   pc->prev = pc->next = NULL;
   pc->str  = text;

   /* Fix the str pointer & copy the string */
   memcpy(text, pc_in->str, pc_in->len);
   text[pc->len] = 0;

   return(pc);
}

/**
 * Add to the tail of the list
 */
chunk_t *chunk_add(const chunk_t *pc_in)
{
   return(chunk_add_after(pc_in, cpd.list_chunks.prev));
}

chunk_t *chunk_add_after(const chunk_t *pc_in,
                         chunk_t *ref)
{
   chunk_t *pc;

   if ((pc = chunk_dup(pc_in)) != NULL)
   {
      pc->next       = ref->next;
      ref->next      = pc;
      pc->prev       = ref;
      pc->next->prev = pc;
   }
   return(pc);
}

chunk_t *chunk_add_before(const chunk_t *pc_in,
                          chunk_t *ref)
{
   chunk_t *pc;

   if ((pc = chunk_dup(pc_in)) != NULL)
   {
      pc->prev       = ref->prev;
      ref->prev      = pc;
      pc->next       = ref;
      pc->prev->next = pc;
   }
   return(pc);
}

void chunk_del(chunk_t *pc)
{
   pc->next->prev = pc->prev;
   pc->prev->next = pc->next;
   free(pc);
}

/**
 * Gets the next NEWLINE chunk
 */
chunk_t *chunk_get_next_nl(chunk_t *cur)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_next(pc);
   } while ((pc != NULL) && !chunk_is_newline(pc));
   return(pc);
}

/**
 * Gets the prev NEWLINE chunk
 */
chunk_t *chunk_get_prev_nl(chunk_t *cur)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_prev(pc);
   } while ((pc != NULL) && !chunk_is_newline(pc));
   return(pc);
}

/**
 * Gets the next non-NEWLINE chunk
 */
chunk_t *chunk_get_next_nnl(chunk_t *cur)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_next(pc);
   } while (chunk_is_newline(pc));
   return(pc);
}

/**
 * Gets the prev non-NEWLINE chunk
 */
chunk_t *chunk_get_prev_nnl(chunk_t *cur)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_prev(pc);
   } while ((pc != NULL) && chunk_is_newline(pc));
   return(pc);
}

/**
 * Gets the next non-NEWLINE and non-comment chunk
 */
chunk_t *chunk_get_next_ncnl(chunk_t *cur)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_next(pc);
   } while ((pc != NULL) && (chunk_is_comment(pc) || chunk_is_newline(pc)));
   return(pc);
}

/**
 * Gets the next non-NEWLINE and non-comment chunk, non-preprocessor chunk
 */
chunk_t *chunk_get_next_ncnlnp(chunk_t *cur)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_next(pc);
   } while ((pc != NULL) && (chunk_is_comment(pc) ||
                             chunk_is_newline(pc) ||
                             chunk_is_preproc(pc)));
   return(pc);
}

/**
 * Gets the prev non-NEWLINE and non-comment chunk, non-preprocessor chunk
 */
chunk_t *chunk_get_prev_ncnlnp(chunk_t *cur)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_prev(pc);
   } while ((pc != NULL) && (chunk_is_comment(pc) ||
                             chunk_is_newline(pc) ||
                             chunk_is_preproc(pc)));
   return(pc);
}

/**
 * Gets the next non-blank chunk
 */
chunk_t *chunk_get_next_nblank(chunk_t *cur)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_next(pc);
   } while ((pc != NULL) && (chunk_is_comment(pc) ||
                             chunk_is_newline(pc) ||
                             chunk_is_blank(pc)));
   return(pc);
}
/**
 * Gets the prev non-blank chunk
 */
chunk_t *chunk_get_prev_nblank(chunk_t *cur)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_prev(pc);
   } while ((pc != NULL) && (chunk_is_comment(pc) || chunk_is_newline(pc) ||
                             chunk_is_blank(pc)));
   return(pc);
}

/**
 * Gets the next non-comment chunk
 */
chunk_t *chunk_get_next_nc(chunk_t *cur)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_next(pc);
   } while ((pc != NULL) && chunk_is_comment(pc));
   return(pc);
}

/**
 * Gets the prev non-NEWLINE and non-comment chunk
 */
chunk_t *chunk_get_prev_ncnl(chunk_t *cur)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_prev(pc);
   } while ((pc != NULL) && (chunk_is_comment(pc) || chunk_is_newline(pc)));
   return(pc);
}

/**
 * Gets the prev non-comment chunk
 */
chunk_t *chunk_get_prev_nc(chunk_t *cur)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_prev(pc);
   } while ((pc != NULL) && chunk_is_comment(pc));
   return(pc);
}

/**
 * Grabs the next chunk of the given type at the level.
 *
 * @param cur     Starting chunk
 * @param type    The type to look for
 * @param level   -1 (any level) or the level to match
 * @return        NULL or the match
 */
chunk_t *chunk_get_next_type(chunk_t *cur, c_token_t type,
                             int level)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_next(pc);
      if ((pc == NULL) ||
          ((pc->type == type) && ((pc->level == level) || (level < 0))))
      {
         break;
      }
   } while (pc != NULL);
   return(pc);
}

/**
 * Grabs the prev chunk of the given type at the level.
 *
 * @param cur     Starting chunk
 * @param type    The type to look for
 * @param level   -1 (any level) or the level to match
 * @return        NULL or the match
 */
chunk_t *chunk_get_prev_type(chunk_t *cur, c_token_t type,
                             int level)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_prev(pc);
      if ((pc == NULL) ||
          ((pc->type == type) && ((pc->level == level) || (level < 0))))
      {
         break;
      }
   } while (pc != NULL);
   return(pc);
}

chunk_t *chunk_get_head(void)
{
   if (cpd.list_chunks.next != &cpd.list_chunks)
   {
      return(cpd.list_chunks.next);
   }
   return(NULL);
}

chunk_t *chunk_get_tail(void)
{
   if (cpd.list_chunks.prev != &cpd.list_chunks)
   {
      return(cpd.list_chunks.prev);
   }
   return(NULL);
}

chunk_t *chunk_get_next(chunk_t *cur)
{
   if ((cur != NULL) && (cur->next != &cpd.list_chunks))
   {
      return(cur->next);
   }
   return(NULL);
}

chunk_t *chunk_get_prev(chunk_t *cur)
{
   if ((cur != NULL) && (cur->prev != &cpd.list_chunks))
   {
      return(cur->prev);
   }
   return(NULL);
}

/**
 * Check to see if there is a newline bewteen the two chunks
 */
BOOL chunk_is_newline_between(chunk_t *start, chunk_t *end)
{
   chunk_t *pc;

   for (pc = start; pc != end; pc = chunk_get_next(pc))
   {
      if (chunk_is_newline(pc))
      {
         return(TRUE);
      }
   }
   return(FALSE);
}

