/**
 * @file chunk_tests.cpp
 *
 * @author
 * @license GPL v2+
 */

#include "chunk_tests.h"

#include "chunk_list.h"
#include "chunk_tools.h"
#include "keywords.h"
#include "match_tools.h"


bool chunk_is_add_assign_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "+=", 2));
} // chunk_is_add_assign_str


bool chunk_is_add_assign_token(chunk_t *pc)
{
   return(  chunk_is_add_assign_str(pc)
         && chunk_is_assign_token(pc));
} // chunk_is_add_assign_token


bool chunk_is_add_assign_token_overload(chunk_t *pc)
{
   return(  chunk_is_add_assign_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_add_assign_token_overload


bool chunk_is_after(chunk_t *pc, chunk_t *after, bool test_equal)
{
   LOG_FUNC_ENTRY();

   if (pc != nullptr)
   {
      if (  test_equal
         && pc == after)
      {
         return(true);
      }
      else if (after != nullptr)
      {
         auto pc_column    = pc->orig_col;
         auto pc_line      = pc->orig_line;
         auto after_column = after->orig_col;
         auto after_line   = after->orig_line;

         return(  pc_line > after_line
               || (  pc_line == after_line
                  && pc_column > after_column));
      }
   }
   return(false);
} // chunk_is_after


bool chunk_is_alignof_str(chunk_t *pc)
{
   return(  chunk_is_str(pc, "alignof", 7)
         || chunk_is_str(pc, "_Alignof", 8));
} // chunk_is_alignof_str


bool chunk_is_alignof_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_ALIGN));
} // chunk_is_alignof_token


bool chunk_is_ampersand_assign_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "&=", 2));
} // chunk_is_ampersand_assign_str


bool chunk_is_ampersand_assign_token(chunk_t *pc)
{
   return(  chunk_is_ampersand_assign_str(pc)
         && chunk_is_assign_token(pc));
} // chunk_is_ampersand_assign_token


bool chunk_is_ampersand_assign_token_overload(chunk_t *pc)
{
   return(  chunk_is_ampersand_assign_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_ampersand_assign_token_overload


bool chunk_is_ampersand_str(chunk_t *pc)
{
   return(  pc != nullptr
         && (pc->len() == 1)
         && (pc->str[0] == '&'));
} // chunk_is_ampersand_str


bool chunk_is_ampersand_token(chunk_t *pc)
{
   return(  chunk_is_token(pc, CT_ADDR)
         || chunk_is_token(pc, CT_AMP)
         || chunk_is_token(pc, CT_BYREF));
} // chunk_is_ampersand_token


bool chunk_is_ampersand_token_overload(chunk_t *pc)
{
   return(  chunk_is_ampersand_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_ampersand_token_overload


bool chunk_is_angle_close_str(chunk_t *pc)
{
   return(  pc != nullptr
         && (pc->len() == 1)
         && (pc->str[0] == '>'));
} // chunk_is_angle_close_str


bool chunk_is_angle_close_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_ANGLE_CLOSE));
} // chunk_is_angle_close_token


bool chunk_is_angle_close_token_overload(chunk_t *pc)
{
   return(  chunk_is_angle_close_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_angle_close_token_overload


bool chunk_is_angle_open_str(chunk_t *pc)
{
   return(  pc != nullptr
         && (pc->len() == 1)
         && (pc->str[0] == '<'));
} // chunk_is_angle_open_str


bool chunk_is_angle_open_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_ANGLE_OPEN));
} // chunk_is_angle_open_token


bool chunk_is_angle_open_token_overload(chunk_t *pc)
{
   return(  chunk_is_angle_open_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_angle_open_token_overload


bool chunk_is_assign_str(chunk_t *pc)
{
   return(  pc != nullptr
         && (pc->len() == 1)
         && (pc->str[0] == '='));
} // chunk_is_assign_str


bool chunk_is_assign_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_ASSIGN));
} // chunk_is_assign_token


bool chunk_is_assign_token_overload(chunk_t *pc)
{
   return(  chunk_is_assign_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_assign_token_overload


bool chunk_is_auto_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "auto", 4));
} // chunk_is_auto_str


bool chunk_is_auto_token(chunk_t *pc)
{
   return(  chunk_is_auto_str(pc)
         && chunk_is_token(pc, CT_TYPE));
} // chunk_is_auto_token


bool chunk_is_before(chunk_t *pc, chunk_t *before, bool test_equal)
{
   LOG_FUNC_ENTRY();

   if (pc != nullptr)
   {
      if (  test_equal
         && pc == before)
      {
         return(true);
      }
      else if (before != nullptr)
      {
         auto pc_column     = pc->orig_col;
         auto pc_line       = pc->orig_line;
         auto before_column = before->orig_col;
         auto before_line   = before->orig_line;

         return(  pc_line < before_line
               || (  pc_line == before_line
                  && pc_column < before_column));
      }
   }
   return(false);
} // chunk_is_before


bool chunk_is_between(chunk_t *pc, chunk_t *after, chunk_t *before, bool test_equal)
{
   return(  chunk_is_before(pc, before, test_equal)
         && chunk_is_after(pc, after, test_equal));
} // chunk_is_between


bool chunk_is_bitwise_or_assign_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "|=", 2));
} // chunk_is_bitwise_or_assign_str


