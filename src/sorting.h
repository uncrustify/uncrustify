/**
 * @file sorting.h
 * prototypes for sorting.c
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef SORTING_H_INCLUDED
#define SORTING_H_INCLUDED

#include "uncrustify_types.h"
#include <stdlib.h>

#define MAX_NUMBER_TO_SORT                           1024
#define MAX_LINES_TO_CHECK_FOR_SORT_AFTER_INCLUDE    128
#define MAX_GAP_THRESHOLD_BETWEEN_INCLUDE_TO_SORT    32

/**
 * alphabetically sort the #include or #import
 * statements of a file
 *
 * @todo better use a chunk pointer parameter
 * instead of a global variable
 */
void sort_imports(void);


#endif /* SORTING_H_INCLUDED */
