/**
 * @file flag_parens.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "flag_parens.h"

#include "uncrustify.h"


Chunk *flag_parens(Chunk *po, pcf_flags_t flags, E_Token opentype, E_Token parenttype, bool parent_all)
{
   LOG_FUNC_ENTRY();
   Chunk *paren_close;

   paren_close = chunk_skip_to_match(po, E_Scope::PREPROC);

   if (paren_close == nullptr)
   {
      LOG_FMT(LERR, "%s(%d): no match for '%s' at [%zu:%zu]",
              __func__, __LINE__, po->Text(), po->orig_line, po->orig_col);
      log_func_stack_inline(LERR);
      cpd.error_count++;
      return(nullptr);
   }
   LOG_FMT(LFLPAREN, "%s(%d): between  po is '%s', orig_line is %zu, orig_col is %zu, and\n",
           __func__, __LINE__, po->Text(), po->orig_line, po->orig_col);
   LOG_FMT(LFLPAREN, "%s(%d): paren_close is '%s', orig_line is %zu, orig_col is %zu, type is %s, parent_type is %s\n",
           __func__, __LINE__, paren_close->Text(), paren_close->orig_line, paren_close->orig_col,
           get_token_name(opentype), get_token_name(parenttype));
   log_func_stack_inline(LFLPAREN);

   // the last chunk must be also modified. Issue #2149
   Chunk *after_paren_close = paren_close->GetNext();

   if (po != paren_close)
   {
      if (  flags != PCF_NONE
         || (  parent_all
            && parenttype != CT_NONE))
      {
         Chunk *pc;

         for (pc = po->GetNext(E_Scope::PREPROC);
              pc != nullptr && pc->IsNotNullChunk() && pc != after_paren_close;
              pc = pc->GetNext(E_Scope::PREPROC))
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
         set_chunk_type(paren_close, (E_Token)(opentype + 1));
      }

      if (parenttype != CT_NONE)
      {
         set_chunk_parent(po, parenttype);
         set_chunk_parent(paren_close, parenttype);
      }
   }
   return(paren_close->GetNextNcNnl(E_Scope::PREPROC));
} // flag_parens
