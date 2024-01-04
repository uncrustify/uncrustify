/**
 * @file newline_add_after.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "newline_add_after.h"

#include "mark_change.h"
#include "setup_newline_add.h"

#define MARK_CHANGE()    mark_change(__func__, __LINE__)


Chunk *newline_add_after(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   if (pc->IsNullChunk())
   {
      return(Chunk::NullChunkPtr);
   }
   Chunk *next = pc->GetNextNvb();

   if (next->IsNewline())
   {
      // Already has a newline after this chunk
      return(next);
   }
   LOG_FMT(LNEWLINE, "%s(%d): '%s' on line %zu",
           __func__, __LINE__, pc->Text(), pc->GetOrigLine());
   log_func_stack_inline(LNEWLINE);

   Chunk nl;

   nl.SetOrigLine(pc->GetOrigLine());
   nl.SetOrigCol(pc->GetOrigCol());
   setup_newline_add(pc, &nl, next);

   MARK_CHANGE();
   // TO DO: check why the next statement is necessary
   nl.SetOrigCol(pc->GetOrigCol());
   nl.SetPpLevel(pc->GetPpLevel());
   return(nl.CopyAndAddAfter(pc));
} // newline_add_after
