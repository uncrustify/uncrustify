/**
 * @file align_struct_initializers.cpp
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align_struct_initializers.h"

#include "align_init_brace.h"
#include "chunk_list.h"


void align_struct_initializers(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc = chunk_get_head();
   while (pc != nullptr)
   {
      chunk_t *prev = chunk_get_prev_ncnl(pc);
      if (  chunk_is_token(prev, CT_ASSIGN)
         && (  chunk_is_token(pc, CT_BRACE_OPEN)
            || (language_is_set(LANG_D) && chunk_is_token(pc, CT_SQUARE_OPEN))))
      {
         align_init_brace(pc);
      }
      pc = chunk_get_next_type(pc, CT_BRACE_OPEN, -1);
   }
} // align_struct_initializers
