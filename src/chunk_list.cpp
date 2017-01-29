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

#include "uncrustify_types.h"
#include "ListManager.h"
#include "prototypes.h"
#include "uncrustify.h"
#include "space.h"


/** use this enum to define in what direction or location an
 * operation shall be performed. */
enum loc_t
{
   before, /**< indicates a position or direction upwards (=pref) */
   after   /**< indicates a position or direction downwards (=next) */
};


/***************************************************************************//**
 * @brief prototype for a function that checks a chunk to have a given type
 *
 * @note this typedef defines the function type "check_t"
 * for a function pointer of type
 * bool function(chunk_t *pc)
 ******************************************************************************/
typedef bool (*check_t)(chunk_t *pc);


/***************************************************************************//**
 * @brief prototype for a function that searches through a chunk list
 *
 * @note this typedef defines the function type "search_t"
 * for a function pointer of type
 * chunk_t *function(chunk_t *cur, nav_t nav)
 ******************************************************************************/
typedef chunk_t * (*search_t)(chunk_t *cur, nav_t nav);


typedef ListManager<chunk_t> ChunkList_t;


/* \todo if we use C++ we can overload the following two functions
 * and thus name them equally */

/**
 * \brief search for a chunk that satisfies a condition in a chunk list
 *
 * A generic function that traverses a chunks list either
 * in forward or reverse direction. The traversal continues until a
 * chunk satisfies the condition defined by the compare function.
 * Depending on the parameter cond the condition will either be
 * checked to be true or false.
 *
 * Whenever a chunk list traversal is to be performed this function
 * shall be used. This keeps the code clear and easy to understand.
 *
 * If there are performance issues this function might be worth to
 * be optimized as it is heavily used.
 *
 * @retval NULL    - no requested chunk was found or invalid parameters provided
 * @retval chunk_t - pointer to the found chunk
 */
static chunk_t
*chunk_search(chunk_t       *cur,           /**< [in] chunk to start search at */
              const check_t check_fct,      /**< [in] compare function */
              const nav_t   nav = CNAV_ALL, /**< [in] code parts to consider for search */
              const loc_t   dir = after,    /**< [in] search direction */
              const bool    cond = true     /**< [in] success condition */
              );


/**
 * \brief search a chunk of a given category in a chunk list
 *
 * traverses a chunk list either in forward or backward direction.
 * The traversal continues until a chunk of a given category is found.
 *
 * This function is a specialization of chunk_search.
 *
 * @retval NULL    - no chunk found or invalid parameters provided
 * @retval chunk_t - pointer to the found chunk
 */
static chunk_t
*chunk_search_type(chunk_t         *cur,           /**< [in] chunk to start search at */
                   const c_token_t type,           /**< [in] category to search for */
                   const nav_t     nav = CNAV_ALL, /**< [in] code parts to consider for search */
                   const loc_t     dir = after     /**< [in] search direction */
                   );


chunk_t
*chunk_search_typelevel(chunk_t   *cur,           /**< [in] chunk to start search at */
                        c_token_t type,           /**< [in] category to search for */
                        nav_t     nav = CNAV_ALL, /**< [in] code parts to consider for search */
                        loc_t     dir = after,    /**< [in] search direction */
                        int       level = -1      /**< {in]  */
                        );


/**
 * \brief searches a chunk that is non-NEWLINE, non-comment and non-preprocessor
 *
 * traverses a chunk list either in forward or backward direction.
 * The traversal continues until a chunk of a given category is found.
 *
 * @retval NULL    - no chunk found or invalid parameters provided
 * @retval chunk_t - pointer to the found chunk
 */
static chunk_t
*chunk_get_ncnlnp(chunk_t     *cur,           /**< [in] chunk to start search at */
                  const nav_t nav = CNAV_ALL, /**< [in] code parts to consider for search */
                  const loc_t dir = after     /**< [in] search direction */
                  );


/**
 * \brief tbd
 *
 * @return
 */
chunk_t
*chunk_search_str(chunk_t    *cur,  /**< [in] chunk to start search at */
                  const char *str,  /**< {in]  */
                  nav_t      nav,   /**< [in] code parts to consider for search */
                  loc_t      dir,   /**< [in] search direction */
                  int        level, /**< {in]  */
                  size_t     len    /**< {in]  */
                  );


/**
 * \brief  Add a copy after the given chunk.
 *
 * If ref is NULL, add at the head.
 *
 * @return tbd
 */
static chunk_t
*chunk_add(const chunk_t *pc_in,     /**< {in] chunk to add to list */
           chunk_t       *ref,       /**< [in] insert position in list */
           const loc_t   pos = after /**< [in] insert before or after */
           );


