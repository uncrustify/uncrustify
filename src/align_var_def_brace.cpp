/**
 * @file align_var_def_brace.cpp
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align_var_def_brace.h"

#include "align_stack.h"
#include "align_tools.h"
#include "log_rules.h"

constexpr static auto LCURRENT = LAVDB;

using namespace uncrustify;


Chunk *align_var_def_brace(Chunk *start, size_t span, size_t *p_nl_count)
{
   LOG_FUNC_ENTRY();

   if (start->IsNullChunk())
   {
      return(nullptr);
   }
   Chunk  *next;
   size_t myspan   = span;
   size_t mythresh = 0;
   size_t mygap    = 0;

   // Override the span, if this is a struct/union
   if (  get_chunk_parent_type(start) == CT_STRUCT
      || get_chunk_parent_type(start) == CT_UNION)
   {
      log_rule_B("align_var_struct_span");
      myspan = options::align_var_struct_span();
      log_rule_B("align_var_struct_thresh");
      mythresh = options::align_var_struct_thresh();
      log_rule_B("align_var_struct_gap");
      mygap = options::align_var_struct_gap();
   }
   else if (get_chunk_parent_type(start) == CT_CLASS)
   {
      log_rule_B("align_var_class_span");
      myspan = options::align_var_class_span();
      log_rule_B("align_var_class_thresh");
      mythresh = options::align_var_class_thresh();
      log_rule_B("align_var_class_gap");
      mygap = options::align_var_class_gap();
   }
   else
   {
      log_rule_B("align_var_def_thresh");
      mythresh = options::align_var_def_thresh();
      log_rule_B("align_var_def_gap");
      mygap = options::align_var_def_gap();
   }
   // can't be any variable definitions in a "= {" block
   Chunk *prev = start->GetPrevNcNnl();

   if (chunk_is_token(prev, CT_ASSIGN))
   {
      LOG_FMT(LAVDB, "%s(%d): start->Text() '%s', type is %s, on orig_line %zu (abort due to assign)\n",
              __func__, __LINE__, start->Text(), get_token_name(start->type), start->orig_line);

      Chunk *pc = start->GetNextType(CT_BRACE_CLOSE, start->level);
      return(pc->GetNextNcNnl());
   }
   char copy[1000];

   LOG_FMT(LAVDB, "%s(%d): start->Text() '%s', type is %s, on orig_line %zu\n",
           __func__, __LINE__, start->ElidedText(copy), get_token_name(start->type), start->orig_line);

   log_rule_B("align_var_def_inline");
   auto const align_mask =
      PCF_IN_FCN_DEF | PCF_VAR_1ST |
      (options::align_var_def_inline() ? PCF_NONE : PCF_VAR_INLINE);

   // Set up the variable/prototype/definition aligner
   AlignStack as;

   as.Start(myspan, mythresh);
   as.m_gap = mygap;
   log_rule_B("align_var_def_star_style");
   as.m_star_style = static_cast<AlignStack::StarStyle>(options::align_var_def_star_style());
   log_rule_B("align_var_def_amp_style");
   as.m_amp_style = static_cast<AlignStack::StarStyle>(options::align_var_def_amp_style());

   // Set up the bit colon aligner
   AlignStack as_bc;

   as_bc.Start(myspan, 0);
   log_rule_B("align_var_def_colon_gap");
   as_bc.m_gap = options::align_var_def_colon_gap();

   AlignStack as_at; // attribute

   as_at.Start(myspan, 0);

   // Set up the brace open aligner
   AlignStack as_br;

   as_br.Start(myspan, mythresh);
   log_rule_B("align_single_line_brace_gap");
   as_br.m_gap = options::align_single_line_brace_gap();

   bool fp_look_bro   = false;
   bool did_this_line = false;

   log_rule_B("align_mix_var_proto");
   bool  fp_active = options::align_mix_var_proto();
   Chunk *pc       = start->GetNext();

   while (  pc != nullptr
         && pc->IsNotNullChunk()
         && (  pc->level >= start->level
            || pc->level == 0))
   {
      if (chunk_is_newline(pc))
      {
         LOG_FMT(LAVDB, "%s(%d): orig_line is %zu, orig_col is %zu, <Newline>\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col);
      }
      else
      {
         LOG_FMT(LAVDB, "%s(%d): orig_line is %zu, orig_col is %zu, Text() '%s', type is %s\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
      }

      if (pc->IsComment())
      {
         if (pc->nl_count > 0)
         {
            as.NewLines(pc->nl_count);
            as_bc.NewLines(pc->nl_count);
            as_at.NewLines(pc->nl_count);
            as_br.NewLines(pc->nl_count);
         }
         pc = pc->GetNext();
         continue;
      }

      if (  fp_active
         && !pc->flags.test(PCF_IN_CLASS_BASE))
      {
         // WARNING: Duplicate from the align_func_proto()
         log_rule_B("align_single_line_func");

         if (  chunk_is_token(pc, CT_FUNC_PROTO)
            || (  chunk_is_token(pc, CT_FUNC_DEF)
               && options::align_single_line_func()))
         {
            LOG_FMT(LAVDB, "%s(%d): add = '%s', orig_line is %zu, orig_col is %zu, level is %zu\n",
                    __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col, pc->level);

            Chunk *toadd;

            log_rule_B("align_on_operator");

            if (  get_chunk_parent_type(pc) == CT_OPERATOR
               && options::align_on_operator())
            {
               toadd = pc->GetPrevNcNnl();
            }
            else
            {
               toadd = pc;
            }
            as.Add(step_back_over_member(toadd));
            log_rule_B("align_single_line_brace");
            fp_look_bro = (chunk_is_token(pc, CT_FUNC_DEF))
                          && options::align_single_line_brace();
         }
         else if (  fp_look_bro
                 && chunk_is_token(pc, CT_BRACE_OPEN)
                 && pc->flags.test(PCF_ONE_LINER))
         {
            as_br.Add(pc);
            fp_look_bro = false;
         }
      }

      // process nested braces
      if (chunk_is_token(pc, CT_BRACE_OPEN))
      {
         size_t sub_nl_count = 0;

         pc = align_var_def_brace(pc, span, &sub_nl_count);

         if (sub_nl_count > 0)
         {
            fp_look_bro   = false;
            did_this_line = false;
            as.NewLines(sub_nl_count);
            as_bc.NewLines(sub_nl_count);
            as_at.NewLines(sub_nl_count);
            as_br.NewLines(sub_nl_count);

            if (p_nl_count != nullptr)
            {
               *p_nl_count += sub_nl_count;
            }
         }
         continue;
      }

      // Done with this brace set?
      if (chunk_is_token(pc, CT_BRACE_CLOSE))
      {
         pc = pc->GetNext();
         break;
      }

      if (chunk_is_newline(pc))
      {
         fp_look_bro   = false;
         did_this_line = false;
         as.NewLines(pc->nl_count);
         as_bc.NewLines(pc->nl_count);
         as_at.NewLines(pc->nl_count);
         as_br.NewLines(pc->nl_count);

         if (p_nl_count != nullptr)
         {
            *p_nl_count += pc->nl_count;
         }
      }
      LOG_FMT(LAVDB, "%s(%d): pc->Text() is '%s', level is %zu, pc->brace_level is %zu\n",
              __func__, __LINE__, chunk_is_newline(pc) ? "Newline" : pc->Text(), pc->level, pc->brace_level);

      if (!chunk_is_newline(pc))
      {
         LOG_FMT(LAVDB, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() '%s', type is %s\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));

         if (chunk_is_not_token(pc, CT_IGNORED))
         {
            LOG_FMT(LAVDB, "   ");
            log_pcf_flags(LAVDB, pc->flags);
         }
      }

      // don't align stuff inside parenthesis/squares/angles
      if (pc->level > pc->brace_level)
      {
         pc = pc->GetNext();
         continue;
      }

      // If this is a variable def, update the max_col
      if (  !pc->flags.test(PCF_IN_CLASS_BASE)
         && chunk_is_not_token(pc, CT_FUNC_CLASS_DEF)
         && chunk_is_not_token(pc, CT_FUNC_CLASS_PROTO)
         && ((pc->flags & align_mask) == PCF_VAR_1ST)
         && chunk_is_not_token(pc, CT_FUNC_DEF)                                   // Issue 1452
         && (  (pc->level == (start->level + 1))
            || pc->level == 0)
         && pc->prev != nullptr
         && pc->prev->type != CT_MEMBER)
      {
         LOG_FMT(LAVDB, "%s(%d): a-did_this_line is %s\n",
                 __func__, __LINE__, did_this_line ? "TRUE" : "FALSE");
         LOG_FMT(LAVDB, "%s(%d): Text() is '%s', orig_line is %zu, orig_col is %zu, level is %zu\n",
                 __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col, pc->level);

         if (!did_this_line)
         {
            if (  get_chunk_parent_type(start) == CT_STRUCT
               && (as.m_star_style == AlignStack::SS_INCLUDE))
            {
               // we must look after the previous token
               Chunk *prev_local = pc->prev;

               while (  chunk_is_token(prev_local, CT_PTR_TYPE)
                     || chunk_is_token(prev_local, CT_ADDR))
               {
                  LOG_FMT(LAVDB, "%s(%d): prev_local '%s', prev_local->type %s\n",
                          __func__, __LINE__, prev_local->Text(), get_token_name(prev_local->type));
                  prev_local = prev_local->prev;
               }
               pc = prev_local->next;
            }
            // we must look after the previous token
            Chunk *prev_local = pc->prev;

            if (chunk_is_not_token(prev_local, CT_DEREF))                    // Issue #2971
            {
               LOG_FMT(LAVDB, "%s(%d): add = '%s', orig_line is %zu, orig_col is %zu, level is %zu\n",
                       __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col, pc->level);

               as.Add(step_back_over_member(pc));
            }
            log_rule_B("align_var_def_colon");

            if (options::align_var_def_colon())
            {
               next = pc->GetNextNc();

               if (chunk_is_token(next, CT_BIT_COLON))
               {
                  as_bc.Add(next);
               }
            }
            log_rule_B("align_var_def_attribute");

            if (options::align_var_def_attribute())
            {
               next = pc;

               while ((next = next->GetNextNc())->IsNotNullChunk())
               {
                  if (chunk_is_token(next, CT_ATTRIBUTE))
                  {
                     as_at.Add(next);
                     break;
                  }

                  if (  chunk_is_token(next, CT_SEMICOLON)
                     || chunk_is_newline(next))
                  {
                     break;
                  }
               }
            }
         }
         did_this_line = true;
      }
      else if (chunk_is_token(pc, CT_BIT_COLON))
      {
         if (!did_this_line)
         {
            as_bc.Add(pc);
            did_this_line = true;
         }
      }
      else
      {
         LOG_FMT(LAVDB, "%s(%d): b-did_this_line is %s\n",
                 __func__, __LINE__, did_this_line ? "TRUE" : "FALSE");
      }
      pc = pc->GetNext();
   }
   as.End();
   as_bc.End();
   as_at.End();
   as_br.End();

   return(pc);
} // align_var_def_brace
