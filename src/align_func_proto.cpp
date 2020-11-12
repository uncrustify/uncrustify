/**
 * @file align_func_proto.cpp
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align_func_proto.h"

#include "align_stack.h"
#include "align_tools.h"
#include "log_rules.h"
#include "uncrustify_limits.h"

#include <algorithm>                           // to get max

using namespace uncrustify;


void align_func_proto(size_t span)
{
   LOG_FUNC_ENTRY();

   size_t myspan   = span;
   size_t mythresh = 0;

   log_rule_B("align_func_proto_gap");
   size_t mygap = options::align_func_proto_gap();

   log_rule_B("align_func_proto_thresh");
   mythresh = options::align_func_proto_thresh();

   // Issue #2771
   // we align token-1 and token-2 if:
   //   token-1->level == token-2->level
   //   and
   //   token-1->brace_level == token-2->brace_level
   // we don't check if token-1 and token-2 are in the same block
   size_t max_level_is       = 0;
   size_t max_brace_level_is = 0;

   log_rule_B("align_var_def_star_style");
   size_t mystar_style = options::align_var_def_star_style();

   log_rule_B("align_var_def_amp_style");
   size_t       myamp_style = options::align_var_def_amp_style();

   const size_t max_level_count = 16;
   const size_t max_brace_level = 16;

   AlignStack   many_as[max_level_count + 1][max_brace_level + 1];

   // Issue #2771
   AlignStack many_as_brace[max_level_count + 1][max_brace_level + 1];

   log_rule_B("align_single_line_brace_gap");
   size_t mybr_gap = options::align_single_line_brace_gap();

   for (size_t idx = 0; idx <= max_level_count; idx++)
   {
      for (size_t idx_brace = 0; idx_brace <= max_brace_level; idx_brace++)
      {
         many_as[idx][idx_brace].Start(myspan, mythresh);
         many_as[idx][idx_brace].m_gap        = mygap;
         many_as[idx][idx_brace].m_star_style = static_cast<AlignStack::StarStyle>(mystar_style);
         many_as[idx][idx_brace].m_amp_style  = static_cast<AlignStack::StarStyle>(myamp_style);

         many_as_brace[idx][idx_brace].Start(myspan, 0);
         many_as_brace[idx][idx_brace].m_gap = mybr_gap;
      }
   }

   bool    look_bro = false;
   chunk_t *toadd;

   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next(pc))
   {
      LOG_FMT(LAS, "%s(%d): orig_line is %zu, orig_col is %zu, text() is '%s', type is %s, level is %zu, brace_level is %zu\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(),
              get_token_name(pc->type), pc->level, pc->brace_level);

      if (pc->level > max_level_count)                             // Issue #2960
      {
         fprintf(stderr, "%s(%d): pc->level is %zu. This is too big, at line %zu, column %zu. Make a report, please.\n",
                 __func__, __LINE__, pc->level, pc->orig_line, pc->orig_col);
         log_flush(true);
         exit(EX_SOFTWARE);
      }

      if (pc->brace_level > max_brace_level)
      {
         fprintf(stderr, "%s(%d): pc->brace_level is %zu. This is too big, at line %zu, column %zu. Make a report, please.\n",
                 __func__, __LINE__, pc->level, pc->orig_line, pc->orig_col);
         log_flush(true);
         exit(EX_SOFTWARE);
      }

      if (  chunk_is_newline(pc)
         && !pc->flags.test(PCF_IN_FCN_CALL))                 // Issue #2831
      {
         look_bro = false;
         many_as[pc->level][pc->brace_level].Debug();
         many_as_brace[pc->level][pc->brace_level].Debug();

         for (size_t idx = 0; idx <= max_level_count; idx++)
         {
            for (size_t idx_brace = 0; idx_brace <= max_brace_level; idx_brace++)
            {
               many_as[idx][idx_brace].NewLines(pc->nl_count);
            }
         }

         many_as_brace[pc->level][pc->brace_level].NewLines(pc->nl_count);
      }
      else if (  chunk_is_token(pc, CT_FUNC_PROTO)
              || (  chunk_is_token(pc, CT_FUNC_DEF)
                 && options::align_single_line_func()))
      {
         log_rule_B("align_single_line_func");
         log_rule_B("align_on_operator");

         if (  get_chunk_parent_type(pc) == CT_OPERATOR
            && options::align_on_operator())
         {
            toadd = chunk_get_prev_ncnl(pc);
         }
         else
         {
            toadd = pc;
         }

         if (pc->level > max_level_count)
         {
            fprintf(stderr, "%s(%d): Not enought memory for Stack\n",
                    __func__, __LINE__);
            fprintf(stderr, "%s(%d): the current maximum for level is %zu\n",
                    __func__, __LINE__, max_level_count);
            log_flush(true);
         }

         if (pc->level > max_brace_level)
         {
            fprintf(stderr, "%s(%d): Not enought memory for Stack\n",
                    __func__, __LINE__);
            fprintf(stderr, "%s(%d): the current maximum for brace_level is %zu\n",
                    __func__, __LINE__, max_brace_level);
            log_flush(true);
            exit(EX_SOFTWARE);
         }
         chunk_t *tmp = step_back_over_member(toadd);
         LOG_FMT(LAS, "%s(%d): tmp->text() is '%s', orig_line is %zu, orig_col is %zu, level is %zu, brace_level is %zu\n",
                 __func__, __LINE__, tmp->text(), tmp->orig_line, tmp->orig_col,
                 tmp->level, tmp->brace_level);
         max_level_is       = max(max_level_is, pc->level);
         max_brace_level_is = max(max_brace_level_is, pc->level);
         many_as[pc->level][pc->brace_level].Add(tmp);
         log_rule_B("align_single_line_brace");
         look_bro = (chunk_is_token(pc, CT_FUNC_DEF))
                    && options::align_single_line_brace();
      }
      else if (  look_bro
              && chunk_is_token(pc, CT_BRACE_OPEN)
              && pc->flags.test(PCF_ONE_LINER))
      {
         many_as_brace[pc->level][pc->brace_level].Add(pc);
         look_bro = false;
      }
   }

   LOG_FMT(LAS, "%s(%d):  as\n", __func__, __LINE__);

   for (size_t idx = 0; idx <= max_level_count; idx++)
   {
      for (size_t idx_brace = 0; idx_brace <= max_brace_level; idx_brace++)
      {
         many_as[idx][idx_brace].End();
         many_as_brace[idx][idx_brace].End();
      }
   }
} // align_func_proto
