/**
 * @file skip_c99_array.h
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef SKIP_C99_ARRAY
#define SKIP_C99_ARRAY

#include "uncrustify_types.h"

/**
 * @brief return the chunk the follows after a C array
 *
 * The provided chunk is considered an array if it is an opening square
 * (CT_SQUARE_OPEN) and the matching close is followed by an equal sign '='
 *
 * Example:                  array[25] = 12;
 *                               /|\     /|\
 *                                |       |
 * provided chunk has to point to [       |
 * returned chunk points to              12
 *
 * @param chunk  chunk to operate on
 *
 * @return the chunk after the '=' if the check succeeds
 * @return nullptr in all other cases
 */
chunk_t *skip_c99_array(chunk_t *sq_open);

#endif /* SKIP_C99_ARRAY */
