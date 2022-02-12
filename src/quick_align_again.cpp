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
#include "chunk.h"


void quick_align_again(void)
{
   LOG_FUNC_ENTRY();

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      char copy[1000];
      LOG_FMT(LALAGAIN, "%s(%d): orig_line is %zu, orig_col is %zu, column is %zu, Text() '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->column, pc->ElidedText(copy));

      if (  pc->align.next != nullptr
         && pc->flags.test(PCF_ALIGN_START))
      {
         AlignStack as;
         as.Start(100, 0);
         as.m_right_align = pc->align.right_align;
         as.m_star_style  = static_cast<AlignStack::StarStyle>(pc->align.star_style);
         as.m_amp_style   = static_cast<AlignStack::StarStyle>(pc->align.amp_style);
         as.m_gap         = pc->align.gap;

         LOG_FMT(LALAGAIN, "%s(%d):   pc->Text() is '%s', orig_line is %zu\n",
                 __func__, __LINE__, pc->Text(), pc->orig_line);
         as.Add(pc->align.start);
         chunk_flags_set(pc, PCF_WAS_ALIGNED);

         for (Chunk *tmp = pc->align.next; tmp != nullptr; tmp = tmp->align.next)
         {
            chunk_flags_set(tmp, PCF_WAS_ALIGNED);
            as.Add(tmp->align.start);
            LOG_FMT(LALAGAIN, "%s(%d):    => tmp->Text() is '%s', orig_line is %zu\n",
                    __func__, __LINE__, tmp->Text(), tmp->orig_line);
         }

         LOG_FMT(LALAGAIN, "\n");
         as.End();
      }
   }
} // quick_align_again
