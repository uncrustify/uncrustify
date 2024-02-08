/**
 * @file add.cpp
 *
 * @author  Guy Maurel
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align/add.h"
#include "chunk.h"
#include "ChunkStack.h"
#include "uncrustify.h"


void align_add(ChunkStack &cs, Chunk *pc, size_t &max_col)
{
   LOG_FUNC_ENTRY();

   size_t min_col;
   Chunk  *prev = Chunk::NullChunkPtr;

   if (pc->IsNotNullChunk())
   {
      prev = pc->GetPrev();
   }

   if (  prev->IsNullChunk()
      || prev->IsNewline())
   {
      min_col = 1;
      LOG_FMT(LALADD, "%s(%d): pc orig line=%zu, pc->col=%zu max_col=%zu min_col=%zu\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetColumn(), max_col, min_col);
   }
   else
   {
      if (prev->Is(CT_COMMENT_MULTI))
      {
         min_col = prev->GetOrigColEnd() + 1;
      }
      else
      {
         min_col = prev->GetColumn() + prev->Len() + 1;
      }
      LOG_FMT(LALADD, "%s(%d): pc orig line=%zu, pc->col=%zu max_col=%zu min_col=%zu multi:%s prev->col=%zu prev->Len()=%zu %s\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetColumn(), max_col, min_col, (prev->Is(CT_COMMENT_MULTI)) ? "Y" : "N",
              (prev->Is(CT_COMMENT_MULTI)) ? prev->GetOrigColEnd() : (UINT32)prev->GetColumn(), prev->Len(), get_token_name(prev->GetType()));
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
