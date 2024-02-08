/**
 * @file after.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef NEWLINES_AFTER_H_INCLUDED
#define NEWLINES_AFTER_H_INCLUDED

class Chunk;

void newline_after_label_colon();
void newline_after_multiline_comment();

/**
 * Put a empty line after a return statement, unless it is followed by a
 * close brace.
 *
 * May not work with PAWN
 */
void newline_after_return(Chunk *start);

#endif /* NEWLINES_AFTER_H_INCLUDED */
