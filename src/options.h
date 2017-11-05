/**
 * @file options.h
 * Enum and settings for all the options.
 *
 * @author  Ben Gardner
 * @author  Guy Maurel since version 0.62 for uncrustify4Qt
 *          October 2015, 2016
 * @license GPL v2+
 */
#ifndef OPTIONS_H_INCLUDED
#define OPTIONS_H_INCLUDED

#ifdef EMSCRIPTEN
#include <vector>
#endif
#include <list>
#include <map>
#include <string>

/**
 * abbreviations used
 *
 * av     = argument values
 * bal    = balance, balanced
 * pstart = pointer star
 * rel    = relative
 */


enum argtype_e
{
   AT_BOOL,    //! true / false
   AT_IARF,    //! Ignore / Add / Remove / Force
   AT_NUM,     //! Number
   AT_LINE,    //! Line Endings
   AT_POS,     //! start/end or Trail/Lead
   AT_STRING,  //! string value
   AT_UNUM,    //! unsigned Number
   AT_TFI,     //! false / true / ignore
};

//! Arg values - these are bit fields
enum argval_t
{
   AV_IGNORE      = 0,                    //! option ignores a given feature
   AV_ADD         = (1u << 0),            //! option adds a given feature
   AV_REMOVE      = (1u << 1),            //! option removes a given feature
   AV_FORCE       = (AV_ADD | AV_REMOVE), //! option forces the usage of a given feature
   AV_NOT_DEFINED = (1u << 2)             //! to be used with QT, SIGNAL SLOT macros
};

//! Line endings
enum lineends_e
{
   LE_LF,      //! "\n"   typically used on Unix/Linux system
   LE_CRLF,    //! "\r\n" typically used on Windows systems
   LE_CR,      //! "\r"   carriage return without newline
   LE_AUTO     //! keep last
};

//! Token position - these are bit fields
enum tokenpos_e
{
   TP_IGNORE      = 0,                        //! don't change it
   TP_BREAK       = 1,                        //! add a newline before or after the if not present
   TP_FORCE       = 2,                        //! force a newline on one side and not the other
   TP_LEAD        = 4,                        //! at the start of a line or leading if wrapped line
   TP_LEAD_BREAK  = (TP_LEAD | TP_BREAK),
   TP_LEAD_FORCE  = (TP_LEAD | TP_FORCE),
   TP_TRAIL       = 8,                        //! at the end of a line or trailing if wrapped line
   TP_TRAIL_BREAK = (TP_TRAIL | TP_BREAK),
   TP_TRAIL_FORCE = (TP_TRAIL | TP_FORCE),
   TP_JOIN        = 16,                       //! remove newlines on both sides
};

//! True, False or Ignore
enum TrueFalseIgnore_e
{
   TFI_FALSE  = false,                    //! false
   TFI_TRUE   = true,                     //! true
   TFI_IGNORE = 2,                        //! ignore
};

/**
 * Uncrustify options are configured with a parameter of this type.
 * Depending on the option the meaning (and thus type) of the
 * parameter varies. Therefore we use a union that provides all
 * possible types.
 */
union op_val_t
{
   argval_t          a;    //! ignore / add / remove / force
   int               n;    //! a signed number
   bool              b;    //! a bool flag
   lineends_e        le;   //! line ending type
   tokenpos_e        tp;   //! token position type
   const char        *str; //! a string
   size_t            u;    //! an unsigned number
   TrueFalseIgnore_e tfi;  //! false / true / ignore
};

/**
 * list of all group identifiers that are used to group uncrustify options
 * The order here must be the same as in the file options.cpp
 */
enum uncrustify_groups
{
   UG_general,        //! group for options that do not fit into other groups
   UG_space,          //! group for options that modify spaces
   UG_indent,         //! group for options that handle indentation
   UG_newline,        //! group for options that modify newlines
   UG_blankline,      //! group for options that modify blank lines
   UG_position,       //! group for options that modify positions
   UG_linesplit,      //! group for options that split lines
   UG_align,          //! group for alignment options
   UG_comment,        //! group for comment related options
   UG_codemodify,     //! group for options that modify the code
   UG_preprocessor,   //! group for all preprocessor related options
   UG_sort_includes,  //! group for all sorting options
   UG_Use_Ext,
   UG_warnlevels,
   UG_group_count
};

//! lists all options that uncrustify has
enum uncrustify_options
{
   // Keep this grouped by functionality

   // group: UG_general, "General options"                                         0
   UO_newlines,                 // Set to AUTO, LF, CRLF, or CR

   /*
    * Basic Indenting stuff
    */
   UO_input_tab_size,           // tab size on input file: usually 8
   UO_output_tab_size,          // tab size for output: usually 8
   UO_string_escape_char,       // the string escape char to use
   UO_string_escape_char2,      // the string escape char to use
   UO_string_replace_tab_chars, // replace tab chars found in strings to the escape sequence \t
   UO_tok_split_gte,            // allow split of '>>=' in template detection
   UO_disable_processing_cmt,   // override UNCRUSTIFY_DEFAULT_OFF_TEXT
   UO_enable_processing_cmt,    // override UNCRUSTIFY_DEFAULT_ON_TEXT
   UO_enable_digraphs,
   UO_utf8_bom,
   UO_utf8_byte,
   UO_utf8_force,

