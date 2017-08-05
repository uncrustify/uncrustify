/**
 * @file unc_tools.h
 *
 * @author  Guy Maurel since version 0.62 for uncrustify4Qt
 *          October 2015, 2016
 * @license GPL v2+
 */

#ifndef UNC_TOOLS_H_INCLUDED
#define UNC_TOOLS_H_INCLUDED

#include "prototypes.h"
#include "uncrustify_types.h"
#include "chunk_list.h"


void prot_the_line(int theLine, unsigned int actual_line);


void prot_the_source(int theLine);


void examine_Data(const char *func_name, int theLine, int what);


//! dump the chunk list to a file
void dump_out(unsigned int type);


//! create chunk list from a file
void dump_in(unsigned int type);


#endif /* UNC_TOOLS_H_INCLUDED */
