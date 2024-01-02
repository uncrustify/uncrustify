/**
 * @file newline_func_multi_line.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "newline_func_multi_line.h"

#include "log_rules.h"
#include "newline_iarf.h"
#include "uncrustify.h"
constexpr static auto LCURRENT = LNEWLINE;


/**
 * Adds newlines to multi-line function call/decl/def
 * Start points to the open paren
 */
void newline_func_multi_line(Chunk *start)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNFD, "%s(%d): called on %zu:%zu '%s' [%s/%s]\n",
           __func__, __LINE__, start->GetOrigLine(), start->GetOrigCol(),
           start->Text(), get_token_name(start->GetType()), get_token_name(start->GetParentType()));

   bool add_start;
   bool add_args;
   bool add_end;

   if (  start->GetParentType() == CT_FUNC_DEF
      || start->GetParentType() == CT_FUNC_CLASS_DEF)
   {
      log_rule_B("nl_func_def_start_multi_line");
      add_start = options::nl_func_def_start_multi_line();
      log_rule_B("nl_func_def_args_multi_line");
      add_args = options::nl_func_def_args_multi_line();
      log_rule_B("nl_func_def_end_multi_line");
      add_end = options::nl_func_def_end_multi_line();
   }
   else if (  start->GetParentType() == CT_FUNC_CALL
           || start->GetParentType() == CT_FUNC_CALL_USER)
   {
      log_rule_B("nl_func_call_start_multi_line");
      add_start = options::nl_func_call_start_multi_line();
      log_rule_B("nl_func_call_args_multi_line");
      add_args = options::nl_func_call_args_multi_line();
      log_rule_B("nl_func_call_end_multi_line");
      add_end = options::nl_func_call_end_multi_line();
   }
   else
   {
      log_rule_B("nl_func_decl_start_multi_line");
      add_start = options::nl_func_decl_start_multi_line();
      log_rule_B("nl_func_decl_args_multi_line");
      add_args = options::nl_func_decl_args_multi_line();
      log_rule_B("nl_func_decl_end_multi_line");
      add_end = options::nl_func_decl_end_multi_line();
   }

   if (  !add_start
      && !add_args
      && !add_end)
   {
      return;
   }
   Chunk *pc = start->GetNextNcNnl();

   while (  pc->IsNotNullChunk()
         && pc->GetLevel() > start->GetLevel())
   {
      pc = pc->GetNextNcNnl();
   }

   if (  pc->Is(CT_FPAREN_CLOSE)
      && start->IsNewlineBetween(pc))
   {
      Chunk *start_next         = start->GetNextNcNnl();
      bool  has_leading_closure = (  start_next->GetParentType() == CT_OC_BLOCK_EXPR
                                  || start_next->GetParentType() == CT_CPP_LAMBDA
                                  || start_next->Is(CT_BRACE_OPEN));

      Chunk *prev_end            = pc->GetPrevNcNnl();
      bool  has_trailing_closure = (  prev_end->GetParentType() == CT_OC_BLOCK_EXPR
                                   || prev_end->GetParentType() == CT_CPP_LAMBDA
                                   || prev_end->Is(CT_BRACE_OPEN));

      if (  add_start
         && !start->GetNext()->IsNewline())
      {
         log_rule_B("nl_func_call_args_multi_line_ignore_closures");

         if (options::nl_func_call_args_multi_line_ignore_closures())
         {
            if (  !has_leading_closure
               && !has_trailing_closure)
            {
               newline_iarf(start, IARF_ADD);
            }
         }
         else
         {
            newline_iarf(start, IARF_ADD);
         }
      }

      if (  add_end
         && !pc->GetPrev()->IsNewline())
      {
         log_rule_B("nl_func_call_args_multi_line_ignore_closures");

         if (options::nl_func_call_args_multi_line_ignore_closures())
         {
            if (  !has_leading_closure
               && !has_trailing_closure)
            {
               newline_iarf(pc->GetPrev(), IARF_ADD);
            }
         }
         else
         {
            newline_iarf(pc->GetPrev(), IARF_ADD);
         }
      }

      if (add_args)
      {
         // process the function in reverse and leave the first comma if the option to leave trailing closure
         // is on. nl_func_call_args_multi_line_ignore_trailing_closure
         for (pc = start->GetNextNcNnl();
              pc->IsNotNullChunk() && pc->GetLevel() > start->GetLevel();
              pc = pc->GetNextNcNnl())
         {
            if (  pc->Is(CT_COMMA)
               && (pc->GetLevel() == (start->GetLevel() + 1)))
            {
               Chunk *tmp = pc->GetNext();

               if (tmp->IsComment())
               {
                  pc = tmp;
               }

               if (!pc->GetNext()->IsNewline())
               {
                  log_rule_B("nl_func_call_args_multi_line_ignore_closures");

                  if (options::nl_func_call_args_multi_line_ignore_closures())
                  {
                     Chunk *prev_comma  = pc->GetPrevNcNnl();
                     Chunk *after_comma = pc->GetNextNcNnl();

                     if (!(  (  prev_comma->GetParentType() == CT_OC_BLOCK_EXPR
                             || prev_comma->GetParentType() == CT_CPP_LAMBDA
                             || prev_comma->Is(CT_BRACE_OPEN))
                          || (  after_comma->GetParentType() == CT_OC_BLOCK_EXPR
                             || after_comma->GetParentType() == CT_CPP_LAMBDA
                             || after_comma->Is(CT_BRACE_OPEN))))
                     {
                        newline_iarf(pc, IARF_ADD);
                     }
                  }
                  else
                  {
                     newline_iarf(pc, IARF_ADD);
                  }
               }
            }
         }
      }
   }
} // newline_func_multi_line
