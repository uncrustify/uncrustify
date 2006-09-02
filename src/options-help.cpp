/**
 * @file options-help.cpp
 * Help strings for all the options.
 *
 * $Id: options.h 481 2006-09-02 16:32:20Z bengardner $
 */

#include "options.h"

/**
 * Prints out the help for an option
 *
 * @param out  The stream
 * @param op   The option
 */
const char *detailed_help(uncrustify_options op)
{
   const CHAR *text = NULL;

   switch (op)
   {
   case UO_input_tab_size:
      text = "Sets the size of the tab on input. Usually 8.";
      break;

   case UO_output_tab_size:
      text = "Sets the output tab size.\n"
             "Only used if using tabs for aligning. Usually 8..";
      break;

   case UO_indent_columns:
      text = "The number of columns to indent per level.\n"
             " Usually 2, 3, 4, or 8.";
      break;

   case UO_indent_with_tabs:
      text = "0=Spaces only.\n"
             "1=Indent with tabs, align with spaces.\n"
             "2=indent and align with tabs.";
      break;

      /* skipped a few */

   case UO_sp_paren_brace:
      text = "Space between ')' and '{'";
      break;

   case UO_sp_fparen_brace:
      text = "Space between ')' and '{' of function";
      break;

   case UO_sp_sparen_brace:
      text = "Space between ')' and '{' of if, while, etc";
      break;

      /* skipped a few */
   case UO_code_width:
      text = "Try to limit code width to this number of columns";
      break;

   case UO_indent_paren_nl:               // indent-align under paren for open followed by nl
   case UO_indent_square_nl:              // indent-align under square for open followed by nl
   case UO_pp_indent:                     // indent preproc 1 space per level (add/ignore/remove)
   case UO_pp_space:                      // spaces between # and word (add/ignore/remove)
   case UO_indent_switch_case:            // spaces to indent case from switch
   case UO_indent_case_body:              // spaces to indent case body from case
   case UO_indent_case_brace:             // spaces to indent '{' from case (usually 0 or indent_columns)
   case UO_indent_brace:                  // spaces to indent '{' from level (usually 0)
   case UO_indent_braces:                 // whether to indent the braces or not
   case UO_indent_label:                  // 0=left >0=col from left, <0=sub from brace indent
   case UO_indent_align_string:           // True/False - indent align broken strings
   case UO_indent_col1_comment:           // indent comments in column 1
   case UO_indent_func_call_param:        // indent continued function calls to indent_columns
   case UO_indent_namespace:              // indent stuff inside namespace braces
   case UO_indent_class:                  // indent stuff inside class braces
   case UO_indent_class_colon:            // indent stuff after a class colon
   case UO_sp_after_cast:                 // space after cast - "(int) a" vs "(int)a"
   case UO_sp_before_byref:               // space before '&' of 'fcn(int& idx)'
   case UO_sp_after_byref:                // space after a '&'  as in 'int& var'
   case UO_sp_inside_fparen:              // space inside 'foo( xxx )' vs 'foo(xxx)'
   case UO_sp_inside_fparens:             // space inside 'foo( )' vs 'foo()'
   case UO_sp_inside_paren:               // space inside '+ ( xxx )' vs '+ (xxx)'
   case UO_sp_inside_square:              // space inside 'byte[ 5 ]' vs 'byte[5]'
   case UO_sp_inside_sparen:              // space inside 'if( xxx )' vs 'if(xxx)'
   case UO_sp_inside_angle:               // space inside '<>', as in '<class T>'
   case UO_sp_before_sparen:              // space before '(' of 'if/for/while/switch'
   case UO_sp_after_sparen:               // space after  ')' of 'if/for/while/switch'
   case UO_sp_before_angle:               // space before '<>', as in '<class T>'
   case UO_sp_after_angle:                // space after  '<>', as in '<class T>'
   case UO_sp_before_square:              // space before single '['
   case UO_sp_before_squares:             // space before '[]', as in 'byte []'
   case UO_sp_paren_paren:                // space between nested parens - '( (' vs '(('
   case UO_sp_return_paren:               // space between 'return' and '('
   case UO_sp_sizeof_paren:               // space between 'sizeof' and '('
   case UO_sp_after_comma:                // space after ','
   case UO_sp_arith:                      // space around + - / * etc
   case UO_sp_bool:                       // space around || &&
   case UO_sp_compare:                    // space around < > ==, etc
   case UO_sp_assign:                     // space around =, +=, etc
   case UO_sp_func_def_paren:             // space between 'func' and '(' - "foo (" vs "foo("
   case UO_sp_func_call_paren:            // space between 'func' and '(' - "foo (" vs "foo("
   case UO_sp_func_proto_paren:           // space between 'func' and '(' - "foo (" vs "foo("
   case UO_sp_func_class_paren:           // space between ctor/dtor and '('
   case UO_sp_type_func:                  // space between return type and 'func'
   case UO_sp_before_ptr_star:            // space before a '*' that is part of a type
   case UO_sp_after_ptr_star:             // space after a '*' that is part of a type
   case UO_sp_between_ptr_star:           // space between two '*' that are part of a type
   case UO_sp_special_semi:               // space empty stmt ';' on while, if, for
   case UO_sp_before_semi:                // space before all ';'
   case UO_sp_inside_braces:              // space inside '{' and '}' - "{ 1, 2, 3 }"
   case UO_sp_inside_braces_enum:         // space inside enum '{' and '}' - "{ a, b, c }"
   case UO_sp_inside_braces_struct:       // space inside struct/union '{' and '}'
   case UO_sp_macro:                      // space between macro and value, ie '#define a 6'
   case UO_sp_macro_func:                 // space between macro and value, ie '#define a 6'
   case UO_sp_square_fparen:              // weird pawn stuff: native yark[rect](a[rect])
   case UO_sp_after_tag:                  // pawn: space after a tag colon
   case UO_sp_after_operator:             // space after operator when followed by a punctuator
   case UO_align_with_tabs:               // use tabs for aligning (0/1)
   case UO_align_keep_tabs:               // keep non-indenting tabs
   case UO_align_on_tabstop:              // always align on tabstops
   case UO_align_nl_cont:                 // align the back-slash \n combo (macros)
   case UO_align_enum_equ:                // align the '=' in enums
   case UO_align_assign_span:             // align on '='. 0=don't align
   case UO_align_assign_thresh:           // threshold for aligning on '='. 0=no limit
   case UO_align_right_cmt_span:          // align comment that end lines. 0=don't align
   case UO_align_var_def_span:            // align variable defs on variable (span for regular stuff)
   case UO_align_var_def_thresh:          // align variable defs threshold
   case UO_align_var_def_inline:          // also align inline struct/enum/union var defs
   case UO_align_var_def_star:            // the star is part of the variable name
   case UO_align_var_def_colon:           // align the colon in struct bit fields
   case UO_align_var_struct_span:         // span for struct/union (0=don't align)
   case UO_align_pp_define_span:          // align bodies in #define statments
   case UO_align_pp_define_gap:           // min space between define label and value "#define a <---> 16"
   case UO_align_struct_init_span:        // align structure initializer values
   case UO_align_func_proto_span:         // align function prototypes
   case UO_align_number_left:             // left-align numbers (not fully supported, yet)
   case UO_align_typedef_span:            // align single-line typedefs
   case UO_align_typedef_gap:             // minimum spacing
   case UO_align_typedef_star_style:      // Start aligning style
   case UO_nl_fdef_brace:                 // "int foo() {" vs "int foo()\n{"
   case UO_nl_func_decl_start:            // newline after the '(' in a function decl
   case UO_nl_func_decl_args:             // newline after each ',' in a function decl
   case UO_nl_func_decl_end:              // newline before the ')' in a function decl
   case UO_nl_func_type_name:             // newline between return type and func name in def
   case UO_nl_func_var_def_blk:           // newline after a block of variable defs
   case UO_nl_before_case:                // newline before 'case' statement
   case UO_nl_after_return:               /* newline after return statement */
   case UO_nl_after_case:                 /* disallow nested "case 1: a=3;" */
   case UO_nl_fcall_brace:                /* newline between function call and open brace */
   case UO_nl_squeeze_ifdef:              /* no blanks after #ifxx, #elxx, or before #endif */
   case UO_nl_enum_brace:                 /* nl between enum and brace */
   case UO_nl_struct_brace:               /* nl between struct and brace */
   case UO_nl_union_brace:                /* nl between union and brace */
   case UO_nl_assign_brace:               /* nl between '=' and brace */
   case UO_nl_class_brace:                /* nl between class name and brace */
   case UO_nl_namespace_brace:            /* nl between namespace name and brace */
   case UO_nl_do_brace:                   /* nl between do and { */
   case UO_nl_if_brace:                   /* nl between if and { */
   case UO_nl_for_brace:                  /* nl between for and { */
   case UO_nl_else_brace:                 /* nl between else and { */
   case UO_nl_while_brace:                /* nl between while and { */
   case UO_nl_switch_brace:               /* nl between switch and { */
   case UO_nl_brace_else:                 // nl between } and else
   case UO_nl_brace_while:                // nl between } and while of do stmt
   case UO_nl_elseif_brace:               // nl between close paren and open brace in 'else if () {'
   case UO_nl_define_macro:               // alter newlines in #define macros
   case UO_nl_start_of_file:              // alter newlines at the start of file
   case UO_nl_start_of_file_min:          // min number of newlines at the start of the file
   case UO_nl_end_of_file:                // alter newlines at the end of file
   case UO_nl_end_of_file_min:            // min number of newlines at the end of the file
   case UO_nl_class_init_args:            // newline after comma in class init args
   case UO_nl_collapse_empty_body:        // change { \n } into {}
   case UO_nl_template_class:             // newline between '>' and class in "template <x> class"
   case UO_pos_bool:                      // position of trailing/leading &&/||
   case UO_pos_class_colon:               // position of trailing/leading class colon
   case UO_nl_before_block_comment:       // before a block comment (stand-alone comment-multi), except after brace open
   case UO_nl_after_func_body:            // after the closing brace of a function body
   case UO_nl_after_func_proto:           // after each prototype
   case UO_nl_after_func_proto_group:     // after a block of prototypes
   case UO_nl_max:                        // maximum consecutive newlines (3 = 2 blank lines)
   case UO_eat_blanks_after_open_brace:   // remove blank lines after {
   case UO_eat_blanks_before_close_brace: // remove blank lines before }
   case UO_mod_paren_on_return:           // add or remove paren on return
   case UO_mod_full_brace_nl:             // max number of newlines to span w/o braces
   case UO_mod_full_brace_if:             // add or remove braces on if
   case UO_mod_full_brace_for:            // add or remove braces on for
   case UO_mod_full_brace_do:             // add or remove braces on do
   case UO_mod_full_brace_while:          // add or remove braces on while
   case UO_mod_pawn_semicolon:            // add optional semicolons
   case UO_mod_full_brace_function:       // add optional braces on Pawn functions
   case UO_cmt_star_cont:                 // put a star on subsequent comment lines
   case UO_cmt_cpp_to_c:                  // convert CPP comments to C comments
   case UO_cmt_cpp_group:                 // if UO_cmt_cpp_to_c, try to group in one big C comment
   case UO_cmt_cpp_nl_start:              // put a blank /* at the start of a converted group
   case UO_cmt_cpp_nl_end:                // put a nl before the */ in a converted group
   case UO_string_escape_char:            // the string escape char to use
   default:
      text = "Not yet documented.";
      break;
   }

   return(text);
}
