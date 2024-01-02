/**
 * @file blank_line_set.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "blank_line_set.h"

#include "chunk.h"
#include "mark_change.h"

#define MARK_CHANGE()    mark_change(__func__, __LINE__)


void blank_line_set(Chunk *pc, Option<unsigned> &opt)
{
   LOG_FUNC_ENTRY();

   if (pc->IsNullChunk())
   {
      return;
   }
   const unsigned optval = opt();

   if (  (optval > 0)
      && (pc->GetNlCount() != optval))
   {
      LOG_FMT(LBLANKD, "%s(%d): do_blank_lines: %s set line %zu to %u\n",
              __func__, __LINE__, opt.name(), pc->GetOrigLine(), optval);
      pc->SetNlCount(optval);
      MARK_CHANGE();
   }
} // blank_line_set
