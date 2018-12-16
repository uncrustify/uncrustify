/**
 * @file prototypes.h
 * Big jumble of prototypes used in Uncrustify.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef C_PARSE_PROTOTYPES_H_INCLUDED
#define C_PARSE_PROTOTYPES_H_INCLUDED

#include "uncrustify_types.h"
#include "chunk_list.h"

#include <string>
#include <deque>


/**
 * Advances to the next tab stop.
 * Column 1 is the left-most column.
 *
 * @param col     The current column
 * @param tabsize The tabsize
 * @return the next tabstop column
 */
static inline size_t calc_next_tab_column(size_t col, size_t tabsize)
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


/**
 * Advances to the next tab stop for output.
 *
 * @param col  The current column
 * @return the next tabstop column
 */
static inline size_t next_tab_column(size_t col)
{
   return(calc_next_tab_column(col, uncrustify::options::output_tab_size()));
}


#endif /* C_PARSE_PROTOTYPES_H_INCLUDED */
