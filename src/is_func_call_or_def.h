/**
 * @file is_func_call_or_def.h
 * prototype for is_func_call_or_def.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef IS_FUNC_CALL_OR_DEF_H_INCLUDED
#define IS_FUNC_CALL_OR_DEF_H_INCLUDED

#include "chunk.h"


/**
 * Test if an opening brace is part of a function call or definition.
 */
bool is_func_call_or_def(Chunk *pc);


#endif /* IS_FUNC_CALL_OR_DEF_H_INCLUDED */
