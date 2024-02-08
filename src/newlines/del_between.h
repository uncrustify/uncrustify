/**
 * @file del_between.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#ifndef NEWLINES_DEL_BETWEEN_H_INCLUDED
#define NEWLINES_DEL_BETWEEN_H_INCLUDED

class Chunk;

/**
 * Removes any CT_NEWLINE or CT_NL_CONT between start and end.
 * Start must be before end on the chunk list.
 * If the 'PCF_IN_PREPROC' status differs between two tags, we can't remove
 * the newline.
 *
 * @param start  The starting chunk (if it is a newline, it will be removed!)
 * @param end    The ending chunk (will not be removed, even if it is a newline)
 *
 * @return true/false - removed something
 */
void newline_del_between(Chunk *start, Chunk *end);

#endif /* NEWLINES_DEL_BETWEEN_H_INCLUDED */
