/**
 * @file newlines_if_for_while_switch.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "newlines_if_for_while_switch.h"

#include "log_rules.h"
#include "newline_add_after.h"
#include "newline_add_between.h"
#include "newline_iarf_pair.h"
#include "one_liner_nl_ok.h"

constexpr static auto LCURRENT = LNEWLINE;


bool newlines_if_for_while_switch(Chunk *start, iarf_e nl_opt)
{
   LOG_FUNC_ENTRY();

   log_rule_B("nl_define_macro");

   if (  nl_opt == IARF_IGNORE
      || (  start->TestFlags(PCF_IN_PREPROC)
         && !options::nl_define_macro()))
   {
      return(false);
   }
   bool  retval = false;
   Chunk *pc    = start->GetNextNcNnl();

   if (pc->Is(CT_SPAREN_OPEN))
   {
      Chunk *close_paren = pc->GetNextType(CT_SPAREN_CLOSE, pc->GetLevel());
      Chunk *brace_open  = close_paren->GetNextNcNnl();

      if (  (  brace_open->Is(CT_BRACE_OPEN)
            || brace_open->Is(CT_VBRACE_OPEN))
         && one_liner_nl_ok(brace_open))
      {
         log_rule_B("nl_multi_line_cond");

         if (options::nl_multi_line_cond())
         {
            while ((pc = pc->GetNext()) != close_paren)
            {
               if (pc->IsNewline())
               {
                  nl_opt = IARF_ADD;
                  break;
               }
            }
         }

         if (brace_open->Is(CT_VBRACE_OPEN))
         {
            // Can only add - we don't want to create a one-line here
            if (nl_opt & IARF_ADD)
            {
               newline_iarf_pair(close_paren, brace_open->GetNextNcNnl(), nl_opt);
               pc = brace_open->GetNextType(CT_VBRACE_CLOSE, brace_open->GetLevel());

               if (  !pc->GetPrevNc()->IsNewline()
                  && !pc->GetNextNc()->IsNewline())
               {
                  newline_add_after(pc);
                  retval = true;
               }
            }
         }
         else
         {
            newline_iarf_pair(close_paren, brace_open, nl_opt);
            Chunk *next = brace_open->GetNextNcNnl();

            if (brace_open->GetType() != next->GetType())                       // Issue #2836
            {
               newline_add_between(brace_open, brace_open->GetNextNcNnl());
            }
            // Make sure nothing is cuddled with the closing brace
            pc = brace_open->GetNextType(CT_BRACE_CLOSE, brace_open->GetLevel());
            newline_add_between(pc, pc->GetNextNcNnlNet());
            retval = true;
         }
      }
   }
   return(retval);
} // newlines_if_for_while_switch
