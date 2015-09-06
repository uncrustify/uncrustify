/**
 * @file options.cpp
 * Parses the options from the config file.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "uncrustify_types.h"
#include "args.h"
#include "prototypes.h"
#include "uncrustify_version.h"
#include <cstring>
#ifdef HAVE_STRINGS_H
#include <strings.h>  /* strcasecmp() */
#endif
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include "unc_ctype.h"


static map<string, option_map_value>           option_name_map;
static map<uncrustify_groups, group_map_value> group_map;
static uncrustify_groups current_group;


static void unc_add_option(const char         *name,
                           uncrustify_options id,
                           argtype_e          type,
                           const char         *short_desc = NULL,
                           const char         *long_desc = NULL,
                           int                min_val = 0,
                           int                max_val = 16);


void unc_begin_group(uncrustify_groups id, const char *short_desc,
                     const char *long_desc)
{
   current_group = id;

   group_map_value value;

   value.id         = id;
   value.short_desc = short_desc;
   value.long_desc  = long_desc;

   group_map[id] = value;
}


void unc_add_option(const char *name, uncrustify_options id, argtype_e type,
                    const char *short_desc, const char *long_desc,
                    int min_val, int max_val)
{
   group_map[current_group].options.push_back(id);

   option_map_value value;

   value.id         = id;
   value.group_id   = current_group;
   value.type       = type;
   value.name       = name;
   value.short_desc = short_desc;
   value.long_desc  = long_desc;
   value.min_val    = 0;

   /* Calculate the max/min values */
   switch (type)
   {
   case AT_BOOL:
      value.max_val = 1;
      break;

   case AT_IARF:
      value.max_val = 3;
      break;

   case AT_NUM:
      value.min_val = min_val;
      value.max_val = max_val;
      break;

   case AT_LINE:
      value.max_val = 3;
      break;

   case AT_POS:
      value.max_val = 2;
      break;

   case AT_STRING:
      value.max_val = 0;
      break;

   default:
      fprintf(stderr, "FATAL: Illegal option type %d for '%s'\n", type, name);
      exit(EXIT_FAILURE);
   }

   option_name_map[name] = value;

   int name_len = strlen(name);
   if (name_len > cpd.max_option_name_len)
   {
      cpd.max_option_name_len = name_len;
   }
}


/* only compare alpha-numeric characters */
static bool match_text(const char *str1, const char *str2)
{
   int matches = 0;

   while ((*str1 != 0) && (*str2 != 0))
   {
      if (!unc_isalnum(*str1))
      {
         str1++;
         continue;
      }
      if (!unc_isalnum(*str2))
      {
         str2++;
         continue;
      }
      if (unc_tolower(*str1) != unc_tolower(*str2))
      {
         return(false);
      }
      matches++;
      str1++;
      str2++;
   }
   return(matches && (*str1 == 0) && (*str2 == 0));
}


const option_map_value *unc_find_option(const char *name)
{
   if (option_name_map.find(name) == option_name_map.end())
   {
      /* Try a more aggressive search */
      for (option_name_map_it it = option_name_map.begin();
           it != option_name_map.end();
           it++)
      {
         if (match_text(it->second.name, name))
         {
            return(&it->second);
         }
      }
      return(NULL);
   }
   return(&option_name_map[name]);
}


