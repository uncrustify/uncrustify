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


typedef ListManager<chunk_t> ChunkList_t;


/**
 * use this enum to define in what direction or location an
 * operation shall be performed.
 */
enum class direction_e : unsigned int
{
   FORWARD,
   BACKWARD
};


/**
 * @brief prototype for a function that checks a chunk to have a given type
 *
 * @note this typedef defines the function type "check_t"
 * for a function pointer of type
 * bool function(chunk_t *pc)
 */
typedef bool (*check_t)(chunk_t *pc);


/**
 * @brief prototype for a function that searches through a chunk list
 *
 * @note this typedef defines the function type "search_t"
 * for a function pointer of type
 * chunk_t *function(chunk_t *cur, nav_t scope)
 */
typedef chunk_t * (*search_t)(chunk_t *cur, scope_e scope);


/**
 * @brief search for a chunk that satisfies a condition in a chunk list
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
 * @param  cur        chunk to start search at
 * @param  check_fct  compare function
 * @param  scope      code parts to consider for search
 * @param  dir        search direction
 * @param  cond       success condition
 *
 * @retval nullptr  no requested chunk was found or invalid parameters provided
 * @retval chunk_t  pointer to the found chunk
 */
static chunk_t *chunk_search(chunk_t *cur, const check_t check_fct, const scope_e scope = scope_e::ALL, const direction_e dir = direction_e::FORWARD, const bool cond = true);


static void chunk_log(chunk_t *pc, const char *text);


/*
 * TODO: if we use C++ we can overload the following two functions
 * and thus name them equally
 */

/**
 * @brief search a chunk of a given category in a chunk list
 *
 * traverses a chunk list either in forward or backward direction.
 * The traversal continues until a chunk of a given category is found.
 *
 * This function is a specialization of chunk_search.
 *
 * @param cur    chunk to start search at
 * @param type   category to search for
 * @param scope  code parts to consider for search
 * @param dir    search direction
 *
 * @retval nullptr  no chunk found or invalid parameters provided
 * @retval chunk_t  pointer to the found chunk
 */
static chunk_t *chunk_search_type(chunk_t *cur, const c_token_t type, const scope_e scope = scope_e::ALL, const direction_e dir = direction_e::FORWARD);


/**
 * @brief search a chunk of a given type and level
 *
 * Traverses a chunk list in the specified direction until a chunk of a given type
 * is found.
 *
 * This function is a specialization of chunk_search.
 *
 * @param cur    chunk to start search at
 * @param type   category to search for
 * @param scope  code parts to consider for search
 * @param dir    search direction
 * @param level  nesting level to match or -1 / ANY_LEVEL
 *
 * @retval nullptr  no chunk found or invalid parameters provided
 * @retval chunk_t  pointer to the found chunk
 */
static chunk_t *chunk_search_typelevel(chunk_t *cur, c_token_t type, scope_e scope = scope_e::ALL, direction_e dir = direction_e::FORWARD, int level = -1);


/**
 * @brief searches a chunk that is non-NEWLINE, non-comment and non-preprocessor
 *
 * Traverses a chunk list either in forward or backward direction.
 * The traversal continues until a chunk of a given category is found.
 *
 * @param cur    chunk to start search at
 * @param scope  code parts to consider for search
 * @param dir    search direction
 *
 * @retval nullptr  no chunk found or invalid parameters provided
 * @retval chunk_t  pointer to the found chunk
 */
static chunk_t *chunk_get_ncnlnp(chunk_t *cur, const scope_e scope = scope_e::ALL, const direction_e dir = direction_e::FORWARD);


static chunk_t *chunk_get_ncnlnpnd(chunk_t *cur, const scope_e scope = scope_e::ALL, const direction_e dir = direction_e::FORWARD);


/**
 * @brief searches a chunk that holds a specific string
 *
 * Traverses a chunk list either in forward or backward direction until a chunk
 * with the provided string was found. Additionally a nesting level can be
 * provided to narrow down the search.
 *
 * @param  cur    chunk to start search at
 * @param  str    string that searched chunk needs to have
 * @param  len    length of the string
 * @param  scope  code parts to consider for search
 * @param  dir    search direction
 * @param  level  nesting level of the searched chunk, ignored when negative
 *
 * @retval NULL     no chunk found or invalid parameters provided
 * @retval chunk_t  pointer to the found chunk
 */
static chunk_t *chunk_search_str(chunk_t *cur, const char *str, size_t len, scope_e scope, direction_e dir, int level);