bool chunk_is_bitwise_or_assign_token(chunk_t *pc)
{
   return(  chunk_is_bitwise_or_assign_str(pc)
         && chunk_is_assign_token(pc));
} // chunk_is_bitwise_or_assign_token


bool chunk_is_bitwise_or_assign_token_overload(chunk_t *pc)
{
   return(  chunk_is_bitwise_or_assign_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_bitwise_or_assign_token_overload


bool chunk_is_bitwise_or_str(chunk_t *pc)
{
   return(  pc != nullptr
         && (pc->len() == 1)
         && (pc->str[0] == '|'));
} // chunk_is_bitwise_or_str


bool chunk_is_bitwise_or_token(chunk_t *pc)
{
   return(  chunk_is_bitwise_or_str(pc)
         && chunk_is_token(pc, CT_ARITH));
} // chunk_is_bitwise_or_token


bool chunk_is_bitwise_or_token_overload(chunk_t *pc)
{
   return(  chunk_is_bitwise_or_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_bitwise_or_token_overload


bool chunk_is_brace_close_str(chunk_t *pc)
{
   return(  pc != nullptr
         && (pc->len() == 1)
         && (pc->str[0] == '}'));
} // chunk_is_brace_close_str


bool chunk_is_brace_close_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_BRACE_CLOSE));
} // chunk_is_brace_close_token


bool chunk_is_brace_open_str(chunk_t *pc)
{
   return(  pc != nullptr
         && (pc->len() == 1)
         && (pc->str[0] == '{'));
} // chunk_is_brace_open_str


bool chunk_is_brace_open_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_BRACE_OPEN));
} // chunk_is_brace_open_token


bool chunk_is_caret_assign_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "^=", 2));
} // chunk_is_caret_assign_str


bool chunk_is_caret_assign_token(chunk_t *pc)
{
   return(  chunk_is_caret_assign_str(pc)
         && chunk_is_assign_token(pc));
} // chunk_is_caret_assign_token


bool chunk_is_caret_assign_token_overload(chunk_t *pc)
{
   return(  chunk_is_caret_assign_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_caret_assign_token_overload


bool chunk_is_caret_str(chunk_t *pc)
{
   return(  pc != nullptr
         && (pc->len() == 1)
         && (pc->str[0] == '^'));
} // chunk_is_caret_str


bool chunk_is_caret_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_CARET));
} // chunk_is_caret_token


bool chunk_is_caret_token_overload(chunk_t *pc)
{
   return(  chunk_is_caret_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_caret_token_overload


bool chunk_is_catch_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "catch", 5));
} // chunk_is_catch_str


bool chunk_is_catch_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_CATCH));
} // chunk_is_catch_token


bool chunk_is_char_literal(chunk_t *pc)
{
   // TODO: Need to revisit this

   return(  chunk_is_token(pc, CT_STRING)
         && pc->len() == 1);
} // chunk_is_char_literal


bool chunk_is_colon_str(chunk_t *pc)
{
   return(  pc != nullptr
         && (pc->len() == 1)
         && (pc->str[0] == ':'));
} // chunk_is_colon_str


bool chunk_is_colon_token(chunk_t *pc)
{
   return(  chunk_is_token(pc, CT_ACCESS_COLON)
         || chunk_is_token(pc, CT_ASM_COLON)
         || chunk_is_token(pc, CT_BIT_COLON)
         || chunk_is_token(pc, CT_CASE_COLON)
         || chunk_is_token(pc, CT_CLASS_COLON)
         || chunk_is_token(pc, CT_COLON)
         || chunk_is_token(pc, CT_COND_COLON)
         || chunk_is_token(pc, CT_CONSTR_COLON)
         || chunk_is_token(pc, CT_CS_SQ_COLON)
         || chunk_is_token(pc, CT_D_ARRAY_COLON)
         || chunk_is_token(pc, CT_FOR_COLON)
         || chunk_is_token(pc, CT_LABEL_COLON)
         || chunk_is_token(pc, CT_OC_COLON)
         || chunk_is_token(pc, CT_OC_DICT_COLON)
         || chunk_is_token(pc, CT_TAG_COLON)
         || chunk_is_token(pc, CT_WHERE_COLON));
} // chunk_is_colon_token


bool chunk_is_comma_str(chunk_t *pc)
{
   return(  pc != nullptr
         && (pc->len() == 1)
         && (pc->str[0] == ','));
} // chunk_is_comma_str


bool chunk_is_comma_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_COMMA));
} // chunk_is_comma_token


bool chunk_is_comma_token_overload(chunk_t *pc)
{
   return(  chunk_is_comma_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_comma_token_overload


bool chunk_is_compare_equal_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "==", 2));
} // chunk_is_compare_equal_str


bool chunk_is_compare_equal_token(chunk_t *pc)
{
   return(  chunk_is_compare_equal_str(pc)
         && chunk_is_token(pc, CT_COMPARE));
} // chunk_is_compare_equal_token


