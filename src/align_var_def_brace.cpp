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
#include "uncrustify.h"

using namespace uncrustify;


chunk_t *align_var_def_brace(chunk_t *start, size_t span, size_t *p_nl_count)
{
   LOG_FUNC_ENTRY();

   if (start == nullptr)
   {
      return(nullptr);
   }

   chunk_t *next;
   size_t  myspan   = span;
   size_t  mythresh = 0;
   size_t  mygap    = 0;

   // Override the span, if this is a struct/union
   if (start->parent_type == CT_STRUCT || start->parent_type == CT_UNION)
   {
      myspan   = options::align_var_struct_span();
      mythresh = options::align_var_struct_thresh();
      mygap    = options::align_var_struct_gap();
   }
   else if (start->parent_type == CT_CLASS)
   {
      myspan   = options::align_var_class_span();
      mythresh = options::align_var_class_thresh();
      mygap    = options::align_var_class_gap();
   }
   else
   {
      mythresh = options::align_var_def_thresh();
      mygap    = options::align_var_def_gap();
   }

   // can't be any variable definitions in a "= {" block
   chunk_t *prev = chunk_get_prev_ncnl(start);
   if (chunk_is_token(prev, CT_ASSIGN))
   {
      LOG_FMT(LAVDB, "%s(%d): start->text() '%s', type is %s, on orig_line %zu (abort due to assign)\n",
              __func__, __LINE__, start->text(), get_token_name(start->type), start->orig_line);

      chunk_t *pc = chunk_get_next_type(start, CT_BRACE_CLOSE, start->level);
      return(chunk_get_next_ncnl(pc));
   }

   LOG_FMT(LAVDB, "%s(%d): start->text() '%s', type is %s, on orig_line %zu\n",
           __func__, __LINE__, start->text(), get_token_name(start->type), start->orig_line);

   UINT64 align_mask = PCF_IN_FCN_DEF | PCF_VAR_1ST;
   if (!options::align_var_def_inline())
   {
      align_mask |= PCF_VAR_INLINE;
   }

   // Set up the variable/prototype/definition aligner
   AlignStack as;
   as.Start(myspan, mythresh);
   as.m_gap        = mygap;
   as.m_star_style = static_cast<AlignStack::StarStyle>(options::align_var_def_star_style());
   as.m_amp_style  = static_cast<AlignStack::StarStyle>(options::align_var_def_amp_style());

   // Set up the bit colon aligner
   AlignStack as_bc;
   as_bc.Start(myspan, 0);
   as_bc.m_gap = options::align_var_def_colon_gap();

   AlignStack as_at; // attribute
   as_at.Start(myspan, 0);

   // Set up the brace open aligner
   AlignStack as_br;
   as_br.Start(myspan, mythresh);
   as_br.m_gap = options::align_single_line_brace_gap();

   bool    fp_look_bro   = false;
   bool    did_this_line = false;
   bool    fp_active     = options::align_mix_var_proto();
   chunk_t *pc           = chunk_get_next(start);
   while (  pc != nullptr
         && (pc->level >= start->level || pc->level == 0))
   {
      if (chunk_is_newline(pc))
      {
         LOG_FMT(LAVDB, "%s(%d): orig_line is %zu, orig_col is %zu, <Newline>\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col);
      }
      else
      {
         LOG_FMT(LAVDB, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s', type is %s\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
      }
      if (chunk_is_comment(pc))
      {
         if (pc->nl_count > 0)
         {
            as.NewLines(pc->nl_count);
            as_bc.NewLines(pc->nl_count);
            as_at.NewLines(pc->nl_count);
            as_br.NewLines(pc->nl_count);
         }
         pc = chunk_get_next(pc);
         continue;
      }

      if (fp_active && !(pc->flags & PCF_IN_CLASS_BASE))
      {
         // WARNING: Duplicate from the align_func_proto()
         if (  chunk_is_token(pc, CT_FUNC_PROTO)
            || (  chunk_is_token(pc, CT_FUNC_DEF)
               && options::align_single_line_func()))
         {
            LOG_FMT(LAVDB, "%s(%d): add=[%s], orig_line is %zu, orig_col is %zu, level is %zu\n",
                    __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col, pc->level);

            chunk_t *toadd;
            if (  pc->parent_type == CT_OPERATOR
               && options::align_on_operator())
            {
               toadd = chunk_get_prev_ncnl(pc);
            }
            else
            {
               toadd = pc;
            }
            as.Add(step_back_over_member(toadd));
            fp_look_bro = (chunk_is_token(pc, CT_FUNC_DEF))
                          && options::align_single_line_brace();
         }
         else if (  fp_look_bro
                 && chunk_is_token(pc, CT_BRACE_OPEN)
                 && (pc->flags & PCF_ONE_LINER))
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
         pc = chunk_get_next(pc);
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

      LOG_FMT(LAVDB, "%s(%d): pc->text() is '%s', level is %zu, pc->brace_level is %zu\n",
              __func__, __LINE__, chunk_is_newline(pc) ? "Newline" : pc->text(), pc->level, pc->brace_level);
      if (!chunk_is_newline(pc))
      {
         LOG_FMT(LAVDB, "%s(%d): pc->orig_line is %zu, orig_col is %zu, text() '%s', type is %s\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
         if (!chunk_is_token(pc, CT_IGNORED))
         {
            LOG_FMT(LAVDB, "   ");
            log_pcf_flags(LAVDB, pc->flags);
         }
      }
      // don't align stuff inside parenthesis/squares/angles
      if (pc->level > pc->brace_level)
      {
         pc = chunk_get_next(pc);
         continue;
      }

      // If this is a variable def, update the max_col
      if (  !(pc->flags & PCF_IN_CLASS_BASE)
         && pc->type != CT_FUNC_CLASS_DEF
         && pc->type != CT_FUNC_CLASS_PROTO
         && ((pc->flags & align_mask) == PCF_VAR_1ST)
         && pc->type != CT_FUNC_DEF                                   // Issue 1452
         && ((pc->level == (start->level + 1)) || pc->level == 0)
         && pc->prev != nullptr
         && pc->prev->type != CT_MEMBER)
      {
         LOG_FMT(LAVDB, "%s(%d): did_this_line is %s\n",
                 __func__, __LINE__, did_this_line ? "TRUE" : "FALSE");
         LOG_FMT(LAVDB, "%s(%d): text() is '%s', orig_line is %zu, orig_col is %zu, level is %zu\n",
                 __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col, pc->level);
         if (!did_this_line)
         {
            if (  start->parent_type == CT_STRUCT
               && (as.m_star_style == AlignStack::SS_INCLUDE))
            {
               // we must look after the previous token
               chunk_t *prev_local = pc->prev;
               while (  chunk_is_token(prev_local, CT_PTR_TYPE)
                     || chunk_is_token(prev_local, CT_ADDR))
               {
                  LOG_FMT(LAVDB, "%s(%d): prev_local '%s', prev_local->type %s\n",
                          __func__, __LINE__, prev_local->text(), get_token_name(prev_local->type));
                  prev_local = prev_local->prev;
               }
               pc = prev_local->next;
            }
            LOG_FMT(LAVDB, "%s(%d): add = '%s', orig_line is %zu, orig_col is %zu, level is %zu\n",
                    __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col, pc->level);

            as.Add(step_back_over_member(pc));

            if (options::align_var_def_colon())
            {
               next = chunk_get_next_nc(pc);
               if (chunk_is_token(next, CT_BIT_COLON))
               {
                  as_bc.Add(next);
               }
            }
            if (options::align_var_def_attribute())
            {
               next = pc;
               while ((next = chunk_get_next_nc(next)) != nullptr)
               {
                  if (chunk_is_token(next, CT_ATTRIBUTE))
                  {
                     as_at.Add(next);
                     break;
                  }
                  if (chunk_is_token(next, CT_SEMICOLON) || chunk_is_newline(next))
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
      pc = chunk_get_next(pc);
   }

   as.End();
   as_bc.End();
   as_at.End();
   as_br.End();

   return(pc);
} // align_var_def_brace
