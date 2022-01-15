/**
 * @file align_oc_decl_colon.cpp
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align_oc_decl_colon.h"

#include "align_stack.h"
#include "chunk.h"


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

   Chunk *pc = chunk_get_head();

   while (  pc != nullptr
         && pc->isNotNullChunk())
   {
      if (chunk_is_not_token(pc, CT_OC_SCOPE))
      {
         pc = pc->get_next();
         continue;
      }
      nas.Reset();
      cas.Reset();

      size_t level = pc->level;
      pc = chunk_get_next_nc_nnl(pc, scope_e::PREPROC);

      did_line = false;

      while (  pc != nullptr
            && pc->isNotNullChunk()
            && pc->level >= level)
      {
         // The declaration ends with an open brace or semicolon
         if (  chunk_is_token(pc, CT_BRACE_OPEN)
            || chunk_is_semicolon(pc))
         {
            break;
         }

         if (chunk_is_newline(pc))
         {
            nas.NewLines(pc->nl_count);
            cas.NewLines(pc->nl_count);
            did_line = false;
         }
         else if (  !did_line
                 && chunk_is_token(pc, CT_OC_COLON))
         {
            cas.Add(pc);

            Chunk *tmp  = pc->get_prev(scope_e::PREPROC);
            Chunk *tmp2 = chunk_get_prev_nc_nnl(tmp, scope_e::PREPROC);

            // Check for an un-labeled parameter
            if (  (  chunk_is_token(tmp, CT_WORD)
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
         pc = pc->get_next(scope_e::PREPROC);
      }
      nas.End();
      cas.End();
   }
} // align_oc_decl_colon
