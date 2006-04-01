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
   BOOL,    /**< TRUE / FALSE */
   IARF,    /**< Ignore / Add / Remove / Force */
   NUM,     /**< Number */
};

/** Arg values - these are bit fields*/
enum ArgVal
{
   IGNORE = 0,
   ADD    = 1,
   REMOVE = 2,
   FORCE  = 3, /**< remove + add */
};


/**
 * Keep this grouped by functionality
 */
enum Option
{
   /*
    * Basic Indenting stuff
    */
   indent,                   //TODO: 0=don't change indentation, 1=change indentation

   input_tab_size,           // tab size on input file: usually 8
   output_tab_size,          // tab size for output: usually 8

   indent_columns,           // ie 3 or 8
   indent_with_tabs,         // 1=only to the 'level' indent, 2=use tabs for indenting
   indent_brace_struct,      //TODO: spaces to indent brace after struct/enum/union def
   indent_paren,             //TODO: indent for open paren on next line (1)
   indent_paren_nl,          // indent-align under paren for open followed by nl
   leave_preproc_space,      //TODO: if true, leave the spaces between '#' and preprocessor commands
   pp_indent,                //TODO: spaces to indent preprocessors (0)

   indent_switch_case,       // spaces to indent case from switch
   indent_case_body,         // spaces to indent case body from case
   indent_case_brace,        //TODO: spaces to indent '{' from case (usually 0 or indent_columns)

   indent_brace,             // spaces to indent '{' from level (usually 0)
   indent_label,             // 0=left >0=col from left, <0=sub from brace indent

   indent_align_string,      // True/False - indent align broken strings

   indent_col1_comment,      // indent comments in column 1

   indent_func_call_param,   // indent continued function calls to indent_columns

   /*
    * Misc inter-element spacing
    */

   sp_before_sparen,     // space before '(' of 'if/for/while/switch'
   sp_after_sparen,      /* space after  ')' of 'if/for/while/switch'
                             * the do-while does not get set here */
   sp_paren_brace,       // space between ')' and '{'

   sp_after_cast,        // space after cast - "(int) a" vs "(int)a"

   sp_inside_sparen,     // space inside 'if( xxx )' vs 'if(xxx)'
   sp_inside_fparen,     // space inside 'foo( xxx )' vs 'foo(xxx)'
   sp_inside_paren,      // space inside '+ ( xxx )' vs '+ (xxx)'
   sp_inside_square,     // space inside 'byte[ 5 ]' vs 'byte[5]'
   sp_before_square,     // space before single '['
   sp_before_squares,    // space before '[]', as in 'byte []'

   sp_paren_paren,       // space between nested parens - '( (' vs '(('

   sp_return_paren,      // space between 'return' and '('
   sp_sizeof_paren,      // space between 'sizeof' and '('

   sp_after_comma,       // space after ','

   sp_arith,             // space around + - / * etc
   sp_bool,              // space around || &&
   sp_compare,           // space around < > ==, etc
   sp_assign,            // space around =, +=, etc

   sp_func_def_paren,    // space between 'func' and '(' - "foo (" vs "foo("
   sp_func_call_paren,   // space between 'func' and '(' - "foo (" vs "foo("
   sp_func_proto_paren,  // space between 'func' and '(' - "foo (" vs "foo("

   sp_type_func,         // space between return type and 'func'
                            // a minimum of 1 is forced except for '*'

   sp_special_semi,      /* space empty stmt ';' on while, if, for
                             * example "while (*p++ = ' ') ;" */
   sp_before_semi,          // space before all ';'
   sp_inside_braces,        // space inside '{' and '}' - "{ 1, 2, 3 }"
   sp_inside_braces_enum,   // space inside enum '{' and '}' - "{ a, b, c }"
   sp_inside_braces_struct, // space inside struct/union '{' and '}'


   /*
    * Line splitting options (for long lines)
    */

   code_width,           //TODO: ie 80 columns
   ls_before_bool_op,    //TODO: break line before of after boolean op
   ls_before_paren,      //TODO: break before open paren
   ls_after_arith,       //TODO: break after arith op '+', etc
   ls_honor_newlines,    //TODO: don't remove newlines on split lines


   /*
    * code alignment (not left column spaces/tabs)
    */

