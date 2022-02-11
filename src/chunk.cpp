/**
 * @file chunk.cpp
 * Manages and navigates the list of chunks.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "chunk.h"

#include "ListManager.h"
#include "prototypes.h"
#include "space.h"

typedef ListManager<Chunk> ChunkList_t;


ChunkList_t g_cl; //! global chunk list


/*
 * Chunk class methods
 */

// Null Chunk
Chunk        Chunk::NullChunk(true);
Chunk *const Chunk::NullChunkPtr(&Chunk::NullChunk);


Chunk::Chunk(bool null_c)
   : null_chunk(null_c)
{
   reset();
}


Chunk::Chunk(const Chunk &o)
   : null_chunk(o.null_chunk)
{
   copyFrom(o);
}


Chunk &Chunk::operator=(const Chunk &o)
{
   if (this != &o)
   {
      copyFrom(o);
   }
   return(*this);
}


void Chunk::copyFrom(const Chunk &o)
{
   next        = nullptr;
   prev        = nullptr;
   parent      = nullptr;
   align       = o.align;
   indent      = o.indent;
   type        = o.type;
   parent_type = o.parent_type;

   orig_line     = o.orig_line;
   orig_col      = o.orig_col;
   orig_col_end  = o.orig_col_end;
   orig_prev_sp  = o.orig_prev_sp;
   flags         = o.flags;
   column        = o.column;
   column_indent = o.column_indent;

   nl_count  = o.nl_count;
   nl_column = o.nl_column;
   level     = o.level;

   brace_level = o.brace_level;
   pp_level    = o.pp_level;
   after_tab   = o.after_tab;
   str         = o.str;

   tracking = o.tracking;
}


void Chunk::reset()
{
   memset(&align, 0, sizeof(align));
   memset(&indent, 0, sizeof(indent));
   next          = nullptr;
   prev          = nullptr;
   parent        = nullptr;
   type          = CT_NONE;
   parent_type   = CT_NONE;
   orig_line     = 0;
   orig_col      = 0;
   orig_col_end  = 0;
   orig_prev_sp  = 0;
   flags         = PCF_NONE;
   column        = 0;
   column_indent = 0;
   nl_count      = 0;
   nl_column     = 0;
   level         = 0;
   brace_level   = 0;
   pp_level      = 999;                                // use a big value to find some errors
   after_tab     = false;
   // for debugging purpose only
   tracking = nullptr;
   str.clear();
}


size_t Chunk::len() const
{
   return(str.size());
}


//! provides the content of a string a zero terminated character pointer
const char *Chunk::text() const
{
   return(str.c_str());
}


const char *Chunk::elided_text(char *for_the_copy) const
{
   const char *test_it       = text();
   size_t     test_it_length = strlen(test_it);

   size_t     truncate_value = uncrustify::options::debug_truncate();

   if (truncate_value != 0)
   {
      if (test_it_length > truncate_value)
      {
         memset(for_the_copy, 0, 1000);

         if (test_it_length < truncate_value + 30)
         {
            strncpy(for_the_copy, test_it, truncate_value - 30);
            for_the_copy[truncate_value - 30] = 0;
         }
         else
         {
            strncpy(for_the_copy, test_it, truncate_value);
            for_the_copy[truncate_value] = 0;
         }
         char *message = strcat(for_the_copy, " ... <The string is truncated>");

         return(message);
      }
      else
      {
         return(test_it);
      }
   }
   return(test_it);
}


Chunk *Chunk::get_next(scope_e scope) const
{
   if (this->isNullChunk())
   {
      return(NullChunkPtr);
   }
   Chunk *pc = g_cl.GetNext(this);

   if (  pc == nullptr
      || pc->isNullChunk())
   {
      return(NullChunkPtr);
   }

   if (scope == scope_e::ALL)
   {
      return(pc);
   }

   if (this->flags.test(PCF_IN_PREPROC))
   {
      // If in a preproc, return a null chunk if trying to leave
      if (!pc->flags.test(PCF_IN_PREPROC))
      {
         return(NullChunkPtr);
      }
      return(pc);
   }

   // Not in a preproc, skip any preproc
   while (  pc != nullptr
         && pc->isNotNullChunk()
         && pc->flags.test(PCF_IN_PREPROC))
   {
      pc = g_cl.GetNext(pc);
   }

   if (  pc == nullptr
      || pc->isNullChunk())
   {
      return(NullChunkPtr);
   }
   return(pc);
} // Chunk::get_next


