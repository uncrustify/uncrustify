/**
 * @file double_space_struct_enum_union.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/double_space_struct_enum_union.h"

#include "chunk.h"
#include "logger.h"
#include "newlines/double_newline.h"


//! Make sure there is a blank line after a commented group of values
void newlines_double_space_struct_enum_union(Chunk *open_brace)
{
   LOG_FUNC_ENTRY();

   Chunk *pc = open_brace;

   while (  (pc = pc->GetNextNc())->IsNotNullChunk()
         && pc->GetLevel() > open_brace->GetLevel())
   {
      if (  pc->GetLevel() != (open_brace->GetLevel() + 1)
         || pc->IsNot(CT_NEWLINE))
      {
         continue;
      }
      /*
       * If the newline is NOT after a comment or a brace open and
       * it is before a comment, then make sure that the newline is
       * at least doubled
       */
      Chunk *prev = pc->GetPrev();

      if (  !prev->IsComment()
         && prev->IsNot(CT_BRACE_OPEN)
         && pc->GetNext()->IsComment())
      {
         if (pc->GetNlCount() < 2)
         {
            double_newline(pc);
         }
      }
   }
} // newlines_double_space_struct_enum_union