   align_with_tabs,            // use tabs for aligning (0/1)
   align_on_tabstop,           // always align on tabstops
   align_nl_cont,              // align the back-slash \n combo (macros)
   align_enum_equ,             // align the '=' in enums
   align_assign_span,          // align on '='. 0=don't align
   align_right_cmt_span,       // align comment that end lines. 0=don't align
   align_var_def_span,         // align variable defs on variable (span for regular stuff)
   align_var_def_inline,       // also align inline struct/enum/union var defs
   align_var_def_star,         // the star is part of the variable name
   align_var_def_colon,        // align the colon in struct bit fields
   align_var_struct_span,      // span for struct/union (0=don't align)
   align_pp_define_span,       // align bodies in #define statments
   align_pp_define_col_min,    //TODO: min column for a #define value
   align_pp_define_col_max,    //TODO: max column for a #define value
   align_pp_define_gap,        // min space between define label and value "#define a <---> 16"
   align_enum_col_min,         //TODO: the min column for enum '=' alignment
   align_enum_col_max,         //TODO: the max column for enum '=' alignment
   align_struct_init_span,     // align structure initializer values
   align_func_proto_span,      // align function prototypes
   align_number_left,          // left-align numbers (not fully supported, yet)
   align_typedef_span,         // align single-line typedefs
   align_typedef_gap,          // minimum spacing
   align_keep_tabs,            // keep non-indenting tabs


   /*
    * Newline adding and removing options
    */

   nl_fdef_brace,             // "int foo() {" vs "int foo()\n{"
   nl_func_decl_args,         //TODO: newline after each ',' in a function decl
   nl_func_decl_end,          //TODO: newline before the ')' in a function decl
   nl_func_type_name,         //TODO: newline between return type and func name in def
   nl_func_var_def_blk,       // newline after a block of variable defs
   nl_before_case,            // newline before 'case' statement
   nl_after_return,           /* newline after return statement */
   nl_after_case,             /* disallow nested "case 1: a=3;" */
   nl_fcall_brace,            /* newline between function call and open brace */
   nl_squeeze_ifdef,          /* no blanks after #ifxx, #elxx, or before #endif */
   nl_enum_brace,             /* nl between enum and brace */
   nl_struct_brace,           /* nl between struct and brace */
   nl_union_brace,            /* nl between union and brace */
   nl_assign_brace,           /* nl between '=' and brace */

   nl_do_brace,               /* nl between do and { */
   nl_if_brace,               /* nl between if and { */
   nl_for_brace,              /* nl between for and { */
   nl_else_brace,             /* nl between else and { */
   nl_while_brace,            /* nl between while and { */
   nl_switch_brace,           /* nl between switch and { */
   nl_brace_else,             // nl between } and else
   nl_brace_while,            // nl between } and while of do stmt

   nl_define_macro,           // alter newlines in #define macros


   /*
    * Blank line options
    */

   blc_before_block_comment,      //TODO: before a block comment (stand-alone comment-multi)
   blc_after_func_body,           //TODO: after the closing brace of a function body
   blc_after_func_proto,          //TODO: after each prototype
   blc_after_func_proto_group,    //TODO: after a block of prototypes
   blc_after_var_def_group,       //TODO: after a group of variable defs at top of proc
   blc_after_ifdef,               //TODO: after #if or #ifdef - but not if covers whole file
   blc_max,                       //TODO: maximum consecutive newlines (3 is good)

   eat_blanks_after_open_brace,   // remove blank lines after {
   eat_blanks_before_close_brace, // remove blank lines before }


   /*
    * code modifying options (non-whitespace)
    */

   mod_paren_on_return,       // add or remove paren on return
   mod_full_brace_nl,         // max number of newlines to span w/o braces
   mod_full_brace_if,         // add or remove braces on if
   mod_full_brace_for,        // add or remove braces on for
   mod_full_brace_do,         // add or remove braces on do
   mod_full_brace_while,      // add or remove braces on while


   /*
    * Comment modifications
    */

   cmt_star_cont,       // put a star on subsequent comment lines


