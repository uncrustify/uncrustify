/**
 * @file newline_min_after.h
 * prototype for newline_min_after.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef NEWLINE_MIN_AFTER_H_INCLUDED
#define NEWLINE_MIN_AFTER_H_INCLUDED

#include "chunk.h"

#include <cstddef>

void newline_min_after(Chunk *ref, size_t count, E_PcfFlag flag);

#endif /* NEWLINE_MIN_AFTER_H_INCLUDED */
