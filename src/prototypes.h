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
#include "chunk_list.h"

#include <string>
#include <deque>

/*
 *  uncrustify.cpp
 */

const char *get_token_name(c_token_t token);
c_token_t find_token_name(const char *text);
void log_pcf_flags(log_sev_t sev, UINT64 flags);
const char *path_basename(const char *path);
int path_dirname_len(const char *filename);
const char *get_file_extension(int& idx);
void print_extensions(FILE *pfile);
const char *extension_add(const char *ext_text, const char *lang_text);


/*
 * detect.cpp
 */

void detect_options();


/*
 *  output.cpp
 */

void output_text(FILE *pfile);
void output_parsed(FILE *pfile);
void add_long_preprocessor_conditional_block_comment(void);


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
void print_options(FILE *pfile);

string argtype_to_string(argtype_e argtype);
string bool_to_string(bool val);
string argval_to_string(argval_t argval);
string number_to_string(int number);
string lineends_to_string(lineends_e linends);
string tokenpos_to_string(tokenpos_e tokenpos);
string op_val_to_string(argtype_e argtype, op_val_t op_val);

/*
 *  indent.cpp
 */

void indent_text(void);
void indent_preproc(void);
void indent_to_column(chunk_t *pc, int column);
void align_to_column(chunk_t *pc, int column);
bool ifdef_over_whole_file();
void reindent_line(chunk_t *pc, int column);
void quick_indent_again(void);


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
chunk_t *insert_comment_after(chunk_t *ref, c_token_t cmt_type, const unc_text& cmt_text);


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
int space_needed(chunk_t *first, chunk_t *second);
void space_add_after(chunk_t *pc, int count);


/*
 *  combine.cpp
 */

void fix_symbols(void);
void combine_labels(void);
void mark_comments(void);
void make_type(chunk_t *pc);

void flag_series(chunk_t *start, chunk_t *end, UINT64 set_flags, UINT64 clr_flags = 0, chunk_nav_t nav = CNAV_ALL);

chunk_t *skip_template_next(chunk_t *ang_open);
chunk_t *skip_template_prev(chunk_t *ang_close);

chunk_t *skip_tsquare_next(chunk_t *ary_def);

chunk_t *skip_attribute_next(chunk_t *attr);
chunk_t *skip_attribute_prev(chunk_t *fp_close);

void remove_extra_returns();
void remove_brace_only_has_return();


/*
 *  newlines.cpp
 */

void newlines_remove_newlines(void);
void newlines_cleanup_braces(bool first);
void newlines_insert_blank_lines(void);
void newlines_eat_extra_blank_lines(void);
void newlines_squeeze_ifdef(void);
void newlines_eat_start_end(void);
void newlines_chunk_pos(c_token_t chunk_type, tokenpos_e mode);
void newlines_class_colon_pos(c_token_t tok);
void newlines_cleanup_dup(void);
void annotations_newlines(void);
void newline_after_multiline_comment(void);
void newline_after_label_colon(void);
void do_blank_lines(void);

void newline_iarf(chunk_t *pc, argval_t av);

chunk_t *newline_add_before(chunk_t *pc);
chunk_t *newline_force_before(chunk_t *pc);
chunk_t *newline_add_after(chunk_t *pc);
chunk_t *newline_force_after(chunk_t *pc);
void newline_del_between(chunk_t *start, chunk_t *end);
chunk_t *newline_add_between(chunk_t *start, chunk_t *end);

/*
 *  tokenize.cpp
 */

void tokenize(const deque<int>& data, chunk_t *ref);


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
c_token_t find_keyword_type(const char *word, int len);
void add_keyword(const char *tag, c_token_t type);
void print_keywords(FILE *pfile);
void clear_keyword_file(void);
pattern_class get_token_pattern_class(c_token_t tok);
void keywords_are_sorted(void);


/*
 *  defines.cpp
 */

int load_define_file(const char *filename);
void add_define(const char *tag, const char *value);
void print_defines(FILE *pfile);
void clear_defines(void);


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
 * unicode.cpp
 */
void write_bom();
void write_char(int ch);
void write_string(const unc_text& text);
bool decode_unicode(const vector<UINT8>& in_data, deque<int>& out_data, CharEncoding& enc, bool& has_bom);
void encode_utf8(int ch, vector<UINT8>& res);


/*
 * semicolons.cpp
 */
void remove_extra_semicolons(void);

/*
 * compat_posix.cpp / compat_win32.cpp
 */
bool unc_getenv(const char *name, std::string& str);
bool unc_homedir(std::string& home);


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
   if (cpd.frag_cols > 0)
   {
      col += cpd.frag_cols - 1;
   }
   col = 1 + ((((col - 1) / tabsize) + 1) * tabsize);
   if (cpd.frag_cols > 0)
   {
      col -= cpd.frag_cols - 1;
   }
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
   if (col <= 0)
   {
      col = 1;
   }
   if ((col % cpd.settings[UO_output_tab_size].n) != 1)
   {
      col = next_tab_column(col);
   }
   return(col);
}

#endif /* C_PARSE_PROTOTYPES_H_INCLUDED */
