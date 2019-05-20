/**
 * @file unc_tools.h
 *
 * @author  Guy Maurel
 *          October 2015, 2016, 2017, 2018, 2019
 * @license GPL v2+
 */

#ifndef UNC_TOOLS_H_INCLUDED
#define UNC_TOOLS_H_INCLUDED

//#define DEVELOP_ONLY
#ifdef DEVELOP_ONLY
#include "chunk_list.h"
#include "prototypes.h"
#include "uncrustify_types.h"


void prot_the_line(int theLine, unsigned int actual_line);


void prot_the_source(int theLine);


void examine_Data(const char *func_name, int theLine, int what);


//! dump the chunk list to a file
void dump_out(unsigned int type);


//! create chunk list from a file
void dump_in(unsigned int type);


#endif /* DEVELOP_ONLY */
#endif /* UNC_TOOLS_H_INCLUDED */
