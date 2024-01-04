/**
 * @file newlines_remove_newlines.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "newlines_remove_newlines.h"

#include "chunk.h"
#include "newline_iarf.h"

using namespace uncrustify;


void newlines_remove_newlines()
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LBLANK, "%s(%d):\n", __func__, __LINE__);
   Chunk *pc = Chunk::GetHead();

   if (!pc->IsNewline())
   {
      pc = pc->GetNextNl();
   }
   Chunk *next;
   Chunk *prev;

   while (pc->IsNotNullChunk())
   {
      // Remove all newlines not in preproc
      if (!pc->TestFlags(PCF_IN_PREPROC))
      {
         next = pc->GetNext();
         prev = pc->GetPrev();
         newline_iarf(pc, IARF_REMOVE);

         if (next == Chunk::GetHead())
         {
            pc = next;
            continue;
         }
         else if (  prev->IsNotNullChunk()
                 && !prev->GetNext()->IsNewline())
         {
            pc = prev;
         }
      }
      pc = pc->GetNextNl();
   }
} // newlines_remove_newlines
