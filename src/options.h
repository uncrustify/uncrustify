/**
 * @file options.h
 * Enum and settings for all the options.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef OPTIONS_H_INCLUDED
#define OPTIONS_H_INCLUDED

#include <list>
#include <map>
#include <string>

enum argtype_e
{
   AT_BOOL,    /**< true / false */
   AT_IARF,    /**< Ignore / Add / Remove / Force */
   AT_NUM,     /**< Number */
   AT_LINE,    /**< Line Endings */
   AT_POS,     /**< start/end or Trail/Lead */
   AT_STRING,  /**< string value */
};

/** Arg values - these are bit fields*/
enum argval_t
{
   AV_IGNORE = 0,
   AV_ADD    = 1,
   AV_REMOVE = 2,
   AV_FORCE  = 3, /**< remove + add */
};

/** Line endings */
enum lineends_e
{
   LE_LF,      /* "\n"   */
   LE_CRLF,    /* "\r\n" */
   LE_CR,      /* "\r"   */

   LE_AUTO,    /* keep last */
};

/** Token position - these are bit fields */
enum tokenpos_e
{
   TP_IGNORE      = 0,     /* don't change it */
   TP_BREAK       = 1,     /* add a newline before or after the if not present */
   TP_FORCE       = 2,     /* force a newline on one side and not the other */
   TP_LEAD        = 4,     /* at the start of a line or leading if wrapped line */
   TP_LEAD_BREAK  = (TP_LEAD | TP_BREAK),
   TP_LEAD_FORCE  = (TP_LEAD | TP_FORCE),
   TP_TRAIL       = 8,     /* at the end of a line or trailing if wrapped line */
   TP_TRAIL_BREAK = (TP_TRAIL | TP_BREAK),
   TP_TRAIL_FORCE = (TP_TRAIL | TP_FORCE),
   TP_JOIN        = 16,    /* remove newlines on both sides */
};

union op_val_t
{
   argval_t   a;
   int        n;
   bool       b;
   lineends_e le;
   tokenpos_e tp;
   const char *str;
};

/** Groups for options */
enum uncrustify_groups
{
   UG_general,
   UG_indent,
   UG_space,
   UG_align,
   UG_newline,
   UG_position,
   UG_linesplit,
   UG_blankline,
   UG_codemodify,
   UG_comment,
   UG_preprocessor,
   UG_group_count
};

/**
 * Keep this grouped by functionality
 */
enum uncrustify_options
{
   UO_newlines,                 // Set to AUTO, LF, CRLF, or CR

   /*
    * Basic Indenting stuff
    */
   //UO_indent,                   //TODO: 0=don't change indentation, 1=change indentation
   UO_tok_split_gte,             // allow split of '>>=' in template detection

   UO_utf8_byte,
   UO_utf8_force,
   UO_utf8_bom,

   UO_dont_protect_xcode_code_placeholders,

   UO_input_tab_size,           // tab size on input file: usually 8
   UO_output_tab_size,          // tab size for output: usually 8

   UO_indent_columns,           // ie 3 or 8
   UO_indent_continue,
   UO_indent_with_tabs,         // 1=only to the 'level' indent, 2=use tabs for indenting
   UO_indent_cmt_with_tabs,
   //UO_indent_brace_struct,      //TODO: spaces to indent brace after struct/enum/union def
   //UO_indent_paren,             //TODO: indent for open paren on next line (1)
   UO_indent_paren_nl,           // indent-align under paren for open followed by nl
   UO_indent_square_nl,          // indent-align under square for open followed by nl
   UO_indent_paren_close,        // indent of close paren after a newline
   UO_indent_comma_paren,        // indent of comma if inside a paren
   UO_indent_bool_paren,         // indent of bool if inside a paren
   UO_indent_first_bool_expr,    // if UO_indent_bool_paren == true, aligns the first expression to the following ones
   UO_pp_indent,                 // indent preproc 1 space per level (add/ignore/remove)
   UO_pp_indent_at_level,        // indent #if, #else, #endif at brace level
   UO_pp_indent_count,
   UO_pp_define_at_level,        // indent #define at brace level
   UO_pp_space,                  // spaces between # and word (add/ignore/remove)
   UO_pp_space_count,            // the number of spaces for add/force
   UO_pp_indent_region,          // indent of #region and #endregion, see indent_label
   UO_pp_region_indent_code,     // whether to indent the code inside region stuff
   UO_pp_indent_if,
   UO_pp_if_indent_code,

