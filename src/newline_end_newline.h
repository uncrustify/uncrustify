/**
 * @file newline_end_newline.h
 * @file newline_end_newline.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef NEWLINE_END_NEWLINE_H_INCLUDED
#define NEWLINE_END_NEWLINE_H_INCLUDED

#include "chunk.h"

//! Ensure that the next non-comment token after close brace is a newline
void newline_end_newline(Chunk *br_close);


#endif /* NEWLINE_END_NEWLINE_H_INCLUDED */