void register_options(void)
{
   unc_begin_group(UG_general, "General options");
   unc_add_option("newlines", UO_newlines, AT_LINE,
                  "The type of line endings");
   unc_add_option("input_tab_size", UO_input_tab_size, AT_NUM,
                  "The original size of tabs in the input", "", 1, 32);
   unc_add_option("output_tab_size", UO_output_tab_size, AT_NUM,
                  "The size of tabs in the output (only used if align_with_tabs=true)", "", 1, 32);
   unc_add_option("string_escape_char", UO_string_escape_char, AT_NUM,
                  "The ASCII value of the string escape char, usually 92 (\\) or 94 (^). (Pawn)", "", 0, 255);
   unc_add_option("string_escape_char2", UO_string_escape_char2, AT_NUM,
                  "Alternate string escape char for Pawn. Only works right before the quote char.", "", 0, 255);
   unc_add_option("string_replace_tab_chars", UO_string_replace_tab_chars, AT_BOOL,
                  "Replace tab characters found in string literals with the escape sequence \\t instead.");
   unc_add_option("tok_split_gte", UO_tok_split_gte, AT_BOOL,
                  "Allow interpreting '>=' and '>>=' as part of a template in 'void f(list<list<B>>=val);'.\n"
                  "If true (default), 'assert(x<0 && y>=3)' will be broken.\n"
                  "Improvements to template detection may make this option obsolete.");
   unc_add_option("disable_processing_cmt", UO_disable_processing_cmt, AT_STRING,
                  "Override the default ' *INDENT-OFF*' in comments for disabling processing of part of the file.");
   unc_add_option("enable_processing_cmt", UO_enable_processing_cmt, AT_STRING,
                  "Override the default ' *INDENT-ON*' in comments for enabling processing of part of the file.");

   unc_add_option("utf8_bom", UO_utf8_bom, AT_IARF,
                  "Control what to do with the UTF-8 BOM (recommend 'remove')");
   unc_add_option("utf8_byte", UO_utf8_byte, AT_BOOL,
                  "If the file contains bytes with values between 128 and 255, but is not UTF-8, then output as UTF-8");
   unc_add_option("utf8_force", UO_utf8_force, AT_BOOL,
                  "Force the output encoding to UTF-8");

   unc_begin_group(UG_space, "Spacing options");
   unc_add_option("sp_arith", UO_sp_arith, AT_IARF,
                  "Add or remove space around arithmetic operator '+', '-', '/', '*', etc");
   unc_add_option("sp_assign", UO_sp_assign, AT_IARF,
                  "Add or remove space around assignment operator '=', '+=', etc");
   unc_add_option("sp_cpp_lambda_assign", UO_sp_cpp_lambda_assign, AT_IARF,
                  "Add or remove space around '=' in C++11 lambda capture specifications. Overrides sp_assign");
   unc_add_option("sp_cpp_lambda_paren", UO_sp_cpp_lambda_paren, AT_IARF,
                  "Add or remove space after the capture specification in C++11 lambda.");
   unc_add_option("sp_assign_default", UO_sp_assign_default, AT_IARF,
                  "Add or remove space around assignment operator '=' in a prototype");
   unc_add_option("sp_before_assign", UO_sp_before_assign, AT_IARF,
                  "Add or remove space before assignment operator '=', '+=', etc. Overrides sp_assign.");
   unc_add_option("sp_after_assign", UO_sp_after_assign, AT_IARF,
                  "Add or remove space after assignment operator '=', '+=', etc. Overrides sp_assign.");
   unc_add_option("sp_enum_paren", UO_sp_enum_paren, AT_IARF,
                  "Add or remove space in 'NS_ENUM ('");
   unc_add_option("sp_enum_assign", UO_sp_enum_assign, AT_IARF,
                  "Add or remove space around assignment '=' in enum");
   unc_add_option("sp_enum_before_assign", UO_sp_enum_before_assign, AT_IARF,
                  "Add or remove space before assignment '=' in enum. Overrides sp_enum_assign.");
   unc_add_option("sp_enum_after_assign", UO_sp_enum_after_assign, AT_IARF,
                  "Add or remove space after assignment '=' in enum. Overrides sp_enum_assign.");
   unc_add_option("sp_pp_concat", UO_sp_pp_concat, AT_IARF,
                  "Add or remove space around preprocessor '##' concatenation operator. Default=Add");
   unc_add_option("sp_pp_stringify", UO_sp_pp_stringify, AT_IARF,
                  "Add or remove space after preprocessor '#' stringify operator. Also affects the '#@' charizing operator.");
   unc_add_option("sp_before_pp_stringify", UO_sp_before_pp_stringify, AT_IARF,
                  "Add or remove space before preprocessor '#' stringify operator as in '#define x(y) L#y'.");
   unc_add_option("sp_bool", UO_sp_bool, AT_IARF,
                  "Add or remove space around boolean operators '&&' and '||'");
   unc_add_option("sp_compare", UO_sp_compare, AT_IARF,
                  "Add or remove space around compare operator '<', '>', '==', etc");
   unc_add_option("sp_inside_paren", UO_sp_inside_paren, AT_IARF,
                  "Add or remove space inside '(' and ')'");
   unc_add_option("sp_paren_paren", UO_sp_paren_paren, AT_IARF,
                  "Add or remove space between nested parens: '((' vs ') )'");
   unc_add_option("sp_cparen_oparen", UO_sp_cparen_oparen, AT_IARF,
                  "Add or remove space between back-to-back parens: ')(' vs ') ('");
   unc_add_option("sp_balance_nested_parens", UO_sp_balance_nested_parens, AT_BOOL,
                  "Whether to balance spaces inside nested parens");
   unc_add_option("sp_paren_brace", UO_sp_paren_brace, AT_IARF,
                  "Add or remove space between ')' and '{'");
   unc_add_option("sp_before_ptr_star", UO_sp_before_ptr_star, AT_IARF,
                  "Add or remove space before pointer star '*'");
   unc_add_option("sp_before_unnamed_ptr_star", UO_sp_before_unnamed_ptr_star, AT_IARF,
                  "Add or remove space before pointer star '*' that isn't followed by a variable name\n"
                  "If set to 'ignore', sp_before_ptr_star is used instead.");
   unc_add_option("sp_between_ptr_star", UO_sp_between_ptr_star, AT_IARF,
                  "Add or remove space between pointer stars '*'");
   unc_add_option("sp_after_ptr_star", UO_sp_after_ptr_star, AT_IARF,
                  "Add or remove space after pointer star '*', if followed by a word.");
   unc_add_option("sp_after_ptr_star_qualifier", UO_sp_after_ptr_star_qualifier, AT_IARF,
                  "Add or remove space after pointer star '*', if followed by a qualifier.");
   unc_add_option("sp_after_ptr_star_func", UO_sp_after_ptr_star_func, AT_IARF,
                  "Add or remove space after a pointer star '*', if followed by a func proto/def.");
   unc_add_option("sp_ptr_star_paren", UO_sp_ptr_star_paren, AT_IARF,
                  "Add or remove space after a pointer star '*', if followed by an open paren (function types).");
   unc_add_option("sp_before_ptr_star_func", UO_sp_before_ptr_star_func, AT_IARF,
                  "Add or remove space before a pointer star '*', if followed by a func proto/def.");
   unc_add_option("sp_before_byref", UO_sp_before_byref, AT_IARF,
                  "Add or remove space before a reference sign '&'");
   unc_add_option("sp_before_unnamed_byref", UO_sp_before_unnamed_byref, AT_IARF,
                  "Add or remove space before a reference sign '&' that isn't followed by a variable name\n"
                  "If set to 'ignore', sp_before_byref is used instead.");
   unc_add_option("sp_after_byref", UO_sp_after_byref, AT_IARF,
                  "Add or remove space after reference sign '&', if followed by a word.");
   unc_add_option("sp_after_byref_func", UO_sp_after_byref_func, AT_IARF,
                  "Add or remove space after a reference sign '&', if followed by a func proto/def.");
   unc_add_option("sp_before_byref_func", UO_sp_before_byref_func, AT_IARF,
                  "Add or remove space before a reference sign '&', if followed by a func proto/def.");
   unc_add_option("sp_after_type", UO_sp_after_type, AT_IARF,
                  "Add or remove space between type and word. Default=Force");
   unc_add_option("sp_before_template_paren", UO_sp_before_template_paren, AT_IARF,
                  "Add or remove space before the paren in the D constructs 'template Foo(' and 'class Foo('.");
   unc_add_option("sp_template_angle", UO_sp_template_angle, AT_IARF,
                  "Add or remove space in 'template <' vs 'template<'.\n"
                  "If set to ignore, sp_before_angle is used.");
   unc_add_option("sp_before_angle", UO_sp_before_angle, AT_IARF,
                  "Add or remove space before '<>'");
   unc_add_option("sp_inside_angle", UO_sp_inside_angle, AT_IARF,
                  "Add or remove space inside '<' and '>'");
   unc_add_option("sp_after_angle", UO_sp_after_angle, AT_IARF,
                  "Add or remove space after '<>'");
   unc_add_option("sp_angle_paren", UO_sp_angle_paren, AT_IARF,
                  "Add or remove space between '<>' and '(' as found in 'new List<byte>();'");
   unc_add_option("sp_angle_word", UO_sp_angle_word, AT_IARF,
                  "Add or remove space between '<>' and a word as in 'List<byte> m;'");
   unc_add_option("sp_angle_shift", UO_sp_angle_shift, AT_IARF,
                  "Add or remove space between '>' and '>' in '>>' (template stuff C++/C# only). Default=Add");
   unc_add_option("sp_permit_cpp11_shift", UO_sp_permit_cpp11_shift, AT_BOOL,
                  "Permit removal of the space between '>>' in 'foo<bar<int> >' (C++11 only). Default=False\n"
                  "sp_angle_shift cannot remove the space without this option.");
   unc_add_option("sp_before_sparen", UO_sp_before_sparen, AT_IARF,
                  "Add or remove space before '(' of 'if', 'for', 'switch', and 'while'");
   unc_add_option("sp_inside_sparen", UO_sp_inside_sparen, AT_IARF,
                  "Add or remove space inside if-condition '(' and ')'");
   unc_add_option("sp_inside_sparen_close", UO_sp_inside_sparen_close, AT_IARF,
                  "Add or remove space before if-condition ')'. Overrides sp_inside_sparen.");
   unc_add_option("sp_inside_sparen_open", UO_sp_inside_sparen_open, AT_IARF,
                  "Add or remove space after if-condition '('. Overrides sp_inside_sparen.");
   unc_add_option("sp_after_sparen", UO_sp_after_sparen, AT_IARF,
                  "Add or remove space after ')' of 'if', 'for', 'switch', and 'while'");
   unc_add_option("sp_sparen_brace", UO_sp_sparen_brace, AT_IARF,
                  "Add or remove space between ')' and '{' of 'if', 'for', 'switch', and 'while'");
   unc_add_option("sp_invariant_paren", UO_sp_invariant_paren, AT_IARF,
                  "Add or remove space between 'invariant' and '(' in the D language.");
   unc_add_option("sp_after_invariant_paren", UO_sp_after_invariant_paren, AT_IARF,
                  "Add or remove space after the ')' in 'invariant (C) c' in the D language.");
   unc_add_option("sp_special_semi", UO_sp_special_semi, AT_IARF,
                  "Add or remove space before empty statement ';' on 'if', 'for' and 'while'");
   unc_add_option("sp_before_semi", UO_sp_before_semi, AT_IARF,
                  "Add or remove space before ';'. Default=Remove");
   unc_add_option("sp_before_semi_for", UO_sp_before_semi_for, AT_IARF,
                  "Add or remove space before ';' in non-empty 'for' statements");
   unc_add_option("sp_before_semi_for_empty", UO_sp_before_semi_for_empty, AT_IARF,
                  "Add or remove space before a semicolon of an empty part of a for statement.");
   unc_add_option("sp_after_semi", UO_sp_after_semi, AT_IARF,
                  "Add or remove space after ';', except when followed by a comment. Default=Add");
   unc_add_option("sp_after_semi_for", UO_sp_after_semi_for, AT_IARF,
                  "Add or remove space after ';' in non-empty 'for' statements. Default=Force");
   unc_add_option("sp_after_semi_for_empty", UO_sp_after_semi_for_empty, AT_IARF,
                  "Add or remove space after the final semicolon of an empty part of a for statement: for ( ; ; <here> ).");
   unc_add_option("sp_before_square", UO_sp_before_square, AT_IARF,
                  "Add or remove space before '[' (except '[]')");
   unc_add_option("sp_before_squares", UO_sp_before_squares, AT_IARF,
                  "Add or remove space before '[]'");
   unc_add_option("sp_inside_square", UO_sp_inside_square, AT_IARF,
                  "Add or remove space inside a non-empty '[' and ']'");
   unc_add_option("sp_after_comma", UO_sp_after_comma, AT_IARF,
                  "Add or remove space after ','");
   unc_add_option("sp_before_comma", UO_sp_before_comma, AT_IARF,
                  "Add or remove space before ','");
   unc_add_option("sp_after_mdatype_commas", UO_sp_after_mdatype_commas, AT_IARF,
                  "Add or remove space between ',' and ']' in multidimensional array type 'int[,,]'");
   unc_add_option("sp_before_mdatype_commas", UO_sp_before_mdatype_commas, AT_IARF,
                  "Add or remove space between '[' and ',' in multidimensional array type 'int[,,]'");
   unc_add_option("sp_between_mdatype_commas", UO_sp_between_mdatype_commas, AT_IARF,
                  "Add or remove space between ',' in multidimensional array type 'int[,,]'");
   unc_add_option("sp_paren_comma", UO_sp_paren_comma, AT_IARF,
                  "Add or remove space between an open paren and comma: '(,' vs '( ,'\n");
   unc_add_option("sp_before_ellipsis", UO_sp_before_ellipsis, AT_IARF,
                  "Add or remove space before the variadic '...' when preceded by a non-punctuator");
   unc_add_option("sp_after_class_colon", UO_sp_after_class_colon, AT_IARF,
                  "Add or remove space after class ':'");
   unc_add_option("sp_before_class_colon", UO_sp_before_class_colon, AT_IARF,
                  "Add or remove space before class ':'");
   unc_add_option("sp_after_constr_colon", UO_sp_after_constr_colon, AT_IARF,
                  "Add or remove space after class constructor ':'");
   unc_add_option("sp_before_constr_colon", UO_sp_before_constr_colon, AT_IARF,
                  "Add or remove space before class constructor ':'");
   unc_add_option("sp_before_case_colon", UO_sp_before_case_colon, AT_IARF,
                  "Add or remove space before case ':'. Default=Remove");
   unc_add_option("sp_after_operator", UO_sp_after_operator, AT_IARF,
                  "Add or remove space between 'operator' and operator sign");
   unc_add_option("sp_after_operator_sym", UO_sp_after_operator_sym, AT_IARF,
                  "Add or remove space between the operator symbol and the open paren, as in 'operator ++('");
   unc_add_option("sp_after_cast", UO_sp_after_cast, AT_IARF,
                  "Add or remove space after C/D cast, i.e. 'cast(int)a' vs 'cast(int) a' or '(int)a' vs '(int) a'");
   unc_add_option("sp_inside_paren_cast", UO_sp_inside_paren_cast, AT_IARF,
                  "Add or remove spaces inside cast parens");
   unc_add_option("sp_cpp_cast_paren", UO_sp_cpp_cast_paren, AT_IARF,
                  "Add or remove space between the type and open paren in a C++ cast, i.e. 'int(exp)' vs 'int (exp)'");
   unc_add_option("sp_sizeof_paren", UO_sp_sizeof_paren, AT_IARF,
                  "Add or remove space between 'sizeof' and '('");
   unc_add_option("sp_after_tag", UO_sp_after_tag, AT_IARF,
                  "Add or remove space after the tag keyword (Pawn)");
   unc_add_option("sp_inside_braces_enum", UO_sp_inside_braces_enum, AT_IARF,
                  "Add or remove space inside enum '{' and '}'");
   unc_add_option("sp_inside_braces_struct", UO_sp_inside_braces_struct, AT_IARF,
                  "Add or remove space inside struct/union '{' and '}'");
   unc_add_option("sp_inside_braces", UO_sp_inside_braces, AT_IARF,
                  "Add or remove space inside '{' and '}'");
   unc_add_option("sp_inside_braces_empty", UO_sp_inside_braces_empty, AT_IARF,
                  "Add or remove space inside '{}'");
   unc_add_option("sp_type_func", UO_sp_type_func, AT_IARF,
                  "Add or remove space between return type and function name\n"
                  "A minimum of 1 is forced except for pointer return types.");
   unc_add_option("sp_func_proto_paren", UO_sp_func_proto_paren, AT_IARF,
                  "Add or remove space between function name and '(' on function declaration");
   unc_add_option("sp_func_def_paren", UO_sp_func_def_paren, AT_IARF,
                  "Add or remove space between function name and '(' on function definition");
   unc_add_option("sp_inside_fparens", UO_sp_inside_fparens, AT_IARF,
                  "Add or remove space inside empty function '()'");
   unc_add_option("sp_inside_fparen", UO_sp_inside_fparen, AT_IARF,
                  "Add or remove space inside function '(' and ')'");
   unc_add_option("sp_inside_tparen", UO_sp_inside_tparen, AT_IARF,
                  "Add or remove space inside the first parens in the function type: 'void (*x)(...)'");
   unc_add_option("sp_after_tparen_close", UO_sp_after_tparen_close, AT_IARF,
                  "Add or remove between the parens in the function type: 'void (*x)(...)'");
   unc_add_option("sp_square_fparen", UO_sp_square_fparen, AT_IARF,
                  "Add or remove space between ']' and '(' when part of a function call.");
   unc_add_option("sp_fparen_brace", UO_sp_fparen_brace, AT_IARF,
                  "Add or remove space between ')' and '{' of function");
   unc_add_option("sp_fparen_dbrace", UO_sp_fparen_dbrace, AT_IARF,
                  "Java: Add or remove space between ')' and '{{' of double brace initializer.");
   unc_add_option("sp_func_call_paren", UO_sp_func_call_paren, AT_IARF,
                  "Add or remove space between function name and '(' on function calls");
   unc_add_option("sp_func_call_paren_empty", UO_sp_func_call_paren_empty, AT_IARF,
                  "Add or remove space between function name and '()' on function calls without parameters.\n"
                  "If set to 'ignore' (the default), sp_func_call_paren is used.");
   unc_add_option("sp_func_call_user_paren", UO_sp_func_call_user_paren, AT_IARF,
                  "Add or remove space between the user function name and '(' on function calls\n"
                  "You need to set a keyword to be a user function, like this: 'set func_call_user _' in the config file.");
   unc_add_option("sp_func_class_paren", UO_sp_func_class_paren, AT_IARF,
                  "Add or remove space between a constructor/destructor and the open paren");
   unc_add_option("sp_return_paren", UO_sp_return_paren, AT_IARF,
                  "Add or remove space between 'return' and '('");
   unc_add_option("sp_attribute_paren", UO_sp_attribute_paren, AT_IARF,
                  "Add or remove space between '__attribute__' and '('");
   unc_add_option("sp_defined_paren", UO_sp_defined_paren, AT_IARF,
                  "Add or remove space between 'defined' and '(' in '#if defined (FOO)'");
   unc_add_option("sp_throw_paren", UO_sp_throw_paren, AT_IARF,
                  "Add or remove space between 'throw' and '(' in 'throw (something)'");
   unc_add_option("sp_after_throw", UO_sp_after_throw, AT_IARF,
                  "Add or remove space between 'throw' and anything other than '(' as in '@throw [...];'");
   unc_add_option("sp_catch_paren", UO_sp_catch_paren, AT_IARF,
                  "Add or remove space between 'catch' and '(' in 'catch (something) { }'\n"
                  "If set to ignore, sp_before_sparen is used.");
   unc_add_option("sp_version_paren", UO_sp_version_paren, AT_IARF,
                  "Add or remove space between 'version' and '(' in 'version (something) { }' (D language)\n"
                  "If set to ignore, sp_before_sparen is used.");
   unc_add_option("sp_scope_paren", UO_sp_scope_paren, AT_IARF,
                  "Add or remove space between 'scope' and '(' in 'scope (something) { }' (D language)\n"
                  "If set to ignore, sp_before_sparen is used.");
   unc_add_option("sp_macro", UO_sp_macro, AT_IARF,
                  "Add or remove space between macro and value");
   unc_add_option("sp_macro_func", UO_sp_macro_func, AT_IARF,
                  "Add or remove space between macro function ')' and value");
   unc_add_option("sp_else_brace", UO_sp_else_brace, AT_IARF,
                  "Add or remove space between 'else' and '{' if on the same line");
   unc_add_option("sp_brace_else", UO_sp_brace_else, AT_IARF,
                  "Add or remove space between '}' and 'else' if on the same line");
   unc_add_option("sp_brace_typedef", UO_sp_brace_typedef, AT_IARF,
                  "Add or remove space between '}' and the name of a typedef on the same line");
   unc_add_option("sp_catch_brace", UO_sp_catch_brace, AT_IARF,
                  "Add or remove space between 'catch' and '{' if on the same line");
   unc_add_option("sp_brace_catch", UO_sp_brace_catch, AT_IARF,
                  "Add or remove space between '}' and 'catch' if on the same line");
   unc_add_option("sp_finally_brace", UO_sp_finally_brace, AT_IARF,
                  "Add or remove space between 'finally' and '{' if on the same line");
   unc_add_option("sp_brace_finally", UO_sp_brace_finally, AT_IARF,
                  "Add or remove space between '}' and 'finally' if on the same line");
   unc_add_option("sp_try_brace", UO_sp_try_brace, AT_IARF,
                  "Add or remove space between 'try' and '{' if on the same line");
   unc_add_option("sp_getset_brace", UO_sp_getset_brace, AT_IARF,
                  "Add or remove space between get/set and '{' if on the same line");
   unc_add_option("sp_word_brace", UO_sp_word_brace, AT_IARF,
                  "Add or remove space between a variable and '{' for C++ uniform initialization");
   unc_add_option("sp_word_brace_ns", UO_sp_word_brace_ns, AT_IARF,
                  "Add or remove space between a variable and '{' for a namespace");
   unc_add_option("sp_before_dc", UO_sp_before_dc, AT_IARF,
                  "Add or remove space before the '::' operator");
   unc_add_option("sp_after_dc", UO_sp_after_dc, AT_IARF,
                  "Add or remove space after the '::' operator");
   unc_add_option("sp_d_array_colon", UO_sp_d_array_colon, AT_IARF,
                  "Add or remove around the D named array initializer ':' operator");
   unc_add_option("sp_not", UO_sp_not, AT_IARF,
                  "Add or remove space after the '!' (not) operator. Default=Remove");
   unc_add_option("sp_inv", UO_sp_inv, AT_IARF,
                  "Add or remove space after the '~' (invert) operator. Default=Remove");
   unc_add_option("sp_addr", UO_sp_addr, AT_IARF,
                  "Add or remove space after the '&' (address-of) operator. Default=Remove\n"
                  "This does not affect the spacing after a '&' that is part of a type.");
   unc_add_option("sp_member", UO_sp_member, AT_IARF,
                  "Add or remove space around the '.' or '->' operators. Default=Remove");
   unc_add_option("sp_deref", UO_sp_deref, AT_IARF,
                  "Add or remove space after the '*' (dereference) operator. Default=Remove\n"
                  "This does not affect the spacing after a '*' that is part of a type.");
   unc_add_option("sp_sign", UO_sp_sign, AT_IARF,
                  "Add or remove space after '+' or '-', as in 'x = -5' or 'y = +7'. Default=Remove");
   unc_add_option("sp_incdec", UO_sp_incdec, AT_IARF,
                  "Add or remove space before or after '++' and '--', as in '(--x)' or 'y++;'. Default=Remove");

   unc_add_option("sp_before_nl_cont", UO_sp_before_nl_cont, AT_IARF,
                  "Add or remove space before a backslash-newline at the end of a line. Default=Add");

   unc_add_option("sp_after_oc_scope", UO_sp_after_oc_scope, AT_IARF,
                  "Add or remove space after the scope '+' or '-', as in '-(void) foo;' or '+(int) bar;'");
   unc_add_option("sp_after_oc_colon", UO_sp_after_oc_colon, AT_IARF,
                  "Add or remove space after the colon in message specs\n"
                  "'-(int) f:(int) x;' vs '-(int) f: (int) x;'");
   unc_add_option("sp_before_oc_colon", UO_sp_before_oc_colon, AT_IARF,
                  "Add or remove space before the colon in message specs\n"
                  "'-(int) f: (int) x;' vs '-(int) f : (int) x;'");
   unc_add_option("sp_after_oc_dict_colon", UO_sp_after_oc_dict_colon, AT_IARF,
                  "Add or remove space after the colon in immutable dictionary expression\n"
                  "'NSDictionary *test = @{@\"foo\" :@\"bar\"};'");
   unc_add_option("sp_before_oc_dict_colon", UO_sp_before_oc_dict_colon, AT_IARF,
                  "Add or remove space before the colon in immutable dictionary expression\n"
                  "'NSDictionary *test = @{@\"foo\" :@\"bar\"};'");
   unc_add_option("sp_after_send_oc_colon", UO_sp_after_send_oc_colon, AT_IARF,
                  "Add or remove space after the colon in message specs\n"
                  "'[object setValue:1];' vs '[object setValue: 1];'");
   unc_add_option("sp_before_send_oc_colon", UO_sp_before_send_oc_colon, AT_IARF,
                  "Add or remove space before the colon in message specs\n"
                  "'[object setValue:1];' vs '[object setValue :1];'");
   unc_add_option("sp_after_oc_type", UO_sp_after_oc_type, AT_IARF,
                  "Add or remove space after the (type) in message specs\n"
                  "'-(int)f: (int) x;' vs '-(int)f: (int)x;'");
   unc_add_option("sp_after_oc_return_type", UO_sp_after_oc_return_type, AT_IARF,
                  "Add or remove space after the first (type) in message specs\n"
                  "'-(int) f:(int)x;' vs '-(int)f:(int)x;'");
   unc_add_option("sp_after_oc_at_sel", UO_sp_after_oc_at_sel, AT_IARF,
                  "Add or remove space between '@selector' and '('\n"
                  "'@selector(msgName)' vs '@selector (msgName)'\n"
                  "Also applies to @protocol() constructs");
   unc_add_option("sp_after_oc_at_sel_parens", UO_sp_after_oc_at_sel_parens, AT_IARF,
                  "Add or remove space between '@selector(x)' and the following word\n"
                  "'@selector(foo) a:' vs '@selector(foo)a:'");
   unc_add_option("sp_inside_oc_at_sel_parens", UO_sp_inside_oc_at_sel_parens, AT_IARF,
                  "Add or remove space inside '@selector' parens\n"
                  "'@selector(foo)' vs '@selector( foo )'\n"
                  "Also applies to @protocol() constructs");
   unc_add_option("sp_before_oc_block_caret", UO_sp_before_oc_block_caret, AT_IARF,
                  "Add or remove space before a block pointer caret\n"
                  "'^int (int arg){...}' vs. ' ^int (int arg){...}'");
   unc_add_option("sp_after_oc_block_caret", UO_sp_after_oc_block_caret, AT_IARF,
                  "Add or remove space after a block pointer caret\n"
                  "'^int (int arg){...}' vs. '^ int (int arg){...}'");
   unc_add_option("sp_after_oc_msg_receiver", UO_sp_after_oc_msg_receiver, AT_IARF,
                  "Add or remove space between the receiver and selector in a message.\n"
                  "'[receiver selector ...]'");
   unc_add_option("sp_after_oc_property", UO_sp_after_oc_property, AT_IARF,
                  "Add or remove space after @property.");

   unc_add_option("sp_cond_colon", UO_sp_cond_colon, AT_IARF,
                  "Add or remove space around the ':' in 'b ? t : f'");
   unc_add_option("sp_cond_colon_before", UO_sp_cond_colon_before, AT_IARF,
                  "Add or remove space before the ':' in 'b ? t : f'. Overrides sp_cond_colon.");
   unc_add_option("sp_cond_colon_after", UO_sp_cond_colon_after, AT_IARF,
                  "Add or remove space after the ':' in 'b ? t : f'. Overrides sp_cond_colon.");
   unc_add_option("sp_cond_question", UO_sp_cond_question, AT_IARF,
                  "Add or remove space around the '?' in 'b ? t : f'");
   unc_add_option("sp_cond_question_before", UO_sp_cond_question_before, AT_IARF,
                  "Add or remove space before the '?' in 'b ? t : f'. Overrides sp_cond_question.");
   unc_add_option("sp_cond_question_after", UO_sp_cond_question_after, AT_IARF,
                  "Add or remove space after the '?' in 'b ? t : f'. Overrides sp_cond_question.");
   unc_add_option("sp_cond_ternary_short", UO_sp_cond_ternary_short, AT_IARF,
                  "In the abbreviated ternary form (a ?: b), add/remove space between ? and :.'. Overrides all other sp_cond_* options.");
   unc_add_option("sp_case_label", UO_sp_case_label, AT_IARF,
                  "Fix the spacing between 'case' and the label. Only 'ignore' and 'force' make sense here.");
   unc_add_option("sp_range", UO_sp_range, AT_IARF,
                  "Control the space around the D '..' operator.");

   unc_add_option("sp_after_for_colon", UO_sp_after_for_colon, AT_IARF,
                  "Control the spacing after ':' in 'for (TYPE VAR : EXPR)' (Java)");
   unc_add_option("sp_before_for_colon", UO_sp_before_for_colon, AT_IARF,
                  "Control the spacing before ':' in 'for (TYPE VAR : EXPR)' (Java)");

   unc_add_option("sp_extern_paren", UO_sp_extern_paren, AT_IARF,
                  "Control the spacing in 'extern (C)' (D)");

   unc_add_option("sp_cmt_cpp_start", UO_sp_cmt_cpp_start, AT_IARF,
                  "Control the space after the opening of a C++ comment '// A' vs '//A'");
   unc_add_option("sp_cmt_cpp_doxygen", UO_sp_cmt_cpp_doxygen, AT_BOOL,
                  "TRUE: If space is added with sp_cmt_cpp_start, do it after doxygen sequences like '///', '///<', '//!' and '//!<'.");

   unc_add_option("sp_endif_cmt", UO_sp_endif_cmt, AT_IARF,
                  "Controls the spaces between #else or #endif and a trailing comment");
   unc_add_option("sp_after_new", UO_sp_after_new, AT_IARF,
                  "Controls the spaces after 'new', 'delete', and 'delete[]'");
   unc_add_option("sp_between_new_paren", UO_sp_between_new_paren, AT_IARF,
                  "Controls the spaces between new and '(' in 'new()'");
   unc_add_option("sp_before_tr_emb_cmt", UO_sp_before_tr_emb_cmt, AT_IARF,
                  "Controls the spaces before a trailing or embedded comment");
   unc_add_option("sp_num_before_tr_emb_cmt", UO_sp_num_before_tr_emb_cmt, AT_NUM,
                  "Number of spaces before a trailing or embedded comment");

   unc_add_option("sp_annotation_paren", UO_sp_annotation_paren, AT_IARF,
                  "Control space between a Java annotation and the open paren.");

   unc_begin_group(UG_indent, "Indenting");
   unc_add_option("indent_columns", UO_indent_columns, AT_NUM,
                  "The number of columns to indent per level.\n"
                  "Usually 2, 3, 4, or 8.");
   unc_add_option("indent_continue", UO_indent_continue, AT_NUM,
                  "The continuation indent. If non-zero, this overrides the indent of '(' and '=' continuation indents.\n"
                  "For FreeBSD, this is set to 4. Negative value is absolute and not increased for each ( level");
   unc_add_option("indent_with_tabs", UO_indent_with_tabs, AT_NUM,
                  "How to use tabs when indenting code\n"
                  "0=spaces only\n"
                  "1=indent with tabs to brace level, align with spaces\n"
                  "2=indent and align with tabs, using spaces when not on a tabstop", "", 0, 2);
   unc_add_option("indent_cmt_with_tabs", UO_indent_cmt_with_tabs, AT_BOOL,
                  "Comments that are not a brace level are indented with tabs on a tabstop.\n"
                  "Requires indent_with_tabs=2. If false, will use spaces.");
   unc_add_option("indent_align_string", UO_indent_align_string, AT_BOOL,
                  "Whether to indent strings broken by '\\' so that they line up");
   unc_add_option("indent_xml_string", UO_indent_xml_string, AT_NUM,
                  "The number of spaces to indent multi-line XML strings.\n"
                  "Requires indent_align_string=True");
   unc_add_option("indent_brace", UO_indent_brace, AT_NUM,
                  "Spaces to indent '{' from level");
   unc_add_option("indent_braces", UO_indent_braces, AT_BOOL,
                  "Whether braces are indented to the body level");
   unc_add_option("indent_braces_no_func", UO_indent_braces_no_func, AT_BOOL,
                  "Disabled indenting function braces if indent_braces is true");
   unc_add_option("indent_braces_no_class", UO_indent_braces_no_class, AT_BOOL,
                  "Disabled indenting class braces if indent_braces is true");
   unc_add_option("indent_braces_no_struct", UO_indent_braces_no_struct, AT_BOOL,
                  "Disabled indenting struct braces if indent_braces is true");
   unc_add_option("indent_brace_parent", UO_indent_brace_parent, AT_BOOL,
                  "Indent based on the size of the brace parent, i.e. 'if' => 3 spaces, 'for' => 4 spaces, etc.");
   unc_add_option("indent_paren_open_brace", UO_indent_paren_open_brace, AT_BOOL,
                  "Indent based on the paren open instead of the brace open in '({\\n', default is to indent by brace.");
   unc_add_option("indent_namespace", UO_indent_namespace, AT_BOOL,
                  "Whether the 'namespace' body is indented");
   unc_add_option("indent_namespace_single_indent", UO_indent_namespace_single_indent, AT_BOOL,
                  "Only indent one namespace and no sub-namespaces.\n"
                  "Requires indent_namespace=true.");
   unc_add_option("indent_namespace_level", UO_indent_namespace_level, AT_NUM,
                  "The number of spaces to indent a namespace block");
   unc_add_option("indent_namespace_limit", UO_indent_namespace_limit, AT_NUM,
                  "If the body of the namespace is longer than this number, it won't be indented.\n"
                  "Requires indent_namespace=true. Default=0 (no limit)", NULL, 0, 255);
   unc_add_option("indent_extern", UO_indent_extern, AT_BOOL,
                  "Whether the 'extern \"C\"' body is indented");
   unc_add_option("indent_class", UO_indent_class, AT_BOOL,
                  "Whether the 'class' body is indented");
   unc_add_option("indent_class_colon", UO_indent_class_colon, AT_BOOL,
                  "Whether to indent the stuff after a leading base class colon");
   unc_add_option("indent_class_on_colon", UO_indent_class_on_colon, AT_BOOL,
                  "Indent based on a class colon instead of the stuff after the colon.\n"
                  "Requires indent_class_colon=true. Default=false");
   unc_add_option("indent_constr_colon", UO_indent_constr_colon, AT_BOOL,
                  "Whether to indent the stuff after a leading class initializer colon");
   unc_add_option("indent_ctor_init_leading", UO_indent_ctor_init_leading, AT_NUM,
                  "Virtual indent from the ':' for member initializers. Default is 2");
   unc_add_option("indent_ctor_init", UO_indent_ctor_init, AT_NUM,
                  "Additional indenting for constructor initializer list");
   unc_add_option("indent_else_if", UO_indent_else_if, AT_BOOL,
                  "False=treat 'else\\nif' as 'else if' for indenting purposes\n"
                  "True=indent the 'if' one level\n");
   unc_add_option("indent_var_def_blk", UO_indent_var_def_blk, AT_NUM,
                  "Amount to indent variable declarations after a open brace. neg=relative, pos=absolute");
   unc_add_option("indent_var_def_cont", UO_indent_var_def_cont, AT_BOOL,
                  "Indent continued variable declarations instead of aligning.");
   unc_add_option("indent_shift", UO_indent_shift, AT_BOOL,
                  "Indent continued shift expressions ('<<' and '>>') instead of aligning.\n"
                  "Turn align_left_shift off when enabling this.");

   unc_add_option("indent_func_def_force_col1", UO_indent_func_def_force_col1, AT_BOOL,
                  "True:  force indentation of function definition to start in column 1\n"
                  "False: use the default behavior");
   unc_add_option("indent_func_call_param", UO_indent_func_call_param, AT_BOOL,
                  "True:  indent continued function call parameters one indent level\n"
                  "False: align parameters under the open paren");
   unc_add_option("indent_func_def_param", UO_indent_func_def_param, AT_BOOL,
                  "Same as indent_func_call_param, but for function defs");
   unc_add_option("indent_func_proto_param", UO_indent_func_proto_param, AT_BOOL,
                  "Same as indent_func_call_param, but for function protos");
   unc_add_option("indent_func_class_param", UO_indent_func_class_param, AT_BOOL,
                  "Same as indent_func_call_param, but for class declarations");
   unc_add_option("indent_func_ctor_var_param", UO_indent_func_ctor_var_param, AT_BOOL,
                  "Same as indent_func_call_param, but for class variable constructors");
   unc_add_option("indent_template_param", UO_indent_template_param, AT_BOOL,
                  "Same as indent_func_call_param, but for templates");
   unc_add_option("indent_func_param_double", UO_indent_func_param_double, AT_BOOL,
                  "Double the indent for indent_func_xxx_param options");

   unc_add_option("indent_func_const", UO_indent_func_const, AT_NUM,
                  "Indentation column for standalone 'const' function decl/proto qualifier");
   unc_add_option("indent_func_throw", UO_indent_func_throw, AT_NUM,
                  "Indentation column for standalone 'throw' function decl/proto qualifier");

   unc_add_option("indent_member", UO_indent_member, AT_NUM,
                  "The number of spaces to indent a continued '->' or '.'\n"
                  "Usually set to 0, 1, or indent_columns.");
   unc_add_option("indent_sing_line_comments", UO_indent_sing_line_comments, AT_NUM,
                  "Spaces to indent single line ('//') comments on lines before code");
   unc_add_option("indent_relative_single_line_comments", UO_indent_relative_single_line_comments, AT_BOOL,
                  "If set, will indent trailing single line ('//') comments relative\n"
                  "to the code instead of trying to keep the same absolute column");
   unc_add_option("indent_switch_case", UO_indent_switch_case, AT_NUM,
                  "Spaces to indent 'case' from 'switch'\n"
                  "Usually 0 or indent_columns.");
   unc_add_option("indent_case_shift", UO_indent_case_shift, AT_NUM,
                  "Spaces to shift the 'case' line, without affecting any other lines\n"
                  "Usually 0.");
   unc_add_option("indent_case_brace", UO_indent_case_brace, AT_NUM,
                  "Spaces to indent '{' from 'case'.\n"
                  "By default, the brace will appear under the 'c' in case.\n"
                  "Usually set to 0 or indent_columns.");
   unc_add_option("indent_col1_comment", UO_indent_col1_comment, AT_BOOL,
                  "Whether to indent comments found in first column");
   unc_add_option("indent_label", UO_indent_label, AT_NUM,
                  "How to indent goto labels\n"
                  " >0 : absolute column where 1 is the leftmost column\n"
                  " <=0 : subtract from brace indent", "", -16, 16);
   unc_add_option("indent_access_spec", UO_indent_access_spec, AT_NUM,
                  "Same as indent_label, but for access specifiers that are followed by a colon", "", -16, 16);
   unc_add_option("indent_access_spec_body", UO_indent_access_spec_body, AT_BOOL,
                  "Indent the code after an access specifier by one level.\n"
                  "If set, this option forces 'indent_access_spec=0'");
   unc_add_option("indent_paren_nl", UO_indent_paren_nl, AT_BOOL,
                  "If an open paren is followed by a newline, indent the next line so that it lines up after the open paren (not recommended)");
   unc_add_option("indent_paren_close", UO_indent_paren_close, AT_NUM,
                  "Controls the indent of a close paren after a newline.\n"
                  "0: Indent to body level\n"
                  "1: Align under the open paren\n"
                  "2: Indent to the brace level");
   unc_add_option("indent_comma_paren", UO_indent_comma_paren, AT_BOOL,
                  "Controls the indent of a comma when inside a paren."
                  "If TRUE, aligns under the open paren");
   unc_add_option("indent_bool_paren", UO_indent_bool_paren, AT_BOOL,
                  "Controls the indent of a BOOL operator when inside a paren."
                  "If TRUE, aligns under the open paren");
   unc_add_option("indent_first_bool_expr", UO_indent_first_bool_expr, AT_BOOL,
                  "If 'indent_bool_paren' is true, controls the indent of the first expression. "
                  "If TRUE, aligns the first expression to the following ones");
   unc_add_option("indent_square_nl", UO_indent_square_nl, AT_BOOL,
                  "If an open square is followed by a newline, indent the next line so that it lines up after the open square (not recommended)");
   unc_add_option("indent_preserve_sql", UO_indent_preserve_sql, AT_BOOL,
                  "Don't change the relative indent of ESQL/C 'EXEC SQL' bodies");
   unc_add_option("indent_align_assign", UO_indent_align_assign, AT_BOOL,
                  "Align continued statements at the '='. Default=True\n"
                  "If FALSE or the '=' is followed by a newline, the next line is indent one tab.");
   unc_add_option("indent_oc_block", UO_indent_oc_block, AT_BOOL,
                  "Indent OC blocks at brace level instead of usual rules.");
   unc_add_option("indent_oc_block_msg", UO_indent_oc_block_msg, AT_NUM,
                  "Indent OC blocks in a message relative to the parameter name.\n"
                  "0=use indent_oc_block rules, 1+=spaces to indent", 0, 16);
   unc_add_option("indent_oc_msg_colon", UO_indent_oc_msg_colon, AT_NUM,
                  "Minimum indent for subsequent parameters", 0, 5000);
   unc_add_option("indent_oc_msg_prioritize_first_colon", UO_indent_oc_msg_prioritize_first_colon, AT_BOOL,
                  "If true, prioritize aligning with initial colon (and stripping spaces from lines, if necessary).\n"
                  "Default is true.");
   unc_add_option("indent_oc_block_msg_xcode_style", UO_indent_oc_block_msg_xcode_style, AT_BOOL,
                  "If indent_oc_block_msg and this option are on, blocks will be indented the way that Xcode does by default (from keyword if the parameter is on its own line; otherwise, from the previous indentation level).");
   unc_add_option("indent_oc_block_msg_from_keyword", UO_indent_oc_block_msg_from_keyword, AT_BOOL,
                  "If indent_oc_block_msg and this option are on, blocks will be indented from where the brace is relative to a msg keyword.");
   unc_add_option("indent_oc_block_msg_from_colon", UO_indent_oc_block_msg_from_colon, AT_BOOL,
                  "If indent_oc_block_msg and this option are on, blocks will be indented from where the brace is relative to a msg colon.");
   unc_add_option("indent_oc_block_msg_from_caret", UO_indent_oc_block_msg_from_caret, AT_BOOL,
                  "If indent_oc_block_msg and this option are on, blocks will be indented from where the block caret is.");
   unc_add_option("indent_oc_block_msg_from_brace", UO_indent_oc_block_msg_from_brace, AT_BOOL,
                  "If indent_oc_block_msg and this option are on, blocks will be indented from where the brace is.");
   unc_add_option("indent_min_vbrace_open", UO_indent_min_vbrace_open, AT_NUM,
                  "When identing after virtual brace open and newline add further spaces to reach this min. indent.");
   unc_add_option("indent_vbrace_open_on_tabstop", UO_indent_vbrace_open_on_tabstop, AT_BOOL,
                  "TRUE: When identing after virtual brace open and newline add further spaces "
                  "after regular indent to reach next tabstop.");

   unc_begin_group(UG_newline, "Newline adding and removing options");
   unc_add_option("nl_collapse_empty_body", UO_nl_collapse_empty_body, AT_BOOL,
                  "Whether to collapse empty blocks between '{' and '}'");

   unc_add_option("nl_assign_leave_one_liners", UO_nl_assign_leave_one_liners, AT_BOOL,
                  "Don't split one-line braced assignments - 'foo_t f = { 1, 2 };'");
   unc_add_option("nl_class_leave_one_liners", UO_nl_class_leave_one_liners, AT_BOOL,
                  "Don't split one-line braced statements inside a class xx { } body");
   unc_add_option("nl_enum_leave_one_liners", UO_nl_enum_leave_one_liners, AT_BOOL,
                  "Don't split one-line enums: 'enum foo { BAR = 15 };'");
   unc_add_option("nl_getset_leave_one_liners", UO_nl_getset_leave_one_liners, AT_BOOL,
                  "Don't split one-line get or set functions");
   unc_add_option("nl_func_leave_one_liners", UO_nl_func_leave_one_liners, AT_BOOL,
                  "Don't split one-line function definitions - 'int foo() { return 0; }'");
   unc_add_option("nl_cpp_lambda_leave_one_liners", UO_nl_cpp_lambda_leave_one_liners, AT_BOOL,
                  "Don't split one-line C++11 lambdas - '[]() { return 0; }'");
   unc_add_option("nl_if_leave_one_liners", UO_nl_if_leave_one_liners, AT_BOOL,
                  "Don't split one-line if/else statements - 'if(a) b++;'");
   unc_add_option("nl_oc_msg_leave_one_liner", UO_nl_oc_msg_leave_one_liner, AT_BOOL,
                  "Don't split one-line OC messages");

   unc_add_option("nl_start_of_file", UO_nl_start_of_file, AT_IARF,
                  "Add or remove newlines at the start of the file");
   unc_add_option("nl_start_of_file_min", UO_nl_start_of_file_min, AT_NUM,
                  "The number of newlines at the start of the file (only used if nl_start_of_file is 'add' or 'force'");
   unc_add_option("nl_end_of_file", UO_nl_end_of_file, AT_IARF,
                  "Add or remove newline at the end of the file");
   unc_add_option("nl_end_of_file_min", UO_nl_end_of_file_min, AT_NUM,
                  "The number of newlines at the end of the file (only used if nl_end_of_file is 'add' or 'force')");
   unc_add_option("nl_assign_brace", UO_nl_assign_brace, AT_IARF,
                  "Add or remove newline between '=' and '{'");
   unc_add_option("nl_assign_square", UO_nl_assign_square, AT_IARF,
                  "Add or remove newline between '=' and '[' (D only)");
   unc_add_option("nl_after_square_assign", UO_nl_after_square_assign, AT_IARF,
                  "Add or remove newline after '= [' (D only). Will also affect the newline before the ']'");
   unc_add_option("nl_func_var_def_blk", UO_nl_func_var_def_blk, AT_NUM,
                  "The number of blank lines after a block of variable definitions at the top of a function body\n"
                  "0 = No change (default)");
   unc_add_option("nl_typedef_blk_start", UO_nl_typedef_blk_start, AT_NUM,
                  "The number of newlines before a block of typedefs\n"
                  "0 = No change (default)");
   unc_add_option("nl_typedef_blk_end", UO_nl_typedef_blk_end, AT_NUM,
                  "The number of newlines after a block of typedefs\n"
                  "0 = No change (default)");
   unc_add_option("nl_typedef_blk_in", UO_nl_typedef_blk_in, AT_NUM,
                  "The maximum consecutive newlines within a block of typedefs\n"
                  "0 = No change (default)");
   unc_add_option("nl_var_def_blk_start", UO_nl_var_def_blk_start, AT_NUM,
                  "The number of newlines before a block of variable definitions not at the top of a function body\n"
                  "0 = No change (default)");
   unc_add_option("nl_var_def_blk_end", UO_nl_var_def_blk_end, AT_NUM,
                  "The number of newlines after a block of variable definitions not at the top of a function body\n"
                  "0 = No change (default)");
   unc_add_option("nl_var_def_blk_in", UO_nl_var_def_blk_in, AT_NUM,
                  "The maximum consecutive newlines within a block of variable definitions\n"
                  "0 = No change (default)");
   unc_add_option("nl_fcall_brace", UO_nl_fcall_brace, AT_IARF,
                  "Add or remove newline between a function call's ')' and '{', as in:\n"
                  "list_for_each(item, &list) { }");
   unc_add_option("nl_enum_brace", UO_nl_enum_brace, AT_IARF,
                  "Add or remove newline between 'enum' and '{'");
   unc_add_option("nl_struct_brace", UO_nl_struct_brace, AT_IARF,
                  "Add or remove newline between 'struct and '{'");
   unc_add_option("nl_union_brace", UO_nl_union_brace, AT_IARF,
                  "Add or remove newline between 'union' and '{'");
   unc_add_option("nl_if_brace", UO_nl_if_brace, AT_IARF,
                  "Add or remove newline between 'if' and '{'");
   unc_add_option("nl_brace_else", UO_nl_brace_else, AT_IARF,
                  "Add or remove newline between '}' and 'else'");
   unc_add_option("nl_elseif_brace", UO_nl_elseif_brace, AT_IARF,
                  "Add or remove newline between 'else if' and '{'\n"
                  "If set to ignore, nl_if_brace is used instead");
   unc_add_option("nl_else_brace", UO_nl_else_brace, AT_IARF,
                  "Add or remove newline between 'else' and '{'");
   unc_add_option("nl_else_if", UO_nl_else_if, AT_IARF,
                  "Add or remove newline between 'else' and 'if'");
   unc_add_option("nl_brace_finally", UO_nl_brace_finally, AT_IARF,
                  "Add or remove newline between '}' and 'finally'");
   unc_add_option("nl_finally_brace", UO_nl_finally_brace, AT_IARF,
                  "Add or remove newline between 'finally' and '{'");
   unc_add_option("nl_try_brace", UO_nl_try_brace, AT_IARF,
                  "Add or remove newline between 'try' and '{'");
   unc_add_option("nl_getset_brace", UO_nl_getset_brace, AT_IARF,
                  "Add or remove newline between get/set and '{'");
   unc_add_option("nl_for_brace", UO_nl_for_brace, AT_IARF,
                  "Add or remove newline between 'for' and '{'");
   unc_add_option("nl_catch_brace", UO_nl_catch_brace, AT_IARF,
                  "Add or remove newline between 'catch' and '{'");
   unc_add_option("nl_brace_catch", UO_nl_brace_catch, AT_IARF,
                  "Add or remove newline between '}' and 'catch'");
   unc_add_option("nl_brace_square", UO_nl_brace_square, AT_IARF,
                  "Add or remove newline between '}' and ']'");
   unc_add_option("nl_brace_fparen", UO_nl_brace_fparen, AT_IARF,
                  "Add or remove newline between '}' and ')' in a function invocation");
   unc_add_option("nl_while_brace", UO_nl_while_brace, AT_IARF,
                  "Add or remove newline between 'while' and '{'");
   unc_add_option("nl_scope_brace", UO_nl_scope_brace, AT_IARF,
                  "Add or remove newline between 'scope (x)' and '{' (D)");
   unc_add_option("nl_unittest_brace", UO_nl_unittest_brace, AT_IARF,
                  "Add or remove newline between 'unittest' and '{' (D)");
   unc_add_option("nl_version_brace", UO_nl_version_brace, AT_IARF,
                  "Add or remove newline between 'version (x)' and '{' (D)");
   unc_add_option("nl_using_brace", UO_nl_using_brace, AT_IARF,
                  "Add or remove newline between 'using' and '{'");
   unc_add_option("nl_brace_brace", UO_nl_brace_brace, AT_IARF,
                  "Add or remove newline between two open or close braces.\n"
                  "Due to general newline/brace handling, REMOVE may not work.");
   unc_add_option("nl_do_brace", UO_nl_do_brace, AT_IARF,
                  "Add or remove newline between 'do' and '{'");
   unc_add_option("nl_brace_while", UO_nl_brace_while, AT_IARF,
                  "Add or remove newline between '}' and 'while' of 'do' statement");
   unc_add_option("nl_switch_brace", UO_nl_switch_brace, AT_IARF,
                  "Add or remove newline between 'switch' and '{'");
   unc_add_option("nl_multi_line_cond", UO_nl_multi_line_cond, AT_BOOL,
                  "Add a newline between ')' and '{' if the ')' is on a different line than the if/for/etc.\n"
                  "Overrides nl_for_brace, nl_if_brace, nl_switch_brace, nl_while_switch, and nl_catch_brace.");
   unc_add_option("nl_multi_line_define", UO_nl_multi_line_define, AT_BOOL,
                  "Force a newline in a define after the macro name for multi-line defines.");
   unc_add_option("nl_before_case", UO_nl_before_case, AT_BOOL,
                  "Whether to put a newline before 'case' statement");
   unc_add_option("nl_before_throw", UO_nl_before_throw, AT_IARF,
                  "Add or remove newline between ')' and 'throw'");
   unc_add_option("nl_after_case", UO_nl_after_case, AT_BOOL,
                  "Whether to put a newline after 'case' statement");
   unc_add_option("nl_case_colon_brace", UO_nl_case_colon_brace, AT_IARF,
                  "Add or remove a newline between a case ':' and '{'. Overrides nl_after_case.");
   unc_add_option("nl_namespace_brace", UO_nl_namespace_brace, AT_IARF,
                  "Newline between namespace and {");
   unc_add_option("nl_template_class", UO_nl_template_class, AT_IARF,
                  "Add or remove newline between 'template<>' and whatever follows.");
   unc_add_option("nl_class_brace", UO_nl_class_brace, AT_IARF,
                  "Add or remove newline between 'class' and '{'");
   unc_add_option("nl_class_init_args", UO_nl_class_init_args, AT_IARF,
                  "Add or remove newline after each ',' in the class base list");
   unc_add_option("nl_constr_init_args", UO_nl_constr_init_args, AT_IARF,
                  "Add or remove newline after each ',' in the constructor member initialization");
   unc_add_option("nl_func_type_name", UO_nl_func_type_name, AT_IARF,
                  "Add or remove newline between return type and function name in a function definition");
   unc_add_option("nl_func_type_name_class", UO_nl_func_type_name_class, AT_IARF,
                  "Add or remove newline between return type and function name inside a class {}\n"
                  "Uses nl_func_type_name or nl_func_proto_type_name if set to ignore.");
   unc_add_option("nl_func_scope_name", UO_nl_func_scope_name, AT_IARF,
                  "Add or remove newline between function scope and name in a definition\n"
                  "Controls the newline after '::' in 'void A::f() { }'");
   unc_add_option("nl_func_proto_type_name", UO_nl_func_proto_type_name, AT_IARF,
                  "Add or remove newline between return type and function name in a prototype");
   unc_add_option("nl_func_paren", UO_nl_func_paren, AT_IARF,
                  "Add or remove newline between a function name and the opening '('");
   unc_add_option("nl_func_def_paren", UO_nl_func_def_paren, AT_IARF,
                  "Add or remove newline between a function name and the opening '(' in the definition");
   unc_add_option("nl_func_decl_start", UO_nl_func_decl_start, AT_IARF,
                  "Add or remove newline after '(' in a function declaration");
   unc_add_option("nl_func_def_start", UO_nl_func_def_start, AT_IARF,
                  "Add or remove newline after '(' in a function definition");
   unc_add_option("nl_func_decl_start_single", UO_nl_func_decl_start_single, AT_IARF,
                  "Overrides nl_func_decl_start when there is only one parameter.");
   unc_add_option("nl_func_def_start_single", UO_nl_func_def_start_single, AT_IARF,
                  "Overrides nl_func_def_start when there is only one parameter.");
   unc_add_option("nl_func_decl_args", UO_nl_func_decl_args, AT_IARF,
                  "Add or remove newline after each ',' in a function declaration");
   unc_add_option("nl_func_def_args", UO_nl_func_def_args, AT_IARF,
                  "Add or remove newline after each ',' in a function definition");
   unc_add_option("nl_func_decl_end", UO_nl_func_decl_end, AT_IARF,
                  "Add or remove newline before the ')' in a function declaration");
   unc_add_option("nl_func_def_end", UO_nl_func_def_end, AT_IARF,
                  "Add or remove newline before the ')' in a function definition");
   unc_add_option("nl_func_decl_end_single", UO_nl_func_decl_end_single, AT_IARF,
                  "Overrides nl_func_decl_end when there is only one parameter.");
   unc_add_option("nl_func_def_end_single", UO_nl_func_def_end_single, AT_IARF,
                  "Overrides nl_func_def_end when there is only one parameter.");
   unc_add_option("nl_func_decl_empty", UO_nl_func_decl_empty, AT_IARF,
                  "Add or remove newline between '()' in a function declaration.");
   unc_add_option("nl_func_def_empty", UO_nl_func_def_empty, AT_IARF,
                  "Add or remove newline between '()' in a function definition.");
   unc_add_option("nl_oc_msg_args", UO_nl_oc_msg_args, AT_BOOL,
                  "Whether to put each OC message parameter on a separate line\n"
                  "See nl_oc_msg_leave_one_liner");
   unc_add_option("nl_fdef_brace", UO_nl_fdef_brace, AT_IARF,
                  "Add or remove newline between function signature and '{'");
   unc_add_option("nl_cpp_ldef_brace", UO_nl_cpp_ldef_brace, AT_IARF,
                  "Add or remove newline between C++11 lambda signature and '{'");
   unc_add_option("nl_return_expr", UO_nl_return_expr, AT_IARF,
                  "Add or remove a newline between the return keyword and return expression.");
   unc_add_option("nl_after_semicolon", UO_nl_after_semicolon, AT_BOOL,
                  "Whether to put a newline after semicolons, except in 'for' statements");
   unc_add_option("nl_paren_dbrace_open", UO_nl_paren_dbrace_open, AT_IARF,
                  "Java: Control the newline between the ')' and '{{' of the double brace initializer.");
   unc_add_option("nl_after_brace_open", UO_nl_after_brace_open, AT_BOOL,
                  "Whether to put a newline after brace open.\n"
                  "This also adds a newline before the matching brace close.");
   unc_add_option("nl_after_brace_open_cmt", UO_nl_after_brace_open_cmt, AT_BOOL,
                  "If nl_after_brace_open and nl_after_brace_open_cmt are true, a newline is\n"
                  "placed between the open brace and a trailing single-line comment.");
   unc_add_option("nl_after_vbrace_open", UO_nl_after_vbrace_open, AT_BOOL,
                  "Whether to put a newline after a virtual brace open with a non-empty body.\n"
                  "These occur in un-braced if/while/do/for statement bodies.");
   unc_add_option("nl_after_vbrace_open_empty", UO_nl_after_vbrace_open_empty, AT_BOOL,
                  "Whether to put a newline after a virtual brace open with an empty body.\n"
                  "These occur in un-braced if/while/do/for statement bodies.");
   unc_add_option("nl_after_brace_close", UO_nl_after_brace_close, AT_BOOL,
                  "Whether to put a newline after a brace close.\n"
                  "Does not apply if followed by a necessary ';'.");
   unc_add_option("nl_after_vbrace_close", UO_nl_after_vbrace_close, AT_BOOL,
                  "Whether to put a newline after a virtual brace close.\n"
                  "Would add a newline before return in: 'if (foo) a++; return;'");
   unc_add_option("nl_brace_struct_var", UO_nl_brace_struct_var, AT_IARF,
                  "Control the newline between the close brace and 'b' in: 'struct { int a; } b;'\n"
                  "Affects enums, unions, and structures. If set to ignore, uses nl_after_brace_close");
   unc_add_option("nl_define_macro", UO_nl_define_macro, AT_BOOL,
                  "Whether to alter newlines in '#define' macros");
   unc_add_option("nl_squeeze_ifdef", UO_nl_squeeze_ifdef, AT_BOOL,
                  "Whether to not put blanks after '#ifxx', '#elxx', or before '#endif'. Does not affect the whole-file #ifdef.");
   unc_add_option("nl_before_if", UO_nl_before_if, AT_IARF,
                  "Add or remove blank line before 'if'");
   unc_add_option("nl_after_if", UO_nl_after_if, AT_IARF,
                  "Add or remove blank line after 'if' statement");
   unc_add_option("nl_before_for", UO_nl_before_for, AT_IARF,
                  "Add or remove blank line before 'for'");
   unc_add_option("nl_after_for", UO_nl_after_for, AT_IARF,
                  "Add or remove blank line after 'for' statement");
   unc_add_option("nl_before_while", UO_nl_before_while, AT_IARF,
                  "Add or remove blank line before 'while'");
   unc_add_option("nl_after_while", UO_nl_after_while, AT_IARF,
                  "Add or remove blank line after 'while' statement");
   unc_add_option("nl_before_switch", UO_nl_before_switch, AT_IARF,
                  "Add or remove blank line before 'switch'");
   unc_add_option("nl_after_switch", UO_nl_after_switch, AT_IARF,
                  "Add or remove blank line after 'switch' statement");
   unc_add_option("nl_before_do", UO_nl_before_do, AT_IARF,
                  "Add or remove blank line before 'do'");
   unc_add_option("nl_after_do", UO_nl_after_do, AT_IARF,
                  "Add or remove blank line after 'do/while' statement");
   unc_add_option("nl_ds_struct_enum_cmt", UO_nl_ds_struct_enum_cmt, AT_BOOL,
                  "Whether to double-space commented-entries in struct/enum");
   unc_add_option("nl_ds_struct_enum_close_brace", UO_nl_ds_struct_enum_close_brace, AT_BOOL,
                  "Whether to double-space before the close brace of a struct/union/enum\n"
                  "(lower priority than 'eat_blanks_before_close_brace')");
   unc_add_option("nl_class_colon", UO_nl_class_colon, AT_IARF,
                  "Add or remove a newline around a class colon.\n"
                  "Related to pos_class_colon, nl_class_init_args, and pos_class_comma.");
   unc_add_option("nl_constr_colon", UO_nl_constr_colon, AT_IARF,
                  "Add or remove a newline around a class constructor colon.\n"
                  "Related to pos_constr_colon, nl_constr_init_args, and pos_constr_comma.");
   unc_add_option("nl_create_if_one_liner", UO_nl_create_if_one_liner, AT_BOOL,
                  "Change simple unbraced if statements into a one-liner\n"
                  "'if(b)\\n i++;' => 'if(b) i++;'");
   unc_add_option("nl_create_for_one_liner", UO_nl_create_for_one_liner, AT_BOOL,
                  "Change simple unbraced for statements into a one-liner\n"
                  "'for (i=0;i<5;i++)\\n foo(i);' => 'for (i=0;i<5;i++) foo(i);'");
   unc_add_option("nl_create_while_one_liner", UO_nl_create_while_one_liner, AT_BOOL,
                  "Change simple unbraced while statements into a one-liner\n"
                  "'while (i<5)\\n foo(i++);' => 'while (i<5) foo(i++);'");

   unc_begin_group(UG_blankline, "Blank line options", "Note that it takes 2 newlines to get a blank line");
   unc_add_option("nl_max", UO_nl_max, AT_NUM,
                  "The maximum consecutive newlines");
   unc_add_option("nl_max_blank_in_func_minus_one", UO_nl_max_blank_in_func_minus_one, AT_NUM,
                   "The maximum consecutive newlines in function");
   unc_add_option("nl_after_func_proto", UO_nl_after_func_proto, AT_NUM,
                  "The number of newlines after a function prototype, if followed by another function prototype");
   unc_add_option("nl_after_func_proto_group", UO_nl_after_func_proto_group, AT_NUM,
                  "The number of newlines after a function prototype, if not followed by another function prototype");
   unc_add_option("nl_after_func_body", UO_nl_after_func_body, AT_NUM,
                  "The number of newlines after '}' of a multi-line function body");
   unc_add_option("nl_after_func_body_class", UO_nl_after_func_body_class, AT_NUM,
                  "The number of newlines after '}' of a multi-line function body in a class declaration");
   unc_add_option("nl_after_func_body_one_liner", UO_nl_after_func_body_one_liner, AT_NUM,
                  "The number of newlines after '}' of a single line function body");
   unc_add_option("nl_before_block_comment", UO_nl_before_block_comment, AT_NUM,
                  "The minimum number of newlines before a multi-line comment.\n"
                  "Doesn't apply if after a brace open or another multi-line comment.");
   unc_add_option("nl_before_c_comment", UO_nl_before_c_comment, AT_NUM,
                  "The minimum number of newlines before a single-line C comment.\n"
                  "Doesn't apply if after a brace open or other single-line C comments.");
   unc_add_option("nl_before_cpp_comment", UO_nl_before_cpp_comment, AT_NUM,
                  "The minimum number of newlines before a CPP comment.\n"
                  "Doesn't apply if after a brace open or other CPP comments.");
   unc_add_option("nl_after_multiline_comment", UO_nl_after_multiline_comment, AT_BOOL,
                  "Whether to force a newline after a multi-line comment.");
   unc_add_option("nl_after_label_colon", UO_nl_after_label_colon, AT_BOOL,
                  "Whether to force a newline after a label's colon.");

   unc_add_option("nl_after_struct", UO_nl_after_struct, AT_NUM,
                  "The number of newlines after '}' or ';' of a struct/enum/union definition");
   unc_add_option("nl_after_class", UO_nl_after_class, AT_NUM,
                  "The number of newlines after '}' or ';' of a class definition");

   unc_add_option("nl_before_access_spec", UO_nl_before_access_spec, AT_NUM,
                  "The number of newlines before a 'private:', 'public:', 'protected:', 'signals:', or 'slots:' label.\n"
                  "Will not change the newline count if after a brace open.\n"
                  "0 = No change.");
   unc_add_option("nl_after_access_spec", UO_nl_after_access_spec, AT_NUM,
                  "The number of newlines after a 'private:', 'public:', 'protected:', 'signals:', or 'slots:' label.\n"
                  "0 = No change.");

   unc_add_option("nl_comment_func_def", UO_nl_comment_func_def, AT_NUM,
                  "The number of newlines between a function def and the function comment.\n"
                  "0 = No change.");

   unc_add_option("nl_after_try_catch_finally", UO_nl_after_try_catch_finally, AT_NUM,
                  "The number of newlines after a try-catch-finally block that isn't followed by a brace close.\n"
                  "0 = No change.");
   unc_add_option("nl_around_cs_property", UO_nl_around_cs_property, AT_NUM,
                  "The number of newlines before and after a property, indexer or event decl.\n"
                  "0 = No change.");
   unc_add_option("nl_between_get_set", UO_nl_between_get_set, AT_NUM,
                  "The number of newlines between the get/set/add/remove handlers in C#.\n"
                  "0 = No change.");
   unc_add_option("nl_property_brace", UO_nl_property_brace, AT_IARF,
                  "Add or remove newline between C# property and the '{'");

   unc_add_option("eat_blanks_after_open_brace", UO_eat_blanks_after_open_brace, AT_BOOL,
                  "Whether to remove blank lines after '{'");
   unc_add_option("eat_blanks_before_close_brace", UO_eat_blanks_before_close_brace, AT_BOOL,
                  "Whether to remove blank lines before '}'");
   unc_add_option("nl_remove_extra_newlines", UO_nl_remove_extra_newlines, AT_NUM,
                  "How aggressively to remove extra newlines not in preproc.\n"
                  "0: No change\n"
                  "1: Remove most newlines not handled by other config\n"
                  "2: Remove all newlines and reformat completely by config");
   unc_add_option("nl_before_return", UO_nl_before_return, AT_BOOL,
                  "Whether to put a blank line before 'return' statements, unless after an open brace.");
   unc_add_option("nl_after_return", UO_nl_after_return, AT_BOOL,
                  "Whether to put a blank line after 'return' statements, unless followed by a close brace.");
   unc_add_option("nl_after_annotation", UO_nl_after_annotation, AT_IARF,
                  "Whether to put a newline after a Java annotation statement.\n"
                  "Only affects annotations that are after a newline.");
   unc_add_option("nl_between_annotation", UO_nl_between_annotation, AT_IARF,
                  "Controls the newline between two annotations.");

   unc_begin_group(UG_position, "Positioning options");
   unc_add_option("pos_arith", UO_pos_arith, AT_POS,
                  "The position of arithmetic operators in wrapped expressions");
   unc_add_option("pos_assign", UO_pos_assign, AT_POS,
                  "The position of assignment in wrapped expressions.\n"
                  "Do not affect '=' followed by '{'");
   unc_add_option("pos_bool", UO_pos_bool, AT_POS,
                  "The position of boolean operators in wrapped expressions");
   unc_add_option("pos_compare", UO_pos_compare, AT_POS,
                  "The position of comparison operators in wrapped expressions");
   unc_add_option("pos_conditional", UO_pos_conditional, AT_POS,
                  "The position of conditional (b ? t : f) operators in wrapped expressions");
   unc_add_option("pos_comma", UO_pos_comma, AT_POS,
                  "The position of the comma in wrapped expressions");
   unc_add_option("pos_class_comma", UO_pos_class_comma, AT_POS,
                  "The position of the comma in the class base list");
   unc_add_option("pos_constr_comma", UO_pos_constr_comma, AT_POS,
                  "The position of the comma in the constructor initialization list");
   unc_add_option("pos_class_colon", UO_pos_class_colon, AT_POS,
                  "The position of colons between class and base class list");
   unc_add_option("pos_constr_colon", UO_pos_constr_colon, AT_POS,
                  "The position of colons between constructor and member initialization");

   unc_begin_group(UG_linesplit, "Line Splitting options");
   unc_add_option("code_width", UO_code_width, AT_NUM,
                  "Try to limit code width to N number of columns", "", 16, 256);
   unc_add_option("ls_for_split_full", UO_ls_for_split_full, AT_BOOL,
                  "Whether to fully split long 'for' statements at semi-colons");
   unc_add_option("ls_func_split_full", UO_ls_func_split_full, AT_BOOL,
                  "Whether to fully split long function protos/calls at commas");
   unc_add_option("ls_code_width", UO_ls_code_width, AT_BOOL,
                  "Whether to split lines as close to code_width as possible and ignore some groupings");

   unc_begin_group(UG_align, "Code alignment (not left column spaces/tabs)");
   unc_add_option("align_keep_tabs", UO_align_keep_tabs, AT_BOOL,
                  "Whether to keep non-indenting tabs");
   unc_add_option("align_with_tabs", UO_align_with_tabs, AT_BOOL,
                  "Whether to use tabs for aligning");
   unc_add_option("align_on_tabstop", UO_align_on_tabstop, AT_BOOL,
                  "Whether to bump out to the next tab when aligning");
   unc_add_option("align_number_left", UO_align_number_left, AT_BOOL,
                  "Whether to left-align numbers");
   unc_add_option("align_keep_extra_space", UO_align_keep_extra_space, AT_BOOL,
                  "Whether to keep whitespace not required for alignment.");
   unc_add_option("align_func_params", UO_align_func_params, AT_BOOL,
                  "Align variable definitions in prototypes and functions");
   unc_add_option("align_same_func_call_params", UO_align_same_func_call_params, AT_BOOL,
                  "Align parameters in single-line functions that have the same name.\n"
                  "The function names must already be aligned with each other.");
   unc_add_option("align_var_def_span", UO_align_var_def_span, AT_NUM,
                  "The span for aligning variable definitions (0=don't align)", "", 0, 5000);
   unc_add_option("align_var_def_star_style", UO_align_var_def_star_style, AT_NUM,
                  "How to align the star in variable definitions.\n"
                  " 0=Part of the type     'void *   foo;'\n"
                  " 1=Part of the variable 'void     *foo;'\n"
                  " 2=Dangling             'void    *foo;'", "", 0, 2);
   unc_add_option("align_var_def_amp_style", UO_align_var_def_amp_style, AT_NUM,
                  "How to align the '&' in variable definitions.\n"
                  " 0=Part of the type\n"
                  " 1=Part of the variable\n"
                  " 2=Dangling", "", 0, 2);
   unc_add_option("align_var_def_thresh", UO_align_var_def_thresh, AT_NUM,
                  "The threshold for aligning variable definitions (0=no limit)", "", 0, 5000);
   unc_add_option("align_var_def_gap", UO_align_var_def_gap, AT_NUM,
                  "The gap for aligning variable definitions");
   unc_add_option("align_var_def_colon", UO_align_var_def_colon, AT_BOOL,
                  "Whether to align the colon in struct bit fields");
   unc_add_option("align_var_def_attribute", UO_align_var_def_attribute, AT_BOOL,
                  "Whether to align any attribute after the variable name");
   unc_add_option("align_var_def_inline", UO_align_var_def_inline, AT_BOOL,
                  "Whether to align inline struct/enum/union variable definitions");
   unc_add_option("align_assign_span", UO_align_assign_span, AT_NUM,
                  "The span for aligning on '=' in assignments (0=don't align)", "", 0, 5000);
   unc_add_option("align_assign_thresh", UO_align_assign_thresh, AT_NUM,
                  "The threshold for aligning on '=' in assignments (0=no limit)", "", 0, 5000);
   unc_add_option("align_enum_equ_span", UO_align_enum_equ_span, AT_NUM,
                  "The span for aligning on '=' in enums (0=don't align)", "", 0, 5000);
   unc_add_option("align_enum_equ_thresh", UO_align_enum_equ_thresh, AT_NUM,
                  "The threshold for aligning on '=' in enums (0=no limit)", "", 0, 5000);
   unc_add_option("align_var_struct_span", UO_align_var_struct_span, AT_NUM,
                  "The span for aligning struct/union (0=don't align)", "", 0, 5000);
   unc_add_option("align_var_struct_thresh", UO_align_var_struct_thresh, AT_NUM,
                  "The threshold for aligning struct/union member definitions (0=no limit)", "", 0, 5000);
   unc_add_option("align_var_struct_gap", UO_align_var_struct_gap, AT_NUM,
                  "The gap for aligning struct/union member definitions");
   unc_add_option("align_struct_init_span", UO_align_struct_init_span, AT_NUM,
                  "The span for aligning struct initializer values (0=don't align)", "", 0, 5000);
   unc_add_option("align_typedef_gap", UO_align_typedef_gap, AT_NUM,
                  "The minimum space between the type and the synonym of a typedef");
   unc_add_option("align_typedef_span", UO_align_typedef_span, AT_NUM,
                  "The span for aligning single-line typedefs (0=don't align)");
   unc_add_option("align_typedef_func", UO_align_typedef_func, AT_NUM,
                  "How to align typedef'd functions with other typedefs\n"
                  "0: Don't mix them at all\n"
                  "1: align the open paren with the types\n"
                  "2: align the function type name with the other type names");
   unc_add_option("align_typedef_star_style", UO_align_typedef_star_style, AT_NUM,
                  "Controls the positioning of the '*' in typedefs. Just try it.\n"
                  "0: Align on typedef type, ignore '*'\n"
                  "1: The '*' is part of type name: typedef int  *pint;\n"
                  "2: The '*' is part of the type, but dangling: typedef int *pint;", "", 0, 2);
   unc_add_option("align_typedef_amp_style", UO_align_typedef_amp_style, AT_NUM,
                  "Controls the positioning of the '&' in typedefs. Just try it.\n"
                  "0: Align on typedef type, ignore '&'\n"
                  "1: The '&' is part of type name: typedef int  &pint;\n"
                  "2: The '&' is part of the type, but dangling: typedef int &pint;", "", 0, 2);

   unc_add_option("align_right_cmt_span", UO_align_right_cmt_span, AT_NUM,
                  "The span for aligning comments that end lines (0=don't align)", "", 0, 5000);
   unc_add_option("align_right_cmt_mix", UO_align_right_cmt_mix, AT_BOOL,
                  "If aligning comments, mix with comments after '}' and #endif with less than 3 spaces before the comment");
   unc_add_option("align_right_cmt_gap", UO_align_right_cmt_gap, AT_NUM,
                  "If a trailing comment is more than this number of columns away from the text it follows,\n"
                  "it will qualify for being aligned. This has to be > 0 to do anything.");
   unc_add_option("align_right_cmt_at_col", UO_align_right_cmt_at_col, AT_NUM,
                  "Align trailing comment at or beyond column N; 'pulls in' comments as a bonus side effect (0=ignore)", "", 0, 200);
   unc_add_option("align_func_proto_span", UO_align_func_proto_span, AT_NUM,
                  "The span for aligning function prototypes (0=don't align)", "", 0, 5000);
   unc_add_option("align_func_proto_gap", UO_align_func_proto_gap, AT_NUM,
                  "Minimum gap between the return type and the function name.");
   unc_add_option("align_on_operator", UO_align_on_operator, AT_BOOL,
                  "Align function protos on the 'operator' keyword instead of what follows");
   unc_add_option("align_mix_var_proto", UO_align_mix_var_proto, AT_BOOL,
                  "Whether to mix aligning prototype and variable declarations.\n"
                  "If true, align_var_def_XXX options are used instead of align_func_proto_XXX options.");
   unc_add_option("align_single_line_func", UO_align_single_line_func, AT_BOOL,
                  "Align single-line functions with function prototypes, uses align_func_proto_span");
   unc_add_option("align_single_line_brace", UO_align_single_line_brace, AT_BOOL,
                  "Aligning the open brace of single-line functions.\n"
                  "Requires align_single_line_func=true, uses align_func_proto_span");
   unc_add_option("align_single_line_brace_gap", UO_align_single_line_brace_gap, AT_NUM,
                  "Gap for align_single_line_brace.\n");
   unc_add_option("align_oc_msg_spec_span", UO_align_oc_msg_spec_span, AT_NUM,
                  "The span for aligning ObjC msg spec (0=don't align)", "", 0, 5000);
   unc_add_option("align_nl_cont", UO_align_nl_cont, AT_BOOL,
                  "Whether to align macros wrapped with a backslash and a newline.\n"
                  "This will not work right if the macro contains a multi-line comment.");
   unc_add_option("align_pp_define_together", UO_align_pp_define_together, AT_BOOL,
                  "# Align macro functions and variables together\n");
   unc_add_option("align_pp_define_gap", UO_align_pp_define_gap, AT_NUM,
                  "The minimum space between label and value of a preprocessor define");
   unc_add_option("align_pp_define_span", UO_align_pp_define_span, AT_NUM,
                  "The span for aligning on '#define' bodies (0=don't align, other=number of lines including comments between blocks)", "", 0, 5000);
   unc_add_option("align_left_shift", UO_align_left_shift, AT_BOOL,
                  "Align lines that start with '<<' with previous '<<'. Default=true");

   unc_add_option("align_oc_msg_colon_span", UO_align_oc_msg_colon_span, AT_NUM,
                  "Span for aligning parameters in an Obj-C message call on the ':' (0=don't align)", 0, 5000);
   unc_add_option("align_oc_msg_colon_first", UO_align_oc_msg_colon_first, AT_BOOL,
                  "If true, always align with the first parameter, even if it is too short.");
   unc_add_option("align_oc_decl_colon", UO_align_oc_decl_colon, AT_BOOL,
                  "Aligning parameters in an Obj-C '+' or '-' declaration on the ':'");

   unc_begin_group(UG_comment, "Comment modifications");
   unc_add_option("cmt_width", UO_cmt_width, AT_NUM,
                  "Try to wrap comments at cmt_width columns", "", 16, 256);
   unc_add_option("cmt_reflow_mode", UO_cmt_reflow_mode, AT_NUM,
                  "Set the comment reflow mode (default: 0)\n"
                  "0: no reflowing (apart from the line wrapping due to cmt_width)\n"
                  "1: no touching at all\n"
                  "2: full reflow\n", "", 0, 2);
   unc_add_option("cmt_convert_tab_to_spaces", UO_cmt_convert_tab_to_spaces, AT_BOOL,
                  "Whether to convert all tabs to spaces in comments. Default is to leave tabs inside comments alone, unless used for indenting.");
   unc_add_option("cmt_indent_multi", UO_cmt_indent_multi, AT_BOOL,
                  "If false, disable all multi-line comment changes, including cmt_width. keyword substitution, and leading chars.\n"
                  "Default is true.");
   unc_add_option("cmt_c_group", UO_cmt_c_group, AT_BOOL,
                  "Whether to group c-comments that look like they are in a block");
   unc_add_option("cmt_c_nl_start", UO_cmt_c_nl_start, AT_BOOL,
                  "Whether to put an empty '/*' on the first line of the combined c-comment");
   unc_add_option("cmt_c_nl_end", UO_cmt_c_nl_end, AT_BOOL,
                  "Whether to put a newline before the closing '*/' of the combined c-comment");
   unc_add_option("cmt_cpp_group", UO_cmt_cpp_group, AT_BOOL,
                  "Whether to group cpp-comments that look like they are in a block");
   unc_add_option("cmt_cpp_nl_start", UO_cmt_cpp_nl_start, AT_BOOL,
                  "Whether to put an empty '/*' on the first line of the combined cpp-comment");
   unc_add_option("cmt_cpp_nl_end", UO_cmt_cpp_nl_end, AT_BOOL,
                  "Whether to put a newline before the closing '*/' of the combined cpp-comment");
   unc_add_option("cmt_cpp_to_c", UO_cmt_cpp_to_c, AT_BOOL,
                  "Whether to change cpp-comments into c-comments");
   unc_add_option("cmt_star_cont", UO_cmt_star_cont, AT_BOOL,
                  "Whether to put a star on subsequent comment lines");
   unc_add_option("cmt_sp_before_star_cont", UO_cmt_sp_before_star_cont, AT_NUM,
                  "The number of spaces to insert at the start of subsequent comment lines");
   unc_add_option("cmt_sp_after_star_cont", UO_cmt_sp_after_star_cont, AT_NUM,
                  "The number of spaces to insert after the star on subsequent comment lines");

   unc_add_option("cmt_multi_check_last", UO_cmt_multi_check_last, AT_BOOL,
                  "For multi-line comments with a '*' lead, remove leading spaces if the first and last lines of\n"
                  "the comment are the same length. Default=True");

   unc_add_option("cmt_insert_file_header", UO_cmt_insert_file_header, AT_STRING,
                  "The filename that contains text to insert at the head of a file if the file doesn't start with a C/C++ comment.\n"
                  "Will substitute $(filename) with the current file's name.");
   unc_add_option("cmt_insert_file_footer", UO_cmt_insert_file_footer, AT_STRING,
                  "The filename that contains text to insert at the end of a file if the file doesn't end with a C/C++ comment.\n"
                  "Will substitute $(filename) with the current file's name.");
   unc_add_option("cmt_insert_func_header", UO_cmt_insert_func_header, AT_STRING,
                  "The filename that contains text to insert before a function implementation if the function isn't preceded with a C/C++ comment.\n"
                  "Will substitute $(function) with the function name and $(javaparam) with the javadoc @param and @return stuff.\n"
                  "Will also substitute $(fclass) with the class name: void CFoo::Bar() { ... }");
   unc_add_option("cmt_insert_class_header", UO_cmt_insert_class_header, AT_STRING,
                  "The filename that contains text to insert before a class if the class isn't preceded with a C/C++ comment.\n"
                  "Will substitute $(class) with the class name.");
   unc_add_option("cmt_insert_oc_msg_header", UO_cmt_insert_oc_msg_header, AT_STRING,
                  "The filename that contains text to insert before a Obj-C message specification if the method isn't preceded with a C/C++ comment.\n"
                  "Will substitute $(message) with the function name and $(javaparam) with the javadoc @param and @return stuff.");
   unc_add_option("cmt_insert_before_preproc", UO_cmt_insert_before_preproc, AT_BOOL,
                  "If a preprocessor is encountered when stepping backwards from a function name, then\n"
                  "this option decides whether the comment should be inserted.\n"
                  "Affects cmt_insert_oc_msg_header, cmt_insert_func_header and cmt_insert_class_header.");

   unc_begin_group(UG_codemodify, "Code modifying options (non-whitespace)");
   unc_add_option("mod_full_brace_do", UO_mod_full_brace_do, AT_IARF,
                  "Add or remove braces on single-line 'do' statement");
   unc_add_option("mod_full_brace_for", UO_mod_full_brace_for, AT_IARF,
                  "Add or remove braces on single-line 'for' statement");
   unc_add_option("mod_full_brace_function", UO_mod_full_brace_function, AT_IARF,
                  "Add or remove braces on single-line function definitions. (Pawn)");
   unc_add_option("mod_full_brace_if", UO_mod_full_brace_if, AT_IARF,
                  "Add or remove braces on single-line 'if' statement. Will not remove the braces if they contain an 'else'.");
   unc_add_option("mod_full_brace_if_chain", UO_mod_full_brace_if_chain, AT_BOOL,
                  "Make all if/elseif/else statements in a chain be braced or not. Overrides mod_full_brace_if.\n"
                  "If any must be braced, they are all braced.  If all can be unbraced, then the braces are removed.");
   unc_add_option("mod_full_brace_nl", UO_mod_full_brace_nl, AT_NUM,
                  "Don't remove braces around statements that span N newlines", "", 0, 5000);
   unc_add_option("mod_full_brace_while", UO_mod_full_brace_while, AT_IARF,
                  "Add or remove braces on single-line 'while' statement");
   unc_add_option("mod_full_brace_using", UO_mod_full_brace_using, AT_IARF,
                  "Add or remove braces on single-line 'using ()' statement");
   unc_add_option("mod_paren_on_return", UO_mod_paren_on_return, AT_IARF,
                  "Add or remove unnecessary paren on 'return' statement");
   unc_add_option("mod_pawn_semicolon", UO_mod_pawn_semicolon, AT_BOOL,
                  "Whether to change optional semicolons to real semicolons");
   unc_add_option("mod_full_paren_if_bool", UO_mod_full_paren_if_bool, AT_BOOL,
                  "Add parens on 'while' and 'if' statement around bools");
   unc_add_option("mod_remove_extra_semicolon", UO_mod_remove_extra_semicolon, AT_BOOL,
                  "Whether to remove superfluous semicolons");
   unc_add_option("mod_add_long_function_closebrace_comment", UO_mod_add_long_function_closebrace_comment, AT_NUM,
                  "If a function body exceeds the specified number of newlines and doesn't have a comment after\n"
                  "the close brace, a comment will be added.");
   unc_add_option("mod_add_long_namespace_closebrace_comment", UO_mod_add_long_namespace_closebrace_comment, AT_NUM,
                  "If a namespace body exceeds the specified number of newlines and doesn't have a comment after\n"
                  "the close brace, a comment will be added.");
   unc_add_option("mod_add_long_switch_closebrace_comment", UO_mod_add_long_switch_closebrace_comment, AT_NUM,
                  "If a switch body exceeds the specified number of newlines and doesn't have a comment after\n"
                  "the close brace, a comment will be added.");
   unc_add_option("mod_add_long_ifdef_endif_comment", UO_mod_add_long_ifdef_endif_comment, AT_NUM,
                  "If an #ifdef body exceeds the specified number of newlines and doesn't have a comment after\n"
                  "the #endif, a comment will be added.");
   unc_add_option("mod_add_long_ifdef_else_comment", UO_mod_add_long_ifdef_else_comment, AT_NUM,
                  "If an #ifdef or #else body exceeds the specified number of newlines and doesn't have a comment after\n"
                  "the #else, a comment will be added.");
   unc_add_option("mod_sort_import", UO_mod_sort_import, AT_BOOL,
                  "If TRUE, will sort consecutive single-line 'import' statements [Java, D]");
   unc_add_option("mod_sort_using", UO_mod_sort_using, AT_BOOL,
                  "If TRUE, will sort consecutive single-line 'using' statements [C#]");
   unc_add_option("mod_sort_include", UO_mod_sort_include, AT_BOOL,
                  "If TRUE, will sort consecutive single-line '#include' statements [C/C++] and '#import' statements [Obj-C]\n"
                  "This is generally a bad idea, as it may break your code.");
   unc_add_option("mod_move_case_break", UO_mod_move_case_break, AT_BOOL,
                  "If TRUE, it will move a 'break' that appears after a fully braced 'case' before the close brace.");
   unc_add_option("mod_case_brace", UO_mod_case_brace, AT_IARF,
                  "Will add or remove the braces around a fully braced case statement.\n"
                  "Will only remove the braces if there are no variable declarations in the block.");
   unc_add_option("mod_remove_empty_return", UO_mod_remove_empty_return, AT_BOOL,
                  "If TRUE, it will remove a void 'return;' that appears as the last statement in a function.");
   unc_add_option("mod_remove_brace_only_has_return", UO_mod_remove_brace_only_has_return, AT_BOOL,
                   "If TRUE, it will remove brace that appears as the block only has single return statement.");

   unc_begin_group(UG_preprocessor, "Preprocessor options");
   unc_add_option("pp_indent", UO_pp_indent, AT_IARF,
                  "Control indent of preprocessors inside #if blocks at brace level 0 (file-level)");
   unc_add_option("pp_indent_at_level", UO_pp_indent_at_level, AT_BOOL,
                  "Whether to indent #if/#else/#endif at the brace level (true) or from column 1 (false)");
   unc_add_option("pp_indent_count", UO_pp_indent_count, AT_NUM,
                  "Specifies the number of columns to indent preprocessors per level at brace level 0 (file-level).\n"
                  "If pp_indent_at_level=false, specifies the number of columns to indent preprocessors per level at brace level > 0 (function-level).\n"
                  "Default=1.");
   unc_add_option("pp_space", UO_pp_space, AT_IARF,
                  "Add or remove space after # based on pp_level of #if blocks");
   unc_add_option("pp_space_count", UO_pp_space_count, AT_NUM,
                  "Sets the number of spaces added with pp_space");
   unc_add_option("pp_indent_region", UO_pp_indent_region, AT_NUM,
                  "The indent for #region and #endregion in C# and '#pragma region' in C/C++");
   unc_add_option("pp_region_indent_code", UO_pp_region_indent_code, AT_BOOL,
                  "Whether to indent the code between #region and #endregion");
   unc_add_option("pp_indent_if", UO_pp_indent_if, AT_NUM,
                  "If pp_indent_at_level=true, sets the indent for #if, #else, and #endif when not at file-level.\n"
                  "0:  indent preprocessors using output_tab_size.\n"
                  ">0: column at which all preprocessors will be indented.");
   unc_add_option("pp_if_indent_code", UO_pp_if_indent_code, AT_BOOL,
                  "Control whether to indent the code between #if, #else and #endif.");
   unc_add_option("pp_define_at_level", UO_pp_define_at_level, AT_BOOL,
                  "Whether to indent '#define' at the brace level (true) or from column 1 (false)");
}