   // group: UG_space, "Spacing options"                                                        1
   UO_sp_arith,                    // space around + - / * etc
                                   // also ">>>" "<<" ">>" "%" "|"
   UO_sp_arith_additive,           // space around + or -
   UO_sp_assign,                   // space around =, +=, etc
   UO_sp_cpp_lambda_assign,        // space around the capture spec [=](...){...}
   UO_sp_cpp_lambda_paren,         // space after the capture spec [] (...){...}
   UO_sp_assign_default,           // space around '=' in prototype
   UO_sp_before_assign,            // space before =, +=, etc
   UO_sp_after_assign,             // space after =, +=, etc
   UO_sp_enum_paren,               // space in 'NS_ENUM ('"
   UO_sp_enum_assign,              // space around = in enum
   UO_sp_enum_before_assign,       // space before = in enum
   UO_sp_enum_after_assign,        // space after = in enum
   UO_sp_enum_colon,               // space around ':' in enum
   UO_sp_pp_concat,                // space around ##
   UO_sp_pp_stringify,             // space after #
   UO_sp_before_pp_stringify,      // space before # in a #define x(y) L#y
   UO_sp_bool,                     // space around || &&
   UO_sp_compare,                  // space around < > ==, etc
   UO_sp_inside_paren,             // space inside '+ ( xxx )' vs '+ (xxx)'
   UO_sp_paren_paren,              // space between nested parens - '( (' vs '(('
   UO_sp_cparen_oparen,            // space between nested parens - ') (' vs ')('
   UO_sp_balance_nested_parens,    // balance spaces inside nested parens
   UO_sp_paren_brace,              // space between ')' and '{'
   UO_sp_before_ptr_star,          // space before a '*' that is part of a type
   UO_sp_before_unnamed_ptr_star,  //
   UO_sp_between_ptr_star,         // space between two '*' that are part of a type
   UO_sp_after_ptr_star,           // space after a '*' that is part of a type
   UO_sp_after_ptr_star_qualifier, // space after a '*' next to a qualifier
   UO_sp_after_ptr_star_func,      // space between a '*' and a function proto/def
   UO_sp_ptr_star_paren,           //
   UO_sp_before_ptr_star_func,     //
   UO_sp_before_byref,             // space before '&' of 'fcn(int& idx)'
   UO_sp_before_unnamed_byref,     //
   UO_sp_after_byref,              // space after a '&'  as in 'int& var'
   UO_sp_after_byref_func,         //
   UO_sp_before_byref_func,        //
   UO_sp_after_type,               // space between type and word
   UO_sp_before_template_paren,    // D: 'template Foo('
   UO_sp_template_angle,           //
   UO_sp_before_angle,             // space before '<>', as in '<class T>'
   UO_sp_inside_angle,             // space inside '<>', as in '<class T>'
   UO_sp_angle_colon,              // space between '<>' and ':'
   UO_sp_after_angle,              // space after  '<>', as in '<class T>'
   UO_sp_angle_paren,              // space between '<>' and '(' in 'a = new List<byte>(foo);'
   UO_sp_angle_paren_empty,        // space between '<>' and '()' in 'a = new List<byte>();'
   UO_sp_angle_word,               // space between '<>' and a word in 'List<byte> a;
                                   // or template <typename T> static ...'
   UO_sp_angle_shift,              // '> >' vs '>>'
   UO_sp_permit_cpp11_shift,       // '>>' vs '> >' for C++11 code
   UO_sp_before_sparen,            // space before '(' of 'if/for/while/switch/etc'
   UO_sp_inside_sparen,            // space inside 'if( xxx )' vs 'if(xxx)'
   UO_sp_inside_sparen_close,      //
   UO_sp_inside_sparen_open,       //
   UO_sp_after_sparen,             // space after  ')' of 'if/for/while/switch/etc'
                                   // the do-while does not get set here
   UO_sp_sparen_brace,             // space between ')' and '{' of if, while, etc
   UO_sp_invariant_paren,          //
   UO_sp_after_invariant_paren,    //
   UO_sp_special_semi,             // space empty stmt ';' on while, if, for
                                   //   example 'while (*p++ = ' ') ;'
   UO_sp_before_semi,              // space before all ';'
   UO_sp_before_semi_for,          // space before the two ';' in a for() - non-empty
   UO_sp_before_semi_for_empty,    // space before ';' in empty for statement
   UO_sp_after_semi,               //
   UO_sp_after_semi_for,           //
   UO_sp_after_semi_for_empty,     // space after final ';' in empty for statement
   UO_sp_before_square,            // space before single '['
   UO_sp_before_squares,           // space before '[]', as in 'byte []'
   UO_sp_inside_square,            // space inside 'byte[ 5 ]' vs 'byte[5]'
   UO_sp_after_comma,              // space after ','
   UO_sp_before_comma,             // space before ','
   UO_sp_after_mdatype_commas,     //
   UO_sp_before_mdatype_commas,    //
   UO_sp_between_mdatype_commas,   //
   UO_sp_paren_comma,              //
   UO_sp_before_ellipsis,          // space before '...'
   UO_sp_after_class_colon,        // space after class ':'
   UO_sp_before_class_colon,       // space before class ':'
   UO_sp_after_constr_colon,       // space after class constructor ':'
   UO_sp_before_constr_colon,      // space before class constructor ':'
   UO_sp_before_case_colon,        // space before case ':'
   UO_sp_after_operator,           // space after operator when followed by a punctuator
   UO_sp_after_operator_sym,       // space after operator when followed by a punctuator
   UO_sp_after_operator_sym_empty, // space after operator sign when the operator has no arguments
   UO_sp_after_cast,               // space after C & D cast - '(int) a' vs '(int)a'
   UO_sp_inside_paren_cast,        // spaces inside the parens of a cast
   UO_sp_cpp_cast_paren,           //
   UO_sp_sizeof_paren,             // space between 'sizeof' and '('
   UO_sp_after_tag,                // pawn: space after a tag colon
   UO_sp_inside_braces_enum,       // space inside enum '{' and '}' - '{ a, b, c }'
   UO_sp_inside_braces_struct,     // space inside struct/union '{' and '}'
   UO_sp_after_type_brace_init_lst_open,
   UO_sp_before_type_brace_init_lst_close,
   UO_sp_inside_type_brace_init_lst,
   UO_sp_inside_braces,            // space inside '{' and '}' - '{ 1, 2, 3 }'
   UO_sp_inside_braces_empty,      // space inside '{' and '}' - '{ }'
   UO_sp_type_func,                // space between return type and 'func'
                                   // a minimum of 1 is forced except for '*'
   UO_sp_type_brace_init_lst,
   UO_sp_func_proto_paren,         // space between 'func' and '(' - 'foo (' vs 'foo('
   UO_sp_func_proto_paren_empty,   // space between 'func' and '()' - "foo ()" vs "foo()"
   UO_sp_func_def_paren,           // space between 'func' and '(' - 'foo (' vs 'foo('
   UO_sp_func_def_paren_empty,     // space between 'func' and '()' - "foo ()" vs "foo()"
   UO_sp_inside_fparens,           // space inside 'foo( )' vs 'foo()'
   UO_sp_inside_fparen,            // space inside 'foo( xxx )' vs 'foo(xxx)'
   UO_sp_inside_tparen,            //
   UO_sp_after_tparen_close,       //
   UO_sp_square_fparen,            // weird pawn stuff: native yark[rect](a[rect])
   UO_sp_fparen_brace,             // space between ')' and '{' of function
   UO_sp_fparen_dbrace,            // space between ')' and '{{' of double-brace init
   UO_sp_func_call_paren,          // space between 'func' and '(' - 'foo (' vs 'foo('
   UO_sp_func_call_paren_empty,    //
   UO_sp_func_call_user_paren,     //
   UO_sp_func_class_paren,         // space between ctor/dtor and '('
   UO_sp_func_class_paren_empty,   // space between ctor/dtor and '()'
   UO_sp_return_paren,             // space between 'return' and '('
   UO_sp_attribute_paren,          // space between '__attribute__' and '('
   UO_sp_defined_paren,            //
   UO_sp_throw_paren,              //
   UO_sp_after_throw,              //
   UO_sp_catch_paren,              //
   UO_sp_version_paren,            //
   UO_sp_scope_paren,              //
   UO_sp_super_paren,              //
   UO_sp_this_paren,               //
   UO_sp_macro,                    // space between macro and value, ie '#define a 6'
   UO_sp_macro_func,               // space between macro and value, ie '#define a 6'
   UO_sp_else_brace,               //
   UO_sp_brace_else,               //
   UO_sp_brace_typedef,            //
   UO_sp_catch_brace,              //
   UO_sp_brace_catch,              //
   UO_sp_finally_brace,            //
   UO_sp_brace_finally,            //
   UO_sp_try_brace,                //
   UO_sp_getset_brace,             //
   UO_sp_word_brace,               //
   UO_sp_word_brace_ns,            //
   UO_sp_before_dc,                //
   UO_sp_after_dc,                 //
   UO_sp_d_array_colon,            //
   UO_sp_not,                      //
   UO_sp_inv,                      //
   UO_sp_addr,                     //
   UO_sp_member,                   //
   UO_sp_deref,                    //
   UO_sp_sign,                     //
   UO_sp_incdec,                   //
   UO_sp_before_nl_cont,           //
   UO_sp_after_oc_scope,           //
   UO_sp_after_oc_colon,           //
   UO_sp_before_oc_colon,          //
   UO_sp_after_oc_dict_colon,      //
   UO_sp_before_oc_dict_colon,     //
   UO_sp_after_send_oc_colon,      //
   UO_sp_before_send_oc_colon,     //
   UO_sp_after_oc_type,            //
   UO_sp_after_oc_return_type,     //
   UO_sp_after_oc_at_sel,          //
   UO_sp_after_oc_at_sel_parens,   //
   UO_sp_inside_oc_at_sel_parens,  //
   UO_sp_before_oc_block_caret,    //
   UO_sp_after_oc_block_caret,     //
   UO_sp_after_oc_msg_receiver,    //
   UO_sp_after_oc_property,        //
   UO_sp_cond_colon,               //
   UO_sp_cond_colon_before,        //
   UO_sp_cond_colon_after,         //
   UO_sp_cond_question,            //
   UO_sp_cond_question_before,     //
   UO_sp_cond_question_after,      //
   UO_sp_cond_ternary_short,       //
   UO_sp_case_label,               //
   UO_sp_range,                    //
   UO_sp_after_for_colon,          //
   UO_sp_before_for_colon,         //
   UO_sp_extern_paren,             //
   UO_sp_cmt_cpp_start,            //
   UO_sp_cmt_cpp_doxygen,          // in case of UO_sp_cmt_cpp_start:
                                   // treat '///', '///<', '//!' and '//!<' as a unity (add space behind)
   UO_sp_cmt_cpp_qttr,             // in case of UO_sp_cmt_cpp_start:
                                   // treat '//:', '//=', '//~' as a unity (add space behind)
   UO_sp_endif_cmt,                //
   UO_sp_after_new,                //
   UO_sp_between_new_paren,        //
   UO_sp_after_newop_paren,        //
   UO_sp_inside_newop_paren,       //
   UO_sp_inside_newop_paren_open,  //
   UO_sp_inside_newop_paren_close, //
   UO_sp_before_tr_emb_cmt,        // treatment of spaces before comments following code
   UO_sp_num_before_tr_emb_cmt,    // number of spaces before comments following code
   UO_sp_annotation_paren,         //
   UO_sp_skip_vbrace_tokens,       //
   UO_force_tab_after_define,      // force <TAB> after #define, Issue # 876

