/**
 * @file align_init_brace.cpp
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align_init_brace.h"
#include "align_log_al.h"
#include "align_tab_column.h"
#include "align_tools.h"
#include "chunk_list.h"
#include "indent.h"
#include "uncrustify.h"

using namespace uncrustify;


void align_init_brace(chunk_t *start)
{
   LOG_FUNC_ENTRY();

   chunk_t *num_token = nullptr;

   cpd.al_cnt       = 0;
   cpd.al_c99_array = false;

   LOG_FMT(LALBR, "%s(%d): start @ orig_line is %zu, orig_col is %zu\n",
           __func__, __LINE__, start->orig_line, start->orig_col);

   chunk_t *pc       = chunk_get_next_ncnl(start);
   chunk_t *pcSingle = scan_ib_line(pc, true);
   if (  pcSingle == nullptr
      || (chunk_is_token(pcSingle, CT_BRACE_CLOSE) && pcSingle->parent_type == CT_ASSIGN))
   {
      // single line - nothing to do
      LOG_FMT(LALBR, "%s(%d): single line - nothing to do\n", __func__, __LINE__);
      return;
   }
   LOG_FMT(LALBR, "%s(%d): is not a single line\n", __func__, __LINE__);

   do
   {
      pc = scan_ib_line(pc, false);

      // debug dump the current frame
      LOG_FMT(LALBR, "%s(%d): debug dump after, orig_line is %zu\n",
              __func__, __LINE__, pc->orig_line);
      align_log_al(LALBR, pc->orig_line);

      while (chunk_is_newline(pc))
      {
         pc = chunk_get_next(pc);
      }
   } while (pc != nullptr && pc->level > start->level);

   // debug dump the current frame
   align_log_al(LALBR, start->orig_line);

   if (  options::align_on_tabstop()
      && cpd.al_cnt >= 1
      && (cpd.al[0].type == CT_ASSIGN))
   {
      cpd.al[0].col = align_tab_column(cpd.al[0].col);
   }

   pc = chunk_get_next(start);
   size_t idx = 0;
   do
   {
      chunk_t *tmp;
      if (idx == 0 && ((tmp = skip_c99_array(pc)) != nullptr))
      {
         pc = tmp;
         if (pc != nullptr)
         {
            LOG_FMT(LALBR, " -%zu- skipped '[] =' to %s\n",
                    pc->orig_line, get_token_name(pc->type));
         }
         continue;
      }

      chunk_t *next = pc;
      if (idx < cpd.al_cnt)
      {
         LOG_FMT(LALBR, " (%zu) check %s vs %s -- ",
                 idx, get_token_name(pc->type), get_token_name(cpd.al[idx].type));
         if (pc->type == cpd.al[idx].type)
         {
            if (idx == 0 && cpd.al_c99_array)
            {
               chunk_t *prev = chunk_get_prev(pc);
               if (chunk_is_newline(prev))
               {
                  chunk_flags_set(pc, PCF_DONT_INDENT);
               }
            }
            LOG_FMT(LALBR, " [%s] to col %zu\n", pc->text(), cpd.al[idx].col);

            if (num_token != nullptr)
            {
               int col_diff = pc->column - num_token->column;

               reindent_line(num_token, cpd.al[idx].col - col_diff);
               //LOG_FMT(LSYS, "-= %zu =- NUM indent [%s] col=%d diff=%d\n",
               //        num_token->orig_line,
               //        num_token->text(), cpd.al[idx - 1].col, col_diff);

               chunk_flags_set(num_token, PCF_WAS_ALIGNED);
               num_token = nullptr;
            }

            // Comma's need to 'fall back' to the previous token
            if (chunk_is_token(pc, CT_COMMA))
            {
               next = chunk_get_next(pc);
               if (next != nullptr && !chunk_is_newline(next))
               {
                  //LOG_FMT(LSYS, "-= %zu =- indent [%s] col=%d len=%d\n",
                  //        next->orig_line,
                  //        next->text(), cpd.al[idx].col, cpd.al[idx].len);

                  if (  (idx < (cpd.al_cnt - 1))
                     && options::align_number_right()
                     && (  chunk_is_token(next, CT_NUMBER_FP)
                        || chunk_is_token(next, CT_NUMBER)
                        || chunk_is_token(next, CT_POS)
                        || chunk_is_token(next, CT_NEG)))
                  {
                     // Need to wait until the next match to indent numbers
                     num_token = next;
                  }
                  else if (idx < (cpd.al_cnt - 1))
                  {
                     LOG_FMT(LALBR, "%s(%d): idx is %zu, al_cnt is %zu, cpd.al[%zu].col is %zu, cpd.al[%zu].len is %zu\n",
                             __func__, __LINE__, idx, cpd.al_cnt, idx, cpd.al[idx].col, idx, cpd.al[idx].len);
                     reindent_line(next, cpd.al[idx].col + cpd.al[idx].len);
                     chunk_flags_set(next, PCF_WAS_ALIGNED);
                  }
               }
            }
            else
            {
               // first item on the line
               LOG_FMT(LALBR, "%s(%d): idx is %zu, cpd.al[%zu].col is %zu\n",
                       __func__, __LINE__, idx, idx, cpd.al[idx].col);
               reindent_line(pc, cpd.al[idx].col);
               chunk_flags_set(pc, PCF_WAS_ALIGNED);

               // see if we need to right-align a number
               if (  (idx < (cpd.al_cnt - 1))
                  && options::align_number_right())
               {
                  next = chunk_get_next(pc);
                  if (  next != nullptr
                     && !chunk_is_newline(next)
                     && (  chunk_is_token(next, CT_NUMBER_FP)
                        || chunk_is_token(next, CT_NUMBER)
                        || chunk_is_token(next, CT_POS)
                        || chunk_is_token(next, CT_NEG)))
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
            LOG_FMT(LALBR, " no match\n");
         }
      }
      if (chunk_is_newline(pc) || chunk_is_newline(next))
      {
         idx = 0;
      }
      pc = chunk_get_next(pc);
   } while (pc != nullptr && pc->level > start->level);
} // align_init_brace