const group_map_value *get_group_name(int ug)
{
   for (group_map_it it = group_map.begin();
        it != group_map.end();
        it++)
   {
      if (it->second.id == ug)
      {
         return(&it->second);
      }
   }
   return(NULL);
}


const option_map_value *get_option_name(int uo)
{
   for (option_name_map_it it = option_name_map.begin();
        it != option_name_map.end();
        it++)
   {
      if (it->second.id == uo)
      {
         return(&it->second);
      }
   }
   return(NULL);
}


/**
 * Convert the value string to the correct type in dest.
 */
static void convert_value(const option_map_value *entry, const char *val, op_val_t *dest)
{
   const option_map_value *tmp;
   bool btrue;
   int  mult;

   if (entry->type == AT_LINE)
   {
      if (strcasecmp(val, "CRLF") == 0)
      {
         dest->le = LE_CRLF;
         return;
      }
      if (strcasecmp(val, "LF") == 0)
      {
         dest->le = LE_LF;
         return;
      }
      if (strcasecmp(val, "CR") == 0)
      {
         dest->le = LE_CR;
         return;
      }
      if (strcasecmp(val, "AUTO") != 0)
      {
         LOG_FMT(LWARN, "%s:%d Expected AUTO, LF, CRLF, or CR for %s, got %s\n",
                 cpd.filename, cpd.line_number, entry->name, val);
         cpd.error_count++;
      }
      dest->le = LE_AUTO;
      return;
   }

   if (entry->type == AT_POS)
   {
      if (strcasecmp(val, "JOIN") == 0)
      {
         dest->tp = TP_JOIN;
         return;
      }
      if (strcasecmp(val, "LEAD") == 0)
      {
         dest->tp = TP_LEAD;
         return;
      }
      if (strcasecmp(val, "LEAD_BREAK") == 0)
      {
         dest->tp = TP_LEAD_BREAK;
         return;
      }
      if (strcasecmp(val, "LEAD_FORCE") == 0)
      {
         dest->tp = TP_LEAD_FORCE;
         return;
      }
      if (strcasecmp(val, "TRAIL") == 0)
      {
         dest->tp = TP_TRAIL;
         return;
      }
      if (strcasecmp(val, "TRAIL_BREAK") == 0)
      {
         dest->tp = TP_TRAIL_BREAK;
         return;
      }
      if (strcasecmp(val, "TRAIL_FORCE") == 0)
      {
         dest->tp = TP_TRAIL_FORCE;
         return;
      }
      if (strcasecmp(val, "IGNORE") != 0)
      {
         LOG_FMT(LWARN, "%s:%d Expected IGNORE, JOIN, LEAD, LEAD_BREAK, LEAD_FORCE, "
                        "TRAIL, TRAIL_BREAK, TRAIL_FORCE for %s, got %s\n",
                 cpd.filename, cpd.line_number, entry->name, val);
         cpd.error_count++;
      }
      dest->tp = TP_IGNORE;
      return;
   }

   if (entry->type == AT_NUM)
   {
      if (unc_isdigit(*val) ||
          (unc_isdigit(val[1]) && ((*val == '-') || (*val == '+'))))
      {
         dest->n = strtol(val, NULL, 0);
         return;
      }
      else
      {
         /* Try to see if it is a variable */
         mult = 1;
         if (*val == '-')
         {
            mult = -1;
            val++;
         }

         if (((tmp = unc_find_option(val)) != NULL) && (tmp->type == entry->type))
         {
            dest->n = cpd.settings[tmp->id].n * mult;
            return;
         }
      }
      LOG_FMT(LWARN, "%s:%d Expected a number for %s, got %s\n",
              cpd.filename, cpd.line_number, entry->name, val);
      cpd.error_count++;
      dest->n = 0;
      return;
   }

   if (entry->type == AT_BOOL)
   {
      if ((strcasecmp(val, "true") == 0) ||
          (strcasecmp(val, "t") == 0) ||
          (strcmp(val, "1") == 0))
      {
         dest->b = true;
         return;
      }

      if ((strcasecmp(val, "false") == 0) ||
          (strcasecmp(val, "f") == 0) ||
          (strcmp(val, "0") == 0))
      {
         dest->b = false;
         return;
      }

      btrue = true;
      if ((*val == '-') || (*val == '~'))
      {
         btrue = false;
         val++;
      }

      if (((tmp = unc_find_option(val)) != NULL) && (tmp->type == entry->type))
      {
         dest->b = cpd.settings[tmp->id].b ? btrue : !btrue;
         return;
      }
      LOG_FMT(LWARN, "%s:%d Expected 'True' or 'False' for %s, got %s\n",
              cpd.filename, cpd.line_number, entry->name, val);
      cpd.error_count++;
      dest->b = false;
      return;
   }

   if (entry->type == AT_STRING)
   {
      dest->str = strdup(val);
      return;
   }

   /* Must be AT_IARF */

   if ((strcasecmp(val, "add") == 0) || (strcasecmp(val, "a") == 0))
   {
      dest->a = AV_ADD;
      return;
   }
   if ((strcasecmp(val, "remove") == 0) || (strcasecmp(val, "r") == 0))
   {
      dest->a = AV_REMOVE;
      return;
   }
   if ((strcasecmp(val, "force") == 0) || (strcasecmp(val, "f") == 0))
   {
      dest->a = AV_FORCE;
      return;
   }
   if ((strcasecmp(val, "ignore") == 0) || (strcasecmp(val, "i") == 0))
   {
      dest->a = AV_IGNORE;
      return;
   }
   if (((tmp = unc_find_option(val)) != NULL) && (tmp->type == entry->type))
   {
      dest->a = cpd.settings[tmp->id].a;
      return;
   }
   LOG_FMT(LWARN, "%s:%d Expected 'Add', 'Remove', 'Force', or 'Ignore' for %s, got %s\n",
           cpd.filename, cpd.line_number, entry->name, val);
   cpd.error_count++;
   dest->a = AV_IGNORE;
}


