/**
 * @file blank_line_set.h
 * prototype for blank_line_set.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef BLANK_LINE_SET_H_INCLUDED
#define BLANK_LINE_SET_H_INCLUDED

#include "chunk.h"
#include "option.h"

using namespace uncrustify;

#define MARK_CHANGE()    mark_change(__func__, __LINE__)

void blank_line_set(Chunk *pc, Option<unsigned> &opt);

#endif /* BLANK_LINE_SET_H_INCLUDED */
