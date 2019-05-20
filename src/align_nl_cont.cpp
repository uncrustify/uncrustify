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
#include "ChunkStack.h"
#include "uncrustify.h"


chunk_t *align_nl_cont(chunk_t *start)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LALNLC, "%s(%d): start on [%s] on line %zu\n",
           __func__, __LINE__, get_token_name(start->type), start->orig_line);

   // Find the max column
   ChunkStack cs;
   size_t     max_col = 0;
   chunk_t    *pc     = start;
   while (  pc != nullptr
         && pc->type != CT_NEWLINE
         && pc->type != CT_COMMENT_MULTI)
   {
      if (chunk_is_token(pc, CT_NL_CONT))
      {
         align_add(cs, pc, max_col);
      }
      pc = chunk_get_next(pc);
   }

   // NL_CONT is always the last thing on a line
   chunk_t *tmp;
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
   chunk_t *pc = chunk_get_head();
   while (pc != nullptr)
   {
      if (pc->type != CT_NL_CONT)
      {
         pc = chunk_get_next_type(pc, CT_NL_CONT, -1);
         continue;
      }
      pc = align_nl_cont(pc);
   }
} // align_backslash_newline
