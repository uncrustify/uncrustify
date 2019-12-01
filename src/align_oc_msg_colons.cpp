/**
 * @file align_oc_msg_colons.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "align_oc_msg_colons.h"
#include "align_stack.h"
#include "chunk_list.h"
#include "uncrustify_types.h"

using namespace uncrustify;


void align_oc_msg_colon(chunk_t *so)
{
   LOG_FUNC_ENTRY();

   AlignStack nas;   // for the parameter tag
   nas.Reset();
   nas.m_right_align = !options::align_on_tabstop();

   AlignStack cas;   // for the colons
   size_t     span = options::align_oc_msg_colon_span();
   cas.Start(span);

   size_t  level = so->level;
   chunk_t *pc   = chunk_get_next_ncnl(so, scope_e::PREPROC);

   bool    did_line   = false;
   bool    has_colon  = false;
   size_t  lcnt       = 0; // line count with no colon for span
   bool    first_line = true;

   while (pc != nullptr && pc->level > level)
   {
      if (pc->level > (level + 1))
      {
         // do nothing
      }
      else if (chunk_is_newline(pc))
      {
         if (!has_colon)
         {
            ++lcnt;
         }
         did_line = false;

         if (options::align_oc_msg_colon_xcode_like() && first_line && !has_colon)
         {
            span = 0;
         }
         has_colon  = !has_colon;
         first_line = false;
      }
      else if (  !did_line
              && (lcnt < span + 1)
              && chunk_is_token(pc, CT_OC_COLON))
      {
         has_colon = true;
         cas.Add(pc);
         chunk_t *tmp = chunk_get_prev(pc);

         if (  tmp != nullptr
            && (chunk_is_token(tmp, CT_OC_MSG_FUNC) || chunk_is_token(tmp, CT_OC_MSG_NAME)))
         {
            nas.Add(tmp);
            chunk_flags_set(tmp, PCF_DONT_INDENT);
         }
         did_line = true;
      }
      pc = chunk_get_next(pc, scope_e::PREPROC);
   }
   nas.m_skip_first = !options::align_oc_msg_colon_first();
   cas.m_skip_first = !options::align_oc_msg_colon_first();

   // find the longest args that isn't the first one
   size_t  first_len = 0;
   size_t  mlen      = 0;
   chunk_t *longest  = nullptr;

   size_t  len = nas.m_aligned.Len();

   for (size_t idx = 0; idx < len; idx++)
   {
      chunk_t *tmp = nas.m_aligned.GetChunk(idx);

      if (tmp != nullptr)
      {
         size_t tlen = tmp->str.size();

         if (tlen > mlen)
         {
            mlen = tlen;

            if (idx != 0)
            {
               longest = tmp;
            }
         }

         if (idx == 0)
         {
            first_len = tlen + 1;
         }
      }
   }

   // add spaces before the longest arg
   len = options::indent_oc_msg_colon();
   size_t len_diff    = mlen - first_len;
   size_t indent_size = options::indent_columns();

   // Align with first colon if possible by removing spaces
   if (  longest
      && options::indent_oc_msg_prioritize_first_colon()
      && len_diff > 0
      && ((longest->column >= len_diff) && (longest->column - len_diff) > (longest->brace_level * indent_size)))
   {
      longest->column -= len_diff;
   }
   else if (longest && len > 0)
   {
      chunk_t chunk;

      chunk.type        = CT_SPACE;
      chunk.orig_line   = longest->orig_line;
      chunk.orig_col    = longest->orig_col;
      chunk.parent_type = CT_NONE;
      chunk.level       = longest->level;
      chunk.brace_level = longest->brace_level;
      chunk.flags       = longest->flags & PCF_COPY_FLAGS;

      // start at one since we already indent for the '['
      for (size_t idx = 1; idx < len; idx++)
      {
         chunk.str.append(' ');
      }

      chunk_add_before(&chunk, longest);
   }
   nas.End();
   cas.End();
} // align_oc_msg_colon


void align_oc_msg_colons(void)
{
   LOG_FUNC_ENTRY();

   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next(pc))
   {
      if (chunk_is_token(pc, CT_SQUARE_OPEN) && pc->parent_type == CT_OC_MSG)
      {
         align_oc_msg_colon(pc);
      }
   }
} // align_oc_msg_colons
