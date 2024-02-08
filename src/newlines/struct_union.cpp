/**
 * @file struct_union.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/struct_union.h"

#include "log_rules.h"
#include "newlines/iarf.h"
#include "options.h"


using namespace uncrustify;


constexpr static auto LCURRENT = LNEWLINE;


void newlines_struct_union(Chunk *start, iarf_e nl_opt, bool leave_trailing)
{
   LOG_FUNC_ENTRY();

   log_rule_B("nl_define_macro");

   if (  nl_opt == IARF_IGNORE
      || (  start->TestFlags(PCF_IN_PREPROC)
         && !options::nl_define_macro()))
   {
      return;
   }
   /*
    * step past any junk between the keyword and the open brace
    * Quit if we hit a semicolon or '=', which are not expected.
    */
   size_t level = start->GetLevel();
   Chunk  *pc   = start->GetNextNcNnl();

   while (  pc->IsNotNullChunk()
         && pc->GetLevel() >= level)
   {
      if (  pc->GetLevel() == level
         && (  pc->Is(CT_BRACE_OPEN)
            || pc->IsSemicolon()
            || pc->Is(CT_ASSIGN)))
      {
         break;
      }
      start = pc;
      pc    = pc->GetNextNcNnl();
   }

   // If we hit a brace open, then we need to toy with the newlines
   if (pc->Is(CT_BRACE_OPEN))
   {
      // Skip over embedded C comments
      Chunk *next = pc->GetNext();

      while (next->Is(CT_COMMENT))
      {
         next = next->GetNext();
      }

      if (  leave_trailing
         && !next->IsCommentOrNewline())
      {
         nl_opt = IARF_IGNORE;
      }
      newline_iarf_pair(start, pc, nl_opt);
   }
} // newlines_struct_union
