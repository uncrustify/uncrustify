/**
 * @file unc_tools.h
 *
 * @author  Guy Maurel
 *          October 2015, 2016, 2017, 2018, 2019, 2020, 2021
 * @license GPL v2+
 */

#ifndef UNC_TOOLS_H_INCLUDED
#define UNC_TOOLS_H_INCLUDED

#include "prototypes.h"

#if defined DEBUG
#define PROT_THE_LINE    prot_the_line(__func__, __LINE__, 0, 0);
#else
#define PROT_THE_LINE    /* do nothing */;
#endif


void prot_the_line(const char *func_name, int theLine, unsigned int actual_line, size_t partNumber);


void prot_the_line_pc(Chunk *pc_sub, const char *func_name, int theLine, unsigned int actual_line, size_t partNumber);


void prot_all_lines(const char *func_name, int theLine);


void prot_the_source(int theLine);


void examine_Data(const char *func_name, int theLine, int what);


//! dump the chunk list to a file
void dump_out(unsigned int type);


//! create chunk list from a file
void dump_in(unsigned int type);


size_t get_A_Number();

#endif /* UNC_TOOLS_H_INCLUDED */