   // group: UG_indent, "Indenting"                                                                2
   UO_indent_columns,                       // ie 3 or 8
   UO_indent_continue,                      //
   UO_indent_param,                         // indent value of indent_*_param
   UO_indent_with_tabs,                     // 1=only to the 'level' indent, 2=use tabs for indenting
   UO_indent_cmt_with_tabs,                 //
   UO_indent_align_string,                  // True/False - indent align broken strings
   UO_indent_xml_string,                    // Number amount to indent XML strings
   UO_indent_brace,                         // spaces to indent '{' from level (usually 0)
   UO_indent_braces,                        // whether to indent the braces or not
   UO_indent_braces_no_func,                // whether to not indent the function braces
                                            // (depends on UO_indent_braces)
   UO_indent_braces_no_class,               // whether to not indent the class braces
                                            // (depends on UO_indent_braces)
   UO_indent_braces_no_struct,              // whether to not indent the struct braces
                                            // (depends on UO_indent_braces)
   UO_indent_brace_parent,                  // indent the braces based on the parent size (if=3, for=4, etc)
   UO_indent_paren_open_brace,              // indent on paren level in '({', default by {
   UO_indent_cs_delegate_brace,             // indent a C# delegate by another level. default: false
   UO_indent_namespace,                     // indent stuff inside namespace braces
   UO_indent_namespace_single_indent,       // indent one namespace and no sub-namespaces
   UO_indent_namespace_level,               // level to indent namespace blocks
   UO_indent_namespace_limit,               // no indent if namespace is longer than this
   UO_indent_extern,
   UO_indent_class,                         // indent stuff inside class braces
   UO_indent_class_colon,                   // indent stuff after a class colon
   UO_indent_class_on_colon,                // indent stuff on a class colon
   UO_indent_constr_colon,                  // indent stuff after a constr colon
   UO_indent_ctor_init_leading,             // virtual indent from the ':' for member initializers.
                                            // Default is 2. (applies to the leading colon case)
   UO_indent_ctor_init,                     // additional indenting for ctor initializer lists
   UO_indent_else_if,                       //
   UO_indent_var_def_blk,                   // indent a variable def block that appears at the top
   UO_indent_var_def_cont,                  //
   UO_indent_shift,                         // if a shift expression spans multiple lines, indent
   UO_indent_func_def_force_col1,           // force indentation of function definition to start in column 1
   UO_indent_func_call_param,               // indent continued function calls to indent_columns
   UO_indent_func_def_param,                // same, but for function defs
   UO_indent_func_proto_param,              // same, but for function protos
   UO_indent_func_class_param,              // same, but for classes
   UO_indent_func_ctor_var_param,           //
   UO_indent_template_param,                //
   UO_indent_func_param_double,             // double the tab indent for
                                            // Use both values of the options indent_columns and indent_param
   UO_indent_func_const,                    // indentation for standalone 'const' qualifier
   UO_indent_func_throw,                    // indentation for standalone 'throw' qualifier
   UO_indent_member,                        // indent lines broken at a member '.' or '->'
   UO_indent_sing_line_comments,            // indent single line ('//') comments on lines before code
   UO_indent_relative_single_line_comments, // indent single line ('//') comments after code
   UO_indent_switch_case,                   // spaces to indent case from switch
   UO_indent_switch_pp,                     // whether to indent preproccesor statements inside of switch statements
   UO_indent_case_shift,                    // spaces to shift the line with the 'case'
   UO_indent_case_brace,                    // spaces to indent '{' from case (usually 0 or indent_columns)
   UO_indent_col1_comment,                  // indent comments in column 1
   UO_indent_label,                         // 0=left >0=col from left, <0=sub from brace indent
   UO_indent_access_spec,                   // same as indent_label, but for 'private:', 'public:'
   UO_indent_access_spec_body,              // indent private/public/protected inside a class
                                            // (overrides indent_access_spec)
   UO_indent_paren_nl,                      // indent-align under paren for open followed by nl
   UO_indent_paren_close,                   // indent of close paren after a newline
   UO_indent_paren_after_func_def,          // indent of open paren for a function definition
   UO_indent_paren_after_func_decl,         // indent of open paren for a function declaration
   UO_indent_paren_after_func_call,         // indent of open paren for a function call
   UO_indent_comma_paren,                   // indent of comma if inside a paren
   UO_indent_bool_paren,                    // indent of bool if inside a paren
   UO_indent_first_bool_expr,               // if UO_indent_bool_paren == true, aligns the first
                                            // expression to the following ones
   UO_indent_square_nl,                     // indent-align under square for open followed by nl
   UO_indent_preserve_sql,                  // preserve indent of EXEC SQL statement body
   UO_indent_align_assign,                  //
   UO_indent_oc_block,                      //
   UO_indent_oc_block_msg,                  //
   UO_indent_oc_msg_colon,                  //
   UO_indent_oc_msg_prioritize_first_colon, //
   UO_indent_oc_block_msg_xcode_style,      //
   UO_indent_oc_block_msg_from_keyword,     //
   UO_indent_oc_block_msg_from_colon,       //
   UO_indent_oc_block_msg_from_caret,       //
   UO_indent_oc_block_msg_from_brace,       //
   UO_indent_min_vbrace_open,               // min. indent after virtual brace open and newline
   UO_indent_vbrace_open_on_tabstop,        // when identing after virtual brace open and newline
                                            // add further spaces to reach next tabstop
   UO_indent_token_after_brace,             //
   UO_indent_cpp_lambda_body,               // indent cpp lambda or not
   UO_indent_using_block,                   // indent (or not) an using block if no braces are used,
   UO_indent_ternary_operator,              // indent continuation of ternary operator
   UO_indent_ignore_asm_block,              // ignore indent and align for asm blocks as they have their own indentation
   // UO_indent_brace_struct,      TODO: spaces to indent brace after struct/enum/union def
   // UO_indent_paren,             TODO: indent for open paren on next line (1)
   // UO_indent,                   TODO: 0=don't change indentation, 1=change indentation

