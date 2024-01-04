/**
 * @file one_liner_nl_ok.h
 * prototype for one_liner_nl_ok.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef ONE_LINER_NL_OK_H_INCLUDED
#define ONE_LINER_NL_OK_H_INCLUDED

#include "chunk.h"

/**
 * Checks to see if it is OK to add a newline around the chunk.
 * Don't want to break one-liners...
 * return value:
 *  true: a new line may be added
 * false: a new line may NOT be added
 */
bool one_liner_nl_ok(Chunk *pc);

#endif /* ONE_LINER_NL_OK_H_INCLUDED */
