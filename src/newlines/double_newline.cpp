/**
 * @file double_newline.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#include "newlines/double_newline.h"

#include "chunk.h"
#include "mark_change.h"
#include "newlines/can_increase_nl.h"


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
      LOG_FMT(LNEWLINE, "VBRACE_CLOSE \n");
   }
   else
   {
      LOG_FMT(LNEWLINE, "'%s' \n", prev->Text());
   }
   LOG_FMT(LNEWLINE, "on line %zu", prev->GetOrigLine());

   LOG_FMT(LNEWLINE, "%s(%d): ", __func__, __LINE__);

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
