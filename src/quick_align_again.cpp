/**
 * @file quick_align_again.cpp
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "quick_align_again.h"

#include "align_stack.h"
#include "chunk_list.h"


void quick_align_again(void)
{
   LOG_FUNC_ENTRY();
   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next(pc))
   {
      LOG_FMT(LALAGAIN, "%s(%d): pc->orig_line is %zu, pc->orig_col is %zu, pc->text() '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
      if (pc->align.next != nullptr && (pc->flags & PCF_ALIGN_START))
      {
         AlignStack as;
         as.Start(100, 0);
         as.m_right_align = pc->align.right_align;
         as.m_star_style  = static_cast<AlignStack::StarStyle>(pc->align.star_style);
         as.m_amp_style   = static_cast<AlignStack::StarStyle>(pc->align.amp_style);
         as.m_gap         = pc->align.gap;

         LOG_FMT(LALAGAIN, "%s(%d):   pc->text() is '%s', orig_line is %zu\n",
                 __func__, __LINE__, pc->text(), pc->orig_line);
         as.Add(pc->align.start);
         chunk_flags_set(pc, PCF_WAS_ALIGNED);
         for (chunk_t *tmp = pc->align.next; tmp != nullptr; tmp = tmp->align.next)
         {
            chunk_flags_set(tmp, PCF_WAS_ALIGNED);
            as.Add(tmp->align.start);
            LOG_FMT(LALAGAIN, "%s(%d):    => tmp->text() is '%s', orig_line is %zu\n",
                    __func__, __LINE__, tmp->text(), tmp->orig_line);
         }
         LOG_FMT(LALAGAIN, "\n");
         as.End();
      }
   }
} // quick_align_again
