/**
 * @file iarf.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef NEWLINES_IARF_H_INCLUDED
#define NEWLINES_IARF_H_INCLUDED

#include "option.h"

class Chunk;

/**
 * Does a simple Ignore, Add, Remove, or Force after the given chunk
 *
 * @param pc  The chunk
 * @param av  The IARF value
 */
void newline_iarf(Chunk *pc, uncrustify::iarf_e av);

/**
 * Does the Ignore, Add, Remove, or Force thing between two chunks
 *
 * @param before  The first chunk
 * @param after   The second chunk
 * @param av      The IARF value
 */
void newline_iarf_pair(Chunk *before, Chunk *after, uncrustify::iarf_e av, bool check_nl_assign_leave_one_liners = false);

#endif /* NEWLINES_IARF_H_INCLUDED */
