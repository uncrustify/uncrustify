/**
 * @file blank_line_max.h
 * prototype for blank_line_max.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef BLANK_LINE_MAX_H_INCLUDED
#define BLANK_LINE_MAX_H_INCLUDED


#include "chunk.h"
#include "option.h"

using namespace uncrustify;

void blank_line_max(Chunk *pc, Option<unsigned> &opt);

#endif /* BLANK_LINE_MAX_H_INCLUDED */
