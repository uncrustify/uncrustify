/**
 * @file squeeze.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/squeeze.h"

#include "chunk.h"
#include "log_rules.h"
#include "mark_change.h"
#include "newlines/add.h"
#include "newlines/del_between.h"


constexpr static auto LCURRENT = LNEWLINE;


using namespace uncrustify;


void newlines_squeeze_ifdef()
{
   LOG_FUNC_ENTRY();

   Chunk *pc;

   for (pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      if (  pc->Is(CT_PREPROC)
         && (  pc->GetLevel() > 0
            || options::nl_squeeze_ifdef_top_level()))
      {
         log_rule_B("nl_squeeze_ifdef_top_level");
         Chunk *ppr = pc->GetNext();

         if (  ppr->Is(CT_PP_IF)
            || ppr->Is(CT_PP_ELSE)
            || ppr->Is(CT_PP_ENDIF))
         {
            Chunk *pnl = Chunk::NullChunkPtr;
            Chunk *nnl = ppr->GetNextNl();

            if (  ppr->Is(CT_PP_ELSE)
               || ppr->Is(CT_PP_ENDIF))
            {
               pnl = pc->GetPrevNl();
            }
            Chunk *tmp1;
            Chunk *tmp2;

            if (nnl->IsNotNullChunk())
            {
               if (pnl->IsNotNullChunk())
               {
                  if (pnl->GetNlCount() > 1)
                  {
                     pnl->SetNlCount(1);
                     MARK_CHANGE();

                     tmp1 = pnl->GetPrevNnl();
                     tmp2 = nnl->GetPrevNnl();

                     LOG_FMT(LNEWLINE, "%s(%d): moved from after line %zu to after %zu\n",
                             __func__, __LINE__, tmp1->GetOrigLine(), tmp2->GetOrigLine());
                  }
               }

               if (  ppr->Is(CT_PP_IF)
                  || ppr->Is(CT_PP_ELSE))
               {
                  if (nnl->GetNlCount() > 1)
                  {
                     tmp1 = nnl->GetPrevNnl();
                     LOG_FMT(LNEWLINE, "%s(%d): trimmed newlines after line %zu from %zu\n",
                             __func__, __LINE__, tmp1->GetOrigLine(), nnl->GetNlCount());
                     nnl->SetNlCount(1);
                     MARK_CHANGE();
                  }
               }
            }
         }
      }
   }
} // newlines_squeeze_ifdef


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
