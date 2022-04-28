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
   Reset();
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


void Chunk::Reset()
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


size_t Chunk::Len() const
{
   return(str.size());
}


const char *Chunk::Text() const
{
   return(str.c_str());
}


const char *Chunk::ElidedText(char *for_the_copy) const
{
   const char *test_it       = Text();
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


Chunk *Chunk::GetNext(const E_Scope scope) const
{
   if (IsNullChunk())
   {
      return(NullChunkPtr);
   }
   Chunk *pc = g_cl.GetNext(this);

   if (  pc == nullptr
      || pc->IsNullChunk())
   {
      return(NullChunkPtr);
   }

   if (scope == E_Scope::ALL)
   {
      return(pc);
   }

   if (flags.test(PCF_IN_PREPROC))
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
         && pc->IsNotNullChunk()
         && pc->flags.test(PCF_IN_PREPROC))
   {
      pc = g_cl.GetNext(pc);
   }

   if (  pc == nullptr
      || pc->IsNullChunk())
   {
      return(NullChunkPtr);
   }
   return(pc);
} // Chunk::GetNext


Chunk *Chunk::GetPrev(const E_Scope scope) const
{
   if (IsNullChunk())
   {
      return(NullChunkPtr);
   }
   Chunk *pc = g_cl.GetPrev(this);

   if (  pc == nullptr
      || pc->IsNullChunk())
   {
      return(NullChunkPtr);
   }

   if (scope == E_Scope::ALL)
   {
      return(pc);
   }

   if (flags.test(PCF_IN_PREPROC))
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
         && pc->IsNotNullChunk()
         && pc->flags.test(PCF_IN_PREPROC))
   {
      pc = g_cl.GetPrev(pc);
   }

   if (  pc == nullptr
      || pc->IsNullChunk())
   {
      return(NullChunkPtr);
   }
   return(pc);
} // Chunk::GetPrev


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
 * This function is a specialization of Chunk::Search.
 *
 * @param cur    chunk to start search at
 * @param type   category to search for
 * @param scope  code parts to consider for search
 * @param dir    search direction
 *
 * @retval nullptr  no chunk found or invalid parameters provided
 * @retval Chunk  pointer to the found chunk
 */
static Chunk *chunk_search_type(Chunk *cur, const E_Token type, const E_Scope scope = E_Scope::ALL, const E_Direction dir = E_Direction::FORWARD);


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
static Chunk *chunk_add(const Chunk *pc_in, Chunk *ref, const E_Direction pos = E_Direction::FORWARD);


Chunk *Chunk::GetHead(void)
{
   Chunk *ret = g_cl.GetHead();

   if (ret == nullptr)
   {
      return(Chunk::NullChunkPtr);
   }
   return(ret);
}


Chunk *Chunk::GetTail(void)
{
   Chunk *ret = g_cl.GetTail();

   if (ret == nullptr)
   {
      return(Chunk::NullChunkPtr);
   }
   return(ret);
}


Chunk::T_SearchFnPtr Chunk::GetSearchFn(const E_Direction dir)
{
   return((dir == E_Direction::FORWARD) ? &Chunk::GetNext : &Chunk::GetPrev);
}


Chunk *Chunk::Search(const T_CheckFnPtr checkFn, const E_Scope scope,
                     const E_Direction dir, const bool cond) const
{
   T_SearchFnPtr searchFnPtr = GetSearchFn(dir);
   Chunk         *pc         = const_cast<Chunk *>(this);

   do                                      // loop over the chunk list
   {
      pc = (pc->*searchFnPtr)(scope);      // in either direction while
   } while (  pc->IsNotNullChunk()         // the end of the list was not reached yet
           && ((pc->*checkFn)() != cond)); // and the demanded chunk was not found either

   return(pc);                             // the latest chunk is the searched one
}


Chunk *chunk_search_prev_cat(Chunk *pc, const E_Token cat)
{
   return(chunk_search_type(pc, cat, E_Scope::ALL, E_Direction::BACKWARD));
}


Chunk *chunk_search_next_cat(Chunk *pc, const E_Token cat)
{
   return(chunk_search_type(pc, cat, E_Scope::ALL, E_Direction::FORWARD));
}