int set_option_value(const char *name, const char *value)
{
   const option_map_value *entry;

   if ((entry = unc_find_option(name)) != NULL)
   {
      convert_value(entry, value, &cpd.settings[entry->id]);
      return(entry->id);
   }
   return(-1);
}


bool is_path_relative(const char *path)
{
#ifdef WIN32
   // X:\path\to\file style absolute disk path
   if (isalpha(path[0]) && (path[1] == ':'))
   {
      return false;
   }

   // \\server\path\to\file style absolute UNC path
   if ((path[0] == '\\') && (path[1] == '\\'))
   {
      return false;
   }
#endif

   // /path/to/file style absolute path
   if (path[0] == '/')
   {
      return false;
   }

   return true;
}


int load_option_file(const char *filename)
{
   FILE *pfile;
   char buffer[256];
   char *ptr;
   int  id;
   char *args[32];
   int  argc;
   int  idx;

   cpd.line_number = 0;

#ifdef WIN32
   /* "/dev/null" not understood by "fopen" in windoze */
   if (strcasecmp(filename, "/dev/null") == 0)
   {
      return(0);
   }
#endif

   pfile = fopen(filename, "r");
   if (pfile == NULL)
   {
      LOG_FMT(LERR, "%s: fopen(%s) failed: %s (%d)\n",
              __func__, filename, strerror(errno), errno);
      cpd.error_count++;
      return(-1);
   }

   /* Read in the file line by line */
   while (fgets(buffer, sizeof(buffer), pfile) != NULL)
   {
      cpd.line_number++;

      /* Chop off trailing comments */
      if ((ptr = strchr(buffer, '#')) != NULL)
      {
         *ptr = 0;
      }

      /* Blow away the '=' to make things simple */
      if ((ptr = strchr(buffer, '=')) != NULL)
      {
         *ptr = ' ';
      }

      /* Blow away all commas */
      ptr = buffer;
      while ((ptr = strchr(ptr, ',')) != NULL)
      {
         *ptr = ' ';
      }

      /* Split the line */
      argc = Args::SplitLine(buffer, args, ARRAY_SIZE(args) - 1);
      if (argc < 2)
      {
         if (argc > 0)
         {
            LOG_FMT(LWARN, "%s:%d Wrong number of arguments: %s...\n",
                    filename, cpd.line_number, buffer);
            cpd.error_count++;
         }
         continue;
      }
      args[argc] = NULL;

      if (strcasecmp(args[0], "type") == 0)
      {
         for (idx = 1; idx < argc; idx++)
         {
            add_keyword(args[idx], CT_TYPE);
         }
      }
      else if (strcasecmp(args[0], "define") == 0)
      {
         add_define(args[1], args[2]);
      }
      else if (strcasecmp(args[0], "macro-open") == 0)
      {
         add_keyword(args[1], CT_MACRO_OPEN);
      }
      else if (strcasecmp(args[0], "macro-close") == 0)
      {
         add_keyword(args[1], CT_MACRO_CLOSE);
      }
      else if (strcasecmp(args[0], "macro-else") == 0)
      {
         add_keyword(args[1], CT_MACRO_ELSE);
      }
      else if (strcasecmp(args[0], "set") == 0)
      {
         if (argc < 3)
         {
            LOG_FMT(LWARN, "%s:%d 'set' requires at least three arguments\n",
                    filename, cpd.line_number);
         }
         else
         {
            c_token_t id = find_token_name(args[1]);
            if (id != CT_NONE)
            {
               LOG_FMT(LNOTE, "%s:%d set '%s':", filename, cpd.line_number, args[1]);
               for (idx = 2; idx < argc; idx++)
               {
                  LOG_FMT(LNOTE, " '%s'", args[idx]);
                  add_keyword(args[idx], id);
               }
               LOG_FMT(LNOTE, "\n");
            }
            else
            {
               LOG_FMT(LWARN, "%s:%d unknown type '%s':", filename, cpd.line_number, args[1]);
            }
         }
      }
      else if (strcasecmp(args[0], "include") == 0)
      {
         int save_line_no = cpd.line_number;

         if (is_path_relative(args[1]))
         {
            /* include is a relative path to the current config file */
            unc_text ut = filename;
            ut.resize(path_dirname_len(filename));
            ut.append(args[1]);
            (void)load_option_file(ut.c_str());
         }
         else
         {
            /* include is an absolute Unix path */
            (void)load_option_file(args[1]);
         }

         cpd.line_number = save_line_no;
      }
      else if (strcasecmp(args[0], "file_ext") == 0)
      {
         if (argc < 3)
         {
            LOG_FMT(LWARN, "%s:%d 'file_ext' requires at least three arguments\n",
                    filename, cpd.line_number);
         }
         else
         {
            for (idx = 2; idx < argc; idx++)
            {
               const char *lang_name = extension_add(args[idx], args[1]);
               if (lang_name)
               {
                  LOG_FMT(LNOTE, "%s:%d file_ext '%s' => '%s'\n",
                          filename, cpd.line_number, args[idx], lang_name);
               }
               else
               {
                  LOG_FMT(LWARN, "%s:%d file_ext has unknown language '%s'\n",
                          filename, cpd.line_number, args[1]);
               }
            }
         }
      }
      else
      {
         /* must be a regular option = value */
         if ((id = set_option_value(args[0], args[1])) < 0)
         {
            LOG_FMT(LWARN, "%s:%d Unknown symbol '%s'\n",
                    filename, cpd.line_number, args[0]);
            cpd.error_count++;
         }
      }
   }

   fclose(pfile);
   return(0);
}


