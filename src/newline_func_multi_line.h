/**
 * @file newline_func_multi_line.h
 * prototype for newline_func_multi_line.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef NEWLINE_FUNC_MULTI_LINE_H_INCLUDED
#define NEWLINE_FUNC_MULTI_LINE_H_INCLUDED

#include "chunk.h"

/**
 * Adds newlines to multi-line function call/decl/def
 * Start points to the open paren
 */
void newline_func_multi_line(Chunk *start);


#endif /* NEWLINE_FUNC_MULTI_LINE_H_INCLUDED */
