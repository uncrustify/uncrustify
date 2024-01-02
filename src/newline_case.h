/**
 * @file newline_case.h
 * prototype for newline_case.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef NEWLINE_CASE_H_INCLUDED
#define NEWLINE_CASE_H_INCLUDED


#include "chunk.h"


/**
 * Put a empty line between the 'case' statement and the previous case colon
 * or semicolon.
 * Does not work with PAWN (?)
 */
void newline_case(Chunk *start);


#endif /* NEWLINE_CASE_H_INCLUDED */
