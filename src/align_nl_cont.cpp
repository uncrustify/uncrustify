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
         && pc->IsNot(CT_NEWLINE)
         && pc->IsNot(CT_COMMENT_MULTI))
   {
      if (pc->Is(CT_NL_CONT))
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


void align_backslash_newline()
{
   LOG_FUNC_ENTRY();
   Chunk *pc = Chunk::GetHead();

   while (pc->IsNotNullChunk())
   {
      if (pc->IsNot(CT_NL_CONT))
      {
         pc = pc->GetNextType(CT_NL_CONT);
         continue;
      }
      pc = align_nl_cont(pc);
   }
} // align_backslash_newline
