/**
 * @file do_else.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef NEWLINES_DO_ELSE_H_INCLUDED
#define NEWLINES_DO_ELSE_H_INCLUDED

#include "option.h"

class Chunk;

/**
 * Adds/removes a newline between else and '{'.
 * "else {" or "else \n {"
 */
void newlines_do_else(Chunk *start, uncrustify::iarf_e nl_opt);

#endif /* NEWLINES_DO_ELSE_H_INCLUDED */