bool are_chunks_in_same_line(Chunk *start, Chunk *end)
{
   if (start == nullptr)
   {
      return(false);
   }
   Chunk *tmp = start->GetNext();

   while (  tmp->IsNotNullChunk()
         && tmp != end)
   {
      if (chunk_is_token(tmp, CT_NEWLINE))
      {
         return(false);
      }
      tmp = tmp->GetNext();
   }
   return(true);
}


static Chunk *chunk_search_type(Chunk *cur, const E_Token type,
                                const E_Scope scope, const E_Direction dir)
{
   /*
    * Depending on the parameter dir the search function searches
    * in forward or backward direction
    */
   Chunk::T_SearchFnPtr search_function = Chunk::GetSearchFn(dir);
   Chunk                *pc             = cur;

   if (pc == nullptr)
   {
      pc = Chunk::NullChunkPtr;
   }

   do                                     // loop over the chunk list
   {
      pc = (pc->*search_function)(scope); // in either direction while
   } while (  pc->IsNotNullChunk()        // the end of the list was not reached yet
           && pc->type != type);          // and the demanded chunk was not found either

   if (pc->IsNullChunk())
   {
      pc = nullptr;
   }
   return(pc);                            // the latest chunk is the searched one
}


Chunk *Chunk::SearchTypeLevel(const E_Token cType, const E_Scope scope,
                              const E_Direction dir, const int cLevel) const
{
   T_SearchFnPtr searchFnPtr = GetSearchFn(dir);
   Chunk         *pc         = const_cast<Chunk *>(this);

   do                                                // loop over the chunk list
   {
      pc = (pc->*searchFnPtr)(scope);                // in either direction while
   } while (  pc->IsNotNullChunk()                   // the end of the list was not reached yet
           && (!pc->IsTypeAndLevel(cType, cLevel))); // and the chunk was not found either

   return(pc);                                       // the latest chunk is the searched one
}


Chunk *Chunk::SearchStringLevel(const char *cStr, const size_t len, int cLevel,
                                const E_Scope scope, const E_Direction dir) const
{
   T_SearchFnPtr searchFnPtr = GetSearchFn(dir);
   Chunk         *pc         = const_cast<Chunk *>(this);

   do                                                // loop over the chunk list
   {
      pc = (pc->*searchFnPtr)(scope);                // in either direction while
   } while (  pc->IsNotNullChunk()                       // the end of the list was not reached yet
           && !pc->IsStringAndLevel(cStr, len, cLevel)); // and the demanded chunk was not found either

   return(pc);                                           // the latest chunk is the searched one
}


Chunk *Chunk::SearchPpa(const T_CheckFnPtr checkFn, const bool cond) const
{
   if (!flags.test(PCF_IN_PREPROC))
   {
      // if not in preprocessor, do a regular search
      return(Search(checkFn, E_Scope::ALL, E_Direction::FORWARD, cond));
   }
   Chunk *pc = GetNext();

   while (pc->IsNotNullChunk())
   {
      if (!pc->flags.test(PCF_IN_PREPROC))
      {
         // Bail if we run off the end of the preprocessor directive, but return
         // the token because the caller may need to know where the search ended
         assert(chunk_is_token(pc, CT_NEWLINE));
         return(pc);
      }

      if (chunk_is_token(pc, CT_NL_CONT))
      {
         // Skip line continuation
         pc = pc->GetNext();
         continue;
      }

      if ((pc->*checkFn)() == cond)
      {
         // Requested token was found
         return(pc);
      }
      pc = pc->GetNext();
   }
   // Ran out of tokens
   return(Chunk::NullChunkPtr);
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
      LOG_FMT(log, "Text() is '%s', type is %s,\n", chunk->Text(), get_token_name(chunk->type));
   }
}


