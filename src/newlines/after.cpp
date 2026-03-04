/**
 * @file after.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/after.h"

#include "chunk.h"
#include "logger.h"
#include "newlines/add.h"
#include "newlines/double_newline.h"


void newline_after_label_colon()
{
   LOG_FUNC_ENTRY();

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (pc->IsNot(E_Token::LABEL_COLON))
      {
         continue;
      }
      newline_add_after(pc);
   }
} // newline_after_label_colon


void newline_after_multiline_comment()
{
   LOG_FUNC_ENTRY();

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (pc->IsNot(E_Token::COMMENT_MULTI))
      {
         continue;
      }
      Chunk *tmp = pc;

      while (  ((tmp = tmp->GetNext())->IsNotNullChunk())
            && !tmp->IsNewline())
      {
         if (!tmp->IsComment())
         {
            newline_add_before(tmp);
            break;
         }
      }
   }
} // newline_after_multiline_comment


void newline_after_return(const Chunk *start)
{
   LOG_FUNC_ENTRY();

   const Chunk *semi  = start->GetNextType(E_Token::SEMICOLON, start->GetLevel());
   const Chunk *after = semi->GetNextNcNnlNet();

   // If we hit a brace or an 'else', then a newline isn't needed
   if (  after->IsNullChunk()
      || after->IsBraceClose()
      || after->Is(E_Token::ELSE))
   {
      return;
   }
   Chunk *pc;

   for (pc = semi->GetNext(); pc != after; pc = pc->GetNext())
   {
      if (pc->Is(E_Token::NEWLINE))
      {
         if (pc->GetNlCount() < 2)
         {
            double_newline(pc);
         }
         return;
      }
   }
} // newline_after_return
