/**
 * @file parent_for_pp.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "parent_for_pp.h"

#include "chunk.h"


void do_parent_for_pp()
{
   LOG_FUNC_ENTRY();

   std::vector<Chunk *> viz;

   Chunk                *pc = Chunk::GetHead()->GetNextNcNnl();

   while (pc->IsNotNullChunk())
   {
      // CT_PP_IF,            // #if, #ifdef, or #ifndef
      // CT_PP_ELSE,          // #else or #elif
      // CT_PP_ENDIF,         // #endif
      if (pc->Is(CT_PP_IF))
      {
         LOG_FMT(LMCB, "%s(%d): IF: orig line %zu, orig col is %zu\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol());
         log_pcf_flags(LMCB, pc->GetFlags());
         viz.push_back(pc);
      }
      else if (pc->Is(CT_PP_ELSE))
      {
         LOG_FMT(LMCB, "%s(%d): ELSE: orig line %zu, orig col is %zu\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol());
         log_pcf_flags(LMCB, pc->GetFlags());
         size_t level = pc->GetPpLevel();
         Chunk  *a    = viz.at(level - 1);
         pc->SetParent(a);
      }
      else if (pc->Is(CT_PP_ENDIF))
      {
         LOG_FMT(LMCB, "%s(%d): ENDIF: orig line %zu, orig col is %zu\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol());
         log_pcf_flags(LMCB, pc->GetFlags());
         size_t level = pc->GetPpLevel();
         Chunk  *a    = viz.at(level);
         pc->SetParent(a);
         viz.pop_back();
      }
      pc = pc->GetNextNcNnl();
   }
} // do_parent_for_pp
