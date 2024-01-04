/**
 * @file newlines_do_else.h
 * rototype for newlines_do_else.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef NEWLINES_DO_ELSE_H_INCLUDED
#define NEWLINES_DO_ELSE_H_INCLUDED

#include "chunk.h"

using namespace uncrustify;

/**
 * Adds/removes a newline between else and '{'.
 * "else {" or "else \n {"
 */
void newlines_do_else(Chunk *start, iarf_e nl_opt);

#endif /* NEWLINES_DO_ELSE_H_INCLUDED */
