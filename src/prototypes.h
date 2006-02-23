/**
 * @file prototypes.c
 * Big jumble of prototypes used in Uncrustify.
 *
 * $Id: prototypes.h,v 1.19 2006/02/13 03:30:20 bengardner Exp $
 */
#ifndef C_PARSE_PROTOTYPES_H_INCLUDED
#define C_PARSE_PROTOTYPES_H_INCLUDED

#include "cparse_types.h"

BOOL chunk_comment(chunk_t *pc);
BOOL chunk_number(chunk_t *pc);
BOOL chunk_string(chunk_t *pc);
BOOL chunk_word(chunk_t *pc);
BOOL chunk_next(chunk_t *pc);
BOOL chunk_whitespace(chunk_t *pc);


chunk_t *output_square(chunk_t *pc);
chunk_t *output_brace(chunk_t *pc);
chunk_t *output_paren(chunk_t *pc);
chunk_t *output_preproc(chunk_t *pc);
chunk_t *output_incdec(chunk_t *pc);
chunk_t *output_return(chunk_t *pc);
chunk_t *output_sizeof(chunk_t *pc);
chunk_t *output_enum(chunk_t *pc);
chunk_t *output_struct(chunk_t *pc);
chunk_t *output_typedef(chunk_t *pc);
chunk_t *output_do(chunk_t *pc);
chunk_t *output_switch(chunk_t *pc);
chunk_t *output_while(chunk_t *pc);
chunk_t *output_for(chunk_t *pc);
chunk_t *output_else(chunk_t *pc);
chunk_t *output_if(chunk_t *pc);
chunk_t *output_junk(chunk_t *pc);
chunk_t *output_word(chunk_t *pc);
chunk_t *output_comment(chunk_t *pc);


void output_text(FILE *pfile);
void output_parsed(FILE *pfile);
void output_comment_multi(chunk_t *pc);
void output_options(FILE *pfile);
void output_indent(int level, BOOL allow_tabs);
void output_to_column(int column, BOOL allow_tabs);
void add_text(const char *text);
void add_char(char ch);
void set_arg_defaults(void);


chunk_t *reformat_typedef(chunk_t *pc);


void indent_column(chunk_t *pc, int column);
void reindent_line(chunk_t *pc, int column);
void indent_level(void);
void align_all(void);
void align_backslash_newline(void);
void align_right_comments(void);
void align_func_proto(int span);
void align_preprocessor(void);
void align_struct_initializers(void);


chunk_t *align_nl_cont(chunk_t *start);
chunk_t *align_assign(chunk_t *first, int span);


void combine_labels(void);
void do_braces(void);
void space_text(void);
void fix_symbols(void);
void newlines_cleanup_braces(void);
void newlines_squeeze_ifdef(void);
void do_stuff(void);
void parse_buffer(const char *data, int data_len);
BOOL parse_next(chunk_t *pc);


const chunk_tag_t *find_keyword(const char *word, int len);
const chunk_tag_t *find_punctuator(const char *str, uint8_t lang_flags);

void indent_comments(void);
void mark_comments(void);


void pf_copy(struct parse_frame *dst, const struct parse_frame *src);
void pf_push(struct parse_frame *pf);
void pf_push_under(struct parse_frame *pf);
void pf_copy_tos(struct parse_frame *pf);
void pf_trash_tos(void);
void pf_pop(struct parse_frame *pf);
void pf_check(struct parse_frame *frm, chunk_t *pc);

void indent_text(void);


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
   return(calc_next_tab_column(col, cpd.settings[UO_output_tab_size]));
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
   if ((col % cpd.settings[UO_output_tab_size]) != 1)
   {
      calc_next_tab_column(col, cpd.settings[UO_output_tab_size]);
   }
   return(col);
}

int load_config_file(const char *filename);
int set_option_value(const char *name, const char *value);


const char *get_option_name(enum uncrustify_options uo);
const char *get_token_name(c_token_t token);

#endif   /* C_PARSE_PROTOTYPES_H_INCLUDED */