Chunk *Chunk::get_prev(scope_e scope) const
{
   if (this->isNullChunk())
   {
      return(NullChunkPtr);
   }
   Chunk *pc = g_cl.GetPrev(this);

   if (  pc == nullptr
      || pc->isNullChunk())
   {
      return(NullChunkPtr);
   }

   if (scope == scope_e::ALL)
   {
      return(pc);
   }

   if (this->flags.test(PCF_IN_PREPROC))
   {
      // If in a preproc, return a null chunk if trying to leave
      if (!pc->flags.test(PCF_IN_PREPROC))
      {
         return(NullChunkPtr);
      }
      return(pc);
   }

   // Not in a preproc, skip any preproc
   while (  pc != nullptr
         && pc->isNotNullChunk()
         && pc->flags.test(PCF_IN_PREPROC))
   {
      pc = g_cl.GetPrev(pc);
   }

   if (  pc == nullptr
      || pc->isNullChunk())
   {
      return(NullChunkPtr);
   }
   return(pc);
} // Chunk::get_prev


/**
 *
 * This is a temporary internal function that will progressively be
 * replaced by Chunk::Search().
 *
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
 * @retval Chunk  pointer to the found chunk
 */
// TODO remove when finished
static Chunk *__internal_chunk_search(Chunk *cur, const check_t check_fct, const scope_e scope = scope_e::ALL, const direction_e dir = direction_e::FORWARD, const bool cond = true);


/**
 * @brief search for a chunk that satisfies a condition in a chunk list.
 *
 * This function is similar to __internal_chunk_search, except that it is tweaked to
 * handle searches inside of preprocessor directives. Specifically, if the
 * starting token is inside a preprocessor directive, it will ignore a line
 * continuation, and will abort the search if it reaches the end of the
 * directive. This function only searches forward.
 *
 * @param  cur        chunk to start search at
 * @param  check_fct  compare function
 * @param  scope      code parts to consider for search
 * @param  cond       success condition
 *
 * @retval nullptr  no requested chunk was found or invalid parameters provided
 * @retval Chunk  pointer to the found chunk or pointer to the chunk at the
 *                  end of the preprocessor directive
 */
static Chunk *chunk_ppa_search(Chunk *cur, const check_t check_fct, const bool cond = true);


static void chunk_log(Chunk *pc, const char *text);


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
 * This function is a specialization of __internal_chunk_search.
 *
 * @param cur    chunk to start search at
 * @param type   category to search for
 * @param scope  code parts to consider for search
 * @param dir    search direction
 *
 * @retval nullptr  no chunk found or invalid parameters provided
 * @retval Chunk  pointer to the found chunk
 */
static Chunk *chunk_search_type(Chunk *cur, const c_token_t type, const scope_e scope = scope_e::ALL, const direction_e dir = direction_e::FORWARD);


/**
 * @brief search a chunk of a given type and level
 *
 * Traverses a chunk list in the specified direction until a chunk of a given type
 * is found.
 *
 * This function is a specialization of __internal_chunk_search.
 *
 * @param cur    chunk to start search at
 * @param type   category to search for
 * @param scope  code parts to consider for search
 * @param dir    search direction
 * @param level  nesting level to match or -1 / ANY_LEVEL
 *
 * @retval nullptr  no chunk found or invalid parameters provided
 * @retval Chunk  pointer to the found chunk
 */
static Chunk *chunk_search_type_level(Chunk *cur, c_token_t type, scope_e scope = scope_e::ALL, direction_e dir = direction_e::FORWARD, int level = -1);


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
 * @retval Chunk  pointer to the found chunk
 */
static Chunk *chunk_search_str(Chunk *cur, const char *str, size_t len, scope_e scope, direction_e dir, int level);


/**
 * @brief Add a new chunk before/after the given position in a chunk list
 *
 * If ref is nullptr, add either at the head or tail based on the specified pos
 *
 * @param  pc_in  chunk to add to list
 * @param  ref    insert position in list
 * @param  pos    insert before or after
 *
 * @return Chunk  pointer to the added chunk
 */
