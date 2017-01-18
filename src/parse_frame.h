/**
 * @file parse_frame.h
 * prototypes for parse_frame.c
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef PARSE_FRAME_H_INCLUDED
#define PARSE_FRAME_H_INCLUDED

#include "uncrustify_types.h"


void pf_copy(parse_frame_t *dst, const parse_frame_t *src);
void pf_push(parse_frame_t *pf);
void pf_push_under(parse_frame_t *pf);
void pf_copy_tos(parse_frame_t *pf);
void pf_trash_tos(void);
void pf_pop(parse_frame_t *pf);
int pf_check(parse_frame_t *frm, chunk_t *pc);

#endif /* PARSE_FRAME_H_INCLUDED */
