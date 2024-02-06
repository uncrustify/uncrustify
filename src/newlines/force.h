/**
 * @file force.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef NEWLINES_FORCE_H_INCLUDED
#define NEWLINES_FORCE_H_INCLUDED

#include "chunk.h"

Chunk *newline_force_after(Chunk *pc);

Chunk *newline_force_before(Chunk *pc);

#endif /* NEWLINES_FORCE_H_INCLUDED */
