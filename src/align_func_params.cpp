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

#include <algorithm>                           // to get max

constexpr static auto LCURRENT = LALIGN;

using namespace uncrustify;


Chunk *align_func_param(Chunk *start)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LAS, "AlignStack::%s(%d): Candidate is '%s': orig_line is %zu, column is %zu, type is %s, level is %zu\n",
           __func__, __LINE__, start->Text(), start->orig_line, start->column,
           get_token_name(start->type), start->level);
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
   const size_t HOW_MANY_AS = 16;                         // Issue #2921
   AlignStack   many_as[HOW_MANY_AS + 1];

   size_t       max_level_is = 0;

   log_rule_B("align_var_def_star_style");
   log_rule_B("align_var_def_amp_style");

   for (size_t idx = 0; idx <= HOW_MANY_AS; idx++)
   {
      many_as[idx].Start(myspan, mythresh);
      many_as[idx].m_gap        = mygap;
      many_as[idx].m_star_style = static_cast<AlignStack::StarStyle>(options::align_var_def_star_style());
      many_as[idx].m_amp_style  = static_cast<AlignStack::StarStyle>(options::align_var_def_amp_style());
   }

   size_t comma_count = 0;
   size_t chunk_count = 0;
   Chunk  *pc         = start;

   while ((pc = pc->GetNext())->IsNotNullChunk())
   {
      chunk_count++;
      LOG_FMT(LFLPAREN, "%s(%d): orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(),
              get_token_name(pc->type));

      if (chunk_is_token(pc, CT_FUNC_VAR))                    // Issue #2278
      {
         // look after 'protect parenthesis'
         Chunk *after = pc->GetNextNc();

         if (chunk_is_token(after, CT_PAREN_CLOSE))
         {
            Chunk *before = after->GetPrevType(CT_PAREN_OPEN, after->level);

            if (before->IsNotNullChunk())
            {
               // these are 'protect parenthesis'
               // change the types and the level
               set_chunk_type(before, CT_PPAREN_OPEN);
               set_chunk_type(after, CT_PPAREN_CLOSE);
               pc->level = before->level;
               Chunk *tmp = pc->GetPrevNc();

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
               fprintf(stderr, "%s(%d): the current maximum is %zu\n",
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
         if (!pc->IsComment())
         {
            comma_count = 2;
            break;
         }
      }
      else if (chunk_is_token(pc, CT_COMMA))
      {
         if (pc->flags.test(PCF_IN_TEMPLATE))            // Issue #2757
         {
            LOG_FMT(LFLPAREN, "%s(%d): comma is in template\n",
                    __func__, __LINE__);
         }
         else
         {
            Chunk *tmp_prev = pc->GetPrevNc();

            if (!chunk_is_newline(tmp_prev))  // don't count leading commas
            {
               comma_count++;
               LOG_FMT(LFLPAREN, "%s(%d): comma_count is %zu\n",
                       __func__, __LINE__, comma_count);
            }
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
   Chunk *pc = Chunk::GetHead();

   while ((pc = pc->GetNext())->IsNotNullChunk())
   {
      LOG_FMT(LFLPAREN, "%s(%d): orig_line is %zu, orig_col is %zu, Text() is '%s', parent_type is %s, parent_type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(),
              get_token_name(pc->type), get_token_name(pc->parent_type));

      if (  chunk_is_not_token(pc, CT_FPAREN_OPEN)
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