   UO_indent_switch_case,         // spaces to indent case from switch
   UO_indent_case_shift,          // spaces to shift the line with the 'case'
   UO_indent_case_brace,          // spaces to indent '{' from case (usually 0 or indent_columns)

   UO_indent_brace,               // spaces to indent '{' from level (usually 0)
   UO_indent_braces,              // whether to indent the braces or not
   UO_indent_braces_no_func,      // whether to not indent the function braces (depends on UO_indent_braces)
   UO_indent_braces_no_class,     // whether to not indent the class braces (depends on UO_indent_braces)
   UO_indent_braces_no_struct,    // whether to not indent the struct braces (depends on UO_indent_braces)
   UO_indent_brace_parent,        // indent the braces based on the parent size (if=3, for=4, etc)
   UO_indent_paren_open_brace,    // indent on paren level in '({', default by {
   UO_indent_label,               // 0=left >0=col from left, <0=sub from brace indent
   UO_indent_access_spec,         // same as indent_label, but for "private:", "public:"
   UO_indent_access_spec_body,    // indent private/public/protected inside a class (overrides indent_access_spec)

   UO_indent_align_string,        // True/False - indent align broken strings
   UO_indent_xml_string,          // Number amount to indent XML strings

   UO_indent_col1_comment,        // indent comments in column 1

   UO_indent_func_def_force_col1, // force indentation of function definition to start in column 1
   UO_indent_func_call_param,     // indent continued function calls to indent_columns
   UO_indent_func_proto_param,    // same, but for function protos
   UO_indent_func_def_param,      // same, but for function defs
   UO_indent_func_class_param,    // same, but for classes
   UO_indent_func_ctor_var_param,
   UO_indent_template_param,
   UO_indent_func_param_double,             // double the tab indent for

   UO_indent_func_const,                    // indentation for standalone "const" qualifier
   UO_indent_func_throw,                    // indentation for standalone "throw" qualifier

   UO_indent_namespace,                     // indent stuff inside namespace braces
   UO_indent_namespace_single_indent,       // indent one namespace and no sub-namespaces
   UO_indent_namespace_level,               // level to indent namespace blocks
   UO_indent_namespace_limit,               // no indent if namespace is longer than this
   UO_indent_extern,
   UO_indent_class,                         // indent stuff inside class braces
   UO_indent_class_colon,                   // indent stuff after a class colon
   UO_indent_class_on_colon,                // indent stuff on a class colon
   UO_indent_constr_colon,                  // indent stuff after a constr colon

   UO_indent_ctor_init_leading,             // virtual indent from the ':' for member initializers. Default is 2. (applies to the leading colon case)
   UO_indent_ctor_init,                     // additional indenting for ctor initializer lists

   UO_indent_member,                        // indent lines broken at a member '.' or '->'

   UO_indent_sing_line_comments,            // indent single line ('//') comments on lines before code
   UO_indent_relative_single_line_comments, // indent single line ('//') comments after code
   UO_indent_preserve_sql,                  // preserve indent of EXEC SQL statement body
   UO_indent_align_assign,
   UO_indent_oc_block,
   UO_indent_oc_block_msg,
   UO_indent_oc_msg_prioritize_first_colon,
   UO_indent_oc_msg_colon,
   UO_indent_oc_block_msg_xcode_style,
   UO_indent_oc_block_msg_from_brace,
   UO_indent_oc_block_msg_from_caret,
   UO_indent_oc_block_msg_from_colon,
   UO_indent_oc_block_msg_from_keyword,

   UO_indent_else_if,
   UO_indent_var_def_blk,        // indent a variable def block that appears at the top
   UO_indent_var_def_cont,
   UO_indent_shift,              // if a shift expression spans multiple lines, indent

   UO_indent_min_vbrace_open,               // min. indent after virtual brace open and newline
   UO_indent_vbrace_open_on_tabstop,        // when identing after virtual brace open and newline add further spaces to reach next tabstop

   /*
    * Misc inter-element spacing
    */

   UO_sp_paren_brace,           // space between ')' and '{'
   UO_sp_fparen_brace,          // space between ')' and '{' of function
   UO_sp_fparen_dbrace,         // space between ')' and '{{' of double-brace init
   UO_sp_sparen_brace,          // space between ')' and '{' of if, while, etc

   UO_sp_after_cast,            // space after C & D cast - "(int) a" vs "(int)a"
   UO_sp_inside_paren_cast,     // spaces inside the parens of a cast
   UO_sp_cpp_cast_paren,

   UO_sp_before_byref,          // space before '&' of 'fcn(int& idx)'
   UO_sp_before_unnamed_byref,
   UO_sp_after_byref,           // space after a '&'  as in 'int& var'

