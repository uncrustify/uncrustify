/**
 * @file get_closing_brace.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/get_closing_brace.h"

#include "chunk.h"


Chunk *get_closing_brace(Chunk *start)
{
   LOG_FUNC_ENTRY();

   Chunk  *pc;
   size_t level = start->GetLevel();

   for (pc = start; (pc = pc->GetNext())->IsNotNullChunk();)
   {
      if (  (pc->IsBraceClose())
         && pc->GetLevel() == level)
      {
         return(pc);
      }

      // for some reason, we can have newlines between if and opening brace that are lower level than either
      if (  !pc->IsNewline()
         && pc->GetLevel() < level)
      {
         return(Chunk::NullChunkPtr);
      }
   }

   return(Chunk::NullChunkPtr);
} // get_closing_brace
