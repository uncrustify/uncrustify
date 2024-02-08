/**
 * @file if_for_while_switch.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef NEWLINES_IF_FOR_WHILE_SWITCH_H_INCLUDED
#define NEWLINES_IF_FOR_WHILE_SWITCH_H_INCLUDED

#include "option.h"

class Chunk;

/**
 * Add or remove a newline between the closing paren and opening brace.
 * Also uncuddles anything on the closing brace. (may get fixed later)
 *
 * "if (...) { \n" or "if (...) \n { \n"
 *
 * For virtual braces, we can only add a newline after the vbrace open.
 * If we do so, also add a newline after the vbrace close.
 */
bool newlines_if_for_while_switch(Chunk *start, uncrustify::iarf_e nl_opt);

/**
 * Add or remove extra newline after end of the block started in chunk.
 * Doesn't do anything if close brace after it
 * Interesting issue is that at this point, nls can be before or after vbraces
 * VBraces will stay VBraces, conversion to real ones should have already happened
 * "if (...)\ncode\ncode" or "if (...)\ncode\n\ncode"
 */
void newlines_if_for_while_switch_post_blank_lines(Chunk *start, uncrustify::iarf_e nl_opt);

/**
 * Add or remove extra newline before the chunk.
 * Adds before comments
 * Doesn't do anything if open brace before it
 * "code\n\ncomment\nif (...)" or "code\ncomment\nif (...)"
 */
void newlines_if_for_while_switch_pre_blank_lines(Chunk *start, uncrustify::iarf_e nl_opt);

#endif /* NEWLINES_IF_FOR_WHILE_SWITCH_H_INCLUDED */