   /* This is used to get the enumeration count */
   option_count
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
   { Option.align_assign_span,             "align_assign_span",             ArgType.NUM  },
   { Option.align_enum_col_max,            "align_enum_col_max",            ArgType.NUM  },
   { Option.align_enum_col_min,            "align_enum_col_min",            ArgType.NUM  },
   { Option.align_enum_equ,                "align_enum_equ",                ArgType.NUM  },
   { Option.align_func_proto_span,         "align_func_proto_span",         ArgType.NUM  },
   { Option.align_keep_tabs,               "align_keep_tabs",               ArgType.BOOL },
   { Option.align_nl_cont,                 "align_nl_cont",                 ArgType.BOOL },
   { Option.align_number_left,             "align_number_left",             ArgType.BOOL },
   { Option.align_on_tabstop,              "align_on_tabstop",              ArgType.BOOL },
   { Option.align_pp_define_col_max,       "align_pp_define_col_max",       ArgType.NUM  },
   { Option.align_pp_define_col_min,       "align_pp_define_col_min",       ArgType.NUM  },
   { Option.align_pp_define_gap,           "align_pp_define_gap",           ArgType.NUM  },
   { Option.align_pp_define_span,          "align_pp_define_span",          ArgType.NUM  },
   { Option.align_right_cmt_span,          "align_right_cmt_span",          ArgType.NUM  },
   { Option.align_struct_init_span,        "align_struct_init_span",        ArgType.NUM  },
   { Option.align_typedef_gap,             "align_typedef_gap",             ArgType.NUM  },
   { Option.align_typedef_span,            "align_typedef_span",            ArgType.NUM  },
   { Option.align_var_def_colon,           "align_var_def_colon",           ArgType.BOOL },
   { Option.align_var_def_inline,          "align_var_def_inline",          ArgType.BOOL },
   { Option.align_var_def_span,            "align_var_def_span",            ArgType.NUM  },
   { Option.align_var_def_star,            "align_var_def_star",            ArgType.BOOL },
   { Option.align_var_struct_span,         "align_var_struct_span",         ArgType.NUM  },
   { Option.align_with_tabs,               "align_with_tabs",               ArgType.BOOL },
   { Option.blc_after_func_body,           "blc_after_func_body",           ArgType.NUM  },
   { Option.blc_after_func_proto,          "blc_after_func_proto",          ArgType.NUM  },
   { Option.blc_after_func_proto_group,    "blc_after_func_proto_group",    ArgType.NUM  },
   { Option.blc_after_ifdef,               "blc_after_ifdef",               ArgType.NUM  },
   { Option.blc_after_var_def_group,       "blc_after_var_def_group",       ArgType.NUM  },
   { Option.blc_before_block_comment,      "blc_before_block_comment",      ArgType.NUM  },
   { Option.blc_max,                       "blc_max",                       ArgType.NUM  },
   { Option.cmt_star_cont,                 "cmt_star_cont",                 ArgType.BOOL },
   { Option.code_width,                    "code_width",                    ArgType.NUM  },
   { Option.eat_blanks_after_open_brace,   "eat_blanks_after_open_brace",   ArgType.BOOL },
   { Option.eat_blanks_before_close_brace, "eat_blanks_before_close_brace", ArgType.BOOL },
   { Option.indent,                        "indent",                        ArgType.BOOL },
   { Option.indent_align_string,           "indent_align_string",           ArgType.BOOL },
   { Option.indent_brace,                  "indent_brace",                  ArgType.NUM  },
   { Option.indent_brace_struct,           "indent_brace_struct",           ArgType.NUM  },
   { Option.indent_case_body,              "indent_case_body",              ArgType.NUM  },
   { Option.indent_case_brace,             "indent_case_brace",             ArgType.NUM  },
   { Option.indent_col1_comment,           "indent_col1_comment",           ArgType.BOOL },
   { Option.indent_columns,                "indent_columns",                ArgType.NUM  },
   { Option.indent_func_call_param,        "indent_func_call_param",        ArgType.BOOL },
   { Option.indent_label,                  "indent_label",                  ArgType.NUM  },
   { Option.indent_paren,                  "indent_paren",                  ArgType.NUM  },
   { Option.indent_paren_nl,               "indent_paren_nl",               ArgType.BOOL },
   { Option.indent_switch_case,            "indent_switch_case",            ArgType.NUM  },
   { Option.indent_with_tabs,              "indent_with_tabs",              ArgType.NUM  },
   { Option.input_tab_size,                "input_tab_size",                ArgType.NUM  },
   { Option.leave_preproc_space,           "leave_preproc_space",           ArgType.BOOL },
   { Option.ls_after_arith,                "ls_after_arith",                ArgType.BOOL },
   { Option.ls_before_bool_op,             "ls_before_bool_op",             ArgType.BOOL },
   { Option.ls_before_paren,               "ls_before_paren",               ArgType.BOOL },
   { Option.ls_honor_newlines,             "ls_honor_newlines",             ArgType.BOOL },
   { Option.mod_full_brace_do,             "mod_full_brace_do",             ArgType.IARF },
   { Option.mod_full_brace_for,            "mod_full_brace_for",            ArgType.IARF },
   { Option.mod_full_brace_if,             "mod_full_brace_if",             ArgType.IARF },
   { Option.mod_full_brace_nl,             "mod_full_brace_nl",             ArgType.NUM  },
   { Option.mod_full_brace_while,          "mod_full_brace_while",          ArgType.IARF },
   { Option.mod_paren_on_return,           "mod_paren_on_return",           ArgType.IARF },
   { Option.nl_after_case,                 "nl_after_case",                 ArgType.BOOL },
   { Option.nl_after_return,               "nl_after_return",               ArgType.BOOL },
   { Option.nl_assign_brace,               "nl_assign_brace",               ArgType.IARF },
   { Option.nl_before_case,                "nl_before_case",                ArgType.BOOL },
   { Option.nl_brace_else,                 "nl_brace_else",                 ArgType.IARF },
   { Option.nl_brace_while,                "nl_brace_while",                ArgType.IARF },
   { Option.nl_define_macro,               "nl_define_macro",               ArgType.BOOL },
   { Option.nl_do_brace,                   "nl_do_brace",                   ArgType.IARF },
   { Option.nl_else_brace,                 "nl_else_brace",                 ArgType.IARF },
   { Option.nl_enum_brace,                 "nl_enum_brace",                 ArgType.IARF },
   { Option.nl_fcall_brace,                "nl_fcall_brace",                ArgType.IARF },
   { Option.nl_fdef_brace,                 "nl_fdef_brace",                 ArgType.IARF },
   { Option.nl_for_brace,                  "nl_for_brace",                  ArgType.IARF },
   { Option.nl_func_decl_args,             "nl_func_decl_args",             ArgType.IARF },
   { Option.nl_func_decl_end,              "nl_func_decl_end",              ArgType.IARF },
   { Option.nl_func_type_name,             "nl_func_type_name",             ArgType.IARF },
   { Option.nl_func_var_def_blk,           "nl_func_var_def_blk",           ArgType.NUM  },
   { Option.nl_if_brace,                   "nl_if_brace",                   ArgType.IARF },
   { Option.nl_squeeze_ifdef,              "nl_squeeze_ifdef",              ArgType.BOOL },
   { Option.nl_struct_brace,               "nl_struct_brace",               ArgType.IARF },
   { Option.nl_switch_brace,               "nl_switch_brace",               ArgType.IARF },
   { Option.nl_union_brace,                "nl_union_brace",                ArgType.IARF },
   { Option.nl_while_brace,                "nl_while_brace",                ArgType.IARF },
   { Option.output_tab_size,               "output_tab_size",               ArgType.NUM  },
   { Option.pp_indent,                     "pp_indent",                     ArgType.NUM  },
   { Option.sp_after_cast,                 "sp_after_cast",                 ArgType.IARF },
   { Option.sp_after_comma,                "sp_after_comma",                ArgType.IARF },
   { Option.sp_after_sparen,               "sp_after_sparen",               ArgType.IARF },
   { Option.sp_arith,                      "sp_arith",                      ArgType.IARF },
   { Option.sp_assign,                     "sp_assign",                     ArgType.IARF },
   { Option.sp_before_semi,                "sp_before_semi",                ArgType.IARF },
   { Option.sp_before_sparen,              "sp_before_sparen",              ArgType.IARF },
   { Option.sp_before_square,              "sp_before_square",              ArgType.IARF },
   { Option.sp_before_squares,             "sp_before_squares",             ArgType.IARF },
   { Option.sp_bool,                       "sp_bool",                       ArgType.IARF },
   { Option.sp_compare,                    "sp_compare",                    ArgType.IARF },
   { Option.sp_func_call_paren,            "sp_func_call_paren",            ArgType.IARF },
   { Option.sp_func_def_paren,             "sp_func_def_paren",             ArgType.IARF },
   { Option.sp_func_proto_paren,           "sp_func_proto_paren",           ArgType.IARF },
   { Option.sp_inside_braces,              "sp_inside_braces",              ArgType.IARF },
   { Option.sp_inside_braces_enum,         "sp_inside_braces_enum",         ArgType.IARF },
   { Option.sp_inside_braces_struct,       "sp_inside_braces_struct",       ArgType.IARF },
   { Option.sp_inside_fparen,              "sp_inside_fparen",              ArgType.IARF },
   { Option.sp_inside_paren,               "sp_inside_paren",               ArgType.IARF },
   { Option.sp_inside_sparen,              "sp_inside_sparen",              ArgType.IARF },
   { Option.sp_inside_square,              "sp_inside_square",              ArgType.IARF },
   { Option.sp_paren_brace,                "sp_paren_brace",                ArgType.IARF },
   { Option.sp_paren_paren,                "sp_paren_paren",                ArgType.IARF },
   { Option.sp_return_paren,               "sp_return_paren",               ArgType.IARF },
   { Option.sp_sizeof_paren,               "sp_sizeof_paren",               ArgType.IARF },
   { Option.sp_special_semi,               "sp_special_semi",               ArgType.IARF },
   { Option.sp_type_func,                  "sp_type_func",                  ArgType.IARF },
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

