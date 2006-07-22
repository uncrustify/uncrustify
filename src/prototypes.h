/**
 * @file prototypes.h
 * Big jumble of prototypes used in Uncrustify.
 *
 * $Id$
 */
#ifndef C_PARSE_PROTOTYPES_H_INCLUDED
#define C_PARSE_PROTOTYPES_H_INCLUDED

#include "uncrustify_types.h"


/*
 *  uncrustify.c
 */

const char *get_token_name(c_token_t token);
void log_pcf_flags(log_sev_t sev, UINT32 flags);


/*
 *  c_output.c
 */

void output_text(FILE *pfile);
void output_parsed(FILE *pfile);
void output_comment_multi(chunk_t *pc);
void output_options(FILE *pfile);
void output_to_column(int column, bool allow_tabs);
void add_text(const char *text);
void add_text_len(const char *text, int len);
void add_char(char ch);


/*
 *  c_args.c
 */

void set_option_defaults(void);
int load_option_file(const char *filename);
int set_option_value(const char *name, const char *value);
const options_name_tab *get_option_name(int uo);


/*
 *  c_indent.c
 */

void indent_text(void);
void indent_preproc(void);
void indent_to_column(chunk_t *pc, int column);
void reindent_line(chunk_t *pc, int column);


/*
 *  c_align.c
 */

void align_all(void);
void align_backslash_newline(void);
void align_right_comments(void);
void align_func_proto(int span);
void align_preprocessor(void);
void align_struct_initializers(void);
chunk_t *align_nl_cont(chunk_t *start);
chunk_t *align_assign(chunk_t *first, int span, int thresh);


/*
 *  braces.c
 */

void do_braces(void);


/*
 *  c_space.c
 */

void space_text(void);
int space_col_align(chunk_t *first, chunk_t *second);


/*
 *  c_combine.c
 */

void fix_symbols(void);
void combine_labels(void);
void mark_comments(void);
void make_type(chunk_t *pc);


/*
 *  c_newlines.c
 */

void newlines_cleanup_braces(void);
void newlines_squeeze_ifdef(void);
void newlines_eat_start_end(void);
void newlines_bool_pos(void);
void do_blank_lines(void);


/*
 *  tokenize.c
 */

void tokenize(const char *data, int data_len);


/*
 *  tokenize_cleanup.c
 */

void tokenize_cleanup(void);


/*
 *  brace_cleanup.c
 */

void brace_cleanup(void);


/*
 *  keywords.c
 */

int load_keyword_file(const char *filename);
const chunk_tag_t *find_keyword(const char *word, int len);
void add_keyword(const char *tag, c_token_t type, UINT8 lang_flags);
void output_types(FILE *pfile);


/*
 *  defines.c
 */

int load_define_file(const char *filename);
const define_tag_t *find_define(const char *word, int len);
void add_define(const char *tag, const char *value);
void output_defines(FILE *pfile);


/*
 *  punctuators.c
 */
const chunk_tag_t *find_punctuator(const char *str, UINT8 lang_flags);


/*
 *  parse_frame.c
 */

void pf_copy(struct parse_frame *dst, const struct parse_frame *src);
void pf_push(struct parse_frame *pf);
void pf_push_under(struct parse_frame *pf);
void pf_copy_tos(struct parse_frame *pf);
void pf_trash_tos(void);
void pf_pop(struct parse_frame *pf);
int  pf_check(struct parse_frame *frm, chunk_t *pc);



/**
 * Advances to the next tab stop.
 * Column 1 is the left-most column.
 *
 * @param col     The current column
 * @param tabsize The tabsize
 * @return the next tabstop column
 */
static_inline
int calc_next_tab_column(int col, int tabsize)
{
   if (col <= 0)
   {
      col = 1;
   }
   col = 1 + ((((col - 1) / tabsize) + 1) * tabsize);
   return(col);
}

/**
 * Advances to the next tab stop for output.
 *
 * @param col  The current column
 * @return the next tabstop column
 */
static_inline
int next_tab_column(int col)
{
   return(calc_next_tab_column(col, cpd.settings[UO_output_tab_size].n));
}

/**
 * Advances to the next tab stop if not currently on one.
 *
 * @param col  The current column
 * @return the next tabstop column
 */
static_inline
int align_tab_column(int col)
{
   if ((col % cpd.settings[UO_output_tab_size].n) != 1)
   {
      calc_next_tab_column(col, cpd.settings[UO_output_tab_size].n);
   }
   return(col);
}


#endif   /* C_PARSE_PROTOTYPES_H_INCLUDED */
