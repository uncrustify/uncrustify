/**
 * @file case.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef NEWLINES_CASE_H_INCLUDED
#define NEWLINES_CASE_H_INCLUDED

class Chunk;

/**
 * Put a empty line between the 'case' statement and the previous case colon
 * or semicolon.
 * Does not work with PAWN (?)
 */
void newline_case(Chunk *start);
void newline_case_colon(Chunk *start);

#endif /* NEWLINES_CASE_H_INCLUDED */
