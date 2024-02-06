/**
 * @file remove_next_newlines.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "newlines/remove_next_newlines.h"

#include "mark_change.h"

#define MARK_CHANGE()    mark_change(__func__, __LINE__)


void remove_next_newlines(Chunk *start)
{
   LOG_FUNC_ENTRY();

   Chunk *next;

   while ((next = start->GetNext())->IsNotNullChunk())
   {
      if (  next->IsNewline()
         && next->SafeToDeleteNl())
      {
         Chunk::Delete(next);
         MARK_CHANGE();
      }
      else if (next->IsVBrace())
      {
         start = next;
      }
      else
      {
         break;
      }
   }
} // remove_next_newlines
