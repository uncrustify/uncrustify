/**
 * @file align_assign.cpp
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align_assign.h"
#include "align_stack.h"
#include "uncrustify.h"

using namespace uncrustify;


chunk_t *align_assign(chunk_t *first, size_t span, size_t thresh, size_t *p_nl_count)
{
   LOG_FUNC_ENTRY();

   if (first == nullptr)
   {
      // coveralls will complain here. There are no example for that.
      // see https://en.wikipedia.org/wiki/Robustness_principle
      return(nullptr);
   }
   size_t my_level = first->level;

   LOG_FMT(LALASS, "%s(%d): [my_level is %zu]: start checking with '%s', on orig_line %zu, span is %zu, thresh is %zu\n",
           __func__, __LINE__, my_level, first->text(), first->orig_line, span, thresh);

   // If we are aligning on a tabstop, we shouldn't right-align
   AlignStack as;    // regular assigns
   as.Start(span, thresh);
   as.m_right_align = !options::align_on_tabstop();

   AlignStack vdas;  // variable def assigns
   vdas.Start(span, thresh);
   vdas.m_right_align = as.m_right_align;

   AlignStack fcnDefault;
   fcnDefault.Start(span, thresh);
   fcnDefault.m_right_align = as.m_right_align;

   AlignStack fcnProto;
   fcnProto.Start(span, thresh);
   fcnProto.m_right_align = as.m_right_align;

   size_t  var_def_cnt = 0;
   size_t  equ_count   = 0;
   size_t  tmp;
   chunk_t *pc = first;
   while (pc != nullptr)
   {
      LOG_FMT(LALASS, "%s(%d): orig_line is %zu, check pc->text() '%s', type is %s, parent_type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->text(), get_token_name(pc->type), get_token_name(pc->parent_type));

      // Don't check inside SPAREN, PAREN or SQUARE groups
      if (  chunk_is_token(pc, CT_SPAREN_OPEN)
            // || chunk_is_token(pc, CT_FPAREN_OPEN) Issue #1340
         || chunk_is_token(pc, CT_SQUARE_OPEN)
         || chunk_is_token(pc, CT_PAREN_OPEN))
      {
         LOG_FMT(LALASS, "%s(%d): Don't check inside SPAREN, PAREN or SQUARE groups, type is %s\n",
                 __func__, __LINE__, get_token_name(pc->type));
         tmp = pc->orig_line;
         pc  = chunk_skip_to_match(pc);
         if (pc != nullptr)
         {
            as.NewLines(pc->orig_line - tmp);
            vdas.NewLines(pc->orig_line - tmp);
            fcnDefault.NewLines(pc->orig_line - tmp);
            fcnProto.NewLines(pc->orig_line - tmp);
         }
         continue;
      }


      // Recurse if a brace set is found
      if (chunk_is_token(pc, CT_BRACE_OPEN) || chunk_is_token(pc, CT_VBRACE_OPEN))
      {
         size_t myspan;
         size_t mythresh;

         size_t sub_nl_count = 0;

         if (pc->parent_type == CT_ENUM)
         {
            myspan   = options::align_enum_equ_span();
            mythresh = options::align_enum_equ_thresh();
         }
         else
         {
            myspan   = options::align_assign_span();
            mythresh = options::align_assign_thresh();
         }

         pc = align_assign(chunk_get_next_ncnl(pc), myspan, mythresh, &sub_nl_count);
         if (sub_nl_count > 0)
         {
            as.NewLines(sub_nl_count);
            vdas.NewLines(sub_nl_count);
            fcnDefault.NewLines(sub_nl_count);
            fcnProto.NewLines(sub_nl_count);
            if (p_nl_count != nullptr)
            {
               *p_nl_count += sub_nl_count;
            }
         }
         continue;
      }

      // Done with this brace set?
      if (chunk_is_token(pc, CT_BRACE_CLOSE) || chunk_is_token(pc, CT_VBRACE_CLOSE))
      {
         pc = chunk_get_next(pc);
         break;
      }


      if (chunk_is_newline(pc))
      {
         as.NewLines(pc->nl_count);
         vdas.NewLines(pc->nl_count);
         fcnDefault.NewLines(pc->nl_count);
         fcnProto.NewLines(pc->nl_count);

         if (p_nl_count != nullptr)
         {
            *p_nl_count += pc->nl_count;
         }

         var_def_cnt = 0;
         equ_count   = 0;
      }
      else if (  (pc->flags & PCF_VAR_DEF)
              && !(pc->flags & PCF_IN_CONST_ARGS) // Issue #1717
              && !(pc->flags & PCF_IN_FCN_DEF)    // Issue #1717
              && !(pc->flags & PCF_IN_FCN_CALL))  // Issue #1717
      {
         LOG_FMT(LALASS, "%s(%d): log_pcf_flags pc->flags:\n   ", __func__, __LINE__);
         log_pcf_flags(LALASS, pc->flags);
         var_def_cnt++;
      }
      else if (var_def_cnt > 1)
      {
         // we hit the second variable def - don't look for assigns, don't align
         vdas.Reset();
      }
      else if (  equ_count == 0                      // indent only if first '=' in line
              && (pc->flags & PCF_IN_TEMPLATE) == 0  // and it is not inside a template #999
              && (  chunk_is_token(pc, CT_ASSIGN)
                 || chunk_is_token(pc, CT_ASSIGN_DEFAULT_ARG)
                 || chunk_is_token(pc, CT_ASSIGN_FUNC_PROTO)))
      {
         if (chunk_is_token(pc, CT_ASSIGN))               // Issue #2236
         {
            equ_count++;
         }
         LOG_FMT(LALASS, "%s(%d): align_assign_decl_func() is %d\n",
                 __func__, __LINE__, options::align_assign_decl_func());
         LOG_FMT(LALASS, "%s(%d): log_pcf_flags pc->flags: ", __func__, __LINE__);
         log_pcf_flags(LALASS, pc->flags);

         if (  options::align_assign_decl_func() == 0         // Align with other assignments (default)
            && (  chunk_is_token(pc, CT_ASSIGN_DEFAULT_ARG)   // Foo( int bar = 777 );
               || chunk_is_token(pc, CT_ASSIGN_FUNC_PROTO)))  // Foo( const Foo & ) = delete;
         {
            LOG_FMT(LALASS, "%s(%d): fcnDefault.Add on '%s' on orig_line %zu, orig_col is %zu\n",
                    __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
            fcnDefault.Add(pc);
         }
         else if (options::align_assign_decl_func() == 1)   // Align with each other
         {
            if (chunk_is_token(pc, CT_ASSIGN_DEFAULT_ARG))  // Foo( int bar = 777 );
            {
               LOG_FMT(LALASS, "%s(%d): default: fcnDefault.Add on '%s' on orig_line %zu, orig_col is %zu\n",
                       __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
               fcnDefault.Add(pc);
            }
            else if (chunk_is_token(pc, CT_ASSIGN_FUNC_PROTO))  // Foo( const Foo & ) = delete;
            {
               LOG_FMT(LALASS, "%s(%d): proto: fcnProto.Add on '%s' on orig_line %zu, orig_col is %zu\n",
                       __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
               fcnProto.Add(pc);
            }
            else if (chunk_is_token(pc, CT_ASSIGN)) // Issue #2197
            {
               LOG_FMT(LALASS, "%s(%d): vdas.Add on '%s' on orig_line %zu, orig_col is %zu\n",
                       __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
               vdas.Add(pc);
            }
         }
         else if (  options::align_assign_decl_func() == 2         // Don't align
                 && (  chunk_is_token(pc, CT_ASSIGN_DEFAULT_ARG)   // Foo( int bar = 777 );
                    || chunk_is_token(pc, CT_ASSIGN_FUNC_PROTO)))  // Foo( const Foo & ) = delete;
         {
            LOG_FMT(LALASS, "%s(%d): Don't align\n",               // Issue #2236
                    __func__, __LINE__);
         }
         else if (var_def_cnt != 0)
         {
            LOG_FMT(LALASS, "%s(%d): vdas.Add on '%s' on orig_line %zu, orig_col is %zu\n",
                    __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
            vdas.Add(pc);
         }
         else
         {
            if (chunk_is_token(pc, CT_ASSIGN))
            //if (options::align_assign_decl_func() != 2)
            {
               LOG_FMT(LALASS, "%s(%d): as.Add on '%s' on orig_line %zu, orig_col is %zu\n",
                       __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
               as.Add(pc);
            }
         }
      }

      pc = chunk_get_next(pc);
   }

   as.End();
   vdas.End();
   fcnDefault.End();
   fcnProto.End();

   if (pc != nullptr)
   {
      LOG_FMT(LALASS, "%s(%d): done on '%s' on orig_line %zu\n",
              __func__, __LINE__, pc->text(), pc->orig_line);
   }
   else
   {
      LOG_FMT(LALASS, "%s(%d): done on NULL\n", __func__, __LINE__);
   }

   return(pc);
} // align_assign

