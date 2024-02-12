/**
 * @file is_var_def.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef IS_VAR_DEF_H_INCLUDED
#define IS_VAR_DEF_H_INCLUDED

class Chunk;

//! Check if token starts a variable declaration
bool is_var_def(Chunk *pc, Chunk *next);

#endif /* IS_VAR_DEF_H_INCLUDED */
