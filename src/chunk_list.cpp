/**
 * @file chunk_list.cpp
 * Manages and navigates the list of chunks.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "chunk_list.h"
#include <cstring>
#include <cstdlib>

#include "ListManager.h"
#include "prototypes.h"

typedef ListManager<chunk_t>   ChunkList;

ChunkList g_cl;

chunk_t *chunk_get_head(void)
{
   return(g_cl.GetHead());
}


chunk_t *chunk_get_tail(void)
{
   return(g_cl.GetTail());
}


chunk_t *chunk_get_next(chunk_t *cur, chunk_nav_t nav)
{
   if (cur == NULL)
   {
      return(NULL);
   }
   chunk_t *pc = g_cl.GetNext(cur);
   if ((pc == NULL) || (nav == CNAV_ALL))
   {
      return(pc);
   }
   if (cur->flags & PCF_IN_PREPROC)
   {
      /* If in a preproc, return NULL if trying to leave */
      if ((pc->flags & PCF_IN_PREPROC) == 0)
      {
         return(NULL);
      }
      return(pc);
   }
   /* Not in a preproc, skip any proproc */
   while ((pc != NULL) && (pc->flags & PCF_IN_PREPROC))
   {
      pc = g_cl.GetNext(pc);
   }
   return(pc);
}


chunk_t *chunk_get_prev(chunk_t *cur, chunk_nav_t nav)
{
   if (cur == NULL)
   {
      return(NULL);
   }
   chunk_t *pc = g_cl.GetPrev(cur);
   if ((pc == NULL) || (nav == CNAV_ALL))
   {
      return(pc);
   }
   if (cur->flags & PCF_IN_PREPROC)
   {
      /* If in a preproc, return NULL if trying to leave */
      if ((pc->flags & PCF_IN_PREPROC) == 0)
      {
         return(NULL);
      }
      return(pc);
   }
   /* Not in a preproc, skip any proproc */
   while ((pc != NULL) && (pc->flags & PCF_IN_PREPROC))
   {
      pc = g_cl.GetPrev(pc);
   }
   return(pc);
}


chunk_t *chunk_dup(const chunk_t *pc_in)
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


/**
 * Add a copy after the given chunk.
 * If ref is NULL, add at the head.
 */
chunk_t *chunk_add_after(const chunk_t *pc_in, chunk_t *ref)
{
   chunk_t *pc;

   if ((pc = chunk_dup(pc_in)) != NULL)
   {
      if (ref != NULL)
      {
         g_cl.AddAfter(pc, ref);
      }
      else
      {
         g_cl.AddHead(pc);
      }
   }
   return(pc);
}


/**
 * Add a copy before the given chunk.
 * If ref is NULL, add at the head.
 */
chunk_t *chunk_add_before(const chunk_t *pc_in, chunk_t *ref)
{
   chunk_t *pc;

   if ((pc = chunk_dup(pc_in)) != NULL)
   {
      if (ref != NULL)
      {
         g_cl.AddBefore(pc, ref);
      }
      else
      {
         g_cl.AddTail(pc);
      }
   }
   return(pc);
}


void chunk_del(chunk_t *pc)
{
   g_cl.Pop(pc);
   //if ((pc->flags & PCF_OWN_STR) && (pc->str != NULL))
   //{
   //   delete[] (char *)pc->str;
   //   pc->str = NULL;
   //}
   delete pc;
}


void chunk_move_after(chunk_t *pc_in, chunk_t *ref)
{
   g_cl.Pop(pc_in);
   g_cl.AddAfter(pc_in, ref);

   /* HACK: Adjust the original column */
   pc_in->column       = ref->column + space_col_align(ref, pc_in);
   pc_in->orig_col     = pc_in->column;
   pc_in->orig_col_end = pc_in->orig_col + pc_in->len();
}


/**
 * Gets the next NEWLINE chunk
 */
chunk_t *chunk_get_next_nl(chunk_t *cur, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_next(pc, nav);
   } while ((pc != NULL) && !chunk_is_newline(pc));
   return(pc);
}


