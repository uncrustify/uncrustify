/**
 * @file tools.h
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef ALIGN_TOOLS_H_INCLUDED
#define ALIGN_TOOLS_H_INCLUDED

#include <cstddef>

class Chunk;


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
 * @return Chunk::NullChunkPtr in all other cases
 */
Chunk *skip_c99_array(Chunk *sq_open);

/**
 * Scans a line for stuff to align on.
 *
 * We trigger on BRACE_OPEN, FPAREN_OPEN, ASSIGN, and COMMA.
 * We want to align the NEXT item.
 */
Chunk *scan_ib_line(Chunk *start);

void ib_shift_out(size_t idx, size_t num);

Chunk *step_back_over_member(Chunk *pc);

#endif /* ALIGN_TOOLS_H_INCLUDED */