   UO_sp_after_type,            // space between type and word
   UO_sp_before_template_paren, // D: "template Foo("

   UO_sp_inside_fparen,         // space inside 'foo( xxx )' vs 'foo(xxx)'
   UO_sp_inside_fparens,        // space inside 'foo( )' vs 'foo()'
   UO_sp_inside_tparen,
   UO_sp_after_tparen_close,
   UO_sp_inside_paren,          // space inside '+ ( xxx )' vs '+ (xxx)'
   UO_sp_inside_square,         // space inside 'byte[ 5 ]' vs 'byte[5]'
   UO_sp_inside_sparen,         // space inside 'if( xxx )' vs 'if(xxx)'
   UO_sp_inside_sparen_close,
   UO_sp_inside_sparen_open,
   UO_sp_inside_angle,          // space inside '<>', as in '<class T>'

   UO_sp_before_sparen,         // space before '(' of 'if/for/while/switch'
   UO_sp_after_sparen,          /* space after  ')' of 'if/for/while/switch'
                                 * the do-while does not get set here */
   UO_sp_after_invariant_paren,
   UO_sp_invariant_paren,

   UO_sp_template_angle,
   UO_sp_before_angle,          // space before '<>', as in '<class T>'
   UO_sp_after_angle,           // space after  '<>', as in '<class T>'
   UO_sp_angle_paren,           // space between '<>' and '(' in "a = new List<byte>();"
   UO_sp_angle_word,            // space between '<>' and a word in "List<byte> a;"
   UO_sp_angle_shift,           // '> >' vs '>>'
   UO_sp_permit_cpp11_shift,    // '>>' vs '> >' for C++11 code

   UO_sp_before_square,         // space before single '['
   UO_sp_before_squares,        // space before '[]', as in 'byte []'

   UO_sp_paren_paren,           // space between nested parens - '( (' vs '(('
   UO_sp_cparen_oparen,         // space between nested parens - ') (' vs ')('
   UO_sp_balance_nested_parens, // balance spaces inside nested parens

   UO_sp_return_paren,          // space between 'return' and '('
   UO_sp_sizeof_paren,          // space between 'sizeof' and '('

   UO_sp_after_comma,           // space after ','
   UO_sp_before_comma,          // space before ','
   UO_sp_paren_comma,

   UO_sp_before_mdatype_commas,
   UO_sp_between_mdatype_commas,
   UO_sp_after_mdatype_commas,

   UO_sp_before_ellipsis,       // space before '...'

   UO_sp_arith,                 // space around + - / * etc
   UO_sp_bool,                  // space around || &&
   UO_sp_pp_concat,             // space around ##
   UO_sp_pp_stringify,          // space after #
   UO_sp_before_pp_stringify,   // space before # in a #define x(y) L#y
   UO_sp_compare,               // space around < > ==, etc
   UO_sp_assign,                // space around =, +=, etc
   UO_sp_cpp_lambda_assign,     // space around the capture spec [=](...){...}
   UO_sp_cpp_lambda_paren,      // space after the capture spec [] (...){...}
   UO_sp_assign_default,        // space around '=' in prototype
   UO_sp_before_assign,         // space before =, +=, etc
   UO_sp_after_assign,          // space after =, +=, etc
   UO_sp_enum_paren,
   UO_sp_enum_assign,           // space around = in enum
   UO_sp_enum_before_assign,    // space before = in enum
   UO_sp_enum_after_assign,     // space after = in enum
   UO_sp_after_class_colon,     // space after class ':'
   UO_sp_before_class_colon,    // space before class ':'
   UO_sp_after_constr_colon,
   UO_sp_before_constr_colon,
   UO_sp_before_case_colon,

   UO_sp_func_def_paren,        // space between 'func' and '(' - "foo (" vs "foo("
   UO_sp_func_call_paren,       // space between 'func' and '(' - "foo (" vs "foo("
   UO_sp_func_call_paren_empty,
   UO_sp_func_call_user_paren,
   UO_sp_func_proto_paren,      // space between 'func' and '(' - "foo (" vs "foo("
   UO_sp_func_class_paren,      // space between ctor/dtor and '('

   UO_sp_attribute_paren,       // space between '__attribute__' and '('
   UO_sp_defined_paren,
   UO_sp_throw_paren,
   UO_sp_after_throw,
   UO_sp_catch_paren,
   UO_sp_version_paren,
   UO_sp_scope_paren,