bool chunk_is_compare_equal_token_overload(chunk_t *pc)
{
   return(  chunk_is_compare_equal_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_compare_equal_token_overload


bool chunk_is_compare_greater_equal_str(chunk_t *pc)
{
   return(chunk_is_str(pc, ">=", 2));
} // chunk_is_compare_greater_equal_str


bool chunk_is_compare_greater_equal_token(chunk_t *pc)
{
   return(  chunk_is_compare_greater_equal_str(pc)
         && chunk_is_token(pc, CT_COMPARE));
} // chunk_is_compare_greater_equal_token


bool chunk_is_compare_greater_equal_token_overload(chunk_t *pc)
{
   return(  chunk_is_compare_greater_equal_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_compare_greater_equal_token_overload


bool chunk_is_compare_inequal_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "!=", 2));
} // chunk_is_compare_inequal_str


bool chunk_is_compare_inequal_token(chunk_t *pc)
{
   return(  chunk_is_compare_inequal_str(pc)
         && chunk_is_token(pc, CT_COMPARE));
} // chunk_is_compare_inequal_token


bool chunk_is_compare_inequal_token_overload(chunk_t *pc)
{
   return(  chunk_is_compare_inequal_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_compare_inequal_token_overload


bool chunk_is_compare_less_equal_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "<=", 2));
} // chunk_is_compare_less_equal_str


bool chunk_is_compare_less_equal_token(chunk_t *pc)
{
   return(  chunk_is_compare_less_equal_str(pc)
         && chunk_is_token(pc, CT_COMPARE));
} // chunk_is_compare_less_equal_token


bool chunk_is_compare_less_equal_token_overload(chunk_t *pc)
{
   return(  chunk_is_compare_less_equal_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_compare_less_equal_token_overload


bool chunk_is_compare_three_way_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "<=>", 3));
} // chunk_is_compare_three_way_str


bool chunk_is_compare_three_way_token(chunk_t *pc)
{
   return(  chunk_is_compare_three_way_str(pc)
         && chunk_is_token(pc, CT_COMPARE));
} // chunk_is_compare_three_way_token


bool chunk_is_compare_three_way_token_overload(chunk_t *pc)
{
   return(  chunk_is_compare_three_way_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_compare_three_way_token_overload


bool chunk_is_comparison_str(chunk_t *pc)
{
   return(  chunk_is_angle_close_str(pc)
         || chunk_is_angle_open_str(pc)
         || chunk_is_compare_equal_str(pc)
         || chunk_is_compare_greater_equal_str(pc)
         || chunk_is_compare_inequal_str(pc)
         || chunk_is_compare_less_equal_str(pc)
         || chunk_is_compare_three_way_str(pc));
} // chunk_is_comparison_str


bool chunk_is_comparison_token(chunk_t *pc)
{
   return(  chunk_is_comparison_str(pc)
         && chunk_is_token(pc, CT_COMPARE));
} // chunk_is_comparison_token


bool chunk_is_comparison_token_overload(chunk_t *pc)
{
   return(  chunk_is_comparison_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_comparison_token_overload


bool chunk_is_const_cast_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "const_cast", 10));
} // chunk_is_const_cast_str


bool chunk_is_const_cast_token(chunk_t *pc)
{
   return(  chunk_is_const_cast_str(pc)
         && chunk_is_token(pc, CT_TYPE_CAST));
} // chunk_is_const_cast_token


bool chunk_is_const_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "const", 5));
} // chunk_is_const_str


bool chunk_is_const_token(chunk_t *pc)
{
   return(  chunk_is_const_str(pc)
         && chunk_is_token(pc, CT_QUALIFIER));
} // chunk_is_const_token


bool chunk_is_constexpr_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "constexpr", 9));
} // chunk_is_constexpr_str


bool chunk_is_constexpr_token(chunk_t *pc)
{
   return(  chunk_is_constexpr_str(pc)
         && chunk_is_token(pc, CT_QUALIFIER));
} // chunk_is_constexpr_token


bool chunk_is_cpp_type_cast_str(chunk_t *pc)
{
   return(  chunk_is_const_cast_str(pc)
         || chunk_is_dynamic_cast_str(pc)
         || chunk_is_reinterpret_cast_str(pc)
         || chunk_is_static_cast_str(pc));
} // chunk_is_cpp_type_cast_str


bool chunk_is_cpp_type_cast_token(chunk_t *pc)
{
   return(  chunk_is_const_cast_token(pc)
         || chunk_is_dynamic_cast_token(pc)
         || chunk_is_reinterpret_cast_token(pc)
         || chunk_is_static_cast_token(pc));
} // chunk_is_cpp_type_cast_token


bool chunk_is_cv_qualifier_str(chunk_t *pc)
{
   return(  chunk_is_const_str(pc)
         || chunk_is_volatile_str(pc));
} // chunk_is_cv_qualifier_str


bool chunk_is_cv_qualifier_token(chunk_t *pc)
{
   return(  chunk_is_const_token(pc)
         || chunk_is_volatile_token(pc));
} // chunk_is_cv_qualifier_token


bool chunk_is_decltype_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "decltype", 8));
} // chunk_is_decltype_str


bool chunk_is_decltype_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_DECLTYPE));
} // chunk_is_decltype_token


bool chunk_is_decrement_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "--", 2));
} // chunk_is_decrement_str


bool chunk_is_decrement_token(chunk_t *pc)
{
   return(  chunk_is_decrement_str(pc)
         && (  chunk_is_token(pc, CT_INCDEC_AFTER)
            || chunk_is_token(pc, CT_INCDEC_BEFORE)));
} // chunk_is_decrement_token


