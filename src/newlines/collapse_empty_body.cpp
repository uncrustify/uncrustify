/**
 * @file collapse_empty_body.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/collapse_empty_body.h"

#include "chunk.h"
#include "mark_change.h"


void collapse_empty_body(Chunk *br_open)
{
   for (Chunk *pc = br_open->GetNext(); pc->IsNot(CT_BRACE_CLOSE); pc = pc->GetNext())
   {
      if (  pc->Is(CT_NEWLINE)
         && pc->SafeToDeleteNl())
      {
         pc = pc->GetPrev();
         Chunk *next = pc->GetNext();
         Chunk::Delete(next);
         MARK_CHANGE();
      }
   }
} // collapse_empty_body