   UO_sp_type_func,                // space between return type and 'func'
   // a minimum of 1 is forced except for '*'
   UO_sp_before_ptr_star,          // space before a '*' that is part of a type
   UO_sp_before_unnamed_ptr_star,
   UO_sp_after_ptr_star,           // space after a '*' that is part of a type
   UO_sp_after_ptr_star_qualifier, // space after a '*' next to a qualifier
   UO_sp_after_ptr_star_func,      // space between a '*' and a function proto/def
   UO_sp_ptr_star_paren,
   UO_sp_before_ptr_star_func,
   UO_sp_after_byref_func,
   UO_sp_before_byref_func,
   UO_sp_between_ptr_star,      // space between two '*' that are part of a type

   UO_sp_special_semi,          /* space empty stmt ';' on while, if, for
                                 * example "while (*p++ = ' ') ;" */
   UO_sp_before_semi,           // space before all ';'
   UO_sp_before_semi_for,       // space before the two ';' in a for() - non-empty
   UO_sp_before_semi_for_empty, // space before ';' in empty for statement
   UO_sp_after_semi,
   UO_sp_after_semi_for,
   UO_sp_after_semi_for_empty,  // space after final ';' in empty for statement
   UO_sp_inside_braces,         // space inside '{' and '}' - "{ 1, 2, 3 }"
   UO_sp_inside_braces_empty,   // space inside '{' and '}' - "{ }"
   UO_sp_inside_braces_enum,    // space inside enum '{' and '}' - "{ a, b, c }"
   UO_sp_inside_braces_struct,  // space inside struct/union '{' and '}'

   UO_sp_macro,                 // space between macro and value, ie '#define a 6'
   UO_sp_macro_func,            // space between macro and value, ie '#define a 6'

   UO_sp_square_fparen,         // weird pawn stuff: native yark[rect](a[rect])
   UO_sp_after_tag,             // pawn: space after a tag colon

   UO_sp_after_operator,        // space after operator when followed by a punctuator
   UO_sp_after_operator_sym,    // space after operator when followed by a punctuator
   UO_sp_else_brace,
   UO_sp_brace_else,
   UO_sp_brace_typedef,

   UO_sp_catch_brace,
   UO_sp_brace_catch,
   UO_sp_finally_brace,
   UO_sp_brace_finally,
   UO_sp_try_brace,
   UO_sp_getset_brace,

   UO_sp_word_brace,
   UO_sp_word_brace_ns,

   UO_sp_before_dc,
   UO_sp_after_dc,
   UO_sp_d_array_colon,
   UO_sp_not,
   UO_sp_inv,
   UO_sp_addr,
   UO_sp_deref,
   UO_sp_member,
   UO_sp_sign,
   UO_sp_incdec,
   UO_sp_before_nl_cont,
   UO_sp_after_oc_scope,
   UO_sp_before_oc_colon,
   UO_sp_after_oc_colon,
   UO_sp_before_oc_dict_colon,
   UO_sp_after_oc_dict_colon,
   UO_sp_before_send_oc_colon,
   UO_sp_after_send_oc_colon,
   UO_sp_after_oc_type,
   UO_sp_after_oc_return_type,
   UO_sp_after_oc_at_sel,
   UO_sp_after_oc_at_sel_parens,
   UO_sp_inside_oc_at_sel_parens,
   UO_sp_before_oc_block_caret,
   UO_sp_after_oc_block_caret,
   UO_sp_after_oc_msg_receiver,
   UO_sp_after_oc_property,
   UO_sp_cond_colon,
   UO_sp_cond_colon_before,
   UO_sp_cond_colon_after,
   UO_sp_cond_question,
   UO_sp_cond_question_before,
   UO_sp_cond_question_after,
   UO_sp_cond_ternary_short,
   UO_sp_case_label,
   UO_sp_range,
   UO_sp_cmt_cpp_start,
   UO_sp_cmt_cpp_doxygen,       // in case of UO_sp_cmt_cpp_start: treat '///', '///<', '//!' and '//!<' as a unity (add space behind)
   UO_sp_endif_cmt,
   UO_sp_after_new,
   UO_sp_between_new_paren,
   UO_sp_before_tr_emb_cmt,     // treatment of spaces before comments following code
   UO_sp_num_before_tr_emb_cmt, // number of spaces before comments following code
   UO_sp_annotation_paren,
   UO_sp_after_for_colon,
   UO_sp_before_for_colon,
   UO_sp_extern_paren,

   /*
    * Line splitting options (for long lines)
    */