/**
 * \brief Determines which chunk search function to use
 *
 * Depending on the required search direction return a pointer
 * to the corresponding chunk search function.
 *
 * @return pointer to chunk search function
 */
static search_t
select_search_fct(const loc_t dir = after /**< [in] search direction */
                  );


static void chunk_log(chunk_t *pc, const char *text);


ChunkList_t g_cl; /** global chunk list */


static search_t select_search_fct(const loc_t dir)
{
   return((dir == after) ? chunk_get_next : chunk_get_prev);
}


chunk_t *chunk_search_prev_cat(chunk_t *pc, const c_token_t cat)
{
   return(chunk_search_type(pc, cat, CNAV_ALL, before));
}


chunk_t *chunk_search_next_cat(chunk_t *pc, const c_token_t cat)
{
   return(chunk_search_type(pc, cat, CNAV_ALL, after));
}


static chunk_t
*chunk_search_type(chunk_t *cur, const c_token_t type, const nav_t nav, const loc_t dir)
{
   /* Depending on the parameter dir the search function searches
    * in forward or backward direction */
   search_t search_function = select_search_fct(dir);
   chunk_t  *pc             = cur;

   do                                /* loop over the chunk list */
   {
      pc = search_function(pc, nav); /* in either direction while */
   } while ((pc != NULL) &&          /* the end of the list was not reached yet */
            (pc->type != type));     /* and the demanded chunk was not found either */
   return(pc);                       /* the latest chunk is the searched one */
}


inline bool is_expected_type_and_level(chunk_t *pc, c_token_t type, int level)
{
   return(((pc->type == type) &&            /* the type is as expected and */
           ((pc->level == (size_t)level) || /* the level is as expected or */
            (level < 0))));                 /* we don't care about the level */
}


chunk_t *chunk_search_typelevel(chunk_t *cur, c_token_t type, nav_t nav, loc_t dir, int level)
{
   /* Depending on the parameter dir the search function searches
    * in forward or backward direction */
   search_t search_function = select_search_fct(dir);
   chunk_t  *pc             = cur;

   do                                /* loop over the chunk list */
   {
      pc = search_function(pc, nav); /* in either direction while */
#if DEBUG
      if (pc != NULL)
      {
         LOG_FMT(LCHUNK, "%s(%d): pc: %s, type is %s, orig_line=%zu, orig_col=%zu\n",
                 __func__, __LINE__, pc->text(), get_token_name(pc->type), pc->orig_line, pc->orig_col);
      }
#endif
   } while ((pc != NULL) &&          /* the end of the list was not reached yet */
            (is_expected_type_and_level(pc, type, level) == false));
   return(pc);                       /* the latest chunk is the searched one */
}


inline bool is_expected_string_and_level(chunk_t *pc, const char *str, int level, size_t len)
{
   return(((pc->len() == len) &&                  /* the length is as expected and */
           (memcmp(str, pc->text(), len) == 0) && /* the strings equals */
           ((pc->level == (size_t)level) ||       /* the level is as expected or */
            (level < 0))));                       /* we don't care about the level */
}


chunk_t *chunk_search_str(chunk_t *cur, const char *str, nav_t nav, loc_t dir, int level, size_t len)
{
   /* Depending on the parameter dir the search function searches
    * in forward or backward direction */
   search_t search_function = select_search_fct(dir);
   chunk_t  *pc             = cur;

   do                                /* loop over the chunk list */
   {
      pc = search_function(pc, nav); /* in either direction while */
   } while ((pc != NULL) &&          /* the end of the list was not reached yet */
            (is_expected_string_and_level(pc, str, level, len) == false));
   return(pc);                       /* the latest chunk is the searched one */
}


/* \todo the following function shall be made similar to the search functions */
chunk_t *chunk_first_on_line(chunk_t *pc)
{
   chunk_t *first = pc;

   while (((pc = chunk_get_prev(pc)) != NULL) && !chunk_is_newline(pc))
   {
      first = pc;
   }

   return(first);
}


static chunk_t
*chunk_search(chunk_t *cur, const check_t check_fct, const nav_t nav,
              const loc_t dir, const bool cond)
{
   /* Depending on the parameter dir the search function searches
    * in forward or backward direction */
   search_t search_function = select_search_fct(dir);
   chunk_t  *pc             = cur;

   do                                 /* loop over the chunk list */
   {
      pc = search_function(pc, nav);  /* in either direction while */
   } while ((pc != NULL) &&           /* the end of the list was not reached yet */
            (check_fct(pc) != cond)); /* and the demanded chunk was not found either */
   return(pc);                        /* the latest chunk is the searched one */
}


