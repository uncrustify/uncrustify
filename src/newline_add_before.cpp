/**
 * @file newline_add_before.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "newline_add_before.h"

#include "mark_change.h"
#include "setup_newline_add.h"

#define MARK_CHANGE()    mark_change(__func__, __LINE__)


Chunk *newline_add_before(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   Chunk nl;
   Chunk *prev = pc->GetPrevNvb();

   if (prev->IsNewline())
   {
      // Already has a newline before this chunk
      return(prev);
   }
   LOG_FMT(LNEWLINE, "%s(%d): Text() '%s', on orig line is %zu, orig col is %zu, pc column is %zu",
           __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol(), pc->GetColumn());
   log_func_stack_inline(LNEWLINE);

   setup_newline_add(prev, &nl, pc);
   nl.SetOrigCol(pc->GetOrigCol());
   nl.SetPpLevel(pc->GetPpLevel());
   LOG_FMT(LNEWLINE, "%s(%d): nl column is %zu\n",
           __func__, __LINE__, nl.GetColumn());

   MARK_CHANGE();
   return(nl.CopyAndAddBefore(pc));
} // newline_add_before
