/**
 * @file add.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef NEWLINES_ADD_H_INCLUDED
#define NEWLINES_ADD_H_INCLUDED

class Chunk;

Chunk *newline_add_after(Chunk *pc);
Chunk *newline_add_before(Chunk *pc);

/**
 * Add a newline between two tokens.
 * If there is already a newline between then, nothing is done.
 * Otherwise a newline is inserted.
 *
 * If end is CT_BRACE_OPEN and a comment and newline follow, then
 * the brace open is moved instead of inserting a newline.
 *
 * In this situation:
 *    if (...) { //comment
 *
 * you get:
 *    if (...)   //comment
 *    {
 */
Chunk *newline_add_between(Chunk *start, Chunk *end);

#endif /* NEWLINES_ADD_H_INCLUDED */
