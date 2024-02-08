/**
 * @file before_return.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef NEWLINES_BEFORE_RETURN_H_INCLUDED
#define NEWLINES_BEFORE_RETURN_H_INCLUDED

class Chunk;

//! Put a blank line before a return statement, unless it is after an open brace
void newline_before_return(Chunk *start);

#endif /* NEWLINES_BEFORE_RETURN_H_INCLUDED */
