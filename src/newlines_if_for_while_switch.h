/**
 * @file newlines_if_for_while_switch.h
 * prototype for newlines_if_for_while_switch.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef NEWLINES_IF_FOR_WHILE_SWITCH_H_INCLUDED
#define NEWLINES_IF_FOR_WHILE_SWITCH_H_INCLUDED

#include "chunk.h"

using namespace uncrustify;

/**
 * Add or remove a newline between the closing paren and opening brace.
 * Also uncuddles anything on the closing brace. (may get fixed later)
 *
 * "if (...) { \n" or "if (...) \n { \n"
 *
 * For virtual braces, we can only add a newline after the vbrace open.
 * If we do so, also add a newline after the vbrace close.
 */
bool newlines_if_for_while_switch(Chunk *start, iarf_e nl_opt);

#endif /* NEWLINES_IF_FOR_WHILE_SWITCH_H_INCLUDED */
