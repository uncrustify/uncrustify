/**
 * @file flag_decltype.h
 *
 * @license GPL v2+
 */

#ifndef TOKENIZER_FLAG_DECLTYPE_INCLUDED
#define TOKENIZER_FLAG_DECLTYPE_INCLUDED

class Chunk;

/**
 * Flags all chunks within a cpp decltype expression from the opening
 * brace to the closing brace
 *
 * @return Returns true if expression is a valid decltype expression
 */
bool flag_cpp_decltype(Chunk *pc);

#endif
