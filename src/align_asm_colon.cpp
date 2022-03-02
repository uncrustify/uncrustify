/**
 * @file align_asm_colon.cpp
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align_asm_colon.h"

#include "align_stack.h"
#include "chunk.h"


void align_asm_colon(void)
{
   LOG_FUNC_ENTRY();

   bool       did_nl;
   AlignStack cas; // for the colons

   cas.Start(4);

   Chunk *pc = Chunk::GetHead();

   while (pc->IsNotNullChunk())
   {
      if (chunk_is_not_token(pc, CT_ASM_COLON))
      {
         pc = pc->GetNext();
         continue;
      }
      cas.Reset();

      pc = pc->GetNextNcNnl(E_Scope::PREPROC);
      size_t level = pc ? pc->level : 0;
      did_nl = true;

      while (  pc->IsNotNullChunk()
            && pc->level >= level)
      {
         if (chunk_is_newline(pc))
         {
            cas.NewLines(pc->nl_count);
            did_nl = true;
         }
         else if (chunk_is_token(pc, CT_ASM_COLON))
         {
            cas.Flush();
            did_nl = true;
         }
         else if (did_nl)
         {
            did_nl = false;
            cas.Add(pc);
         }
         pc = pc->GetNextNc(E_Scope::PREPROC);
      }
      cas.End();
   }
} // align_asm_colon