   UO_code_width,           // ie 80 columns
   UO_ls_for_split_full,    // try to split long 'for' statements at semi-colons
   UO_ls_func_split_full,   // try to split long func proto/def at comma
   UO_ls_code_width,        // try to split at code_width
   //UO_ls_before_bool_op,    //TODO: break line before or after boolean op
   //UO_ls_before_paren,      //TODO: break before open paren
   //UO_ls_after_arith,       //TODO: break after arith op '+', etc
   //UO_ls_honor_newlines,    //TODO: don't remove newlines on split lines


   /*
    * code alignment (not left column spaces/tabs)
    */

   UO_align_with_tabs,            // use tabs for aligning (0/1)
   UO_align_keep_tabs,            // keep non-indenting tabs
   UO_align_on_tabstop,           // always align on tabstops
   UO_align_nl_cont,              // align the back-slash \n combo (macros)
   UO_align_enum_equ_span,        // align the '=' in enums
   UO_align_enum_equ_thresh,      // threshold for aligning on '=' in enums. 0=no limit
   UO_align_assign_span,          // align on '='. 0=don't align
   UO_align_assign_thresh,        // threshold for aligning on '='. 0=no limit
   UO_align_right_cmt_span,       // align comment that end lines. 0=don't align
   UO_align_right_cmt_mix,        // mix comments after '}' and preproc with others
   UO_align_right_cmt_gap,
   UO_align_right_cmt_at_col,     // align comment that end lines at or beyond column N; 'pulls in' comments as a bonus side effect
   UO_align_func_params,          // align prototype variable defs on variable
   UO_align_same_func_call_params,
   UO_align_var_def_span,         // align variable defs on variable (span for regular stuff)
   UO_align_var_def_thresh,       // align variable defs threshold
   UO_align_var_def_gap,          // align variable defs gap
   UO_align_var_def_colon_gap,    // align variable defs gap for bit colons
   UO_align_var_def_inline,       // also align inline struct/enum/union var defs
   UO_align_var_def_star_style,   // see UO_align_typedef_star_style
   UO_align_var_def_amp_style,    // see UO_align_typedef_star_style
   UO_align_var_def_colon,        // align the colon in struct bit fields
   UO_align_var_def_attribute,
   UO_align_var_struct_span,      // span for struct/union (0=don't align)
   UO_align_var_struct_thresh,    // threshold for struct/union, 0=no limit
   UO_align_var_struct_gap,       // gap for struct/union
   UO_align_pp_define_together,   // align macro functions and variables together
   UO_align_pp_define_span,       // align bodies in #define statements
   //UO_align_pp_define_col_min,    //TODO: min column for a #define value
   //UO_align_pp_define_col_max,    //TODO: max column for a #define value
   UO_align_pp_define_gap,        // min space between define label and value "#define a <---> 16"
   //UO_align_enum_col_min,         //TODO: the min column for enum '=' alignment
   //UO_align_enum_col_max,         //TODO: the max column for enum '=' alignment
   UO_align_struct_init_span,      // align structure initializer values
   UO_align_func_proto_span,       // align function prototypes
   UO_align_func_proto_gap,        // align function prototypes
   UO_align_on_operator,
   UO_align_mix_var_proto,         // mix function prototypes and variable decl
   UO_align_single_line_func,      // mix single line function with prototypes
   UO_align_single_line_brace,     // align the open brace of single line functions
   UO_align_single_line_brace_gap, // gap for align_single_line_brace
   UO_align_oc_msg_spec_span,      // align ObjC msg spec
   UO_align_number_left,           // left-align numbers (not fully supported, yet)
   UO_align_typedef_span,          // align single-line typedefs
   UO_align_typedef_gap,           // minimum spacing
   UO_align_typedef_func,          // how to align func type with types
   UO_align_typedef_star_style,    // Start aligning style
                                   // 0: '*' not part of type
                                   // 1: '*' part of the type - no space
                                   // 2: '*' part of type, dangling
   UO_align_typedef_amp_style,     // align_typedef_star_style for ref '&' stuff
   //UO_align_struct_array_brace,  // TODO: align array of structure initializers
   UO_align_left_shift,
//    UO_align_oc_msg_colon,
   UO_align_oc_msg_colon_span,
   UO_align_oc_msg_colon_first,
   UO_align_oc_decl_colon,
   UO_align_keep_extra_space,      // don't squash extra whitespace

   /*
    * Newline adding and removing options
    */

