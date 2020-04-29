/**
 * @file align_func_params.cpp
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align_func_params.h"
#include "align_stack.h"
#include "log_rules.h"

#ifdef WIN32
#include <algorithm>                           // to get max
#endif /* ifdef WIN32 */

using namespace uncrustify;


chunk_t *align_func_param(chunk_t *start)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LAS, "AlignStack::%s(%d): Candidate is '%s': orig_line is %zu, column is %zu, type is %s, level is %zu\n",
           __func__, __LINE__, start->text(), start->orig_line, start->column, get_token_name(start->type), start->level);
   // Defaults, if the align_func_params = true
   size_t myspan   = 2;
   size_t mythresh = 0;
   size_t mygap    = 0;

   // Override, if the align_func_params_span > 0
   log_rule_B("align_func_params_span");

   if (options::align_func_params_span() > 0)
   {
      myspan = options::align_func_params_span();
      log_rule_B("align_func_params_thresh");
      mythresh = options::align_func_params_thresh();
      log_rule_B("align_func_params_gap");
      mygap = options::align_func_params_gap();
   }
#define HOW_MANY_AS    8
   size_t     max_level_is = 0;
   AlignStack many_as[HOW_MANY_AS + 1];
   // NOTE: many_as[0] is not used

   log_rule_B("align_var_def_star_style");
   log_rule_B("align_var_def_amp_style");

   for (size_t idx = 1; idx <= HOW_MANY_AS; idx++)
   {
      many_as[idx].Start(myspan, mythresh);
      many_as[idx].m_gap        = mygap;
      many_as[idx].m_star_style = static_cast<AlignStack::StarStyle>(options::align_var_def_star_style());
      many_as[idx].m_amp_style  = static_cast<AlignStack::StarStyle>(options::align_var_def_amp_style());
   }

   size_t  comma_count = 0;
   size_t  chunk_count = 0;
   chunk_t *pc         = start;

   while ((pc = chunk_get_next(pc)) != nullptr)
   {
      chunk_count++;
      LOG_FMT(LFLPAREN, "%s(%d): orig_line is %zu, orig_col is %zu, text() is '%s', type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
      //LOG_FMT(LFLPAREN, "   pc->flags: ");
      //log_pcf_flags(LFLPAREN, pc->flags);

      if (chunk_is_token(pc, CT_FUNC_VAR))                    // Issue #2278
      {
         // look after 'protect parenthesis'
         chunk_t *after = chunk_get_next_nc(pc);

         if (chunk_is_token(after, CT_PAREN_CLOSE))
         {
            chunk_t *before = chunk_get_prev_type(after, CT_PAREN_OPEN, after->level);

            if (before != nullptr)
            {
               // these are 'protect parenthesis'
               // change the types and the level
               set_chunk_type(before, CT_PPAREN_OPEN);
               set_chunk_type(after, CT_PPAREN_CLOSE);
               pc->level = before->level;
               chunk_t *tmp = chunk_get_prev_nc(pc);

               if (chunk_is_token(tmp, CT_PTR_TYPE))
               {
                  tmp->level = before->level;
               }
            }
         }
      }

      if (chunk_is_newline(pc))
      {
         comma_count = 0;
         chunk_count = 0;
         many_as[pc->level].NewLines(pc->nl_count);
      }
      else if (pc->level <= start->level)
      {
         // for debuging purpose only
         //for (size_t idx = 1; idx <= max_level_is; idx++)
         //{
         //   many_as[idx].Debug();
         //}

         break;
      }
      else if (pc->flags.test(PCF_VAR_DEF))
      {
         if (chunk_count > 1)
         {
            if (pc->level > HOW_MANY_AS)
            {
               fprintf(stderr, "%s(%d): Not enought memory for Stack\n",
                       __func__, __LINE__);
               fprintf(stderr, "%s(%d): the current maximum is %d\n",
                       __func__, __LINE__, HOW_MANY_AS);
               log_flush(true);
               exit(EX_SOFTWARE);
            }
            max_level_is = max(max_level_is, pc->level);
            many_as[pc->level].Add(pc);
         }
      }
      else if (comma_count > 0)
      {
         if (!chunk_is_comment(pc))
         {
            comma_count = 2;
            break;
         }
      }
      else if (chunk_is_token(pc, CT_COMMA))
      {
         chunk_t *tmp_prev = chunk_get_prev_nc(pc);

         if (!chunk_is_newline(tmp_prev))  // don't count leading commas
         {
            comma_count++;
         }
      }
   }

   if (comma_count <= 1)
   {
      for (size_t idx = 1; idx <= max_level_is; idx++)
      {
         many_as[idx].End();
      }
   }
   return(pc);
} // align_func_param


void align_func_params(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc = chunk_get_head();

   while ((pc = chunk_get_next(pc)) != nullptr)
   {
      LOG_FMT(LFLPAREN, "%s(%d): orig_line is %zu, orig_col is %zu, text() is '%s', parent_type is %s, parent_type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type), get_token_name(pc->parent_type));

      if (  pc->type != CT_FPAREN_OPEN
         || (  get_chunk_parent_type(pc) != CT_FUNC_PROTO
            && get_chunk_parent_type(pc) != CT_FUNC_DEF
            && get_chunk_parent_type(pc) != CT_FUNC_CLASS_PROTO
            && get_chunk_parent_type(pc) != CT_FUNC_CLASS_DEF
            && get_chunk_parent_type(pc) != CT_TYPEDEF))
      {
         continue;
      }
      // We are on a open parenthesis of a prototype
      pc = align_func_param(pc);
   }
} // void align_func_params
