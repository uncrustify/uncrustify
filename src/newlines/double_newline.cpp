/**
 * @file double_newline.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "newlines/double_newline.h"

#include "can_increase_nl.h"
#include "mark_change.h"

#define MARK_CHANGE()    mark_change(__func__, __LINE__)


void double_newline(Chunk *nl)
{
   LOG_FUNC_ENTRY();

   Chunk *prev = nl->GetPrev();

   if (prev->IsNullChunk())
   {
      return;
   }
   LOG_FMT(LNEWLINE, "%s(%d): add newline after ", __func__, __LINE__);

   if (prev->Is(CT_VBRACE_CLOSE))
   {
      LOG_FMT(LNEWLINE, "VBRACE_CLOSE ");
   }
   else
   {
      LOG_FMT(LNEWLINE, "'%s' ", prev->Text());
   }
   LOG_FMT(LNEWLINE, "on line %zu", prev->GetOrigLine());

   if (!can_increase_nl(nl))
   {
      LOG_FMT(LNEWLINE, " - denied\n");
      return;
   }
   LOG_FMT(LNEWLINE, " - done\n");

   if (nl->GetNlCount() != 2)
   {
      nl->SetNlCount(2);
      MARK_CHANGE();
   }
} // double_newline
