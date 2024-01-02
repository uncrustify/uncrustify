/**
 * @file newline_after_return.cpp
 * prototype for newline_after_return.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef NEWLINE_AFTER_RETURN_H_INCLUDED
#define NEWLINE_AFTER_RETURN_H_INCLUDED


#include "chunk.h"

/**
 * Put a empty line after a return statement, unless it is followed by a
 * close brace.
 *
 * May not work with PAWN
 */
void newline_after_return(Chunk *start);


#endif /* NEWLINE_AFTER_RETURN_H_INCLUDED */