   // group: UG_newline, "Newline adding and removing options"                                  3
   UO_nl_collapse_empty_body,         // change '{ \n }' into '{}'
   UO_nl_assign_leave_one_liners,     // leave one-line assign bodies in 'foo_t f = { a, b, c };'
   UO_nl_class_leave_one_liners,      // leave one-line function bodies in 'class xx { here }'
   UO_nl_enum_leave_one_liners,       // leave one-line enum bodies in 'enum FOO { BAR = 5 };'
   UO_nl_getset_leave_one_liners,     // leave one-line get/set bodies
   UO_nl_func_leave_one_liners,       // leave one-line function def bodies
   UO_nl_cpp_lambda_leave_one_liners, // leave one-line C++11 lambda bodies
   UO_nl_if_leave_one_liners,         //
   UO_nl_while_leave_one_liners,      //
   UO_nl_oc_msg_leave_one_liner,      // Don't split one-line OC messages
   UO_nl_oc_block_brace,              // Add or remove newline between Objective-C block signature and '{'
   UO_nl_start_of_file,               // alter newlines at the start of file
   UO_nl_start_of_file_min,           // min number of newlines at the start of the file
   UO_nl_end_of_file,                 // alter newlines at the end of file
   UO_nl_end_of_file_min,             // min number of newlines at the end of the file
   UO_nl_assign_brace,                // newline between '=' and '{'
   UO_nl_assign_square,               // newline between '=' and '['
   UO_nl_after_square_assign,         // newline after '= ['
   UO_nl_func_var_def_blk,            // newline after first block of func variable defs
   UO_nl_typedef_blk_start,           // newline before typedef block
   UO_nl_typedef_blk_end,             // newline after typedef block
   UO_nl_typedef_blk_in,              // newline max within typedef block
   UO_nl_var_def_blk_start,           // newline before variable defs block
   UO_nl_var_def_blk_end,             // newline after variable defs block
   UO_nl_var_def_blk_in,              // newline max within variable defs block
   UO_nl_fcall_brace,                 // newline between function call and open brace
   UO_nl_enum_brace,                  // newline between enum and brace
   UO_nl_enum_class,                  // newline between enum and class
   UO_nl_enum_class_identifier,       // newline between enum class and 'identifier'
   UO_nl_enum_identifier_colon,       // newline between enum class 'type' and ':'
   UO_nl_enum_colon_type,             // newline between enum class identifier :' and 'type'
                                      // newline between enum class identifier :' 'type' and 'type'
                                      // i.e.            enum class abcd : unsigned int
   UO_nl_struct_brace,                // newline between struct and brace
   UO_nl_union_brace,                 // newline between union and brace
   UO_nl_if_brace,                    // newline between 'if' and '{'
   UO_nl_brace_else,                  // newline between '}' and 'else'
   UO_nl_elseif_brace,                // newline between close paren and open brace in 'else if () {'
   UO_nl_else_brace,                  // newline between 'else' and '{'
   UO_nl_else_if,                     // newline between 'else' and 'if'
   UO_nl_before_if_closing_paren,     // newline before 'if'/'else if' closing parenthesis
   UO_nl_brace_finally,               // newline between '}' and 'finally'
   UO_nl_finally_brace,               // newline between 'finally' and '{'
   UO_nl_try_brace,                   // newline between 'try' and '{'
   UO_nl_getset_brace,                // newline between 'get/set' and '{'
   UO_nl_for_brace,                   // newline between 'for' and '{'
   UO_nl_catch_brace,                 // newline between 'catch' and '{'
   UO_nl_brace_catch,                 // newline between '}' and 'catch'
   UO_nl_brace_square,                // newline between '}' and ']'
   UO_nl_brace_fparen,                // newline between '}' and ')' of a function invocation
   UO_nl_while_brace,                 // newline between 'while' and '{'
   UO_nl_scope_brace,                 // Add or remove newline between 'scope (x)' and '{' (D)
   UO_nl_unittest_brace,              // newline between 'unittest' and '{'
   UO_nl_version_brace,               // Add or remove newline between 'version (x)' and '{' (D)
   UO_nl_using_brace,                 // Add or remove newline between 'using' and '{'
   UO_nl_brace_brace,                 // newline between '{{' or '}}'
   UO_nl_do_brace,                    // newline between 'do' and '{'
   UO_nl_brace_while,                 // newline between '}' and 'while' of do stmt
   UO_nl_switch_brace,                // newline between 'switch' and '{'
   UO_nl_synchronized_brace,          // newline between 'synchronized' and '{'
   UO_nl_multi_line_cond,             // newline between ')' and '{' when cond spans >=2 lines
   UO_nl_multi_line_define,           // newline after define XXX for multi-line define
   UO_nl_before_case,                 // newline before 'case' statement, not after the first 'case'
   UO_nl_before_throw,                // Add or remove newline between ')' and 'throw'
   UO_nl_after_case,                  // disallow nested 'case 1: a=3;'
   UO_nl_case_colon_brace,            // Add or remove a newline between a case ':' and '{'.
                                      // Overrides nl_after_case
   UO_nl_namespace_brace,             // newline between namespace name and brace
   UO_nl_template_class,              // newline between '>' and class in 'template <x> class'
   UO_nl_class_brace,                 // newline between class name and brace
   UO_nl_class_init_args,             // newline before/after each comma in the base class list
                                      // (tied to UO_pos_class_comma)
   UO_nl_constr_init_args,            // newline after comma in class init args
   UO_nl_enum_own_lines,              // put each element of an enum def. on its own line
   UO_nl_func_type_name,              // newline between return type and func name in def
   UO_nl_func_type_name_class,        // newline between return type and func name in class
   UO_nl_func_class_scope,            // Add or remove newline between class specification and '::'
                                      // in 'void A::f() { }'
                                      // Only appears in separate member implementation (does not appear
                                      // with in-line implmementation)
   UO_nl_func_scope_name,             // Add or remove newline between function scope and name in a definition
                                      // Controls the newline after '::' in 'void A::f() { }'
   UO_nl_func_proto_type_name,        // nl_func_type_name, but for prottypes
   UO_nl_func_paren,                  // newline between function and open paren
   UO_nl_func_paren_empty,            // Overrides nl_func_paren for functions with no parameters
   UO_nl_func_def_paren,              // Add or remove newline between a function name and
                                      // the opening '(' in the definition
   UO_nl_func_def_paren_empty,        // Overrides nl_func_def_paren for functions with no parameters
   UO_nl_func_call_paren,             // Add or remove newline between a function name and
                                      // the opening '(' in the call
   UO_nl_func_call_paren_empty,       // Overrides nl_func_call_paren for functions with no parameters
   UO_nl_func_decl_start,             // newline after the '(' in a function decl
   UO_nl_func_def_start,              // newline after the '(' in a function def
   UO_nl_func_decl_start_single,      // Overrides nl_func_decl_start when there is only one parameter
   UO_nl_func_def_start_single,       // Overrides nl_func_def_start when there is only one parameter
   UO_nl_func_decl_start_multi_line,  // newline after the '(' in a function decl if '(' and ')'
                                      // are on different lines
   UO_nl_func_def_start_multi_line,   // newline after the '(' in a function def if '(' and ')'
                                      // are on different lines
   UO_nl_func_decl_args,              // newline after each ',' in a function decl
   UO_nl_func_def_args,               // Add or remove newline after each ',' in a function definition
   UO_nl_func_decl_args_multi_line,   // newline after each ',' in a function decl if '(' and ')'
                                      // are on different lines
   UO_nl_func_def_args_multi_line,    // Add or remove newline after each ',' in a function definition
                                      // if '(' and ')' are on different lines
   UO_nl_func_decl_end,               // newline before the ')' in a function decl
   UO_nl_func_def_end,                // newline before the ')' in a function def
   UO_nl_func_decl_end_single,        // Overrides nl_func_decl_end when there is only one parameter
   UO_nl_func_def_end_single,         // Overrides nl_func_def_end when there is only one parameter
   UO_nl_func_decl_end_multi_line,    // newline before the ')' in a function decl if '(' and ')'
                                      // are on different lines
   UO_nl_func_def_end_multi_line,     // newline before the ')' in a function def if '(' and ')'
                                      // are on different lines
   UO_nl_func_decl_empty,             // as above, but for empty parens '()'
   UO_nl_func_def_empty,              // as above, but for empty parens '()'
   UO_nl_func_call_empty,             // as above, but for empty parens '()'
   UO_nl_func_call_start_multi_line,  // newline after the '(' in a function call if '(' and ')'
                                      // are on different lines
   UO_nl_func_call_args_multi_line,   // newline after each ',' in a function call if '(' and ')'
                                      // are on different lines
   UO_nl_func_call_end_multi_line,    // newline before the ')' in a function call if '(' and ')'
                                      // are on different lines
   UO_nl_oc_msg_args,                 // Whether to put each OC message parameter on a separate line
                                      // See nl_oc_msg_leave_one_liner
   UO_nl_fdef_brace,                  // 'int foo() {' vs 'int foo()\n{'
   UO_nl_cpp_ldef_brace,              // '[&x](int a) {' vs '[&x](int a)\n{'
   UO_nl_return_expr,                 // Add or remove a newline between the 'return' keyword and
                                      // 'return' expression
   UO_nl_after_semicolon,             // disallow multiple statements on a line 'a=1;b=4;'
   UO_nl_paren_dbrace_open,           // Java: Control the newline between the ')' and '{{' of the
                                      // double brace initializer
   UO_nl_type_brace_init_lst,         // newline between type and unnamed temporary direct-list-initialization
   UO_nl_type_brace_init_lst_open,    // newline after open brace in unnamed temporary direct-list-initialization
   UO_nl_type_brace_init_lst_close,   // newline before close brace in unnamed temporary direct-list-initialization
   UO_nl_after_brace_open,            // force a newline after a brace open
   UO_nl_after_brace_open_cmt,        // put the newline before the comment
   UO_nl_after_vbrace_open,           // force a newline after a virtual brace open
   UO_nl_after_vbrace_open_empty,     // force a newline after a virtual brace open
   UO_nl_after_brace_close,           // force a newline after a brace close
   UO_nl_after_vbrace_close,          // force a newline after a virtual brace close
   UO_nl_brace_struct_var,            // force a newline after a brace close
   UO_nl_define_macro,                // alter newlines in #define macros
   UO_nl_squeeze_ifdef,               // no blanks after #ifxx, #elxx, or before #elxx and #endif
   UO_nl_squeeze_ifdef_top_level,     // when set, nl_squeeze_ifdef will be applied to top-level
                                      // #ifdefs as well
   UO_nl_before_if,                   // newline before 'if'
   UO_nl_after_if,                    // newline after 'if'/'else'
   UO_nl_before_for,                  // newline before 'for'
   UO_nl_after_for,                   // newline after for 'close'
   UO_nl_before_while,                // newline before 'while'
   UO_nl_after_while,                 // newline after while 'close'
   UO_nl_before_switch,               // newline before 'switch'
   UO_nl_after_switch,                // newline after switch 'close'
   UO_nl_before_synchronized,         // newline before 'synchronized'
   UO_nl_after_synchronized,          // newline after synchronized 'close'
   UO_nl_before_do,                   // newline before 'do'
   UO_nl_after_do,                    // newline after 'while' of do
   UO_nl_ds_struct_enum_cmt,          // newline between commented-elements of struct/enum
   UO_nl_ds_struct_enum_close_brace,  // force newline before '}' of struct/union/enum
   UO_nl_before_func_class_def,       // newline before 'func_class_def'
   UO_nl_before_func_class_proto,     // newline before 'func_class_proto'
   UO_nl_class_colon,                 // newline before/after class colon (tied to UO_pos_class_colon)
   UO_nl_constr_colon,                // newline before/after class constr colon (tied to UO_pos_constr_colon)
   UO_nl_create_if_one_liner,         // Change simple unbraced if statements into a one-liner
                                      // 'if(b)\n i++;' => 'if(b) i++;'
   UO_nl_create_for_one_liner,        // Change simple unbraced for statements into a one-liner
                                      // 'for (i=0;i<5;i++)\n foo(i);' => 'for (i=0;i<5;i++) foo(i);'
   UO_nl_create_while_one_liner,      // Change simple unbraced while statements into a one-liner
                                      // 'while (i<5)\n foo(i++);' => 'while (i<5) foo(i++);'
                                      // Change that back:
   UO_nl_split_if_one_liner,          // Change a one-liner for statement into simple unbraced for
                                      // 'if(b) i++;' => 'if(b)\n i++;'
   UO_nl_split_for_one_liner,         // Change a one-liner while statement into simple unbraced while
                                      // 'for (i=0;i<5;i++) foo(i);' => 'for (i=0;i<5;i++)\n foo(i);'
   UO_nl_split_while_one_liner,       // Change a one-liner if statement into simple unbraced if
                                      // 'while (i<5) foo(i++);' => 'while (i<5)\n foo(i++);'

