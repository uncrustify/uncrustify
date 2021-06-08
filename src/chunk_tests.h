/**
 * @file chunk_tests.h
 *
 * @author
 * @license GPL v2+
 */

#ifndef CHUNK_TESTS_H_INCLUDED
#define CHUNK_TESTS_H_INCLUDED


/**
 * Tests whether or not the input chunk matches the '+=' string
 */
bool chunk_is_add_assign_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '+=' token
 */
bool chunk_is_add_assign_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '+=' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_add_assign_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the first chunk occurs AFTER the second chunk in the argument
 * list
 * @param  pc         points to the first chunk
 * @param  after      points to the second chunk
 * @param  test_equal if true, also tests whether or not the first and second chunks
 *                    in fact refer to the same chunk
 * @return            true or false
 */
bool chunk_is_after(struct chunk_t *pc, struct chunk_t *after, bool test_equal = true);


/**
 * Tests whether or not the input chunk matches the 'alignof' string
 */
bool chunk_is_alignof_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'alignof' token
 */
bool chunk_is_alignof_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '&=' string
 */
bool chunk_is_ampersand_assign_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '&=' token
 */
bool chunk_is_ampersand_assign_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '&=' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_ampersand_assign_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '&' string
 */
bool chunk_is_ampersand_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '&' token
 */
bool chunk_is_ampersand_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '&' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_ampersand_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '>' string
 */
bool chunk_is_angle_close_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '>' token
 */
bool chunk_is_angle_close_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '>' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_angle_close_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '<' string
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_angle_open_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '<' token
 */
bool chunk_is_angle_open_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '<' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_angle_open_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '=' string
 */
bool chunk_is_assign_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '=' token
 */
bool chunk_is_assign_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '=' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_assign_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'auto' string
 */
bool chunk_is_auto_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'auto' token
 */
bool chunk_is_auto_token(struct chunk_t *pc);


/**
 * Tests whether or not the first chunk occurs BEFORE the second chunk in the argument
 * list
 * @param  pc         points to the first chunk
 * @param  before     points to the second chunk
 * @param  test_equal if true, also tests whether or not the first and second chunks
 *                    in fact refer to the same chunk
 * @return            true or false
 */
bool chunk_is_before(struct chunk_t *pc, struct chunk_t *before, bool test_equal = true);


/**
 * Tests whether or not the first chunk occurs both AFTER and BEFORE
 * the second and third chunks in the argument list, respectively
 * @param  pc         points to the first chunk
 * @param  after      points to the second chunk
 * @param  before     points to the third chunk
 * @param  test_equal if true, also tests whether or not the first, second, and third
 *                    chunks in fact refer to the same chunk
 * @return            true or false
 */
bool chunk_is_between(struct chunk_t *pc, struct chunk_t *after, struct chunk_t *before, bool test_equal = true);


/**
 * Tests whether or not the input chunk matches the '|=' string
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_bitwise_or_assign_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '|=' token
 */
bool chunk_is_bitwise_or_assign_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '|=' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_bitwise_or_assign_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '|' string
 */
bool chunk_is_bitwise_or_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '|' token
 */
bool chunk_is_bitwise_or_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '|' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_bitwise_or_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '}' string
 */
bool chunk_is_brace_close_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '}' token
 */
bool chunk_is_brace_close_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '{' string
 */
bool chunk_is_brace_open_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '{' token
 */
bool chunk_is_brace_open_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '^=' string
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_caret_assign_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '^=' token
 */
bool chunk_is_caret_assign_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '^=' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_caret_assign_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '^' string
 */
bool chunk_is_caret_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '^' token
 */
bool chunk_is_caret_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '^' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_caret_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'catch' string
 */
bool chunk_is_catch_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'catch' token
 */
bool chunk_is_catch_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk is a character literal
 */
