/**
 * @file align_tab_column.h
 *
 * @author  Guy Maurel
 * split from prototypes.h
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef ALIGN_TAB_COLUMN_H_INCLUDED
#define ALIGN_TAB_COLUMN_H_INCLUDED

#include "uncrustify_types.h"

/**
 * Advances to the next tab stop if not currently on one.
 *
 * @param col  The current column
 * @return the next tabstop column
 */
size_t align_tab_column(size_t col);

#endif /* ALIGN_TAB_COLUMN_H_INCLUDED */