/**
 * Gets the prev NEWLINE chunk
 */
chunk_t *chunk_get_prev_nl(chunk_t *cur, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_prev(pc, nav);
   } while ((pc != NULL) && !chunk_is_newline(pc));
   return(pc);
}


/**
 * Gets the next non-NEWLINE chunk
 */
chunk_t *chunk_get_next_nnl(chunk_t *cur, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_next(pc, nav);
   } while (chunk_is_newline(pc));
   return(pc);
}


/**
 * Gets the prev non-NEWLINE chunk
 */
chunk_t *chunk_get_prev_nnl(chunk_t *cur, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_prev(pc, nav);
   } while ((pc != NULL) && chunk_is_newline(pc));
   return(pc);
}


/**
 * Gets the next non-NEWLINE and non-comment chunk
 */
chunk_t *chunk_get_next_ncnl(chunk_t *cur, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_next(pc, nav);
   } while ((pc != NULL) && (chunk_is_comment(pc) || chunk_is_newline(pc)));
   return(pc);
}


/**
 * Gets the next non-NEWLINE and non-comment chunk, non-preprocessor chunk
 */
chunk_t *chunk_get_next_ncnlnp(chunk_t *cur, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   if (chunk_is_preproc(cur))
   {
      do
      {
         pc = chunk_get_next(pc, nav);
      } while ((pc != NULL) && chunk_is_preproc(pc) &&
               (chunk_is_comment(pc) || chunk_is_newline(pc)));
   }
   else
   {
      do
      {
         pc = chunk_get_next(pc, nav);
      } while ((pc != NULL) && (chunk_is_comment(pc) ||
                                chunk_is_newline(pc) ||
                                chunk_is_preproc(pc)));
   }
   return(pc);
}


/**
 * Gets the prev non-NEWLINE and non-comment chunk, non-preprocessor chunk
 */
chunk_t *chunk_get_prev_ncnlnp(chunk_t *cur, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   if (chunk_is_preproc(cur))
   {
      do
      {
         pc = chunk_get_prev(pc, nav);
      } while ((pc != NULL) && chunk_is_preproc(pc) &&
               (chunk_is_comment(pc) || chunk_is_newline(pc)));
   }
   else
   {
      do
      {
         pc = chunk_get_prev(pc, nav);
      } while ((pc != NULL) && (chunk_is_comment(pc) ||
                                chunk_is_newline(pc) ||
                                chunk_is_preproc(pc)));
   }
   return(pc);
}


/**
 * Gets the next non-blank chunk
 */
chunk_t *chunk_get_next_nblank(chunk_t *cur, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_next(pc, nav);
   } while ((pc != NULL) && (chunk_is_comment(pc) ||
                             chunk_is_newline(pc) ||
                             chunk_is_blank(pc)));
   return(pc);
}


/**
 * Gets the prev non-blank chunk
 */
chunk_t *chunk_get_prev_nblank(chunk_t *cur, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_prev(pc, nav);
   } while ((pc != NULL) && (chunk_is_comment(pc) || chunk_is_newline(pc) ||
                             chunk_is_blank(pc)));
   return(pc);
}


/**
 * Gets the next non-comment chunk
 */
chunk_t *chunk_get_next_nc(chunk_t *cur, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_next(pc, nav);
   } while ((pc != NULL) && chunk_is_comment(pc));
   return(pc);
}


/**
 * Gets the prev non-NEWLINE and non-comment chunk
 */
chunk_t *chunk_get_prev_ncnl(chunk_t *cur, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_prev(pc, nav);
   } while ((pc != NULL) && (chunk_is_comment(pc) || chunk_is_newline(pc)));
   return(pc);
}


/**
 * Gets the prev non-comment chunk
 */
chunk_t *chunk_get_prev_nc(chunk_t *cur, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_prev(pc, nav);
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
                             int level, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_next(pc, nav);
      if ((pc == NULL) ||
          ((pc->type == type) && ((pc->level == level) || (level < 0))))
      {
         break;
      }
   } while (pc != NULL);
   return(pc);
}


