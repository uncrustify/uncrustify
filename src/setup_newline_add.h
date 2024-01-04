/**
 * @file setup_newline_add.h
 * prototype for setup_newline_add.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef SETUP_NEWLINE_ADD_H_INCLUDED
#define SETUP_NEWLINE_ADD_H_INCLUDED

#include "chunk.h"

/**
 * Basic approach:
 * 1. Find next open brace
 * 2. Find next close brace
 * 3. Determine why the braces are there
 * a. struct/union/enum "enum [name] {"
 * c. assignment "= {"
 * b. if/while/switch/for/etc ") {"
 * d. else "} else {"
 */
void setup_newline_add(Chunk *prev, Chunk *nl, Chunk *next);


#endif /* SETUP_NEWLINE_ADD_H_INCLUDED */
