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


#endif /* REINDENT_LINE_H_INCLUDED */
