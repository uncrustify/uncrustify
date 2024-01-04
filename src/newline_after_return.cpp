/**
 * @file newline_after_return.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "newline_after_return.h"

#include "double_newline.h"


void newline_after_return(Chunk *start)
{
   LOG_FUNC_ENTRY();

   Chunk *semi  = start->GetNextType(CT_SEMICOLON, start->GetLevel());
   Chunk *after = semi->GetNextNcNnlNet();

   // If we hit a brace or an 'else', then a newline isn't needed
   if (  after->IsNullChunk()
      || after->IsBraceClose()
      || after->Is(CT_ELSE))
   {
      return;
   }
   Chunk *pc;

   for (pc = semi->GetNext(); pc != after; pc = pc->GetNext())
   {
      if (pc->Is(CT_NEWLINE))
      {
         if (pc->GetNlCount() < 2)
         {
            double_newline(pc);
         }
         return;
      }
   }
} // newline_after_return
