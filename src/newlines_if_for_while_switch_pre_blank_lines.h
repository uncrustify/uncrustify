/**
 * @file newlines_if_for_while_switch_pre_blank_lines.h
 * prototype for newlines_if_for_while_switch_pre_blank_lines.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef NEWLINES_IF_FOR_WHILE_SWITCH_PRE_BLANK_LINES_H_INCLUDED
#define NEWLINES_IF_FOR_WHILE_SWITCH_PRE_BLANK_LINES_H_INCLUDED

#include "newlines_if_for_while_switch_pre_blank_lines.h"

#include "chunk.h"

using namespace uncrustify;

/**
 * Add or remove extra newline before the chunk.
 * Adds before comments
 * Doesn't do anything if open brace before it
 * "code\n\ncomment\nif (...)" or "code\ncomment\nif (...)"
 */
void newlines_if_for_while_switch_pre_blank_lines(Chunk *start, iarf_e nl_opt);

#endif /* NEWLINES_IF_FOR_WHILE_SWITCH_PRE_BLANK_LINES_H_INCLUDED */
