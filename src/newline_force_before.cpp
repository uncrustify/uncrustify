/**
 * @file newline_force_before.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "newline_force_before.h"

#include "chunk.h"
#include "mark_change.h"
#include "newline_add_before.h"

#define MARK_CHANGE()    mark_change(__func__, __LINE__)


Chunk *newline_force_before(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   Chunk *nl = newline_add_before(pc);

   if (  nl->IsNotNullChunk()
      && nl->GetNlCount() > 1)
   {
      nl->SetNlCount(1);
      MARK_CHANGE();
   }
   return(nl);
} // newline_force_before
