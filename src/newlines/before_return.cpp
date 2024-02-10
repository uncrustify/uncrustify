/**
 * @file before_return.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/before_return.h"

#include "chunk.h"
#include "mark_change.h"


void newline_before_return(Chunk *start)
{
   LOG_FUNC_ENTRY();

   Chunk *pc = start->GetPrev();
   Chunk *nl = pc;

   // Skip over single preceding newline
   if (pc->IsNewline())
   {
      // Do we already have a blank line?
      if (nl->GetNlCount() > 1)
      {
         return;
      }
      pc = nl->GetPrev();
   }

   // Skip over preceding comments that are not a trailing comment, taking
   // into account that comment blocks may span multiple lines.
   // Trailing comments are considered part of the previous token, not the
   // return statement.  They are handled below.
   while (  pc->IsComment()
         && pc->GetParentType() != CT_COMMENT_END)
   {
      pc = pc->GetPrev();

      if (!pc->IsNewline())
      {
         return;
      }
      nl = pc;
      pc = pc->GetPrev();
   }
   pc = nl->GetPrev();

   // Peek over trailing comment of previous token
   if (  pc->IsComment()
      && pc->GetParentType() == CT_COMMENT_END)
   {
      pc = pc->GetPrev();
   }

   // Don't add extra blanks after an opening brace or a case statement
   if (  pc->IsNullChunk()
      || (  pc->Is(CT_BRACE_OPEN)
         || pc->Is(CT_VBRACE_OPEN)
         || pc->Is(CT_CASE_COLON)))
   {
      return;
   }

   if (  nl->IsNewline()
      && nl->GetNlCount() < 2)
   {
      nl->SetNlCount(nl->GetNlCount() + 1);
      MARK_CHANGE();
      LOG_FMT(LBLANK, "%s(%d): orig line is %zu, orig col is %zu, text is '%s', new line count is now %zu\n",
              __func__, __LINE__, nl->GetOrigLine(), nl->GetOrigCol(), nl->Text(), nl->GetNlCount());
   }
} // newline_before_return
