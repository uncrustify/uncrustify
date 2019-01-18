/**
 * @file align_tools.h
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef ALIGN_TOOLS_H_INCLUDED
#define ALIGN_TOOLS_H_INCLUDED

#include "chunk_list.h"
#include "uncrustify_types.h"


/**
 * @brief return the chunk the follows after a C array
 *
 * The provided chunk is considered an array if it is an opening square
 * (CT_SQUARE_OPEN) and the matching close is followed by an equal sign '='
 *
 * Example:                  array[25] = 12;
 *                               /|\     /|\
 *                                |       |
 * provided chunk has to point to [       |
 * returned chunk points to              12
 *
 * @param chunk  chunk to operate on
 *
 * @return the chunk after the '=' if the check succeeds
 * @return nullptr in all other cases
 */
chunk_t *skip_c99_array(chunk_t *sq_open);

/**
 * Scans a line for stuff to align on.
 *
 * We trigger on BRACE_OPEN, FPAREN_OPEN, ASSIGN, and COMMA.
 * We want to align the NEXT item.
 */
chunk_t *scan_ib_line(chunk_t *start, bool first_pass);

void ib_shift_out(size_t idx, size_t num);

chunk_t *step_back_over_member(chunk_t *pc);

#endif /* ALIGN_TOOLS_H_INCLUDED */
