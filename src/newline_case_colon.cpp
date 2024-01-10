/**
 * @file newline_case_colon.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "newline_case_colon.h"

#include "newline_add.h"


void newline_case_colon(Chunk *start)
{
   LOG_FUNC_ENTRY();

   // Scan forwards until a non-comment is found
   Chunk *pc = start;

   do
   {
      pc = pc->GetNext();
   } while (pc->IsComment());

   if (  pc->IsNotNullChunk()
      && !pc->IsNewline())
   {
      newline_add_before(pc);
   }
} // newline_case_colon
