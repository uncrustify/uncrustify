/**
 * @file add.h
 *
 * @author  Guy Maurel
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef ALIGN_ADD_H_INCLUDED
#define ALIGN_ADD_H_INCLUDED

#include <cstddef>      // to get size_t

class Chunk;
class ChunkStack;

void align_add(ChunkStack &cs, Chunk *pc, size_t &max_col);

#endif /* ALIGN_ADD_H_INCLUDED */
