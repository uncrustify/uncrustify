/**
 * @file struct_initializers.cpp
 *
 * @author  Guy Maurel
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align/struct_initializers.h"

#include "align/init_brace.h"
#include "chunk.h"


void align_struct_initializers()
{
   LOG_FUNC_ENTRY();
   Chunk *pc = Chunk::GetHead();

   while (pc->IsNotNullChunk())
   {
      Chunk *prev = pc->GetPrevNcNnl();

      if (  prev->Is(CT_ASSIGN)
         && (  pc->Is(CT_BRACE_OPEN)
            || (  language_is_set(lang_flag_e::LANG_D)
               && pc->Is(CT_SQUARE_OPEN))))
      {
         align_init_brace(pc);
      }
      pc = pc->GetNextType(CT_BRACE_OPEN);
   }
} // align_struct_initializers