static Chunk *chunk_add(const Chunk *pc_in, Chunk *ref, const direction_e pos = direction_e::FORWARD);


Chunk *Chunk::get_head(void)
{
   Chunk *ret = g_cl.GetHead();

   if (ret == nullptr)
   {
      return(Chunk::NullChunkPtr);
   }
   return(ret);
}


Chunk *Chunk::get_tail(void)
{
   Chunk *ret = g_cl.GetTail();

   if (ret == nullptr)
   {
      return(Chunk::NullChunkPtr);
   }
   return(ret);
}


Chunk::Search_t Chunk::Search_dir_fct(const direction_e dir)
{
   return((dir == direction_e::FORWARD) ? &Chunk::get_next : &Chunk::get_prev);
}


// TODO replace ::check_t with Chunk::Check_t when feasible
Chunk *Chunk::Search(const ::check_t check_fct, const scope_e scope,
                     const direction_e dir, const bool cond)
{
   Search_t dir_fct = Search_dir_fct(dir);
   Chunk    *pc     = this;

   do                                   // loop over the chunk list
   {
      pc = (pc->*dir_fct)(scope);       // in either direction while
   } while (  pc->isNotNullChunk()      // the end of the list was not reached yet
           && (check_fct(pc) != cond)); // and the demanded chunk was not found either

   return(pc);                          // the latest chunk is the searched one
}


static Chunk *__internal_chunk_search(Chunk *cur, const check_t check_fct, const scope_e scope,
                                      const direction_e dir, const bool cond)
{
   /*
    * Depending on the parameter dir the search function searches
    * in forward or backward direction */
   Chunk::Search_t search_function = Chunk::Search_dir_fct(dir);
   Chunk           *pc             = cur;

   if (pc == nullptr)
   {
      pc = Chunk::NullChunkPtr;
   }

   do                                     // loop over the chunk list
   {
      pc = (pc->*search_function)(scope); // in either direction while
   } while (  pc->isNotNullChunk()        // the end of the list was not reached yet
           && (check_fct(pc) != cond));   // and the demanded chunk was not found either

   if (pc->isNullChunk())
   {
      pc = nullptr;
   }
   return(pc);                            // the latest chunk is the searched one
}


Chunk *chunk_search_prev_cat(Chunk *pc, const c_token_t cat)
{
   return(chunk_search_type(pc, cat, scope_e::ALL, direction_e::BACKWARD));
}


Chunk *chunk_search_next_cat(Chunk *pc, const c_token_t cat)
{
   return(chunk_search_type(pc, cat, scope_e::ALL, direction_e::FORWARD));
}


bool are_chunks_in_same_line(Chunk *start, Chunk *end)
{
   if (start == nullptr)
   {
      return(false);
   }
   Chunk *tmp = start->get_next();

   while (  tmp->isNotNullChunk()
         && tmp != end)
   {
      if (chunk_is_token(tmp, CT_NEWLINE))
      {
         return(false);
      }
      tmp = tmp->get_next();
   }
   return(true);
}


static Chunk *chunk_search_type(Chunk *cur, const c_token_t type,
                                const scope_e scope, const direction_e dir)
{
   /*
    * Depending on the parameter dir the search function searches
    * in forward or backward direction
    */
   Chunk::Search_t search_function = Chunk::Search_dir_fct(dir);
   Chunk           *pc             = cur;

   if (pc == nullptr)
   {
      pc = Chunk::NullChunkPtr;
   }

   do                                     // loop over the chunk list
   {
      pc = (pc->*search_function)(scope); // in either direction while
   } while (  pc->isNotNullChunk()        // the end of the list was not reached yet
           && pc->type != type);          // and the demanded chunk was not found either

   if (pc->isNullChunk())
   {
      pc = nullptr;
   }
   return(pc);                            // the latest chunk is the searched one
}


static Chunk *chunk_search_type_level(Chunk *cur, c_token_t type, scope_e scope, direction_e dir, int level)
{
   /*
    * Depending on the parameter dir the search function searches
    * in forward or backward direction
    */
   Chunk::Search_t search_function = Chunk::Search_dir_fct(dir);
   Chunk           *pc             = cur;

   if (pc == nullptr)
   {
      pc = Chunk::NullChunkPtr;
   }

   do                                     // loop over the chunk list
   {
      pc = (pc->*search_function)(scope); // in either direction while
   } while (  pc->isNotNullChunk()        // the end of the list was not reached yet
           && (!is_expected_type_and_level(pc, type, level)));

   if (pc->isNullChunk())
   {
      pc = nullptr;
   }
   return(pc);                            // the latest chunk is the searched one
}


