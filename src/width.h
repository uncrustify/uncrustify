/**
 * @file width.h
 * prototypes for width.c
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef WIDTH_H_INCLUDED
#define WIDTH_H_INCLUDED

#include "uncrustify_types.h"


/**
 * Step forward until a token goes beyond the limit and then call split_line()
 * to split the line at or before that point.
 */
void do_code_width(void);


#endif /* WIDTH_H_INCLUDED */
