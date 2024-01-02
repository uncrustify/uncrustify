/**
 * @file newlines_enum_entries.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "newlines_enum_entries.h"

#include "newline_iarf.h"


//! If requested, make sure each entry in an enum is on its own line
void newlines_enum_entries(Chunk *open_brace, iarf_e av)
{
   LOG_FUNC_ENTRY();

   for (Chunk *pc = open_brace->GetNextNc();
        pc->IsNotNullChunk() && pc->GetLevel() > open_brace->GetLevel();
        pc = pc->GetNextNc())
   {
      if (  (pc->GetLevel() != (open_brace->GetLevel() + 1))
         || pc->IsNot(CT_COMMA)
         || (  pc->Is(CT_COMMA)
            && (  pc->GetNext()->GetType() == CT_COMMENT_CPP
               || pc->GetNext()->GetType() == CT_COMMENT
               || pc->GetNext()->GetType() == CT_COMMENT_MULTI)))
      {
         continue;
      }
      newline_iarf(pc, av);
   }

   newline_iarf(open_brace, av);
} // newlines_enum_entries
