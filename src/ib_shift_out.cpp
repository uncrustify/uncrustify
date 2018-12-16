/**
 * @file ib_shift_out.cpp
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "ib_shift_out.h"
#include "uncrustify_types.h"


void ib_shift_out(size_t idx, size_t num)
{
   while (idx < cpd.al_cnt)
   {
      cpd.al[idx].col += num;
      idx++;
   }
} // ib_shift_out
