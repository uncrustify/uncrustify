/**
 * @file remove_next_newlines.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/remove_next_newlines.h"

#include "chunk.h"
#include "logger.h"
#include "mark_change.h"


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