bool chunk_is_decrement_token_overload(chunk_t *pc)
{
   return(  chunk_is_decrement_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_decrement_token_overload


bool chunk_is_delete_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "delete", 6));
} // chunk_is_delete_str


bool chunk_is_delete_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_DELETE));
} // chunk_is_delete_token


bool chunk_is_delete_token_overload(chunk_t *pc)
{
   return(  chunk_is_delete_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_delete_token_overload


bool chunk_is_divide_assign_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "/=", 2));
} // chunk_is_divide_assign_str


bool chunk_is_divide_assign_token(chunk_t *pc)
{
   return(  chunk_is_divide_assign_str(pc)
         && chunk_is_assign_token(pc));
} // chunk_is_divide_assign_token


bool chunk_is_divide_assign_token_overload(chunk_t *pc)
{
   return(  chunk_is_divide_assign_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_divide_assign_token_overload


bool chunk_is_divide_str(chunk_t *pc)
{
   return(  pc != nullptr
         && (pc->len() == 1)
         && (pc->str[0] == '/'));
} // chunk_is_divide_str


bool chunk_is_divide_token(chunk_t *pc)
{
   return(  chunk_is_divide_str(pc)
         && chunk_is_token(pc, CT_ARITH));
} // chunk_is_divide_token


bool chunk_is_divide_token_overload(chunk_t *pc)
{
   return(  chunk_is_divide_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_divide_token_overload


bool chunk_is_double_ampersand_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "&&", 2));
} // chunk_is_double_ampersand_str


bool chunk_is_double_ampersand_token(chunk_t *pc)
{
   return(  chunk_is_double_ampersand_str(pc)
         && (  chunk_is_token(pc, CT_BOOL)
            || chunk_is_token(pc, CT_BYREF)));
} // chunk_is_double_ampersand_token


bool chunk_is_double_ampersand_token_overload(chunk_t *pc)
{
   return(  chunk_is_double_ampersand_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_double_ampersand_token_overload


bool chunk_is_double_colon_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "::", 2));
} // chunk_is_double_colon_str


bool chunk_is_double_colon_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_DC_MEMBER));
} // chunk_is_double_colon_token


bool chunk_is_dynamic_cast_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "dynamic_cast", 12));
} // chunk_is_dynamic_cast_str


bool chunk_is_dynamic_cast_token(chunk_t *pc)
{
   return(  chunk_is_dynamic_cast_str(pc)
         && chunk_is_token(pc, CT_TYPE_CAST));
} // chunk_is_dynamic_cast_token


bool chunk_is_ellipsis_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "...", 3));
} // chunk_is_ellipsis_str


bool chunk_is_ellipsis_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_ELLIPSIS));
} // chunk_is_ellipsis_token


bool chunk_is_empty_parens_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "()", 2));
} // chunk_is_empty_parens_str


bool chunk_is_empty_square_brackets_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "[]", 2));
} // chunk_is_empty_square_brackets_str


bool chunk_is_equals_str(chunk_t *pc)
{
   return(  pc != nullptr
         && (pc->len() == 1)
         && (pc->str[0] == '='));
} // chunk_is_equals_str


bool chunk_is_equals_token(chunk_t *pc)
{
   return(  chunk_is_equals_str(pc)
         && chunk_is_assign_token(pc));
} // chunk_is_equals_token


bool chunk_is_equals_token_overload(chunk_t *pc)
{
   return(  chunk_is_equals_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_equals_token_overload


bool chunk_is_floating_point_number_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_NUMBER_FP));
} // chunk_is_floating_point_number_token


bool chunk_is_function_call_token_overload(chunk_t *pc)
{
   return(  chunk_is_empty_parens_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_function_call_token_overload


bool chunk_is_identifier(chunk_t *pc, bool skip_dc)
{
   LOG_FUNC_ENTRY();

   if (skip_dc)
   {
      pc = chunk_skip_dc_member(pc, scope_e::PREPROC);
   }
   return(  chunk_is_token(pc, CT_FUNC_CALL)
         || chunk_is_token(pc, CT_FUNC_CALL_USER)
         || chunk_is_token(pc, CT_FUNC_CLASS_DEF)
         || chunk_is_token(pc, CT_FUNC_CLASS_PROTO)
         || chunk_is_token(pc, CT_FUNC_CTOR_VAR)
         || chunk_is_token(pc, CT_FUNC_DEF)
         || chunk_is_token(pc, CT_FUNC_PROTO)
         || chunk_is_token(pc, CT_FUNC_TYPE)
         || chunk_is_token(pc, CT_FUNC_VAR)
         || chunk_is_token(pc, CT_FUNCTION)
         || chunk_is_token(pc, CT_FUNC_CALL_USER)
         || chunk_is_token(pc, CT_FUNCTION)
         || (  chunk_is_token(pc, CT_TYPE)
            && !chunk_is_keyword(pc))
         || chunk_is_token(pc, CT_WORD));
} // chunk_is_identifier


bool chunk_is_increment_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "++", 2));
} // chunk_is_increment_str


bool chunk_is_increment_token(chunk_t *pc)
{
   return(  chunk_is_increment_str(pc)
         && (  chunk_is_token(pc, CT_INCDEC_AFTER)
            || chunk_is_token(pc, CT_INCDEC_BEFORE)));
} // chunk_is_increment_token