static void chunk_log(Chunk *pc, const char *text)
{
   if (  pc != nullptr
      && pc->IsNotNullChunk()
      && (cpd.unc_stage != unc_stage_e::TOKENIZE)
      && (cpd.unc_stage != unc_stage_e::CLEANUP))
   {
      const log_sev_t log   = LCHUNK;
      Chunk           *prev = pc->GetPrev();
      Chunk           *next = pc->GetNext();

      chunk_log_msg(pc, log, text);

      if (  prev->IsNotNullChunk()
         && next->IsNotNullChunk())
      {
         chunk_log_msg(prev, log, "   @ between");
         chunk_log_msg(next, log, "   and");
      }
      else if (next->IsNotNullChunk())
      {
         chunk_log_msg(next, log, "   @ before");
      }
      else if (prev->IsNotNullChunk())
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
   return(chunk_add(pc_in, ref, E_Direction::FORWARD));
}


Chunk *chunk_add_before(const Chunk *pc_in, Chunk *ref)
{
   return(chunk_add(pc_in, ref, E_Direction::BACKWARD));
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
   pc_in->orig_col_end = pc_in->orig_col + pc_in->Len();
}


Chunk *Chunk::GetNextNl(const E_Scope scope) const
{
   return(Search(&Chunk::IsNewline, scope, E_Direction::FORWARD, true));
}


Chunk *Chunk::GetPrevNl(const E_Scope scope) const
{
   return(Search(&Chunk::IsNewline, scope, E_Direction::BACKWARD, true));
}


Chunk *Chunk::GetNextNnl(const E_Scope scope) const
{
   return(Search(&Chunk::IsNewline, scope, E_Direction::FORWARD, false));
}


Chunk *Chunk::GetPrevNnl(const E_Scope scope) const
{
   return(Search(&Chunk::IsNewline, scope, E_Direction::BACKWARD, false));
}


Chunk *Chunk::GetNextNc(const E_Scope scope) const
{
   return(Search(&Chunk::IsComment, scope, E_Direction::FORWARD, false));
}


Chunk *Chunk::GetPrevNc(const E_Scope scope) const
{
   return(Search(&Chunk::IsComment, scope, E_Direction::BACKWARD, false));
}


Chunk *Chunk::GetNextNcNnl(const E_Scope scope) const
{
   return(Search(&Chunk::IsCommentOrNewline, scope, E_Direction::FORWARD, false));
}


Chunk *Chunk::GetPrevNcNnl(const E_Scope scope) const
{
   return(Search(&Chunk::IsCommentOrNewline, scope, E_Direction::BACKWARD, false));
}


Chunk *Chunk::GetNextNcNnlNpp(const E_Scope scope) const
{
   return(Search(&Chunk::IsCommentNewlineOrPreproc, scope, E_Direction::FORWARD, false));
}


Chunk *Chunk::GetPrevNcNnlNpp(const E_Scope scope) const
{
   return(Search(&Chunk::IsCommentNewlineOrPreproc, scope, E_Direction::BACKWARD, false));
}


Chunk *Chunk::GetNextNppOrNcNnl(const E_Scope scope) const
{
   return(Search(&Chunk::IsCommentOrNewlineInPreproc, scope, E_Direction::FORWARD, false));
}


Chunk *Chunk::GetPrevNppOrNcNnl(const E_Scope scope) const
{
   return(Search(&Chunk::IsCommentOrNewlineInPreproc, scope, E_Direction::BACKWARD, false));
}


Chunk *Chunk::PpaGetNextNcNnl() const
{
   return(SearchPpa(&Chunk::IsCommentOrNewline, false));
}


Chunk *Chunk::GetNextNcNnlNet(const E_Scope scope) const
{
   return(Search(&Chunk::IsCommentNewlineOrEmptyText, scope, E_Direction::FORWARD, false));
}


Chunk *Chunk::GetPrevNcNnlNet(const E_Scope scope) const
{
   return(Search(&Chunk::IsCommentNewlineOrEmptyText, scope, E_Direction::BACKWARD, false));
}


Chunk *Chunk::GetPrevNcNnlNi(const E_Scope scope) const
{
   return(Search(&Chunk::IsCommentNewlineOrIgnored, scope, E_Direction::BACKWARD, false));
}


Chunk *Chunk::GetNextNisq(const E_Scope scope) const
{
   return(Search(&Chunk::IsSquareBracket, scope, E_Direction::FORWARD, false));
}


Chunk *Chunk::GetNextType(const E_Token cType, const int cLevel, const E_Scope scope) const
{
   return(SearchTypeLevel(cType, scope, E_Direction::FORWARD, cLevel));
}


Chunk *Chunk::GetPrevType(const E_Token cType, const int cLevel, const E_Scope scope) const
{
   return(SearchTypeLevel(cType, scope, E_Direction::BACKWARD, cLevel));
}


Chunk *Chunk::GetNextString(const char *cStr, const size_t len, const int cLevel, const E_Scope scope) const
{
   return(SearchStringLevel(cStr, len, cLevel, scope, E_Direction::FORWARD));
}


Chunk *Chunk::GetPrevString(const char *cStr, const size_t len, const int cLevel, const E_Scope scope) const
{
   return(SearchStringLevel(cStr, len, cLevel, scope, E_Direction::BACKWARD));
}


bool chunk_is_newline_between(Chunk *start, Chunk *end)
{
   for (Chunk *pc = start; pc != nullptr && pc != end; pc = pc->GetNext())
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
      || pc->IsNullChunk())
   {
      return(Chunk::NullChunkPtr);
   }
   Chunk *first = pc;

   while (  (pc = pc->GetPrev())->IsNotNullChunk()
         && !chunk_is_newline(pc))
   {
      first = pc;
   }
   return(first);
}


