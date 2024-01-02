/**
 * @file newline_after_multiline_comment.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "newline_after_multiline_comment.h"

#include "chunk.h"
#include "newline_add_before.h"


void newline_after_multiline_comment()
{
   LOG_FUNC_ENTRY();

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (pc->IsNot(CT_COMMENT_MULTI))
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
