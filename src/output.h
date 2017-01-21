/**
 * @file output.h
 * prototypes for output.c
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef OUTPUT_H_INCLUDED
#define OUTPUT_H_INCLUDED

#include <stdio.h>
#include "unc_text.h"

/**
 * All output text is sent here, one char at a time.
 */
static void add_char(UINT32 ch);

/**
 * Advance to a specific column
 * cpd.column is the current column
 *
 * @param column  The column to advance to
 */
static void output_to_column(size_t column, bool allow_tabs);


/**
 * This renders the chunk list to a file.
 */
void output_text(FILE *pfile);


void output_parsed(FILE *pfile);


/**
 * See also it's preprocessor counterpart
 *   add_long_closebrace_comment
 * in braces.cpp
 *
 * Note: since this concerns itself with the preprocessor -- which is line-oriented --
 * it turns out that just looking at pc->pp_level is NOT the right thing to do.
 * See a --parsed dump if you don't believe this: an '#endif' will be one level
 * UP from the corresponding #ifdef when you look at the tokens 'ifdef' versus 'endif',
 * but it's a whole another story when you look at their CT_PREPROC ('#') tokens!
 *
 * Hence we need to track and seek matching CT_PREPROC pp_levels here, which complicates
 * things a little bit, but not much.
 */
void add_long_preprocessor_conditional_block_comment(void);


/**
 * Output a comment to the column using indent_with_tabs and
 * indent_cmt_with_tabs as the rules.
 * base_col is the indent of the first line of the comment.
 * On the first line, column == base_col.
 * On subsequent lines, column >= base_col.
 *
 * @param brace_col the brace-level indent of the comment
 * @param base_col  the indent of the start of the comment (multiline)
 * @param column    the column that we should end up in
 */
static void cmt_output_indent(int brace_col, int base_col, int column);


#endif /* OUTPUT_H_INCLUDED */
