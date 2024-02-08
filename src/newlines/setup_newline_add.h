/**
 * @file setup_newline_add.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef SETUP_NEWLINES_ADD_H_INCLUDED
#define SETUP_NEWLINES_ADD_H_INCLUDED

class Chunk;

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

#endif /* SETUP_NEWLINES_ADD_H_INCLUDED */
