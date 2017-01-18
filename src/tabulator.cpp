/**
 * @file tabulator.cpp
 * Handling of tabstops
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "tabulator.h"
#include "uncrustify_types.h"
#include "chunk_list.h"


size_t calc_next_tab_column(size_t col, size_t tabsize)
{
   if (col == 0)
   {
      col = 1;
   }
   if (cpd.frag_cols > 0)
   {
      col += cpd.frag_cols - 1;
   }
   col = 1 + ((((col - 1) / tabsize) + 1) * tabsize);
   if (cpd.frag_cols > 0)
   {
      col -= cpd.frag_cols - 1;
   }
   return(col);
}


size_t next_tab_column(size_t col)
{
   return(calc_next_tab_column(col, cpd.settings[UO_output_tab_size].u));
}


size_t align_tab_column(size_t col)
{
   if (col == 0)
   {
      col = 1;
   }
   if ((col % cpd.settings[UO_output_tab_size].u) != 1)
   {
      col = next_tab_column(col);
   }
   return(col);
}
