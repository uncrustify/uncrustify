/**
 * @file blank_line_max.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "blank_line_max.h"

#include "mark_change.h"

#define MARK_CHANGE()    mark_change(__func__, __LINE__)


void blank_line_max(Chunk *pc, Option<unsigned> &opt)
{
   LOG_FUNC_ENTRY();

   if (pc == nullptr)
   {
      return;
   }
   const auto optval = opt();

   if (  (optval > 0)
      && (pc->GetNlCount() > optval))
   {
      LOG_FMT(LBLANKD, "%s(%d): do_blank_lines: %s max line %zu\n",
              __func__, __LINE__, opt.name(), pc->GetOrigLine());
      pc->SetNlCount(optval);
      MARK_CHANGE();
   }
} // blank_line_max