static Chunk *chunk_search_str(Chunk *cur, const char *str, size_t len, scope_e scope, direction_e dir, int level)
{
   /*
    * Depending on the parameter dir the search function searches
    * in forward or backward direction */
   Chunk::Search_t search_function = Chunk::Search_dir_fct(dir);
   Chunk           *pc             = cur;

   if (pc == nullptr)
   {
      pc = Chunk::NullChunkPtr;
   }

   do                                     // loop over the chunk list
   {
      pc = (pc->*search_function)(scope); // in either direction while
   } while (  pc->isNotNullChunk()        // the end of the list was not reached yet
           && (!is_expected_string_and_level(pc, str, level, len)));

   if (pc->isNullChunk())
   {
      pc = nullptr;
   }
   return(pc);                            // the latest chunk is the searched one
}


static Chunk *chunk_ppa_search(Chunk *cur, const check_t check_fct, const bool cond)
{
   if (  cur != nullptr
      && !cur->flags.test(PCF_IN_PREPROC))
   {
      // if not in preprocessor, do a regular search
      return(__internal_chunk_search(cur, check_fct, scope_e::ALL,
                                     direction_e::FORWARD, cond));
   }
   Chunk *pc = cur;

   while (  pc != nullptr
         && (pc = pc->next) != nullptr)
   {
      if (!pc->flags.test(PCF_IN_PREPROC))
      {
         // Bail if we run off the end of the preprocessor directive, but
         // return the next token, NOT nullptr, because the caller may need to
         // know where the search ended
         assert(chunk_is_token(pc, CT_NEWLINE));
         return(pc);
      }

      if (chunk_is_token(pc, CT_NL_CONT))
      {
         // Skip line continuation
         continue;
      }

      if (check_fct(pc) == cond)
      {
         // Requested token was found
         return(pc);
      }
   }
   // Ran out of tokens
   return(nullptr);
}


static void chunk_log_msg(Chunk *chunk, const log_sev_t log, const char *str)
{
   LOG_FMT(log, "%s orig_line is %zu, orig_col is %zu, ",
           str, chunk->orig_line, chunk->orig_col);

   if (chunk_is_token(chunk, CT_NEWLINE))
   {
      LOG_FMT(log, "<Newline>,\n");
   }
   else if (chunk_is_token(chunk, CT_VBRACE_OPEN))
   {
      LOG_FMT(log, "<VBRACE_OPEN>,\n");
   }
   else if (chunk_is_token(chunk, CT_VBRACE_CLOSE))
   {
      LOG_FMT(log, "<VBRACE_CLOSE>,\n");
   }
   else
   {
      LOG_FMT(log, "text() is '%s', type is %s,\n", chunk->text(), get_token_name(chunk->type));
   }
}


static void chunk_log(Chunk *pc, const char *text)
{
   if (  pc != nullptr
      && pc->isNotNullChunk()
      && (cpd.unc_stage != unc_stage_e::TOKENIZE)
      && (cpd.unc_stage != unc_stage_e::CLEANUP))
   {
      const log_sev_t log   = LCHUNK;
      Chunk           *prev = pc->get_prev();
      Chunk           *next = pc->get_next();

      chunk_log_msg(pc, log, text);

      if (  prev->isNotNullChunk()
         && next->isNotNullChunk())
      {
         chunk_log_msg(prev, log, "   @ between");
         chunk_log_msg(next, log, "   and");
      }
      else if (next->isNotNullChunk())
      {
         chunk_log_msg(next, log, "   @ before");
      }
      else if (prev->isNotNullChunk())
      {
         chunk_log_msg(prev, log, "   @ after");
      }
      LOG_FMT(log, "   stage is %s",                             // Issue #3034
              get_unc_stage_name(cpd.unc_stage));
      log_func_stack_inline(log);
   }
}


Chunk *chunk_add_after(const Chunk *pc_in, Chunk *ref)
{
   return(chunk_add(pc_in, ref, direction_e::FORWARD));
}