   UO_nl_fdef_brace,                 // "int foo() {" vs "int foo()\n{"
   UO_nl_cpp_ldef_brace,             // "[&x](int a) {" vs "[&x](int a)\n{"
   UO_nl_func_paren,                 // newline between function and open paren
   UO_nl_func_def_paren,
   UO_nl_func_decl_start,            // newline after the '(' in a function decl
   UO_nl_func_def_start,             // newline after the '(' in a function def
   UO_nl_func_decl_start_single,
   UO_nl_func_def_start_single,
   UO_nl_func_decl_args,             // newline after each ',' in a function decl
   UO_nl_func_def_args,
   UO_nl_func_decl_end,              // newline before the ')' in a function decl
   UO_nl_func_def_end,               // newline before the ')' in a function decl
   UO_nl_func_decl_end_single,
   UO_nl_func_def_end_single,
   UO_nl_func_decl_empty,            // as above, but for empty parens '()'
   UO_nl_func_def_empty,             // as above, but for empty parens '()'
   UO_nl_func_type_name,             // newline between return type and func name in def
   UO_nl_func_type_name_class,       // newline between return type and func name in class
   UO_nl_func_scope_name,
   UO_nl_func_proto_type_name,       // nl_func_type_name, but for prottypes
   UO_nl_func_var_def_blk,           // newline after first block of func variable defs
   UO_nl_typedef_blk_start,          // newline before typedef block
   UO_nl_typedef_blk_end,            // newline after typedef block
   UO_nl_typedef_blk_in,             // newline max within typedef block
   UO_nl_var_def_blk_start,          // newline before variable defs block
   UO_nl_var_def_blk_end,            // newline after variable defs block
   UO_nl_var_def_blk_in,             // newline max within variable defs block
   UO_nl_before_case,                // newline before 'case' statement
   UO_nl_before_throw,
   UO_nl_before_return,
   UO_nl_after_return,               /* newline after return statement */
   UO_nl_return_expr,
   UO_nl_after_annotation,
   UO_nl_between_annotation,
   UO_nl_after_case,                 /* disallow nested "case 1: a=3;" */
   UO_nl_after_semicolon,            // disallow multiple statements on a line "a=1;b=4;"
   UO_nl_paren_dbrace_open,
   UO_nl_after_brace_open,           // force a newline after a brace open
   UO_nl_after_brace_open_cmt,       // put the newline before the comment
   UO_nl_after_vbrace_open,          // force a newline after a virtual brace open
   UO_nl_after_vbrace_open_empty,    // force a newline after a virtual brace open
   UO_nl_after_brace_close,          // force a newline after a brace close
   UO_nl_after_vbrace_close,         // force a newline after a virtual brace close
   UO_nl_brace_struct_var,           // force a newline after a brace close
   UO_nl_fcall_brace,                /* newline between function call and open brace */
   UO_nl_squeeze_ifdef,              /* no blanks after #ifxx, #elxx, or before #endif */
   UO_nl_enum_brace,                 /* nl between enum and brace */
   UO_nl_struct_brace,               /* nl between struct and brace */
   UO_nl_union_brace,                /* nl between union and brace */
   UO_nl_assign_brace,               /* nl between '=' and brace */
   UO_nl_assign_square,              /* nl between '=' and '[' */
   UO_nl_after_square_assign,        /* nl after '= [' */
   UO_nl_class_brace,                /* nl between class name and brace */
   UO_nl_namespace_brace,            /* nl between namespace name and brace */

   UO_nl_brace_brace,                /* nl between '{{' or '}}' */
   UO_nl_do_brace,                   /* nl between do and { */
   UO_nl_if_brace,                   /* nl between if and { */
   UO_nl_for_brace,                  /* nl between for and { */
   UO_nl_else_if,
   UO_nl_else_brace,                 /* nl between else and { */
   UO_nl_finally_brace,              /* nl between finally and { */
   UO_nl_brace_finally,              /* nl between } and finally */
   UO_nl_try_brace,                  /* nl between try and { */
   UO_nl_getset_brace,               /* nl between get/set and { */
   UO_nl_catch_brace,                /* nl between catch and { */
   UO_nl_brace_catch,                /* nl between } and catch */
   UO_nl_brace_square,               /* nl between } and ] */
   UO_nl_brace_fparen,               /* nl between } and ) of a function invocation */
   UO_nl_while_brace,                /* nl between while and { */
   UO_nl_unittest_brace,             /* nl between unittest and { */
   UO_nl_scope_brace,
   UO_nl_version_brace,
   UO_nl_using_brace,
   UO_nl_switch_brace,                /* nl between switch and { */
   UO_nl_brace_else,                  // nl between } and else
   UO_nl_brace_while,                 // nl between } and while of do stmt