/* \todo maybe it is better to combine chunk_get_next and chunk_get_prev
 * into a common function However this should be done with the preprocessor
 * to avoid addition check conditions that would be evaluated in the
 * while loop of the calling function */
chunk_t *chunk_get_next(chunk_t *cur, nav_t nav)
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


chunk_t *chunk_get_prev(chunk_t *cur, nav_t nav)
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
   /* Not in a preproc, skip any preproc */
   while ((pc != NULL) && (pc->flags & PCF_IN_PREPROC))
   {
      pc = g_cl.GetPrev(pc);
   }
   return(pc);
}


chunk_t *chunk_get_head(void)
{
   return(g_cl.GetHead());
}


chunk_t *chunk_get_tail(void)
{
   return(g_cl.GetTail());
}


chunk_t *chunk_dup(const chunk_t *pc_in)
{
   chunk_t *pc = new chunk_t; /* Allocate a new chunk */

   if (pc == NULL)
   {
      /* \todo print an error message first or better clean up properly */
      exit(EXIT_FAILURE);
   }

   /* Copy all fields and then init the entry */
   *pc = *pc_in;
   g_cl.InitEntry(pc);

   return(pc);
}


static void chunk_log_msg(chunk_t *chunk, const log_sev_t log, const char *str)
{
   LOG_FMT(log, "%s %zu:%zu '%s' [%s]",
           str, chunk->orig_line, chunk->orig_col, chunk->text(),
           get_token_name(chunk->type));
}


static void chunk_log(chunk_t *pc, const char *text)
{
   if ((pc != NULL) && (cpd.unc_stage != US_TOKENIZE) && (cpd.unc_stage != US_CLEANUP))
   {
      const log_sev_t log   = LCHUNK;
      chunk_t         *prev = chunk_get_prev(pc);
      chunk_t         *next = chunk_get_next(pc);

      chunk_log_msg(pc, log, text);

      if (prev && next)
      {
         chunk_log_msg(prev, log, " @ between");
         chunk_log_msg(next, log, " and");
      }
      else if (next)
      {
         chunk_log_msg(next, log, " @ before");
      }
      else if (prev)
      {
         chunk_log_msg(prev, log, " @ after");
      }
      LOG_FMT(log, " stage=%d", cpd.unc_stage);
      log_func_stack_inline(log);
   }
}


static chunk_t *chunk_add(const chunk_t *pc_in, chunk_t *ref, const loc_t pos)
{
   chunk_t *pc = chunk_dup(pc_in);

   if (pc != NULL)
   {
      if (ref != NULL) /* ref is a valid chunk */
      {
         (pos == after) ? g_cl.AddAfter(pc, ref) : g_cl.AddBefore(pc, ref);
      }
      else /* ref == NULL */
      {
         (pos == after) ? g_cl.AddHead(pc) : g_cl.AddTail(pc);
      }
      chunk_log(pc, "chunk_add");
   }
   return(pc); /* \todo what is returned here? */
}


chunk_t *chunk_add_after(const chunk_t *pc_in, chunk_t *ref)
{
   return(chunk_add(pc_in, ref, after));
}


