/**
 * @file newline_after_label_colon.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "newline_after_label_colon.h"

#include "chunk.h"
#include "newline_add_after.h"


void newline_after_label_colon()
{
   LOG_FUNC_ENTRY();

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (pc->IsNot(CT_LABEL_COLON))
      {
         continue;
      }
      newline_add_after(pc);
   }
} // newline_after_label_colon
