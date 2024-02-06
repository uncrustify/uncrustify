/**
 * @file insert_blank_lines.h
 *
 * @author  Ben Gardner
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef NEWLINES_INSERT_BLANK_LINES_H_INCLUDED
#define NEWLINES_INSERT_BLANK_LINES_H_INCLUDED


#include "chunk.h"
#include "token_enum.h"


//! Handle insertion/removal of blank lines before if/for/while/do and functions
void newlines_insert_blank_lines();


#endif /* NEWLINES_INSERT_BLANK_LINES_H_INCLUDED */
