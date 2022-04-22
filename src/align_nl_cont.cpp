/**
 * @file align_nl_cont.cpp
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align_nl_cont.h"

#include "align_add.h"
#include "uncrustify.h"


Chunk *align_nl_cont(Chunk *start)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LALNLC, "%s(%d): start on [%s] on line %zu\n",
           __func__, __LINE__, get_token_name(start->type), start->orig_line);

   // Find the max column
   ChunkStack cs;
   size_t     max_col = 0;
   Chunk      *pc     = start;

   while (  pc->IsNotNullChunk()
         && chunk_is_not_token(pc, CT_NEWLINE)
         && chunk_is_not_token(pc, CT_COMMENT_MULTI))
   {
      if (chunk_is_token(pc, CT_NL_CONT))
      {
         align_add(cs, pc, max_col);
      }
      pc = pc->GetNext();
   }
   // NL_CONT is always the last thing on a line
   Chunk *tmp;

   while ((tmp = cs.Pop_Back()) != nullptr)
   {
      chunk_flags_set(tmp, PCF_WAS_ALIGNED);
      tmp->column = max_col;
   }
   return(pc);
} // align_nl_cont


void align_backslash_newline(void)
{
   LOG_FUNC_ENTRY();
   Chunk *pc = Chunk::GetHead();

   while (pc->IsNotNullChunk())
   {
      if (chunk_is_not_token(pc, CT_NL_CONT))
      {
         pc = pc->GetNextType(CT_NL_CONT, -1);
         continue;
      }
      pc = align_nl_cont(pc);
   }
} // align_backslash_newline
