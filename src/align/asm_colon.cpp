/**
 * @file asm_colon.cpp
 *
 * @author  Guy Maurel
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align/asm_colon.h"

#include "align/stack.h"
#include "chunk.h"


void align_asm_colon()
{
   LOG_FUNC_ENTRY();

   bool       did_nl;
   AlignStack cas; // for the colons

   cas.Start(4);

   Chunk *pc = Chunk::GetHead();

   while (pc->IsNotNullChunk())
   {
      if (pc->IsNot(CT_ASM_COLON))
      {
         pc = pc->GetNext();
         continue;
      }
      cas.Reset();

      pc = pc->GetNextNcNnl(E_Scope::PREPROC);
      size_t level = pc->IsNotNullChunk() ? pc->GetLevel() : 0;
      did_nl = true;

      while (  pc->IsNotNullChunk()
            && pc->GetLevel() >= level)
      {
         if (pc->IsNewline())
         {
            cas.NewLines(pc->GetNlCount());
            did_nl = true;
         }
         else if (pc->Is(CT_ASM_COLON))
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
