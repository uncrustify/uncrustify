/**
 * @file newline_iarf_pair.h
 * prototypes for newline_iarf_pair.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#ifndef NEWLINES_IARF_PAIR_H_INCLUDED
#define NEWLINES_IARF_PAIR_H_INCLUDED

#include "chunk.h"

using namespace uncrustify;

/**
 * Does the Ignore, Add, Remove, or Force thing between two chunks
 *
 * @param before  The first chunk
 * @param after   The second chunk
 * @param av      The IARF value
 */
void newline_iarf_pair(Chunk *before, Chunk *after, iarf_e av, bool check_nl_assign_leave_one_liners = false);

#endif /* NEWLINES_IARF_PAIR_H_INCLUDED */
