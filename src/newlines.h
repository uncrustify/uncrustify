/**
 * @file newlines.h
 * prototypes for newlines.cpp
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef NEWLINES_H_INCLUDED
#define NEWLINES_H_INCLUDED

#include "chunk.h"

/**
 * Double the newline, if allowed.
 */
void double_newline(Chunk *nl);

/**
 * Remove all extra newlines.
 * Modify line breaks as needed.
 */
void newlines_remove_newlines();


/**
 * Remove all newlines that fail the checks performed by the can_increase_nl() function
 */
void newlines_remove_disallowed();


/** Step through all chunks, altering newlines inside parens of if/for/while/do as needed.
 * Handles the style options: nl_multi_line_sparen_open, nl_multi_line_sparen_close, nl_before_if_closing_paren
 */
void newlines_sparens();

//! Step through all chunks.
void newlines_cleanup_braces(bool first);


void newlines_cleanup_angles();


//! Handle insertion/removal of blank lines before if/for/while/do and functions
void newlines_insert_blank_lines();


/**
 * Handle removal of extra blank lines in functions
 * x <= 0: do nothing, x > 0: allow max x-1 blank lines
 */
void newlines_functions_remove_extra_blank_lines();


void newlines_squeeze_ifdef();

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
void newlines_squeeze_paren_close();


//! removes unnecessary newlines at start and end of a file
void newlines_eat_start_end();


/**
 * Searches for a chunk of type chunk_type and moves them, if needed.
 * Will not move tokens that are on their own line or have other than
 * exactly 1 newline before (options::pos_comma() == TRAIL) or after (options::pos_comma() == LEAD).
 * We can't remove a newline if it is right before a preprocessor.
 */
void newlines_chunk_pos(E_Token chunk_type, uncrustify::token_pos_e mode);


/**
 * Searches for CT_CLASS_COLON and moves them, if needed.
 * Also breaks up the args
 */
void newlines_class_colon_pos(E_Token tok);


void newlines_cleanup_dup();


void annotations_newlines();


void newline_after_multiline_comment();


//! Handle insertion of blank lines after label colons
void newline_after_label_colon();


/**
 * Scans for newline tokens and changes the nl_count.
 * A newline token has a minimum nl_count of 1.
 * Note that a blank line is actually 2 newlines, unless the newline is the
 * first chunk.
 * So, most comparisons have +1 below.
 */
void do_blank_lines();


/**
 * Clears the PCF_ONE_LINER flag on the current line.
 * Done right before inserting a newline.
 */
void undo_one_liner(Chunk *pc);


/**
 * Does a simple Ignore, Add, Remove, or Force after the given chunk
 *
 * @param pc  The chunk
 * @param av  The IARF value
 */
void newline_iarf(Chunk *pc, uncrustify::iarf_e av);


/**
 * Add a newline before the chunk if there isn't already a newline present.
 * Virtual braces are skipped, as they do not contribute to the output.
 */
Chunk *newline_add_before(Chunk *pc);


/**
 * Add a newline after the chunk if there isn't already a newline present.
 * Virtual braces are skipped, as they do not contribute to the output.
 */
Chunk *newline_force_before(Chunk *pc);


Chunk *newline_add_after(Chunk *pc);


Chunk *newline_force_after(Chunk *pc);


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


#endif /* NEWLINES_H_INCLUDED */
