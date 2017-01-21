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

/**
 * Logs the entire parse frame stack
 */
void pf_log_all(log_sev_t logsev);


/**
 * Copies src to dst.
 */
void pf_copy(parse_frame_t *dst, const parse_frame_t *src);


/**
 * Push a copy of the parse frame onto the stack.
 * This is called on #if and #ifdef.
 */
void pf_push(parse_frame_t *pf);
void pf_push_under(parse_frame_t *pf);
void pf_copy_tos(parse_frame_t *pf);
void pf_trash_tos(void);
void pf_pop(parse_frame_t *pf);
int pf_check(parse_frame_t *frm, chunk_t *pc);


/**
 * Logs one parse frame
 */
void pf_log(log_sev_t logsev, parse_frame_t *pf);


#endif /* PARSE_FRAME_H_INCLUDED */
