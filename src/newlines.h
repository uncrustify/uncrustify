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

#endif /* NEWLINES_H_INCLUDED */
