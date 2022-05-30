/**
 * @file flag_braced_init_list.h
 *
 * @license GPL v2+
 */

#ifndef FLAG_BRACED_INIT_LIST_INCLUDED
#define FLAG_BRACED_INIT_LIST_INCLUDED

#include "chunk.h"


/**
 * Detect a cpp braced init list
 */
bool detect_cpp_braced_init_list(Chunk *pc, Chunk *next);


/**
 * Flags the opening and closing braces of an expression deemed to be
 * a cpp braced initializer list; a call to detect_cpp_braced_init_list()
 * should first be made prior to calling this function
 */
void flag_cpp_braced_init_list(Chunk *pc, Chunk *next);


#endif
