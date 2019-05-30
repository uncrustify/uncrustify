/**
 * @file cs_top_is_question.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 * extract from combine.cpp
 */

#include "cs_top_is_question.h"

#include "chunk_list.h"


bool cs_top_is_question(ChunkStack &cs, size_t level)
{
   chunk_t *pc = cs.Empty() ? nullptr : cs.Top()->m_pc;

   return(  chunk_is_token(pc, CT_QUESTION)
         && pc->level == level);
}
