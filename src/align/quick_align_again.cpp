/**
 * @file quick_align_again.cpp
 *
 * @author  Guy Maurel
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "align/quick_align_again.h"

#include "align/stack.h"
#include "chunk.h"


void quick_align_again()
{
   LOG_FUNC_ENTRY();

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      char copy[1000];
      LOG_FMT(LALAGAIN, "%s(%d): orig line is %zu, orig col is %zu, column is %zu, Text() '%s'\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetColumn(), pc->ElidedText(copy));

      if (  pc->GetAlignData().next->IsNotNullChunk()
         && pc->TestFlags(PCF_ALIGN_START))
      {
         AlignStack as;
         as.Start(100, 0);
         as.m_right_align = pc->GetAlignData().right_align;
         as.m_star_style  = static_cast<AlignStack::StarStyle>(pc->GetAlignData().star_style);
         as.m_amp_style   = static_cast<AlignStack::StarStyle>(pc->GetAlignData().amp_style);
         as.m_gap         = pc->GetAlignData().gap;

         LOG_FMT(LALAGAIN, "%s(%d):   pc->Text() is '%s', orig line is %zu\n",
                 __func__, __LINE__, pc->Text(), pc->GetOrigLine());
         as.Add(pc->GetAlignData().start);
         pc->SetFlagBits(PCF_WAS_ALIGNED);

         for (Chunk *tmp = pc->GetAlignData().next; tmp->IsNotNullChunk(); tmp = tmp->GetAlignData().next)
         {
            tmp->SetFlagBits(PCF_WAS_ALIGNED);
            as.Add(tmp->GetAlignData().start);
            LOG_FMT(LALAGAIN, "%s(%d):    => tmp->Text() is '%s', orig line is %zu\n",
                    __func__, __LINE__, tmp->Text(), tmp->GetOrigLine());
         }

         LOG_FMT(LALAGAIN, "\n");
         as.End();
      }
   }
} // quick_align_again
