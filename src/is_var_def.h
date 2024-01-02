/**
 * @file is_var_def.h
 * prototypes for is_var_def.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef IS_VAR_DEF_H_INCLUDED
#define IS_VAR_DEF_H_INCLUDED

#include "chunk.h"

//! Check if token starts a variable declaration
bool is_var_def(Chunk *pc, Chunk *next);


#endif /* IS_VAR_DEF_H_INCLUDED */
