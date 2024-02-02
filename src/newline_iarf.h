/**
 * @file newline_iarf.h
 * prototypes for newline_iarf.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.h
 * @license GPL v2+
 */
#ifndef NEWLINE_IARF_H_INCLUDED
#define NEWLINE_IARF_H_INCLUDED

#include "chunk.h"


using namespace uncrustify;


/**
 * Does a simple Ignore, Add, Remove, or Force after the given chunk
 *
 * @param pc  The chunk
 * @param av  The IARF value
 */
void newline_iarf(Chunk *pc, iarf_e av);

/**
 * Does the Ignore, Add, Remove, or Force thing between two chunks
 *
 * @param before  The first chunk
 * @param after   The second chunk
 * @param av      The IARF value
 */
void newline_iarf_pair(Chunk *before, Chunk *after, iarf_e av, bool check_nl_assign_leave_one_liners = false);

#endif /* NEWLINE_IARF_H_INCLUDED */
