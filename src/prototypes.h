/**
 * @file prototypes.h
 * Big jumble of prototypes used in Uncrustify.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef C_PARSE_PROTOTYPES_H_INCLUDED
#define C_PARSE_PROTOTYPES_H_INCLUDED

#include "uncrustify_types.h"

#include <string>

/*
 *  uncrustify.cpp
 */

const char *get_token_name(c_token_t token);
c_token_t find_token_name(const char *text);
void log_pcf_flags(log_sev_t sev, UINT32 flags);
const char *path_basename(const char *path);
int path_dirname_len(const char *filename);
const char *get_file_extension(int& idx);


/*
 * detect.cpp
 */

void detect_options(const char *data, int data_len);


/*
 *  output.cpp
 */

void output_text(FILE *pfile);
void output_parsed(FILE *pfile);
void output_options(FILE *pfile);


/*
 *  options.cpp
 */

void unc_begin_group(uncrustify_groups id, const char *short_desc, const char *long_desc = NULL);
void register_options(void);
void set_option_defaults(void);
int load_option_file(const char *filename);
int save_option_file(FILE *pfile, bool withDoc);
int set_option_value(const char *name, const char *value);
const group_map_value *get_group_name(int ug);
const option_map_value *get_option_name(int uo);
void print_options(FILE *pfile, bool verbose);

std::string argtype_to_string(argtype_e argtype);
std::string bool_to_string(bool val);
std::string argval_to_string(argval_t argval);
std::string number_to_string(int number);
std::string lineends_to_string(lineends_e linends);
std::string tokenpos_to_string(tokenpos_e tokenpos);
std::string op_val_to_string(argtype_e argtype, op_val_t op_val);

/*
 *  indent.cpp
 */

void indent_text(void);
void indent_preproc(void);
void indent_to_column(chunk_t *pc, int column);
void align_to_column(chunk_t *pc, int column);

#define reindent_line(pc, col)    reindent_line2(pc, col, __func__, __LINE__)
void reindent_line2(chunk_t *pc, int column, const char *fcn_name, int lineno);


/*
 *  align.cpp
 */

void align_all(void);
void align_backslash_newline(void);
void align_right_comments(void);
void align_preprocessor(void);
void align_struct_initializers(void);
chunk_t *align_nl_cont(chunk_t *start);
chunk_t *align_assign(chunk_t *first, int span, int thresh);
void quick_align_again(void);


/*
 *  braces.cpp
 */

void do_braces(void);
void add_long_closebrace_comment(void);
chunk_t *insert_comment_after(chunk_t *ref, c_token_t cmt_type, int cmt_len, const char *cmt_text);


/*
 *  sorting.cpp
 */

void sort_imports(void);


/*
 *  parens.cpp
 */

void do_parens(void);


/*
 *  space.cpp
 */

void space_text(void);
void space_text_balance_nested_parens(void);
int space_col_align(chunk_t *first, chunk_t *second);
bool space_needed(chunk_t *first, chunk_t *second);
void space_add_after(chunk_t *pc, int count);


/*
 *  combine.cpp
 */

void fix_symbols(void);
void combine_labels(void);
void mark_comments(void);
void make_type(chunk_t *pc);

chunk_t *skip_template_next(chunk_t *ang_open);
chunk_t *skip_template_prev(chunk_t *ang_close);

chunk_t *skip_attribute_next(chunk_t *attr);
chunk_t *skip_attribute_prev(chunk_t *fp_close);

void remove_extra_returns();


/*
 *  newlines.cpp
 */

void newlines_cleanup_braces(void);
void newlines_insert_blank_lines(void);
void newlines_squeeze_ifdef(void);
void newlines_eat_start_end(void);
void newlines_chunk_pos(c_token_t chunk_type, tokenpos_e mode);
void newlines_class_colon_pos(void);
void newlines_cleanup_dup(void);
void newline_after_multiline_comment(void);
void do_blank_lines(void);
void newline_iarf(chunk_t *pc, argval_t av);

#define newline_add_before(pc)    newline_add_before2(pc, __func__, __LINE__)
chunk_t *newline_add_before2(chunk_t *pc, const char *fcn, int line);

#define newline_add_after(pc)     newline_add_after2(pc, __func__, __LINE__)
chunk_t *newline_add_after2(chunk_t *pc, const char *fcn, int line);


#define newline_del_between(start, end) \
   newline_del_between2(start, end, __func__, __LINE__)

void newline_del_between2(chunk_t *start, chunk_t *end,
                          const char *func, int line);

#define newline_add_between(start, end) \
   newline_add_between2(start, end, __func__, __LINE__)

chunk_t *newline_add_between2(chunk_t *start, chunk_t *end,
                              const char *func, int line);

/*
 *  tokenize.cpp
 */

void tokenize(const char *data, int data_len, chunk_t *ref);


/*
 *  tokenize_cleanup.cpp
 */

void tokenize_cleanup(void);


/*
 *  brace_cleanup.cpp
 */

void brace_cleanup(void);


/*
 *  keywords.cpp
 */

int load_keyword_file(const char *filename);
const chunk_tag_t *find_keyword(const char *word, int len);
void add_keyword(const char *tag, c_token_t type, int lang_flags);
void output_types(FILE *pfile);
const chunk_tag_t *get_custom_keyword_idx(int& idx);
void clear_keyword_file(void);
pattern_class get_token_pattern_class(c_token_t tok);
bool keywords_are_sorted(void);


/*
 *  defines.cpp
 */

int load_define_file(const char *filename);
const define_tag_t *find_define(const char *word, int len);
void add_define(const char *tag, const char *value);
const define_tag_t *get_define_idx(int& idx);
void output_defines(FILE *pfile);
void clear_defines(void);
void add_long_preprocessor_conditional_block_comment(void);


/*
 *  punctuators.cpp
 */
const chunk_tag_t *find_punctuator(const char *str, int lang_flags);


/*
 *  parse_frame.cpp
 */

void pf_copy(struct parse_frame *dst, const struct parse_frame *src);
void pf_push(struct parse_frame *pf);
void pf_push_under(struct parse_frame *pf);
void pf_copy_tos(struct parse_frame *pf);
void pf_trash_tos(void);
void pf_pop(struct parse_frame *pf);
int pf_check(struct parse_frame *frm, chunk_t *pc);


/*
 * width.cpp
 */
void do_code_width(void);


/*
 * lang_pawn.cpp
 */
void pawn_prescan(void);
void pawn_add_virtual_semicolons();
chunk_t *pawn_check_vsemicolon(chunk_t *pc);
void pawn_scrub_vsemi(void);
chunk_t *pawn_add_vsemi_after(chunk_t *pc);


/*
 * universalindentgui.cpp
 */
void print_universal_indent_cfg(FILE *pfile);


/*
 * semicolons.cpp
 */
void remove_extra_semicolons(void);


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