int save_option_file(FILE *pfile, bool withDoc)
{
   string     val_string;
   const char *val_str;
   int        val_len;
   int        name_len;
   int        idx;

   fprintf(pfile, "# Uncrustify %s\n", UNCRUSTIFY_VERSION);

   /* Print the options by group */
   for (group_map_it jt = group_map.begin(); jt != group_map.end(); jt++)
   {
      if (withDoc)
      {
         fputs("\n#\n", pfile);
         fprintf(pfile, "# %s\n", jt->second.short_desc);
         fputs("#\n\n", pfile);
      }

      bool first = true;

      for (option_list_it it = jt->second.options.begin(); it != jt->second.options.end(); it++)
      {
         const option_map_value *option = get_option_name(*it);

         if (withDoc && (option->short_desc != NULL) && (*option->short_desc != 0))
         {
            fprintf(pfile, "%s# ", first ? "" : "\n");
            for (idx = 0; option->short_desc[idx] != 0; idx++)
            {
               fputc(option->short_desc[idx], pfile);
               if ((option->short_desc[idx] == '\n') &&
                   (option->short_desc[idx + 1] != 0))
               {
                  fputs("# ", pfile);
               }
            }
            if (option->short_desc[idx - 1] != '\n')
            {
               fputc('\n', pfile);
            }
         }
         first      = false;
         val_string = op_val_to_string(option->type, cpd.settings[option->id]);
         val_str    = val_string.c_str();
         val_len    = strlen(val_str);
         name_len   = strlen(option->name);

         fprintf(pfile, "%s %*.s= ",
                 option->name, cpd.max_option_name_len - name_len, " ");
         if (option->type == AT_STRING)
         {
            fprintf(pfile, "\"%s\"", val_str);
         }
         else
         {
            fprintf(pfile, "%s", val_str);
         }
         if (withDoc)
         {
            fprintf(pfile, "%*.s # %s",
                    8 - val_len, " ",
                    argtype_to_string(option->type).c_str());
         }
         fputs("\n", pfile);
      }
   }

   if (withDoc)
   {
      fprintf(pfile,
              "\n"
              "# You can force a token to be a type with the 'type' option.\n"
              "# Example:\n"
              "# type myfoo1 myfoo2\n"
              "#\n"
              "# You can create custom macro-based indentation using macro-open,\n"
              "# macro-else and macro-close.\n"
              "# Example:\n"
              "# macro-open  BEGIN_TEMPLATE_MESSAGE_MAP\n"
              "# macro-open  BEGIN_MESSAGE_MAP\n"
              "# macro-close END_MESSAGE_MAP\n"
              "#\n"
              "# You can assign any keyword to any type with the set option.\n"
              "# set func_call_user _ N_\n"
              "#\n"
              "# The full syntax description of all custom definition config entries\n"
              "# is shown below:\n"
              "#\n"
              "# define custom tokens as:\n"
              "# - embed whitespace in token using '\' escape character, or\n"
              "#   put token in quotes\n"
              "# - these: ' \" and ` are recognized as quote delimiters\n"
              "#\n"
              "# type token1 token2 token3 ...\n"
              "#             ^ optionally specify multiple tokens on a single line\n"
              "# define def_token output_token\n"
              "#                  ^ output_token is optional, then NULL is assumed\n"
              "# macro-open token\n"
              "# macro-close token\n"
              "# macro-else token\n"
              "# set id token1 token2 ...\n"
              "#               ^ optionally specify multiple tokens on a single line\n"
              "#     ^ id is one of the names in token_enum.h sans the CT_ prefix,\n"
              "#       e.g. PP_PRAGMA\n"
              "#\n"
              "# all tokens are separated by any mix of ',' commas, '=' equal signs\n"
              "# and whitespace (space, tab)\n"
              "#\n"
              "# You can add support for other file extensions using the 'file_ext' command.\n"
              "# The first arg is the language name used with the '-l' option.\n"
              "# The remaining args are file extensions, matched with 'endswith'.\n"
              "#   file_ext CPP .ch .cxx .cpp.in\n"
              "#\n"
              );
   }

   /* Print custom keywords */
   print_keywords(pfile);

   /* Print custom defines */
   print_defines(pfile);

   /* Print custom file extensions */
   print_extensions(pfile);

   return(0);
}


