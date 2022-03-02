/**
 * @file align_braced_init_list.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "align_braced_init_list.h"

#include "align_stack.h"
#include "log_rules.h"

constexpr static auto LCURRENT = LALASS;

using namespace uncrustify;


Chunk *align_braced_init_list(Chunk *first, size_t span, size_t thresh, size_t *p_nl_count)
{
   LOG_FUNC_ENTRY();

   if (  first == nullptr
      || first->IsNullChunk())
   {
      // coveralls will complain here. There are no example for that.
      // see https://en.wikipedia.org/wiki/Robustness_principle
      return(nullptr);
   }
   size_t my_level = first->level;

   char   copy[1000];

   LOG_FMT(LALASS, "%s(%d): [my_level is %zu]: start checking with '%s', on orig_line %zu, span is %zu, thresh is %zu\n",
           __func__, __LINE__, my_level, first->ElidedText(copy), first->orig_line, span, thresh);

   // If we are aligning on a tabstop, we shouldn't right-align

   AlignStack vdas;  // variable def assigns

   vdas.Start(span, thresh);
   vdas.m_right_align = !options::align_on_tabstop();

   size_t var_def_cnt = 0;
   size_t equ_count   = 0;
   size_t tmp;
   Chunk  *pc = first;

   while (  pc != nullptr
         && pc->IsNotNullChunk())
   {
      LOG_FMT(LALASS, "%s(%d): orig_line is %zu, check pc->Text() '%s', type is %s, parent_type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->ElidedText(copy), get_token_name(pc->type), get_token_name(get_chunk_parent_type(pc)));

      // Don't check inside SPAREN, PAREN or SQUARE groups
      if (  chunk_is_token(pc, CT_SPAREN_OPEN)
         || chunk_is_token(pc, CT_SQUARE_OPEN)
         || chunk_is_token(pc, CT_PAREN_OPEN))
      {
         LOG_FMT(LALASS, "%s(%d)OK: Don't check inside SPAREN, PAREN or SQUARE groups, type is %s\n",
                 __func__, __LINE__, get_token_name(pc->type));
         tmp = pc->orig_line;
         pc  = chunk_skip_to_match(pc);

         if (pc != nullptr)
         {
            vdas.NewLines(pc->orig_line - tmp);
         }
         continue;
      }

      // Recurse if a brace set is found
      if (  chunk_is_token(pc, CT_BRACE_OPEN)
         && !(get_chunk_parent_type(pc) == CT_BRACED_INIT_LIST))
      {
         size_t myspan;
         size_t mythresh;

         size_t sub_nl_count = 0;

         log_rule_B("align_braced_init_list_span");
         myspan = options::align_braced_init_list_span();
         log_rule_B("align_braced_init_list_thresh");
         mythresh = options::align_braced_init_list_thresh();
         pc       = align_braced_init_list(pc->GetNextNcNnl(), myspan, mythresh, &sub_nl_count);

         if (sub_nl_count > 0)
         {
            vdas.NewLines(sub_nl_count);

            if (p_nl_count != nullptr)
            {
               *p_nl_count += sub_nl_count;
            }
         }
         continue;
      }

      // Done with this brace set?
      if (  (  chunk_is_token(pc, CT_BRACE_CLOSE)
            || chunk_is_token(pc, CT_VBRACE_CLOSE))
         && !(get_chunk_parent_type(pc) == CT_BRACED_INIT_LIST))
      {
         pc = pc->GetNext();
         break;
      }

      if (chunk_is_newline(pc))
      {
         vdas.NewLines(pc->nl_count);

         if (p_nl_count != nullptr)
         {
            *p_nl_count += pc->nl_count;
         }
         var_def_cnt = 0;
         equ_count   = 0;
      }
      else if (  pc->flags.test(PCF_VAR_DEF)
              && !pc->flags.test(PCF_IN_CONST_ARGS) // Issue #1717
              && !pc->flags.test(PCF_IN_FCN_DEF)    // Issue #1717
              && !pc->flags.test(PCF_IN_FCN_CALL))  // Issue #1717
      {
         // produces much more log output. Use it only debugging purpose
         //LOG_FMT(LALASS, "%s(%d): log_pcf_flags pc->flags:\n   ", __func__, __LINE__);
         //log_pcf_flags(LALASS, pc->flags);
         var_def_cnt++;
      }
      else if (var_def_cnt > 1)
      {
         // we hit the second variable def - don't look, don't align
         vdas.Reset();
      }
      else if (  equ_count == 0
              && !pc->flags.test(PCF_IN_TEMPLATE)
              && chunk_is_token(pc, CT_BRACE_OPEN)
              && (get_chunk_parent_type(pc) == CT_BRACED_INIT_LIST))

      {
         equ_count++;
         LOG_FMT(LALASS, "%s(%d)OK: align_braced_init_list_span() is %d\n",
                 __func__, __LINE__, options::align_braced_init_list_span());
         // produces much more log output. Use it only debugging purpose
         //LOG_FMT(LALASS, "%s(%d): log_pcf_flags pc->flags: ", __func__, __LINE__);
         //log_pcf_flags(LALASS, pc->flags);

         if (var_def_cnt != 0)
         {
            LOG_FMT(LALASS, "%s(%d)OK: vdas.Add on '%s' on orig_line %zu, orig_col is %zu\n",
                    __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col);
            vdas.Add(pc);
         }
      }
      pc = pc->GetNext();
   }
   vdas.End();

   if (pc != nullptr)
   {
      LOG_FMT(LALASS, "%s(%d): done on '%s' on orig_line %zu\n",
              __func__, __LINE__, pc->Text(), pc->orig_line);
   }
   else
   {
      LOG_FMT(LALASS, "%s(%d): done on NULL\n", __func__, __LINE__);
   }
   return(pc);
} // align_braced_init_list
