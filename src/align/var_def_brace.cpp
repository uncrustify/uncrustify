/**
 * @file var_def_brace.cpp
 *
 * @author  Guy Maurel
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align/var_def_brace.h"

#include "align/stack.h"
#include "align/tools.h"
#include "log_rules.h"


constexpr static auto LCURRENT = LAVDB;


using namespace uncrustify;


Chunk *align_var_def_brace(Chunk *start, size_t span, size_t *p_nl_count)
{
   LOG_FUNC_ENTRY();

   if (start->IsNullChunk())
   {
      return(Chunk::NullChunkPtr);
   }
   Chunk  *next;
   size_t myspan   = span;
   size_t mythresh = 0;
   size_t mygap    = 0;

   // Override the span, if this is a struct/union
   if (  start->GetParentType() == CT_STRUCT
      || start->GetParentType() == CT_UNION)
   {
      log_rule_B("align_var_struct_span");
      myspan = options::align_var_struct_span();
      log_rule_B("align_var_struct_thresh");
      mythresh = options::align_var_struct_thresh();
      log_rule_B("align_var_struct_gap");
      mygap = options::align_var_struct_gap();
   }
   else if (start->GetParentType() == CT_CLASS)
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

   if (prev->Is(CT_ASSIGN))
   {
      LOG_FMT(LAVDB, "%s(%d): start->Text() '%s', type is %s, on orig line %zu (abort due to assign)\n",
              __func__, __LINE__, start->Text(), get_token_name(start->GetType()), start->GetOrigLine());

      Chunk *pc = start->GetNextType(CT_BRACE_CLOSE, start->GetLevel());
      return(pc->GetNextNcNnl());
   }
   char copy[1000];

   LOG_FMT(LAVDB, "%s(%d): start->Text() '%s', type is %s, on orig line %zu\n",
           __func__, __LINE__, start->ElidedText(copy), get_token_name(start->GetType()), start->GetOrigLine());

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
   LOG_FMT(LAVDB, "%s(%d): start->Text() is '%s', level is %zu, brace level is %zu\n",
           __func__, __LINE__, start->IsNewline() ? "Newline" : start->Text(), start->GetLevel(), start->GetBraceLevel());

   while (pc->IsNotNullChunk())
   {
      LOG_CHUNK(LAVDB, pc);

      if (  pc->GetLevel() < start->GetLevel()
         && pc->GetLevel() != 0
         && !pc->IsPreproc())
      {
         LOG_FMT(LAVDB, "%s(%d): orig line is %zu, orig col is %zu, Text() '%s', type is %s, PRE is %s\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), get_token_name(pc->GetType()), pc->IsPreproc() ? "true" : "false");
         break;
      }

      if (pc->IsComment())
      {
         if (pc->GetNlCount() > 0)
         {
            as.NewLines(pc->GetNlCount());
            as_bc.NewLines(pc->GetNlCount());
            as_at.NewLines(pc->GetNlCount());
            as_br.NewLines(pc->GetNlCount());
         }
         pc = pc->GetNext();
         LOG_FMT(LAVDB, "%s(%d): pc->Text() is '%s', level is %zu, brace level is %zu\n",
                 __func__, __LINE__, pc->IsNewline() ? "Newline" : pc->Text(), pc->GetLevel(), pc->GetBraceLevel());
         continue;
      }

      if (  fp_active
         && !pc->TestFlags(PCF_IN_CLASS_BASE))
      {
         // WARNING: Duplicate from the align_func_proto()
         log_rule_B("align_single_line_func");

         if (  pc->Is(CT_FUNC_PROTO)
            || (  pc->Is(CT_FUNC_DEF)
               && options::align_single_line_func()))
         {
            LOG_FMT(LAVDB, "%s(%d): add = '%s', orig line is %zu, orig col is %zu, level is %zu\n",
                    __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel());

            Chunk *toadd;

            log_rule_B("align_on_operator");

            if (  pc->GetParentType() == CT_OPERATOR
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
            fp_look_bro = (pc->Is(CT_FUNC_DEF))
                          && options::align_single_line_brace();
         }
         else if (  fp_look_bro
                 && pc->Is(CT_BRACE_OPEN)
                 && pc->TestFlags(PCF_ONE_LINER))
         {
            as_br.Add(pc);
            fp_look_bro = false;
         }
      }

      // process nested braces
      if (pc->Is(CT_BRACE_OPEN))
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
      if (pc->Is(CT_BRACE_CLOSE))
      {
         pc = pc->GetNext();
         LOG_FMT(LAVDB, "%s(%d): pc->Text() is '%s', level is %zu, brace level is %zu\n",
                 __func__, __LINE__, pc->IsNewline() ? "Newline" : pc->Text(), pc->GetLevel(), pc->GetBraceLevel());
         break;
      }

      if (pc->IsNewline())
      {
         fp_look_bro   = false;
         did_this_line = false;
         as.NewLines(pc->GetNlCount());
         as_bc.NewLines(pc->GetNlCount());
         as_at.NewLines(pc->GetNlCount());
         as_br.NewLines(pc->GetNlCount());

         if (p_nl_count != nullptr)
         {
            *p_nl_count += pc->GetNlCount();
         }
      }
      LOG_FMT(LAVDB, "%s(%d): pc->Text() is '%s', level is %zu, brace level is %zu\n",
              __func__, __LINE__, pc->IsNewline() ? "Newline" : pc->Text(), pc->GetLevel(), pc->GetBraceLevel());

      if (!pc->IsNewline())
      {
         LOG_FMT(LAVDB, "%s(%d): pc orig line is %zu, orig col is %zu, Text() '%s', type is %s\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), get_token_name(pc->GetType()));

         if (pc->IsNot(CT_IGNORED))
         {
            LOG_FMT(LAVDB, "   ");
            log_pcf_flags(LAVDB, pc->GetFlags());
         }
      }

      // don't align stuff inside parenthesis/squares/angles
      if (pc->GetLevel() > pc->GetBraceLevel())
      {
         pc = pc->GetNext();
         LOG_FMT(LAVDB, "%s(%d): pc->Text() is '%s', level is %zu, brace level is %zu\n",
                 __func__, __LINE__, pc->IsNewline() ? "Newline" : pc->Text(), pc->GetLevel(), pc->GetBraceLevel());
         continue;
      }

      // If this is a variable def, update the max_col
      if (  !pc->TestFlags(PCF_IN_CLASS_BASE)
         && pc->IsNot(CT_FUNC_CLASS_DEF)
         && pc->IsNot(CT_FUNC_CLASS_PROTO)
         && ((pc->GetFlags() & align_mask) == PCF_VAR_1ST)
         && pc->IsNot(CT_FUNC_DEF)                                   // Issue 1452
         && (  (pc->GetLevel() == (start->GetLevel() + 1))
            || pc->GetLevel() == 0)
         && pc->GetPrev()->IsNot(CT_MEMBER))
      {
         LOG_FMT(LAVDB, "%s(%d): a-did_this_line is %s\n",
                 __func__, __LINE__, did_this_line ? "TRUE" : "FALSE");
         LOG_FMT(LAVDB, "%s(%d): Text() is '%s', orig line is %zu, orig col is %zu, level is %zu\n",
                 __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel());

         if (!did_this_line)
         {
            if (  start->GetParentType() == CT_STRUCT
               && (as.m_star_style == AlignStack::SS_INCLUDE))
            {
               // we must look after the previous token
               Chunk *prev_local = pc->GetPrev();

               while (  prev_local->Is(CT_PTR_TYPE)
                     || prev_local->Is(CT_ADDR))
               {
                  LOG_FMT(LAVDB, "%s(%d): prev_local '%s', prev_local->GetType() %s\n",
                          __func__, __LINE__, prev_local->Text(), get_token_name(prev_local->GetType()));
                  prev_local = prev_local->GetPrev();
               }
               pc = prev_local->GetNext();
               LOG_FMT(LAVDB, "%s(%d): pc->Text() is '%s', level is %zu, brace level is %zu\n",
                       __func__, __LINE__, pc->IsNewline() ? "Newline" : pc->Text(), pc->GetLevel(), pc->GetBraceLevel());
            }
            // we must look after the previous token
            Chunk *prev_local = pc->GetPrev();

            if (prev_local->IsNot(CT_DEREF))                    // Issue #2971
            {
               LOG_FMT(LAVDB, "%s(%d): add = '%s', orig line is %zu, orig col is %zu, level is %zu\n",
                       __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel());

               as.Add(step_back_over_member(pc));
            }
            log_rule_B("align_var_def_colon");

            if (options::align_var_def_colon())
            {
               next = pc->GetNextNc();
               LOG_FMT(LAVDB, "%s(%d): pc->Text() is '%s', level is %zu, brace level is %zu\n",
                       __func__, __LINE__, pc->IsNewline() ? "Newline" : pc->Text(), pc->GetLevel(), pc->GetBraceLevel());

               if (next->Is(CT_BIT_COLON))
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
                  if (next->Is(CT_ATTRIBUTE))
                  {
                     as_at.Add(next);
                     break;
                  }

                  if (  next->Is(CT_SEMICOLON)
                     || next->IsNewline())
                  {
                     break;
                  }
               }
            }
         }
         did_this_line = true;
      }
      else if (pc->Is(CT_BIT_COLON))
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
      LOG_FMT(LAVDB, "%s(%d): pc->Text() is '%s', level is %zu, brace level is %zu\n",
              __func__, __LINE__, pc->IsNewline() ? "Newline" : pc->Text(), pc->GetLevel(), pc->GetBraceLevel());
   }
   as.End();
   as_bc.End();
   as_at.End();
   as_br.End();

   return(pc);
} // align_var_def_brace