void print_options(FILE *pfile)
{
   int        max_width = 0;
   int        cur_width;
   const char *text;

   const char *names[] =
   {
      "{ False, True }",
      "{ Ignore, Add, Remove, Force }",
      "Number",
      "{ Auto, LF, CR, CRLF }",
      "{ Ignore, Lead, Trail }",
      "String",
   };

   option_name_map_it it;

   /* Find the max width of the names */
   for (it = option_name_map.begin(); it != option_name_map.end(); it++)
   {
      cur_width = strlen(it->second.name);
      if (cur_width > max_width)
      {
         max_width = cur_width;
      }
   }
   max_width++;

   fprintf(pfile, "# Uncrustify %s\n", UNCRUSTIFY_VERSION);

   /* Print the all out */
   for (group_map_it jt = group_map.begin(); jt != group_map.end(); jt++)
   {
      fprintf(pfile, "#\n# %s\n#\n\n", jt->second.short_desc);

      for (option_list_it it = jt->second.options.begin(); it != jt->second.options.end(); it++)
      {
         const option_map_value *option = get_option_name(*it);
         cur_width = strlen(option->name);
         fprintf(pfile, "%s%*c%s\n",
                 option->name,
                 max_width - cur_width, ' ',
                 names[option->type]);

         text = option->short_desc;

         if (text != NULL)
         {
            fputs("  ", pfile);
            while (*text != 0)
            {
               fputc(*text, pfile);
               if (*text == '\n')
               {
                  fputs("  ", pfile);
               }
               text++;
            }
         }
         fputs("\n\n", pfile);
      }
   }
}


