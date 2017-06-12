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


/*
 *  punctuators.cpp
 */

/**
 * Checks if the first max. 6 chars of a given string match a punctuator
 *
 * @param str         string that will be checked, can be shorter than 6 chars
 * @param lang_flags  specifies from which language punctuators will be
 *                    considered
 * @return				 chunk tag of the found punctuator
 *                    nullptr if nothing found
 */
const chunk_tag_t *find_punctuator(const char *str, int lang_flags);


/**
 * Advances to the next tab stop.
 * Column 1 is the left-most column.
 *
 * @param col     The current column
 * @param tabsize The tabsize
 * @return the next tabstop column
 */
static_inline
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


/**
 * Advances to the next tab stop for output.
 *
 * @param col  The current column
 * @return the next tabstop column
 */
static_inline
size_t next_tab_column(size_t col)
{
   return(calc_next_tab_column(col, cpd.settings[UO_output_tab_size].u));
}


/**
 * Advances to the next tab stop if not currently on one.
 *
 * @param col  The current column
 * @return the next tabstop column
 */
static_inline
size_t align_tab_column(size_t col)
{
   //if (col <= 0)
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


#endif /* C_PARSE_PROTOTYPES_H_INCLUDED */
