/**
 * @file func_def_or_call.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef NEWLINES_FUNC_DEF_OR_CALL_H_INCLUDED
#define NEWLINES_FUNC_DEF_OR_CALL_H_INCLUDED

class Chunk;

/**
 * Formats a function declaration
 * Start points to the open paren
 */
void newline_func_def_or_call(Chunk *start);

/**
 * Adds newlines to multi-line function call/decl/def
 * Start points to the open paren
 */
void newline_func_multi_line(Chunk *start);

#endif /* NEWLINES_FUNC_DEF_OR_CALL_H_INCLUDED */
