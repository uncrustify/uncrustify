/**
 * @file double_newline.h
 * prototypes for double_newline.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.h
 * @license GPL v2+
 */

#ifndef DOUBLE_NEWLINE_H_INCLUDED
#define DOUBLE_NEWLINE_H_INCLUDED

#include "chunk.h"

/**
 * Double the newline, if allowed.
 */
void double_newline(Chunk *nl);

#endif /* DOUBLE_NEWLINE_H_INCLUDED */
