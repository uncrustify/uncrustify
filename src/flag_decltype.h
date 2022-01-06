/**
 * @file flag_decltype.h
 *
 * @license GPL v2+
 */

#ifndef FLAG_DECLTYPE_INCLUDED
#define FLAG_DECLTYPE_INCLUDED


/**
 * Flags all chunks within a cpp decltype expression from the opening
 * brace to the closing brace
 *
 * @return Returns true if expression is a valid decltype expression
 */
bool flag_cpp_decltype(Chunk *pc);


#endif
