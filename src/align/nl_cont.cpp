/**
 * @file nl_cont.cpp
 *
 * @author  Guy Maurel
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align/nl_cont.h"

#include "align/add.h"
#include "chunk.h"
#include "ChunkStack.h"
#include "uncrustify.h"

#include <algorithm>                   // to get max
#include <limits>


using namespace std;
using namespace uncrustify;


Chunk *align_nl_cont(Chunk *start)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LALNLC, "%s(%d): start on [%s] on line %zu\n",
           __func__, __LINE__, get_token_name(start->GetType()), start->GetOrigLine());

   // Decide which column to align to
   ChunkStack cs;
   size_t     align_col = 0;
   size_t     min_col   = numeric_limits<size_t>::max();
   size_t     max_col   = 0;
   Chunk      *pc       = start;

   while (  pc->IsNotNullChunk()
         && pc->IsNot(CT_NEWLINE)
         && pc->IsNot(CT_COMMENT_MULTI))
   {
      if (pc->Is(CT_NL_CONT))
      {
         align_add(cs, pc, align_col);
         min_col = min(min_col, pc->GetColumn());
         max_col = max(max_col, pc->GetColumn());
      }
      pc = pc->GetNext();
   }
   align_col = align_col - 1 + options::align_nl_cont_spaces();

   if (options::align_nl_cont() == 2) // align with leftmost backslash
   {
      align_col = max(align_col, min_col);
   }
   else if (options::align_nl_cont() == 3) // align with rightmost backslash
   {
      align_col = max(align_col, max_col);
   }
   // NL_CONT is always the last thing on a line
   Chunk *tmp;

   while ((tmp = cs.Pop_Back())->IsNotNullChunk())
   {
      tmp->SetFlagBits(PCF_WAS_ALIGNED);
      tmp->SetColumn(align_col);
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
