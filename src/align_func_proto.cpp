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

using namespace uncrustify;


void align_func_proto(size_t span)
{
   LOG_FUNC_ENTRY();

   size_t mythresh = 0;
   mythresh = options::align_func_proto_thresh();

   AlignStack as;
   as.Start(span, mythresh);
   as.m_gap        = options::align_func_proto_gap();
   as.m_star_style = static_cast<AlignStack::StarStyle>(options::align_var_def_star_style());
   as.m_amp_style  = static_cast<AlignStack::StarStyle>(options::align_var_def_amp_style());

   AlignStack as_br;
   as_br.Start(span, 0);
   as_br.m_gap = options::align_single_line_brace_gap();

   bool    look_bro = false;
   chunk_t *toadd;

   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next(pc))
   {
      if (chunk_is_newline(pc))
      {
         look_bro = false;
         as.NewLines(pc->nl_count);
         as_br.NewLines(pc->nl_count);
      }
      else if (  chunk_is_token(pc, CT_FUNC_PROTO)
              || (  chunk_is_token(pc, CT_FUNC_DEF)
                 && options::align_single_line_func()))
      {
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
         look_bro = (chunk_is_token(pc, CT_FUNC_DEF))
                    && options::align_single_line_brace();
      }
      else if (  look_bro
              && chunk_is_token(pc, CT_BRACE_OPEN)
              && (pc->flags & PCF_ONE_LINER))
      {
         as_br.Add(pc);
         look_bro = false;
      }
   }
   as.End();
   as_br.End();
} // align_func_proto
