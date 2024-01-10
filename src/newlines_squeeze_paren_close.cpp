/**
 * @file newlines_squeeze_paren_close.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "newlines_squeeze_paren_close.h"

#include "chunk.h"
#include "newline_add.h"
#include "newline_del_between.h"


void newlines_squeeze_paren_close()
{
   LOG_FUNC_ENTRY();

   Chunk *pc;

   for (pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      Chunk *next;
      Chunk *prev;

      if (pc->Is(CT_NEWLINE))
      {
         prev = pc->GetPrev();
      }
      else
      {
         prev = pc;
      }
      next = pc->GetNext();

      if (  next->IsNotNullChunk()
         && prev->IsNotNullChunk()
         && next->IsParenClose()
         && prev->IsParenClose())
      {
         Chunk *prev_op = prev->GetOpeningParen();
         Chunk *next_op = next->GetOpeningParen();
         bool  flag     = true;

         Chunk *tmp = prev;

         while (tmp->IsParenClose())
         {
            tmp = tmp->GetPrev();
         }

         if (tmp->IsNot(CT_NEWLINE))
         {
            flag = false;
         }

         if (flag)
         {
            if (next_op->IsOnSameLine(prev_op))
            {
               if (pc->Is(CT_NEWLINE))
               {
                  pc = next;
               }
               newline_del_between(prev, next);
            }
            else
            {
               newline_add_between(prev, next);
            }
         }
      }
   }
} // newlines_squeeze_paren_close