   // group: UG_blankline, "Blank line options", "Note that it takes 2 newlines to get a blank line"      4
   UO_nl_max,                          // maximum consecutive newlines (3 = 2 blank lines)
   UO_nl_max_blank_in_func,            // maximum n-1 consecutive newlines in function (n <= 0 = No change)
                                       // Will not change the newline count if after a brace open (0 = No change)
   UO_nl_after_func_proto,             // The number of newlines after a function prototype, if followed
                                       // by another function prototype
   UO_nl_after_func_proto_group,       // The number of newlines after a function prototype, if not followed
                                       // by another function prototype
   UO_nl_after_func_class_proto,       // The number of newlines after a function class prototype, if followed
                                       // by another function class prototype
   UO_nl_after_func_class_proto_group, // The number of newlines after a function class prototype, if not
                                       // followed by another function class prototype
   UO_nl_before_func_body_def,         // The number of newlines before a multi-line function def body
   UO_nl_before_func_body_proto,       // The number of newlines before a multi-line function prototype body
   UO_nl_after_func_body,              // The number of newlines after '}' of a multi-line function body
   UO_nl_after_func_body_class,        // The number of newlines after '}' of a multi-line function body
                                       // in a class declaration
   UO_nl_after_func_body_one_liner,    // The number of newlines after '}' of a single line function body
   UO_nl_before_block_comment,         // before a block comment (stand-alone comment-multi),
                                       // except after brace open
   UO_nl_before_c_comment,             // The minimum number of newlines before a single-line C comment
                                       // Doesn't apply if after a brace open or other single-line C comments
   UO_nl_before_cpp_comment,           // The minimum number of newlines before a CPP comment
                                       // Doesn't apply if after a brace open or other CPP comments
   UO_nl_after_multiline_comment,      // newline after multiline comment
   UO_nl_after_label_colon,            // newline after a label followed by a colon
   UO_nl_after_struct,                 // The number of newlines after '}' or ';' of a
                                       // struct/enum/union definition
   UO_nl_before_class,                 // The number of newlines before a class definition
   UO_nl_after_class,                  // The number of newlines after '}' or ';' of a class definition
   UO_nl_before_access_spec,           // The number of newlines before a 'private:', 'public:',
                                       // 'protected:', 'signals:', or 'slots:' label
   UO_nl_after_access_spec,            // The number of newlines after a 'private:', 'public:',
                                       // 'protected:', 'signals:' or 'slots:' label
                                       // (0 = No change)
   UO_nl_comment_func_def,             // The number of newlines between a function def and the function comment
                                       // (0 = No change)
   UO_nl_after_try_catch_finally,      // The number of newlines after a try-catch-finally block
                                       // that isn't followed by a brace close
                                       // (0 = No change)
   UO_nl_around_cs_property,           // The number of newlines before and after a property, indexer
                                       // or event decl
                                       // (0 = No change)
   UO_nl_between_get_set,              // The number of newlines between the get/set/add/remove handlers in C#
                                       // (0 = No change)
   UO_nl_property_brace,               // Add or remove newline between C# property and the '{'
   UO_eat_blanks_after_open_brace,     // remove blank lines after {
   UO_eat_blanks_before_close_brace,   // remove blank lines before }
   UO_nl_remove_extra_newlines,        // How aggressively to remove extra newlines not in preproc
                                       // (0 = No change)
                                       // (1 = Remove most newlines not handled by other config)
                                       // (2 = Remove all newlines and reformat completely by config)
   UO_nl_before_return,                // Whether to put a blank line before 'return' statements,
                                       // unless after an open brace
   UO_nl_after_return,                 // newline after 'return' statement
   UO_nl_after_annotation,             // Whether to put a newline after a Java annotation statement
                                       // Only affects annotations that are after a newline
   UO_nl_between_annotation,           // Controls the newline between two annotations
   // UO_nl_after_ifdef,                 after #if or #ifdef - but not if covers whole file
   // UO_nl_after_func_class_def,         newline after 'func_class_def'
   // UO_ls_before_bool_op,    TODO: break line before or after boolean op
   // UO_ls_before_paren,      TODO: break before open paren
   // UO_ls_after_arith,       TODO: break after arith op '+', etc
   // UO_ls_honor_newlines,    TODO: don't remove newlines on split lines

