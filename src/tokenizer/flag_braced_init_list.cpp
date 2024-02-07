/**
 * @file flag_braced_init_list.cpp
 *
 * @license GPL v2+
 */

#include "chunk.h"

#include "tokenizer/flag_braced_init_list.h"

#include "uncrustify.h"


bool detect_cpp_braced_init_list(Chunk *pc, Chunk *next)
{
   LOG_FUNC_ENTRY();
   // Issue #2332
   bool we_have_a_case_before = false;

   if (pc->Is(CT_COLON))
   {
      // check if we have a case before
      Chunk *switch_before = pc->GetPrevType(CT_CASE, pc->GetLevel());

      if (switch_before->IsNotNullChunk())
      {
         LOG_FMT(LFCNR, "%s(%d): switch_before orig line is %zu, orig col is %zu, Text() is '%s', type is %s\n",
                 __func__, __LINE__, switch_before->GetOrigLine(), switch_before->GetOrigCol(),
                 switch_before->Text(), get_token_name(switch_before->GetType()));
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
         && (  pc->GetParentType() == CT_NONE
            || pc->GetParentType() == CT_BRACED_INIT_LIST)))
   {
      LOG_FMT(LFCNR, "%s(%d): orig line is %zu, orig col is %zu, Text() is '%s', type is %s\n   ",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), get_token_name(pc->GetType()));
      log_pcf_flags(LFCNR, pc->GetFlags());
      auto brace_open = pc->GetNextNcNnl();

      if (  brace_open->Is(CT_BRACE_OPEN)
         && (  brace_open->GetParentType() == CT_NONE
            || brace_open->GetParentType() == CT_ASSIGN
            || brace_open->GetParentType() == CT_RETURN
            || brace_open->GetParentType() == CT_BRACED_INIT_LIST))
      {
         log_pcf_flags(LFCNR, brace_open->GetFlags());
         auto brace_close = next->GetClosingParen();

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
   Chunk *brace_open  = pc->GetNextNcNnl();
   Chunk *brace_close = next->GetClosingParen();

   brace_open->SetParentType(CT_BRACED_INIT_LIST);
   brace_close->SetParentType(CT_BRACED_INIT_LIST);

   Chunk *tmp = brace_close->GetNextNcNnl();

   if (tmp->IsNotNullChunk())
   {
      tmp->ResetFlagBits(PCF_EXPR_START | PCF_STMT_START);

      // Flag call operator
      if (tmp->Is(CT_PAREN_OPEN))
      {
         Chunk *c = tmp->GetClosingParen();

         if (c->IsNotNullChunk())
         {
            tmp->SetType(CT_FPAREN_OPEN);
            tmp->SetParentType(CT_FUNC_CALL);
            c->SetType(CT_FPAREN_CLOSE);
            c->SetParentType(CT_FUNC_CALL);
         }
      }
   }
   // TODO: Change pc->GetType() CT_WORD -> CT_TYPE
   // for the case CT_ASSIGN (and others).

   // TODO: Move this block to the fix_fcn_call_args function.
   if (  pc->Is(CT_WORD)
      && pc->TestFlags(PCF_IN_FCN_CALL))
   {
      pc->SetType(CT_TYPE);
   }
} // flag_cpp_braced_init_list
