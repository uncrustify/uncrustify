/**
 * @file combine_fix.h
 * prototypes for combine_fix.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 * extract fom combine.h
 */

#ifndef COMBINE_FIX_H_INCLUDED
#define COMBINE_FIX_H_INCLUDED

#include "chunk_list.h"
#include "uncrustify_types.h"


/**
 * We are on the start of a sequence that could be a variable definition
 *  - FPAREN_OPEN (parent == CT_FOR)
 *  - BRACE_OPEN
 *  - SEMICOLON
 */
chunk_t *fix_variable_definition(chunk_t *pc);


#endif /* COMBINE_FIX_H_INCLUDED */
