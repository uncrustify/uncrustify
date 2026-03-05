/**
 * @file func_params.cpp
 *
 * @author  Guy Maurel
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "align/func_params.h"

#include "align/stack.h"
#include "log_rules.h"

#include <algorithm>                           // to get max
#include <cstdio>                              // to get fprintf


constexpr static auto LCURRENT = LALIGN;


using namespace uncrustify;


Chunk *align_func_param(Chunk *start)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LAS, "AlignStack::%s(%d): Candidate is '%s': orig line is %zu, column is %zu, type is %s, level is %zu\n",
           __func__, __LINE__, start->GetLogText(), start->GetOrigLine(), start->GetColumn(),
           get_token_name(start->GetType()), start->GetLevel());
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

   // Line skip configuration
   log_rule_B("align_func_params_span_num_empty_lines");
   log_rule_B("align_func_params_span_num_pp_lines");
   log_rule_B("align_func_params_span_num_cmt_lines");
   LineSkipConfig skip_cfg = {};
   skip_cfg.empty_lines = options::align_func_params_span_num_empty_lines();
   skip_cfg.pp_lines    = options::align_func_params_span_num_pp_lines();
   skip_cfg.cmt_lines   = options::align_func_params_span_num_cmt_lines();

   // Working copy of skip config - one budget per level
   LineSkipConfig skip_budgets[HOW_MANY_AS + 1];

   size_t         max_level_is = 0;

   log_rule_B("align_var_def_star_style");
   log_rule_B("align_var_def_amp_style");

   for (size_t idx = 0; idx <= HOW_MANY_AS; idx++)
   {
      many_as[idx].Start(myspan, mythresh);
      many_as[idx].m_gap        = mygap;
      many_as[idx].m_star_style = static_cast<AlignStack::StarStyle>(options::align_var_def_star_style());
      many_as[idx].m_amp_style  = static_cast<AlignStack::StarStyle>(options::align_var_def_amp_style());
      skip_budgets[idx]         = skip_cfg;
   }

   size_t comma_count = 0;
   size_t chunk_count = 0;
   Chunk  *pc         = start;

   while ((pc = pc->GetNext())->IsNotNullChunk())
   {
      chunk_count++;
      LOG_CHUNK(LTOK, pc);

      if (pc->Is(E_Token::CT_FUNC_VAR))                    // Issue #2278
      {
         // look after 'protect parenthesis'
         Chunk *after = pc->GetNextNc();

         if (after->Is(E_Token::CT_PAREN_CLOSE))
         {
            Chunk *before = after->GetPrevType(E_Token::CT_PAREN_OPEN, after->GetLevel());

            if (before->IsNotNullChunk())
            {
               // these are 'protect parenthesis'
               // change the types and the level
               before->SetType(E_Token::CT_PPAREN_OPEN);
               after->SetType(E_Token::CT_PPAREN_CLOSE);
               pc->SetLevel(before->GetLevel());
               Chunk *tmp = pc->GetPrevNc();

               if (tmp->Is(E_Token::CT_PTR_TYPE))
               {
                  tmp->SetLevel(before->GetLevel());
               }
            }
         }
         else if (after->Is(E_Token::CT_TPAREN_CLOSE))
         {
            Chunk *before = after->GetPrevType(E_Token::CT_TPAREN_OPEN, after->GetLevel());

            if (before->IsNotNullChunk())
            {
               // these are 'protect parenthesis'
               // change the types and the level
               before->SetType(E_Token::CT_TPAREN_OPEN);
               after->SetType(E_Token::CT_TPAREN_CLOSE);
               pc->SetLevel(before->GetLevel());
               Chunk *tmp = pc->GetPrevNc();

               if (tmp->Is(E_Token::CT_PTR_TYPE))
               {
                  tmp->SetLevel(before->GetLevel());
               }
            }
         }
      }

      if (pc->IsNewline())
      {
         comma_count = 0;
         chunk_count = 0;
         size_t level  = pc->GetLevel();
         size_t nl_cnt = pc->GetNlCountFiltered(skip_budgets[level]);
         LOG_FMT(LAS, "%s(%d): NL orig line %zu, level %zu, nl_cnt %zu, budget: empty=%zu pp=%zu cmt=%zu\n",
                 __func__, __LINE__, pc->GetOrigLine(), level, nl_cnt,
                 skip_budgets[level].empty_lines, skip_budgets[level].pp_lines, skip_budgets[level].cmt_lines);

         if (nl_cnt > 0)
         {
            many_as[level].NewLines(nl_cnt);
         }
      }
      else if (  pc->GetLevel() <= start->GetLevel()
              && !pc->TestFlags(PCF_IN_PREPROC))
      {
         LOG_FMT(LAS, "%s(%d): BREAK at orig line %zu, text '%s', type %s, pc level %zu, start level %zu\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetLogText(),
                 get_token_name(pc->GetType()), pc->GetLevel(), start->GetLevel());
         break;
      }
      else if (pc->TestFlags(PCF_VAR_DEF))
      {
         if (chunk_count > 1)
         {
            if (pc->GetLevel() > HOW_MANY_AS)
            {
               fprintf(stderr, "%s(%d): Not enough memory for Stack\n",
                       __func__, __LINE__);
               fprintf(stderr, "%s(%d): the current maximum is %zu\n",
                       __func__, __LINE__, HOW_MANY_AS);
               log_flush(true);
               exit(EX_SOFTWARE);
            }
            max_level_is = std::max(max_level_is, pc->GetLevel());
            many_as[pc->GetLevel()].Add(pc);
            skip_budgets[pc->GetLevel()] = skip_cfg;  // Reset budget after adding
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
      else if (pc->Is(E_Token::CT_COMMA))
      {
         if (pc->TestFlags(PCF_IN_TEMPLATE))            // Issue #2757
         {
            LOG_FMT(LFLPAREN, "%s(%d): comma is in template\n",
                    __func__, __LINE__);
         }
         else
         {
            Chunk *tmp_prev = pc->GetPrevNc();

            if (!tmp_prev->IsNewline())  // don't count leading commas
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


void align_func_params()
{
   LOG_FUNC_ENTRY();
   Chunk *pc = Chunk::GetHead();

   while ((pc = pc->GetNext())->IsNotNullChunk())
   {
      LOG_FMT(LFLPAREN, "%s(%d): orig line is %zu, orig col is %zu, text is '%s', type is %s, parent type is %s\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLogText(),
              get_token_name(pc->GetType()), get_token_name(pc->GetParentType()));

      if (  pc->IsNot(E_Token::CT_FPAREN_OPEN)
         || (  pc->GetParentType() != E_Token::CT_FUNC_PROTO
            && pc->GetParentType() != E_Token::CT_FUNC_DEF
            && pc->GetParentType() != E_Token::CT_FUNC_CLASS_PROTO
            && pc->GetParentType() != E_Token::CT_FUNC_CLASS_DEF
            && pc->GetParentType() != E_Token::CT_TYPEDEF))
      {
         continue;
      }
      // We are on a open parenthesis of a prototype
      pc = align_func_param(pc);
   }
} // void align_func_params