bool chunk_is_increment_token_overload(chunk_t *pc)
{
   return(  chunk_is_increment_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_increment_token_overload


bool chunk_is_integral_number_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_NUMBER));
} // chunk_is_integral_number_token


bool chunk_is_intrinsic_type(chunk_t *pc)
{
   return(  chunk_is_token(pc, CT_TYPE)
         && chunk_is_keyword(pc));
} // chunk_is_intrinsic_type


bool chunk_is_keyword(chunk_t *pc)
{
   return(find_keyword_type(pc->text(),
                            pc->str.size()) != CT_WORD);
} // chunk_is_keyword


bool chunk_is_logical_or_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "||", 2));
} // chunk_is_logical_or_str


bool chunk_is_logical_or_token(chunk_t *pc)
{
   return(  chunk_is_logical_or_str(pc)
         && chunk_is_token(pc, CT_BOOL));
} // chunk_is_logical_or_token


bool chunk_is_logical_or_token_overload(chunk_t *pc)
{
   return(  chunk_is_logical_or_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_logical_or_token_overload


bool chunk_is_lshift_assign_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "<<=", 3));
} // chunk_is_lshift_assign_str


bool chunk_is_lshift_assign_token(chunk_t *pc)
{
   return(  chunk_is_lshift_assign_str(pc)
         && chunk_is_assign_token(pc));
} // chunk_is_lshift_assign_token


bool chunk_is_lshift_assign_token_overload(chunk_t *pc)
{
   return(  chunk_is_lshift_assign_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_lshift_assign_token_overload


bool chunk_is_lshift_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "<<", 2));
} // chunk_is_lshift_str


bool chunk_is_lshift_token(chunk_t *pc)
{
   return(  chunk_is_lshift_str(pc)
         && chunk_is_token(pc, CT_SHIFT));
} // chunk_is_lshift_token


bool chunk_is_lshift_token_overload(chunk_t *pc)
{
   return(  chunk_is_lshift_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_lshift_token_overload


bool chunk_is_macro_reference(chunk_t *pc)
{
   LOG_FUNC_ENTRY();

   auto *next = chunk_get_head();

   if (  (  language_is_set(LANG_CPP)
         || language_is_set(LANG_C))
      && chunk_is_token(pc, CT_WORD)
      && !pc->flags.test(PCF_IN_PREPROC))
   {
      while (next != nullptr)
      {
         if (  next->flags.test(PCF_IN_PREPROC)
            && std::strcmp(pc->str.c_str(), next->str.c_str()) == 0)
         {
            return(true);
         }
         next = chunk_search_next_cat(next, CT_MACRO);
      }
   }
   return(false);
} // chunk_is_macro_reference


bool chunk_is_member_str(chunk_t *pc)
{
   return(  chunk_is_str(pc, ".", 1)
         || chunk_is_str(pc, ".*", 2)
         || chunk_is_str(pc, "->", 2)
         || chunk_is_str(pc, "->*", 3));
} // chunk_is_member_str


bool chunk_is_member_token(chunk_t *pc)
{
   return(  chunk_is_member_str(pc)
         && chunk_is_token(pc, CT_MEMBER));
} // chunk_is_member_token


bool chunk_is_member_token_overload(chunk_t *pc)
{
   return(  (  chunk_is_str(pc, "->", 2)
            || chunk_is_str(pc, "->*", 3))
         && chunk_is_overloaded_token(pc));
} // chunk_is_member_token_overload


bool chunk_is_minus_str(chunk_t *pc)
{
   return(  pc != nullptr
         && (pc->len() == 1)
         && (pc->str[0] == '-'));
} // chunk_is_minus_str


bool chunk_is_minus_token(chunk_t *pc)
{
   return(  (  chunk_is_minus_str(pc)
            && chunk_is_token(pc, CT_ARITH))
         || chunk_is_token(pc, CT_MINUS));
} // chunk_is_minus_token


bool chunk_is_minus_token_overload(chunk_t *pc)
{
   return(  chunk_is_minus_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_minus_token_overload


bool chunk_is_modulo_assign_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "%=", 2));
} // chunk_is_modulo_assign_str


bool chunk_is_modulo_assign_token(chunk_t *pc)
{
   return(  chunk_is_modulo_assign_str(pc)
         && chunk_is_assign_token(pc));
} // chunk_is_modulo_assign_token


bool chunk_is_modulo_assign_token_overload(chunk_t *pc)
{
   return(  chunk_is_modulo_assign_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_modulo_assign_token_overload


bool chunk_is_modulo_str(chunk_t *pc)
{
   return(  pc != nullptr
         && (pc->len() == 1)
         && (pc->str[0] == '%'));
} // chunk_is_modulo_str


bool chunk_is_modulo_token(chunk_t *pc)
{
   return(  chunk_is_modulo_str(pc)
         && chunk_is_token(pc, CT_ARITH));
} // chunk_is_modulo_token


bool chunk_is_modulo_token_overload(chunk_t *pc)
{
   return(  chunk_is_modulo_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_modulo_token_overload


bool chunk_is_mutable_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "mutable", 7));
} // chunk_is_mutable_str


bool chunk_is_mutable_token(chunk_t *pc)
{
   return(  chunk_is_mutable_str(pc)
         && chunk_is_token(pc, CT_QUALIFIER));
} // chunk_is_mutable_token


bool chunk_is_new_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "new", 3));
} // chunk_is_new_str


