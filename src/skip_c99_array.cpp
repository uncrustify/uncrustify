/**
 * @file skip_c99_array.cpp
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "skip_c99_array.h"
#include "chunk_list.h"


chunk_t *skip_c99_array(chunk_t *sq_open)
{
   if (chunk_is_token(sq_open, CT_SQUARE_OPEN))
   {
      chunk_t *tmp = chunk_get_next_nc(chunk_skip_to_match(sq_open));

      if (chunk_is_token(tmp, CT_ASSIGN))
      {
         return(chunk_get_next_nc(tmp));
      }
   }
   return(nullptr);
} // skip_c99_array
