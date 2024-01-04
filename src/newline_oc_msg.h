/**
 * @file newline_oc_msg.h
 * prototype for newline_oc_msg.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef NEWLINE_OC_MSG_H_INCLUDED
#define NEWLINE_OC_MSG_H_INCLUDED


#include "chunk.h"

/**
 * Formats a message, adding newlines before the item before the colons.
 *
 * Start points to the open '[' in:
 * [myObject doFooWith:arg1 name:arg2  // some lines with >1 arg
 *            error:arg3];
 */
void newline_oc_msg(Chunk *start);


#endif /* NEWLINE_OC_MSG_H_INCLUDED */
