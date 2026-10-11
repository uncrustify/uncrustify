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
      if (pc->IsNot(E_Token::CT_LABEL_COLON))
      {
         continue;
      }
      newline_add_after(pc);
   }
} // newline_after_label_colon

void newline_after_singleline_comment()
{
   LOG_FUNC_ENTRY();

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
       if (!pc->Is(E_Token::CT_COMMENT))
       {
          continue;
       }

       // Only handle comments that are alone on their own line, e.g.
       //   /* tab knowledge base */
       // NOT trailing comments like:
       //   int nl;    /* next line */
       Chunk *prev = pc->GetPrev();
       if (prev->IsNotNullChunk() && !prev->IsNewline())
       {
          continue;              // trailing comment on a code line -> skip
       }

       Chunk *nl = pc->GetNext(); // the newline ending the comment's own line
       if (nl == nullptr || !nl->IsNewline())
       {
          continue;
       }

       Chunk *after_nl = nl->GetNext();
       if (after_nl == nullptr)
       {
          continue;
       }

       if (after_nl->Is(E_Token::CT_BRACE_CLOSE))
       {
          continue;   // don't force a blank line right before a closing brace
       }

       if (nl->GetNlCount() >= 2)
       {
          continue;   // already a blank line here
       }

       double_newline(nl);
   }
} // newline_after_singleline_comment

void newline_after_multiline_comment()
{
   LOG_FUNC_ENTRY();

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (pc->IsNot(E_Token::CT_COMMENT_MULTI))
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

   const Chunk *semi  = start->GetNextType(E_Token::CT_SEMICOLON, start->GetLevel());
   const Chunk *after = semi->GetNextNcNnlNet();

   // If we hit a brace or an 'else', then a newline isn't needed
   if (  after->IsNullChunk()
      || after->IsBraceClose()
      || after->Is(E_Token::CT_ELSE))
   {
      return;
   }
   Chunk *pc;

   for (pc = semi->GetNext(); pc != after; pc = pc->GetNext())
   {
      if (pc->Is(E_Token::CT_NEWLINE))
      {
         if (pc->GetNlCount() < 2)
         {
            double_newline(pc);
         }
         return;
      }
   }
} // newline_after_return
