/**
 * @file align_init_brace.h
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef ALIGN_INIT_BRACE_H_INCLUDED
#define ALIGN_INIT_BRACE_H_INCLUDED

#include "uncrustify_types.h"

/**
 * Generically aligns on '=', '{', '(' and item after ','
 * It scans the first line and picks up the location of those tags.
 * It then scans subsequent lines and adjusts the column.
 * Finally it does a second pass to align everything.
 *
 * Aligns all the '=' signs in structure assignments.
 * a = {
 *    .a    = 1;
 *    .type = fast;
 * };
 *
 * And aligns on '{', numbers, strings, words.
 * colors[] = {
 *    {"red",   {255, 0,   0}}, {"blue",   {  0, 255, 0}},
 *    {"green", {  0, 0, 255}}, {"purple", {255, 255, 0}},
 * };
 *
 * For the C99 indexed array assignment, the leading []= is skipped (no aligning)
 * struct foo_t bars[] =
 * {
 *    [0] = { .name = "bar",
 *            .age  = 21 },
 *    [1] = { .name = "barley",
 *            .age  = 55 },
 * };
 *
 * NOTE: this assumes that spacing is at the minimum correct spacing (ie force)
 *       if it isn't, some extra spaces will be inserted.
 *
 * @param start   Points to the open brace chunk
 */
void align_init_brace(chunk_t *start);

#endif /* ALIGN_INIT_BRACE_H_INCLUDED */
