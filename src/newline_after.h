/**
 * @file newline_after.h
 * prototype for newline_colon.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef NEWLINE_AFTER_H_INCLUDED
#define NEWLINE_AFTER_H_INCLUDED

#include "chunk.h"

void newline_after_label_colon();

void newline_after_multiline_comment();

/**
 * Put a empty line after a return statement, unless it is followed by a
 * close brace.
 *
 * May not work with PAWN
 */
void newline_after_return(Chunk *start);

#endif /* NEWLINE_AFTER_H_INCLUDED */
