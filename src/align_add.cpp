/**
 * @file align_add.cpp
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align_add.h"
#include "uncrustify.h"


void align_add(ChunkStack &cs, Chunk *pc, size_t &max_col)
{
   LOG_FUNC_ENTRY();

   size_t min_col;
   Chunk  *prev = Chunk::NullChunkPtr;

   if (pc != nullptr)
   {
      prev = pc->GetPrev();
   }

   if (  prev->IsNullChunk()
      || chunk_is_newline(prev))
   {
      min_col = 1;
      LOG_FMT(LALADD, "%s(%d): pc->orig_line=%zu, pc->col=%zu max_col=%zu min_col=%zu\n",
              __func__, __LINE__, pc->orig_line, pc->column, max_col, min_col);
   }
   else
   {
      if (chunk_is_token(prev, CT_COMMENT_MULTI))
      {
         min_col = prev->orig_col_end + 1;
      }
      else
      {
         min_col = prev->column + prev->Len() + 1;
      }
      LOG_FMT(LALADD, "%s(%d): pc->orig_line=%zu, pc->col=%zu max_col=%zu min_col=%zu multi:%s prev->col=%zu prev->Len()=%zu %s\n",
              __func__, __LINE__, pc->orig_line, pc->column, max_col, min_col, (chunk_is_token(prev, CT_COMMENT_MULTI)) ? "Y" : "N",
              (chunk_is_token(prev, CT_COMMENT_MULTI)) ? prev->orig_col_end : (UINT32)prev->column, prev->Len(), get_token_name(prev->type));
   }

   if (cs.Empty())
   {
      max_col = 0;
   }
   cs.Push_Back(pc);

   if (min_col > max_col)
   {
      max_col = min_col;
   }
} // align_add
