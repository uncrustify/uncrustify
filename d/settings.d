/**
 * @file settings.d
 * Settings for all the options.
 *
 * $Id$
 */

module uncrustify.settings;

import std.string;


enum ArgType
{
   AT_BOOL,    /**< TRUE / FALSE */
   AT_IARF,    /**< Ignore / Add / Remove / Force */
   AT_NUM,     /**< Number */
};

/** Arg values - these are bit fields*/
enum ArgVal
{
   AV_IGNORE = 0,
   AV_ADD    = 1,
   AV_REMOVE = 2,
   AV_FORCE  = 3, /**< remove + add */
};


/**
 * Keep this grouped by functionality
 */
enum Option
{
   /*
    * Basic Indenting stuff
    */
   UO_indent,                   //TODO: 0=don't change indentation, 1=change indentation
   UO_input_tab_size,           // tab size on input file: usually 8
   UO_output_tab_size,          // tab size for output: usually 8
   UO_indent_columns,           // ie 3 or 8
   UO_indent_with_tabs,         // 1=only to the 'level' indent, 2=use tabs for indenting
   UO_indent_brace_struct,      //TODO: spaces to indent brace after struct/enum/union def
   UO_indent_paren,             //TODO: indent for open paren on next line (1)
   UO_indent_paren_nl,          // indent-align under paren for open followed by nl
   UO_leave_preproc_space,      //TODO: if true, leave the spaces between '#' and preprocessor commands
   UO_pp_indent,                //TODO: spaces to indent preprocessors (0)
   UO_indent_switch_case,       // spaces to indent case from switch
   UO_indent_case_brace,        //TODO: spaces to indent '{' from case (usually 0 or indent_columns)
   UO_indent_brace,             // spaces to indent '{' from level (usually 0)
   UO_indent_label,             // 0=left >0=col from left, <0=sub from brace indent
   UO_indent_align_string,      // True/False - indent align broken strings
   UO_indent_col1_comment,      // indent comments in column 1
   UO_indent_func_call_param,   // indent continued function calls to indent_columns

   /*
    * Misc inter-element spacing
    */

   UO_sp_before_sparen,     // space before '(' of 'if/for/while/switch'
   UO_sp_after_sparen,      /* space after  ')' of 'if/for/while/switch'
                                * the do-while does not get set here */
   UO_sp_paren_brace,       // space between ')' and '{'

   UO_sp_after_cast,        // space after cast - "(int) a" vs "(int)a"

   UO_sp_inside_sparen,     // space inside 'if( xxx )' vs 'if(xxx)'
   UO_sp_inside_fparen,     // space inside 'foo( xxx )' vs 'foo(xxx)'
   UO_sp_inside_paren,      // space inside '+ ( xxx )' vs '+ (xxx)'
   UO_sp_inside_square,     // space inside 'byte[ 5 ]' vs 'byte[5]'
   UO_sp_before_square,     // space before single '['
   UO_sp_before_squares,    // space before '[]', as in 'byte []'

   UO_sp_paren_paren,       // space between nested parens - '( (' vs '(('

   UO_sp_return_paren,      // space between 'return' and '('
   UO_sp_sizeof_paren,      // space between 'sizeof' and '('
     _
   UO_sp_after_comma,       // space after ','

   UO_sp_arith,             // space around + - / * etc
   UO_sp_bool,              // space around || &&
   UO_sp_compare,           // space around < > ==, etc
   UO_sp_assign,            // space around =, +=, etc

   UO_sp_func_def_paren,    // space between 'func' and '(' - "foo (" vs "foo("
   UO_sp_func_call_paren,   // space between 'func' and '(' - "foo (" vs "foo("
   UO_sp_func_proto_paren,  // space between 'func' and '(' - "foo (" vs "foo("

   UO_sp_type_func,         // space between return type and 'func'
                               // a minimum of 1 is forced except for '*'

   UO_sp_special_semi,      /* space empty stmt ';' on while, if, for
                                * example "while (*p++ = ' ') ;" */
   UO_sp_before_semi,          // space before all ';'
   UO_sp_inside_braces,        // space inside '{' and '}' - "{ 1, 2, 3 }"
   UO_sp_inside_braces_enum,   // space inside enum '{' and '}' - "{ a, b, c }"
   UO_sp_inside_braces_struct, // space inside struct/union '{' and '}'


   /*
    * Line splitting options (for long lines)
    */

