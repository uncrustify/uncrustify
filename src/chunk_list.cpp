/**
 * @file chunk_list.cpp
 * Manages and navigates the list of chunks.
 *
 * $Id$
 */

#include "chunk_list.h"
#include <cstring>
#include <cstdlib>

#include "ListManager.h"

typedef ListManager <chunk_t>   ChunkList;

ChunkList g_cl;

static chunk_t *chunk_dup(const chunk_t *pc_in);


chunk_t *chunk_get_head(void)
{
   return(g_cl.GetHead());
}

chunk_t *chunk_get_tail(void)
{
   return(g_cl.GetTail());
}

chunk_t *chunk_get_next(chunk_t *cur)
{
   return(g_cl.GetNext(cur));
}

chunk_t *chunk_get_prev(chunk_t *cur)
{
   return(g_cl.GetPrev(cur));
}

static chunk_t *chunk_dup(const chunk_t *pc_in)
{
   chunk_t *pc;

   /* Allocate the entry */
   pc = new chunk_t;
   if (pc == NULL)
   {
      exit(1);
   }

   /* Copy all fields and then init the entry */
   *pc = *pc_in;
   g_cl.InitEntry(pc);

   return(pc);
}

/**
 * Add to the tail of the list
 */
chunk_t *chunk_add(const chunk_t *pc_in)
{
   chunk_t *pc;

   if ((pc = chunk_dup(pc_in)) != NULL)
   {
      g_cl.AddTail(pc);
   }
   return(pc);
}

chunk_t *chunk_add_after(const chunk_t *pc_in, chunk_t *ref)
{
   chunk_t *pc;

   if ((pc = chunk_dup(pc_in)) != NULL)
   {
      g_cl.AddAfter(pc, ref);
   }
   return(pc);
}

chunk_t *chunk_add_before(const chunk_t *pc_in, chunk_t *ref)
{
   chunk_t *pc;

   if ((pc = chunk_dup(pc_in)) != NULL)
   {
      g_cl.AddBefore(pc, ref);
   }
   return(pc);
}

void chunk_del(chunk_t *pc)
{
   g_cl.Pop(pc);
   delete pc;
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

   if (chunk_is_preproc(cur))
   {
      do
      {
         pc = chunk_get_next(pc);
      } while ((pc != NULL) && chunk_is_preproc(pc) &&
               (chunk_is_comment(pc) || chunk_is_newline(pc)));
   }
   else
   {
      do
      {
         pc = chunk_get_next(pc);
      } while ((pc != NULL) && (chunk_is_comment(pc) ||
                                chunk_is_newline(pc) ||
                                chunk_is_preproc(pc)));
   }
   return(pc);
}

/**
 * Gets the prev non-NEWLINE and non-comment chunk, non-preprocessor chunk
 */
chunk_t *chunk_get_prev_ncnlnp(chunk_t *cur)
{
   chunk_t *pc = cur;

   if (chunk_is_preproc(cur))
   {
      do
      {
         pc = chunk_get_prev(pc);
      } while ((pc != NULL) && chunk_is_preproc(pc) &&
               (chunk_is_comment(pc) || chunk_is_newline(pc)));
   }
   else
   {
      do
      {
         pc = chunk_get_prev(pc);
      } while ((pc != NULL) && (chunk_is_comment(pc) ||
                                chunk_is_newline(pc) ||
                                chunk_is_preproc(pc)));
   }
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

chunk_t *chunk_get_next_str(chunk_t *cur, const char *str, int len, int level)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_next(pc);
      if ((pc == NULL) ||
          ((pc->len == len) && (memcmp(str, pc->str, len) == 0) &&
           ((pc->level == level) || (level < 0))))
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

chunk_t *chunk_get_prev_str(chunk_t *cur, const char *str, int len, int level)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_prev(pc);
      if ((pc == NULL) ||
          ((pc->len == len) && (memcmp(str, pc->str, len) == 0) &&
           ((pc->level == level) || (level < 0))))
      {
         break;
      }
   } while (pc != NULL);
   return(pc);
}

/**
 * Check to see if there is a newline bewteen the two chunks
 */
bool chunk_is_newline_between(chunk_t *start, chunk_t *end)
{
   chunk_t *pc;

   for (pc = start; pc != end; pc = chunk_get_next(pc))
   {
      if (chunk_is_newline(pc))
      {
         return(true);
      }
   }
   return(false);
}


/**
 * Swaps the two chunks.
 *
 * @param pc1  The first chunk
 * @param pc2  The second chunk
 */
void chunk_swap(chunk_t *pc1, chunk_t *pc2)
{
   g_cl.Swap(pc1, pc2);
}