   // group: UG_position, "Positioning options"                                                           5
   UO_pos_arith,                      // position of trailing/leading arithmetic ops
   UO_pos_assign,                     // position of trailing/leading =
   UO_pos_bool,                       // position of trailing/leading &&/||
   UO_pos_compare,                    // position of trailing/leading <=/>, etc
   UO_pos_conditional,                // position of trailing/leading (b ? t : f)
   UO_pos_comma,                      // position of comma in functions
   UO_pos_enum_comma,                 // position of comma in enum entries
   UO_pos_class_comma,                // position of comma in the base class list if there
                                      // are more than one line,
                                      //   (tied to UO_nl_class_init_args).
   UO_pos_constr_comma,               // position of comma in constructor init list
   UO_pos_class_colon,                // position of trailing/leading class colon, between
                                      // class and base class list
                                      //   (tied to UO_nl_class_colon)
   UO_pos_constr_colon,               // position of trailing/leading class constr colon
                                      //   (tied to UO_nl_constr_colon, UO_nl_constr_init_args,
                                      // UO_pos_constr_colon,

   // group: UG_linesplit, "Line Splitting options"                                                       6
   UO_code_width,           // ie 80 columns
   UO_ls_for_split_full,    // try to split long 'for' statements at semi-colons
   UO_ls_func_split_full,   // try to split long func proto/def at comma
   UO_ls_code_width,        // try to split at code_width

