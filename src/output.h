/**
 * @file output.h
 * prototypes for output.c
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef OUTPUT_H_INCLUDED
#define OUTPUT_H_INCLUDED

#include "unc_text.h"

#include <stdio.h>


//! This renders the chunk list to a file.
void output_parsed(FILE *pfile, bool withOptions = true);


//! This renders the chunk list to a file formatted as csv.
void output_parsed_csv(FILE *pfile);


//! This renders the chunk list to a file.
void output_text(FILE *pfile);


//! This save the next formatting step to a file
void dump_step(const char *filename, const char *description);


/**
 * See also it's preprocessor counterpart
 *   add_long_closebrace_comment
 * in braces.cpp
 *
 * Note: since this concerns itself with the preprocessor -- which is line-oriented --
 * it turns out that just looking at pc->GetPpLevel() is NOT the right thing to do.
 * See a --parsed dump if you don't believe this: an '#endif' will be one level
 * UP from the corresponding #ifdef when you look at the tokens 'ifdef' versus 'endif',
 * but it's a whole another story when you look at their CT_PREPROC ('#') tokens!
 *
 * Hence we need to track and seek matching CT_PREPROC pp_levels here, which complicates
 * things a little bit, but not much.
 */
void add_long_preprocessor_conditional_block_comment();


#endif /* OUTPUT_H_INCLUDED */
