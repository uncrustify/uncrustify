/**
 * @file oc_msg.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef NEWLINES_OC_MSG_H_INCLUDED
#define NEWLINES_OC_MSG_H_INCLUDED

class Chunk;

/**
 * Formats a message, adding newlines before the item before the colons.
 *
 * Start points to the open '[' in:
 * [myObject doFooWith:arg1 name:arg2  // some lines with >1 arg
 *            error:arg3];
 */
void newline_oc_msg(Chunk *start);


#endif /* NEWLINES_OC_MSG_H_INCLUDED */
