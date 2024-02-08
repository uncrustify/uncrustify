/**
 * @file functions_remove_extra_blank_lines.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/functions_remove_extra_blank_lines.h"

#include "chunk.h"
#include "log_rules.h"
#include "mark_change.h"
#include "newlines/remove_next_newlines.h"


constexpr static auto LCURRENT = LNEWLINE;


using namespace uncrustify;


void newlines_functions_remove_extra_blank_lines()
{
   LOG_FUNC_ENTRY();

   const size_t nl_max_blank_in_func = options::nl_max_blank_in_func();

   log_rule_B("nl_max_blank_in_func");

   if (nl_max_blank_in_func == 0)
   {
      LOG_FMT(LNEWLINE, "%s(%d): nl_max_blank_in_func is zero\n", __func__, __LINE__);
      return;
   }

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      LOG_FMT(LNEWLINE, "%s(%d): orig line is %zu, orig col is %zu, Text() '%s', type is %s\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), get_token_name(pc->GetType()));

      if (  pc->IsNot(CT_BRACE_OPEN)
         || (  pc->GetParentType() != CT_FUNC_DEF
            && pc->GetParentType() != CT_CPP_LAMBDA))
      {
         continue;
      }
      const size_t startMoveLevel = pc->GetLevel();

      while (pc->IsNotNullChunk())
      {
         if (  pc->Is(CT_BRACE_CLOSE)
            && pc->GetLevel() == startMoveLevel)
         {
            break;
         }

         // delete newlines
         if (  !pc->Is(CT_COMMENT_MULTI)   // Issue #2195
            && pc->GetNlCount() > nl_max_blank_in_func)
         {
            LOG_FMT(LNEWLINE, "%s(%d): orig line is %zu, orig col is %zu, Text() '%s', type is %s\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), get_token_name(pc->GetType()));
            pc->SetNlCount(nl_max_blank_in_func);
            MARK_CHANGE();
            remove_next_newlines(pc);
         }
         else
         {
            pc = pc->GetNext();
         }
      }
   }
} // newlines_functions_remove_extra_blank_lines