/**
 * Sets non-zero settings defaults
 *
 * TODO: select from various sets? - i.e., K&R, GNU, Linux, Ben
 */
void set_option_defaults(void)
{
   cpd.settings[UO_newlines].le                = LE_AUTO;
   cpd.settings[UO_input_tab_size].n           = 8;
   cpd.settings[UO_output_tab_size].n          = 8;
   cpd.settings[UO_indent_ctor_init_leading].n = 2;
   cpd.settings[UO_indent_columns].n           = 8;
   cpd.settings[UO_indent_with_tabs].n         = 1;
   cpd.settings[UO_indent_label].n             = 1;
   cpd.settings[UO_indent_access_spec].n       = 1;
   cpd.settings[UO_sp_before_comma].a          = AV_REMOVE;
   cpd.settings[UO_sp_paren_comma].a           = AV_FORCE;
   cpd.settings[UO_string_escape_char].n       = '\\';
   cpd.settings[UO_sp_not].a               = AV_REMOVE;
   cpd.settings[UO_sp_inv].a               = AV_REMOVE;
   cpd.settings[UO_sp_addr].a              = AV_REMOVE;
   cpd.settings[UO_sp_deref].a             = AV_REMOVE;
   cpd.settings[UO_sp_member].a            = AV_REMOVE;
   cpd.settings[UO_sp_sign].a              = AV_REMOVE;
   cpd.settings[UO_sp_incdec].a            = AV_REMOVE;
   cpd.settings[UO_sp_after_type].a        = AV_FORCE;
   cpd.settings[UO_sp_before_nl_cont].a    = AV_ADD;
   cpd.settings[UO_sp_before_case_colon].a = AV_REMOVE;
   cpd.settings[UO_sp_before_semi].a       = AV_REMOVE;
   cpd.settings[UO_sp_after_semi].a        = AV_ADD;
   cpd.settings[UO_sp_after_semi_for].a    = AV_FORCE;
   cpd.settings[UO_cmt_indent_multi].b     = true;
   cpd.settings[UO_cmt_multi_check_last].b = true;
   cpd.settings[UO_pp_indent_count].n      = 1;
   cpd.settings[UO_align_left_shift].b     = true;
   cpd.settings[UO_indent_align_assign].b  = true;
   cpd.settings[UO_sp_pp_concat].a         = AV_ADD;
   cpd.settings[UO_sp_angle_shift].a       = AV_ADD;
   cpd.settings[UO_sp_word_brace].a        = AV_ADD;
   cpd.settings[UO_sp_word_brace_ns].a     = AV_ADD;
   cpd.settings[UO_indent_oc_msg_prioritize_first_colon].b = true;
}