   // group: UG_align, "Code alignment (not left column spaces/tabs)"                                     7
   UO_align_keep_tabs,             // keep non-indenting tabs
   UO_align_with_tabs,             // use tabs for aligning (0/1)
   UO_align_on_tabstop,            // always align on tabstops
   UO_align_number_right,          // right-align numbers (not fully supported, yet)
   UO_align_keep_extra_space,      // don't squash extra whitespace
   UO_align_func_params,           // align prototype variable defs on variable
   UO_align_func_params_span,      // align parameter defs in function on parameter name
   UO_align_func_params_thresh,    // align function parameter defs threshold
   UO_align_func_params_gap,       // align function parameter defs gap
   UO_align_same_func_call_params, //
   UO_align_var_def_span,          // align variable defs on variable (span for regular stuff)
   UO_align_var_def_star_style,    // see UO_align_typedef_star_style
   UO_align_var_def_amp_style,     // see UO_align_typedef_star_style
   UO_align_var_def_thresh,        // align variable defs threshold
   UO_align_var_def_gap,           // align variable defs gap
   UO_align_var_def_colon,         // align the colon in struct bit fields
   UO_align_var_def_colon_gap,     // align variable defs gap for bit colons
   UO_align_var_def_attribute,     //
   UO_align_var_def_inline,        // also align inline struct/enum/union var defs
   UO_align_assign_span,           // align on '='. 0=don't align
   UO_align_assign_thresh,         // threshold for aligning on '='. 0=no limit
   UO_align_enum_equ_span,         // align the '=' in enums
   UO_align_enum_equ_thresh,       // threshold for aligning on '=' in enums. 0=no limit
   UO_align_var_class_span,        // span for class (0=don't align)
   UO_align_var_class_thresh,      // threshold for class, 0=no limit
   UO_align_var_class_gap,         // gap for class
   UO_align_var_struct_span,       // span for struct/union (0=don't align)
   UO_align_var_struct_thresh,     // threshold for struct/union, 0=no limit
   UO_align_var_struct_gap,        // gap for struct/union
   UO_align_struct_init_span,      // align structure initializer values
   UO_align_typedef_gap,           // minimum spacing
   UO_align_typedef_span,          // align single-line typedefs
   UO_align_typedef_func,          // how to align func type with types
   UO_align_typedef_star_style,    // Start aligning style
                                   // 0: '*' not part of type
                                   // 1: '*' part of the type - no space
                                   // 2: '*' part of type, dangling
   UO_align_typedef_amp_style,     // align_typedef_star_style for ref '&' stuff
   UO_align_right_cmt_span,        // align comment that end lines. 0=don't align
   UO_align_right_cmt_mix,         // mix comments after '}' and preproc with others
   UO_align_right_cmt_gap,         //
   UO_align_right_cmt_at_col,      // align comment that end lines at or beyond column N;
                                   // 'pulls in' comments as a bonus side effect
   UO_align_func_proto_span,       // align function prototypes
   UO_align_func_proto_gap,        // align function prototypes
   UO_align_on_operator,           //
   UO_align_mix_var_proto,         // mix function prototypes and variable decl
   UO_align_single_line_func,      // mix single line function with prototypes
   UO_align_single_line_brace,     // align the open brace of single line functions
   UO_align_single_line_brace_gap, // gap for align_single_line_brace
   UO_align_oc_msg_spec_span,      // align ObjC msg spec
   UO_align_nl_cont,               // align the back-slash \n combo (macros)
   UO_align_pp_define_together,    // align macro functions and variables together
   UO_align_pp_define_gap,         // min space between define label and value '#define a <---> 16'
   UO_align_pp_define_span,        // align bodies in #define statements
   UO_align_left_shift,            //
   UO_align_asm_colon,             //
   UO_align_oc_msg_colon_span,     //
   UO_align_oc_msg_colon_first,    //
   UO_align_oc_decl_colon,         //
   // UO_align_pp_define_col_min,    TODO: min column for a #define value
   // UO_align_pp_define_col_max,    TODO: max column for a #define value
   // UO_align_enum_col_min,         TODO: the min column for enum '=' alignment
   // UO_align_enum_col_max,         TODO: the max column for enum '=' alignment
   // UO_align_struct_array_brace,   TODO: align array of structure initializers

   // group: UG_comment, "Comment modifications"                                                    8
   UO_cmt_width,                   // column to wrap comments
   UO_cmt_reflow_mode,             // comment reflow style
   UO_cmt_convert_tab_to_spaces,   //
   UO_cmt_indent_multi,            // change left indent of multiline comments
   UO_cmt_c_group,                 // try to group neighboring C comments
   UO_cmt_c_nl_start,              // put a blank /* at the start of a combined group
   UO_cmt_c_nl_end,                // put a newline before the */ in a combined group
   UO_cmt_cpp_group,               // if UO_cmt_cpp_to_c, try to group in one big C comment
   UO_cmt_cpp_nl_start,            // put a blank /* at the start of a converted group
   UO_cmt_cpp_nl_end,              // put a newline before the */ in a converted group
   UO_cmt_cpp_to_c,                // convert CPP comments to C comments
   UO_cmt_star_cont,               // put a star on subsequent comment lines
   UO_cmt_sp_before_star_cont,     // # of spaces for subsequent comment lines (before possible star)
   UO_cmt_sp_after_star_cont,      // # of spaces for subsequent comment lines (after star)
   UO_cmt_multi_check_last,        // no space after '*' prefix when comment start and end are of equal length
   UO_cmt_multi_first_len_minimum, // controls the xtra_indent for the last line of a multi-line comment
                                   // For multi-line comments with a '*' lead, remove leading spaces if the
                                   // first and last lines of the comment are the same length AND if the
                                   // length is bigger as the first_len minimum.
                                   // Default=4
   UO_cmt_insert_file_header,      //
   UO_cmt_insert_file_footer,      //
   UO_cmt_insert_func_header,      //
   UO_cmt_insert_class_header,     //
   UO_cmt_insert_oc_msg_header,    //
   UO_cmt_insert_before_preproc,   //
   UO_cmt_insert_before_inlines,   //
   UO_cmt_insert_before_ctor_dtor, //

   // group: UG_codemodify, "Code modifying options (non-whitespace)"                               9
   UO_mod_full_brace_do,                         // add or remove braces on single-line do
   UO_mod_full_brace_for,                        // add or remove braces on single-line for
   UO_mod_full_brace_function,                   // add optional braces on Pawn functions
   UO_mod_full_brace_if,                         // add or remove braces on single-line if
   UO_mod_full_brace_if_chain,                   // make all if/elseif/else statements in a chain
                                                 // be braced or not
   UO_mod_full_brace_if_chain_only,              // make all if/elseif/else statements in a chain with at least
                                                 // one 'else' or 'else if' fully braced
   UO_mod_full_brace_nl,                         // max number of newlines to span w/o braces
   UO_mod_full_brace_nl_block_rem_mlcond,        // block brace removal on multiline condition
   UO_mod_full_brace_while,                      // add or remove braces on single-line while
   UO_mod_full_brace_using,                      // add or remove braces on using
   UO_mod_paren_on_return,                       // add or remove paren on return
   UO_mod_pawn_semicolon,                        // add optional semicolons
   UO_mod_full_paren_if_bool,                    //
   UO_mod_remove_extra_semicolon,                // remove extra semicolons
   UO_mod_add_long_function_closebrace_comment,  //
   UO_mod_add_long_namespace_closebrace_comment, //
   UO_mod_add_long_class_closebrace_comment,     //
   UO_mod_add_long_switch_closebrace_comment,    //
   UO_mod_add_long_ifdef_endif_comment,          //
   UO_mod_add_long_ifdef_else_comment,           //
   UO_mod_sort_import,                           //
   UO_mod_sort_using,                            //
   UO_mod_sort_include,                          //
   UO_mod_move_case_break,                       //
   UO_mod_case_brace,                            //
   UO_mod_remove_empty_return,                   //
   UO_mod_sort_oc_properties,                    // organizes objective c properties
   //  Sorting options for objc properties
   UO_mod_sort_oc_property_class_weight,         // Determines weight of class
   UO_mod_sort_oc_property_thread_safe_weight,   // Determines weight of atomic/nonatomic
   UO_mod_sort_oc_property_readwrite_weight,     // Determines weight of readwrite
   UO_mod_sort_oc_property_reference_weight,     // Determines weight of reference type
                                                 // (retain, copy, assign, weak, strong)
   UO_mod_sort_oc_property_getter_weight,        // Determines weight of getter type (getter=)
   UO_mod_sort_oc_property_setter_weight,        // Determines weight of setter type (setter=)
   UO_mod_sort_oc_property_nullability_weight,   // Determines weight of nullability type (nullable/nonnull)


