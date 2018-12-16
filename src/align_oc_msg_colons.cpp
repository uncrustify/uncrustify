/**
 * @file align_oc_msg_colons.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "align_oc_msg_colons.h"
#include "align_oc_msg_colon.h"
#include "chunk_list.h"
#include "align_stack.h"

using namespace uncrustify;


void align_oc_msg_colons(void)
{
   LOG_FUNC_ENTRY();

   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next(pc))
   {
      if (chunk_is_token(pc, CT_SQUARE_OPEN) && pc->parent_type == CT_OC_MSG)
      {
         align_oc_msg_colon(pc);
      }
   }
} // align_oc_msg_colons
