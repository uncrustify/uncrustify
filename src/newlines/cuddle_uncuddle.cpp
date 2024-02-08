/**
 * @file cuddle_uncuddle.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/cuddle_uncuddle.h"

#include "log_rules.h"
#include "newlines/iarf.h"
#include "options.h"


using namespace uncrustify;


constexpr static auto LCURRENT = LNEWLINE;


void newlines_cuddle_uncuddle(Chunk *start, iarf_e nl_opt)
{
   LOG_FUNC_ENTRY();

   log_rule_B("nl_define_macro");

   if (  start->TestFlags(PCF_IN_PREPROC)
      && !options::nl_define_macro())
   {
      return;
   }
   Chunk *br_close = start->GetPrevNcNnlNi();   // Issue #2279

   if (br_close->Is(CT_BRACE_CLOSE))
   {
      newline_iarf_pair(br_close, start, nl_opt);
   }
} // newlines_cuddle_uncuddle
