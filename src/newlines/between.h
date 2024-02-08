/**
 * @file between.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#ifndef NEWLINES_BETWEEN_H_INCLUDED
#define NEWLINES_BETWEEN_H_INCLUDED

#include "chunk.h"

/**
 * Counts newlines between two chunk elements
 *
 * @param  pc_start  chunk from which the counting of newlines will start
 * @param  pc_end    chunk at which the counting of newlines will end
 * @param  newlines  reference in which the amount of newlines will be written to
 *                   (will be initialized with 0)
 * @param  scope     specifies region chunks should/should not be considered.
 *
 * @return false  if pc_start or pc_end are null chunks or if pc_end is not reached
 * @return true   if above cases are not met
 */
bool newlines_between(Chunk *pc_start, Chunk *pc_end, size_t &newlines, E_Scope scope = E_Scope::ALL);


#endif /* NEWLINES_BETWEEN_H_INCLUDED */