bool chunk_is_last_on_line(Chunk *pc)  //TODO: pc should be const here
{
   // check if pc is the very last chunk of the file
   const Chunk *end = Chunk::GetTail();

   if (pc == end)
   {
      return(true);
   }
   // if the next chunk is a newline then pc is the last chunk on its line
   const Chunk *next = pc->GetNext();

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

   if (  pc1->IsNullChunk()
      || pc2->IsNullChunk()
      || pc1 == pc2)
   {
      return;
   }
   /*
    * Example start:
    * ? - start1 - a1 - b1 - nl1 - ? - ref2 - start2 - a2 - b2 - nl2 - ?
    *      ^- pc1                              ^- pc2
    */
   Chunk *ref2 = pc2->GetPrev();

   // Move the line started at pc2 before pc1
   while (  pc2->IsNotNullChunk()
         && !chunk_is_newline(pc2))
   {
      Chunk *tmp = pc2->GetNext();
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
   while (  pc1->IsNotNullChunk()
         && !chunk_is_newline(pc1))
   {
      Chunk *tmp = pc1->GetNext();
      g_cl.Pop(pc1);

      if (ref2->IsNotNullChunk())
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
   if (  pc1->IsNotNullChunk()
      && pc2->IsNotNullChunk())
   {
      size_t nl_count = pc1->nl_count;

      pc1->nl_count = pc2->nl_count;
      pc2->nl_count = nl_count;

      chunk_swap(pc1, pc2);
   }
} // chunk_swap_lines


Chunk *Chunk::GetNextNvb(const E_Scope scope) const
{
   return(Search(&Chunk::IsVBrace, scope, E_Direction::FORWARD, false));
}


Chunk *Chunk::GetPrevNvb(const E_Scope scope) const
{
   return(Search(&Chunk::IsVBrace, scope, E_Direction::BACKWARD, false));
}


void chunk_flags_set_real(Chunk *pc, pcf_flags_t clr_bits, pcf_flags_t set_bits)
{
   if (  pc != nullptr
      && pc->IsNotNullChunk())
   {
      LOG_FUNC_ENTRY();
      auto const nflags = (pc->flags & ~clr_bits) | set_bits;

      if (pc->flags != nflags)
      {
         LOG_FMT(LSETFLG,
                 "%s(%d): %016llx^%016llx=%016llx\n"
                 "   orig_line is %zu, orig_col is %zu, Text() '%s', type is %s,",
                 __func__, __LINE__,
                 static_cast<pcf_flags_t::int_t>(pc->flags),
                 static_cast<pcf_flags_t::int_t>(pc->flags ^ nflags),
                 static_cast<pcf_flags_t::int_t>(nflags),
                 pc->orig_line, pc->orig_col, pc->Text(),
                 get_token_name(pc->type));
         LOG_FMT(LSETFLG, " parent_type is %s,\n  ",
                 get_token_name(get_chunk_parent_type(pc)));
         log_func_stack_inline(LSETFLG);
         pc->flags = nflags;
      }
   }
}


void set_chunk_type_real(Chunk *pc, E_Token token, const char *func, int line)
{
   LOG_FUNC_ENTRY();

   if (  pc == nullptr
      || pc->IsNullChunk()
      || pc->type == token)
   {
      return;
   }
   LOG_FMT(LSETTYP, "%s(%d): orig_line is %zu, orig_col is %zu, pc->Text() ",
           func, line, pc->orig_line, pc->orig_col);

   if (token == CT_NEWLINE)
   {
      LOG_FMT(LSETTYP, "<Newline>\n");
   }
   else
   {
      LOG_FMT(LSETTYP, "'%s'\n", pc->Text());
   }
   LOG_FMT(LSETTYP, "   pc->type is %s, pc->parent_type is %s => *type is %s, *parent_type is %s\n",
           get_token_name(pc->type), get_token_name(get_chunk_parent_type(pc)),
           get_token_name(token), get_token_name(get_chunk_parent_type(pc)));
   pc->type = token;
} // set_chunk_type_real


void set_chunk_parent_real(Chunk *pc, E_Token token, const char *func, int line)
{
   LOG_FUNC_ENTRY();

   if (  pc == nullptr
      || pc->IsNullChunk()
      || get_chunk_parent_type(pc) == token)
   {
      return;
   }
   LOG_FMT(LSETPAR, "%s(%d): orig_line is %zu, orig_col is %zu, pc->Text() ",
           func, line, pc->orig_line, pc->orig_col);

   if (token == CT_NEWLINE)
   {
      LOG_FMT(LSETPAR, "<Newline>\n");
   }
   else
   {
      char copy[1000];
      LOG_FMT(LSETPAR, "'%s'\n", pc->ElidedText(copy));
   }
   LOG_FMT(LSETPAR, "   pc->type is %s, pc->parent_type is %s => *type is %s, *parent_type is %s\n",
           get_token_name(pc->type), get_token_name(get_chunk_parent_type(pc)),
           get_token_name(token), get_token_name(get_chunk_parent_type(pc)));
   pc->parent_type = token;
} // set_chunk_parent_real


E_Token get_chunk_parent_type(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   if (pc == nullptr)
   {
      return(CT_NONE);
   }
   return(pc->parent_type);
} // get_chunk_parent_type


static Chunk *chunk_add(const Chunk *pc_in, Chunk *ref, const E_Direction pos)
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
         (pos == E_Direction::FORWARD) ? g_cl.AddAfter(pc, ref) : g_cl.AddBefore(pc, ref);
      }
      else // ref == NULL
      {
         (pos == E_Direction::FORWARD) ? g_cl.AddHead(pc) : g_cl.AddTail(pc);
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
      cur = cur->GetNextNcNnl();
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
      cur = cur->GetPrevNcNnl();
   }
   return(cur);
}


