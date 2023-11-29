/**
 * @file check_double_brace_init.h
 * prototypes for check_double_brace_init.cpp
 *
 * @author  Guy Maurel
 * extract from combine.cpp
 * @license GPL v2+
 */
#ifndef CHECK_DOUBLE_BRACE_INIT_H_INCLUDED
#define CHECK_DOUBLE_BRACE_INIT_H_INCLUDED

#include "chunk.h"


/**
 * Combines two tokens into {{ and }} if inside parens and nothing is between
 * either pair.
 */
void check_double_brace_init(Chunk *bo1);


#endif /* CHECK_DOUBLE_BRACE_INIT_H_INCLUDED */
