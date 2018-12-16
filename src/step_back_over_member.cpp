/**
 * @file step_back_over_member.cpp
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "step_back_over_member.h"


chunk_t *step_back_over_member(chunk_t *pc)
{
   chunk_t *tmp;

   // Skip over any class stuff: bool CFoo::bar()
   while (  ((tmp = chunk_get_prev_ncnl(pc)) != nullptr)
         && chunk_is_token(tmp, CT_DC_MEMBER))
   {
      // TODO: verify that we are pointing at something sane?
      pc = chunk_get_prev_ncnl(tmp);
   }
   return(pc);
} // step_back_over_member
