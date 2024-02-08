/**
 * @file blank_line.h
 * prototype for blank_line*.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef BLANK_LINE_H_INCLUDED
#define BLANK_LINE_H_INCLUDED

#include "chunk.h"
#include "option.h"

using namespace uncrustify;

void blank_line_max(Chunk *pc, Option<unsigned> &opt);

void blank_line_set(Chunk *pc, Option<unsigned> &opt);

void do_blank_lines();

//! Handle insertion/removal of blank lines before if/for/while/do and functions
void newlines_insert_blank_lines();

#endif /* BLANK_LINE_H_INCLUDED */