Chunk *chunk_get_pp_start(Chunk *cur)
{
   if (!cur->IsPreproc())
   {
      return(nullptr);
   }

   while (!chunk_is_token(cur, CT_PREPROC))
   {
      cur = cur->GetPrev(E_Scope::PREPROC);
   }
   return(cur);
}


//! skip to the final word/type in a :: chain
static Chunk *chunk_skip_dc_member(Chunk *start, E_Scope scope, E_Direction dir)
{
   LOG_FUNC_ENTRY();

   if (start == nullptr)
   {
      return(nullptr);
   }
   const auto step_fcn = (dir == E_Direction::FORWARD)
                         ? &Chunk::GetNextNcNnl : &Chunk::GetPrevNcNnl;

   Chunk *pc   = start;
   Chunk *next = chunk_is_token(pc, CT_DC_MEMBER) ? pc : (pc->*step_fcn)(scope);

   while (chunk_is_token(next, CT_DC_MEMBER))
   {
      pc = (next->*step_fcn)(scope);

      if (pc->IsNullChunk())
      {
         return(Chunk::NullChunkPtr);
      }
      next = (pc->*step_fcn)(scope);
   }
   return(pc);
}


Chunk *chunk_skip_dc_member(Chunk *start, E_Scope scope)
{
   return(chunk_skip_dc_member(start, scope, E_Direction::FORWARD));
}


Chunk *chunk_skip_dc_member_rev(Chunk *start, E_Scope scope)
{
   return(chunk_skip_dc_member(start, scope, E_Direction::BACKWARD));
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


E_Token get_type_of_the_parent(Chunk *pc)
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