Chunk *chunk_add_before(const Chunk *pc_in, Chunk *ref)
{
   return(chunk_add(pc_in, ref, direction_e::BACKWARD));
}


void chunk_del(Chunk * &pc)
{
   g_cl.Pop(pc);
   delete pc;
   pc = nullptr;
}


void chunk_move_after(Chunk *pc_in, Chunk *ref)
{
   LOG_FUNC_ENTRY();
   g_cl.Pop(pc_in);
   g_cl.AddAfter(pc_in, ref);

   // HACK: Adjust the original column
   pc_in->column       = ref->column + space_col_align(ref, pc_in);
   pc_in->orig_col     = pc_in->column;
   pc_in->orig_col_end = pc_in->orig_col + pc_in->len();
}


Chunk *Chunk::get_next_nl(scope_e scope)
{
   return(Search(chunk_is_newline, scope, direction_e::FORWARD, true));
}


Chunk *Chunk::get_prev_nl(scope_e scope)
{
   return(Search(chunk_is_newline, scope, direction_e::BACKWARD, true));
}


Chunk *Chunk::get_next_nnl(scope_e scope)
{
   return(Search(chunk_is_newline, scope, direction_e::FORWARD, false));
}


Chunk *Chunk::get_prev_nnl(scope_e scope)
{
   return(Search(chunk_is_newline, scope, direction_e::BACKWARD, false));
}


Chunk *Chunk::get_next_nc(scope_e scope)
{
   return(Search(chunk_is_comment, scope, direction_e::FORWARD, false));
}


Chunk *Chunk::get_prev_nc(scope_e scope)
{
   return(Search(chunk_is_comment, scope, direction_e::BACKWARD, false));
}


Chunk *chunk_get_next_nc_nnl(Chunk *cur, scope_e scope)
{
   return(__internal_chunk_search(cur, chunk_is_comment_or_newline, scope, direction_e::FORWARD, false));
}


Chunk *chunk_get_prev_nc_nnl(Chunk *cur, scope_e scope)
{
   return(__internal_chunk_search(cur, chunk_is_comment_or_newline, scope, direction_e::BACKWARD, false));
}


Chunk *chunk_get_next_nc_nnl_np(Chunk *cur, scope_e scope)
{
   return(__internal_chunk_search(cur, chunk_is_comment_newline_or_preproc, scope, direction_e::FORWARD, false));
}


Chunk *chunk_get_prev_nc_nnl_np(Chunk *cur, scope_e scope)
{
   return(__internal_chunk_search(cur, chunk_is_comment_newline_or_preproc, scope, direction_e::BACKWARD, false));
}


Chunk *chunk_get_next_nc_nnl_in_pp(Chunk *cur, scope_e scope)
{
   return(__internal_chunk_search(cur, chunk_is_comment_or_newline_in_preproc, scope, direction_e::FORWARD, false));
}


Chunk *chunk_get_prev_nc_nnl_in_pp(Chunk *cur, scope_e scope)
{
   return(__internal_chunk_search(cur, chunk_is_comment_or_newline_in_preproc, scope, direction_e::BACKWARD, false));
}


Chunk *chunk_ppa_get_next_nc_nnl(Chunk *cur)
{
   return(chunk_ppa_search(cur, chunk_is_comment_or_newline, false));
}


Chunk *chunk_get_next_nc_nnl_nb(Chunk *cur, scope_e scope)
{
   return(__internal_chunk_search(cur, chunk_is_comment_newline_or_blank, scope, direction_e::FORWARD, false));
}


Chunk *chunk_get_prev_nc_nnl_nb(Chunk *cur, scope_e scope)
{
   return(__internal_chunk_search(cur, chunk_is_comment_newline_or_blank, scope, direction_e::BACKWARD, false));
}


Chunk *chunk_get_next_nisq(Chunk *cur, scope_e scope)
{
   return(__internal_chunk_search(cur, chunk_is_balanced_square, scope, direction_e::FORWARD, false));
}


Chunk *chunk_get_prev_nc_nnl_ni(Chunk *cur, scope_e scope)
{
   return(__internal_chunk_search(cur, chunk_is_comment_or_newline_or_ignored, scope, direction_e::BACKWARD, false));
}


Chunk *chunk_get_next_type(Chunk *cur, c_token_t type, int level, scope_e scope)
{
   return(chunk_search_type_level(cur, type, scope, direction_e::FORWARD, level));
}


