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
#include "uncrustify.h"
#include "space.h"


typedef ListManager<chunk_t> ChunkList;


/**
 * \brief search a chunk of a given category in a chunk list
 *
 * follows a chunk list until it finds a chunk of a given category.
 * The search direction is determined by parameter dir.
 *
 * @retval NULL    - no object found, or invalid parameters provided
 * @retval chunk_t - pointer to the found object
 */
static chunk_t *search_chunk(chunk_t         *pc, /**< [in] chunk list to search in */
                             const c_token_t cat, /**< [in] category to search for */
                             const bool      dir  /**< [in] search forward=true, backward=false */
                             );


static void chunk_log(chunk_t *pc, const char *text);


ChunkList g_cl;


chunk_t *chunk_get_head(void)
{
   return(g_cl.GetHead());
}


chunk_t *chunk_get_tail(void)
{
   return(g_cl.GetTail());
}


chunk_t *search_prev_chunk(chunk_t *pc, const c_token_t cat)
{
   return(search_chunk(pc, cat, false));
}


chunk_t *search_next_chunk(chunk_t *pc, const c_token_t cat)
{
   return(search_chunk(pc, cat, true));
}


static chunk_t *search_chunk(chunk_t *pc, const c_token_t cat, const bool dir)
{
   /* no need to check if pc != NULL as this is done in chunk_get_next/prev */
   while ((pc = (dir == true) ? chunk_get_next(pc) : chunk_get_prev(pc)) != NULL)
   {
      if (pc->type == cat)
      {
         return(pc);
      }
   }
   return(NULL);
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
   /* Not in a preproc, skip any preproc */
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
      exit(EXIT_FAILURE);
   }

   /* Copy all fields and then init the entry */
   *pc = *pc_in;
   g_cl.InitEntry(pc);

   return(pc);
}


static void chunk_log(chunk_t *pc, const char *text)
{
   if (pc && (cpd.unc_stage != US_TOKENIZE) && (cpd.unc_stage != US_CLEANUP))
   {
      chunk_t *prev = chunk_get_prev(pc);
      chunk_t *next = chunk_get_next(pc);

      LOG_FMT(LCHUNK, " -- %s: %zu:%zu '%s' [%s]",
              text, pc->orig_line, pc->orig_col, pc->text(),
              get_token_name(pc->type));

      if (prev && next)
      {
         LOG_FMT(LCHUNK, " @ between %zu:%zu '%s' [%s] and %zu:%zu '%s' [%s]",
                 prev->orig_line, prev->orig_col, prev->text(),
                 get_token_name(prev->type),
                 next->orig_line, next->orig_col, next->text(),
                 get_token_name(next->type));
      }
      else if (next)
      {
         LOG_FMT(LCHUNK, " @ before %zu:%zu '%s' [%s]",
                 next->orig_line, next->orig_col, next->text(),
                 get_token_name(next->type));
      }
      else if (prev)
      {
         LOG_FMT(LCHUNK, " @ after %zu:%zu '%s' [%s]",
                 prev->orig_line, prev->orig_col, prev->text(),
                 get_token_name(prev->type));
      }
      LOG_FMT(LCHUNK, " stage=%d", cpd.unc_stage);
      log_func_stack_inline(LCHUNK);
   }
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
      chunk_log(pc, "chunk_add");
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
      chunk_log(pc, "chunk_add");
   }
   return(pc);
}


void chunk_del(chunk_t *pc)
{
   chunk_log(pc, "chunk_del");
   g_cl.Pop(pc);
   delete pc;
}


void chunk_move_after(chunk_t *pc_in, chunk_t *ref)
{
   LOG_FUNC_ENTRY();
   g_cl.Pop(pc_in);
   g_cl.AddAfter(pc_in, ref);

   /* HACK: Adjust the original column */
   pc_in->column       = ref->column + space_col_align(ref, pc_in);
   pc_in->orig_col     = (UINT32)pc_in->column;
   pc_in->orig_col_end = pc_in->orig_col + pc_in->len();
}


chunk_t *chunk_get_next_nl(chunk_t *cur, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_next(pc, nav);
   } while ((pc != NULL) && !chunk_is_newline(pc));
   return(pc);
}


chunk_t *chunk_get_prev_nl(chunk_t *cur, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_prev(pc, nav);
   } while ((pc != NULL) && !chunk_is_newline(pc));
   return(pc);
}


chunk_t *chunk_get_next_nnl(chunk_t *cur, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_next(pc, nav);
   } while (chunk_is_newline(pc));
   return(pc);
}


chunk_t *chunk_get_prev_nnl(chunk_t *cur, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_prev(pc, nav);
   } while ((pc != NULL) && chunk_is_newline(pc));
   return(pc);
}


chunk_t *chunk_get_next_ncnl(chunk_t *cur, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_next(pc, nav);
   } while ((pc != NULL) && (chunk_is_comment(pc) || chunk_is_newline(pc)));
   return(pc);
}


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


chunk_t *chunk_get_next_nisq(chunk_t *cur, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_next(pc, nav);
   } while (pc && ((pc->type == CT_SQUARE_OPEN) ||
                   (pc->type == CT_TSQUARE) ||
                   (pc->type == CT_SQUARE_CLOSE)));
   return(pc);
}


chunk_t *chunk_get_prev_ncnl(chunk_t *cur, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_prev(pc, nav);
   } while ((pc != NULL) && (chunk_is_comment(pc) || chunk_is_newline(pc)));
   return(pc);
}


