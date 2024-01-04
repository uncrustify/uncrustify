/**
 * @file newline_force_after.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "newline_force_after.h"

#include "chunk.h"
#include "mark_change.h"
#include "newline_add_after.h"

#define MARK_CHANGE()    mark_change(__func__, __LINE__)


Chunk *newline_force_after(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   Chunk *nl = newline_add_after(pc);   // add a newline

   if (  nl->IsNotNullChunk()
      && nl->GetNlCount() > 1) // check if there are more than 1 newline
   {
      nl->SetNlCount(1);                   // if so change the newline count back to 1
      MARK_CHANGE();
   }
   return(nl);
} // newline_force_after
