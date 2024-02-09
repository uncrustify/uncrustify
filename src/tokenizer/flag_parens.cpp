/**
 * @file flag_parens.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "tokenizer/flag_parens.h"

#include "chunk.h"
#include "uncrustify.h"


Chunk *flag_parens(Chunk *po, PcfFlags flags, E_Token opentype, E_Token parent_type, bool parent_all)
{
   LOG_FUNC_ENTRY();
   Chunk *paren_close;

   paren_close = po->GetClosingParen(E_Scope::PREPROC);

   if (paren_close->IsNullChunk())
   {
      LOG_FMT(LERR, "%s(%d): no match for '%s' at [%zu:%zu]",
              __func__, __LINE__, po->Text(), po->GetOrigLine(), po->GetOrigCol());
      log_func_stack_inline(LERR);
      exit(EX_SOFTWARE);
   }
   LOG_FMT(LFLPAREN, "%s(%d): between  po is '%s', orig line is %zu, orig col is %zu, and\n",
           __func__, __LINE__, po->Text(), po->GetOrigLine(), po->GetOrigCol());
   LOG_FMT(LFLPAREN, "%s(%d): paren_close is '%s', orig line is %zu, orig col is %zu, type is %s, parent type is %s\n",
           __func__, __LINE__, paren_close->Text(), paren_close->GetOrigLine(), paren_close->GetOrigCol(),
           get_token_name(opentype), get_token_name(parent_type));
   log_func_stack_inline(LFLPAREN);

   // the last chunk must be also modified. Issue #2149
   Chunk *after_paren_close = paren_close->GetNext();

   if (po != paren_close)
   {
      if (  flags != PCF_NONE
         || (  parent_all
            && parent_type != CT_NONE))
      {
         Chunk *pc;

         //for (pc = po;
         for (pc = po->GetNext(E_Scope::PREPROC);
              pc->IsNotNullChunk() && pc != after_paren_close;
              pc = pc->GetNext(E_Scope::PREPROC))
         {
            pc->SetFlagBits(flags);

            if (parent_all)
            {
               pc->SetParentType(parent_type);
            }
         }
      }

      if (opentype != CT_NONE)
      {
         po->SetType(opentype);
         paren_close->SetType((E_Token)(opentype + 1));
      }

      if (parent_type != CT_NONE)
      {
         po->SetParentType(parent_type);
         paren_close->SetParentType(parent_type);
      }
   }
   return(paren_close->GetNextNcNnl(E_Scope::PREPROC));
} // flag_parens
