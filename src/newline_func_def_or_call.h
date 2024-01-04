/**
 * @file newline_func_def_or_call.h
 * prototype for newline_func_def_or_call.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef NEWLINE_FUNC_DEF_OR_CALL_H_INCLUDED
#define NEWLINE_FUNC_DEF_OR_CALL_H_INCLUDED


#include "chunk.h"


/**
 * Formats a function declaration
 * Start points to the open paren
 */
void newline_func_def_or_call(Chunk *start);


#endif /* NEWLINE_FUNC_DEF_OR_CALL_H_INCLUDED */