string argtype_to_string(argtype_e argtype)
{
   switch (argtype)
   {
   case AT_BOOL:
      return("false/true");

   case AT_IARF:
      return("ignore/add/remove/force");

   case AT_NUM:
      return("number");

   case AT_LINE:
      return("auto/lf/crlf/cr");

   case AT_POS:
      return("ignore/join/lead/lead_break/lead_force/trail/trail_break/trail_force");

   case AT_STRING:
      return("string");

   default:
      LOG_FMT(LWARN, "Unknown argtype '%d'\n", argtype);
      return("");
   }
}


string bool_to_string(bool val)
{
   if (val)
   {
      return("true");
   }
   else
   {
      return("false");
   }
}


string argval_to_string(argval_t argval)
{
   switch (argval)
   {
   case AV_IGNORE:
      return("ignore");

   case AV_ADD:
      return("add");

   case AV_REMOVE:
      return("remove");

   case AV_FORCE:
      return("force");

   default:
      LOG_FMT(LWARN, "Unknown argval '%d'\n", argval);
      return("");
   }
}


string number_to_string(int number)
{
   char buffer[12]; // 11 + 1

   sprintf(buffer, "%d", number);

   /*NOTE: this creates a std:string class from the char array.
    *      It isn't returning a pointer to stack memory.
    */
   return(buffer);
}


string lineends_to_string(lineends_e linends)
{
   switch (linends)
   {
   case LE_LF:
      return("lf");

   case LE_CRLF:
      return("crlf");

   case LE_CR:
      return("cr");

   case LE_AUTO:
      return("auto");

   default:
      LOG_FMT(LWARN, "Unknown lineends '%d'\n", linends);
      return("");
   }
}


string tokenpos_to_string(tokenpos_e tokenpos)
{
   switch (tokenpos)
   {
   case TP_IGNORE:
      return("ignore");

   case TP_JOIN:
      return("join");

   case TP_LEAD:
      return("lead");

   case TP_LEAD_BREAK:
      return("lead_break");

   case TP_LEAD_FORCE:
      return("lead_force");

   case TP_TRAIL:
      return("trail");

   case TP_TRAIL_BREAK:
      return("trail_break");

   case TP_TRAIL_FORCE:
      return("trail_force");

   default:
      LOG_FMT(LWARN, "Unknown tokenpos '%d'\n", tokenpos);
      return("");
   }
}


string op_val_to_string(argtype_e argtype, op_val_t op_val)
{
   switch (argtype)
   {
   case AT_BOOL:
      return(bool_to_string(op_val.b));

   case AT_IARF:
      return(argval_to_string(op_val.a));

   case AT_NUM:
      return(number_to_string(op_val.n));

   case AT_LINE:
      return(lineends_to_string(op_val.le));

   case AT_POS:
      return(tokenpos_to_string(op_val.tp));

   case AT_STRING:
      return(op_val.str != NULL ? op_val.str : "");

   default:
      LOG_FMT(LWARN, "Unknown argtype '%d'\n", argtype);
      return("");
   }
}
