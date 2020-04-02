/**
 * @file newlines.h
 * prototypes for newlines.c
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef NEWLINES_H_INCLUDED
#define NEWLINES_H_INCLUDED

#include "chunk_list.h"
#include "uncrustify_types.h"

/**
 * Double the newline, if allowed.
 */
void double_newline(chunk_t *nl);

/**
 * Remove all extra newlines.
 * Modify line breaks as needed.
 */
void newlines_remove_newlines(void);

/** Step through all chunks, altering newlines inside parens of if/for/while/do as needed.
 * Handles the style options: nl_multi_line_sparen_open, nl_multi_line_sparen_close, nl_before_if_closing_paren
 */
void newlines_sparens();

//! Step through all chunks.
void newlines_cleanup_braces(bool first);


void newlines_cleanup_angles();


//! Handle insertion/removal of blank lines before if/for/while/do and functions
void newlines_insert_blank_lines(void);


/**
 * Handle removal of extra blank lines in functions
 * x <= 0: do nothing, x > 0: allow max x-1 blank lines
 */
void newlines_functions_remove_extra_blank_lines(void);


void newlines_squeeze_ifdef(void);

/**
 * In case of consecutive closing parens, which follow a newline,
 * the closing paren are altered to different lines, as per the respective opening parens.
 * In the given example, first 2 opening paren are in same line, hence the respective closing paren are put in the same line.
 * input:
 * func1(func2(
 *    func3(
 *       func4(
 *       )
 *    )
 * )
 * );
 * output:
 * func1(func2(
 *    func3(
 *       func4(
 *       )
 *    )
 * ));
 */
void newlines_squeeze_paren_close(void);


//! removes unnecessary newlines at start and end of a file
void newlines_eat_start_end(void);


/**
 * Searches for a chunk of type chunk_type and moves them, if needed.
 * Will not move tokens that are on their own line or have other than
 * exactly 1 newline before (UO_pos_comma == TRAIL) or after (UO_pos_comma == LEAD).
 * We can't remove a newline if it is right before a preprocessor.
 */
void newlines_chunk_pos(c_token_t chunk_type, uncrustify::token_pos_e mode);


/**
 * Searches for CT_CLASS_COLON and moves them, if needed.
 * Also breaks up the args
 */
void newlines_class_colon_pos(c_token_t tok);


void newlines_cleanup_dup(void);


void annotations_newlines(void);


void newline_after_multiline_comment(void);


//! Handle insertion of blank lines after label colons
void newline_after_label_colon(void);


/**
 * Scans for newline tokens and changes the nl_count.
 * A newline token has a minimum nl_count of 1.
 * Note that a blank line is actually 2 newlines, unless the newline is the
 * first chunk.
 * So, most comparisons have +1 below.
 */
void do_blank_lines(void);


/**
 * Clears the PCF_ONE_LINER flag on the current line.
 * Done right before inserting a newline.
 */
void undo_one_liner(chunk_t *pc);


/**
 * Does a simple Ignore, Add, Remove, or Force after the given chunk
 *
 * @param pc  The chunk
 * @param av  The IARF value
 */
void newline_iarf(chunk_t *pc, uncrustify::iarf_e av);


/**
 * Add a newline before the chunk if there isn't already a newline present.
 * Virtual braces are skipped, as they do not contribute to the output.
 */
chunk_t *newline_add_before(chunk_t *pc);


/**
 * Add a newline after the chunk if there isn't already a newline present.
 * Virtual braces are skipped, as they do not contribute to the output.
 */
chunk_t *newline_force_before(chunk_t *pc);


chunk_t *newline_add_after(chunk_t *pc);


chunk_t *newline_force_after(chunk_t *pc);


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
void newline_del_between(chunk_t *start, chunk_t *end);


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
chunk_t *newline_add_between(chunk_t *start, chunk_t *end);


/**
 * Counts newlines between two chunk elements
 *
 * @param  pc_start  chunk from which the counting of newlines will start
 * @param  pc_end    chunk at which the counting of newlines will end
 * @param  newlines  reference in which the amount of newlines will be written to
 *                   (will be initialized with 0)
 * @param  scope     specifies region chunks should/should not be considered.
 *
 * @return false  if pc_start or pc_end are nullptr or if pc_end is not reached
 * @return true   if above cases are not met
 */
bool newlines_between(chunk_t *pc_start, chunk_t *pc_end, size_t &newlines, scope_e scope = scope_e::ALL);


#endif /* NEWLINES_H_INCLUDED */
