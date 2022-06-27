/**
 * @file flag_braced_init_list.cpp
 *
 * @license GPL v2+
 */

#include "chunk.h"

#include "flag_braced_init_list.h"

#include "uncrustify.h"


bool detect_cpp_braced_init_list(Chunk *pc, Chunk *next)
{
   LOG_FUNC_ENTRY();
   // Issue #2332
   bool we_have_a_case_before = false;

   if (pc->Is(CT_COLON))
   {
      // check if we have a case before
      Chunk *switch_before = pc->GetPrevType(CT_CASE, pc->level);

      if (switch_before->IsNotNullChunk())
      {
         LOG_FMT(LFCNR, "%s(%d): switch_before->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                 __func__, __LINE__, switch_before->orig_line, switch_before->orig_col,
                 switch_before->Text(), get_token_name(switch_before->type));
         we_have_a_case_before = true;
      }
   }

   // Detect a braced-init-list
   if (  pc->Is(CT_WORD)
      || pc->Is(CT_TYPE)
      || pc->Is(CT_ASSIGN)
      || pc->Is(CT_RETURN)
      || pc->Is(CT_COMMA)
      || pc->Is(CT_ANGLE_CLOSE)
      || pc->Is(CT_SQUARE_CLOSE)
      || pc->Is(CT_TSQUARE)
      || pc->Is(CT_FPAREN_OPEN)
      || pc->Is(CT_QUESTION)
      || (  pc->Is(CT_COLON)
         && !we_have_a_case_before)
      || (  pc->Is(CT_BRACE_OPEN)
         && (  get_chunk_parent_type(pc) == CT_NONE
            || get_chunk_parent_type(pc) == CT_BRACED_INIT_LIST)))
   {
      LOG_FMT(LFCNR, "%s(%d): orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n   ",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
      log_pcf_flags(LFCNR, pc->flags);
      auto brace_open = pc->GetNextNcNnl();

      if (  brace_open->Is(CT_BRACE_OPEN)
         && (  get_chunk_parent_type(brace_open) == CT_NONE
            || get_chunk_parent_type(brace_open) == CT_ASSIGN
            || get_chunk_parent_type(brace_open) == CT_RETURN
            || get_chunk_parent_type(brace_open) == CT_BRACED_INIT_LIST))
      {
         log_pcf_flags(LFCNR, brace_open->flags);
         auto brace_close = next->SkipToMatch();

         if (brace_close->Is(CT_BRACE_CLOSE))
         {
            return(true);
         }
      }
   }
   return(false);
} // detect_cpp_braced_init_list


void flag_cpp_braced_init_list(Chunk *pc, Chunk *next)
{
   auto brace_open  = pc->GetNextNcNnl();
   auto brace_close = next->SkipToMatch();

   set_chunk_parent(brace_open, CT_BRACED_INIT_LIST);
   set_chunk_parent(brace_close, CT_BRACED_INIT_LIST);

   auto *tmp = brace_close->GetNextNcNnl();

   if (tmp->IsNotNullChunk())
   {
      chunk_flags_clr(tmp, PCF_EXPR_START | PCF_STMT_START);

      // Flag call operator
      if (tmp->Is(CT_PAREN_OPEN))
      {
         if (auto *const c = tmp->SkipToMatch())
         {
            set_chunk_type(tmp, CT_FPAREN_OPEN);
            set_chunk_parent(tmp, CT_FUNC_CALL);
            set_chunk_type(c, CT_FPAREN_CLOSE);
            set_chunk_parent(c, CT_FUNC_CALL);
         }
      }
   }
   // TODO: Change pc->type CT_WORD -> CT_TYPE
   // for the case CT_ASSIGN (and others).

   // TODO: Move this block to the fix_fcn_call_args function.
   if (  pc->Is(CT_WORD)
      && pc->flags.test(PCF_IN_FCN_CALL))
   {
      set_chunk_type(pc, CT_TYPE);
   }
} // flag_cpp_braced_init_list