bool chunk_is_new_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_NEW));
} // chunk_is_new_token


bool chunk_is_new_token_overload(chunk_t *pc)
{
   return(  chunk_is_new_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_new_token_overload


bool chunk_is_noexcept_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "noexcept", 8));
} // chunk_is_noexcept_str


bool chunk_is_noexcept_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_NOEXCEPT));
} // chunk_is_noexcept_token


bool chunk_is_number_token(chunk_t *pc)
{
   return(  chunk_is_floating_point_number_token(pc)
         || chunk_is_integral_number_token(pc));
} // chunk_is_number


bool chunk_is_operator_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "operator", 8));
} // chunk_is_operator_str


bool chunk_is_operator_token(chunk_t *pc)
{
   return(  chunk_is_operator_str(pc)
         && chunk_is_token(pc, CT_OPERATOR));
} // chunk_is_operator_token


bool chunk_is_overloaded_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_OPERATOR_VAL));
} // chunk_is_overloaded_token


bool chunk_is_override_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "override", 8));
} // chunk_is_override_str


bool chunk_is_override_token(chunk_t *pc)
{
   return(  chunk_is_override_str(pc)
         && chunk_is_token(pc, CT_QUALIFIER));
} // chunk_is_override_token


bool chunk_is_paren_close_str(chunk_t *pc)
{
   return(  pc != nullptr
         && (pc->len() == 1)
         && (pc->str[0] == ')'));
} // chunk_is_paren_close_str


bool chunk_is_paren_close_token(chunk_t *pc)
{
   return(  chunk_is_token(pc, CT_FPAREN_CLOSE)
         || chunk_is_token(pc, CT_LPAREN_CLOSE)
         || chunk_is_token(pc, CT_PAREN_CLOSE)
         || chunk_is_token(pc, CT_SPAREN_CLOSE)
         || chunk_is_token(pc, CT_TPAREN_CLOSE));
} // chunk_is_paren_close_token


bool chunk_is_paren_open_str(chunk_t *pc)
{
   return(  pc != nullptr
         && (pc->len() == 1)
         && (pc->str[0] == '('));
} // chunk_is_paren_open_str


bool chunk_is_paren_open_token(chunk_t *pc)
{
   return(  chunk_is_token(pc, CT_FPAREN_OPEN)
         || chunk_is_token(pc, CT_LPAREN_OPEN)
         || chunk_is_token(pc, CT_PAREN_OPEN)
         || chunk_is_token(pc, CT_SPAREN_OPEN)
         || chunk_is_token(pc, CT_TPAREN_OPEN));
} // chunk_is_paren_open_token


bool chunk_is_plus_str(chunk_t *pc)
{
   return(  pc != nullptr
         && (pc->len() == 1)
         && (pc->str[0] == '+'));
} // chunk_is_plus_str


bool chunk_is_plus_token(chunk_t *pc)
{
   return(  (  chunk_is_plus_str(pc)
            && chunk_is_token(pc, CT_ARITH))
         || chunk_is_token(pc, CT_PLUS));
} // chunk_is_plus_token


bool chunk_is_plus_token_overload(chunk_t *pc)
{
   return(  chunk_is_plus_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_plus_token_overload


bool chunk_is_pointer_reference_or_cv_qualifier(chunk_t *pc)
{
   return(  chunk_is_pointer_or_reference(pc)
         || (  chunk_is_cv_qualifier_token(pc)
            && !chunk_is_cpp_inheritance_access_specifier(pc)));
} // chunk_is_pointer_reference_or_qualifier


bool chunk_is_question_str(chunk_t *pc)
{
   return(  pc != nullptr
         && (pc->len() == 1)
         && (pc->str[0] == '?'));
} // chunk_is_question_str


bool chunk_is_question_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_QUESTION));
} // chunk_is_question_token


bool chunk_is_reinterpret_cast_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "reinterpret_cast", 16));
} // chunk_is_reinterpret_cast_str


bool chunk_is_reinterpret_cast_token(chunk_t *pc)
{
   return(  chunk_is_reinterpret_cast_str(pc)
         && chunk_is_token(pc, CT_TYPE_CAST));
} // chunk_is_reinterpret_cast_token


bool chunk_is_rshift_assign_str(chunk_t *pc)
{
   return(chunk_is_str(pc, ">>=", 2));
} // chunk_is_rshift_assign_str


bool chunk_is_rshift_assign_token(chunk_t *pc)
{
   return(  chunk_is_rshift_assign_str(pc)
         && chunk_is_assign_token(pc));
} // chunk_is_rshift_assign_token


