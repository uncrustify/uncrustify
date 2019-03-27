/**
 * @file flag_parens.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "flag_parens.h"
#include "uncrustify.h"


chunk_t *flag_parens(chunk_t *po, UINT64 flags, c_token_t opentype, c_token_t parenttype, bool parent_all)
{
   LOG_FUNC_ENTRY();
   chunk_t *paren_close;

   paren_close = chunk_skip_to_match(po, scope_e::PREPROC);
   if (paren_close == nullptr)
   {
      LOG_FMT(LERR, "%s(%d): no match for '%s' at [%zu:%zu]",
              __func__, __LINE__, po->text(), po->orig_line, po->orig_col);
      log_func_stack_inline(LERR);
      cpd.error_count++;
      return(nullptr);
   }

   LOG_FMT(LFLPAREN, "%s(%d): between  po is '%s', orig_line is %zu, orig_col is %zu, and\n",
           __func__, __LINE__, po->text(), po->orig_line, po->orig_col);
   LOG_FMT(LFLPAREN, "%s(%d): paren_close is '%s', orig_line is %zu, orig_col is %zu, type is %s, parent_type is %s\n",
           __func__, __LINE__, paren_close->text(), paren_close->orig_line, paren_close->orig_col,
           get_token_name(opentype), get_token_name(parenttype));
   log_func_stack_inline(LFLPAREN);

   // the last chunk must be also modified. Issue #2149
   chunk_t *after_paren_close = chunk_get_next(paren_close);
   if (po != paren_close)
   {
      if (  flags != 0
         || (parent_all && parenttype != CT_NONE))
      {
         chunk_t *pc;
         for (pc = chunk_get_next(po, scope_e::PREPROC);
              pc != nullptr && pc != after_paren_close;
              pc = chunk_get_next(pc, scope_e::PREPROC))
         {
            chunk_flags_set(pc, flags);
            if (parent_all)
            {
               set_chunk_parent(pc, parenttype);
            }
         }
      }

      if (opentype != CT_NONE)
      {
         set_chunk_type(po, opentype);
         set_chunk_type(paren_close, (c_token_t)(opentype + 1));
      }

      if (parenttype != CT_NONE)
      {
         set_chunk_parent(po, parenttype);
         set_chunk_parent(paren_close, parenttype);
      }
   }
   return(chunk_get_next_ncnl(paren_close, scope_e::PREPROC));
} // flag_parens