bool chunk_is_char_literal(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the ':' string
 */
bool chunk_is_colon_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the ':' token
 */
bool chunk_is_colon_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the ',' string
 */
bool chunk_is_comma_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the ',' token
 */
bool chunk_is_comma_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the ',' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_comma_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '==' string
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_compare_equal_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '==' token
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_compare_equal_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '==' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_compare_equal_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '>=' string
 */
bool chunk_is_compare_greater_equal_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '>=' token
 */
bool chunk_is_compare_greater_equal_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '>=' token overload
 */
bool chunk_is_compare_greater_equal_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '!=' string
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_compare_inequal_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '!=' token
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_compare_inequal_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '!=' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_compare_inequal_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '<=' string
 */
bool chunk_is_compare_less_equal_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '<=' token
 */
bool chunk_is_compare_less_equal_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '<=' token overload
 */
bool chunk_is_compare_less_equal_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '<=>' string
 */
bool chunk_is_compare_three_way_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '<=>' token
 */
bool chunk_is_compare_three_way_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '<=>' token overload
 */
bool chunk_is_compare_three_way_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches one of the following strings:
 * '<=>', '<=', '==', '>=', '!=', '<', '>'
 */
bool chunk_is_comparison_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches one of the following tokens:
 * '<=>', '<=', '==', '>=', '!=', '<', '>'
 */
bool chunk_is_comparison_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches one of the following token overloads:
 * '<=>', '<=', '==', '>=', '!=', '<', '>'
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_comparison_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'const' string
 */
bool chunk_is_const_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'const' token
 */
bool chunk_is_const_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'constexpr' string
 */
bool chunk_is_constexpr_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'constexpr' token
 */
bool chunk_is_constexpr_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'const_cast' string
 */
bool chunk_is_const_cast_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'const_cast' token
 */
bool chunk_is_const_cast_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches one of the following strings:
 * 'const_cast', 'dynamic_cast', 'reinterpret_cast', 'static_cast'
 */
bool chunk_is_cpp_type_cast_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches one of the following tokens:
 * 'const_cast', 'dynamic_cast', 'reinterpret_cast', 'static_cast'
 */
bool chunk_is_cpp_type_cast_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches one of the following strings:
 * 'const', 'volatile'
 */
bool chunk_is_cv_qualifier_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches one of the following tokens:
 * 'const', 'volatile'
 */
bool chunk_is_cv_qualifier_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'decltype' string
 */
bool chunk_is_decltype_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'decltype' token
 */
bool chunk_is_decltype_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '--' string
 */
bool chunk_is_decrement_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '--' token
 */
bool chunk_is_decrement_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '--' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_decrement_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'delete' string
 */
bool chunk_is_delete_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'delete' token
 */
bool chunk_is_delete_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'delete' or 'delete []' token overloads
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_delete_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '/=' string
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_divide_assign_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '/=' token
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_divide_assign_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '/=' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_divide_assign_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '/' string
 */
bool chunk_is_divide_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '/' token
 */
bool chunk_is_divide_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '/' token overload
 */
bool chunk_is_divide_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '&&' string
 */
bool chunk_is_double_ampersand_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '&&' token
 */
bool chunk_is_double_ampersand_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '&&' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_double_ampersand_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '::' string
 */
bool chunk_is_double_colon_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '::' token
 */
bool chunk_is_double_colon_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'dynamic_cast' string
 */
bool chunk_is_dynamic_cast_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'dynamic_cast' token
 */
bool chunk_is_dynamic_cast_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '...' string
 */
bool chunk_is_ellipsis_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '...' token
 */
bool chunk_is_ellipsis_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '()' string
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_empty_parens_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '[]' string
 */
bool chunk_is_empty_square_brackets_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '=' string
 */
bool chunk_is_equals_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '=' token
 */
bool chunk_is_equals_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '=' token overload
 */
bool chunk_is_equals_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk is a floating point number token
 */
bool chunk_is_floating_point_number_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '()' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_function_call_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the chunk is an identifier
 * @param  pc      the input chunk
 * @param  skip_dc if true, skips any scope resolution operators and qualifiers that
 *                 may precede the identifier
 * @return         true or false
 */
bool chunk_is_identifier(struct chunk_t *pc, bool skip_dc = true);


/**
 * Tests whether or not the input chunk matches the '++' string
 */
bool chunk_is_increment_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '++' token
 */
bool chunk_is_increment_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '++' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_increment_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk is an integral number token
 */
bool chunk_is_integral_number_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk is an intrinsic type
 */
bool chunk_is_intrinsic_type(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk is a built-in keyword
 */
bool chunk_is_keyword(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '||' string
 */
bool chunk_is_logical_or_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '||' token
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_logical_or_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '||' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_logical_or_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '<<=' string
 */
bool chunk_is_lshift_assign_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '<<=' token
 */
bool chunk_is_lshift_assign_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '<<=' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_lshift_assign_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '<<' string
 */
bool chunk_is_lshift_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '<<' token
 */
bool chunk_is_lshift_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '<<' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_lshift_token_overload(struct chunk_t *pc);


/**
 * Returns true if the chunk under test is a reference to a macro defined elsewhere in
 * the source file currently being processed. Note that a macro may be defined in
 * another source or header file, for which this function does not currently account
 */
bool chunk_is_macro_reference(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches one of the following strings:
 * '.', '.*', '->', '->*'
 */
bool chunk_is_member_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches one of the following tokens:
 * '.', '.*', '->', '->*'
 */
bool chunk_is_member_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches one of the following token overloads:
 * '->', '->*'
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_member_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '-' string
 */
bool chunk_is_minus_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '-' token
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_minus_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '-' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_minus_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '%=' string
 */
bool chunk_is_modulo_assign_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '%=' token
 */
bool chunk_is_modulo_assign_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '%=' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_modulo_assign_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '%' string
 */
bool chunk_is_modulo_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '%' token
 */
bool chunk_is_modulo_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '%' token overload
 */
bool chunk_is_modulo_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'mutable' string
 */
bool chunk_is_mutable_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'mutable' token
 */
bool chunk_is_mutable_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'new' string
 */
bool chunk_is_new_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'new' token
 */
bool chunk_is_new_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'new' or 'new []' token overloads
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_new_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'noexcept' string
 */
bool chunk_is_noexcept_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'noexcept' token
 */
bool chunk_is_noexcept_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk is a number
 */
bool chunk_is_number_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'operator' string
 */
bool chunk_is_operator_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'operator' token
 */
bool chunk_is_operator_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk is an overloaded token
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_overloaded_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'override' string
 */
bool chunk_is_override_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'override' token
 */
bool chunk_is_override_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the ')' string
 */
bool chunk_is_paren_close_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the ')' token
 */
bool chunk_is_paren_close_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '(' string
 */
bool chunk_is_paren_open_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '(' token
 */
bool chunk_is_paren_open_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '+' string
 */
bool chunk_is_plus_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '+' token
 */
bool chunk_is_plus_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '+' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_plus_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk is a pointer/reference operator or a
 * qualifier
 */
bool chunk_is_pointer_reference_or_cv_qualifier(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '?' string
 */
bool chunk_is_question_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '?' token
 */
bool chunk_is_question_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'reinterpret_cast' string
 */
bool chunk_is_reinterpret_cast_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'reinterpret_cast' token
 */
bool chunk_is_reinterpret_cast_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the ';' string
 */
bool chunk_is_semicolon_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the ';' token
 */
bool chunk_is_semicolon_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '>>=' string
 */
bool chunk_is_rshift_assign_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '>>=' token
 */
bool chunk_is_rshift_assign_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '>>=' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_rshift_assign_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '>>' string
 */
bool chunk_is_rshift_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '>>' token
 */
bool chunk_is_rshift_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '>>' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_rshift_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches one of the following strings:
 * '<<', '>>'
 */
bool chunk_is_shift_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches one of the following tokens:
 * '<<', '>>'
 */
bool chunk_is_shift_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches one of the following token overloads:
 * '<<', '>>'
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_shift_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches one of the following strings:
 * '<<=', '>>='
 */
bool chunk_is_shift_assign_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches one of the following tokens:
 * '<<=', '>>='
 */
bool chunk_is_shift_assign_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches one of the following token overloads:
 * '<<=', '>>='
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_shift_assign_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches one of the following string:
 * '<<', '>>'
 */
bool chunk_is_shift_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches one of the following tokens:
 * '<<', '>>'
 */
bool chunk_is_shift_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches one of the following token overloads:
 * '<<', '>>'
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_shift_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'sizeof' string
 */
bool chunk_is_sizeof_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'sizeof' token
 */
bool chunk_is_sizeof_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the ']' string
 */
bool chunk_is_square_close_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the ']' token
 */
bool chunk_is_square_close_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '[' string
 */
bool chunk_is_square_open_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '[' token
 */
bool chunk_is_square_open_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '*=' string
 */
bool chunk_is_star_assign_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '*=' token
 */
bool chunk_is_star_assign_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '*=' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_star_assign_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '*' string
 */
bool chunk_is_star_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '*' token
 */
bool chunk_is_star_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '*' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_star_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'static_cast' string
 */
bool chunk_is_static_cast_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'static_cast' token
 */
bool chunk_is_static_cast_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'static' string
 */
bool chunk_is_static_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'static' token
 */
bool chunk_is_static_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches a string token
 */
bool chunk_is_string_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '[]' token
 */
bool chunk_is_subscript_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '[]' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_subscript_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '-=' string
 */
bool chunk_is_subtract_assign_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '-=' token
 */
bool chunk_is_subtract_assign_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '-=' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_subtract_assign_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'template' string
 */
bool chunk_is_template_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'template' token
 */
bool chunk_is_template_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'throw' string
 */
bool chunk_is_throw_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'throw' token
 */
bool chunk_is_throw_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '~' string
 */
bool chunk_is_tilde_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '~' token
 */
bool chunk_is_tilde_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '~' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_tilde_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'typeid' string
 */
bool chunk_is_typeid_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'typeid' token
 */
bool chunk_is_typeid_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'typename' string
 */
bool chunk_is_typename_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'typename' token
 */
bool chunk_is_typename_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '!' string
 */
bool chunk_is_unary_not_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '!' token
 */
bool chunk_is_unary_not_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the '!' token overload
 * @param  pc   the input chunk
 * @return      true or false
 */
bool chunk_is_unary_not_token_overload(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'using' string
 */
bool chunk_is_using_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'using' token
 */
bool chunk_is_using_token(struct chunk_t *pc);


/**
 * Test whether or not the input chunk matches the 'virtual' string
 */
bool chunk_is_virtual_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'virtual' token
 */
bool chunk_is_virtual_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'volatile' string
 */
bool chunk_is_volatile_str(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk matches the 'volatile' token
 */
bool chunk_is_volatile_token(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk is within a member initializer list
 */
bool chunk_is_within_constructor_initializer_list(struct chunk_t *pc);


/**
 * Tests whether or not the input chunk is within a function definition
 */
bool chunk_is_within_function_definition_body(struct chunk_t *pc);


#endif /* CHUNK_TESTS_H_INCLUDED */
