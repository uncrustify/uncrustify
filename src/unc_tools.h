/**
 * @file unc_tools.h
 *
 * @author  Guy Maurel
 *          October 2015, 2024
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


void prot_the_line(const char *func_name, int theLine_of_code, size_t the_line_to_be_prot, size_t partNumber);


void prot_the_line_pc(Chunk *pc_sub, const char *func_name, int theLine_of_code, size_t the_line_to_be_prot, size_t partNumber);


void prot_some_lines(const char *func_name, int theLine_of_code, size_t from_line, size_t to_line);


void prot_all_lines(const char *func_name, int theLine_of_code);


void prot_the_source(int theLine_of_code);


void prot_the_columns(int theLine_of_code, size_t the_line_to_be_prot);


void prot_the_OrigCols(int theLine_of_code, size_t the_line_to_be_prot);


void rebuild_the_line(int theLine_of_code, size_t the_line_to_be_prot, bool increment = true);


void examine_Data(const char *func_name, int theLine_of_code, int what);


//! dump the chunk list to a file
void dump_out(size_t type);


//! create chunk list from a file
void dump_in(size_t type);


size_t get_A_Number();


void dump_keyword_for_lang(size_t language_count, chunk_tag_t *keyword_for_lang);

//! This save the next formatting step to a file
void dump_step(const char *filename, const char *description);


//! give the oportunity to examine most(all) the data members of a single token
//! may be inserted everythere to follow a value
void examine_token(const char *func_name, int theLine_of_code, size_t orig_line_to_examine, size_t orig_column_to_examine);

#endif /* UNC_TOOLS_H_INCLUDED */