chunk_t *chunk_add_before(const chunk_t *pc_in, chunk_t *ref)
{
   return(chunk_add(pc_in, ref, before));
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


chunk_t *chunk_get_next_nl(chunk_t *cur, nav_t nav)
{
   return(chunk_search(cur, chunk_is_newline, nav, after, true));
}


chunk_t *chunk_get_prev_nl(chunk_t *cur, nav_t nav)
{
   return(chunk_search(cur, chunk_is_newline, nav, before, true));
}


chunk_t *chunk_get_next_nnl(chunk_t *cur, nav_t nav)
{
   return(chunk_search(cur, chunk_is_newline, nav, after, false));
}


chunk_t *chunk_get_prev_nnl(chunk_t *cur, nav_t nav)
{
   return(chunk_search(cur, chunk_is_newline, nav, before, false));
}


chunk_t *chunk_get_next_ncnl(chunk_t *cur, nav_t nav)
{
   return(chunk_search(cur, chunk_is_comment_or_newline, nav, after, false));
}


static chunk_t *chunk_get_ncnlnp(chunk_t *cur, const nav_t nav, const loc_t dir)
{
   chunk_t *pc = cur;

   pc = (chunk_is_preproc(pc) == true) ?
        chunk_search(pc, chunk_is_comment_or_newline_in_preproc, nav, dir, false) :
        chunk_search(pc, chunk_is_comment_newline_or_preproc, nav, dir, false);
   return(pc);
}


chunk_t *chunk_get_next_ncnlnp(chunk_t *cur, nav_t nav)
{
   return(chunk_get_ncnlnp(cur, nav, after));
}


chunk_t *chunk_get_prev_ncnlnp(chunk_t *cur, nav_t nav)
{
   return(chunk_get_ncnlnp(cur, nav, before));
}


chunk_t *chunk_get_next_nblank(chunk_t *cur, nav_t nav)
{
   return(chunk_search(cur, chunk_is_comment_newline_or_blank, nav, after, false));
}


chunk_t *chunk_get_prev_nblank(chunk_t *cur, nav_t nav)
{
   return(chunk_search(cur, chunk_is_comment_newline_or_blank, nav, before, false));
}


chunk_t *chunk_get_next_nc(chunk_t *cur, nav_t nav)
{
   return(chunk_search(cur, chunk_is_comment, nav, after, false));
}


chunk_t *chunk_get_next_nisq(chunk_t *cur, nav_t nav)
{
   return(chunk_search(cur, chunk_is_balanced_square, nav, after, false));
}


chunk_t *chunk_get_prev_ncnl(chunk_t *cur, nav_t nav)
{
   return(chunk_search(cur, chunk_is_comment_or_newline, nav, before, false));
}


chunk_t *chunk_get_prev_nc(chunk_t *cur, nav_t nav)
{
   return(chunk_search(cur, chunk_is_comment, nav, before, false));
}


chunk_t *chunk_get_next_nvb(chunk_t *cur, const nav_t nav)
{
   return(chunk_search(cur, chunk_is_vbrace, nav, after, false));
}


chunk_t *chunk_get_prev_nvb(chunk_t *cur, const nav_t nav)
{
   return(chunk_search(cur, chunk_is_vbrace, nav, before, false));
}


chunk_t
*chunk_get_next_type(chunk_t *cur, c_token_t type, int level, nav_t nav)
{
   return(chunk_search_typelevel(cur, type, nav, after, level));
}


chunk_t
*chunk_get_prev_type(chunk_t *cur, c_token_t type, int level, nav_t nav)
{
   return(chunk_search_typelevel(cur, type, nav, before, level));
}


chunk_t
*chunk_get_next_str(chunk_t *cur, const char *str, size_t len, int level, nav_t nav)
{
   return(chunk_search_str(cur, str, nav, after, level, len));
}


chunk_t
*chunk_get_prev_str(chunk_t *cur, const char *str, size_t len, int level, nav_t nav)
{
   return(chunk_search_str(cur, str, nav, before, level, len));
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


/* \todo this function needs some cleanup */
void chunk_swap_lines(chunk_t *pc1, chunk_t *pc2)
{
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
   chunk_t *ref2 = chunk_get_prev(pc2);

   /* Move the line started at pc2 before pc1 */
   while ((pc2 != NULL) && !chunk_is_newline(pc2))
   {
      chunk_t *tmp = chunk_get_next(pc2);
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
      chunk_t *tmp = chunk_get_next(pc1);
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


void set_chunk_real(chunk_t *pc, c_token_t token, log_sev_t what, const char *str);


void set_chunk_real(chunk_t *pc, c_token_t token, log_sev_t what, const char *str)
{
   LOG_FUNC_ENTRY();

   c_token_t *where;
   c_token_t *type;
   c_token_t *parent_type;

   switch (what)
   {
   case (LSETTYP): where = &pc->type;
      type               = &token;
      parent_type        = &pc->parent_type;
      break;

   case (LSETPAR): where = &pc->parent_type;
      type               = &pc->type;
      parent_type        = &token;
      break;

   default:
      return;
   }

   if ((pc != NULL) && (*where != token))
   {
      LOG_FMT(what, "%s: %zu:%zu '%s' %s:%s => %s:%s",
              str, pc->orig_line, pc->orig_col, pc->text(),
              get_token_name(pc->type), get_token_name(pc->parent_type),
              get_token_name(*type), get_token_name(*parent_type));
      log_func_stack_inline(what);
      *where = token;
   }
}


void set_chunk_type_real(chunk_t *pc, c_token_t tt)
{
   set_chunk_real(pc, tt, LSETTYP, "set_chunk_type");
}


void set_chunk_parent_real(chunk_t *pc, c_token_t pt)
{
   set_chunk_real(pc, pt, LSETPAR, "set_chunk_parent");
}


void chunk_flags_set_real(chunk_t *pc, UINT64 clr_bits, UINT64 set_bits)
{
   if (pc != NULL)
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
