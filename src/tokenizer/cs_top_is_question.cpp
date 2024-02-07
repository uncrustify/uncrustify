/**
 * @file cs_top_is_question.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 * extract from combine.cpp
 */

#include "tokenizer/cs_top_is_question.h"

#include "chunk.h"


bool cs_top_is_question(ChunkStack &cs, size_t level)
{
   Chunk *pc = cs.Empty() ? Chunk::NullChunkPtr : cs.Top()->m_pc;

   return(  pc->Is(CT_QUESTION)
         && pc->GetLevel() == level);
}
