/**
 * @file unc_tools.h
 *
 * @author  Guy Maurel
 *          October 2015, 2023
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

extern char dump_file_name[80];

void set_dump_file_name(const char *name);


void prot_the_line(const char *func_name, int theLine, unsigned int actual_line, size_t partNumber);


void prot_the_line_pc(Chunk *pc_sub, const char *func_name, int theLine, unsigned int actual_line, size_t partNumber);


void prot_all_lines(const char *func_name, int theLine);


void prot_the_source(int theLine);


void prot_the_columns(int theLine, unsigned int actual_line);


void prot_the_OrigCols(int theLine, unsigned int actual_line);


void rebuild_the_line(int theLine, unsigned int actual_line, bool increment = true);


void examine_Data(const char *func_name, int theLine, int what);


//! dump the chunk list to a file
void dump_out(unsigned int type);


//! create chunk list from a file
void dump_in(unsigned int type);


size_t get_A_Number();


void dump_keyword_for_lang(size_t language_count, chunk_tag_t *keyword_for_lang);

//! This save the next formatting step to a file
void dump_step(const char *filename, const char *description);


//! give the oportunity to examine most(all) the data members of a single token
//! may be inserted everythere to follow a value
void examine_token(const char *func_name, int theLine, size_t orig_line_to_examine, size_t orig_column_to_examine);

#endif /* UNC_TOOLS_H_INCLUDED */