bool chunk_is_rshift_assign_token_overload(chunk_t *pc)
{
   return(  chunk_is_rshift_assign_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_rshift_assign_token_overload


bool chunk_is_rshift_str(chunk_t *pc)
{
   return(chunk_is_str(pc, ">>", 2));
} // chunk_is_rshift_str


bool chunk_is_rshift_token(chunk_t *pc)
{
   return(  chunk_is_rshift_str(pc)
         && chunk_is_token(pc, CT_SHIFT));
} // chunk_is_rshift_token


bool chunk_is_rshift_token_overload(chunk_t *pc)
{
   return(  chunk_is_rshift_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_rshift_token_overload


bool chunk_is_semicolon_str(chunk_t *pc)
{
   return(  pc != nullptr
         && (pc->len() == 1)
         && (pc->str[0] == ';'));
} // chunk_is_semicolon_str


bool chunk_is_semicolon_token(chunk_t *pc)
{
   return(  chunk_is_token(pc, CT_SEMICOLON)
         || chunk_is_token(pc, CT_VSEMICOLON));
} // chunk_is_semicolon_token


bool chunk_is_shift_assign_str(chunk_t *pc)
{
   return(  chunk_is_lshift_assign_str(pc)
         || chunk_is_rshift_assign_str(pc));
} // chunk_is_shift_assign_str


bool chunk_is_shift_assign_token(chunk_t *pc)
{
   return(  chunk_is_shift_assign_str(pc)
         && chunk_is_assign_token(pc));
} // chunk_is_shift_assign_token


bool chunk_is_shift_assign_token_overload(chunk_t *pc)
{
   return(  chunk_is_shift_assign_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_shift_assign_token_overload


bool chunk_is_shift_str(chunk_t *pc)
{
   return(  chunk_is_lshift_str(pc)
         || chunk_is_rshift_str(pc));
} // chunk_is_shift_str


bool chunk_is_shift_token(chunk_t *pc)
{
   return(  chunk_is_shift_str(pc)
         && chunk_is_token(pc, CT_SHIFT));
} // chunk_is_shift_token


bool chunk_is_shift_token_overload(chunk_t *pc)
{
   return(  chunk_is_shift_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_shift_token_overload


bool chunk_is_sizeof_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "sizeof", 6));
} // chunk_is_sizeof_str


bool chunk_is_sizeof_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_SIZEOF));
} // chunk_is_sizeof_token


bool chunk_is_square_close_str(chunk_t *pc)
{
   return(  pc != nullptr
         && (pc->len() == 1)
         && (pc->str[0] == ']'));
} // chunk_is_square_close_str


bool chunk_is_square_close_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_SQUARE_CLOSE));
} // chunk_is_square_close_token


bool chunk_is_square_open_str(chunk_t *pc)
{
   return(  pc != nullptr
         && (pc->len() == 1)
         && (pc->str[0] == '['));
} // chunk_is_square_open_str


bool chunk_is_square_open_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_SQUARE_OPEN));
} // chunk_is_square_open_token


bool chunk_is_star_assign_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "*=", 2));
} // chunk_is_star_assign_str


bool chunk_is_star_assign_token(chunk_t *pc)
{
   return(  chunk_is_star_assign_str(pc)
         && chunk_is_assign_token(pc));
} // chunk_is_star_assign_token


bool chunk_is_star_assign_token_overload(chunk_t *pc)
{
   return(  chunk_is_star_assign_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_star_assign_token_overload


bool chunk_is_star_str(chunk_t *pc)
{
   return(  pc != nullptr
         && (pc->len() == 1)
         && (pc->str[0] == '*'));
} // chunk_is_star_str


bool chunk_is_star_token(chunk_t *pc)
{
   return(  chunk_is_star_str(pc)
         && (  chunk_is_token(pc, CT_ARITH)
            || chunk_is_token(pc, CT_DEREF)
            || chunk_is_token(pc, CT_PTR_TYPE)
            || chunk_is_token(pc, CT_STAR)));
} // chunk_is_star_token


bool chunk_is_star_token_overload(chunk_t *pc)
{
   return(  chunk_is_star_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_star_token_overload


bool chunk_is_static_cast_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "static_cast", 11));
} // chunk_is_static_cast_str


bool chunk_is_static_cast_token(chunk_t *pc)
{
   return(  chunk_is_static_cast_str(pc)
         && chunk_is_token(pc, CT_TYPE_CAST));
} // chunk_is_static_cast_token


bool chunk_is_static_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "static", 6));
} // chunk_is_static_str


bool chunk_is_static_token(chunk_t *pc)
{
   return(  chunk_is_static_str(pc)
         && chunk_is_token(pc, CT_QUALIFIER));
} // chunk_is_static_token


bool chunk_is_string_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_STRING));
} // chunk_is_string_token


bool chunk_is_subscript_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_TSQUARE));
} // chunk_is_subscript_token


bool chunk_is_subscript_token_overload(chunk_t *pc)
{
   return(  chunk_is_empty_square_brackets_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_subscript_token_overload


bool chunk_is_subtract_assign_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "-=", 2));
} // chunk_is_subtract_assign_str


bool chunk_is_subtract_assign_token(chunk_t *pc)
{
   return(  chunk_is_subtract_assign_str(pc)
         && chunk_is_assign_token(pc));
} // chunk_is_subtract_assign_token


bool chunk_is_subtract_assign_token_overload(chunk_t *pc)
{
   return(  chunk_is_subtract_assign_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_subtract_assign_token_overload


bool chunk_is_template_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "template", 8));
} // chunk_is_template_str


bool chunk_is_template_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_TEMPLATE));
} // chunk_is_template_token


bool chunk_is_throw_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "throw", 5));
} // chunk_is_throw_str


bool chunk_is_throw_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_THROW));
} // chunk_is_throw_token


