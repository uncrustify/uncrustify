/**
 * @file var_def_blk.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef NEWLINES_VAR_DEF_BLK_H_INCLUDED
#define NEWLINES_VAR_DEF_BLK_H_INCLUDED

#include "chunk.h"


//! Put newline(s) before and/or after a block of variable definitions
Chunk *newline_var_def_blk(Chunk *start);

#endif /* NEWLINES_VAR_DEF_BLK_H_INCLUDED */
