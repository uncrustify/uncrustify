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

   if (pc->Is(E_Token::CT_COLON))
   {
      // check if we have a case before
      Chunk *switch_before = pc->GetPrevType(E_Token::CT_CASE, pc->GetLevel());

      if (switch_before->IsNotNullChunk())
      {
         LOG_FMT(LFCNR, "%s(%d): switch_before orig line is %zu, orig col is %zu, text is '%s', type is %s\n",
                 __func__, __LINE__, switch_before->GetOrigLine(), switch_before->GetOrigCol(),
                 switch_before->GetLogText(), get_token_name(switch_before->GetType()));
         we_have_a_case_before = true;
      }
   }

   // Detect a braced-init-list
   if (  pc->Is(E_Token::CT_WORD)
      || pc->Is(E_Token::CT_TYPE)
      || pc->Is(E_Token::CT_ASSIGN)
      || pc->Is(E_Token::CT_RETURN)
      || pc->Is(E_Token::CT_COMMA)
      || pc->Is(E_Token::CT_ANGLE_CLOSE)
      || pc->Is(E_Token::CT_SQUARE_CLOSE)
      || pc->Is(E_Token::CT_TSQUARE)
      || pc->Is(E_Token::CT_FPAREN_OPEN)
      || pc->Is(E_Token::CT_QUESTION)
      || (  pc->Is(E_Token::CT_COLON)
         && !we_have_a_case_before)
      || (  pc->Is(E_Token::CT_BRACE_OPEN)
         && (  pc->GetParentType() == E_Token::CT_NONE
            || pc->GetParentType() == E_Token::CT_BRACED_INIT_LIST)))
   {
      LOG_FMT(LFCNR, "%s(%d): orig line is %zu, orig col is %zu, text is '%s', type is %s\n   ",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLogText(), get_token_name(pc->GetType()));
      log_pcf_flags(LFCNR, pc->GetFlags());
      auto brace_open = pc->GetNextNcNnl();

      if (  brace_open->Is(E_Token::CT_BRACE_OPEN)
         && (  brace_open->GetParentType() == E_Token::CT_NONE
            || brace_open->GetParentType() == E_Token::CT_ASSIGN
            || brace_open->GetParentType() == E_Token::CT_RETURN
            || brace_open->GetParentType() == E_Token::CT_BRACED_INIT_LIST))
      {
         log_pcf_flags(LFCNR, brace_open->GetFlags());
         auto brace_close = next->GetClosingParen();

         if (brace_close->Is(E_Token::CT_BRACE_CLOSE))
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

   brace_open->SetParentType(E_Token::CT_BRACED_INIT_LIST);
   brace_close->SetParentType(E_Token::CT_BRACED_INIT_LIST);

   Chunk *tmp = brace_close->GetNextNcNnl();

   if (tmp->IsNotNullChunk())
   {
      tmp->ResetFlagBits(PCF_EXPR_START | PCF_STMT_START);

      // Flag call operator
      if (tmp->Is(E_Token::CT_PAREN_OPEN))
      {
         Chunk *c = tmp->GetClosingParen();

         if (c->IsNotNullChunk())
         {
            tmp->SetType(E_Token::CT_FPAREN_OPEN);
            tmp->SetParentType(E_Token::CT_FUNC_CALL);
            c->SetType(E_Token::CT_FPAREN_CLOSE);
            c->SetParentType(E_Token::CT_FUNC_CALL);
         }
      }
   }
   // TODO: Change pc->GetType() E_Token::CT_WORD -> E_Token::CT_TYPE
   // for the case E_Token::CT_ASSIGN (and others).

   // TODO: Move this block to the fix_fcn_call_args function.
   if (  pc->Is(E_Token::CT_WORD)
      && pc->TestFlags(PCF_IN_FCN_CALL))
   {
      pc->SetType(E_Token::CT_TYPE);
   }
} // flag_cpp_braced_init_list
