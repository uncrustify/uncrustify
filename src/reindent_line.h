/**
 * @file reindent_line.h
 * prototypes for reindent_line.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef REINDENT_LINE_H_INCLUDED
#define REINDENT_LINE_H_INCLUDED

#include "chunk.h"


/**
 * Changes the initial indent for a line to the given column
 *
 * @param pc      The chunk at the start of the line
 * @param column  The desired column
 */
void reindent_line(Chunk *pc, size_t column);


/**
 * Shift all the tokens in this line to the right
 *
 * @param first The chunk from which to start shifting
 */
void shift_the_rest_of_the_line(Chunk *first);


#endif /* REINDENT_LINE_H_INCLUDED */
