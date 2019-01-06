/**
 * @file align_func_params.h
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef ALIGN_FUNC_PARAMS_H_INCLUDED
#define ALIGN_FUNC_PARAMS_H_INCLUDED

#include "chunk_list.h"

void align_func_params(void);

chunk_t *align_func_param(chunk_t *start);

#endif /* ALIGN_FUNC_PARAMS_H_INCLUDED */
