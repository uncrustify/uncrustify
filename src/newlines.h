/**
 * @file newlines.h
 * prototypes for newlines.c
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef NEWLINES_H_INCLUDED
#define NEWLINES_H_INCLUDED

#include "uncrustify_types.h"
#include "chunk_list.h"

void newlines_remove_newlines(void);
void newlines_cleanup_braces(bool first);
void newlines_insert_blank_lines(void);
void newlines_functions_remove_extra_blank_lines(void);
void newlines_squeeze_ifdef(void);
void newlines_eat_start_end(void);
void newlines_chunk_pos(c_token_t chunk_type, tokenpos_e mode);
void newlines_class_colon_pos(c_token_t tok);
void newlines_cleanup_dup(void);
void annotations_newlines(void);
void newline_after_multiline_comment(void);
void newline_after_label_colon(void);
void do_blank_lines(void);
void undo_one_liner(chunk_t *pc);

void newline_iarf(chunk_t *pc, argval_t av);

chunk_t *newline_add_before(chunk_t *pc);
chunk_t *newline_force_before(chunk_t *pc);
chunk_t *newline_add_after(chunk_t *pc);
chunk_t *newline_force_after(chunk_t *pc);
void newline_del_between(chunk_t *start, chunk_t *end);
chunk_t *newline_add_between(chunk_t *start, chunk_t *end);

/**
 * Counts newlines between two chunk elements
 *
 * @param  pc_start chunk from which the counting of newlines will start
 * @param  pc_end   chunk at which the counting of newlines will end
 * @param  newlines reference in which the amount of newlines will be written to
 *                  (will be initialized with 0)
 * @param  scope    specifies region chunks should/should not be considered.
 *
 * @return false   if pc_start or pc_end are nullptr or if pc_end is not reached
 *         true    if above cases are not met
 */
bool newlines_between(chunk_t *pc_start, chunk_t *pc_end, size_t &newlines, scope_e scope = scope_e::ALL);

#endif /* NEWLINES_H_INCLUDED */
