/**
 * @file cs_top_is_question.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 * extract from combine.cpp
 */

#include "cs_top_is_question.h"

#include "chunk.h"


bool cs_top_is_question(ChunkStack &cs, size_t level)
{
   Chunk *pc = cs.Empty() ? nullptr : cs.Top()->m_pc;

   return(  chunk_is_token(pc, CT_QUESTION)
         && pc->level == level);
}
