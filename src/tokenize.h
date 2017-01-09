/**
 * @file tokenize.h
 * prototypes for tokenize.c
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef TOKENIZE_H_INCLUDED
#define TOKENIZE_H_INCLUDED

#include "uncrustify_types.h"


void tokenize(const deque<int> &data, chunk_t *ref);

#endif /* TOKENIZE_H_INCLUDED */
