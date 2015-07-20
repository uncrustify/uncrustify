/**
 * @file tokenize_cleanup.h
 * prototypes for tokenize_cleanup.c
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef TOKENIZ_CLEANUP_H_INCLUDED
#define TOKENIZ_CLEANUP_H_INCLUDED

#include "uncrustify_types.h"


void tokenize_cleanup(void);
void split_off_angle_close(chunk_t *pc);

#endif /* TOKENIZ_CLEANUP_H_INCLUDED */