chunk_t *chunk_get_prev_nc(chunk_t *cur, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_prev(pc, nav);
   } while ((pc != NULL) && chunk_is_comment(pc));
   return(pc);
}


chunk_t *chunk_get_next_type(chunk_t *cur, c_token_t type,
                             int level, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_next(pc, nav);
      if ((pc == NULL) ||
          ((pc->type == type) &&
           ((pc->level == (size_t)level) || (level < 0))))
      //                                     ANY_LEVEL
      {
         break;
      }
   } while (pc != NULL);
   return(pc);
}


chunk_t *chunk_get_next_str(chunk_t *cur, const char *str, size_t len, int level,
                            chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_next(pc, nav);
      if ((pc == NULL) ||
          ((pc->len() == len) && (memcmp(str, pc->text(), len) == 0) &&
           ((pc->level == (size_t)level) || (level < 0))))
      //                                     ANY_LEVEL
      {
         break;
      }
   } while (pc != NULL);
   return(pc);
}


chunk_t *chunk_get_prev_type(chunk_t *cur, c_token_t type,
                             int level, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_prev(pc, nav);
      if (pc != NULL)
      {
         LOG_FMT(LCHUNK, "%s(%d): pc: %s, type is %s, orig_line=%zu, orig_col=%zu\n",
                 __func__, __LINE__, pc->text(), get_token_name(pc->type), pc->orig_line, pc->orig_col);
      }
      if ((pc == NULL) ||
          ((pc->type == type) &&
           ((pc->level == (size_t)level) || (level < 0))))
      //                                     ANY_LEVEL
      {
         break;
      }
   } while (pc != NULL);
   return(pc);
}


chunk_t *chunk_get_prev_str(chunk_t *cur, const char *str, size_t len, int level,
                            chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_prev(pc, nav);
      if ((pc == NULL) ||
          ((pc->len() == len) && (memcmp(str, pc->text(), len) == 0) &&
           ((pc->level == (size_t)level) || (level < 0))))
      //                                     ANY_LEVEL
      {
         break;
      }
   } while (pc != NULL);
   return(pc);
}


bool chunk_is_newline_between(chunk_t *start, chunk_t *end)
{
   for (chunk_t *pc = start; pc != end; pc = chunk_get_next(pc))
   {
      if (chunk_is_newline(pc))
      {
         return(true);
      }
   }
   return(false);
}


void chunk_swap(chunk_t *pc1, chunk_t *pc2)
{
   g_cl.Swap(pc1, pc2);
}


chunk_t *chunk_first_on_line(chunk_t *pc)
{
   chunk_t *first = pc;

   while (((pc = chunk_get_prev(pc)) != NULL) && !chunk_is_newline(pc))
   {
      first = pc;
   }

   return(first);
}


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
      size_t nl_count = pc1->nl_count;

      pc1->nl_count = pc2->nl_count;
      pc2->nl_count = nl_count;

      chunk_swap(pc1, pc2);
   }
} // chunk_swap_lines


chunk_t *chunk_get_next_nvb(chunk_t *cur, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_next(pc, nav);
   } while (chunk_is_vbrace(pc));
   return(pc);
}


chunk_t *chunk_get_prev_nvb(chunk_t *cur, chunk_nav_t nav)
{
   chunk_t *pc = cur;

   do
   {
      pc = chunk_get_prev(pc, nav);
   } while (chunk_is_vbrace(pc));
   return(pc);
}


void set_chunk_type_real(chunk_t *pc, c_token_t tt)
{
   LOG_FUNC_ENTRY();
   if (pc && (pc->type != tt))
   {
      LOG_FMT(LSETTYP, "set_chunk_type: %zu:%zu '%s' %s:%s => %s:%s",
              pc->orig_line, pc->orig_col, pc->text(),
              get_token_name(pc->type), get_token_name(pc->parent_type),
              get_token_name(tt), get_token_name(pc->parent_type));
      log_func_stack_inline(LSETTYP);
      pc->type = tt;
   }
}


void set_chunk_parent_real(chunk_t *pc, c_token_t pt)
{
   LOG_FUNC_ENTRY();
   if (pc && (pc->parent_type != pt))
   {
      LOG_FMT(LSETPAR, "set_chunk_parent: %zu:%zu '%s' %s:%s => %s:%s",
              pc->orig_line, pc->orig_col, pc->text(),
              get_token_name(pc->type), get_token_name(pc->parent_type),
              get_token_name(pc->type), get_token_name(pt));
      log_func_stack_inline(LSETPAR);
      pc->parent_type = pt;
   }
}


void chunk_flags_set_real(chunk_t *pc, UINT64 clr_bits, UINT64 set_bits)
{
   if (pc)
   {
      LOG_FUNC_ENTRY();
      UINT64 nflags = (pc->flags & ~clr_bits) | set_bits;
      if (pc->flags != nflags)
      {
         LOG_FMT(LSETFLG, "set_chunk_flags: %016" PRIx64 "^%016" PRIx64 "=%016" PRIx64 " %zu:%zu '%s' %s:%s",
                 pc->flags, pc->flags ^ nflags, nflags,
                 pc->orig_line, pc->orig_col, pc->text(),
                 get_token_name(pc->type), get_token_name(pc->parent_type));
         log_func_stack_inline(LSETFLG);
         pc->flags = nflags;
      }
   }
}
