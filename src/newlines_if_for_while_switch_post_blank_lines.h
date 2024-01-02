/**
 * @file newlines_if_for_while_switch_post_blank_lines.h
 * prototype for newlines_if_for_while_switch_post_blank_lines.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef NEWLINES_IF_FOR_WHILE_SWITCH_POST_BLANK_LINES_H_INCLUDED
#define NEWLINES_IF_FOR_WHILE_SWITCH_POST_BLANK_LINES_H_INCLUDED

#include "chunk.h"

using namespace uncrustify;

/**
 * Add or remove extra newline after end of the block started in chunk.
 * Doesn't do anything if close brace after it
 * Interesting issue is that at this point, nls can be before or after vbraces
 * VBraces will stay VBraces, conversion to real ones should have already happened
 * "if (...)\ncode\ncode" or "if (...)\ncode\n\ncode"
 */
void newlines_if_for_while_switch_post_blank_lines(Chunk *start, uncrustify::iarf_e nl_opt);

#endif /* NEWLINES_IF_FOR_WHILE_SWITCH_POST_BLANK_LINES_H_INCLUDED */