   UO_nl_multi_line_cond,             /* nl between ) and { when cond spans >=2 lines */
   UO_nl_elseif_brace,                // nl between close paren and open brace in 'else if () {'

   UO_nl_multi_line_define,           // nl after define XXX for multi-line define

   UO_nl_before_if,                   // nl before if
   UO_nl_after_if,                    // nl after if/else
   UO_nl_before_for,                  // nl before for
   UO_nl_after_for,                   // nl after for close
   UO_nl_before_while,                // nl before while
   UO_nl_after_while,                 // nl after while close
   UO_nl_before_switch,               // nl before switch
   UO_nl_after_switch,                // nl after switch close
   UO_nl_before_do,                   // nl before do
   UO_nl_after_do,                    // nl after while of do
   UO_nl_ds_struct_enum_cmt,          // nl between commented-elements of struct/enum
   UO_nl_ds_struct_enum_close_brace,  // force nl before } of struct/union/enum

   UO_nl_define_macro,                // alter newlines in #define macros
   UO_nl_start_of_file,               // alter newlines at the start of file
   UO_nl_start_of_file_min,           // min number of newlines at the start of the file
   UO_nl_end_of_file,                 // alter newlines at the end of file
   UO_nl_end_of_file_min,             // min number of newlines at the end of the file

   UO_nl_class_colon,                 // nl before/after class colon (tied to UO_pos_class_colon)
   UO_nl_constr_colon,                // nl before/after class constr colon (tied to UO_pos_constr_colon)
   UO_nl_class_init_args,             // newline after comma in base class list
   UO_nl_constr_init_args,            // newline after comma in class init args
   UO_nl_collapse_empty_body,         // change { \n } into {}
   UO_nl_class_leave_one_liners,      // leave one-line function bodies in "class xx { here }"
   UO_nl_assign_leave_one_liners,     // leave one-line assign bodies in "foo_t f = { a, b, c };"
   UO_nl_enum_leave_one_liners,       // leave one-line enum bodies in "enum FOO { BAR = 5 };"
   UO_nl_getset_leave_one_liners,     // leave one-line get/set bodies
   UO_nl_func_leave_one_liners,       // leave one-line function def bodies
   UO_nl_cpp_lambda_leave_one_liners, // leave one-line C++11 lambda bodies
   UO_nl_if_leave_one_liners,
   UO_nl_case_colon_brace,

   UO_nl_template_class,          // newline between '>' and class in "template <x> class"

   UO_nl_create_if_one_liner,
   UO_nl_create_for_one_liner,
   UO_nl_create_while_one_liner,

   UO_nl_oc_msg_args,
   UO_nl_oc_msg_leave_one_liner,

   UO_pos_arith,                  // position of trailing/leading arithmetic ops
   UO_pos_assign,                 // position of trailing/leading =
   UO_pos_bool,                   // position of trailing/leading &&/||
   UO_pos_compare,                // position of trailing/leading <=/>, etc
   UO_pos_conditional,            // position of trailing/leading (b ? t : f)
   UO_pos_comma,                  // position of comma in functions
   UO_pos_class_comma,            // position of comma in class parent list list
   UO_pos_constr_comma,           // position of comma in constructor init list
   UO_pos_class_colon,            // position of trailing/leading class colon
   UO_pos_constr_colon,           // position of trailing/leading class constr colon


   /*
    * Blank line options
    */

   UO_nl_before_block_comment,       // before a block comment (stand-alone comment-multi), except after brace open
   UO_nl_before_cpp_comment,
   UO_nl_before_c_comment,
   UO_nl_after_multiline_comment,    // NL after multiline comment
   UO_nl_after_label_colon,          // NL after a label followed by a colon
   UO_nl_after_func_body,            // after the closing brace of a function body
   UO_nl_after_func_body_class,
   UO_nl_after_func_body_one_liner,  // after the closing brace of a single line function body
   UO_nl_after_func_proto,           // after each prototype
   UO_nl_after_func_proto_group,     // after a block of prototypes
   //UO_nl_after_ifdef,                // after #if or #ifdef - but not if covers whole file
   UO_nl_after_struct,
   UO_nl_after_class,
   UO_nl_max,                        // maximum consecutive newlines (3 = 2 blank lines)
   UO_nl_max_blank_in_func_minus_one, // maximum consecutive newlines in function(1 = 0 blank lines)
   UO_nl_before_access_spec,         // number of newlines before "private:", "public:" (0=no change)
   UO_nl_after_access_spec,          // number of newlines after "private:", "public:" (0=no change)
   UO_nl_comment_func_def,
   UO_nl_after_try_catch_finally,
   UO_nl_between_get_set,
   UO_nl_around_cs_property,
   UO_nl_property_brace,

