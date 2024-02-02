/**
 * @file newline_var_def_blk.h
 * prototype for newline_var_def_blk.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef NEWLINE_VAR_DEF_BLK_H_INCLUDED
#define NEWLINE_VAR_DEF_BLK_H_INCLUDED

#include "chunk.h"


//! Put newline(s) before and/or after a block of variable definitions
Chunk *newline_var_def_blk(Chunk *start);

#endif /* NEWLINE_VAR_DEF_BLK_H_INCLUDED */
