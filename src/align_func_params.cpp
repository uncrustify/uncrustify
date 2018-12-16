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

using namespace uncrustify;


chunk_t *align_func_param(chunk_t *start)
{
   LOG_FUNC_ENTRY();

   // Defaults, if the align_func_params = true
   size_t myspan   = 2;
   size_t mythresh = 0;
   size_t mygap    = 0;
   // Override, if the align_func_params_span > 0
   if (options::align_func_params_span() > 0)
   {
      myspan   = options::align_func_params_span();
      mythresh = options::align_func_params_thresh();
      mygap    = options::align_func_params_gap();
   }

   AlignStack as;
   as.Start(myspan, mythresh);
   as.m_gap        = mygap;
   as.m_star_style = static_cast<AlignStack::StarStyle>(options::align_var_def_star_style());
   as.m_amp_style  = static_cast<AlignStack::StarStyle>(options::align_var_def_amp_style());

   bool    did_this_line = false;
   size_t  comma_count   = 0;
   size_t  chunk_count   = 0;

   chunk_t *pc = start;
   while ((pc = chunk_get_next(pc)) != nullptr)
   {
      chunk_count++;
      if (chunk_is_newline(pc))
      {
         did_this_line = false;
         comma_count   = 0;
         chunk_count   = 0;
         as.NewLines(pc->nl_count);
      }
      else if (pc->level <= start->level)
      {
         break;
      }
      else if (!did_this_line && (pc->flags & PCF_VAR_DEF))
      {
         if (chunk_count > 1)
         {
            as.Add(pc);
         }
         did_this_line = true;
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
      as.End();
   }

   return(pc);
} // align_func_param


void align_func_params(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc = chunk_get_head();
   while ((pc = chunk_get_next(pc)) != nullptr)
   {
      if (  pc->type != CT_FPAREN_OPEN
         || (  pc->parent_type != CT_FUNC_PROTO
            && pc->parent_type != CT_FUNC_DEF
            && pc->parent_type != CT_FUNC_CLASS_PROTO
            && pc->parent_type != CT_FUNC_CLASS_DEF
            && pc->parent_type != CT_TYPEDEF))
      {
         continue;
      }

      // We are on a open parenthesis of a prototype
      pc = align_func_param(pc);
   }
} // void align_func_params
