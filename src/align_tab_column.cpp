/**
 * @file align_tab_column.cpp
 *
 * @author  Guy Maurel
 * split from prototypes.h
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align_tab_column.h"

#include "chunk_list.h"
#include "prototypes.h"
#include "uncrustify_types.h"

using namespace uncrustify;


/**
 * Advances to the next tab stop if not currently on one.
 *
 * @param col  The current column
 * @return the next tabstop column
 */
size_t align_tab_column(size_t col)
{
   //if (col <= 0)
   if (col == 0)
   {
      col = 1;
   }
   if ((col % uncrustify::options::output_tab_size()) != 1)
   {
      col = next_tab_column(col);
   }
   return(col);
}