   // group: UG_preprocessor, "Preprocessor options"                                                10
   UO_pp_indent,             // indent preproc 1 space per level (add/ignore/remove)
   UO_pp_indent_at_level,    // indent #if, #else, #endif at brace level
   UO_pp_indent_count,       //
   UO_pp_space,              // spaces between # and word (add/ignore/remove)
   UO_pp_space_count,        // the number of spaces for add/force
   UO_pp_indent_region,      // indent of #region and #endregion, see indent_label
   UO_pp_region_indent_code, // whether to indent the code inside region stuff
   UO_pp_indent_if,          //
   UO_pp_if_indent_code,     //
   UO_pp_define_at_level,    // indent #define at brace level
   UO_pp_ignore_define_body, // "Whether to ignore the '#define' body while formatting."
   UO_pp_indent_case,        // Whether to indent case statements between #if, #else, and #endif
                             // Only applies to the indent of the preprocesser that the case statements directly inside of
   UO_pp_indent_func_def,    // Whether to indent whole function definitions between #if, #else, and #endif
                             // Only applies to the indent of the preprocesser that the function definition is directly inside of
   UO_pp_indent_extern,      // Whether to indent extern C blocks between #if, #else, and #endif
                             // Only applies to the indent of the preprocesser that the extern block is directly inside of
   UO_pp_indent_brace,       // Whether to indent braces directly inside #if, #else, and #endif
                             // Only applies to the indent of the preprocesser that the braces are directly inside of

   // group: UG_sort_includes, "Sort includes options"                                              11
   UO_include_category_0,  //
   UO_include_category_1,  //
   UO_include_category_2,  //

   // group: UG_Use_Ext, "Use or Do not Use options", "G"                                           12
   UO_use_indent_func_call_param,           // use/don't use indent_func_call_param Guy 2015-09-24
   UO_use_indent_continue_only_once,        // The value of the indentation for a continuation line is calculate
                                            // differently if the line is:
                                            //   a declaration :your case with QString fileName ...
                                            //   an assignment  :your case with pSettings = new QSettings( ...
                                            // At the second case the option value might be used twice:
                                            //   at the assignment
                                            //   at the function call (if present)
                                            // To prevent the double use of the option value, use this option
                                            // with the value "true". Guy 2016-05-16
   UO_use_options_overriding_for_qt_macros, // SIGNAL/SLOT Qt macros have special formatting options.
                                            // See options_for_QT.cpp for details.

   // group: UG_warnlevels, "Warn levels - 1: error, 2: warning (default), 3: note"                 13
   // Levels to attach to warnings (log_sev_t; default : LWARN)
   UO_warn_level_tabs_found_in_verbatim_string_literals, // if UO_string_replace_tab_chars is set,
                                                         // then we should warn about cases we can't
                                                         // do the replacement


   //UO_dont_protect_xcode_code_placeholders,

   // This is used to get the enumeration count
   UO_option_count
};

// for helping by sort
#define   UO_include_category_first    UO_include_category_0
#define   UO_include_category_last     UO_include_category_2


#ifdef EMSCRIPTEN
#define group_map_value_options_t    std::vector<uncrustify_options>
#else
#define group_map_value_options_t    std::list<uncrustify_options>
#endif

struct group_map_value
{
   uncrustify_groups         id;
   const char                *short_desc;
   const char                *long_desc;
   group_map_value_options_t options;
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


const char *get_argtype_name(argtype_e argtype);


/**
 * @brief defines a new group of uncrustify options
 *
 * The current group is stored as global variable which
 * will be used whenever a new option is added.
 */
void unc_begin_group(uncrustify_groups id, const char *short_desc, const char *long_desc = NULL);


const option_map_value *unc_find_option(const char *name);


//! Add all uncrustify options to the global option list
void register_options(void);


/**
 * Sets non-zero settings defaults
 *
 * TODO: select from various sets? - i.e., K&R, GNU, Linux, Ben
 */
void set_option_defaults(void);


/**
 * processes a single line string to extract configuration settings
 * increments cpd.line_number and cpd.error_count, modifies configLine parameter
 *
 * @param configLine  single line string that will be processed
 * @param filename    for log messages, file from which the configLine param
 *                    was extracted
 */
void process_option_line(char *configLine, const char *filename);


int load_option_file(const char *filename);


int save_option_file(FILE *pfile, bool withDoc);


/**
 * save the used options into a text file
 *
 * @param pfile             file to print into
 * @param withDoc           also print description
 * @param only_not_default  print only options with non default value
 */
int save_option_file_kernel(FILE *pfile, bool withDoc, bool only_not_default);


/**
 * @return >= 0  entry was found
 * @return   -1  entry was not found
 */
int set_option_value(const char *name, const char *value);


/**
 * check if a path/filename uses a relative or absolute path
 *
 * @retval false path is an absolute one
 * @retval true  path is a  relative one
 */
bool is_path_relative(const char *path);


const group_map_value *get_group_name(size_t ug);


const option_map_value *get_option_name(uncrustify_options uo);


void print_options(FILE *pfile);

/**
 * convert a argument type to a string
 *
 * @param val  argument type to convert
 */
std::string argtype_to_string(argtype_e argtype);

/**
 * convert a boolean to a string
 *
 * @param val  boolean to convert
 */
std::string bool_to_string(bool val);

/**
 * convert an argument value to a string
 *
 * @param val  argument value to convert
 */
std::string argval_to_string(argval_t argval);

/**
 * convert an integer number to a string
 *
 * @param val  integer number to convert
 */
std::string number_to_string(int number);

/**
 * convert a line ending type to a string
 *
 * @param val  line ending type to convert
 */
std::string lineends_to_string(lineends_e linends);

/**
 * convert a token to a string
 *
 * @param val  token to convert
 */
std::string tokenpos_to_string(tokenpos_e tokenpos);

/**
 * convert an argument of a given type to a string
 *
 * @param argtype   type of argument
 * @param op_val_t  value of argument
 */
std::string op_val_to_string(argtype_e argtype, op_val_t op_val);


typedef std::map<uncrustify_options, option_map_value>::iterator   option_name_map_it;
typedef std::map<uncrustify_groups, group_map_value>::iterator     group_map_it;
typedef group_map_value_options_t::iterator                        option_list_it;
typedef group_map_value_options_t::const_iterator                  option_list_cit;


#endif /* OPTIONS_H_INCLUDED */
