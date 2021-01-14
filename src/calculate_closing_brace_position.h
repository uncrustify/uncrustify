/**
 * @file calculate_closing_brace_position.h
 * prototype for calculate_closing_brace_position.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#ifndef CALCULATE_CLOSING_BRACE_POSITION_H_INCLUDED
#define CALCULATE_CLOSING_BRACE_POSITION_H_INCLUDED

#include "uncrustify_types.h"


chunk_t *calculate_closing_brace_position(const chunk_t *cl_colon, chunk_t *pc);


#endif /* CALCULATE_CLOSING_BRACE_POSITION_H_INCLUDED */
