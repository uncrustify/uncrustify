/**
 * @file can_increase_nl.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef CAN_INCREASE_NL_H_INCLUDED
#define CAN_INCREASE_NL_H_INCLUDED

class Chunk;

/**
 * Check to see if we are allowed to increase the newline count.
 * We can't increase the newline count:
 *  - if nl_squeeze_ifdef and a preproc is after the newline.
 *  - if eat_blanks_before_close_brace and the next is '}'
 *    - unless function contains an empty body and
 *      nl_inside_empty_func is non-zero
 *  - if eat_blanks_after_open_brace and the prev is '{'
 *    - unless the brace belongs to a namespace
 *      and nl_inside_namespace is non-zero
 */
bool can_increase_nl(Chunk *nl);

#endif /* CAN_INCREASE_NL_H_INCLUDED */
