/**
 * @file min_after.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/min_after.h"

#include "chunk.h"
#include "mark_change.h"
#include "newlines/can_increase_nl.h"
#include "uncrustify.h"


void newline_min_after(Chunk *ref, size_t count, E_PcfFlag flag)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNEWLINE, "%s(%d): for '%s', at orig line %zu, count is %zu,\n   flag is %s:",
           __func__, __LINE__, ref->Text(), ref->GetOrigLine(), count,
           pcf_flags_str(flag).c_str());
   log_func_stack_inline(LNEWLINE);

   Chunk *pc = ref;

   do
   {
      pc = pc->GetNext();
   } while (  pc->IsNotNullChunk()
           && !pc->IsNewline());

   if (pc->IsNotNullChunk())                 // Coverity CID 76002
   {
      LOG_FMT(LNEWLINE, "%s(%d): type is %s, orig line %zu, orig col %zu\n",
              __func__, __LINE__, get_token_name(pc->GetType()), pc->GetOrigLine(), pc->GetOrigCol());
   }
   Chunk *next = pc->GetNext();

   if (next->IsNullChunk())
   {
      return;
   }

   if (  next->IsComment()
      && next->GetNlCount() == 1
      && pc->GetPrev()->IsComment())
   {
      newline_min_after(next, count, flag);
      return;
   }
   pc->SetFlagBits(flag);

   if (  pc->IsNewline()
      && can_increase_nl(pc))
   {
      if (pc->GetNlCount() < count)
      {
         pc->SetNlCount(count);
         MARK_CHANGE();
      }
   }
} // newline_min_after
