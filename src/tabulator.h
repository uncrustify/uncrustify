/**
 * @file tabulator.h
 * Header for tabulator.cpp
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef C_TABULATOR_H_INCLUDED
#define C_TABULATOR_H_INCLUDED

#include "uncrustify_types.h"


/**
 * Advances to the next tab stop.
 * Column 1 is the left-most column.
 *
 * @param col     The current column
 * @param tabsize The tabsize
 * @return the next tabstop column
 */
size_t calc_next_tab_column(size_t col, size_t tabsize);


/**
 * Advances to the next tab stop for output.
 *
 * @param col  The current column
 * @return the next tabstop column
 */
size_t next_tab_column(size_t col);


/**
 * Advances to the next tab stop if not currently on one.
 *
 * @param col  The current column
 * @return the next tabstop column
 */
size_t align_tab_column(size_t col);


#endif /* C_TABULATOR_H_INCLUDED */