/**
 * @brief Add a new chunk before/after the given position in a chunk list
 *
 * If ref is nullptr, add either at the head or tail based on the specified pos
 *
 * @param  pc_in  chunk to add to list
 * @param  ref    insert position in list
 * @param  pos    insert before or after
 *
 * @return chunk_t  pointer to the added chunk
 */
static chunk_t *chunk_add(const chunk_t *pc_in, chunk_t *ref, const direction_e pos = direction_e::FORWARD);


/**
 * @brief Determines which chunk search function to use
 *
 * Depending on the required search direction return a pointer
 * to the corresponding chunk search function.
 *
 * @param dir  search direction
 *
 * @return pointer to chunk search function
 */
static search_t select_search_fct(const direction_e dir = direction_e::FORWARD);


ChunkList_t g_cl; //! global chunk list


chunk_t *chunk_get_head(void)
{
   return(g_cl.GetHead());
}


chunk_t *chunk_get_tail(void)
{
   return(g_cl.GetTail());
}


static search_t select_search_fct(const direction_e dir)
{
   return((dir == direction_e::FORWARD) ? chunk_get_next : chunk_get_prev);
}


chunk_t *chunk_search_prev_cat(chunk_t *pc, const c_token_t cat)
{
   return(chunk_search_type(pc, cat, scope_e::ALL, direction_e::BACKWARD));
}


chunk_t *chunk_search_next_cat(chunk_t *pc, const c_token_t cat)
{
   return(chunk_search_type(pc, cat, scope_e::ALL, direction_e::FORWARD));
}


static chunk_t *chunk_search_type(chunk_t *cur, const c_token_t type,
                                  const scope_e scope, const direction_e dir)
{
   /*
    * Depending on the parameter dir the search function searches
    * in forward or backward direction
    */
   search_t search_function = select_search_fct(dir);
   chunk_t  *pc             = cur;

   do                                  // loop over the chunk list
   {
      pc = search_function(pc, scope); // in either direction while
   } while (  pc != nullptr            // the end of the list was not reached yet
           && pc->type != type);       // and the demanded chunk was not found either
   return(pc);                         // the latest chunk is the searched one
}


