/**
 * @file init_brace.cpp
 *
 * @author  Guy Maurel
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align/init_brace.h"

#include "align/log_al.h"
#include "align/tab_column.h"
#include "align/tools.h"
#include "log_rules.h"
#include "reindent_line.h"


constexpr static auto LCURRENT = LALBR;


using namespace uncrustify;


void align_init_brace(Chunk *start)
{
   LOG_FUNC_ENTRY();

   Chunk *num_token = Chunk::NullChunkPtr;

   cpd.al_cnt       = 0;
   cpd.al_c99_array = false;

   LOG_FMT(LALBR, "%s(%d): start @ orig line is %zu, orig col is %zu\n",
           __func__, __LINE__, start->GetOrigLine(), start->GetOrigCol());

   Chunk *pc       = start->GetNextNcNnl();
   Chunk *pcSingle = scan_ib_line(pc);

   if (  pcSingle->IsNullChunk()
      || (  pcSingle->Is(CT_BRACE_CLOSE)
         && pcSingle->GetParentType() == CT_ASSIGN))
   {
      // single line - nothing to do
      LOG_FMT(LALBR, "%s(%d): single line - nothing to do\n", __func__, __LINE__);
      return;
   }
   LOG_FMT(LALBR, "%s(%d): is not a single line\n", __func__, __LINE__);

   do
   {
      pc = scan_ib_line(pc);

      // debug dump the current frame
      LOG_FMT(LALBR, "%s(%d): debug dump after, orig line is %zu\n",
              __func__, __LINE__, pc->GetOrigLine());
      align_log_al(LALBR, pc->GetOrigLine());

      while (pc->IsNewline())
      {
         pc = pc->GetNext();
      }
   } while (  pc->IsNotNullChunk()
           && pc->GetLevel() > start->GetLevel());

   // debug dump the current frame
   align_log_al(LALBR, start->GetOrigLine());

   log_rule_B("align_on_tabstop");

   if (  options::align_on_tabstop()
      && cpd.al_cnt >= 1
      && (cpd.al[0].type == CT_ASSIGN))
   {
      cpd.al[0].col = align_tab_column(cpd.al[0].col);
   }
   pc = start->GetNext();
   size_t idx = 0;

   do
   {
      Chunk *tmp;

      if (  idx == 0
         && ((tmp = skip_c99_array(pc))->IsNotNullChunk()))
      {
         pc = tmp;

         LOG_FMT(LALBR, " -%zu- skipped '[] =' to %s\n",
                 pc->GetOrigLine(), get_token_name(pc->GetType()));
         continue;
      }
      Chunk *next = pc;

      if (idx < cpd.al_cnt)
      {
         LOG_FMT(LALBR, "%s(%d): (%zu) check %s vs %s -- ??\n",
                 __func__, __LINE__, idx, get_token_name(pc->GetType()), get_token_name(cpd.al[idx].type));

         if (pc->Is(cpd.al[idx].type))
         {
            if (  idx == 0
               && cpd.al_c99_array)
            {
               Chunk *prev = pc->GetPrev();

               if (prev->IsNewline())
               {
                  pc->SetFlagBits(PCF_DONT_INDENT);
               }
            }
            LOG_FMT(LALBR, "%s(%d): cpd.al[%zu].col is %zu\n",
                    __func__, __LINE__, idx, cpd.al[idx].col);
            LOG_FMT(LALBR, "%s(%d): (idx is %zu) check %s vs %s -- [%s] to col %zu\n",
                    __func__, __LINE__,
                    idx, get_token_name(pc->GetType()), get_token_name(cpd.al[idx].type), pc->Text(), cpd.al[idx].col);

            if (num_token->IsNotNullChunk())
            {
               int col_diff = pc->GetColumn() - num_token->GetColumn();

               reindent_line(num_token, cpd.al[idx].col - col_diff);
               //LOG_FMT(LSYS, "-= %zu =- NUM indent [%s] col=%d diff=%d\n",
               //        num_token->GetOrigLine(),
               //        num_token->Text(), cpd.al[idx - 1].col, col_diff);

               num_token->SetFlagBits(PCF_WAS_ALIGNED);
               num_token = Chunk::NullChunkPtr;
            }

            // Comma's need to 'fall back' to the previous token
            if (pc->Is(CT_COMMA))
            {
               next = pc->GetNext();

               if (!next->IsNewline())
               {
                  //LOG_FMT(LSYS, "-= %zu =- indent [%s] col=%d len=%d\n",
                  //        next->GetOrigLine(),
                  //        next->Text(), cpd.al[idx].col, cpd.al[idx].len);

                  log_rule_B("align_number_right");

                  if (  (idx < (cpd.al_cnt - 1))
                     && options::align_number_right()
                     && (  next->Is(CT_NUMBER_FP)
                        || next->Is(CT_NUMBER)
                        || next->Is(CT_POS)
                        || next->Is(CT_NEG)))
                  {
                     // Need to wait until the next match to indent numbers
                     num_token = next;
                  }
                  else if (idx < (cpd.al_cnt - 1))
                  {
                     LOG_FMT(LALBR, "%s(%d): idx is %zu, al_cnt is %zu, cpd.al[%zu].col is %zu, cpd.al[%zu].len is %zu\n",
                             __func__, __LINE__, idx, cpd.al_cnt, idx, cpd.al[idx].col, idx, cpd.al[idx].len);
                     reindent_line(next, cpd.al[idx].col + cpd.al[idx].len);
                     next->SetFlagBits(PCF_WAS_ALIGNED);
                  }
               }
            }
            else
            {
               // first item on the line
               LOG_FMT(LALBR, "%s(%d): idx is %zu, cpd.al[%zu].col is %zu\n",
                       __func__, __LINE__, idx, idx, cpd.al[idx].col);
               reindent_line(pc, cpd.al[idx].col);
               pc->SetFlagBits(PCF_WAS_ALIGNED);

               // see if we need to right-align a number
               log_rule_B("align_number_right");

               if (  (idx < (cpd.al_cnt - 1))
                  && options::align_number_right())
               {
                  next = pc->GetNext();

                  if (  !next->IsNewline()
                     && (  next->Is(CT_NUMBER_FP)
                        || next->Is(CT_NUMBER)
                        || next->Is(CT_POS)
                        || next->Is(CT_NEG)))
                  {
                     // Need to wait until the next match to indent numbers
                     num_token = next;
                  }
               }
            }
            idx++;
         }
         else
         {
            LOG_FMT(LALBR, "%s(%d): (%zu) check %s vs %s -- no match\n",
                    __func__, __LINE__, idx, get_token_name(pc->GetType()), get_token_name(cpd.al[idx].type));
         }
      }

      if (  pc->IsNewline()
         || next->IsNewline())
      {
         idx = 0;
      }
      pc = pc->GetNext();
   } while (  pc->IsNotNullChunk()
           && pc->GetLevel() > start->GetLevel());
} // align_init_brace