Chunk *chunk_get_prev_type(Chunk *cur, c_token_t type, int level, scope_e scope)
{
   return(chunk_search_type_level(cur, type, scope, direction_e::BACKWARD, level));
}


Chunk *chunk_get_next_str(Chunk *cur, const char *str, size_t len, int level, scope_e scope)
{
   return(chunk_search_str(cur, str, len, scope, direction_e::FORWARD, level));
}


Chunk *chunk_get_prev_str(Chunk *cur, const char *str, size_t len, int level, scope_e scope)
{
   return(chunk_search_str(cur, str, len, scope, direction_e::BACKWARD, level));
}


bool chunk_is_newline_between(Chunk *start, Chunk *end)
{
   for (Chunk *pc = start; pc != nullptr && pc != end; pc = pc->get_next())
   {
      if (chunk_is_newline(pc))
      {
         return(true);
      }
   }

   return(false);
}


void chunk_swap(Chunk *pc1, Chunk *pc2)
{
   g_cl.Swap(pc1, pc2);
}


// TODO: the following function shall be made similar to the search functions
Chunk *chunk_first_on_line(Chunk *pc)
{
   if (  pc == nullptr
      || pc->isNullChunk())
   {
      return(Chunk::NullChunkPtr);
   }
   Chunk *first = pc;

   while (  (pc = pc->get_prev()) != nullptr
         && pc->isNotNullChunk()
         && !chunk_is_newline(pc))
   {
      first = pc;
   }
   return(first);
}


bool chunk_is_last_on_line(Chunk *pc)  //TODO: pc should be const here
{
   // check if pc is the very last chunk of the file
   const Chunk *end = Chunk::get_tail();

   if (pc == end)
   {
      return(true);
   }
   // if the next chunk is a newline then pc is the last chunk on its line
   const Chunk *next = pc->get_next();

   if (chunk_is_token(next, CT_NEWLINE))
   {
      return(true);
   }
   return(false);
}