static chunk_t *chunk_search_typelevel(chunk_t *cur, c_token_t type, scope_e scope, direction_e dir, int level)
{
   /*
    * Depending on the parameter dir the search function searches
    * in forward or backward direction
    */
   search_t search_function = select_search_fct(dir);
   chunk_t  *pc             = cur;

   do                                  // loop over the chunk list
   {
      pc = search_function(pc, scope); // in either direction while
#if DEBUG
      if (pc != nullptr)
      {
         if (pc->type == CT_NEWLINE)
         {
            LOG_FMT(LCHUNK, "%s(%d): orig_line is %zu, orig_col is %zu, NEWLINE\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col);
         }
         else
         {
            LOG_FMT(LCHUNK, "%s(%d): orig_line is %zu, orig_col is %zu, pc->text() '%s', type is %s\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
         }
      }
#endif
   } while (  pc != nullptr        // the end of the list was not reached yet
           && (is_expected_type_and_level(pc, type, level) == false));
   return(pc);                     // the latest chunk is the searched one
}


static chunk_t *chunk_search_str(chunk_t *cur, const char *str, size_t len, scope_e scope, direction_e dir, int level)
{
   /*
    * Depending on the parameter dir the search function searches
    * in forward or backward direction */
   search_t search_function = select_search_fct(dir);
   chunk_t  *pc             = cur;

   do                                  // loop over the chunk list
   {
      pc = search_function(pc, scope); // in either direction while
   } while (  pc != nullptr            // the end of the list was not reached yet
           && (is_expected_string_and_level(pc, str, level, len) == false));
   return(pc);                         // the latest chunk is the searched one
}


static chunk_t *chunk_search(chunk_t *cur, const check_t check_fct, const scope_e scope,
                             const direction_e dir, const bool cond)
{
   /*
    * Depending on the parameter dir the search function searches
    * in forward or backward direction */
   search_t search_function = select_search_fct(dir);
   chunk_t  *pc             = cur;

   do                                   // loop over the chunk list
   {
      pc = search_function(pc, scope);  // in either direction while
   } while (  pc != nullptr             // the end of the list was not reached yet
           && (check_fct(pc) != cond)); // and the demanded chunk was not found either
   return(pc);                          // the latest chunk is the searched one
}


/* @todo maybe it is better to combine chunk_get_next and chunk_get_prev
 * into a common function However this should be done with the preprocessor
 * to avoid addition check conditions that would be evaluated in the
 * while loop of the calling function */
chunk_t *chunk_get_next(chunk_t *cur, scope_e scope)
{
   if (cur == nullptr)
   {
      return(nullptr);
   }
   chunk_t *pc = g_cl.GetNext(cur);
   if (pc == nullptr || scope == scope_e::ALL)
   {
      return(pc);
   }
   if (cur->flags & PCF_IN_PREPROC)
   {
      // If in a preproc, return nullptr if trying to leave
      if ((pc->flags & PCF_IN_PREPROC) == 0)
      {
         return(nullptr);
      }
      return(pc);
   }
   // Not in a preproc, skip any preproc
   while (pc != nullptr && (pc->flags & PCF_IN_PREPROC))
   {
      pc = g_cl.GetNext(pc);
   }
   return(pc);
}


chunk_t *chunk_get_prev(chunk_t *cur, scope_e scope)
{
   if (cur == nullptr)
   {
      return(nullptr);
   }
   chunk_t *pc = g_cl.GetPrev(cur);
   if (pc == nullptr || scope == scope_e::ALL)
   {
      return(pc);
   }
   if (cur->flags & PCF_IN_PREPROC)
   {
      // If in a preproc, return NULL if trying to leave
      if ((pc->flags & PCF_IN_PREPROC) == 0)
      {
         return(nullptr);
      }
      return(pc);
   }
   // Not in a preproc, skip any preproc
   while (pc != nullptr && (pc->flags & PCF_IN_PREPROC))
   {
      pc = g_cl.GetPrev(pc);
   }
   return(pc);
}


chunk_t *chunk_dup(const chunk_t *pc_in)
{
   chunk_t *pc = new chunk_t; // Allocate a new chunk

   if (pc == nullptr)
   {
      // @todo clean up properly before crashing
      LOG_FMT(LERR, "Failed to allocate memory\n");
      exit(EXIT_FAILURE);
   }

   // Copy all fields and then init the entry
   *pc = *pc_in; // TODO: what happens if pc_in == nullptr?
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
   if (  pc != nullptr
      && (cpd.unc_stage != unc_stage_e::TOKENIZE)
      && (cpd.unc_stage != unc_stage_e::CLEANUP))
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
      LOG_FMT(log, " stage=%d", (int)cpd.unc_stage);
      log_func_stack_inline(log);
   }
}


chunk_t *chunk_add_after(const chunk_t *pc_in, chunk_t *ref)
{
   return(chunk_add(pc_in, ref, direction_e::FORWARD));
}


chunk_t *chunk_add_before(const chunk_t *pc_in, chunk_t *ref)
{
   return(chunk_add(pc_in, ref, direction_e::BACKWARD));
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

   // HACK: Adjust the original column
   pc_in->column       = ref->column + space_col_align(ref, pc_in);
   pc_in->orig_col     = static_cast<UINT32>(pc_in->column);
   pc_in->orig_col_end = pc_in->orig_col + pc_in->len();
}


chunk_t *chunk_get_next_nl(chunk_t *cur, scope_e scope)
{
   return(chunk_search(cur, chunk_is_newline, scope, direction_e::FORWARD, true));
}


chunk_t *chunk_get_prev_nl(chunk_t *cur, scope_e scope)
{
   return(chunk_search(cur, chunk_is_newline, scope, direction_e::BACKWARD, true));
}


chunk_t *chunk_get_next_nnl(chunk_t *cur, scope_e scope)
{
   return(chunk_search(cur, chunk_is_newline, scope, direction_e::FORWARD, false));
}


chunk_t *chunk_get_prev_nnl(chunk_t *cur, scope_e scope)
{
   return(chunk_search(cur, chunk_is_newline, scope, direction_e::BACKWARD, false));
}


chunk_t *chunk_get_next_ncnl(chunk_t *cur, scope_e scope)
{
   return(chunk_search(cur, chunk_is_comment_or_newline, scope, direction_e::FORWARD, false));
}


chunk_t *chunk_get_next_ncnlnp(chunk_t *cur, scope_e scope)
{
   return(chunk_get_ncnlnp(cur, scope, direction_e::FORWARD));
}


chunk_t *chunk_get_prev_ncnlnp(chunk_t *cur, scope_e scope)
{
   return(chunk_get_ncnlnp(cur, scope, direction_e::BACKWARD));
}


chunk_t *chunk_get_prev_ncnlnpnd(chunk_t *cur, scope_e scope)
{
   return(chunk_get_ncnlnpnd(cur, scope, direction_e::BACKWARD));
}


chunk_t *chunk_get_next_nblank(chunk_t *cur, scope_e scope)
{
   return(chunk_search(cur, chunk_is_comment_newline_or_blank, scope, direction_e::FORWARD, false));
}


chunk_t *chunk_get_prev_nblank(chunk_t *cur, scope_e scope)
{
   return(chunk_search(cur, chunk_is_comment_newline_or_blank, scope, direction_e::BACKWARD, false));
}


chunk_t *chunk_get_next_nc(chunk_t *cur, scope_e scope)
{
   return(chunk_search(cur, chunk_is_comment, scope, direction_e::FORWARD, false));
}


chunk_t *chunk_get_next_nisq(chunk_t *cur, scope_e scope)
{
   return(chunk_search(cur, chunk_is_balanced_square, scope, direction_e::FORWARD, false));
}


chunk_t *chunk_get_prev_ncnl(chunk_t *cur, scope_e scope)
{
   return(chunk_search(cur, chunk_is_comment_or_newline, scope, direction_e::BACKWARD, false));
}


chunk_t *chunk_get_prev_nc(chunk_t *cur, scope_e scope)
{
   return(chunk_search(cur, chunk_is_comment, scope, direction_e::BACKWARD, false));
}


chunk_t *chunk_get_next_type(chunk_t *cur, c_token_t type, int level, scope_e scope)
{
   return(chunk_search_typelevel(cur, type, scope, direction_e::FORWARD, level));
}


chunk_t *chunk_get_next_str(chunk_t *cur, const char *str, size_t len, int level, scope_e scope)
{
   return(chunk_search_str(cur, str, len, scope, direction_e::FORWARD, level));
}


chunk_t *chunk_get_prev_type(chunk_t *cur, c_token_t type, int level, scope_e scope)
{
   return(chunk_search_typelevel(cur, type, scope, direction_e::BACKWARD, level));
}


chunk_t *chunk_get_prev_str(chunk_t *cur, const char *str, size_t len, int level, scope_e scope)
{
   return(chunk_search_str(cur, str, len, scope, direction_e::BACKWARD, level));
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


// TODO: the following function shall be made similar to the search functions
chunk_t *chunk_first_on_line(chunk_t *pc)
{
   chunk_t *first = pc;

   while ((pc = chunk_get_prev(pc)) != nullptr && !chunk_is_newline(pc))
   {
      first = pc;
   }

   return(first);
}


bool chunk_is_last_on_line(chunk_t &pc)  //TODO: pc should be const here
{
   // check if pc is the very last chunk of the file
   const auto *end = chunk_get_tail();

   if (&pc == end)
   {
      return(true);
   }

   // if the next chunk is a newline then pc is the last chunk on its line
   const auto *next = chunk_get_next(&pc);
   if (next != nullptr && next->type == CT_NEWLINE)
   {
      return(true);
   }

   return(false);
}


// TODO: this function needs some cleanup
void chunk_swap_lines(chunk_t *pc1, chunk_t *pc2)
{
   // to swap lines we need to find the first chunk of the lines
   pc1 = chunk_first_on_line(pc1);
   pc2 = chunk_first_on_line(pc2);

   if (  pc1 == nullptr
      || pc2 == nullptr
      || pc1 == pc2)
   {
      return;
   }

   /*
    * Example start:
    * ? - start1 - a1 - b1 - nl1 - ? - ref2 - start2 - a2 - b2 - nl2 - ?
    *      ^- pc1                              ^- pc2
    */
   chunk_t *ref2 = chunk_get_prev(pc2);

   // Move the line started at pc2 before pc1
   while (pc2 != nullptr && !chunk_is_newline(pc2))
   {
      chunk_t *tmp = chunk_get_next(pc2);
      g_cl.Pop(pc2);
      g_cl.AddBefore(pc2, pc1);
      pc2 = tmp;
   }

   /*
    * Should now be:
    * ? - start2 - a2 - b2 - start1 - a1 - b1 - nl1 - ? - ref2 - nl2 - ?
    *                         ^- pc1                              ^- pc2
    */

   // Now move the line started at pc1 after ref2
   while (pc1 != nullptr && !chunk_is_newline(pc1))
   {
      chunk_t *tmp = chunk_get_next(pc1);
      g_cl.Pop(pc1);
      if (ref2 != nullptr)
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

   /*
    * Should now be:
    * ? - start2 - a2 - b2 - nl1 - ? - ref2 - start1 - a1 - b1 - nl2 - ?
    *                         ^- pc1                              ^- pc2
    */

   /*
    * pc1 and pc2 should be the newlines for their lines.
    * swap the chunks and the nl_count so that the spacing remains the same.
    */
   if (pc1 != nullptr && pc2 != nullptr)
   {
      size_t nl_count = pc1->nl_count;

      pc1->nl_count = pc2->nl_count;
      pc2->nl_count = nl_count;

      chunk_swap(pc1, pc2);
   }
} // chunk_swap_lines


chunk_t *chunk_get_next_nvb(chunk_t *cur, const scope_e scope)
{
   return(chunk_search(cur, chunk_is_vbrace, scope, direction_e::FORWARD, false));
}


chunk_t *chunk_get_prev_nvb(chunk_t *cur, const scope_e scope)
{
   return(chunk_search(cur, chunk_is_vbrace, scope, direction_e::BACKWARD, false));
}


void set_chunk_type_real(chunk_t *pc, c_token_t tt)
{
   set_chunk_real(pc, tt, LSETTYP);
}


void set_chunk_parent_real(chunk_t *pc, c_token_t pt)
{
   set_chunk_real(pc, pt, LSETPAR);
}


void chunk_flags_set_real(chunk_t *pc, UINT64 clr_bits, UINT64 set_bits)
{
   if (pc != nullptr)
   {
      LOG_FUNC_ENTRY();
      UINT64 nflags = (pc->flags & ~clr_bits) | set_bits;
      if (pc->flags != nflags)
      {
         LOG_FMT(LSETFLG, "%s(%d): %016" PRIx64 "^%016" PRIx64 "=%016" PRIx64 " orig_line is %zu, orig_col is %zu, text() '%s', type is %s, parent_type is %s",
                 __func__, __LINE__, pc->flags, pc->flags ^ nflags, nflags,
                 pc->orig_line, pc->orig_col, pc->text(),
                 get_token_name(pc->type), get_token_name(pc->parent_type));
         log_func_stack_inline(LSETFLG);
         pc->flags = nflags;
      }
   }
}


void set_chunk_real(chunk_t *pc, c_token_t token, log_sev_t what)
{
   LOG_FUNC_ENTRY();

   c_token_t *where;
   c_token_t *type;
   c_token_t *parent_type;

   switch (what)
   {
   case (LSETTYP):
      where       = &pc->type;
      type        = &token;
      parent_type = &pc->parent_type;
      break;

   case (LSETPAR):
      where       = &pc->parent_type;
      type        = &pc->type;
      parent_type = &token;
      break;

   default:
      return;
   }

   if (pc != nullptr && *where != token)
   {
      LOG_FMT(what, "%s(%d): orig_line is %zu, orig_col is %zu, pc->text() '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
      LOG_FMT(what, "   pc->type is %s, pc->parent_type is %s => *type is %s, *parent_type is %s",
              get_token_name(pc->type), get_token_name(pc->parent_type),
              get_token_name(*type), get_token_name(*parent_type));
      log_func_stack_inline(what);
      *where = token;
   }
}


static chunk_t *chunk_get_ncnlnp(chunk_t *cur, const scope_e scope, const direction_e dir)
{
   chunk_t *pc = cur;

   pc = (chunk_is_preproc(pc) == true) ?
        chunk_search(pc, chunk_is_comment_or_newline_in_preproc, scope, dir, false) :
        chunk_search(pc, chunk_is_comment_newline_or_preproc, scope, dir, false);
   return(pc);
}


static chunk_t *chunk_get_ncnlnpnd(chunk_t *cur, const scope_e scope, const direction_e dir)
{
   search_t search_function = select_search_fct(dir);
   chunk_t  *pc             = cur;

   do                                   // loop over the chunk list
   {
      pc = search_function(pc, scope);  // in either direction while
   } while (  pc != nullptr             // the end of the list was not reached yet
           && !chunk_is_comment_or_newline(pc)
           && !chunk_is_preproc(pc)
           && (pc->type == CT_DC_MEMBER));
   return(pc);
}


static chunk_t *chunk_add(const chunk_t *pc_in, chunk_t *ref, const direction_e pos)
{
   chunk_t *pc = chunk_dup(pc_in);

   if (pc != nullptr)
   {
      if (ref != nullptr) // ref is a valid chunk
      {
         (pos == direction_e::FORWARD) ? g_cl.AddAfter(pc, ref) : g_cl.AddBefore(pc, ref);
      }
      else // ref == NULL
      {
         (pos == direction_e::FORWARD) ? g_cl.AddHead(pc) : g_cl.AddTail(pc);
      }
      chunk_log(pc, "chunk_add");
   }
   return(pc);
}