chunk_t *chunk_get_next_str(chunk_t *cur, const char *str, int len, int level,
                            chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_next(pc, nav);
      if ((pc == NULL) ||
          ((pc->len() == len) && (memcmp(str, pc->text(), len) == 0) &&
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
                             int level, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_prev(pc, nav);
      if ((pc == NULL) ||
          ((pc->type == type) && ((pc->level == level) || (level < 0))))
      {
         break;
      }
   } while (pc != NULL);
   return(pc);
}


chunk_t *chunk_get_prev_str(chunk_t *cur, const char *str, int len, int level,
                            chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_prev(pc, nav);
      if ((pc == NULL) ||
          ((pc->len() == len) && (memcmp(str, pc->text(), len) == 0) &&
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


/**
 * Finds the first chunk on the line that pc is on.
 * This just backs up until a newline or NULL is hit.
 *
 * given: [ a - b - c - n1 - d - e - n2 ]
 * input: [ a | b | c | n1 ] => a
 * input: [ d | e | n2 ]     => d
 */
chunk_t *chunk_first_on_line(chunk_t *pc)
{
   chunk_t *first = pc;

   while (((pc = chunk_get_prev(pc)) != NULL) && !chunk_is_newline(pc))
   {
      first = pc;
   }

   return(first);
}


/**
 * Swaps two lines that are started with the specified chunks.
 *
 * @param pc1  The first chunk of line 1
 * @param pc2  The first chunk of line 2
 */
void chunk_swap_lines(chunk_t *pc1, chunk_t *pc2)
{
   chunk_t *ref2;
   chunk_t *tmp;

   pc1 = chunk_first_on_line(pc1);
   pc2 = chunk_first_on_line(pc2);

   if ((pc1 == NULL) || (pc2 == NULL) || (pc1 == pc2))
   {
      return;
   }

   /**
    * Example start:
    * ? - start1 - a1 - b1 - nl1 - ? - ref2 - start2 - a2 - b2 - nl2 - ?
    *      ^- pc1                              ^- pc2
    */
   ref2 = chunk_get_prev(pc2);

   /* Move the line started at pc2 before pc1 */
   while ((pc2 != NULL) && !chunk_is_newline(pc2))
   {
      tmp = chunk_get_next(pc2);
      g_cl.Pop(pc2);
      g_cl.AddBefore(pc2, pc1);
      pc2 = tmp;
   }

   /**
    * Should now be:
    * ? - start2 - a2 - b2 - start1 - a1 - b1 - nl1 - ? - ref2 - nl2 - ?
    *                         ^- pc1                              ^- pc2
    */

   /* Now move the line started at pc1 after ref2 */
   while ((pc1 != NULL) && !chunk_is_newline(pc1))
   {
      tmp = chunk_get_next(pc1);
      g_cl.Pop(pc1);
      if (ref2 != NULL)
      {
         g_cl.AddAfter(pc1, ref2);
      }
      else
      {
         g_cl.AddHead(pc1);
      }
      ref2 = pc1;
      pc1  = tmp;
   }

   /**
    * Should now be:
    * ? - start2 - a2 - b2 - nl1 - ? - ref2 - start1 - a1 - b1 - nl2 - ?
    *                         ^- pc1                              ^- pc2
    */

   /* pc1 and pc2 should be the newlines for their lines.
    * swap the chunks and the nl_count so that the spacing remains the same.
    */
   if ((pc1 != NULL) && (pc2 != NULL))
   {
      int nl_count = pc1->nl_count;

      pc1->nl_count = pc2->nl_count;
      pc2->nl_count = nl_count;

      chunk_swap(pc1, pc2);
   }
}


/**
 * Gets the next non-vbrace chunk
 */
chunk_t *chunk_get_next_nvb(chunk_t *cur, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_next(pc, nav);
   } while (chunk_is_vbrace(pc));
   return(pc);
}


/**
 * Gets the prev non-vbrace chunk
 */
chunk_t *chunk_get_prev_nvb(chunk_t *cur, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_prev(pc, nav);
   } while (chunk_is_vbrace(pc));
   return(pc);
}