// TODO: this function needs some cleanup
void chunk_swap_lines(Chunk *pc1, Chunk *pc2)
{
   // to swap lines we need to find the first chunk of the lines
   pc1 = chunk_first_on_line(pc1);
   pc2 = chunk_first_on_line(pc2);

   if (  pc1->isNullChunk()
      || pc2->isNullChunk()
      || pc1 == pc2)
   {
      return;
   }
   /*
    * Example start:
    * ? - start1 - a1 - b1 - nl1 - ? - ref2 - start2 - a2 - b2 - nl2 - ?
    *      ^- pc1                              ^- pc2
    */
   Chunk *ref2 = pc2->get_prev();

   // Move the line started at pc2 before pc1
   while (  pc2->isNotNullChunk()
         && !chunk_is_newline(pc2))
   {
      Chunk *tmp = pc2->get_next();
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
   while (  pc1->isNotNullChunk()
         && !chunk_is_newline(pc1))
   {
      Chunk *tmp = pc1->get_next();
      g_cl.Pop(pc1);

      if (ref2->isNotNullChunk())
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
   if (  pc1->isNotNullChunk()
      && pc2->isNotNullChunk())
   {
      size_t nl_count = pc1->nl_count;

      pc1->nl_count = pc2->nl_count;
      pc2->nl_count = nl_count;

      chunk_swap(pc1, pc2);
   }
} // chunk_swap_lines


Chunk *chunk_get_next_nvb(Chunk *cur, const scope_e scope)
{
   return(__internal_chunk_search(cur, chunk_is_vbrace, scope, direction_e::FORWARD, false));
}


Chunk *chunk_get_prev_nvb(Chunk *cur, const scope_e scope)
{
   return(__internal_chunk_search(cur, chunk_is_vbrace, scope, direction_e::BACKWARD, false));
}


void chunk_flags_set_real(Chunk *pc, pcf_flags_t clr_bits, pcf_flags_t set_bits)
{
   if (  pc != nullptr
      && pc->isNotNullChunk())
   {
      LOG_FUNC_ENTRY();
      auto const nflags = (pc->flags & ~clr_bits) | set_bits;

      if (pc->flags != nflags)
      {
         LOG_FMT(LSETFLG,
                 "%s(%d): %016llx^%016llx=%016llx\n"
                 "   orig_line is %zu, orig_col is %zu, text() '%s', type is %s,",
                 __func__, __LINE__,
                 static_cast<pcf_flags_t::int_t>(pc->flags),
                 static_cast<pcf_flags_t::int_t>(pc->flags ^ nflags),
                 static_cast<pcf_flags_t::int_t>(nflags),
                 pc->orig_line, pc->orig_col, pc->text(),
                 get_token_name(pc->type));
         LOG_FMT(LSETFLG, " parent_type is %s,\n  ",
                 get_token_name(get_chunk_parent_type(pc)));
         log_func_stack_inline(LSETFLG);
         pc->flags = nflags;
      }
   }
}


void set_chunk_type_real(Chunk *pc, c_token_t token, const char *func, int line)
{
   LOG_FUNC_ENTRY();

   if (  pc == nullptr
      || pc->type == token)
   {
      return;
   }
   LOG_FMT(LSETTYP, "%s(%d): orig_line is %zu, orig_col is %zu, pc->text() ",
           func, line, pc->orig_line, pc->orig_col);

   if (token == CT_NEWLINE)
   {
      LOG_FMT(LSETTYP, "<Newline>\n");
   }
   else
   {
      LOG_FMT(LSETTYP, "'%s'\n", pc->text());
   }
   LOG_FMT(LSETTYP, "   pc->type is %s, pc->parent_type is %s => *type is %s, *parent_type is %s\n",
           get_token_name(pc->type), get_token_name(get_chunk_parent_type(pc)),
           get_token_name(token), get_token_name(get_chunk_parent_type(pc)));
   pc->type = token;
} // set_chunk_type_real


void set_chunk_parent_real(Chunk *pc, c_token_t token, const char *func, int line)
{
   LOG_FUNC_ENTRY();

   if (  pc == nullptr
      || get_chunk_parent_type(pc) == token)
   {
      return;
   }
   LOG_FMT(LSETPAR, "%s(%d): orig_line is %zu, orig_col is %zu, pc->text() ",
           func, line, pc->orig_line, pc->orig_col);

   if (token == CT_NEWLINE)
   {
      LOG_FMT(LSETPAR, "<Newline>\n");
   }
   else
   {
      char copy[1000];
      LOG_FMT(LSETPAR, "'%s'\n", pc->elided_text(copy));
   }
   LOG_FMT(LSETPAR, "   pc->type is %s, pc->parent_type is %s => *type is %s, *parent_type is %s\n",
           get_token_name(pc->type), get_token_name(get_chunk_parent_type(pc)),
           get_token_name(token), get_token_name(get_chunk_parent_type(pc)));
   pc->parent_type = token;
} // set_chunk_parent_real


c_token_t get_chunk_parent_type(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   if (pc == nullptr)
   {
      return(CT_NONE);
   }
   return(pc->parent_type);
} // get_chunk_parent_type


static Chunk *chunk_add(const Chunk *pc_in, Chunk *ref, const direction_e pos)
{
#ifdef DEBUG
   // test if the pc_in chunk is properly set
   if (pc_in->pp_level == 999)
   {
      fprintf(stderr, "%s(%d): pp_level is not set\n", __func__, __LINE__);
      log_func_stack_inline(LSETFLG);
      log_flush(true);
      exit(EX_SOFTWARE);
   }

   if (pc_in->orig_line == 0)
   {
      fprintf(stderr, "%s(%d): no line number\n", __func__, __LINE__);
      log_func_stack_inline(LSETFLG);
      log_flush(true);
      exit(EX_SOFTWARE);
   }

   if (pc_in->orig_col == 0)
   {
      fprintf(stderr, "%s(%d): no column number\n", __func__, __LINE__);
      log_func_stack_inline(LSETFLG);
      log_flush(true);
      exit(EX_SOFTWARE);
   }
#endif /* DEBUG */

   Chunk *pc = new Chunk(*pc_in);

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
      chunk_log(pc, "chunk_add(A):");
   }
   return(pc);
} // chunk_add


Chunk *chunk_get_next_ssq(Chunk *cur)
{
   while (  chunk_is_token(cur, CT_TSQUARE)
         || chunk_is_token(cur, CT_SQUARE_OPEN))
   {
      if (chunk_is_token(cur, CT_SQUARE_OPEN))
      {
         cur = chunk_skip_to_match(cur);
      }
      cur = chunk_get_next_nc_nnl(cur);
   }
   return(cur);
}


Chunk *chunk_get_prev_ssq(Chunk *cur)
{
   while (  chunk_is_token(cur, CT_TSQUARE)
         || chunk_is_token(cur, CT_SQUARE_CLOSE))
   {
      if (chunk_is_token(cur, CT_SQUARE_CLOSE))
      {
         cur = chunk_skip_to_match_rev(cur);
      }
      cur = chunk_get_prev_nc_nnl(cur);
   }
   return(cur);
}


Chunk *chunk_get_pp_start(Chunk *cur)
{
   if (!chunk_is_preproc(cur))
   {
      return(nullptr);
   }

   while (!chunk_is_token(cur, CT_PREPROC))
   {
      cur = cur->get_prev(scope_e::PREPROC);
   }
   return(cur);
}


//! skip to the final word/type in a :: chain
static Chunk *chunk_skip_dc_member(Chunk *start, scope_e scope, direction_e dir)
{
   LOG_FUNC_ENTRY();

   if (start == nullptr)
   {
      return(nullptr);
   }
   const auto step_fcn = (dir == direction_e::FORWARD)
                         ? chunk_get_next_nc_nnl : chunk_get_prev_nc_nnl;

   Chunk *pc   = start;
   Chunk *next = chunk_is_token(pc, CT_DC_MEMBER) ? pc : step_fcn(pc, scope);

   while (chunk_is_token(next, CT_DC_MEMBER))
   {
      pc = step_fcn(next, scope);

      if (pc == nullptr)
      {
         return(nullptr);
      }
      next = step_fcn(pc, scope);
   }
   return(pc);
}


Chunk *chunk_skip_dc_member(Chunk *start, scope_e scope)
{
   return(chunk_skip_dc_member(start, scope, direction_e::FORWARD));
}


Chunk *chunk_skip_dc_member_rev(Chunk *start, scope_e scope)
{
   return(chunk_skip_dc_member(start, scope, direction_e::BACKWARD));
}


// set parent member
void chunk_set_parent(Chunk *pc, Chunk *parent)
{
   if (pc == nullptr)
   {
      return;
   }

   if (parent == nullptr)
   {
      return;
   }

   if (pc == parent)
   {
      return;
   }
   pc->parent = parent;
}


c_token_t get_type_of_the_parent(Chunk *pc)
{
   if (pc == nullptr)
   {
      return(CT_UNKNOWN);
   }

   if (pc->parent == nullptr)
   {
      return(CT_PARENT_NOT_SET);
   }
   return(pc->parent->type);
}


bool chunk_is_attribute_or_declspec(Chunk *pc)
{
   return(  language_is_set(LANG_CPP)
         && (  chunk_is_token(pc, CT_ATTRIBUTE)
            || chunk_is_token(pc, CT_DECLSPEC)));
}


bool chunk_is_class_enum_struct_union(Chunk *pc)
{
   return(  chunk_is_class_or_struct(pc)
         || chunk_is_enum(pc)
         || chunk_is_token(pc, CT_UNION));
}


bool chunk_is_class_or_struct(Chunk *pc)
{
   return(  chunk_is_token(pc, CT_CLASS)
         || chunk_is_token(pc, CT_STRUCT));
}


bool chunk_is_class_struct_union(Chunk *pc)
{
   return(  chunk_is_class_or_struct(pc)
         || chunk_is_token(pc, CT_UNION));
}


bool chunk_is_enum(Chunk *pc)
{
   return(  chunk_is_token(pc, CT_ENUM)
         || chunk_is_token(pc, CT_ENUM_CLASS));
}


int chunk_compare_position(const Chunk *A_token, const Chunk *B_token)
{
   if (A_token == nullptr)
   {
      assert(A_token);
   }

   if (B_token == nullptr)
   {
      assert(B_token);
   }

   if (A_token->orig_line < B_token->orig_line)
   {
      return(-1);
   }
   else if (A_token->orig_line == B_token->orig_line)
   {
      if (A_token->orig_col < B_token->orig_col)
      {
         return(-1);
      }
      else if (A_token->orig_col == B_token->orig_col)
      {
         return(0);
      }
   }
   return(1);
}
