/**
 * @file newline_force.h
 * prototype for newline_force.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef NEWLINE_FORCE_H_INCLUDED
#define NEWLINE_FORCE_H_INCLUDED

#include "chunk.h"

Chunk *newline_force_after(Chunk *pc);

Chunk *newline_force_before(Chunk *pc);

#endif /* NEWLINE_FORCE_H_INCLUDED */
