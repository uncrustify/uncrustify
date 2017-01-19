/**
 * @file braces.h
 * prototypes for braces.c
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef BRACES_H_INCLUDED
#define BRACES_H_INCLUDED

#include "uncrustify_types.h"


void do_braces(void);
void add_long_closebrace_comment(void);
chunk_t *insert_comment_after(chunk_t *ref, c_token_t cmt_type, const unc_text &cmt_text);

#endif /* BRACES_H_INCLUDED */
