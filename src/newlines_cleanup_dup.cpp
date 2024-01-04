/**
 * @file newlines_cleanup_dup.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "newlines_cleanup_dup.h"

#include "chunk.h"
#include "mark_change.h"

#ifdef WIN32
#include <algorithm>                   // to get max
#endif // ifdef WIN32

#define MARK_CHANGE()    mark_change(__func__, __LINE__)


void newlines_cleanup_dup()
{
   LOG_FUNC_ENTRY();

   Chunk *pc   = Chunk::GetHead();
   Chunk *next = pc;

   while (pc->IsNotNullChunk())
   {
      next = next->GetNext();

      if (  pc->Is(CT_NEWLINE)
         && next->Is(CT_NEWLINE))
      {
         next->SetNlCount(max(pc->GetNlCount(), next->GetNlCount()));
         Chunk::Delete(pc);
         MARK_CHANGE();
      }
      pc = next;
   }
} // newlines_cleanup_dup