   UO_eat_blanks_after_open_brace,   // remove blank lines after {
   UO_eat_blanks_before_close_brace, // remove blank lines before }
   UO_nl_remove_extra_newlines,      // remove extra nl aggressiveness


   /*
    * code modifying options (non-whitespace)
    */

   UO_mod_paren_on_return,        // add or remove paren on return
   UO_mod_full_brace_nl,          // max number of newlines to span w/o braces
   UO_mod_full_brace_if,          // add or remove braces on if
   UO_mod_full_brace_if_chain,
   UO_mod_full_brace_for,         // add or remove braces on for
   UO_mod_full_brace_do,          // add or remove braces on do
   UO_mod_full_brace_while,       // add or remove braces on while
   UO_mod_full_brace_using,       // add or remove braces on using
   UO_mod_pawn_semicolon,         // add optional semicolons
   UO_mod_full_brace_function,    // add optional braces on Pawn functions
   UO_mod_full_paren_if_bool,
   UO_mod_remove_extra_semicolon, // remove extra semicolons
   UO_mod_add_long_function_closebrace_comment,
   UO_mod_add_long_switch_closebrace_comment,
   UO_mod_add_long_ifdef_else_comment,
   UO_mod_add_long_ifdef_endif_comment,
   UO_mod_sort_import,
   UO_mod_sort_using,
   UO_mod_sort_include,
   UO_mod_move_case_break,
   UO_mod_case_brace,
   UO_mod_remove_empty_return,
   UO_mod_remove_brace_only_has_return,


   /*
    * Comment modifications
    */

   UO_cmt_width,                // column to wrap comments
   UO_cmt_reflow_mode,          // comment reflow style
   UO_cmt_indent_multi,         // change left indent of multiline comments
   UO_cmt_star_cont,            // put a star on subsequent comment lines
   UO_cmt_sp_before_star_cont,  // # of spaces for subsequent comment lines (before possible star)
   UO_cmt_sp_after_star_cont,   // # of spaces for subsequent comment lines (after star)
   UO_cmt_cpp_to_c,             // convert CPP comments to C comments
   UO_cmt_cpp_group,            // if UO_cmt_cpp_to_c, try to group in one big C comment
   UO_cmt_c_group,              // try to group neighboring C comments
   UO_cmt_c_nl_start,           // put a blank /* at the start of a combined group
   UO_cmt_c_nl_end,             // put a nl before the */ in a combined group
   UO_cmt_cpp_nl_start,         // put a blank /* at the start of a converted group
   UO_cmt_cpp_nl_end,           // put a nl before the */ in a converted group
   UO_cmt_multi_check_last,     // no space after '*' prefix when comment start and end are of equal length
   UO_cmt_convert_tab_to_spaces,

   UO_cmt_insert_file_header,
   UO_cmt_insert_file_footer,
   UO_cmt_insert_func_header,
   UO_cmt_insert_class_header,
   UO_cmt_insert_oc_msg_header,
   UO_cmt_insert_before_preproc,

   UO_string_escape_char,       // the string escape char to use
   UO_string_escape_char2,      // the string escape char to use
   UO_string_replace_tab_chars, // replace tab chars found in strings to the escape sequence \t
   UO_disable_processing_cmt,   // override UNCRUSTIFY_DEFAULT_OFF_TEXT
   UO_enable_processing_cmt,    // override UNCRUSTIFY_DEFAULT_ON_TEXT

   /* Hack, add comments to the ends of namespaces */
   UO_mod_add_long_namespace_closebrace_comment,

   /* This is used to get the enumeration count */
   UO_option_count
};

struct group_map_value
{
   uncrustify_groups        id;
   const char               *short_desc;
   const char               *long_desc;
   list<uncrustify_options> options;
};

struct option_map_value
{
   uncrustify_options id;
   uncrustify_groups  group_id;
   argtype_e          type;
   int                min_val;
   int                max_val;
   const char         *name;
   const char         *short_desc;
   const char         *long_desc;
};


typedef map<string, option_map_value>::iterator             option_name_map_it;
typedef map<uncrustify_groups, group_map_value>::iterator   group_map_it;
typedef list<uncrustify_options>::iterator                  option_list_it;
typedef list<uncrustify_options>::const_iterator            option_list_cit;

#endif /* OPTIONS_H_INCLUDED */
