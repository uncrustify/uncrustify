/**
 * @file flag_decltype.cpp
 *
 * @license GPL v2+
 */

#include "chunk_list.h"
#include "flag_decltype.h"


bool flag_cpp_decltype(chunk_t *pc)
{
   LOG_FUNC_ENTRY();

   if (chunk_is_token(pc, CT_DECLTYPE))
   {
      auto paren_open = chunk_get_next_ncnnl(pc);

      if (chunk_is_token(paren_open, CT_PAREN_OPEN))
      {
         auto close_paren = chunk_skip_to_match(paren_open);

         if (close_paren != nullptr)
         {
            pc = paren_open;

            do
            {
               chunk_flags_set(pc, PCF_IN_DECLTYPE);
               pc = pc->next;
            } while (pc != close_paren);

            return(true);
         }
      }
   }
   return(false);
} // mark_cpp_decltype
