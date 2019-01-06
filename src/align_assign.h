/**
 * @file align_assign.h
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef ALIGN_ASSIGN_H_INCLUDED
#define ALIGN_ASSIGN_H_INCLUDED

#include "chunk_list.h"

/**
 * Aligns all assignment operators on the same level as first, starting with
 * first.
 * For variable definitions, only consider the '=' for the first variable.
 * Otherwise, only look at the first '=' on the line.
 *
 * @param first  chunk pointing to the first assignment
 */
chunk_t *align_assign(chunk_t *first, size_t span, size_t thresh, size_t *p_nl_count);

#endif /* ALIGN_ASSIGN_H_INCLUDED */
