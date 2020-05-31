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
#include "LIMITATIONS.h"

#ifdef WIN32
#include <algorithm>                           // to get max
#endif /* ifdef WIN32 */

using namespace uncrustify;


void align_func_proto(size_t span)
{
   LOG_FUNC_ENTRY();

   size_t myspan   = span;
   size_t mythresh = 0;
   size_t mygap    = 0;

   log_rule_B("align_func_proto_thresh");
   mythresh = options::align_func_proto_thresh();

   //AlignStack as;
   // Issue #2771
   // we align token-1 and token-2 if:
   //   token-1->level == token-2->level
   //   and
   //   token-1->brace_level == token-2->brace_level
   // we don't check if token-1 and token-2 are in the same block
   size_t     max_level_is       = 0;
   size_t     max_brace_level_is = 0;
   AlignStack many_as[HOW_MANY_AS_LEVEL + 1][HOW_MANY_AS_BRACE_LEVEL + 1];

   //as.Start(span, mythresh);
   log_rule_B("align_func_proto_gap");
   //as.m_gap = options::align_func_proto_gap();
   mygap = options::align_func_proto_gap();
   log_rule_B("align_var_def_star_style");
   //as.m_star_style = static_cast<AlignStack::StarStyle>(options::align_var_def_star_style());
   log_rule_B("align_var_def_amp_style");
   //as.m_amp_style = static_cast<AlignStack::StarStyle>(options::align_var_def_amp_style());
   size_t mystar_style = options::align_var_def_star_style();
   size_t myamp_style  = options::align_var_def_amp_style();

   // 123
   AlignStack as_br;
   // Issue #2771
   AlignStack many_as_brace[HOW_MANY_AS_LEVEL + 1][HOW_MANY_AS_BRACE_LEVEL + 1];

   log_rule_B("align_single_line_brace_gap");
   size_t mybr_gap = options::align_single_line_brace_gap();

   for (size_t idx = 0; idx <= HOW_MANY_AS_LEVEL; idx++)
   {
      for (size_t idx_brace = 0; idx_brace <= HOW_MANY_AS_BRACE_LEVEL; idx_brace++)
      {
         many_as[idx][idx_brace].Start(myspan, mythresh);
         many_as[idx][idx_brace].m_gap        = mygap;
         many_as[idx][idx_brace].m_star_style = static_cast<AlignStack::StarStyle>(mystar_style);
         many_as[idx][idx_brace].m_amp_style  = static_cast<AlignStack::StarStyle>(myamp_style);

         many_as_brace[idx][idx_brace].Start(myspan, 0);
         many_as_brace[idx][idx_brace].m_gap = mybr_gap;
      }
   }

   //as_br.Start(span, 0);
   //log_rule_B("align_single_line_brace_gap");
   //as_br.m_gap = options::align_single_line_brace_gap();

   bool    look_bro = false;
   chunk_t *toadd;

   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next(pc))
   {
      LOG_FMT(LAS, "%s(%d): orig_line is %zu, orig_col is %zu, text() is '%s', type is %s, level is %zu, brace_level is %zu\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type), pc->level, pc->brace_level);

      if (chunk_is_newline(pc))
      {
         look_bro = false;
         //LOG_FMT(LAS, "%s(%d):  as\n", __func__, __LINE__);
         //as.Debug();
         many_as[pc->level][pc->brace_level].Debug();
         //LOG_FMT(LAS, "%s(%d):  as_br\n", __func__, __LINE__);
         //as_br.Debug();
         many_as_brace[pc->level][pc->brace_level].Debug();

         //as.NewLines(pc->nl_count);
         //many_as[pc->level][pc->brace_level].NewLines(pc->nl_count);
         for (size_t idx = 0; idx <= HOW_MANY_AS_LEVEL; idx++)
         {
            for (size_t idx_brace = 0; idx_brace <= HOW_MANY_AS_BRACE_LEVEL; idx_brace++)
            {
               many_as[idx][idx_brace].NewLines(pc->nl_count);
               //many_as_brace[idx][idx_brace].Start(myspan, 0);
            }
         }

         //as_br.NewLines(pc->nl_count);
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

         if (pc->level > HOW_MANY_AS_LEVEL)
         {
            fprintf(stderr, "%s(%d): Not enought memory for Stack\n",
                    __func__, __LINE__);
            fprintf(stderr, "%s(%d): the current maximum for level is %d\n",
                    __func__, __LINE__, HOW_MANY_AS_LEVEL);
            log_flush(true);
         }

         if (pc->level > HOW_MANY_AS_BRACE_LEVEL)
         {
            fprintf(stderr, "%s(%d): Not enought memory for Stack\n",
                    __func__, __LINE__);
            fprintf(stderr, "%s(%d): the current maximum for brace_level is %d\n",
                    __func__, __LINE__, HOW_MANY_AS_BRACE_LEVEL);
            log_flush(true);
            exit(EX_SOFTWARE);
         }
         chunk_t *tmp = step_back_over_member(toadd);
         LOG_FMT(LAS, "%s(%d): tmp->text() is '%s', orig_line is %zu, orig_col is %zu, level is %zu, brace_level is %zu\n",
                 __func__, __LINE__, tmp->text(), tmp->orig_line, tmp->orig_col, tmp->level, tmp->brace_level);
         //as.Add(tmp);
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
         //as_br.Add(pc);
         many_as_brace[pc->level][pc->brace_level].Add(pc);
         look_bro = false;
      }
   }

   LOG_FMT(LAS, "%s(%d):  as\n", __func__, __LINE__);

   //as.Debug();
   for (size_t idx = 0; idx <= HOW_MANY_AS_LEVEL; idx++)
   {
      for (size_t idx_brace = 0; idx_brace <= HOW_MANY_AS_BRACE_LEVEL; idx_brace++)
      {
         many_as[idx][idx_brace].End();
         many_as_brace[idx][idx_brace].End();
      }
   }

   //LOG_FMT(LAS, "%s(%d):  as_br\n", __func__, __LINE__);
   //as_br.Debug();
   //as.End();
   //as_br.End();
} // align_func_proto
