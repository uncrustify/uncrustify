/**
 * @file align_oc_decl_colon.cpp
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align_oc_decl_colon.h"

#include "align_oc_msg_colons.h"
#include "align_stack.h"
#include "chunk_list.h"


using namespace uncrustify;


void align_oc_decl_colon(void)
{
   LOG_FUNC_ENTRY();

   bool       did_line;
   AlignStack cas;   // for the colons
   AlignStack nas;   // for the parameter label
   cas.Start(4);
   nas.Start(4);
   nas.m_right_align = !options::align_on_tabstop();

   chunk_t *pc = chunk_get_head();
   while (pc != nullptr)
   {
      if (pc->type != CT_OC_SCOPE)
      {
         pc = chunk_get_next(pc);
         continue;
      }

      nas.Reset();
      cas.Reset();

      size_t level = pc->level;
      pc = chunk_get_next_ncnl(pc, scope_e::PREPROC);

      did_line = false;

      while (pc != nullptr && pc->level >= level)
      {
         // The declaration ends with an open brace or semicolon
         if (chunk_is_token(pc, CT_BRACE_OPEN) || chunk_is_semicolon(pc))
         {
            break;
         }

         if (chunk_is_newline(pc))
         {
            nas.NewLines(pc->nl_count);
            cas.NewLines(pc->nl_count);
            did_line = false;
         }
         else if (!did_line && chunk_is_token(pc, CT_OC_COLON))
         {
            cas.Add(pc);

            chunk_t *tmp  = chunk_get_prev(pc, scope_e::PREPROC);
            chunk_t *tmp2 = chunk_get_prev_ncnl(tmp, scope_e::PREPROC);

            // Check for an un-labeled parameter
            if (  tmp != nullptr
               && tmp2 != nullptr
               && (  chunk_is_token(tmp, CT_WORD)
                  || chunk_is_token(tmp, CT_TYPE)
                  || chunk_is_token(tmp, CT_OC_MSG_DECL)
                  || chunk_is_token(tmp, CT_OC_MSG_SPEC))
               && (  chunk_is_token(tmp2, CT_WORD)
                  || chunk_is_token(tmp2, CT_TYPE)
                  || chunk_is_token(tmp2, CT_PAREN_CLOSE)))
            {
               nas.Add(tmp);
            }
            did_line = true;
         }
         pc = chunk_get_next(pc, scope_e::PREPROC);
      }
      nas.End();
      cas.End();
   }
} // align_oc_decl_colon
