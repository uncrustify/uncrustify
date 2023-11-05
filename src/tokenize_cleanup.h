/**
 * @file tokenize_cleanup.h
 * prototypes for tokenize_cleanup.cpp
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef TOKENIZ_CLEANUP_H_INCLUDED
#define TOKENIZ_CLEANUP_H_INCLUDED

#include "uncrustify_types.h"


/**
 * @brief clean up tokens
 *
 * Change certain token types based on simple sequence.
 * Example: change '[' + ']' to '[]'
 * Note that level info is not yet available, so it is OK to do all
 * processing that doesn't need to know level info. (that's very little!)
 */
void tokenize_cleanup();


void tokenize_trailing_return_types();


void split_off_angle_close(Chunk *pc);


#endif /* TOKENIZ_CLEANUP_H_INCLUDED */
