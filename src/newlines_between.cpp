/**
 * @file newlines_between.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "newlines_between.h"

#include "uncrustify.h"


bool newlines_between(Chunk *pc_start, Chunk *pc_end, size_t &newlines, E_Scope scope)
{
   if (  pc_start->IsNullChunk()
      || pc_end->IsNullChunk())
   {
      return(false);
   }
   newlines = 0;

   Chunk *it = pc_start;

   for ( ; it->IsNotNullChunk() && it != pc_end; it = it->GetNext(scope))
   {
      newlines += it->GetNlCount();
   }

   // newline count is valid if search stopped on expected chunk
   return(it == pc_end);
} // newlines_between
