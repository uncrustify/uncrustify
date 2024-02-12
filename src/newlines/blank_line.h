/**
 * @file blank_line.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef BLANK_LINE_H_INCLUDED
#define BLANK_LINE_H_INCLUDED

#include "option.h"

class Chunk;


void blank_line_max(Chunk *pc, uncrustify::Option<unsigned> &opt);

void blank_line_set(Chunk *pc, uncrustify::Option<unsigned> &opt);

void do_blank_lines();

//! Handle insertion/removal of blank lines before if/for/while/do and functions
void newlines_insert_blank_lines();

#endif /* BLANK_LINE_H_INCLUDED */
