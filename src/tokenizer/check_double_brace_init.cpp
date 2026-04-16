/**
 * @file check_double_brace_init.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "tokenizer/check_double_brace_init.h"

#include "chunk.h"

using namespace std;
using namespace uncrustify;


/**
 * Combines two tokens into {{ and }} if inside parens and nothing is between
 * either pair.
 */
void check_double_brace_init(Chunk *bo1)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LJDBI, "%s(%d): orig line is %zu, orig col is %zu", __func__, __LINE__, bo1->GetOrigLine(), bo1->GetOrigCol());
   Chunk const *pc = bo1->GetPrevNcNnlNi();   // Issue #2279

   if (pc->IsNullChunk())
   {
      return;
   }

   if (pc->IsParenClose())
   {
      Chunk *bo2 = bo1->GetNext();

      if (bo2->IsNullChunk())
      {
         return;
      }

      if (bo2->Is(E_Token::CT_BRACE_OPEN))
      {
         // found a potential double brace
         Chunk *bc2 = bo2->GetClosingParen();

         if (bc2->IsNullChunk())
         {
            return;
         }
         Chunk *bc1 = bc2->GetNext();

         if (bc1->IsNullChunk())
         {
            return;
         }

         if (bc1->Is(E_Token::CT_BRACE_CLOSE))
         {
            LOG_FMT(LJDBI, " - end, orig line is %zu, orig col is %zu\n", bc2->GetOrigLine(), bc2->GetOrigCol());
            // delete bo2 and bc1
            bo1->Text() += bo2->GetText();
            bo1->SetOrigColEnd(bo2->GetOrigColEnd());
            Chunk::Delete(bo2);
            bo1->SetParentType(E_Token::CT_DOUBLE_BRACE);

            bc2->Text() += bc1->GetText();
            bc2->SetOrigColEnd(bc1->GetOrigColEnd());
            Chunk::Delete(bc1);
            bc2->SetParentType(E_Token::CT_DOUBLE_BRACE);
            return;
         }
      }
   }
   LOG_FMT(LJDBI, " - no\n");
} // check_double_brace_init