   UO_code_width,           //TODO: ie 80 columns
   UO_ls_before_bool_op,    //TODO: break line before of after boolean op
   UO_ls_before_paren,      //TODO: break before open paren
   UO_ls_after_arith,       //TODO: break after arith op '+', etc
   UO_ls_honor_newlines,    //TODO: don't remove newlines on split lines


   /*
    * code alignment (not left column spaces/tabs)
    */

   UO_align_with_tabs,            // use tabs for aligning (0/1)
   UO_align_on_tabstop,           // always align on tabstops
   UO_align_nl_cont,              // align the back-slash \n combo (macros)
   UO_align_enum_equ,             // align the '=' in enums
   UO_align_assign_span,          // align on '='. 0=don't align
   UO_align_right_cmt_span,       // align comment that end lines. 0=don't align
   UO_align_var_def_span,         // align variable defs on variable (span for regular stuff)
   UO_align_var_def_inline,       // also align inline struct/enum/union var defs
   UO_align_var_def_star,         // the star is part of the variable name
   UO_align_var_def_colon,        // align the colon in struct bit fields
   UO_align_var_struct_span,      // span for struct/union (0=don't align)
   UO_align_pp_define_span,       // align bodies in #define statments
   UO_align_pp_define_col_min,    //TODO: min column for a #define value
   UO_align_pp_define_col_max,    //TODO: max column for a #define value
   UO_align_pp_define_gap,        // min space between define label and value "#define a <---> 16"
   UO_align_enum_col_min,         //TODO: the min column for enum '=' alignment
   UO_align_enum_col_max,         //TODO: the max column for enum '=' alignment
   UO_align_struct_init_span,     // align structure initializer values
   UO_align_func_proto_span,      // align function prototypes
   UO_align_number_left,          // left-align numbers (not fully supported, yet)
   UO_align_typedef_span,         // align single-line typedefs
   UO_align_typedef_gap,          // minimum spacing
   UO_align_keep_tabs,            // keep non-indenting tabs


   /*
    * Newline adding and removing options
    */

   UO_nl_fdef_brace,             // "int foo() {" vs "int foo()\n{"
   UO_nl_func_decl_args,         //TODO: newline after each ',' in a function decl
   UO_nl_func_decl_end,          //TODO: newline before the ')' in a function decl
   UO_nl_func_type_name,         //TODO: newline between return type and func name in def
   UO_nl_func_var_def_blk,       // newline after a block of variable defs
   UO_nl_before_case,            // newline before 'case' statement
   UO_nl_after_return,           /* newline after return statement */
   UO_nl_after_case,             /* disallow nested "case 1: a=3;" */
   UO_nl_fcall_brace,            /* newline between function call and open brace */
   UO_nl_squeeze_ifdef,          /* no blanks after #ifxx, #elxx, or before #endif */
   UO_nl_enum_brace,             /* nl between enum and brace */
   UO_nl_struct_brace,           /* nl between struct and brace */
   UO_nl_union_brace,            /* nl between union and brace */
   UO_nl_assign_brace,           /* nl between '=' and brace */

   UO_nl_do_brace,               /* nl between do and { */
   UO_nl_if_brace,               /* nl between if and { */
   UO_nl_for_brace,              /* nl between for and { */
   UO_nl_else_brace,             /* nl between else and { */
   UO_nl_while_brace,            /* nl between while and { */
   UO_nl_switch_brace,           /* nl between switch and { */
   UO_nl_brace_else,             // nl between } and else
   UO_nl_brace_while,            // nl between } and while of do stmt

   UO_nl_define_macro,           // alter newlines in #define macros


   /*
    * Blank line options
    */

   UO_blc_before_block_comment,      //TODO: before a block comment (stand-alone comment-multi)
   UO_blc_after_func_body,           //TODO: after the closing brace of a function body
   UO_blc_after_func_proto,          //TODO: after each prototype
   UO_blc_after_func_proto_group,    //TODO: after a block of prototypes
   UO_blc_after_var_def_group,       //TODO: after a group of variable defs at top of proc
   UO_blc_after_ifdef,               //TODO: after #if or #ifdef - but not if covers whole file
   UO_blc_max,                       //TODO: maximum consecutive newlines (3 is good)

   UO_eat_blanks_after_open_brace,   // remove blank lines after {
   UO_eat_blanks_before_close_brace, // remove blank lines before }


   /*
    * code modifying options (non-whitespace)
    */

