/**
 * @file newlines_remove_disallowed.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "newlines_remove_disallowed.h"

#include "can_increase_nl.h"
#include "chunk.h"
#include "mark_change.h"

#define MARK_CHANGE()    mark_change(__func__, __LINE__)


void newlines_remove_disallowed()
{
   LOG_FUNC_ENTRY();

   Chunk *pc = Chunk::GetHead();
   Chunk *next;

   while ((pc = pc->GetNextNl())->IsNotNullChunk())
   {
      LOG_FMT(LBLANKD, "%s(%d): orig line is %zu, orig col is %zu, <Newline>, nl is %zu\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetNlCount());

      next = pc->GetNext();

      if (  next->IsNotNullChunk()
         && !next->Is(CT_NEWLINE)
         && !can_increase_nl(pc))
      {
         LOG_FMT(LBLANKD, "%s(%d): force to 1 orig line is %zu, orig col is %zu\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol());

         if (pc->GetNlCount() != 1)
         {
            pc->SetNlCount(1);
            MARK_CHANGE();
         }
      }
   }
} // newlines_remove_disallowed