bool chunk_is_tilde_str(chunk_t *pc)
{
   return(  pc != nullptr
         && (pc->len() == 1)
         && (pc->str[0] == '~'));
} // chunk_is_tilde_str


bool chunk_is_tilde_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_INV));
} // chunk_is_tilde_token


bool chunk_is_tilde_token_overload(chunk_t *pc)
{
   return(  chunk_is_tilde_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_tilde_token_overload


bool chunk_is_typeid_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "typeid", 6));
} // chunk_is_typeid_str


bool chunk_is_typeid_token(chunk_t *pc)
{
   return(  chunk_is_typeid_str(pc)
         && chunk_is_token(pc, CT_SIZEOF));
} // chunk_is_typeid_token


bool chunk_is_typename_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "typename", 8));
} // chunk_is_typename_str


bool chunk_is_typename_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_TYPENAME));
} // chunk_is_typename_token


bool chunk_is_unary_not_str(chunk_t *pc)
{
   return(  pc != nullptr
         && (pc->len() == 1)
         && (pc->str[0] == '!'));
} // chunk_is_unary_not_str


bool chunk_is_unary_not_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_NOT));
} // chunk_is_unary_not_token


bool chunk_is_unary_not_token_overload(chunk_t *pc)
{
   return(  chunk_is_unary_not_str(pc)
         && chunk_is_overloaded_token(pc));
} // chunk_is_unary_not_token_overload


bool chunk_is_using_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "using", 5));
} // chunk_is_using_str


bool chunk_is_using_token(chunk_t *pc)
{
   return(  chunk_is_token(pc, CT_USING)
         || chunk_is_token(pc, CT_USING_STMT)
         || chunk_is_token(pc, CT_USING_ALIAS));
} // chunk_is_using_token


bool chunk_is_virtual_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "virtual", 7));
} // chunk_is_virtual_str


bool chunk_is_virtual_token(chunk_t *pc)
{
   return(  chunk_is_virtual_str(pc)
         && chunk_is_token(pc, CT_QUALIFIER));
} // chunk_is_virtual_token


bool chunk_is_volatile_str(chunk_t *pc)
{
   return(chunk_is_str(pc, "volatile", 8));
} // chunk_is_volatile_str


bool chunk_is_volatile_token(chunk_t *pc)
{
   return(chunk_is_token(pc, CT_VOLATILE));
} // chunk_is_volatile_token


bool chunk_is_within_constructor_initializer_list(chunk_t *pc)
{
   LOG_FUNC_ENTRY();

   if (pc != nullptr)
   {
      std::size_t level = pc->level;

      /**
       * Skip to the previous close brace-colon chain
       */
      chunk_t *close_paren = match_chain_prev(pc,
// *INDENT-OFF*
                                              { { ")", 1UL }, { ":", 1UL } },
// *INDENT-ON*
                                              level,
                                              scope_e::PREPROC);

      chunk_t *colon = nullptr;

      if (match_function_header_at_close_paren(close_paren) == nullptr)
      {
         colon = chunk_get_next_ncnnl(close_paren, scope_e::PREPROC);
      }
      chunk_t *end = nullptr;

      if (colon != nullptr)
      {
         /**
          * Skip to the (potential) end of the initialization list
          */
         end = match_chain_next(pc,
// *INDENT-OFF*
                                { { { ")", 1UL }, { "{", 1UL } },
                                  { { "}", 1UL }, { "{", 1UL } } },
// *INDENT-ON*
                                level,
                                scope_e::PREPROC);
      }

      if (end != nullptr)
      {
         /**
          * TODO: Should it already be assumed to be between the colon and
          *       the end chunk by virtue of the tests performed thus far?
          */
         return(chunk_is_between(pc, colon, end));
      }
   }
   return(false);
} // chunk_is_within_member_initializer_list


bool chunk_is_within_function_definition_body(chunk_t *pc)
{
   LOG_FUNC_ENTRY();

   std::size_t level;

   if (  pc != nullptr
      && (level = pc->level) > 0)
   {
      /**
       * Skip to the last open brace
       */
      chunk_t *brace_open = chunk_get_prev_type(pc,
                                                CT_BRACE_OPEN,
                                                level - 1,
                                                scope_e::PREPROC);

      if (chunk_is_brace_open_token(brace_open))
      {
         chunk_t *prev = chunk_get_prev_ncnnlni(brace_open, scope_e::PREPROC);
         prev = skip_member_initialization_list_rev(prev);

         if (chunk_is_colon_token(prev))
         {
            /**
             * Detected a constructor member initialization list
             */
            prev = chunk_get_prev_ncnnlni(prev, scope_e::PREPROC);
         }
         else
         {
            /**
             * Skip any trailing function qualifiers
             */
            prev = skip_trailing_function_qualifiers_rev(prev, scope_e::PREPROC);
         }

         if (match_function_header_at_close_paren(prev) != nullptr)
         {
            return(true);
         }
         else if (  prev != nullptr
                 && prev->level > 0)
         {
            /**
             * It's possible that the chunk is currently inside a braced-initializer list;
             * call the function again with this chunk and perform the test again
             */
            return(chunk_is_within_function_definition_body(prev));
         }
      }
   }
   return(false);
} // chunk_is_within_function_definition_body
