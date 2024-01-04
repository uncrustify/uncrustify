/**
 * @file newline_add_between.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef NEWLINE_ADD_BETWEEN_H_INCLUDED
#define NEWLINE_ADD_BETWEEN_H_INCLUDED

#include "chunk.h"


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

#endif /* NEWLINE_ADD_BETWEEN_H_INCLUDED */