   UO_mod_paren_on_return,       // add or remove paren on return
   UO_mod_full_brace_nl,         // max number of newlines to span w/o braces
   UO_mod_full_brace_if,         // add or remove braces on if
   UO_mod_full_brace_for,        // add or remove braces on for
   UO_mod_full_brace_do,         // add or remove braces on do
   UO_mod_full_brace_while,      // add or remove braces on while


   /*
    * Comment modifications
    */

   UO_cmt_star_cont,       // put a star on subsequent comment lines


   /* This is used to get the enumeration count */
   UO_option_count
};

struct OptionEntry
{
   Option  id;
   char [] name;
   ArgType type;
};

/* Keep this table sorted! */
static OptionEntry [] options_table =
[
   { Option.UO_align_assign_span,             "align_assign_span",             ArgType.AT_NUM  },
   { Option.UO_align_enum_col_max,            "align_enum_col_max",            ArgType.AT_NUM  },
   { Option.UO_align_enum_col_min,            "align_enum_col_min",            ArgType.AT_NUM  },
   { Option.UO_align_enum_equ,                "align_enum_equ",                ArgType.AT_NUM  },
   { Option.UO_align_func_proto_span,         "align_func_proto_span",         ArgType.AT_NUM  },
   { Option.UO_align_keep_tabs,               "align_keep_tabs",               ArgType.AT_BOOL },
   { Option.UO_align_nl_cont,                 "align_nl_cont",                 ArgType.AT_BOOL },
   { Option.UO_align_number_left,             "align_number_left",             ArgType.AT_BOOL },
   { Option.UO_align_on_tabstop,              "align_on_tabstop",              ArgType.AT_BOOL },
   { Option.UO_align_pp_define_col_max,       "align_pp_define_col_max",       ArgType.AT_NUM  },
   { Option.UO_align_pp_define_col_min,       "align_pp_define_col_min",       ArgType.AT_NUM  },
   { Option.UO_align_pp_define_gap,           "align_pp_define_gap",           ArgType.AT_NUM  },
   { Option.UO_align_pp_define_span,          "align_pp_define_span",          ArgType.AT_NUM  },
   { Option.UO_align_right_cmt_span,          "align_right_cmt_span",          ArgType.AT_NUM  },
   { Option.UO_align_struct_init_span,        "align_struct_init_span",        ArgType.AT_NUM  },
   { Option.UO_align_typedef_gap,             "align_typedef_gap",             ArgType.AT_NUM  },
   { Option.UO_align_typedef_span,            "align_typedef_span",            ArgType.AT_NUM  },
   { Option.UO_align_var_def_colon,           "align_var_def_colon",           ArgType.AT_BOOL },
   { Option.UO_align_var_def_inline,          "align_var_def_inline",          ArgType.AT_BOOL },
   { Option.UO_align_var_def_span,            "align_var_def_span",            ArgType.AT_NUM  },
   { Option.UO_align_var_def_star,            "align_var_def_star",            ArgType.AT_BOOL },
   { Option.UO_align_var_struct_span,         "align_var_struct_span",         ArgType.AT_NUM  },
   { Option.UO_align_with_tabs,               "align_with_tabs",               ArgType.AT_BOOL },
   { Option.UO_blc_after_func_body,           "blc_after_func_body",           ArgType.AT_NUM  },
   { Option.UO_blc_after_func_proto,          "blc_after_func_proto",          ArgType.AT_NUM  },
   { Option.UO_blc_after_func_proto_group,    "blc_after_func_proto_group",    ArgType.AT_NUM  },
   { Option.UO_blc_after_ifdef,               "blc_after_ifdef",               ArgType.AT_NUM  },
   { Option.UO_blc_after_var_def_group,       "blc_after_var_def_group",       ArgType.AT_NUM  },
   { Option.UO_blc_before_block_comment,      "blc_before_block_comment",      ArgType.AT_NUM  },
   { Option.UO_blc_max,                       "blc_max",                       ArgType.AT_NUM  },
   { Option.UO_cmt_star_cont,                 "cmt_star_cont",                 ArgType.AT_BOOL },
   { Option.UO_code_width,                    "code_width",                    ArgType.AT_NUM  },
   { Option.UO_eat_blanks_after_open_brace,   "eat_blanks_after_open_brace",   ArgType.AT_BOOL },
   { Option.UO_eat_blanks_before_close_brace, "eat_blanks_before_close_brace", ArgType.AT_BOOL },
   { Option.UO_indent,                        "indent",                        ArgType.AT_BOOL },
   { Option.UO_indent_align_string,           "indent_align_string",           ArgType.AT_BOOL },
   { Option.UO_indent_brace,                  "indent_brace",                  ArgType.AT_NUM  },
   { Option.UO_indent_brace_struct,           "indent_brace_struct",           ArgType.AT_NUM  },
   { Option.UO_indent_case_brace,             "indent_case_brace",             ArgType.AT_NUM  },
   { Option.UO_indent_col1_comment,           "indent_col1_comment",           ArgType.AT_BOOL },
   { Option.UO_indent_columns,                "indent_columns",                ArgType.AT_NUM  },
   { Option.UO_indent_func_call_param,        "indent_func_call_param",        ArgType.AT_BOOL },
   { Option.UO_indent_label,                  "indent_label",                  ArgType.AT_NUM  },
   { Option.UO_indent_paren,                  "indent_paren",                  ArgType.AT_NUM  },
   { Option.UO_indent_paren_nl,               "indent_paren_nl",               ArgType.AT_BOOL },
   { Option.UO_indent_switch_case,            "indent_switch_case",            ArgType.AT_NUM  },
   { Option.UO_indent_with_tabs,              "indent_with_tabs",              ArgType.AT_NUM  },
   { Option.UO_input_tab_size,                "input_tab_size",                ArgType.AT_NUM  },
   { Option.UO_leave_preproc_space,           "leave_preproc_space",           ArgType.AT_BOOL },
   { Option.UO_ls_after_arith,                "ls_after_arith",                ArgType.AT_BOOL },
   { Option.UO_ls_before_bool_op,             "ls_before_bool_op",             ArgType.AT_BOOL },
   { Option.UO_ls_before_paren,               "ls_before_paren",               ArgType.AT_BOOL },
   { Option.UO_ls_honor_newlines,             "ls_honor_newlines",             ArgType.AT_BOOL },
   { Option.UO_mod_full_brace_do,             "mod_full_brace_do",             ArgType.AT_IARF },
   { Option.UO_mod_full_brace_for,            "mod_full_brace_for",            ArgType.AT_IARF },
   { Option.UO_mod_full_brace_if,             "mod_full_brace_if",             ArgType.AT_IARF },
   { Option.UO_mod_full_brace_nl,             "mod_full_brace_nl",             ArgType.AT_NUM  },
   { Option.UO_mod_full_brace_while,          "mod_full_brace_while",          ArgType.AT_IARF },
   { Option.UO_mod_paren_on_return,           "mod_paren_on_return",           ArgType.AT_IARF },
   { Option.UO_nl_after_case,                 "nl_after_case",                 ArgType.AT_BOOL },
   { Option.UO_nl_after_return,               "nl_after_return",               ArgType.AT_BOOL },
   { Option.UO_nl_assign_brace,               "nl_assign_brace",               ArgType.AT_IARF },
   { Option.UO_nl_before_case,                "nl_before_case",                ArgType.AT_BOOL },
   { Option.UO_nl_brace_else,                 "nl_brace_else",                 ArgType.AT_IARF },
   { Option.UO_nl_brace_while,                "nl_brace_while",                ArgType.AT_IARF },
   { Option.UO_nl_define_macro,               "nl_define_macro",               ArgType.AT_BOOL },
   { Option.UO_nl_do_brace,                   "nl_do_brace",                   ArgType.AT_IARF },
   { Option.UO_nl_else_brace,                 "nl_else_brace",                 ArgType.AT_IARF },
   { Option.UO_nl_enum_brace,                 "nl_enum_brace",                 ArgType.AT_IARF },
   { Option.UO_nl_fcall_brace,                "nl_fcall_brace",                ArgType.AT_IARF },
   { Option.UO_nl_fdef_brace,                 "nl_fdef_brace",                 ArgType.AT_IARF },
   { Option.UO_nl_for_brace,                  "nl_for_brace",                  ArgType.AT_IARF },
   { Option.UO_nl_func_decl_args,             "nl_func_decl_args",             ArgType.AT_IARF },
   { Option.UO_nl_func_decl_end,              "nl_func_decl_end",              ArgType.AT_IARF },
   { Option.UO_nl_func_type_name,             "nl_func_type_name",             ArgType.AT_IARF },
   { Option.UO_nl_func_var_def_blk,           "nl_func_var_def_blk",           ArgType.AT_NUM  },
   { Option.UO_nl_if_brace,                   "nl_if_brace",                   ArgType.AT_IARF },
   { Option.UO_nl_squeeze_ifdef,              "nl_squeeze_ifdef",              ArgType.AT_BOOL },
   { Option.UO_nl_struct_brace,               "nl_struct_brace",               ArgType.AT_IARF },
   { Option.UO_nl_switch_brace,               "nl_switch_brace",               ArgType.AT_IARF },
   { Option.UO_nl_union_brace,                "nl_union_brace",                ArgType.AT_IARF },
   { Option.UO_nl_while_brace,                "nl_while_brace",                ArgType.AT_IARF },
   { Option.UO_output_tab_size,               "output_tab_size",               ArgType.AT_NUM  },
   { Option.UO_pp_indent,                     "pp_indent",                     ArgType.AT_NUM  },
   { Option.UO_sp_after_cast,                 "sp_after_cast",                 ArgType.AT_IARF },
   { Option.UO_sp_after_comma,                "sp_after_comma",                ArgType.AT_IARF },
   { Option.UO_sp_after_sparen,               "sp_after_sparen",               ArgType.AT_IARF },
   { Option.UO_sp_arith,                      "sp_arith",                      ArgType.AT_IARF },
   { Option.UO_sp_assign,                     "sp_assign",                     ArgType.AT_IARF },
   { Option.UO_sp_before_semi,                "sp_before_semi",                ArgType.AT_IARF },
   { Option.UO_sp_before_sparen,              "sp_before_sparen",              ArgType.AT_IARF },
   { Option.UO_sp_before_square,              "sp_before_square",              ArgType.AT_IARF },
   { Option.UO_sp_before_squares,             "sp_before_squares",             ArgType.AT_IARF },
   { Option.UO_sp_bool,                       "sp_bool",                       ArgType.AT_IARF },
   { Option.UO_sp_compare,                    "sp_compare",                    ArgType.AT_IARF },
   { Option.UO_sp_func_call_paren,            "sp_func_call_paren",            ArgType.AT_IARF },
   { Option.UO_sp_func_def_paren,             "sp_func_def_paren",             ArgType.AT_IARF },
   { Option.UO_sp_func_proto_paren,           "sp_func_proto_paren",           ArgType.AT_IARF },
   { Option.UO_sp_inside_braces,              "sp_inside_braces",              ArgType.AT_IARF },
   { Option.UO_sp_inside_braces_enum,         "sp_inside_braces_enum",         ArgType.AT_IARF },
   { Option.UO_sp_inside_braces_struct,       "sp_inside_braces_struct",       ArgType.AT_IARF },
   { Option.UO_sp_inside_fparen,              "sp_inside_fparen",              ArgType.AT_IARF },
   { Option.UO_sp_inside_paren,               "sp_inside_paren",               ArgType.AT_IARF },
   { Option.UO_sp_inside_sparen,              "sp_inside_sparen",              ArgType.AT_IARF },
   { Option.UO_sp_inside_square,              "sp_inside_square",              ArgType.AT_IARF },
   { Option.UO_sp_paren_brace,                "sp_paren_brace",                ArgType.AT_IARF },
   { Option.UO_sp_paren_paren,                "sp_paren_paren",                ArgType.AT_IARF },
   { Option.UO_sp_return_paren,               "sp_return_paren",               ArgType.AT_IARF },
   { Option.UO_sp_sizeof_paren,               "sp_sizeof_paren",               ArgType.AT_IARF },
   { Option.UO_sp_special_semi,               "sp_special_semi",               ArgType.AT_IARF },
   { Option.UO_sp_type_func,                  "sp_type_func",                  ArgType.AT_IARF },
];


/**
 * Find the name that goes with the id.
 *
 * @param id   The id to find
 * @return     the name
 */
char [] GetOptionName(Option id)
{
   int idx;

   for (idx = 0; idx < options_table.length; idx++)
   {
      if (options_table[idx].id == id)
      {
         return(options_table[idx].name);
      }
   }
   return("???");
}


/**
 * Search the sorted name table to find a match
 *
 * @param name The name to find
 * @return     null or the entry that matched
 */
OptionEntry * FindOptionEntry(char [] name)
{
   OptionEntry [] oe = options_table;
   int result;
   int idx;

   while (oe.length > 1)
   {
      idx = oe.length / 2;

      result = cmp(name, oe[idx].name);
      if (result == 0)
      {
         return &oe[idx];
      }
      else if (result < 0)
      {
         oe = oe[0 .. idx];
      }
      else
      {
         oe = oe[idx .. $];
      }
   }
   if (std.string.cmp(oe[0].name, name) == 0)
   {
      return &oe[0];
   }
   return null;
}

