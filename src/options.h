/**
 * @file options.h
 * Declarations of all the options.
 *
 * September 2020
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @author  Matthew Woehlke
 * @license GPL v2+
 */

#ifndef OPTIONS_H_INCLUDED
#define OPTIONS_H_INCLUDED

/* NOTE:
 * This file is processed by make_options.py, and must conform to a particular
 * format. Option groups are marked by '//begin ' (in upper case; this example
 * is lower case to prevent being considered a region marker for code folding)
 * followed by the group description. Options consist of two lines of
 * declaration preceded by one or more lines of C++ comments. The comments form
 * the option description and are taken verbatim, aside from stripping the
 * leading '// '. Only comments immediately preceding an option declaration,
 * with no blank lines, are taken as part of the description, so a blank line
 * may be used to separate notations from a description.
 *
 * An option declaration is 'extern TYPE\nNAME;', optionally followed by
 * ' // = VALUE' if the option has a default value that is different from the
 * default-constructed value type of the option. The 'VALUE' must be valid C++
 * code, and is taken verbatim as an argument when creating the option's
 * instantiation. Note also that the line break, as shown, is required.
 */

#include "option_enum.h"

namespace uncrustify
{

namespace options
{

using std::string;

///////////////////////////////////////////////////////////////////////////////
//BEGIN General options

// The type of line endings.
extern Option<line_end_e>
newlines; // = LE_AUTO

// The original size of tabs in the input.
extern BoundedOption<unsigned, 1, 32>
input_tab_size; // = 8

// The size of tabs in the output (only used if align_with_tabs=true).
extern BoundedOption<unsigned, 1, 32>
output_tab_size; // = 8

// The ASCII value of the string escape char, usually 92 (\) or (Pawn) 94 (^).
extern BoundedOption<unsigned, 0, 255>
string_escape_char; // = '\\'

// Alternate string escape char (usually only used for Pawn).
// Only works right before the quote char.
extern BoundedOption<unsigned, 0, 255>
string_escape_char2;

// Replace tab characters found in string literals with the escape sequence \t
// instead.
extern Option<bool>
string_replace_tab_chars;

// Allow interpreting '>=' and '>>=' as part of a template in code like
// 'void f(list<list<B>>=val);'. If true, 'assert(x<0 && y>=3)' will be broken.
// Improvements to template detection may make this option obsolete.
extern Option<bool>
tok_split_gte;

// Disable formatting of NL_CONT ('\\n') ended lines (e.g. multi-line macros).
extern Option<bool>
disable_processing_nl_cont;

// Specify the marker used in comments to disable processing of part of the
// file.
extern Option<string>
disable_processing_cmt; // = UNCRUSTIFY_OFF_TEXT

// Specify the marker used in comments to (re)enable processing in a file.
extern Option<string>
enable_processing_cmt; // = UNCRUSTIFY_ON_TEXT

// Enable parsing of digraphs.
extern Option<bool>
enable_digraphs;

// Option to allow both disable_processing_cmt and enable_processing_cmt
// strings, if specified, to be interpreted as ECMAScript regular expressions.
// If true, a regex search will be performed within comments according to the
// specified patterns in order to disable/enable processing.
extern Option<bool>
processing_cmt_as_regex;

// Add or remove the UTF-8 BOM (recommend 'remove').
extern Option<iarf_e>
utf8_bom;

// If the file contains bytes with values between 128 and 255, but is not
// UTF-8, then output as UTF-8.
extern Option<bool>
utf8_byte;

// Force the output encoding to UTF-8.
extern Option<bool>
utf8_force;

//END

///////////////////////////////////////////////////////////////////////////////
//BEGIN Spacing options

// Add or remove space around non-assignment symbolic operators ('+', '/', '%',
// '<<', and so forth).
extern Option<iarf_e>
sp_arith;

// Add or remove space around arithmetic operators '+' and '-'.
//
// Overrides sp_arith.
extern Option<iarf_e>
sp_arith_additive;

// Add or remove space around assignment operator '=', '+=', etc.
extern Option<iarf_e>
sp_assign;

// Add or remove space around '=' in C++11 lambda capture specifications.
//
// Overrides sp_assign.
extern Option<iarf_e>
sp_cpp_lambda_assign;

// Add or remove space after the capture specification of a C++11 lambda when
// an argument list is present, as in '[] <here> (int x){ ... }'.
extern Option<iarf_e>
sp_cpp_lambda_square_paren;

// Add or remove space after the capture specification of a C++11 lambda with
// no argument list is present, as in '[] <here> { ... }'.
extern Option<iarf_e>
sp_cpp_lambda_square_brace;

// Add or remove space after the opening parenthesis and before the closing
// parenthesis of a argument list of a C++11 lambda, as in
// '[]( <here> ){ ... }'
// with an empty list.
extern Option<iarf_e>
sp_cpp_lambda_argument_list_empty;

// Add or remove space after the opening parenthesis and before the closing
// parenthesis of a argument list of a C++11 lambda, as in
// '[]( <here> int x <here> ){ ... }'.
extern Option<iarf_e>
sp_cpp_lambda_argument_list;

// Add or remove space after the argument list of a C++11 lambda, as in
// '[](int x) <here> { ... }'.
extern Option<iarf_e>
sp_cpp_lambda_paren_brace;

// Add or remove space between a lambda body and its call operator of an
// immediately invoked lambda, as in '[]( ... ){ ... } <here> ( ... )'.
extern Option<iarf_e>
sp_cpp_lambda_fparen;

// Add or remove space around assignment operator '=' in a prototype.
//
// If set to ignore, use sp_assign.
extern Option<iarf_e>
sp_assign_default;

// Add or remove space before assignment operator '=', '+=', etc.
//
// Overrides sp_assign.
extern Option<iarf_e>
sp_before_assign;

// Add or remove space after assignment operator '=', '+=', etc.
//
// Overrides sp_assign.
extern Option<iarf_e>
sp_after_assign;

// Add or remove space in 'enum {'.
extern Option<iarf_e>
sp_enum_brace; // = IARF_ADD

// Add or remove space in 'NS_ENUM ('.
extern Option<iarf_e>
sp_enum_paren;

// Add or remove space around assignment '=' in enum.
extern Option<iarf_e>
sp_enum_assign;

// Add or remove space before assignment '=' in enum.
//
// Overrides sp_enum_assign.
extern Option<iarf_e>
sp_enum_before_assign;

// Add or remove space after assignment '=' in enum.
//
// Overrides sp_enum_assign.
extern Option<iarf_e>
sp_enum_after_assign;

// Add or remove space around assignment ':' in enum.
extern Option<iarf_e>
sp_enum_colon;

// Add or remove space around preprocessor '##' concatenation operator.
extern Option<iarf_e>
sp_pp_concat; // = IARF_ADD

// Add or remove space after preprocessor '#' stringify operator.
// Also affects the '#@' charizing operator.
extern Option<iarf_e>
sp_pp_stringify;

// Add or remove space before preprocessor '#' stringify operator
// as in '#define x(y) L#y'.
extern Option<iarf_e>
sp_before_pp_stringify;

// Add or remove space around boolean operators '&&' and '||'.
extern Option<iarf_e>
sp_bool;

// Add or remove space around compare operator '<', '>', '==', etc.
extern Option<iarf_e>
sp_compare;

// Add or remove space inside '(' and ')'.
extern Option<iarf_e>
sp_inside_paren;

// Add or remove space between nested parentheses, i.e. '((' vs. ') )'.
extern Option<iarf_e>
sp_paren_paren;

// Add or remove space between back-to-back parentheses, i.e. ')(' vs. ') ('.
extern Option<iarf_e>
sp_cparen_oparen;

// Add or remove space between ')' and '{'.
extern Option<iarf_e>
sp_paren_brace;

// Add or remove space between nested braces, i.e. '{{' vs. '{ {'.
extern Option<iarf_e>
sp_brace_brace;

// Add or remove space before pointer star '*'.
extern Option<iarf_e>
sp_before_ptr_star;

// Add or remove space before pointer star '*' that isn't followed by a
// variable name. If set to ignore, sp_before_ptr_star is used instead.
extern Option<iarf_e>
sp_before_unnamed_ptr_star;

// Add or remove space before pointer star '*' that is followed by a qualifier.
// If set to ignore, sp_before_unnamed_ptr_star is used instead.
extern Option<iarf_e>
sp_before_qualifier_ptr_star;

// Add or remove space before pointer star '*' that is followed by 'operator' keyword.
// If set to ignore, sp_before_unnamed_ptr_star is used instead.
extern Option<iarf_e>
sp_before_operator_ptr_star;

// Add or remove space before pointer star '*' that is followed by
// a class scope (as in 'int *MyClass::method()') or namespace scope
// (as in 'int *my_ns::func()').
// If set to ignore, sp_before_unnamed_ptr_star is used instead.
extern Option<iarf_e>
sp_before_scope_ptr_star;

// Add or remove space before pointer star '*' that is followed by '::',
// as in 'int *::func()'.
// If set to ignore, sp_before_unnamed_ptr_star is used instead.
extern Option<iarf_e>
sp_before_global_scope_ptr_star;

// Add or remove space between a qualifier and a pointer star '*' that isn't
// followed by a variable name, as in '(char const *)'. If set to ignore,
// sp_before_ptr_star is used instead.
extern Option<iarf_e>
sp_qualifier_unnamed_ptr_star;

// Add or remove space between pointer stars '*', as in 'int ***a;'.
extern Option<iarf_e>
sp_between_ptr_star;

// Add or remove space between pointer star '*' and reference '&', as in 'int *& a;'.
extern Option<iarf_e>
sp_between_ptr_ref;

// Add or remove space after pointer star '*', if followed by a word.
//
// Overrides sp_type_func.
extern Option<iarf_e>
sp_after_ptr_star;

// Add or remove space after pointer caret '^', if followed by a word.
extern Option<iarf_e>
sp_after_ptr_block_caret;

// Add or remove space after pointer star '*', if followed by a qualifier.
extern Option<iarf_e>
sp_after_ptr_star_qualifier;

// Add or remove space after a pointer star '*', if followed by a function
// prototype or function definition.
//
// Overrides sp_after_ptr_star and sp_type_func.
extern Option<iarf_e>
sp_after_ptr_star_func;

// Add or remove space after a pointer star '*' in the trailing return of a
// function prototype or function definition.
extern Option<iarf_e>
sp_after_ptr_star_trailing;

// Add or remove space between the pointer star '*' and the name of the variable
// in a function pointer definition.
extern Option<iarf_e>
sp_ptr_star_func_var;

// Add or remove space between the pointer star '*' and the name of the type
// in a function pointer type definition.
extern Option<iarf_e>
sp_ptr_star_func_type;

// Add or remove space after a pointer star '*', if followed by an open
// parenthesis, as in 'void* (*)()'.
extern Option<iarf_e>
sp_ptr_star_paren;

// Add or remove space before a pointer star '*', if followed by a function
// prototype or function definition. If set to ignore, sp_before_ptr_star is
// used instead.
extern Option<iarf_e>
sp_before_ptr_star_func;

// Add or remove space between a qualifier and a pointer star '*' followed by
// the name of the function in a function prototype or definition, as in
// 'char const *foo()`. If set to ignore, sp_before_ptr_star is used instead.
extern Option<iarf_e>
sp_qualifier_ptr_star_func;

// Add or remove space before a pointer star '*' in the trailing return of a
// function prototype or function definition.
extern Option<iarf_e>
sp_before_ptr_star_trailing;

// Add or remove space between a qualifier and a pointer star '*' in the
// trailing return of a function prototype or function definition, as in
// 'auto foo() -> char const *'.
extern Option<iarf_e>
sp_qualifier_ptr_star_trailing;

// Add or remove space before a reference sign '&'.
extern Option<iarf_e>
sp_before_byref;

// Add or remove space before a reference sign '&' that isn't followed by a
// variable name. If set to ignore, sp_before_byref is used instead.
extern Option<iarf_e>
sp_before_unnamed_byref;

// Add or remove space after reference sign '&', if followed by a word.
//
// Overrides sp_type_func.
extern Option<iarf_e>
sp_after_byref;

// Add or remove space after a reference sign '&', if followed by a function
// prototype or function definition.
//
// Overrides sp_after_byref and sp_type_func.
extern Option<iarf_e>
sp_after_byref_func;

// Add or remove space before a reference sign '&', if followed by a function
// prototype or function definition.
extern Option<iarf_e>
sp_before_byref_func;

// Add or remove space after a reference sign '&', if followed by an open
// parenthesis, as in 'char& (*)()'.
extern Option<iarf_e>
sp_byref_paren;

// Add or remove space between type and word. In cases where total removal of
// whitespace would be a syntax error, a value of 'remove' is treated the same
// as 'force'.
//
// This also affects some other instances of space following a type that are
// not covered by other options; for example, between the return type and
// parenthesis of a function type template argument, between the type and
// parenthesis of an array parameter, or between 'decltype(...)' and the
// following word.
extern Option<iarf_e>
sp_after_type; // = IARF_FORCE

// Add or remove space between 'decltype(...)' and word,
// brace or function call.
extern Option<iarf_e>
sp_after_decltype;

// (D) Add or remove space before the parenthesis in the D constructs
// 'template Foo(' and 'class Foo('.
extern Option<iarf_e>
sp_before_template_paren;

// Add or remove space between 'template' and '<'.
// If set to ignore, sp_before_angle is used.
extern Option<iarf_e>
sp_template_angle;

// Add or remove space before '<'.
extern Option<iarf_e>
sp_before_angle;

// Add or remove space inside '<' and '>'.
extern Option<iarf_e>
sp_inside_angle;

// Add or remove space inside '<>'.
// if empty.
extern Option<iarf_e>
sp_inside_angle_empty;

// Add or remove space between '>' and ':'.
extern Option<iarf_e>
sp_angle_colon;

// Add or remove space after '>'.
extern Option<iarf_e>
sp_after_angle;

// Add or remove space between '>' and '(' as found in 'new List<byte>(foo);'.
extern Option<iarf_e>
sp_angle_paren;

// Add or remove space between '>' and '()' as found in 'new List<byte>();'.
extern Option<iarf_e>
sp_angle_paren_empty;

// Add or remove space between '>' and a word as in 'List<byte> m;' or
// 'template <typename T> static ...'.
extern Option<iarf_e>
sp_angle_word;

// Add or remove space between '>' and '>' in '>>' (template stuff).
extern Option<iarf_e>
sp_angle_shift; // = IARF_ADD

// (C++11) Permit removal of the space between '>>' in 'foo<bar<int> >'. Note
// that sp_angle_shift cannot remove the space without this option.
extern Option<bool>
sp_permit_cpp11_shift;

// Add or remove space before '(' of control statements ('if', 'for', 'switch',
// 'while', etc.).
extern Option<iarf_e>
sp_before_sparen;

// Add or remove space inside '(' and ')' of control statements other than
// 'for'.
extern Option<iarf_e>
sp_inside_sparen;

// Add or remove space after '(' of control statements other than 'for'.
//
// Overrides sp_inside_sparen.
extern Option<iarf_e>
sp_inside_sparen_open;

// Add or remove space before ')' of control statements other than 'for'.
//
// Overrides sp_inside_sparen.
extern Option<iarf_e>
sp_inside_sparen_close;

// Add or remove space inside '(' and ')' of 'for' statements.
extern Option<iarf_e>
sp_inside_for;

// Add or remove space after '(' of 'for' statements.
//
// Overrides sp_inside_for.
extern Option<iarf_e>
sp_inside_for_open;

// Add or remove space before ')' of 'for' statements.
//
// Overrides sp_inside_for.
extern Option<iarf_e>
sp_inside_for_close;

// Add or remove space between '((' or '))' of control statements.
extern Option<iarf_e>
sp_sparen_paren;

// Add or remove space after ')' of control statements.
extern Option<iarf_e>
sp_after_sparen;

// Add or remove space between ')' and '{' of control statements.
extern Option<iarf_e>
sp_sparen_brace;

// Add or remove space between 'do' and '{'.
extern Option<iarf_e>
sp_do_brace_open;

// Add or remove space between '}' and 'while'.
extern Option<iarf_e>
sp_brace_close_while;

// Add or remove space between 'while' and '('. Overrides sp_before_sparen.
extern Option<iarf_e>
sp_while_paren_open;

// (D) Add or remove space between 'invariant' and '('.
extern Option<iarf_e>
sp_invariant_paren;

// (D) Add or remove space after the ')' in 'invariant (C) c'.
extern Option<iarf_e>
sp_after_invariant_paren;

// Add or remove space before empty statement ';' on 'if', 'for' and 'while'.
// examples:
//   if (b) <here> ;
//   for (a=1; a<10; a++) <here> ;
//   while (*p++ = ' ') <here> ;
extern Option<iarf_e>
sp_special_semi;

// Add or remove space before ';'.
extern Option<iarf_e>
sp_before_semi; // = IARF_REMOVE

// Add or remove space before ';' in non-empty 'for' statements.
extern Option<iarf_e>
sp_before_semi_for;

// Add or remove space before a semicolon of an empty left part of a for
// statement, as in 'for ( <here> ; ; )'.
extern Option<iarf_e>
sp_before_semi_for_empty;

// Add or remove space between the semicolons of an empty middle part of a for
// statement, as in 'for ( ; <here> ; )'.
extern Option<iarf_e>
sp_between_semi_for_empty;

// Add or remove space after ';', except when followed by a comment.
extern Option<iarf_e>
sp_after_semi; // = IARF_ADD

// Add or remove space after ';' in non-empty 'for' statements.
extern Option<iarf_e>
sp_after_semi_for; // = IARF_FORCE

// Add or remove space after the final semicolon of an empty part of a for
// statement, as in 'for ( ; ; <here> )'.
extern Option<iarf_e>
sp_after_semi_for_empty;

// Add or remove space before '[' (except '[]').
extern Option<iarf_e>
sp_before_square;

// Add or remove space before '[' for a variable definition.
extern Option<iarf_e>
sp_before_vardef_square; // = IARF_REMOVE

// Add or remove space before '[' for asm block.
extern Option<iarf_e>
sp_before_square_asm_block;

// Add or remove space before '[]'.
extern Option<iarf_e>
sp_before_squares;

// Add or remove space before C++17 structured bindings
// after byref.
extern Option<iarf_e>
sp_cpp_before_struct_binding_after_byref;

// Add or remove space before C++17 structured bindings.
extern Option<iarf_e>
sp_cpp_before_struct_binding;

// Add or remove space inside a non-empty '[' and ']'.
extern Option<iarf_e>
sp_inside_square;

// Add or remove space inside '[]'.
// if empty.
extern Option<iarf_e>
sp_inside_square_empty;

// (OC) Add or remove space inside a non-empty Objective-C boxed array '@[' and
// ']'. If set to ignore, sp_inside_square is used.
extern Option<iarf_e>
sp_inside_square_oc_array;

// Add or remove space after ',', i.e. 'a,b' vs. 'a, b'.
extern Option<iarf_e>
sp_after_comma;

// Add or remove space before ',', i.e. 'a,b' vs. 'a ,b'.
extern Option<iarf_e>
sp_before_comma; // = IARF_REMOVE

// (C#, Vala) Add or remove space between ',' and ']' in multidimensional array type
// like 'int[,,]'.
extern Option<iarf_e>
sp_after_mdatype_commas;

// (C#, Vala) Add or remove space between '[' and ',' in multidimensional array type
// like 'int[,,]'.
extern Option<iarf_e>
sp_before_mdatype_commas;

// (C#, Vala) Add or remove space between ',' in multidimensional array type
// like 'int[,,]'.
extern Option<iarf_e>
sp_between_mdatype_commas;

// Add or remove space between an open parenthesis and comma,
// i.e. '(,' vs. '( ,'.
extern Option<iarf_e>
sp_paren_comma; // = IARF_FORCE

// Add or remove space between a type and ':'.
extern Option<iarf_e>
sp_type_colon;

// Add or remove space after the variadic '...' when preceded by a
// non-punctuator.
// The value REMOVE will be overridden with FORCE
extern Option<iarf_e>
sp_after_ellipsis;

// Add or remove space before the variadic '...' when preceded by a
// non-punctuator.
// The value REMOVE will be overridden with FORCE
extern Option<iarf_e>
sp_before_ellipsis;

// Add or remove space between a type and '...'.
extern Option<iarf_e>
sp_type_ellipsis;

// Add or remove space between a '*' and '...'.
extern Option<iarf_e>
sp_ptr_type_ellipsis;

// Add or remove space between ')' and '...'.
extern Option<iarf_e>
sp_paren_ellipsis;

// Add or remove space between '&&' and '...'.
extern Option<iarf_e>
sp_byref_ellipsis;

// Add or remove space between ')' and a qualifier such as 'const'.
extern Option<iarf_e>
sp_paren_qualifier;

// Add or remove space between ')' and 'noexcept'.
extern Option<iarf_e>
sp_paren_noexcept;

// Add or remove space after class ':'.
extern Option<iarf_e>
sp_after_class_colon;

// Add or remove space before class ':'.
extern Option<iarf_e>
sp_before_class_colon;

// Add or remove space after class constructor ':'.
extern Option<iarf_e>
sp_after_constr_colon; // = IARF_ADD

// Add or remove space before class constructor ':'.
extern Option<iarf_e>
sp_before_constr_colon; // = IARF_ADD

// Add or remove space before case ':'.
extern Option<iarf_e>
sp_before_case_colon; // = IARF_REMOVE

// Add or remove space between 'operator' and operator sign.
extern Option<iarf_e>
sp_after_operator;

// Add or remove space between the operator symbol and the open parenthesis, as
// in 'operator ++('.
extern Option<iarf_e>
sp_after_operator_sym;

// Overrides sp_after_operator_sym when the operator has no arguments, as in
// 'operator *()'.
extern Option<iarf_e>
sp_after_operator_sym_empty;

// Add or remove space after C/D cast, i.e. 'cast(int)a' vs. 'cast(int) a' or
// '(int)a' vs. '(int) a'.
extern Option<iarf_e>
sp_after_cast;

// Add or remove spaces inside cast parentheses.
extern Option<iarf_e>
sp_inside_paren_cast;

// Add or remove space between the type and open parenthesis in a C++ cast,
// i.e. 'int(exp)' vs. 'int (exp)'.
extern Option<iarf_e>
sp_cpp_cast_paren;

// Add or remove space between 'sizeof' and '('.
extern Option<iarf_e>
sp_sizeof_paren;

// Add or remove space between 'sizeof' and '...'.
extern Option<iarf_e>
sp_sizeof_ellipsis;

// Add or remove space between 'sizeof...' and '('.
extern Option<iarf_e>
sp_sizeof_ellipsis_paren;

// Add or remove space between '...' and a parameter pack.
extern Option<iarf_e>
sp_ellipsis_parameter_pack;

// Add or remove space between a parameter pack and '...'.
extern Option<iarf_e>
sp_parameter_pack_ellipsis;

// Add or remove space between 'decltype' and '('.
extern Option<iarf_e>
sp_decltype_paren;

// (Pawn) Add or remove space after the tag keyword.
extern Option<iarf_e>
sp_after_tag;

// Add or remove space inside enum '{' and '}'.
extern Option<iarf_e>
sp_inside_braces_enum;

// Add or remove space inside struct/union '{' and '}'.
extern Option<iarf_e>
sp_inside_braces_struct;

// (OC) Add or remove space inside Objective-C boxed dictionary '{' and '}'
extern Option<iarf_e>
sp_inside_braces_oc_dict;

// Add or remove space after open brace in an unnamed temporary
// direct-list-initialization
// if statement is a brace_init_lst
// works only if sp_brace_brace is set to ignore.
extern Option<iarf_e>
sp_after_type_brace_init_lst_open;

// Add or remove space before close brace in an unnamed temporary
// direct-list-initialization
// if statement is a brace_init_lst
// works only if sp_brace_brace is set to ignore.
extern Option<iarf_e>
sp_before_type_brace_init_lst_close;

// Add or remove space inside an unnamed temporary direct-list-initialization
// if statement is a brace_init_lst
// works only if sp_brace_brace is set to ignore
// works only if sp_before_type_brace_init_lst_close is set to ignore.
extern Option<iarf_e>
sp_inside_type_brace_init_lst;

// Add or remove space inside '{' and '}'.
extern Option<iarf_e>
sp_inside_braces;

// Add or remove space inside '{}'.
// if empty.
extern Option<iarf_e>
sp_inside_braces_empty;

// Add or remove space around trailing return operator '->'.
extern Option<iarf_e>
sp_trailing_return;

// Add or remove space between return type and function name. A minimum of 1
// is forced except for pointer return types.
extern Option<iarf_e>
sp_type_func;

// Add or remove space between type and open brace of an unnamed temporary
// direct-list-initialization.
extern Option<iarf_e>
sp_type_brace_init_lst;

// Add or remove space between function name and '(' on function declaration.
extern Option<iarf_e>
sp_func_proto_paren;

// Add or remove space between function name and '()' on function declaration
// if empty.
extern Option<iarf_e>
sp_func_proto_paren_empty;

// Add or remove space between function name and '(' with a typedef specifier.
extern Option<iarf_e>
sp_func_type_paren;

// Add or remove space between alias name and '(' of a non-pointer function type typedef.
extern Option<iarf_e>
sp_func_def_paren;

// Add or remove space between function name and '()' on function definition
// if empty.
extern Option<iarf_e>
sp_func_def_paren_empty;

// Add or remove space inside empty function '()'.
// Overrides sp_after_angle unless use_sp_after_angle_always is set to true.
extern Option<iarf_e>
sp_inside_fparens;

// Add or remove space inside function '(' and ')'.
extern Option<iarf_e>
sp_inside_fparen;

// Add or remove space inside user functor '(' and ')'.
extern Option<iarf_e>
sp_func_call_user_inside_rparen;

// Add or remove space inside empty functor '()'.
// Overrides sp_after_angle unless use_sp_after_angle_always is set to true.
extern Option<iarf_e>
sp_inside_rparens;

// Add or remove space inside functor '(' and ')'.
extern Option<iarf_e>
sp_inside_rparen;

// Add or remove space inside the first parentheses in a function type, as in
// 'void (*x)(...)'.
extern Option<iarf_e>
sp_inside_tparen;

// Add or remove space between the ')' and '(' in a function type, as in
// 'void (*x)(...)'.
extern Option<iarf_e>
sp_after_tparen_close;

// Add or remove space between ']' and '(' when part of a function call.
extern Option<iarf_e>
sp_square_fparen;

// Add or remove space between ')' and '{' of function.
extern Option<iarf_e>
sp_fparen_brace;

// Add or remove space between ')' and '{' of a function call in object
// initialization.
//
// Overrides sp_fparen_brace.
extern Option<iarf_e>
sp_fparen_brace_initializer;

// (Java) Add or remove space between ')' and '{{' of double brace initializer.
extern Option<iarf_e>
sp_fparen_dbrace;

// Add or remove space between function name and '(' on function calls.
extern Option<iarf_e>
sp_func_call_paren;

// Add or remove space between function name and '()' on function calls without
// parameters. If set to ignore (the default), sp_func_call_paren is used.
extern Option<iarf_e>
sp_func_call_paren_empty;

// Add or remove space between the user function name and '(' on function
// calls. You need to set a keyword to be a user function in the config file,
// like:
//   set func_call_user tr _ i18n
extern Option<iarf_e>
sp_func_call_user_paren;

// Add or remove space inside user function '(' and ')'.
extern Option<iarf_e>
sp_func_call_user_inside_fparen;

// Add or remove space between nested parentheses with user functions,
// i.e. '((' vs. '( ('.
extern Option<iarf_e>
sp_func_call_user_paren_paren;

// Add or remove space between a constructor/destructor and the open
// parenthesis.
extern Option<iarf_e>
sp_func_class_paren;

// Add or remove space between a constructor without parameters or destructor
// and '()'.
extern Option<iarf_e>
sp_func_class_paren_empty;

// Add or remove space after 'return'.
extern Option<iarf_e>
sp_return; // = IARF_FORCE

// Add or remove space between 'return' and '('.
extern Option<iarf_e>
sp_return_paren;

// Add or remove space between 'return' and '{'.
extern Option<iarf_e>
sp_return_brace;

// Add or remove space between '__attribute__' and '('.
extern Option<iarf_e>
sp_attribute_paren;

// Add or remove space between 'defined' and '(' in '#if defined (FOO)'.
extern Option<iarf_e>
sp_defined_paren;

// Add or remove space between 'throw' and '(' in 'throw (something)'.
extern Option<iarf_e>
sp_throw_paren;

// Add or remove space between 'throw' and anything other than '(' as in
// '@throw [...];'.
extern Option<iarf_e>
sp_after_throw;

// Add or remove space between 'catch' and '(' in 'catch (something) { }'.
// If set to ignore, sp_before_sparen is used.
extern Option<iarf_e>
sp_catch_paren;

// (OC) Add or remove space between '@catch' and '('
// in '@catch (something) { }'. If set to ignore, sp_catch_paren is used.
extern Option<iarf_e>
sp_oc_catch_paren;

// (OC) Add or remove space before Objective-C protocol list
// as in '@protocol Protocol<here><Protocol_A>' or '@interface MyClass : NSObject<here><MyProtocol>'.
extern Option<iarf_e>
sp_before_oc_proto_list;

// (OC) Add or remove space between class name and '('
// in '@interface className(categoryName)<ProtocolName>:BaseClass'
extern Option<iarf_e>
sp_oc_classname_paren;

// (D) Add or remove space between 'version' and '('
// in 'version (something) { }'. If set to ignore, sp_before_sparen is used.
extern Option<iarf_e>
sp_version_paren;

// (D) Add or remove space between 'scope' and '('
// in 'scope (something) { }'. If set to ignore, sp_before_sparen is used.
extern Option<iarf_e>
sp_scope_paren;

// Add or remove space between 'super' and '(' in 'super (something)'.
extern Option<iarf_e>
sp_super_paren; // = IARF_REMOVE

// Add or remove space between 'this' and '(' in 'this (something)'.
extern Option<iarf_e>
sp_this_paren; // = IARF_REMOVE

// Add or remove space between a macro name and its definition.
extern Option<iarf_e>
sp_macro;

// Add or remove space between a macro function ')' and its definition.
extern Option<iarf_e>
sp_macro_func;

// Add or remove space between 'else' and '{' if on the same line.
extern Option<iarf_e>
sp_else_brace;

// Add or remove space between '}' and 'else' if on the same line.
extern Option<iarf_e>
sp_brace_else;

// Add or remove space between '}' and the name of a typedef on the same line.
extern Option<iarf_e>
sp_brace_typedef;

// Add or remove space before the '{' of a 'catch' statement, if the '{' and
// 'catch' are on the same line, as in 'catch (decl) <here> {'.
extern Option<iarf_e>
sp_catch_brace;

// (OC) Add or remove space before the '{' of a '@catch' statement, if the '{'
// and '@catch' are on the same line, as in '@catch (decl) <here> {'.
// If set to ignore, sp_catch_brace is used.
extern Option<iarf_e>
sp_oc_catch_brace;

// Add or remove space between '}' and 'catch' if on the same line.
extern Option<iarf_e>
sp_brace_catch;

// (OC) Add or remove space between '}' and '@catch' if on the same line.
// If set to ignore, sp_brace_catch is used.
extern Option<iarf_e>
sp_oc_brace_catch;

// Add or remove space between 'finally' and '{' if on the same line.
extern Option<iarf_e>
sp_finally_brace;

// Add or remove space between '}' and 'finally' if on the same line.
extern Option<iarf_e>
sp_brace_finally;

// Add or remove space between 'try' and '{' if on the same line.
extern Option<iarf_e>
sp_try_brace;

// Add or remove space between get/set and '{' if on the same line.
extern Option<iarf_e>
sp_getset_brace;

// Add or remove space between a variable and '{' for C++ uniform
// initialization.
extern Option<iarf_e>
sp_word_brace_init_lst;

// Add or remove space between a variable and '{' for a namespace.
extern Option<iarf_e>
sp_word_brace_ns; // = IARF_ADD

// Add or remove space before the '::' operator.
extern Option<iarf_e>
sp_before_dc;

// Add or remove space after the '::' operator.
extern Option<iarf_e>
sp_after_dc;

// (D) Add or remove around the D named array initializer ':' operator.
extern Option<iarf_e>
sp_d_array_colon;

// Add or remove space after the '!' (not) unary operator.
extern Option<iarf_e>
sp_not; // = IARF_REMOVE

// Add or remove space between two '!' (not) unary operators.
// If set to ignore, sp_not will be used.
extern Option<iarf_e>
sp_not_not; // = IARF_IGNORE

// Add or remove space after the '~' (invert) unary operator.
extern Option<iarf_e>
sp_inv; // = IARF_REMOVE

// Add or remove space after the '&' (address-of) unary operator. This does not
// affect the spacing after a '&' that is part of a type.
extern Option<iarf_e>
sp_addr; // = IARF_REMOVE

// Add or remove space around the '.' or '->' operators.
extern Option<iarf_e>
sp_member; // = IARF_REMOVE

// Add or remove space after the '*' (dereference) unary operator. This does
// not affect the spacing after a '*' that is part of a type.
extern Option<iarf_e>
sp_deref; // = IARF_REMOVE

// Add or remove space after '+' or '-', as in 'x = -5' or 'y = +7'.
extern Option<iarf_e>
sp_sign; // = IARF_REMOVE

// Add or remove space between '++' and '--' the word to which it is being
// applied, as in '(--x)' or 'y++;'.
extern Option<iarf_e>
sp_incdec; // = IARF_REMOVE

// Add or remove space before a backslash-newline at the end of a line.
extern Option<iarf_e>
sp_before_nl_cont; // = IARF_ADD

// (OC) Add or remove space after the scope '+' or '-', as in '-(void) foo;'
// or '+(int) bar;'.
extern Option<iarf_e>
sp_after_oc_scope;

// (OC) Add or remove space after the colon in message specs,
// i.e. '-(int) f:(int) x;' vs. '-(int) f: (int) x;'.
extern Option<iarf_e>
sp_after_oc_colon;

// (OC) Add or remove space before the colon in message specs,
// i.e. '-(int) f: (int) x;' vs. '-(int) f : (int) x;'.
extern Option<iarf_e>
sp_before_oc_colon;

// (OC) Add or remove space after the colon in immutable dictionary expression
// 'NSDictionary *test = @{@"foo" :@"bar"};'.
extern Option<iarf_e>
sp_after_oc_dict_colon;

// (OC) Add or remove space before the colon in immutable dictionary expression
// 'NSDictionary *test = @{@"foo" :@"bar"};'.
extern Option<iarf_e>
sp_before_oc_dict_colon;

// (OC) Add or remove space after the colon in message specs,
// i.e. '[object setValue:1];' vs. '[object setValue: 1];'.
extern Option<iarf_e>
sp_after_send_oc_colon;

// (OC) Add or remove space before the colon in message specs,
// i.e. '[object setValue:1];' vs. '[object setValue :1];'.
extern Option<iarf_e>
sp_before_send_oc_colon;

// (OC) Add or remove space after the (type) in message specs,
// i.e. '-(int)f: (int) x;' vs. '-(int)f: (int)x;'.
extern Option<iarf_e>
sp_after_oc_type;

// (OC) Add or remove space after the first (type) in message specs,
// i.e. '-(int) f:(int)x;' vs. '-(int)f:(int)x;'.
extern Option<iarf_e>
sp_after_oc_return_type;

// (OC) Add or remove space between '@selector' and '(',
// i.e. '@selector(msgName)' vs. '@selector (msgName)'.
// Also applies to '@protocol()' constructs.
extern Option<iarf_e>
sp_after_oc_at_sel;

// (OC) Add or remove space between '@selector(x)' and the following word,
// i.e. '@selector(foo) a:' vs. '@selector(foo)a:'.
extern Option<iarf_e>
sp_after_oc_at_sel_parens;

// (OC) Add or remove space inside '@selector' parentheses,
// i.e. '@selector(foo)' vs. '@selector( foo )'.
// Also applies to '@protocol()' constructs.
extern Option<iarf_e>
sp_inside_oc_at_sel_parens;

// (OC) Add or remove space before a block pointer caret,
// i.e. '^int (int arg){...}' vs. ' ^int (int arg){...}'.
extern Option<iarf_e>
sp_before_oc_block_caret;

// (OC) Add or remove space after a block pointer caret,
// i.e. '^int (int arg){...}' vs. '^ int (int arg){...}'.
extern Option<iarf_e>
sp_after_oc_block_caret;

// (OC) Add or remove space between the receiver and selector in a message,
// as in '[receiver selector ...]'.
extern Option<iarf_e>
sp_after_oc_msg_receiver;

// (OC) Add or remove space after '@property'.
extern Option<iarf_e>
sp_after_oc_property;

// (OC) Add or remove space between '@synchronized' and the open parenthesis,
// i.e. '@synchronized(foo)' vs. '@synchronized (foo)'.
extern Option<iarf_e>
sp_after_oc_synchronized;

// Add or remove space around the ':' in 'b ? t : f'.
extern Option<iarf_e>
sp_cond_colon;

// Add or remove space before the ':' in 'b ? t : f'.
//
// Overrides sp_cond_colon.
extern Option<iarf_e>
sp_cond_colon_before;

// Add or remove space after the ':' in 'b ? t : f'.
//
// Overrides sp_cond_colon.
extern Option<iarf_e>
sp_cond_colon_after;

// Add or remove space around the '?' in 'b ? t : f'.
extern Option<iarf_e>
sp_cond_question;

// Add or remove space before the '?' in 'b ? t : f'.
//
// Overrides sp_cond_question.
extern Option<iarf_e>
sp_cond_question_before;

// Add or remove space after the '?' in 'b ? t : f'.
//
// Overrides sp_cond_question.
extern Option<iarf_e>
sp_cond_question_after;

// In the abbreviated ternary form '(a ?: b)', add or remove space between '?'
// and ':'.
//
// Overrides all other sp_cond_* options.
extern Option<iarf_e>
sp_cond_ternary_short;

// Fix the spacing between 'case' and the label. Only 'ignore' and 'force' make
// sense here.
extern Option<iarf_e>
sp_case_label;

// (D) Add or remove space around the D '..' operator.
extern Option<iarf_e>
sp_range;

// Add or remove space after ':' in a Java/C++11 range-based 'for',
// as in 'for (Type var : <here> expr)'.
extern Option<iarf_e>
sp_after_for_colon;

// Add or remove space before ':' in a Java/C++11 range-based 'for',
// as in 'for (Type var <here> : expr)'.
extern Option<iarf_e>
sp_before_for_colon;

// (D) Add or remove space between 'extern' and '(' as in 'extern <here> (C)'.
extern Option<iarf_e>
sp_extern_paren;

// Add or remove space after the opening of a C++ comment, as in '// <here> A'.
extern Option<iarf_e>
sp_cmt_cpp_start;

// remove space after the '//' and the pvs command '-V1234',
// only works with sp_cmt_cpp_start set to add or force.
extern Option<bool>
sp_cmt_cpp_pvs;

// remove space after the '//' and the command 'lint',
// only works with sp_cmt_cpp_start set to add or force.
extern Option<bool>
sp_cmt_cpp_lint;

// Add or remove space in a C++ region marker comment, as in '// <here> BEGIN'.
// A region marker is defined as a comment which is not preceded by other text
// (i.e. the comment is the first non-whitespace on the line), and which starts
// with either 'BEGIN' or 'END'.
//
// Overrides sp_cmt_cpp_start.
extern Option<iarf_e>
sp_cmt_cpp_region;

// If true, space added with sp_cmt_cpp_start will be added after Doxygen
// sequences like '///', '///<', '//!' and '//!<'.
extern Option<bool>
sp_cmt_cpp_doxygen;

// If true, space added with sp_cmt_cpp_start will be added after Qt translator
// or meta-data comments like '//:', '//=', and '//~'.
extern Option<bool>
sp_cmt_cpp_qttr;

// Add or remove space between #else or #endif and a trailing comment.
extern Option<iarf_e>
sp_endif_cmt;

// Add or remove space after 'new', 'delete' and 'delete[]'.
extern Option<iarf_e>
sp_after_new;

// Add or remove space between 'new' and '(' in 'new()'.
extern Option<iarf_e>
sp_between_new_paren;

// Add or remove space between ')' and type in 'new(foo) BAR'.
extern Option<iarf_e>
sp_after_newop_paren;

// Add or remove space inside parentheses of the new operator
// as in 'new(foo) BAR'.
extern Option<iarf_e>
sp_inside_newop_paren;

// Add or remove space after the open parenthesis of the new operator,
// as in 'new(foo) BAR'.
//
// Overrides sp_inside_newop_paren.
extern Option<iarf_e>
sp_inside_newop_paren_open;

// Add or remove space before the close parenthesis of the new operator,
// as in 'new(foo) BAR'.
//
// Overrides sp_inside_newop_paren.
extern Option<iarf_e>
sp_inside_newop_paren_close;

// Add or remove space before a trailing comment.
extern Option<iarf_e>
sp_before_tr_cmt;

// Number of spaces before a trailing comment.
extern BoundedOption<unsigned, 0, 16>
sp_num_before_tr_cmt;

// Add or remove space before an embedded comment.
extern Option<iarf_e>
sp_before_emb_cmt; // = IARF_FORCE

// Number of spaces before an embedded comment.
extern BoundedOption<unsigned, 0, 16>
sp_num_before_emb_cmt; // = 1

// Add or remove space after an embedded comment.
extern Option<iarf_e>
sp_after_emb_cmt; // = IARF_FORCE

// Number of spaces after an embedded comment.
extern BoundedOption<unsigned, 0, 16>
sp_num_after_emb_cmt; // = 1

// Embedded comment spacing options have higher priority (== override)
// than other spacing options (comma, parenthesis, braces, ...)
extern Option<bool>
sp_emb_cmt_priority; // = false

// (Java) Add or remove space between an annotation and the open parenthesis.
extern Option<iarf_e>
sp_annotation_paren;

// If true, vbrace tokens are dropped to the previous token and skipped.
extern Option<bool>
sp_skip_vbrace_tokens;

// Add or remove space after 'noexcept'.
extern Option<iarf_e>
sp_after_noexcept;

// Add or remove space after '_'.
extern Option<iarf_e>
sp_vala_after_translation;

// Add or remove space before a bit colon ':'.
extern Option<iarf_e>
sp_before_bit_colon;

// Add or remove space after a bit colon ':'.
extern Option<iarf_e>
sp_after_bit_colon;

// If true, a <TAB> is inserted after #define.
extern Option<bool>
force_tab_after_define;

// Add or remove space between two strings.
extern Option<iarf_e>
sp_string_string;

// Add or remove space 'struct' and a type.
extern Option<iarf_e>
sp_struct_type;

//END

///////////////////////////////////////////////////////////////////////////////
//BEGIN Indenting options

// The number of columns to indent per level. Usually 2, 3, 4, or 8.
extern BoundedOption<unsigned, 0, 16>
indent_columns; // = 8

// Whether to ignore indent for the first continuation line. Subsequent
// continuation lines will still be indented to match the first.
extern Option<bool>
indent_ignore_first_continue;

// The continuation indent. If non-zero, this overrides the indent of '(', '['
// and '=' continuation indents. Negative values are OK; negative value is
// absolute and not increased for each '(' or '[' level.
//
// For FreeBSD, this is set to 4.
// Requires indent_ignore_first_continue=false.
extern BoundedOption<signed, -16, 16>
indent_continue;

// The continuation indent, only for class header line(s). If non-zero, this
// overrides the indent of 'class' continuation indents.
// Requires indent_ignore_first_continue=false.
extern BoundedOption<unsigned, 0, 16>
indent_continue_class_head;

// Whether to indent empty lines (i.e. lines which contain only spaces before
// the newline character).
extern Option<bool>
indent_single_newlines;

// The continuation indent for func_*_param if they are true. If non-zero, this
// overrides the indent.
extern BoundedOption<unsigned, 0, 16>
indent_param;

// How to use tabs when indenting code.
//
// 0: Spaces only
// 1: Indent with tabs to brace level, align with spaces (default)
// 2: Indent and align with tabs, using spaces when not on a tabstop
extern BoundedOption<unsigned, 0, 2>
indent_with_tabs; // = 1

// Whether to indent comments that are not at a brace level with tabs on a
// tabstop. Requires indent_with_tabs=2. If false, will use spaces.
extern Option<bool>
indent_cmt_with_tabs;

// Whether to indent strings broken by '\' so that they line up.
extern Option<bool>
indent_align_string;

// The number of spaces to indent multi-line XML strings.
// Requires indent_align_string=true.
extern BoundedOption<unsigned, 0, 16>
indent_xml_string;

// Spaces to indent '{' from level.
extern BoundedOption<unsigned, 0, 16>
indent_brace;

// Whether braces are indented to the body level.
extern Option<bool>
indent_braces;

// Whether to disable indenting function braces if indent_braces=true.
extern Option<bool>
indent_braces_no_func;

// Whether to disable indenting class braces if indent_braces=true.
extern Option<bool>
indent_braces_no_class;

// Whether to disable indenting struct braces if indent_braces=true.
extern Option<bool>
indent_braces_no_struct;

// Whether to indent based on the size of the brace parent,
// i.e. 'if' => 3 spaces, 'for' => 4 spaces, etc.
extern Option<bool>
indent_brace_parent;

// Whether to indent based on the open parenthesis instead of the open brace
// in '({\n'.
extern Option<bool>
indent_paren_open_brace;

// (C#) Whether to indent the brace of a C# delegate by another level.
extern Option<bool>
indent_cs_delegate_brace;

// (C#) Whether to indent a C# delegate (to handle delegates with no brace) by
// another level.
extern Option<bool>
indent_cs_delegate_body;

// Whether to indent the body of a 'namespace'.
extern Option<bool>
indent_namespace;

// Whether to indent only the first namespace, and not any nested namespaces.
// Requires indent_namespace=true.
extern Option<bool>
indent_namespace_single_indent;

// The number of spaces to indent a namespace block.
// If set to zero, use the value indent_columns
extern BoundedOption<unsigned, 0, 16>
indent_namespace_level;

// If the body of the namespace is longer than this number, it won't be
// indented. Requires indent_namespace=true. 0 means no limit.
extern BoundedOption<unsigned, 0, 255>
indent_namespace_limit;

// Whether to indent only in inner namespaces (nested in other namespaces).
// Requires indent_namespace=true.
extern Option<bool>
indent_namespace_inner_only;

// Whether the 'extern "C"' body is indented.
extern Option<bool>
indent_extern;

// Whether the 'class' body is indented.
extern Option<bool>
indent_class;

// Whether to ignore indent for the leading base class colon.
extern Option<bool>
indent_ignore_before_class_colon;

// Additional indent before the leading base class colon.
// Negative values decrease indent down to the first column.
// Requires indent_ignore_before_class_colon=false and a newline break before
// the colon (see pos_class_colon and nl_class_colon)
extern BoundedOption<signed, -16, 16>
indent_before_class_colon;

// Whether to indent the stuff after a leading base class colon.
extern Option<bool>
indent_class_colon;

// Whether to indent based on a class colon instead of the stuff after the
// colon. Requires indent_class_colon=true.
extern Option<bool>
indent_class_on_colon;

// Whether to ignore indent for a leading class initializer colon.
extern Option<bool>
indent_ignore_before_constr_colon;

// Whether to indent the stuff after a leading class initializer colon.
extern Option<bool>
indent_constr_colon;

// Virtual indent from the ':' for leading member initializers.
extern BoundedOption<unsigned, 0, 16>
indent_ctor_init_leading; // = 2

// Virtual indent from the ':' for following member initializers.
extern BoundedOption<unsigned, 0, 16>
indent_ctor_init_following; // = 2

// Additional indent for constructor initializer list.
// Negative values decrease indent down to the first column.
extern BoundedOption<signed, -16, 16>
indent_ctor_init;

// Whether to indent 'if' following 'else' as a new block under the 'else'.
// If false, 'else\nif' is treated as 'else if' for indenting purposes.
extern Option<bool>
indent_else_if;

// Amount to indent variable declarations after a open brace.
//
//  <0: Relative
// >=0: Absolute
extern BoundedOption<signed, -16, 16>
indent_var_def_blk;

// Whether to indent continued variable declarations instead of aligning.
extern Option<bool>
indent_var_def_cont;

// How to indent continued shift expressions ('<<' and '>>').
// Set align_left_shift=false when using this.
//  0: Align shift operators instead of indenting them (default)
//  1: Indent by one level
// -1: Preserve original indentation
extern BoundedOption<signed, -1, 1>
indent_shift;

// Whether to force indentation of function definitions to start in column 1.
extern Option<bool>
indent_func_def_force_col1;

// Whether to indent continued function call parameters one indent level,
// rather than aligning parameters under the open parenthesis.
extern Option<bool>
indent_func_call_param;

// Whether to indent continued function definition parameters one indent level,
// rather than aligning parameters under the open parenthesis.
extern Option<bool>
indent_func_def_param;

// for function definitions, only if indent_func_def_param is false
// Allows to align params when appropriate and indent them when not
// behave as if it was true if paren position is more than this value
// if paren position is more than the option value
extern BoundedOption<unsigned, 0, 160>
indent_func_def_param_paren_pos_threshold;

// Whether to indent continued function call prototype one indent level,
// rather than aligning parameters under the open parenthesis.
extern Option<bool>
indent_func_proto_param;

// Whether to indent continued function call declaration one indent level,
// rather than aligning parameters under the open parenthesis.
extern Option<bool>
indent_func_class_param;

// Whether to indent continued class variable constructors one indent level,
// rather than aligning parameters under the open parenthesis.
extern Option<bool>
indent_func_ctor_var_param;

// Whether to indent continued template parameter list one indent level,
// rather than aligning parameters under the open parenthesis.
extern Option<bool>
indent_template_param;

// Double the indent for indent_func_xxx_param options.
// Use both values of the options indent_columns and indent_param.
extern Option<bool>
indent_func_param_double;

// Indentation column for standalone 'const' qualifier on a function
// prototype.
extern BoundedOption<unsigned, 0, 69>
indent_func_const;

// Indentation column for standalone 'throw' qualifier on a function
// prototype.
extern BoundedOption<unsigned, 0, 41>
indent_func_throw;

// How to indent within a macro followed by a brace on the same line
// This allows reducing the indent in macros that have (for example)
// `do { ... } while (0)` blocks bracketing them.
//
// true:  add an indent for the brace on the same line as the macro
// false: do not add an indent for the brace on the same line as the macro
extern Option<bool>
indent_macro_brace; // = true

// The number of spaces to indent a continued '->' or '.'.
// Usually set to 0, 1, or indent_columns.
extern BoundedOption<unsigned, 0, 16>
indent_member;

// Whether lines broken at '.' or '->' should be indented by a single indent.
// The indent_member option will not be effective if this is set to true.
extern Option<bool>
indent_member_single;

// Spaces to indent single line ('//') comments on lines before code.
extern BoundedOption<unsigned, 0, 16>
indent_single_line_comments_before;

// Spaces to indent single line ('//') comments on lines after code.
extern BoundedOption<unsigned, 0, 16>
indent_single_line_comments_after;

// When opening a paren for a control statement (if, for, while, etc), increase
// the indent level by this value. Negative values decrease the indent level.
extern BoundedOption<signed, -16, 16>
indent_sparen_extra;

// Whether to indent trailing single line ('//') comments relative to the code
// instead of trying to keep the same absolute column.
extern Option<bool>
indent_relative_single_line_comments;

// Spaces to indent 'case' from 'switch'. Usually 0 or indent_columns.
// It might be wise to choose the same value for the option indent_case_brace.
extern BoundedOption<unsigned, 0, 16>
indent_switch_case;

// Spaces to indent the body of a 'switch' before any 'case'.
// Usually the same as indent_columns or indent_switch_case.
extern BoundedOption<unsigned, 0, 16>
indent_switch_body;

// Whether to ignore indent for '{' following 'case'.
extern Option<bool>
indent_ignore_case_brace;

// Spaces to indent '{' from 'case'. By default, the brace will appear under
// the 'c' in case. Usually set to 0 or indent_columns. Negative values are OK.
// It might be wise to choose the same value for the option indent_switch_case.
extern BoundedOption<signed, -16, 16>
indent_case_brace;

// indent 'break' with 'case' from 'switch'.
extern Option<bool>
indent_switch_break_with_case;

// Whether to indent preprocessor statements inside of switch statements.
extern Option<bool>
indent_switch_pp; // = true

// Spaces to shift the 'case' line, without affecting any other lines.
// Usually 0.
extern BoundedOption<unsigned, 0, 16>
indent_case_shift;

// Whether to align comments before 'case' with the 'case'.
extern Option<bool>
indent_case_comment; // = true

// Whether to indent comments not found in first column.
extern Option<bool>
indent_comment; // = true

// Whether to indent comments found in first column.
extern Option<bool>
indent_col1_comment;

// Whether to indent multi string literal in first column.
extern Option<bool>
indent_col1_multi_string_literal;

// Align comments on adjacent lines that are this many columns apart or less.
extern BoundedOption<unsigned, 0, 16>
indent_comment_align_thresh; // = 3

// Whether to ignore indent for goto labels.
extern Option<bool>
indent_ignore_label;

// How to indent goto labels. Requires indent_ignore_label=false.
//
//  >0: Absolute column where 1 is the leftmost column
// <=0: Subtract from brace indent
extern BoundedOption<signed, -16, 16>
indent_label; // = 1

// How to indent access specifiers that are followed by a
// colon.
//
//  >0: Absolute column where 1 is the leftmost column
// <=0: Subtract from brace indent
extern BoundedOption<signed, -16, 16>
indent_access_spec; // = 1

// Whether to indent the code after an access specifier by one level.
// If true, this option forces 'indent_access_spec=0'.
extern Option<bool>
indent_access_spec_body;

// If an open parenthesis is followed by a newline, whether to indent the next
// line so that it lines up after the open parenthesis (not recommended).
extern Option<bool>
indent_paren_nl;

// How to indent a close parenthesis after a newline.
//
//  0: Indent to body level (default)
//  1: Align under the open parenthesis
//  2: Indent to the brace level
// -1: Preserve original indentation
extern BoundedOption<signed, -1, 2>
indent_paren_close;

// Whether to indent the open parenthesis of a function definition,
// if the parenthesis is on its own line.
extern Option<bool>
indent_paren_after_func_def;

// Whether to indent the open parenthesis of a function declaration,
// if the parenthesis is on its own line.
extern Option<bool>
indent_paren_after_func_decl;

// Whether to indent the open parenthesis of a function call,
// if the parenthesis is on its own line.
extern Option<bool>
indent_paren_after_func_call;

// How to indent a comma when inside braces.
//  0: Indent by one level (default)
//  1: Align under the open brace
// -1: Preserve original indentation
extern BoundedOption<signed, -1, 1>
indent_comma_brace;

// How to indent a comma when inside parentheses.
//  0: Indent by one level (default)
//  1: Align under the open parenthesis
// -1: Preserve original indentation
extern BoundedOption<signed, -1, 1>
indent_comma_paren;

// How to indent a Boolean operator when inside parentheses.
//  0: Indent by one level (default)
//  1: Align under the open parenthesis
// -1: Preserve original indentation
extern BoundedOption<signed, -1, 1>
indent_bool_paren;

// Whether to ignore the indentation of a Boolean operator when outside
// parentheses.
extern Option<bool>
indent_ignore_bool;


// Whether to indent lines that are nested in boolean expression one more level for each nesting
extern Option<bool>
indent_bool_nested_all; // = false

// Whether to ignore the indentation of an arithmetic operator.
extern Option<bool>
indent_ignore_arith;

// Whether to indent a semicolon when inside a for parenthesis.
// If true, aligns under the open for parenthesis.
extern Option<bool>
indent_semicolon_for_paren;

// Whether to ignore the indentation of a semicolon outside of a 'for'
// statement.
extern Option<bool>
indent_ignore_semicolon;

// Whether to align the first expression to following ones
// if indent_bool_paren=1.
extern Option<bool>
indent_first_bool_expr;

// Whether to align the first expression to following ones
// if indent_semicolon_for_paren=true.
extern Option<bool>
indent_first_for_expr;

// If an open square is followed by a newline, whether to indent the next line
// so that it lines up after the open square (not recommended).
extern Option<bool>
indent_square_nl;

// (ESQL/C) Whether to preserve the relative indent of 'EXEC SQL' bodies.
extern Option<bool>
indent_preserve_sql;

// Whether to ignore the indentation of an assignment operator.
extern Option<bool>
indent_ignore_assign;

// Whether to align continued statements at the '='. If false or if the '=' is
// followed by a newline, the next line is indent one tab.
extern Option<bool>
indent_align_assign; // = true

// If true, the indentation of the chunks after a '=' sequence will be set at
// LHS token indentation column before '='.
extern Option<bool>
indent_off_after_assign;

// Whether to align continued statements at the '('. If false or the '(' is
// followed by a newline, the next line indent is one tab.
extern Option<bool>
indent_align_paren; // = true

// (OC) Whether to indent Objective-C code inside message selectors.
extern Option<bool>
indent_oc_inside_msg_sel;

// (OC) Whether to indent Objective-C blocks at brace level instead of usual
// rules.
extern Option<bool>
indent_oc_block;

// (OC) Indent for Objective-C blocks in a message relative to the parameter
// name.
//
// =0: Use indent_oc_block rules
// >0: Use specified number of spaces to indent
extern BoundedOption<unsigned, 0, 16>
indent_oc_block_msg;

// (OC) Minimum indent for subsequent parameters
extern BoundedOption<unsigned, 0, 5000>
indent_oc_msg_colon;

// (OC) Whether to prioritize aligning with initial colon (and stripping spaces
// from lines, if necessary).
extern Option<bool>
indent_oc_msg_prioritize_first_colon; // = true

// (OC) Whether to indent blocks the way that Xcode does by default
// (from the keyword if the parameter is on its own line; otherwise, from the
// previous indentation level). Requires indent_oc_block_msg=true.
extern Option<bool>
indent_oc_block_msg_xcode_style;

// (OC) Whether to indent blocks from where the brace is, relative to a
// message keyword. Requires indent_oc_block_msg=true.
extern Option<bool>
indent_oc_block_msg_from_keyword;

// (OC) Whether to indent blocks from where the brace is, relative to a message
// colon. Requires indent_oc_block_msg=true.
extern Option<bool>
indent_oc_block_msg_from_colon;

// (OC) Whether to indent blocks from where the block caret is.
// Requires indent_oc_block_msg=true.
extern Option<bool>
indent_oc_block_msg_from_caret;

// (OC) Whether to indent blocks from where the brace caret is.
// Requires indent_oc_block_msg=true.
extern Option<bool>
indent_oc_block_msg_from_brace;

// When indenting after virtual brace open and newline add further spaces to
// reach this minimum indent.
extern BoundedOption<unsigned, 0, 16>
indent_min_vbrace_open;

// Whether to add further spaces after regular indent to reach next tabstop
// when indenting after virtual brace open and newline.
extern Option<bool>
indent_vbrace_open_on_tabstop;

// How to indent after a brace followed by another token (not a newline).
// true:  indent all contained lines to match the token
// false: indent all contained lines to match the brace
extern Option<bool>
indent_token_after_brace; // = true

// Whether to indent the body of a C++11 lambda.
extern Option<bool>
indent_cpp_lambda_body;

// How to indent compound literals that are being returned.
// true: add both the indent from return & the compound literal open brace
//       (i.e. 2 indent levels)
// false: only indent 1 level, don't add the indent for the open brace, only
//        add the indent for the return.
extern Option<bool>
indent_compound_literal_return; // = true

// (C#) Whether to indent a 'using' block if no braces are used.
extern Option<bool>
indent_using_block; // = true

// How to indent the continuation of ternary operator.
//
// 0: Off (default)
// 1: When the `if_false` is a continuation, indent it under the `if_true` branch
// 2: When the `:` is a continuation, indent it under `?`
extern BoundedOption<unsigned, 0, 2>
indent_ternary_operator;

// Whether to indent the statements inside ternary operator.
extern Option<bool>
indent_inside_ternary_operator;

// If true, the indentation of the chunks after a `return` sequence will be set at return indentation column.
extern Option<bool>
indent_off_after_return;

// If true, the indentation of the chunks after a `return new` sequence will be set at return indentation column.
extern Option<bool>
indent_off_after_return_new;

// If true, the tokens after return are indented with regular single indentation. By default (false) the indentation is after the return token.
extern Option<bool>
indent_single_after_return;

// Whether to ignore indent and alignment for 'asm' blocks (i.e. assume they
// have their own indentation).
extern Option<bool>
indent_ignore_asm_block;

// Don't indent the close parenthesis of a function definition,
// if the parenthesis is on its own line.
extern Option<bool>
donot_indent_func_def_close_paren;

//END

///////////////////////////////////////////////////////////////////////////////
//BEGIN Newline adding and removing options

// Whether to collapse empty blocks between '{' and '}' except for functions.
// Use nl_collapse_empty_body_functions to specify how empty function braces
// should be formatted.
extern Option<bool>
nl_collapse_empty_body;

// Whether to collapse empty blocks between '{' and '}' for functions only.
// If true, overrides nl_inside_empty_func.
extern Option<bool>
nl_collapse_empty_body_functions;

// Don't split one-line braced assignments, as in 'foo_t f = { 1, 2 };'.
extern Option<bool>
nl_assign_leave_one_liners;

// Don't split one-line braced statements inside a 'class xx { }' body.
extern Option<bool>
nl_class_leave_one_liners;

// Don't split one-line enums, as in 'enum foo { BAR = 15 };'
extern Option<bool>
nl_enum_leave_one_liners;

// Don't split one-line get or set functions.
extern Option<bool>
nl_getset_leave_one_liners;

// (C#) Don't split one-line property get or set functions.
extern Option<bool>
nl_cs_property_leave_one_liners;

// Don't split one-line function definitions, as in 'int foo() { return 0; }'.
// might modify nl_func_type_name
extern Option<bool>
nl_func_leave_one_liners;

// Don't split one-line C++11 lambdas, as in '[]() { return 0; }'.
extern Option<bool>
nl_cpp_lambda_leave_one_liners;

// Don't split one-line if/else statements, as in 'if(...) b++;'.
extern Option<bool>
nl_if_leave_one_liners;

// Don't split one-line while statements, as in 'while(...) b++;'.
extern Option<bool>
nl_while_leave_one_liners;

// Don't split one-line do statements, as in 'do { b++; } while(...);'.
extern Option<bool>
nl_do_leave_one_liners;

// Don't split one-line for statements, as in 'for(...) b++;'.
extern Option<bool>
nl_for_leave_one_liners;

// (OC) Don't split one-line Objective-C messages.
extern Option<bool>
nl_oc_msg_leave_one_liner;

// (OC) Add or remove newline between method declaration and '{'.
extern Option<iarf_e>
nl_oc_mdef_brace;

// (OC) Add or remove newline between Objective-C block signature and '{'.
extern Option<iarf_e>
nl_oc_block_brace;

// (OC) Add or remove blank line before '@interface' statement.
extern Option<iarf_e>
nl_oc_before_interface;

// (OC) Add or remove blank line before '@implementation' statement.
extern Option<iarf_e>
nl_oc_before_implementation;

// (OC) Add or remove blank line before '@end' statement.
extern Option<iarf_e>
nl_oc_before_end;

// (OC) Add or remove newline between '@interface' and '{'.
extern Option<iarf_e>
nl_oc_interface_brace;

// (OC) Add or remove newline between '@implementation' and '{'.
extern Option<iarf_e>
nl_oc_implementation_brace;

// Add or remove newlines at the start of the file.
extern Option<iarf_e>
nl_start_of_file;

// The minimum number of newlines at the start of the file (only used if
// nl_start_of_file is 'add' or 'force').
extern BoundedOption<unsigned, 0, 16>
nl_start_of_file_min;

// Add or remove newline at the end of the file.
extern Option<iarf_e>
nl_end_of_file;

// The minimum number of newlines at the end of the file (only used if
// nl_end_of_file is 'add' or 'force').
extern BoundedOption<unsigned, 0, 16>
nl_end_of_file_min;

// Add or remove newline between '=' and '{'.
extern Option<iarf_e>
nl_assign_brace;

// (D) Add or remove newline between '=' and '['.
extern Option<iarf_e>
nl_assign_square;

// Add or remove newline between '[]' and '{'.
extern Option<iarf_e>
nl_tsquare_brace;

// (D) Add or remove newline after '= ['. Will also affect the newline before
// the ']'.
extern Option<iarf_e>
nl_after_square_assign;

// Add or remove newline between a function call's ')' and '{', as in
// 'list_for_each(item, &list) { }'.
extern Option<iarf_e>
nl_fcall_brace;

// Add or remove newline between 'enum' and '{'.
extern Option<iarf_e>
nl_enum_brace;

// Add or remove newline between 'enum' and 'class'.
extern Option<iarf_e>
nl_enum_class;

// Add or remove newline between 'enum class' and the identifier.
extern Option<iarf_e>
nl_enum_class_identifier;

// Add or remove newline between 'enum class' type and ':'.
extern Option<iarf_e>
nl_enum_identifier_colon;

// Add or remove newline between 'enum class identifier :' and type.
extern Option<iarf_e>
nl_enum_colon_type;

// Add or remove newline between 'struct and '{'.
extern Option<iarf_e>
nl_struct_brace;

// Add or remove newline between 'union' and '{'.
extern Option<iarf_e>
nl_union_brace;

// Add or remove newline between 'if' and '{'.
extern Option<iarf_e>
nl_if_brace;

// Add or remove newline between '}' and 'else'.
extern Option<iarf_e>
nl_brace_else;

// Add or remove newline between 'else if' and '{'. If set to ignore,
// nl_if_brace is used instead.
extern Option<iarf_e>
nl_elseif_brace;

// Add or remove newline between 'else' and '{'.
extern Option<iarf_e>
nl_else_brace;

// Add or remove newline between 'else' and 'if'.
extern Option<iarf_e>
nl_else_if;

// Add or remove newline before '{' opening brace
extern Option<iarf_e>
nl_before_opening_brace_func_class_def;

// Add or remove newline before 'if'/'else if' closing parenthesis.
extern Option<iarf_e>
nl_before_if_closing_paren;

// Add or remove newline between '}' and 'finally'.
extern Option<iarf_e>
nl_brace_finally;

// Add or remove newline between 'finally' and '{'.
extern Option<iarf_e>
nl_finally_brace;

// Add or remove newline between 'try' and '{'.
extern Option<iarf_e>
nl_try_brace;

// Add or remove newline between get/set and '{'.
extern Option<iarf_e>
nl_getset_brace;

// Add or remove newline between 'for' and '{'.
extern Option<iarf_e>
nl_for_brace;

// Add or remove newline before the '{' of a 'catch' statement, as in
// 'catch (decl) <here> {'.
extern Option<iarf_e>
nl_catch_brace;

// (OC) Add or remove newline before the '{' of a '@catch' statement, as in
// '@catch (decl) <here> {'. If set to ignore, nl_catch_brace is used.
extern Option<iarf_e>
nl_oc_catch_brace;

// Add or remove newline between '}' and 'catch'.
extern Option<iarf_e>
nl_brace_catch;

// (OC) Add or remove newline between '}' and '@catch'. If set to ignore,
// nl_brace_catch is used.
extern Option<iarf_e>
nl_oc_brace_catch;

// Add or remove newline between '}' and ']'.
extern Option<iarf_e>
nl_brace_square;

// Add or remove newline between '}' and ')' in a function invocation.
extern Option<iarf_e>
nl_brace_fparen;

// Add or remove newline between 'while' and '{'.
extern Option<iarf_e>
nl_while_brace;

// (D) Add or remove newline between 'scope (x)' and '{'.
extern Option<iarf_e>
nl_scope_brace;

// (D) Add or remove newline between 'unittest' and '{'.
extern Option<iarf_e>
nl_unittest_brace;

// (D) Add or remove newline between 'version (x)' and '{'.
extern Option<iarf_e>
nl_version_brace;

// (C#) Add or remove newline between 'using' and '{'.
extern Option<iarf_e>
nl_using_brace;

// Add or remove newline between two open or close braces. Due to general
// newline/brace handling, REMOVE may not work.
extern Option<iarf_e>
nl_brace_brace;

// Add or remove newline between 'do' and '{'.
extern Option<iarf_e>
nl_do_brace;

// Add or remove newline between '}' and 'while' of 'do' statement.
extern Option<iarf_e>
nl_brace_while;

// Add or remove newline between 'switch' and '{'.
extern Option<iarf_e>
nl_switch_brace;

// Add or remove newline between 'synchronized' and '{'.
extern Option<iarf_e>
nl_synchronized_brace;

// Add a newline between ')' and '{' if the ')' is on a different line than the
// if/for/etc.
//
// Overrides nl_for_brace, nl_if_brace, nl_switch_brace, nl_while_switch and
// nl_catch_brace.
extern Option<bool>
nl_multi_line_cond;

// Add a newline after '(' if an if/for/while/switch condition spans multiple
// lines
extern Option<iarf_e>
nl_multi_line_sparen_open;

// Add a newline before ')' if an if/for/while/switch condition spans multiple
// lines. Overrides nl_before_if_closing_paren if both are specified.
extern Option<iarf_e>
nl_multi_line_sparen_close;

// Force a newline in a define after the macro name for multi-line defines.
extern Option<bool>
nl_multi_line_define;

// Whether to add a newline before 'case', and a blank line before a 'case'
// statement that follows a ';' or '}'.
extern Option<bool>
nl_before_case;

// Whether to add a newline after a 'case' statement.
extern Option<bool>
nl_after_case;

// Add or remove newline between a case ':' and '{'.
//
// Overrides nl_after_case.
extern Option<iarf_e>
nl_case_colon_brace;

// Add or remove newline between ')' and 'throw'.
extern Option<iarf_e>
nl_before_throw;

// Add or remove newline between 'namespace' and '{'.
extern Option<iarf_e>
nl_namespace_brace;

// Add or remove newline after 'template<...>' of a template class.
extern Option<iarf_e>
nl_template_class;

// Add or remove newline after 'template<...>' of a template class declaration.
//
// Overrides nl_template_class.
extern Option<iarf_e>
nl_template_class_decl;

// Add or remove newline after 'template<>' of a specialized class declaration.
//
// Overrides nl_template_class_decl.
extern Option<iarf_e>
nl_template_class_decl_special;

// Add or remove newline after 'template<...>' of a template class definition.
//
// Overrides nl_template_class.
extern Option<iarf_e>
nl_template_class_def;

// Add or remove newline after 'template<>' of a specialized class definition.
//
// Overrides nl_template_class_def.
extern Option<iarf_e>
nl_template_class_def_special;

// Add or remove newline after 'template<...>' of a template function.
extern Option<iarf_e>
nl_template_func;

// Add or remove newline after 'template<...>' of a template function
// declaration.
//
// Overrides nl_template_func.
extern Option<iarf_e>
nl_template_func_decl;

// Add or remove newline after 'template<>' of a specialized function
// declaration.
//
// Overrides nl_template_func_decl.
extern Option<iarf_e>
nl_template_func_decl_special;

// Add or remove newline after 'template<...>' of a template function
// definition.
//
// Overrides nl_template_func.
extern Option<iarf_e>
nl_template_func_def;

// Add or remove newline after 'template<>' of a specialized function
// definition.
//
// Overrides nl_template_func_def.
extern Option<iarf_e>
nl_template_func_def_special;

// Add or remove newline after 'template<...>' of a template variable.
extern Option<iarf_e>
nl_template_var;

// Add or remove newline between 'template<...>' and 'using' of a templated
// type alias.
extern Option<iarf_e>
nl_template_using;

// Add or remove newline between 'class' and '{'.
extern Option<iarf_e>
nl_class_brace;

// Add or remove newline before or after (depending on pos_class_comma,
// may not be IGNORE) each',' in the base class list.
extern Option<iarf_e>
nl_class_init_args;

// Add or remove newline after each ',' in the constructor member
// initialization. Related to nl_constr_colon, pos_constr_colon and
// pos_constr_comma.
extern Option<iarf_e>
nl_constr_init_args;

// Add or remove newline before first element, after comma, and after last
// element, in 'enum'.
extern Option<iarf_e>
nl_enum_own_lines;

// Add or remove newline between return type and function name in a function
// definition.
// might be modified by nl_func_leave_one_liners
extern Option<iarf_e>
nl_func_type_name;

// Add or remove newline between return type and function name inside a class
// definition. If set to ignore, nl_func_type_name or nl_func_proto_type_name
// is used instead.
extern Option<iarf_e>
nl_func_type_name_class;

// Add or remove newline between class specification and '::'
// in 'void A::f() { }'. Only appears in separate member implementation (does
// not appear with in-line implementation).
extern Option<iarf_e>
nl_func_class_scope;

// Add or remove newline between function scope and name, as in
// 'void A :: <here> f() { }'.
extern Option<iarf_e>
nl_func_scope_name;

// Add or remove newline between return type and function name in a prototype.
extern Option<iarf_e>
nl_func_proto_type_name;

// Add or remove newline between a function name and the opening '(' in the
// declaration.
extern Option<iarf_e>
nl_func_paren;

// Overrides nl_func_paren for functions with no parameters.
extern Option<iarf_e>
nl_func_paren_empty;

// Add or remove newline between a function name and the opening '(' in the
// definition.
extern Option<iarf_e>
nl_func_def_paren;

// Overrides nl_func_def_paren for functions with no parameters.
extern Option<iarf_e>
nl_func_def_paren_empty;

// Add or remove newline between a function name and the opening '(' in the
// call.
extern Option<iarf_e>
nl_func_call_paren;

// Overrides nl_func_call_paren for functions with no parameters.
extern Option<iarf_e>
nl_func_call_paren_empty;

// Add or remove newline after '(' in a function declaration.
extern Option<iarf_e>
nl_func_decl_start;

// Add or remove newline after '(' in a function definition.
extern Option<iarf_e>
nl_func_def_start;

// Overrides nl_func_decl_start when there is only one parameter.
extern Option<iarf_e>
nl_func_decl_start_single;

// Overrides nl_func_def_start when there is only one parameter.
extern Option<iarf_e>
nl_func_def_start_single;

// Whether to add a newline after '(' in a function declaration if '(' and ')'
// are in different lines. If false, nl_func_decl_start is used instead.
extern Option<bool>
nl_func_decl_start_multi_line;

// Whether to add a newline after '(' in a function definition if '(' and ')'
// are in different lines. If false, nl_func_def_start is used instead.
extern Option<bool>
nl_func_def_start_multi_line;

// Add or remove newline after each ',' in a function declaration.
extern Option<iarf_e>
nl_func_decl_args;

// Add or remove newline after each ',' in a function definition.
extern Option<iarf_e>
nl_func_def_args;

// Add or remove newline after each ',' in a function call.
extern Option<iarf_e>
nl_func_call_args;

// Whether to add a newline after each ',' in a function declaration if '('
// and ')' are in different lines. If false, nl_func_decl_args is used instead.
extern Option<bool>
nl_func_decl_args_multi_line;

// Whether to add a newline after each ',' in a function definition if '('
// and ')' are in different lines. If false, nl_func_def_args is used instead.
extern Option<bool>
nl_func_def_args_multi_line;

// Add or remove newline before the ')' in a function declaration.
extern Option<iarf_e>
nl_func_decl_end;

// Add or remove newline before the ')' in a function definition.
extern Option<iarf_e>
nl_func_def_end;

// Overrides nl_func_decl_end when there is only one parameter.
extern Option<iarf_e>
nl_func_decl_end_single;

// Overrides nl_func_def_end when there is only one parameter.
extern Option<iarf_e>
nl_func_def_end_single;

// Whether to add a newline before ')' in a function declaration if '(' and ')'
// are in different lines. If false, nl_func_decl_end is used instead.
extern Option<bool>
nl_func_decl_end_multi_line;

// Whether to add a newline before ')' in a function definition if '(' and ')'
// are in different lines. If false, nl_func_def_end is used instead.
extern Option<bool>
nl_func_def_end_multi_line;

// Add or remove newline between '()' in a function declaration.
extern Option<iarf_e>
nl_func_decl_empty;

// Add or remove newline between '()' in a function definition.
extern Option<iarf_e>
nl_func_def_empty;

// Add or remove newline between '()' in a function call.
extern Option<iarf_e>
nl_func_call_empty;

// Whether to add a newline after '(' in a function call,
// has preference over nl_func_call_start_multi_line.
extern Option<iarf_e>
nl_func_call_start;

// Whether to add a newline before ')' in a function call.
extern Option<iarf_e>
nl_func_call_end;

// Whether to add a newline after '(' in a function call if '(' and ')' are in
// different lines.
extern Option<bool>
nl_func_call_start_multi_line;

// Whether to add a newline after each ',' in a function call if '(' and ')'
// are in different lines.
extern Option<bool>
nl_func_call_args_multi_line;

// Whether to add a newline before ')' in a function call if '(' and ')' are in
// different lines.
extern Option<bool>
nl_func_call_end_multi_line;

// Whether to respect nl_func_call_XXX option in case of closure args.
extern Option<bool>
nl_func_call_args_multi_line_ignore_closures; // false

// Whether to add a newline after '<' of a template parameter list.
extern Option<bool>
nl_template_start;

// Whether to add a newline after each ',' in a template parameter list.
extern Option<bool>
nl_template_args;

// Whether to add a newline before '>' of a template parameter list.
extern Option<bool>
nl_template_end;

// (OC) Whether to put each Objective-C message parameter on a separate line.
// See nl_oc_msg_leave_one_liner.
extern Option<bool>
nl_oc_msg_args;

// (OC) Minimum number of Objective-C message parameters before applying nl_oc_msg_args.
extern BoundedOption<unsigned, 0, 16>
nl_oc_msg_args_min_params;

// (OC) Max code width of Objective-C message before applying nl_oc_msg_args.
extern BoundedOption<unsigned, 0, 10000>
nl_oc_msg_args_max_code_width;

// (OC) Whether to apply nl_oc_msg_args if some of the parameters are already
// on new lines. Overrides nl_oc_msg_args_min_params and nl_oc_msg_args_max_code_width.
extern Option<bool>
nl_oc_msg_args_finish_multi_line;

// Add or remove newline between function signature and '{'.
extern Option<iarf_e>
nl_fdef_brace;

// Add or remove newline between function signature and '{',
// if signature ends with ')'. Overrides nl_fdef_brace.
extern Option<iarf_e>
nl_fdef_brace_cond;

// Add or remove newline between C++11 lambda signature and '{'.
extern Option<iarf_e>
nl_cpp_ldef_brace;

// Add or remove newline between 'return' and the return expression.
extern Option<iarf_e>
nl_return_expr;

// Add or remove newline between 'throw' and the throw expression.
extern Option<iarf_e>
nl_throw_expr;

// Whether to add a newline after semicolons, except in 'for' statements.
extern Option<bool>
nl_after_semicolon;

// (Java) Add or remove newline between the ')' and '{{' of the double brace
// initializer.
extern Option<iarf_e>
nl_paren_dbrace_open;

// Whether to add a newline after the type in an unnamed temporary
// direct-list-initialization, better:
// before a direct-list-initialization.
extern Option<iarf_e>
nl_type_brace_init_lst;

// Whether to add a newline after the open brace in an unnamed temporary
// direct-list-initialization.
extern Option<iarf_e>
nl_type_brace_init_lst_open;

// Whether to add a newline before the close brace in an unnamed temporary
// direct-list-initialization.
extern Option<iarf_e>
nl_type_brace_init_lst_close;

// Whether to add a newline before '{'.
extern Option<bool>
nl_before_brace_open;

// Whether to add a newline after '{'.
extern Option<bool>
nl_after_brace_open;

// Whether to add a newline between the open brace and a trailing single-line
// comment. Requires nl_after_brace_open=true.
extern Option<bool>
nl_after_brace_open_cmt;

// Whether to add a newline after a virtual brace open with a non-empty body.
// These occur in un-braced if/while/do/for statement bodies.
extern Option<bool>
nl_after_vbrace_open;

// Whether to add a newline after a virtual brace open with an empty body.
// These occur in un-braced if/while/do/for statement bodies.
extern Option<bool>
nl_after_vbrace_open_empty;

// Whether to add a newline after '}'. Does not apply if followed by a
// necessary ';'.
extern Option<bool>
nl_after_brace_close;

// Whether to add a newline after a virtual brace close,
// as in 'if (foo) a++; <here> return;'.
extern Option<bool>
nl_after_vbrace_close;

// Add or remove newline between the close brace and identifier,
// as in 'struct { int a; } <here> b;'. Affects enumerations, unions and
// structures. If set to ignore, uses nl_after_brace_close.
extern Option<iarf_e>
nl_brace_struct_var;

// Whether to alter newlines in '#define' macros.
extern Option<bool>
nl_define_macro;

// Whether to alter newlines between consecutive parenthesis closes. The number
// of closing parentheses in a line will depend on respective open parenthesis
// lines.
extern Option<bool>
nl_squeeze_paren_close;

// Whether to remove blanks after '#ifxx' and '#elxx', or before '#elxx' and
// '#endif'. Does not affect top-level #ifdefs.
extern Option<bool>
nl_squeeze_ifdef;

// Makes the nl_squeeze_ifdef option affect the top-level #ifdefs as well.
extern Option<bool>
nl_squeeze_ifdef_top_level;

// Add or remove blank line before 'if'.
extern Option<iarf_e>
nl_before_if;

// Add or remove blank line after 'if' statement. Add/Force work only if the
// next token is not a closing brace.
extern Option<iarf_e>
nl_after_if;

// Add or remove blank line before 'for'.
extern Option<iarf_e>
nl_before_for;

// Add or remove blank line after 'for' statement.
extern Option<iarf_e>
nl_after_for;

// Add or remove blank line before 'while'.
extern Option<iarf_e>
nl_before_while;

// Add or remove blank line after 'while' statement.
extern Option<iarf_e>
nl_after_while;

// Add or remove blank line before 'switch'.
extern Option<iarf_e>
nl_before_switch;

// Add or remove blank line after 'switch' statement.
extern Option<iarf_e>
nl_after_switch;

// Add or remove blank line before 'synchronized'.
extern Option<iarf_e>
nl_before_synchronized;

// Add or remove blank line after 'synchronized' statement.
extern Option<iarf_e>
nl_after_synchronized;

// Add or remove blank line before 'do'.
extern Option<iarf_e>
nl_before_do;

// Add or remove blank line after 'do/while' statement.
extern Option<iarf_e>
nl_after_do;

// Ignore nl_before_{if,for,switch,do,synchronized} if the control
// statement is immediately after a case statement.
// if nl_before_{if,for,switch,do} is set to remove, this option
// does nothing.
extern Option<bool>
nl_before_ignore_after_case;

// Whether to put a blank line before 'return' statements, unless after an open
// brace.
extern Option<bool>
nl_before_return;

// Whether to put a blank line after 'return' statements, unless followed by a
// close brace.
extern Option<bool>
nl_after_return;

// Whether to put a blank line before a member '.' or '->' operators.
extern Option<iarf_e>
nl_before_member;

// (Java) Whether to put a blank line after a member '.' or '->' operators.
extern Option<iarf_e>
nl_after_member;

// Whether to double-space commented-entries in 'struct'/'union'/'enum'.
extern Option<bool>
nl_ds_struct_enum_cmt;

// Whether to force a newline before '}' of a 'struct'/'union'/'enum'.
// (Lower priority than eat_blanks_before_close_brace.)
extern Option<bool>
nl_ds_struct_enum_close_brace;

// Add or remove newline before or after (depending on pos_class_colon) a class
// colon, as in 'class Foo <here> : <or here> public Bar'.
extern Option<iarf_e>
nl_class_colon;

// Add or remove newline around a class constructor colon. The exact position
// depends on nl_constr_init_args, pos_constr_colon and pos_constr_comma.
extern Option<iarf_e>
nl_constr_colon;

// Whether to collapse a two-line namespace, like 'namespace foo\n{ decl; }'
// into a single line. If true, prevents other brace newline rules from turning
// such code into four lines. If true, it also preserves one-liner namespaces.
extern Option<bool>
nl_namespace_two_to_one_liner;

// Whether to remove a newline in simple unbraced if statements, turning them
// into one-liners, as in 'if(b)\n i++;' => 'if(b) i++;'.
extern Option<bool>
nl_create_if_one_liner;

// Whether to remove a newline in simple unbraced for statements, turning them
// into one-liners, as in 'for (...)\n stmt;' => 'for (...) stmt;'.
extern Option<bool>
nl_create_for_one_liner;

// Whether to remove a newline in simple unbraced while statements, turning
// them into one-liners, as in 'while (expr)\n stmt;' => 'while (expr) stmt;'.
extern Option<bool>
nl_create_while_one_liner;

// Whether to collapse a function definition whose body (not counting braces)
// is only one line so that the entire definition (prototype, braces, body) is
// a single line.
extern Option<bool>
nl_create_func_def_one_liner;

// Whether to split one-line simple list definitions into three lines by
// adding newlines, as in 'int a[12] = { <here> 0 <here> };'.
extern Option<bool>
nl_create_list_one_liner;

// Whether to split one-line simple unbraced if statements into two lines by
// adding a newline, as in 'if(b) <here> i++;'.
extern Option<bool>
nl_split_if_one_liner;

// Whether to split one-line simple unbraced for statements into two lines by
// adding a newline, as in 'for (...) <here> stmt;'.
extern Option<bool>
nl_split_for_one_liner;

// Whether to split one-line simple unbraced while statements into two lines by
// adding a newline, as in 'while (expr) <here> stmt;'.
extern Option<bool>
nl_split_while_one_liner;

// Don't add a newline before a cpp-comment in a parameter list of a function
// call.
extern Option<bool>
donot_add_nl_before_cpp_comment;

//END

///////////////////////////////////////////////////////////////////////////////
//BEGIN Blank line options

// The maximum number of consecutive newlines (3 = 2 blank lines).
extern BoundedOption<unsigned, 0, 16>
nl_max;

// The maximum number of consecutive newlines in a function.
extern BoundedOption<unsigned, 0, 16>
nl_max_blank_in_func;

// The number of newlines inside an empty function body.
// This option overrides eat_blanks_after_open_brace and
// eat_blanks_before_close_brace, but is ignored when
// nl_collapse_empty_body_functions=true
extern BoundedOption<unsigned, 0, 16>
nl_inside_empty_func;

// The number of newlines before a function prototype.
extern BoundedOption<unsigned, 0, 16>
nl_before_func_body_proto;

// The number of newlines before a multi-line function definition. Where
// applicable, this option is overridden with eat_blanks_after_open_brace=true
extern BoundedOption<unsigned, 0, 16>
nl_before_func_body_def;

// The number of newlines before a class constructor/destructor prototype.
extern BoundedOption<unsigned, 0, 16>
nl_before_func_class_proto;

// The number of newlines before a class constructor/destructor definition.
extern BoundedOption<unsigned, 0, 16>
nl_before_func_class_def;

// The number of newlines after a function prototype.
extern BoundedOption<unsigned, 0, 16>
nl_after_func_proto;

// The number of newlines after a function prototype, if not followed by
// another function prototype.
extern BoundedOption<unsigned, 0, 16>
nl_after_func_proto_group;

// The number of newlines after a class constructor/destructor prototype.
extern BoundedOption<unsigned, 0, 16>
nl_after_func_class_proto;

// The number of newlines after a class constructor/destructor prototype,
// if not followed by another constructor/destructor prototype.
extern BoundedOption<unsigned, 0, 16>
nl_after_func_class_proto_group;

// Whether one-line method definitions inside a class body should be treated
// as if they were prototypes for the purposes of adding newlines.
//
// Requires nl_class_leave_one_liners=true. Overrides nl_before_func_body_def
// and nl_before_func_class_def for one-liners.
extern Option<bool>
nl_class_leave_one_liner_groups;

// The number of newlines after '}' of a multi-line function body.
//
// Overrides nl_min_after_func_body and nl_max_after_func_body.
extern BoundedOption<unsigned, 0, 16>
nl_after_func_body;

// The minimum number of newlines after '}' of a multi-line function body.
//
// Only works when nl_after_func_body is 0.
extern BoundedOption<unsigned, 0, 16>
nl_min_after_func_body;

// The maximum number of newlines after '}' of a multi-line function body.
//
// Only works when nl_after_func_body is 0.
// Takes precedence over nl_min_after_func_body.
extern BoundedOption<unsigned, 0, 16>
nl_max_after_func_body;

// The number of newlines after '}' of a multi-line function body in a class
// declaration. Also affects class constructors/destructors.
//
// Overrides nl_after_func_body.
extern BoundedOption<unsigned, 0, 16>
nl_after_func_body_class;

// The number of newlines after '}' of a single line function body. Also
// affects class constructors/destructors.
//
// Overrides nl_after_func_body and nl_after_func_body_class.
extern BoundedOption<unsigned, 0, 16>
nl_after_func_body_one_liner;

// The number of newlines before a block of typedefs. If nl_after_access_spec
// is non-zero, that option takes precedence.
//
// 0: No change (default).
extern BoundedOption<unsigned, 0, 16>
nl_typedef_blk_start;

// The number of newlines after a block of typedefs.
//
// 0: No change (default).
extern BoundedOption<unsigned, 0, 16>
nl_typedef_blk_end;

// The maximum number of consecutive newlines within a block of typedefs.
//
// 0: No change (default).
extern BoundedOption<unsigned, 0, 16>
nl_typedef_blk_in;

// The minimum number of blank lines after a block of variable definitions
// at the top of a function body. If any preprocessor directives appear
// between the opening brace of the function and the variable block, then
// it is considered as not at the top of the function.Newlines are added
// before trailing preprocessor directives, if any exist.
//
// 0: No change (default).
extern BoundedOption<unsigned, 0, 16>
nl_var_def_blk_end_func_top;

// The minimum number of empty newlines before a block of variable definitions
// not at the top of a function body. If nl_after_access_spec is non-zero,
// that option takes precedence. Newlines are not added at the top of the
// file or just after an opening brace. Newlines are added above any
// preprocessor directives before the block.
//
// 0: No change (default).
extern BoundedOption<unsigned, 0, 16>
nl_var_def_blk_start;

// The minimum number of empty newlines after a block of variable definitions
// not at the top of a function body. Newlines are not added if the block
// is at the bottom of the file or just before a preprocessor directive.
//
// 0: No change (default).
extern BoundedOption<unsigned, 0, 16>
nl_var_def_blk_end;

// The maximum number of consecutive newlines within a block of variable
// definitions.
//
// 0: No change (default).
extern BoundedOption<unsigned, 0, 16>
nl_var_def_blk_in;

// The minimum number of newlines before a multi-line comment.
// Doesn't apply if after a brace open or another multi-line comment.
extern BoundedOption<unsigned, 0, 16>
nl_before_block_comment;

// The minimum number of newlines before a single-line C comment.
// Doesn't apply if after a brace open or other single-line C comments.
extern BoundedOption<unsigned, 0, 16>
nl_before_c_comment;

// The minimum number of newlines before a CPP comment.
// Doesn't apply if after a brace open or other CPP comments.
extern BoundedOption<unsigned, 0, 16>
nl_before_cpp_comment;

// Whether to force a newline after a multi-line comment.
extern Option<bool>
nl_after_multiline_comment;

// Whether to force a newline after a label's colon.
extern Option<bool>
nl_after_label_colon;

// The number of newlines before a struct definition.
extern BoundedOption<unsigned, 0, 16>
nl_before_struct;

// The number of newlines after '}' or ';' of a struct/enum/union definition.
extern BoundedOption<unsigned, 0, 16>
nl_after_struct;

// The number of newlines before a class definition.
extern BoundedOption<unsigned, 0, 16>
nl_before_class;

// The number of newlines after '}' or ';' of a class definition.
extern BoundedOption<unsigned, 0, 16>
nl_after_class;

// The number of newlines before a namespace.
extern BoundedOption<unsigned, 0, 16>
nl_before_namespace;

// The number of newlines after '{' of a namespace. This also adds newlines
// before the matching '}'.
//
// 0: Apply eat_blanks_after_open_brace or eat_blanks_before_close_brace if
//     applicable, otherwise no change.
//
// Overrides eat_blanks_after_open_brace and eat_blanks_before_close_brace.
extern BoundedOption<unsigned, 0, 16>
nl_inside_namespace;

// The number of newlines after '}' of a namespace.
extern BoundedOption<unsigned, 0, 16>
nl_after_namespace;

// The number of newlines before an access specifier label. This also includes
// the Qt-specific 'signals:' and 'slots:'. Will not change the newline count
// if after a brace open.
//
// 0: No change (default).
extern BoundedOption<unsigned, 0, 16>
nl_before_access_spec;

// The number of newlines after an access specifier label. This also includes
// the Qt-specific 'signals:' and 'slots:'. Will not change the newline count
// if after a brace open.
//
// 0: No change (default).
//
// Overrides nl_typedef_blk_start and nl_var_def_blk_start.
extern BoundedOption<unsigned, 0, 16>
nl_after_access_spec;

// The number of newlines between a function definition and the function
// comment, as in '// comment\n <here> void foo() {...}'.
//
// 0: No change (default).
extern BoundedOption<unsigned, 0, 16>
nl_comment_func_def;

// The number of newlines after a try-catch-finally block that isn't followed
// by a brace close.
//
// 0: No change (default).
extern BoundedOption<unsigned, 0, 16>
nl_after_try_catch_finally;

// (C#) The number of newlines before and after a property, indexer or event
// declaration.
//
// 0: No change (default).
extern BoundedOption<unsigned, 0, 16>
nl_around_cs_property;

// (C#) The number of newlines between the get/set/add/remove handlers.
//
// 0: No change (default).
extern BoundedOption<unsigned, 0, 16>
nl_between_get_set;

// (C#) Add or remove newline between property and the '{'.
extern Option<iarf_e>
nl_property_brace;

// Whether to remove blank lines after '{'.
extern Option<bool>
eat_blanks_after_open_brace;

// Whether to remove blank lines before '}'.
extern Option<bool>
eat_blanks_before_close_brace;

// How aggressively to remove extra newlines not in preprocessor.
//
// 0: No change (default)
// 1: Remove most newlines not handled by other config
// 2: Remove all newlines and reformat completely by config
extern BoundedOption<unsigned, 0, 2>
nl_remove_extra_newlines;

// (Java) Add or remove newline after an annotation statement. Only affects
// annotations that are after a newline.
extern Option<iarf_e>
nl_after_annotation;

// (Java) Add or remove newline between two annotations.
extern Option<iarf_e>
nl_between_annotation;

// The number of newlines before a whole-file #ifdef.
//
// 0: No change (default).
extern BoundedOption<unsigned, 0, 16>
nl_before_whole_file_ifdef;

// The number of newlines after a whole-file #ifdef.
//
// 0: No change (default).
extern BoundedOption<unsigned, 0, 16>
nl_after_whole_file_ifdef;

// The number of newlines before a whole-file #endif.
//
// 0: No change (default).
extern BoundedOption<unsigned, 0, 16>
nl_before_whole_file_endif;

// The number of newlines after a whole-file #endif.
//
// 0: No change (default).
extern BoundedOption<unsigned, 0, 16>
nl_after_whole_file_endif;

//END

///////////////////////////////////////////////////////////////////////////////
//BEGIN Positioning options

// The position of arithmetic operators in wrapped expressions.
extern Option<token_pos_e>
pos_arith;

// The position of assignment in wrapped expressions. Do not affect '='
// followed by '{'.
extern Option<token_pos_e>
pos_assign;

// The position of Boolean operators in wrapped expressions.
extern Option<token_pos_e>
pos_bool;

// The position of comparison operators in wrapped expressions.
extern Option<token_pos_e>
pos_compare;

// The position of conditional operators, as in the '?' and ':' of
// 'expr ? stmt : stmt', in wrapped expressions.
extern Option<token_pos_e>
pos_conditional;

// The position of the comma in wrapped expressions.
extern Option<token_pos_e>
pos_comma;

// The position of the comma in enum entries.
extern Option<token_pos_e>
pos_enum_comma;

// The position of the comma in the base class list if there is more than one
// line. Affects nl_class_init_args.
extern Option<token_pos_e>
pos_class_comma;

// The position of the comma in the constructor initialization list.
// Related to nl_constr_colon, nl_constr_init_args and pos_constr_colon.
extern Option<token_pos_e>
pos_constr_comma;

// The position of trailing/leading class colon, between class and base class
// list. Affects nl_class_colon.
extern Option<token_pos_e>
pos_class_colon;

// The position of colons between constructor and member initialization.
// Related to nl_constr_colon, nl_constr_init_args and pos_constr_comma.
extern Option<token_pos_e>
pos_constr_colon;

// The position of shift operators in wrapped expressions.
extern Option<token_pos_e>
pos_shift;

//END

///////////////////////////////////////////////////////////////////////////////
//BEGIN Line splitting options

// Try to limit code width to N columns.
extern BoundedOption<unsigned, 0, 10000>
code_width;

// Whether to fully split long 'for' statements at semi-colons.
extern Option<bool>
ls_for_split_full;

// Whether to fully split long function prototypes/calls at commas.
// The option ls_code_width has priority over the option ls_func_split_full.
extern Option<bool>
ls_func_split_full;

// Whether to split lines as close to code_width as possible and ignore some
// groupings.
// The option ls_code_width has priority over the option ls_func_split_full.
extern Option<bool>
ls_code_width;

//END

///////////////////////////////////////////////////////////////////////////////
//BEGIN Code alignment options (not left column spaces/tabs)

// Whether to keep non-indenting tabs.
extern Option<bool>
align_keep_tabs;

// Whether to use tabs for aligning.
extern Option<bool>
align_with_tabs;

// Whether to bump out to the next tab when aligning.
extern Option<bool>
align_on_tabstop;

// Whether to right-align numbers.
extern Option<bool>
align_number_right;

// Whether to keep whitespace not required for alignment.
extern Option<bool>
align_keep_extra_space;

// Whether to align variable definitions in prototypes and functions.
extern Option<bool>
align_func_params;

// The span for aligning parameter definitions in function on parameter name.
//
// 0: Don't align (default).
extern BoundedOption<unsigned, 0, 16>
align_func_params_span;

// The threshold for aligning function parameter definitions.
// Use a negative number for absolute thresholds.
//
// 0: No limit (default).
extern BoundedOption<signed, -1000, 5000>
align_func_params_thresh;

// The gap for aligning function parameter definitions.
extern BoundedOption<unsigned, 0, 16>
align_func_params_gap;

// The span for aligning constructor value.
//
// 0: Don't align (default).
extern BoundedOption<unsigned, 0, 16>
align_constr_value_span;

// The threshold for aligning constructor value.
// Use a negative number for absolute thresholds.
//
// 0: No limit (default).
extern BoundedOption<signed, -1000, 5000>
align_constr_value_thresh;

// The gap for aligning constructor value.
extern BoundedOption<unsigned, 0, 16>
align_constr_value_gap;

// Whether to align parameters in single-line functions that have the same
// name. The function names must already be aligned with each other.
extern Option<bool>
align_same_func_call_params;

// The span for aligning function-call parameters for single line functions.
//
// 0: Don't align (default).
extern BoundedOption<unsigned, 0, 5000>
align_same_func_call_params_span;

// The threshold for aligning function-call parameters for single line
// functions.
// Use a negative number for absolute thresholds.
//
// 0: No limit (default).
extern BoundedOption<signed, -1000, 5000>
align_same_func_call_params_thresh;

// The span for aligning variable definitions.
//
// 0: Don't align (default).
extern BoundedOption<unsigned, 0, 5000>
align_var_def_span;

// How to consider (or treat) the '*' in the alignment of variable definitions.
//
// 0: Part of the type     'void *   foo;' (default)
// 1: Part of the variable 'void     *foo;'
// 2: Dangling             'void    *foo;'
// Dangling: the '*' will not be taken into account when aligning.
extern BoundedOption<unsigned, 0, 2>
align_var_def_star_style;

// How to consider (or treat) the '&' in the alignment of variable definitions.
//
// 0: Part of the type     'long &   foo;' (default)
// 1: Part of the variable 'long     &foo;'
// 2: Dangling             'long    &foo;'
// Dangling: the '&' will not be taken into account when aligning.
extern BoundedOption<unsigned, 0, 2>
align_var_def_amp_style;

// The threshold for aligning variable definitions.
// Use a negative number for absolute thresholds.
//
// 0: No limit (default).
extern BoundedOption<signed, -1000, 5000>
align_var_def_thresh;

// The gap for aligning variable definitions.
extern BoundedOption<unsigned, 0, 16>
align_var_def_gap;

// Whether to align the colon in struct bit fields.
extern Option<bool>
align_var_def_colon;

// The gap for aligning the colon in struct bit fields.
extern BoundedOption<unsigned, 0, 16>
align_var_def_colon_gap;

// Whether to align any attribute after the variable name.
extern Option<bool>
align_var_def_attribute;

// Whether to align inline struct/enum/union variable definitions.
extern Option<bool>
align_var_def_inline;

// The span for aligning on '=' in assignments.
//
// 0: Don't align (default).
extern BoundedOption<unsigned, 0, 5000>
align_assign_span;

// The span for aligning on '=' in function prototype modifier.
//
// 0: Don't align (default).
extern BoundedOption<unsigned, 0, 5000>
align_assign_func_proto_span;

// The threshold for aligning on '=' in assignments.
// Use a negative number for absolute thresholds.
//
// 0: No limit (default).
extern BoundedOption<signed, -1000, 5000>
align_assign_thresh;

// Whether to align on the left most assignment when multiple
// definitions are found on the same line.
// Depends on 'align_assign_span' and 'align_assign_thresh' settings.
extern Option<bool>
align_assign_on_multi_var_defs;

// The span for aligning on '{' in braced init list.
//
// 0: Don't align (default).
extern BoundedOption<unsigned, 0, 5000>
align_braced_init_list_span;

// The threshold for aligning on '{' in braced init list.
// Use a negative number for absolute thresholds.
//
// 0: No limit (default).
extern BoundedOption<signed, -1000, 5000>
align_braced_init_list_thresh;

// How to apply align_assign_span to function declaration "assignments", i.e.
// 'virtual void foo() = 0' or '~foo() = {default|delete}'.
//
// 0: Align with other assignments (default)
// 1: Align with each other, ignoring regular assignments
// 2: Don't align
extern BoundedOption<unsigned, 0, 2>
align_assign_decl_func;

// The span for aligning on '=' in enums.
//
// 0: Don't align (default).
extern BoundedOption<unsigned, 0, 5000>
align_enum_equ_span;

// The threshold for aligning on '=' in enums.
// Use a negative number for absolute thresholds.
//
// 0: no limit (default).
extern BoundedOption<signed, -1000, 5000>
align_enum_equ_thresh;

// The span for aligning class member definitions.
//
// 0: Don't align (default).
extern BoundedOption<unsigned, 0, 5000>
align_var_class_span;

// The threshold for aligning class member definitions.
// Use a negative number for absolute thresholds.
//
// 0: No limit (default).
extern BoundedOption<signed, -1000, 5000>
align_var_class_thresh;

// The gap for aligning class member definitions.
extern BoundedOption<unsigned, 0, 16>
align_var_class_gap;

// The span for aligning struct/union member definitions.
//
// 0: Don't align (default).
extern BoundedOption<unsigned, 0, 5000>
align_var_struct_span;

// The threshold for aligning struct/union member definitions.
// Use a negative number for absolute thresholds.
//
// 0: No limit (default).
extern BoundedOption<signed, -1000, 5000>
align_var_struct_thresh;

// The gap for aligning struct/union member definitions.
extern BoundedOption<unsigned, 0, 16>
align_var_struct_gap;

// The span for aligning struct initializer values.
//
// 0: Don't align (default).
extern BoundedOption<unsigned, 0, 5000>
align_struct_init_span;

// The span for aligning single-line typedefs.
//
// 0: Don't align (default).
extern BoundedOption<unsigned, 0, 16>
align_typedef_span;

// The minimum space between the type and the synonym of a typedef.
extern BoundedOption<unsigned, 0, 16>
align_typedef_gap;

// How to align typedef'd functions with other typedefs.
//
// 0: Don't mix them at all (default)
// 1: Align the open parenthesis with the types
// 2: Align the function type name with the other type names
extern BoundedOption<unsigned, 0, 2>
align_typedef_func;

// How to consider (or treat) the '*' in the alignment of typedefs.
//
// 0: Part of the typedef type, 'typedef int * pint;' (default)
// 1: Part of type name:        'typedef int   *pint;'
// 2: Dangling:                 'typedef int  *pint;'
// Dangling: the '*' will not be taken into account when aligning.
extern BoundedOption<unsigned, 0, 2>
align_typedef_star_style;

// How to consider (or treat) the '&' in the alignment of typedefs.
//
// 0: Part of the typedef type, 'typedef int & intref;' (default)
// 1: Part of type name:        'typedef int   &intref;'
// 2: Dangling:                 'typedef int  &intref;'
// Dangling: the '&' will not be taken into account when aligning.
extern BoundedOption<unsigned, 0, 2>
align_typedef_amp_style;

// The span for aligning comments that end lines.
//
// 0: Don't align (default).
extern BoundedOption<unsigned, 0, 5000>
align_right_cmt_span;

// Minimum number of columns between preceding text and a trailing comment in
// order for the comment to qualify for being aligned. Must be non-zero to have
// an effect.
extern BoundedOption<unsigned, 0, 16>
align_right_cmt_gap;

// If aligning comments, whether to mix with comments after '}' and #endif with
// less than three spaces before the comment.
extern Option<bool>
align_right_cmt_mix;

// Whether to only align trailing comments that are at the same brace level.
extern Option<bool>
align_right_cmt_same_level;

// Minimum column at which to align trailing comments. Comments which are
// aligned beyond this column, but which can be aligned in a lesser column,
// may be "pulled in".
//
// 0: Ignore (default).
extern BoundedOption<unsigned, 0, 200>
align_right_cmt_at_col;

// The span for aligning function prototypes.
//
// 0: Don't align (default).
extern BoundedOption<unsigned, 0, 5000>
align_func_proto_span;

// Whether to ignore continuation lines when evaluating the number of
// new lines for the function prototype alignment's span.
//
// false: continuation lines are part of the newlines count
// true:  continuation lines are not counted
extern Option<bool>
align_func_proto_span_ignore_cont_lines; // = false

// How to consider (or treat) the '*' in the alignment of function prototypes.
//
// 0: Part of the type     'void *   foo();' (default)
// 1: Part of the function 'void     *foo();'
// 2: Dangling             'void    *foo();'
// Dangling: the '*' will not be taken into account when aligning.
extern BoundedOption<unsigned, 0, 2>
align_func_proto_star_style;

// How to consider (or treat) the '&' in the alignment of function prototypes.
//
// 0: Part of the type     'long &   foo();' (default)
// 1: Part of the function 'long     &foo();'
// 2: Dangling             'long    &foo();'
// Dangling: the '&' will not be taken into account when aligning.
extern BoundedOption<unsigned, 0, 2>
align_func_proto_amp_style;

// The threshold for aligning function prototypes.
// Use a negative number for absolute thresholds.
//
// 0: No limit (default).
extern BoundedOption<signed, -1000, 5000>
align_func_proto_thresh;

// Minimum gap between the return type and the function name.
extern BoundedOption<unsigned, 0, 16>
align_func_proto_gap;

// Whether to align function prototypes on the 'operator' keyword instead of
// what follows.
extern Option<bool>
align_on_operator;

// Whether to mix aligning prototype and variable declarations. If true,
// align_var_def_XXX options are used instead of align_func_proto_XXX options.
extern Option<bool>
align_mix_var_proto;

// Whether to align single-line functions with function prototypes.
// Uses align_func_proto_span.
extern Option<bool>
align_single_line_func;

// Whether to align the open brace of single-line functions.
// Requires align_single_line_func=true. Uses align_func_proto_span.
extern Option<bool>
align_single_line_brace;

// Gap for align_single_line_brace.
extern BoundedOption<unsigned, 0, 16>
align_single_line_brace_gap;

// (OC) The span for aligning Objective-C message specifications.
//
// 0: Don't align (default).
extern BoundedOption<unsigned, 0, 5000>
align_oc_msg_spec_span;

// Whether and how to align backslashes that split a macro onto multiple lines.
// This will not work right if the macro contains a multi-line comment.
//
// 0: Do nothing (default)
// 1: Align the backslashes in the column at the end of the longest line
// 2: Align with the backslash that is farthest to the left, or, if that
//    backslash is farther left than the end of the longest line, at the end of
//    the longest line
// 3: Align with the backslash that is farthest to the right
extern BoundedOption<unsigned, 0, 3>
align_nl_cont;

// The minimum number of spaces between the end of a line and its continuation
// backslash. Requires align_nl_cont.
extern BoundedOption<unsigned, 0, 16>
align_nl_cont_spaces; // = 1

// Whether to align macro functions and variables together.
extern Option<bool>
align_pp_define_together;

// The span for aligning on '#define' bodies.
//
// =0: Don't align (default)
// >0: Number of lines (including comments) between blocks
extern BoundedOption<unsigned, 0, 5000>
align_pp_define_span;

// The minimum space between label and value of a preprocessor define.
extern BoundedOption<unsigned, 0, 16>
align_pp_define_gap;

// Whether to align lines that start with '<<' with previous '<<'.
extern Option<bool>
align_left_shift; // = true

// Whether to align comma-separated statements following '<<' (as used to
// initialize Eigen matrices).
extern Option<bool>
align_eigen_comma_init;

// Whether to align text after 'asm volatile ()' colons.
extern Option<bool>
align_asm_colon;

// (OC) Span for aligning parameters in an Objective-C message call
// on the ':'.
//
// 0: Don't align.
extern BoundedOption<unsigned, 0, 5000>
align_oc_msg_colon_span;

// (OC) Whether to always align with the first parameter, even if it is too
// short.
extern Option<bool>
align_oc_msg_colon_first;

// (OC) Whether to align parameters in an Objective-C '+' or '-' declaration
// on the ':'.
extern Option<bool>
align_oc_decl_colon;

// (OC) Whether to not align parameters in an Objectve-C message call if first
// colon is not on next line of the message call (the same way Xcode does
// alignment)
extern Option<bool>
align_oc_msg_colon_xcode_like;

//END

///////////////////////////////////////////////////////////////////////////////
//BEGIN Comment modification options

// Try to wrap comments at N columns.
extern BoundedOption<unsigned, 0, 256>
cmt_width;

// How to reflow comments.
//
// 0: No reflowing (apart from the line wrapping due to cmt_width) (default)
// 1: No touching at all
// 2: Full reflow (enable cmt_indent_multi for indent with line wrapping due to cmt_width)
extern BoundedOption<unsigned, 0, 2>
cmt_reflow_mode;

// Path to a file that contains regular expressions describing patterns for
// which the end of one line and the beginning of the next will be folded into
// the same sentence or paragraph during full comment reflow. The regular
// expressions are described using ECMAScript syntax. The syntax for this
// specification is as follows, where "..." indicates the custom regular
// expression and "n" indicates the nth end_of_prev_line_regex and
// beg_of_next_line_regex regular expression pair:
//
// end_of_prev_line_regex[1] = "...$"
// beg_of_next_line_regex[1] = "^..."
// end_of_prev_line_regex[2] = "...$"
// beg_of_next_line_regex[2] = "^..."
//             .
//             .
//             .
// end_of_prev_line_regex[n] = "...$"
// beg_of_next_line_regex[n] = "^..."
//
// Note that use of this option overrides the default reflow fold regular
// expressions, which are internally defined as follows:
//
// end_of_prev_line_regex[1] = "[\w,\]\)]$"
// beg_of_next_line_regex[1] = "^[\w,\[\(]"
// end_of_prev_line_regex[2] = "\.$"
// beg_of_next_line_regex[2] = "^[A-Z]"
extern Option<string>
cmt_reflow_fold_regex_file;

// Whether to indent wrapped lines to the start of the encompassing paragraph
// during full comment reflow (cmt_reflow_mode = 2). Overrides the value
// specified by cmt_sp_after_star_cont.
//
// Note that cmt_align_doxygen_javadoc_tags overrides this option for
// paragraphs associated with javadoc tags
extern Option<bool>
cmt_reflow_indent_to_paragraph_start;

// Whether to convert all tabs to spaces in comments. If false, tabs in
// comments are left alone, unless used for indenting.
extern Option<bool>
cmt_convert_tab_to_spaces;

// TODO This description is confusing and should be revised.

// Whether to apply changes to multi-line comments, including cmt_width,
// keyword substitution and leading chars.
extern Option<bool>
cmt_indent_multi; // = true

// Whether to align doxygen javadoc-style tags ('@param', '@return', etc.)
// and corresponding fields such that groups of consecutive block tags,
// parameter names, and descriptions align with one another. Overrides that
// which is specified by the cmt_sp_after_star_cont. If cmt_width > 0, it may
// be necessary to enable cmt_indent_multi and set cmt_reflow_mode = 2
// in order to achieve the desired alignment for line-wrapping.
extern Option<bool>
cmt_align_doxygen_javadoc_tags;

// The number of spaces to insert after the star and before doxygen
// javadoc-style tags (@param, @return, etc). Requires enabling
// cmt_align_doxygen_javadoc_tags. Overrides that which is specified by the
// cmt_sp_after_star_cont.
extern BoundedOption<unsigned, 0, 16>
cmt_sp_before_doxygen_javadoc_tags; // = 1

// Whether to change trailing, single-line c-comments into cpp-comments.
extern Option<bool>
cmt_trailing_single_line_c_to_cpp;

// Whether to group c-comments that look like they are in a block.
extern Option<bool>
cmt_c_group;

// Whether to put an empty '/*' on the first line of the combined c-comment.
extern Option<bool>
cmt_c_nl_start;

// Whether to add a newline before the closing '*/' of the combined c-comment.
extern Option<bool>
cmt_c_nl_end;

// Whether to change cpp-comments into c-comments.
extern Option<bool>
cmt_cpp_to_c;

// Whether to group cpp-comments that look like they are in a block. Only
// meaningful if cmt_cpp_to_c=true.
extern Option<bool>
cmt_cpp_group;

// Whether to put an empty '/*' on the first line of the combined cpp-comment
// when converting to a c-comment.
//
// Requires cmt_cpp_to_c=true and cmt_cpp_group=true.
extern Option<bool>
cmt_cpp_nl_start;

// Whether to add a newline before the closing '*/' of the combined cpp-comment
// when converting to a c-comment.
//
// Requires cmt_cpp_to_c=true and cmt_cpp_group=true.
extern Option<bool>
cmt_cpp_nl_end;

// Whether to put a star on subsequent comment lines.
extern Option<bool>
cmt_star_cont;

// The number of spaces to insert at the start of subsequent comment lines.
extern BoundedOption<unsigned, 0, 16>
cmt_sp_before_star_cont;

// The number of spaces to insert after the star on subsequent comment lines.
extern BoundedOption<unsigned, 0, 16>
cmt_sp_after_star_cont;

// TODO This description is confusing and should be revised.

// For multi-line comments with a '*' lead, remove leading spaces if the first
// and last lines of the comment are the same length.
extern Option<bool>
cmt_multi_check_last; // = true

// TODO This description is confusing and should be revised.

// For multi-line comments with a '*' lead, remove leading spaces if the first
// and last lines of the comment are the same length AND if the length is
// bigger as the first_len minimum.
extern BoundedOption<unsigned, 1, 20>
cmt_multi_first_len_minimum; // = 4

// Path to a file that contains text to insert at the beginning of a file if
// the file doesn't start with a C/C++ comment. If the inserted text contains
// '$(filename)', that will be replaced with the current file's name.
extern Option<string>
cmt_insert_file_header;

// Path to a file that contains text to insert at the end of a file if the
// file doesn't end with a C/C++ comment. If the inserted text contains
// '$(filename)', that will be replaced with the current file's name.
extern Option<string>
cmt_insert_file_footer;

// Path to a file that contains text to insert before a function definition if
// the function isn't preceded by a C/C++ comment. If the inserted text
// contains '$(function)', '$(javaparam)' or '$(fclass)', these will be
// replaced with, respectively, the name of the function, the javadoc '@param'
// and '@return' stuff, or the name of the class to which the member function
// belongs.
extern Option<string>
cmt_insert_func_header;

// Path to a file that contains text to insert before a class if the class
// isn't preceded by a C/C++ comment. If the inserted text contains '$(class)',
// that will be replaced with the class name.
extern Option<string>
cmt_insert_class_header;

// Path to a file that contains text to insert before an Objective-C message
// specification, if the method isn't preceded by a C/C++ comment. If the
// inserted text contains '$(message)' or '$(javaparam)', these will be
// replaced with, respectively, the name of the function, or the javadoc
// '@param' and '@return' stuff.
extern Option<string>
cmt_insert_oc_msg_header;

// TODO This description may be confusing; consider revising.

// Whether a comment should be inserted if a preprocessor is encountered when
// stepping backwards from a function name.
//
// Applies to cmt_insert_oc_msg_header, cmt_insert_func_header and
// cmt_insert_class_header.
extern Option<bool>
cmt_insert_before_preproc;

// Whether a comment should be inserted if a function is declared inline to a
// class definition.
//
// Applies to cmt_insert_func_header.
extern Option<bool>
cmt_insert_before_inlines; // = true

// Whether a comment should be inserted if the function is a class constructor
// or destructor.
//
// Applies to cmt_insert_func_header.
extern Option<bool>
cmt_insert_before_ctor_dtor;

//END

///////////////////////////////////////////////////////////////////////////////
//BEGIN Code modifying options (non-whitespace)

// Add or remove braces on a single-line 'do' statement.
extern Option<iarf_e>
mod_full_brace_do;

// Add or remove braces on a single-line 'for' statement.
extern Option<iarf_e>
mod_full_brace_for;

// (Pawn) Add or remove braces on a single-line function definition.
extern Option<iarf_e>
mod_full_brace_function;

// Add or remove braces on a single-line 'if' statement. Braces will not be
// removed if the braced statement contains an 'else'.
extern Option<iarf_e>
mod_full_brace_if;

// Whether to enforce that all blocks of an 'if'/'else if'/'else' chain either
// have, or do not have, braces. Overrides mod_full_brace_if.
//
// 0: Don't override mod_full_brace_if
// 1: Add braces to all blocks if any block needs braces and remove braces if
//    they can be removed from all blocks
// 2: Add braces to all blocks if any block already has braces, regardless of
//    whether it needs them
// 3: Add braces to all blocks if any block needs braces and remove braces if
//    they can be removed from all blocks, except if all blocks have braces
//    despite none needing them
extern BoundedOption<unsigned, 0, 3>
mod_full_brace_if_chain;

// Whether to add braces to all blocks of an 'if'/'else if'/'else' chain.
// If true, mod_full_brace_if_chain will only remove braces from an 'if' that
// does not have an 'else if' or 'else'.
extern Option<bool>
mod_full_brace_if_chain_only;

// Add or remove braces on single-line 'while' statement.
extern Option<iarf_e>
mod_full_brace_while;

// Add or remove braces on single-line 'using ()' statement.
extern Option<iarf_e>
mod_full_brace_using;

// Don't remove braces around statements that span N newlines
extern BoundedOption<unsigned, 0, 5000>
mod_full_brace_nl;

// Whether to prevent removal of braces from 'if'/'for'/'while'/etc. blocks
// which span multiple lines.
//
// Affects:
//   mod_full_brace_for
//   mod_full_brace_if
//   mod_full_brace_if_chain
//   mod_full_brace_if_chain_only
//   mod_full_brace_while
//   mod_full_brace_using
//
// Does not affect:
//   mod_full_brace_do
//   mod_full_brace_function
extern Option<bool>
mod_full_brace_nl_block_rem_mlcond;

// Add or remove unnecessary parentheses on 'return' statement.
extern Option<iarf_e>
mod_paren_on_return;

// Add or remove unnecessary parentheses on 'throw' statement.
extern Option<iarf_e>
mod_paren_on_throw;

// (Pawn) Whether to change optional semicolons to real semicolons.
extern Option<bool>
mod_pawn_semicolon;

// Whether to fully parenthesize Boolean expressions in 'while' and 'if'
// statement, as in 'if (a && b > c)' => 'if (a && (b > c))'.
extern Option<bool>
mod_full_paren_if_bool;

// Whether to fully parenthesize Boolean expressions after '='
// statement, as in 'x = a && b > c;' => 'x = (a && (b > c));'.
extern Option<bool>
mod_full_paren_assign_bool;

// Whether to fully parenthesize Boolean expressions after '='
// statement, as in 'return  a && b > c;' => 'return (a && (b > c));'.
extern Option<bool>
mod_full_paren_return_bool;

// Whether to remove superfluous semicolons.
extern Option<bool>
mod_remove_extra_semicolon;

// Whether to remove duplicate include.
extern Option<bool>
mod_remove_duplicate_include;

// the following options (mod_XX_closebrace_comment) use different comment,
// depending of the setting of the next option.
// false: Use the c comment (default)
// true : Use the cpp comment
extern Option<bool>
mod_add_force_c_closebrace_comment;

// If a function body exceeds the specified number of newlines and doesn't have
// a comment after the close brace, a comment will be added.
extern BoundedOption<unsigned, 0, 255>
mod_add_long_function_closebrace_comment;

// If a namespace body exceeds the specified number of newlines and doesn't
// have a comment after the close brace, a comment will be added.
extern BoundedOption<unsigned, 0, 255>
mod_add_long_namespace_closebrace_comment;

// If a class body exceeds the specified number of newlines and doesn't have a
// comment after the close brace, a comment will be added.
extern BoundedOption<unsigned, 0, 255>
mod_add_long_class_closebrace_comment;

// If a switch body exceeds the specified number of newlines and doesn't have a
// comment after the close brace, a comment will be added.
extern BoundedOption<unsigned, 0, 255>
mod_add_long_switch_closebrace_comment;

// If an #ifdef body exceeds the specified number of newlines and doesn't have
// a comment after the #endif, a comment will be added.
extern BoundedOption<unsigned, 0, 255>
mod_add_long_ifdef_endif_comment;

// If an #ifdef or #else body exceeds the specified number of newlines and
// doesn't have a comment after the #else, a comment will be added.
extern BoundedOption<unsigned, 0, 255>
mod_add_long_ifdef_else_comment;

// Whether to take care of the case by the mod_sort_xx options.
extern Option<bool>
mod_sort_case_sensitive;

// Whether to sort consecutive single-line 'import' statements.
extern Option<bool>
mod_sort_import;

// (C#) Whether to sort consecutive single-line 'using' statements.
extern Option<bool>
mod_sort_using;

// Whether to sort consecutive single-line '#include' statements (C/C++) and
// '#import' statements (Objective-C). Be aware that this has the potential to
// break your code if your includes/imports have ordering dependencies.
extern Option<bool>
mod_sort_include;

// Whether to prioritize '#include' and '#import' statements that contain
// filename without extension when sorting is enabled.
extern Option<bool>
mod_sort_incl_import_prioritize_filename;

// Whether to prioritize '#include' and '#import' statements that does not
// contain extensions when sorting is enabled.
extern Option<bool>
mod_sort_incl_import_prioritize_extensionless;

// Whether to prioritize '#include' and '#import' statements that contain
// angle over quotes when sorting is enabled.
extern Option<bool>
mod_sort_incl_import_prioritize_angle_over_quotes;

// Whether to ignore file extension in '#include' and '#import' statements
// for sorting comparison.
extern Option<bool>
mod_sort_incl_import_ignore_extension;

// Whether to group '#include' and '#import' statements when sorting is enabled.
extern Option<bool>
mod_sort_incl_import_grouping_enabled;

// Whether to move a 'break' that appears after a fully braced 'case' before
// the close brace, as in 'case X: { ... } break;' => 'case X: { ... break; }'.
extern Option<bool>
mod_move_case_break;

// Whether to move a 'return' that appears after a fully braced 'case' before
// the close brace, as in 'case X: { ... } return;' => 'case X: { ... return; }'.
extern Option<bool>
mod_move_case_return;

// Add or remove braces around a fully braced case statement. Will only remove
// braces if there are no variable declarations in the block.
extern Option<iarf_e>
mod_case_brace;

// Whether to remove a void 'return;' that appears as the last statement in a
// function.
extern Option<bool>
mod_remove_empty_return;

// Add or remove the comma after the last value of an enumeration.
extern Option<iarf_e>
mod_enum_last_comma;

// Syntax to use for infinite loops.
//
// 0: Leave syntax alone (default)
// 1: Rewrite as `for(;;)`
// 2: Rewrite as `while(true)`
// 3: Rewrite as `do`...`while(true);`
// 4: Rewrite as `while(1)`
// 5: Rewrite as `do`...`while(1);`
//
// Infinite loops that do not already match one of these syntaxes are ignored.
// Other options that affect loop formatting will be applied after transforming
// the syntax.
extern BoundedOption<unsigned, 0, 5>
mod_infinite_loop;

// Add or remove the 'int' keyword in 'int short'.
extern Option<iarf_e>
mod_int_short;

// Add or remove the 'int' keyword in 'short int'.
extern Option<iarf_e>
mod_short_int;

// Add or remove the 'int' keyword in 'int long'.
extern Option<iarf_e>
mod_int_long;

// Add or remove the 'int' keyword in 'long int'.
extern Option<iarf_e>
mod_long_int;

// Add or remove the 'int' keyword in 'int signed'.
extern Option<iarf_e>
mod_int_signed;

// Add or remove the 'int' keyword in 'signed int'.
extern Option<iarf_e>
mod_signed_int;

// Add or remove the 'int' keyword in 'int unsigned'.
extern Option<iarf_e>
mod_int_unsigned;

// Add or remove the 'int' keyword in 'unsigned int'.
extern Option<iarf_e>
mod_unsigned_int;

// If there is a situation where mod_int_* and mod_*_int would result in
// multiple int keywords, whether to keep the rightmost int (the default) or the
// leftmost int.
extern Option<bool>
mod_int_prefer_int_on_left;

// (OC) Whether to organize the properties. If true, properties will be
// rearranged according to the mod_sort_oc_property_*_weight factors.
extern Option<bool>
mod_sort_oc_properties;

// (OC) Weight of a class property modifier.
extern Option<signed>
mod_sort_oc_property_class_weight;

// (OC) Weight of 'atomic' and 'nonatomic'.
extern Option<signed>
mod_sort_oc_property_thread_safe_weight;

// (OC) Weight of 'readwrite' when organizing properties.
extern Option<signed>
mod_sort_oc_property_readwrite_weight;

// (OC) Weight of a reference type specifier ('retain', 'copy', 'assign',
// 'weak', 'strong') when organizing properties.
extern Option<signed>
mod_sort_oc_property_reference_weight;

// (OC) Weight of getter type ('getter=') when organizing properties.
extern Option<signed>
mod_sort_oc_property_getter_weight;

// (OC) Weight of setter type ('setter=') when organizing properties.
extern Option<signed>
mod_sort_oc_property_setter_weight;

// (OC) Weight of nullability type ('nullable', 'nonnull', 'null_unspecified',
// 'null_resettable') when organizing properties.
extern Option<signed>
mod_sort_oc_property_nullability_weight;

//END

///////////////////////////////////////////////////////////////////////////////
//BEGIN Preprocessor options

// How to use tabs when indenting preprocessor code.
//
// -1: Use 'indent_with_tabs' setting (default)
//  0: Spaces only
//  1: Indent with tabs to brace level, align with spaces
//  2: Indent and align with tabs, using spaces when not on a tabstop
extern BoundedOption<signed, -1, 2>
pp_indent_with_tabs; // = -1

// Add or remove indentation of preprocessor directives inside #if blocks
// at brace level 0 (file-level).
extern Option<iarf_e>
pp_indent;

// Whether to indent #if/#else/#endif at the brace level. If false, these are
// indented from column 1.
extern Option<bool>
pp_indent_at_level;

// Whether to indent #if/#else/#endif at the parenthesis level if the brace
// level is 0. If false, these are indented from column 1.
extern Option<bool>
pp_indent_at_level0;

// Specifies the number of columns to indent preprocessors per level
// at brace level 0 (file-level). If pp_indent_at_level=false, also specifies
// the number of columns to indent preprocessors per level
// at brace level > 0 (function-level).
extern BoundedOption<unsigned, 0, 16>
pp_indent_count; // = 1

// Add or remove space after # based on pp level of #if blocks.
extern Option<iarf_e>
pp_space_after;

// Sets the number of spaces per level added with pp_space_after.
extern BoundedOption<unsigned, 0, 16>
pp_space_count;

// The indent for '#region' and '#endregion' in C# and '#pragma region' in
// C/C++. Negative values decrease indent down to the first column.
extern BoundedOption<signed, -16, 16>
pp_indent_region;

// Whether to indent the code between #region and #endregion.
extern Option<bool>
pp_region_indent_code;

// If pp_indent_at_level=true, sets the indent for #if, #else and #endif when
// not at file-level. Negative values decrease indent down to the first column.
//
// =0: Indent preprocessors using output_tab_size
// >0: Column at which all preprocessors will be indented
extern BoundedOption<signed, -16, 16>
pp_indent_if;

// Whether to indent the code between #if, #else and #endif.
extern Option<bool>
pp_if_indent_code;

// Whether to indent the body of an #if that encompasses all the code in the file.
extern Option<bool>
pp_indent_in_guard;

// Whether to indent '#define' at the brace level. If false, these are
// indented from column 1.
extern Option<bool>
pp_define_at_level;

// Whether to indent '#include' at the brace level.
extern Option<bool>
pp_include_at_level;

// Whether to ignore the '#define' body while formatting.
extern Option<bool>
pp_ignore_define_body;

// An offset value that controls the indentation of the body of a multiline #define.
// 'body' refers to all the lines of a multiline #define except the first line.
// Requires 'pp_ignore_define_body = false'.
//
//  <0: Absolute column: the body indentation starts off at the specified column
//      (ex. -3 ==> the body is indented starting from column 3)
// >=0: Relative to the column of the '#' of '#define'
//      (ex.  3 ==> the body is indented starting 3 columns at the right of '#')
extern BoundedOption<signed, -32, 32>
pp_multiline_define_body_indent; // = 8

// TODO The following descriptions are confusing and suffer from sub-optimal
// grammar, and should be revised; from here...

// Whether to indent case statements between #if, #else, and #endif.
// Only applies to the indent of the preprocessor that the case statements
// directly inside of.
extern Option<bool>
pp_indent_case; // = true

// Whether to indent whole function definitions between #if, #else, and #endif.
// Only applies to the indent of the preprocessor that the function definition
// is directly inside of.
extern Option<bool>
pp_indent_func_def; // = true

// Whether to indent extern C blocks between #if, #else, and #endif.
// Only applies to the indent of the preprocessor that the extern block is
// directly inside of.
extern Option<bool>
pp_indent_extern; // = true

// How to indent braces directly inside #if, #else, and #endif.
// Requires pp_if_indent_code=true and only applies to the indent of the
// preprocessor that the braces are directly inside of.
//  0: No extra indent
//  1: Indent by one level
// -1: Preserve original indentation
extern BoundedOption<signed, -1, 1>
pp_indent_brace; // = 1

// Action to perform when unbalanced #if and #else blocks are found.
// 0: do nothing
// 1: print a warning message
// 2: terminate the program with an error (EX_SOFTWARE)
//
// The action will be triggered in the following cases:
// - if an #ifdef block ends on a different indent level than
//   where it started from. Example:
//
//    #ifdef TEST
//      int i;
//      {
//        int j;
//    #endif
//
// - an #elif/#else block ends on a different indent level than
//   the corresponding #ifdef block. Example:
//
//    #ifdef TEST
//        int i;
//    #else
//        }
//      int j;
//    #endif
extern BoundedOption<unsigned, 0, 2>
pp_unbalanced_if_action; // = 0

//END

///////////////////////////////////////////////////////////////////////////////
//BEGIN Sort includes options

// The regex for include category with priority 0.
extern Option<string>
include_category_0;

// The regex for include category with priority 1.
extern Option<string>
include_category_1;

// The regex for include category with priority 2.
extern Option<string>
include_category_2;

//END

///////////////////////////////////////////////////////////////////////////////
//BEGIN Use or Do not Use options

// true:  indent_func_call_param will be used (default)
// false: indent_func_call_param will NOT be used
extern Option<bool>
use_indent_func_call_param; // = true

// The value of the indentation for a continuation line is calculated
// differently if the statement is:
// - a declaration: your case with QString fileName ...
// - an assignment: your case with pSettings = new QSettings( ...
//
// At the second case the indentation value might be used twice:
// - at the assignment
// - at the function call (if present)
//
// To prevent the double use of the indentation value, use this option with the
// value 'true'.
//
// true:  indent_continue will be used only once
// false: indent_continue will be used every time (default)
//
// Requires indent_ignore_first_continue=false.
extern Option<bool>
use_indent_continue_only_once;

// The indentation can be:
// - after the assignment, at the '[' character
// - at the beginning of the lambda body
//
// true:  indentation will be at the beginning of the lambda body
// false: indentation will be after the assignment (default)
extern Option<bool>
indent_cpp_lambda_only_once;

// Whether sp_after_angle takes precedence over sp_inside_fparen. This was the
// historic behavior, but is probably not the desired behavior, so this is off
// by default.
extern Option<bool>
use_sp_after_angle_always;

// Whether to apply special formatting for Qt SIGNAL/SLOT macros. Essentially,
// this tries to format these so that they match Qt's normalized form (i.e. the
// result of QMetaObject::normalizedSignature), which can slightly improve the
// performance of the QObject::connect call, rather than how they would
// otherwise be formatted.
//
// See options_for_QT.cpp for details.
extern Option<bool>
use_options_overriding_for_qt_macros; // = true

// If true: the form feed character is removed from the list of whitespace
// characters. See https://en.cppreference.com/w/cpp/string/byte/isspace.
extern Option<bool>
use_form_feed_no_more_as_whitespace_character;

//END

///////////////////////////////////////////////////////////////////////////////
//BEGIN Warn levels - 1: error, 2: warning (default), 3: note

// (C#) Warning is given if doing tab-to-\t replacement and we have found one
// in a C# verbatim string literal.
extern BoundedOption<unsigned, 1, 3>
warn_level_tabs_found_in_verbatim_string_literals; // = LWARN

// For debugging purpose only.

// Limit the number of loops.
// Used by uncrustify.cpp to exit from infinite loop.
// 0: no limit.
extern Option<signed>
debug_max_number_of_loops;

// Set the number of the line to protocol;
// Used in the function prot_the_line if the 2. parameter is zero.
// 0: nothing protocol.
extern Option<signed>
debug_line_number_to_protocol;

// Set the number of second(s) before terminating formatting the current file,
// 0: no timeout.
// only for linux
extern Option<signed>
debug_timeout;

// Set the number of characters to be printed if the text is too long,
// 0: do not truncate.
extern BoundedOption<unsigned, 0, 960>
debug_truncate;

// sort (or not) the tracking info.
extern Option<bool>
debug_sort_the_tracks;  // = true

// decode (or not) the flags as a new line.
// only if the -p option is set.
extern Option<bool>
debug_decode_the_flags;

// use (or not) the exit(EX_SOFTWARE) function.
extern Option<bool>
debug_use_the_exit_function_pop; // = true

// print (or not) the version in the file defined at the command option -o.
extern Option<bool>
debug_print_version;

// insert the number of the line at the beginning of each line
extern Option<bool>
set_numbering_for_html_output;

//END

} // namespace options

} // namespace uncrustify

#endif /* OPTIONS_H_INCLUDED */
