/**
 * @file newlines_do_else.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines_do_else.h"

#include "log_rules.h"
#include "newline_add_after.h"
#include "newline_add_between.h"
#include "newline_iarf_pair.h"
#include "one_liner_nl_ok.h"

constexpr static auto LCURRENT = LNEWLINE;


void newlines_do_else(Chunk *start, iarf_e nl_opt)
{
   LOG_FUNC_ENTRY();

   log_rule_B("nl_define_macro");
   log_ruleNL("nl_define_macro", start);

   if (  nl_opt == IARF_IGNORE
      || (  start->TestFlags(PCF_IN_PREPROC)
         && !options::nl_define_macro()))
   {
      return;
   }
   Chunk *next = start->GetNextNcNnl();

   if (  next->IsNotNullChunk()
      && (  next->Is(CT_BRACE_OPEN)
         || next->Is(CT_VBRACE_OPEN)))
   {
      if (!one_liner_nl_ok(next))
      {
         LOG_FMT(LNL1LINE, "%s(%d): a new line may NOT be added\n", __func__, __LINE__);
         return;
      }
      LOG_FMT(LNL1LINE, "%s(%d): a new line may be added\n", __func__, __LINE__);

      if (next->Is(CT_VBRACE_OPEN))
      {
         // Can only add - we don't want to create a one-line here
         if (nl_opt & IARF_ADD)
         {
            newline_iarf_pair(start, next->GetNextNcNnl(), nl_opt);
            Chunk *tmp = next->GetNextType(CT_VBRACE_CLOSE, next->GetLevel());

            if (  !tmp->GetNextNc()->IsNewline()
               && !tmp->GetPrevNc()->IsNewline())
            {
               newline_add_after(tmp);
            }
         }
      }
      else
      {
         newline_iarf_pair(start, next, nl_opt);
         newline_add_between(next, next->GetNextNcNnl());
      }
   }
} // newlines_do_else
