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

#define MAX_NUMBER_TO_SORT    256


/**
 * alphabetically sort the #include or #import
 * statements of a file
 *
 * @todo better use a chunk pointer parameter
 * instead of a global variable
 */
void sort_imports(void);


#endif /* SORTING_H_INCLUDED */
