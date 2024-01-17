/**
 * @file newlines_squeeze_ifdef.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "newlines_squeeze_ifdef.h"

#include "chunk.h"
#include "log_rules.h"
#include "mark_change.h"


constexpr static auto LCURRENT = LNEWLINE;


using namespace uncrustify;


#define MARK_CHANGE()    mark_change(__func__, __LINE__)


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
