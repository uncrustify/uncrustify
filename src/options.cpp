/**
 * @file options.cpp
 * Accessors for options.
 *
 * @license GPL v2+
 */

// Generated (mostly) with sed from option.cpp:
// :a
// /unc_add_option/bb
// d;ba
//
// :b
// s/\s*unc_add_option\("(\w+)\",\s*\w+,\s*AT_(\w+).*/\2 \1()\n{ return(cpd.settings[UO_\1].\2); }/
// s/^BOOL/bool&/
// s/^IARF/iarf_e&/
// s/^UNUM/size_t&/
// s/^NUM/int&/
// s/^POS/tokenpos_e&/
// s/^LINE/lineends_e&/
// s/^STRING/std::string/
// s/\.BOOL/.b/
// s/\.IARF/.a/
// s/\.UNUM/.u/
// s/\.NUM/.n/
// s/\.POS/.tp/
// s/\.LINE/.le/
// s/\.STRING/.str/

#include "options.h"

#include "uncrustify_types.h"

namespace uncrustify
{
namespace options
{
lineends_e &newlines()
{
   return(cpd.settings[UO_newlines].le);
}


size_t &input_tab_size()
{
   return(cpd.settings[UO_input_tab_size].u);
}


size_t &output_tab_size()
{
   return(cpd.settings[UO_output_tab_size].u);
}


size_t &string_escape_char()
{
   return(cpd.settings[UO_string_escape_char].u);
}


size_t &string_escape_char2()
{
   return(cpd.settings[UO_string_escape_char2].u);
}


bool &string_replace_tab_chars()
{
   return(cpd.settings[UO_string_replace_tab_chars].b);
}


bool &tok_split_gte()
{
   return(cpd.settings[UO_tok_split_gte].b);
}


std::string disable_processing_cmt()
{
   if (cpd.settings[UO_disable_processing_cmt].str)
   {
      return(cpd.settings[UO_disable_processing_cmt].str);
   }
   return(std::string{});
}


std::string enable_processing_cmt()
{
   if (cpd.settings[UO_enable_processing_cmt].str)
   {
      return(cpd.settings[UO_enable_processing_cmt].str);
   }
   return(std::string{});
}


bool &enable_digraphs()
{
   return(cpd.settings[UO_enable_digraphs].b);
}


iarf_e &utf8_bom()
{
   return(cpd.settings[UO_utf8_bom].a);
}


bool &utf8_byte()
{
   return(cpd.settings[UO_utf8_byte].b);
}


bool &utf8_force()
{
   return(cpd.settings[UO_utf8_force].b);
}


iarf_e &sp_arith()
{
   return(cpd.settings[UO_sp_arith].a);
}


iarf_e &sp_arith_additive()
{
   return(cpd.settings[UO_sp_arith_additive].a);
}


iarf_e &sp_assign()
{
   return(cpd.settings[UO_sp_assign].a);
}


iarf_e &sp_cpp_lambda_assign()
{
   return(cpd.settings[UO_sp_cpp_lambda_assign].a);
}


iarf_e &sp_cpp_lambda_paren()
{
   return(cpd.settings[UO_sp_cpp_lambda_paren].a);
}


iarf_e &sp_assign_default()
{
   return(cpd.settings[UO_sp_assign_default].a);
}


iarf_e &sp_before_assign()
{
   return(cpd.settings[UO_sp_before_assign].a);
}


iarf_e &sp_after_assign()
{
   return(cpd.settings[UO_sp_after_assign].a);
}


iarf_e &sp_enum_paren()
{
   return(cpd.settings[UO_sp_enum_paren].a);
}


iarf_e &sp_enum_assign()
{
   return(cpd.settings[UO_sp_enum_assign].a);
}


iarf_e &sp_enum_before_assign()
{
   return(cpd.settings[UO_sp_enum_before_assign].a);
}


iarf_e &sp_enum_after_assign()
{
   return(cpd.settings[UO_sp_enum_after_assign].a);
}


iarf_e &sp_enum_colon()
{
   return(cpd.settings[UO_sp_enum_colon].a);
}


iarf_e &sp_pp_concat()
{
   return(cpd.settings[UO_sp_pp_concat].a);
}


iarf_e &sp_pp_stringify()
{
   return(cpd.settings[UO_sp_pp_stringify].a);
}


iarf_e &sp_before_pp_stringify()
{
   return(cpd.settings[UO_sp_before_pp_stringify].a);
}


iarf_e &sp_bool()
{
   return(cpd.settings[UO_sp_bool].a);
}


iarf_e &sp_compare()
{
   return(cpd.settings[UO_sp_compare].a);
}


iarf_e &sp_inside_paren()
{
   return(cpd.settings[UO_sp_inside_paren].a);
}


iarf_e &sp_paren_paren()
{
   return(cpd.settings[UO_sp_paren_paren].a);
}


iarf_e &sp_cparen_oparen()
{
   return(cpd.settings[UO_sp_cparen_oparen].a);
}


bool &sp_balance_nested_parens()
{
   return(cpd.settings[UO_sp_balance_nested_parens].b);
}


iarf_e &sp_paren_brace()
{
   return(cpd.settings[UO_sp_paren_brace].a);
}


iarf_e &sp_brace_brace()
{
   return(cpd.settings[UO_sp_brace_brace].a);
}


iarf_e &sp_before_ptr_star()
{
   return(cpd.settings[UO_sp_before_ptr_star].a);
}


iarf_e &sp_before_unnamed_ptr_star()
{
   return(cpd.settings[UO_sp_before_unnamed_ptr_star].a);
}


iarf_e &sp_between_ptr_star()
{
   return(cpd.settings[UO_sp_between_ptr_star].a);
}


iarf_e &sp_after_ptr_star()
{
   return(cpd.settings[UO_sp_after_ptr_star].a);
}


iarf_e &sp_after_ptr_block_caret()
{
   return(cpd.settings[UO_sp_after_ptr_block_caret].a);
}


iarf_e &sp_after_ptr_star_qualifier()
{
   return(cpd.settings[UO_sp_after_ptr_star_qualifier].a);
}


iarf_e &sp_after_ptr_star_func()
{
   return(cpd.settings[UO_sp_after_ptr_star_func].a);
}


iarf_e &sp_ptr_star_paren()
{
   return(cpd.settings[UO_sp_ptr_star_paren].a);
}


iarf_e &sp_before_ptr_star_func()
{
   return(cpd.settings[UO_sp_before_ptr_star_func].a);
}


iarf_e &sp_before_byref()
{
   return(cpd.settings[UO_sp_before_byref].a);
}


iarf_e &sp_before_unnamed_byref()
{
   return(cpd.settings[UO_sp_before_unnamed_byref].a);
}


iarf_e &sp_after_byref()
{
   return(cpd.settings[UO_sp_after_byref].a);
}


iarf_e &sp_after_byref_func()
{
   return(cpd.settings[UO_sp_after_byref_func].a);
}


iarf_e &sp_before_byref_func()
{
   return(cpd.settings[UO_sp_before_byref_func].a);
}


iarf_e &sp_after_type()
{
   return(cpd.settings[UO_sp_after_type].a);
}


iarf_e &sp_after_decltype()
{
   return(cpd.settings[UO_sp_after_decltype].a);
}


iarf_e &sp_before_template_paren()
{
   return(cpd.settings[UO_sp_before_template_paren].a);
}


iarf_e &sp_template_angle()
{
   return(cpd.settings[UO_sp_template_angle].a);
}


iarf_e &sp_before_angle()
{
   return(cpd.settings[UO_sp_before_angle].a);
}


iarf_e &sp_inside_angle()
{
   return(cpd.settings[UO_sp_inside_angle].a);
}


iarf_e &sp_angle_colon()
{
   return(cpd.settings[UO_sp_angle_colon].a);
}


iarf_e &sp_after_angle()
{
   return(cpd.settings[UO_sp_after_angle].a);
}


iarf_e &sp_angle_paren()
{
   return(cpd.settings[UO_sp_angle_paren].a);
}


iarf_e &sp_angle_paren_empty()
{
   return(cpd.settings[UO_sp_angle_paren_empty].a);
}


iarf_e &sp_angle_word()
{
   return(cpd.settings[UO_sp_angle_word].a);
}


iarf_e &sp_angle_shift()
{
   return(cpd.settings[UO_sp_angle_shift].a);
}


bool &sp_permit_cpp11_shift()
{
   return(cpd.settings[UO_sp_permit_cpp11_shift].b);
}


iarf_e &sp_before_sparen()
{
   return(cpd.settings[UO_sp_before_sparen].a);
}


iarf_e &sp_inside_sparen()
{
   return(cpd.settings[UO_sp_inside_sparen].a);
}


iarf_e &sp_inside_sparen_close()
{
   return(cpd.settings[UO_sp_inside_sparen_close].a);
}


iarf_e &sp_inside_sparen_open()
{
   return(cpd.settings[UO_sp_inside_sparen_open].a);
}


iarf_e &sp_after_sparen()
{
   return(cpd.settings[UO_sp_after_sparen].a);
}


iarf_e &sp_sparen_brace()
{
   return(cpd.settings[UO_sp_sparen_brace].a);
}


iarf_e &sp_invariant_paren()
{
   return(cpd.settings[UO_sp_invariant_paren].a);
}


iarf_e &sp_after_invariant_paren()
{
   return(cpd.settings[UO_sp_after_invariant_paren].a);
}


iarf_e &sp_special_semi()
{
   return(cpd.settings[UO_sp_special_semi].a);
}


iarf_e &sp_before_semi()
{
   return(cpd.settings[UO_sp_before_semi].a);
}


iarf_e &sp_before_semi_for()
{
   return(cpd.settings[UO_sp_before_semi_for].a);
}


iarf_e &sp_before_semi_for_empty()
{
   return(cpd.settings[UO_sp_before_semi_for_empty].a);
}


iarf_e &sp_after_semi()
{
   return(cpd.settings[UO_sp_after_semi].a);
}


iarf_e &sp_after_semi_for()
{
   return(cpd.settings[UO_sp_after_semi_for].a);
}


iarf_e &sp_after_semi_for_empty()
{
   return(cpd.settings[UO_sp_after_semi_for_empty].a);
}


iarf_e &sp_before_square()
{
   return(cpd.settings[UO_sp_before_square].a);
}


iarf_e &sp_before_squares()
{
   return(cpd.settings[UO_sp_before_squares].a);
}


iarf_e &sp_cpp_before_struct_binding()
{
   return(cpd.settings[UO_sp_cpp_before_struct_binding].a);
}


iarf_e &sp_inside_square()
{
   return(cpd.settings[UO_sp_inside_square].a);
}


iarf_e &sp_inside_square_oc_array()
{
   return(cpd.settings[UO_sp_inside_square_oc_array].a);
}


iarf_e &sp_after_comma()
{
   return(cpd.settings[UO_sp_after_comma].a);
}


iarf_e &sp_before_comma()
{
   return(cpd.settings[UO_sp_before_comma].a);
}


iarf_e &sp_after_mdatype_commas()
{
   return(cpd.settings[UO_sp_after_mdatype_commas].a);
}


iarf_e &sp_before_mdatype_commas()
{
   return(cpd.settings[UO_sp_before_mdatype_commas].a);
}


iarf_e &sp_between_mdatype_commas()
{
   return(cpd.settings[UO_sp_between_mdatype_commas].a);
}


iarf_e &sp_paren_comma()
{
   return(cpd.settings[UO_sp_paren_comma].a);
}


iarf_e &sp_before_ellipsis()
{
   return(cpd.settings[UO_sp_before_ellipsis].a);
}


iarf_e &sp_type_ellipsis()
{
   return(cpd.settings[UO_sp_type_ellipsis].a);
}


iarf_e &sp_paren_ellipsis()
{
   return(cpd.settings[UO_sp_paren_ellipsis].a);
}


iarf_e &sp_after_class_colon()
{
   return(cpd.settings[UO_sp_after_class_colon].a);
}


iarf_e &sp_before_class_colon()
{
   return(cpd.settings[UO_sp_before_class_colon].a);
}


iarf_e &sp_after_constr_colon()
{
   return(cpd.settings[UO_sp_after_constr_colon].a);
}


iarf_e &sp_before_constr_colon()
{
   return(cpd.settings[UO_sp_before_constr_colon].a);
}


iarf_e &sp_before_case_colon()
{
   return(cpd.settings[UO_sp_before_case_colon].a);
}


iarf_e &sp_after_operator()
{
   return(cpd.settings[UO_sp_after_operator].a);
}


iarf_e &sp_after_operator_sym()
{
   return(cpd.settings[UO_sp_after_operator_sym].a);
}


iarf_e &sp_after_operator_sym_empty()
{
   return(cpd.settings[UO_sp_after_operator_sym_empty].a);
}


iarf_e &sp_after_cast()
{
   return(cpd.settings[UO_sp_after_cast].a);
}


iarf_e &sp_inside_paren_cast()
{
   return(cpd.settings[UO_sp_inside_paren_cast].a);
}


iarf_e &sp_cpp_cast_paren()
{
   return(cpd.settings[UO_sp_cpp_cast_paren].a);
}


iarf_e &sp_sizeof_paren()
{
   return(cpd.settings[UO_sp_sizeof_paren].a);
}


iarf_e &sp_sizeof_ellipsis()
{
   return(cpd.settings[UO_sp_sizeof_ellipsis].a);
}


iarf_e &sp_sizeof_ellipsis_paren()
{
   return(cpd.settings[UO_sp_sizeof_ellipsis_paren].a);
}


iarf_e &sp_decltype_paren()
{
   return(cpd.settings[UO_sp_decltype_paren].a);
}


iarf_e &sp_after_tag()
{
   return(cpd.settings[UO_sp_after_tag].a);
}


iarf_e &sp_inside_braces_enum()
{
   return(cpd.settings[UO_sp_inside_braces_enum].a);
}


iarf_e &sp_inside_braces_struct()
{
   return(cpd.settings[UO_sp_inside_braces_struct].a);
}


iarf_e &sp_inside_braces_oc_dict()
{
   return(cpd.settings[UO_sp_inside_braces_oc_dict].a);
}


iarf_e &sp_after_type_brace_init_lst_open()
{
   return(cpd.settings[UO_sp_after_type_brace_init_lst_open].a);
}


iarf_e &sp_before_type_brace_init_lst_close()
{
   return(cpd.settings[UO_sp_before_type_brace_init_lst_close].a);
}


iarf_e &sp_inside_type_brace_init_lst()
{
   return(cpd.settings[UO_sp_inside_type_brace_init_lst].a);
}


iarf_e &sp_inside_braces()
{
   return(cpd.settings[UO_sp_inside_braces].a);
}


iarf_e &sp_inside_braces_empty()
{
   return(cpd.settings[UO_sp_inside_braces_empty].a);
}


iarf_e &sp_type_func()
{
   return(cpd.settings[UO_sp_type_func].a);
}


iarf_e &sp_type_brace_init_lst()
{
   return(cpd.settings[UO_sp_type_brace_init_lst].a);
}


iarf_e &sp_func_proto_paren()
{
   return(cpd.settings[UO_sp_func_proto_paren].a);
}


iarf_e &sp_func_proto_paren_empty()
{
   return(cpd.settings[UO_sp_func_proto_paren_empty].a);
}


iarf_e &sp_func_def_paren()
{
   return(cpd.settings[UO_sp_func_def_paren].a);
}


iarf_e &sp_func_def_paren_empty()
{
   return(cpd.settings[UO_sp_func_def_paren_empty].a);
}


iarf_e &sp_inside_fparens()
{
   return(cpd.settings[UO_sp_inside_fparens].a);
}


iarf_e &sp_inside_fparen()
{
   return(cpd.settings[UO_sp_inside_fparen].a);
}


iarf_e &sp_inside_tparen()
{
   return(cpd.settings[UO_sp_inside_tparen].a);
}


iarf_e &sp_after_tparen_close()
{
   return(cpd.settings[UO_sp_after_tparen_close].a);
}


iarf_e &sp_square_fparen()
{
   return(cpd.settings[UO_sp_square_fparen].a);
}


iarf_e &sp_fparen_brace()
{
   return(cpd.settings[UO_sp_fparen_brace].a);
}


iarf_e &sp_fparen_brace_initializer()
{
   return(cpd.settings[UO_sp_fparen_brace_initializer].a);
}


iarf_e &sp_fparen_dbrace()
{
   return(cpd.settings[UO_sp_fparen_dbrace].a);
}


iarf_e &sp_func_call_paren()
{
   return(cpd.settings[UO_sp_func_call_paren].a);
}


iarf_e &sp_func_call_paren_empty()
{
   return(cpd.settings[UO_sp_func_call_paren_empty].a);
}


iarf_e &sp_func_call_user_paren()
{
   return(cpd.settings[UO_sp_func_call_user_paren].a);
}


iarf_e &sp_func_call_user_inside_fparen()
{
   return(cpd.settings[UO_sp_func_call_user_inside_fparen].a);
}


iarf_e &sp_func_call_user_paren_paren()
{
   return(cpd.settings[UO_sp_func_call_user_paren_paren].a);
}


iarf_e &sp_func_class_paren()
{
   return(cpd.settings[UO_sp_func_class_paren].a);
}


iarf_e &sp_func_class_paren_empty()
{
   return(cpd.settings[UO_sp_func_class_paren_empty].a);
}


iarf_e &sp_return_paren()
{
   return(cpd.settings[UO_sp_return_paren].a);
}


iarf_e &sp_return_brace()
{
   return(cpd.settings[UO_sp_return_brace].a);
}


iarf_e &sp_attribute_paren()
{
   return(cpd.settings[UO_sp_attribute_paren].a);
}


iarf_e &sp_defined_paren()
{
   return(cpd.settings[UO_sp_defined_paren].a);
}


iarf_e &sp_throw_paren()
{
   return(cpd.settings[UO_sp_throw_paren].a);
}


iarf_e &sp_after_throw()
{
   return(cpd.settings[UO_sp_after_throw].a);
}


iarf_e &sp_catch_paren()
{
   return(cpd.settings[UO_sp_catch_paren].a);
}


iarf_e &sp_oc_catch_paren()
{
   return(cpd.settings[UO_sp_oc_catch_paren].a);
}


iarf_e &sp_version_paren()
{
   return(cpd.settings[UO_sp_version_paren].a);
}


iarf_e &sp_scope_paren()
{
   return(cpd.settings[UO_sp_scope_paren].a);
}


iarf_e &sp_super_paren()
{
   return(cpd.settings[UO_sp_super_paren].a);
}


iarf_e &sp_this_paren()
{
   return(cpd.settings[UO_sp_this_paren].a);
}


iarf_e &sp_macro()
{
   return(cpd.settings[UO_sp_macro].a);
}


iarf_e &sp_macro_func()
{
   return(cpd.settings[UO_sp_macro_func].a);
}


iarf_e &sp_else_brace()
{
   return(cpd.settings[UO_sp_else_brace].a);
}


iarf_e &sp_brace_else()
{
   return(cpd.settings[UO_sp_brace_else].a);
}


iarf_e &sp_brace_typedef()
{
   return(cpd.settings[UO_sp_brace_typedef].a);
}


iarf_e &sp_catch_brace()
{
   return(cpd.settings[UO_sp_catch_brace].a);
}


iarf_e &sp_oc_catch_brace()
{
   return(cpd.settings[UO_sp_oc_catch_brace].a);
}


iarf_e &sp_brace_catch()
{
   return(cpd.settings[UO_sp_brace_catch].a);
}


iarf_e &sp_oc_brace_catch()
{
   return(cpd.settings[UO_sp_oc_brace_catch].a);
}


iarf_e &sp_finally_brace()
{
   return(cpd.settings[UO_sp_finally_brace].a);
}


iarf_e &sp_brace_finally()
{
   return(cpd.settings[UO_sp_brace_finally].a);
}


iarf_e &sp_try_brace()
{
   return(cpd.settings[UO_sp_try_brace].a);
}


iarf_e &sp_getset_brace()
{
   return(cpd.settings[UO_sp_getset_brace].a);
}


iarf_e &sp_word_brace()
{
   return(cpd.settings[UO_sp_word_brace].a);
}


iarf_e &sp_word_brace_ns()
{
   return(cpd.settings[UO_sp_word_brace_ns].a);
}


iarf_e &sp_before_dc()
{
   return(cpd.settings[UO_sp_before_dc].a);
}


iarf_e &sp_after_dc()
{
   return(cpd.settings[UO_sp_after_dc].a);
}


iarf_e &sp_d_array_colon()
{
   return(cpd.settings[UO_sp_d_array_colon].a);
}


iarf_e &sp_not()
{
   return(cpd.settings[UO_sp_not].a);
}


iarf_e &sp_inv()
{
   return(cpd.settings[UO_sp_inv].a);
}


iarf_e &sp_addr()
{
   return(cpd.settings[UO_sp_addr].a);
}


iarf_e &sp_member()
{
   return(cpd.settings[UO_sp_member].a);
}


iarf_e &sp_deref()
{
   return(cpd.settings[UO_sp_deref].a);
}


iarf_e &sp_sign()
{
   return(cpd.settings[UO_sp_sign].a);
}


iarf_e &sp_incdec()
{
   return(cpd.settings[UO_sp_incdec].a);
}


iarf_e &sp_before_nl_cont()
{
   return(cpd.settings[UO_sp_before_nl_cont].a);
}


iarf_e &sp_after_oc_scope()
{
   return(cpd.settings[UO_sp_after_oc_scope].a);
}


iarf_e &sp_after_oc_colon()
{
   return(cpd.settings[UO_sp_after_oc_colon].a);
}


iarf_e &sp_before_oc_colon()
{
   return(cpd.settings[UO_sp_before_oc_colon].a);
}


iarf_e &sp_after_oc_dict_colon()
{
   return(cpd.settings[UO_sp_after_oc_dict_colon].a);
}


iarf_e &sp_before_oc_dict_colon()
{
   return(cpd.settings[UO_sp_before_oc_dict_colon].a);
}


iarf_e &sp_after_send_oc_colon()
{
   return(cpd.settings[UO_sp_after_send_oc_colon].a);
}


iarf_e &sp_before_send_oc_colon()
{
   return(cpd.settings[UO_sp_before_send_oc_colon].a);
}


iarf_e &sp_after_oc_type()
{
   return(cpd.settings[UO_sp_after_oc_type].a);
}


iarf_e &sp_after_oc_return_type()
{
   return(cpd.settings[UO_sp_after_oc_return_type].a);
}


iarf_e &sp_after_oc_at_sel()
{
   return(cpd.settings[UO_sp_after_oc_at_sel].a);
}


iarf_e &sp_after_oc_at_sel_parens()
{
   return(cpd.settings[UO_sp_after_oc_at_sel_parens].a);
}


iarf_e &sp_inside_oc_at_sel_parens()
{
   return(cpd.settings[UO_sp_inside_oc_at_sel_parens].a);
}


iarf_e &sp_before_oc_block_caret()
{
   return(cpd.settings[UO_sp_before_oc_block_caret].a);
}


iarf_e &sp_after_oc_block_caret()
{
   return(cpd.settings[UO_sp_after_oc_block_caret].a);
}


iarf_e &sp_after_oc_msg_receiver()
{
   return(cpd.settings[UO_sp_after_oc_msg_receiver].a);
}


iarf_e &sp_after_oc_property()
{
   return(cpd.settings[UO_sp_after_oc_property].a);
}


iarf_e &sp_after_oc_synchronized()
{
   return(cpd.settings[UO_sp_after_oc_synchronized].a);
}


iarf_e &sp_cond_colon()
{
   return(cpd.settings[UO_sp_cond_colon].a);
}


iarf_e &sp_cond_colon_before()
{
   return(cpd.settings[UO_sp_cond_colon_before].a);
}


iarf_e &sp_cond_colon_after()
{
   return(cpd.settings[UO_sp_cond_colon_after].a);
}


iarf_e &sp_cond_question()
{
   return(cpd.settings[UO_sp_cond_question].a);
}


iarf_e &sp_cond_question_before()
{
   return(cpd.settings[UO_sp_cond_question_before].a);
}


iarf_e &sp_cond_question_after()
{
   return(cpd.settings[UO_sp_cond_question_after].a);
}


iarf_e &sp_cond_ternary_short()
{
   return(cpd.settings[UO_sp_cond_ternary_short].a);
}


iarf_e &sp_case_label()
{
   return(cpd.settings[UO_sp_case_label].a);
}


iarf_e &sp_range()
{
   return(cpd.settings[UO_sp_range].a);
}


iarf_e &sp_after_for_colon()
{
   return(cpd.settings[UO_sp_after_for_colon].a);
}


iarf_e &sp_before_for_colon()
{
   return(cpd.settings[UO_sp_before_for_colon].a);
}


iarf_e &sp_extern_paren()
{
   return(cpd.settings[UO_sp_extern_paren].a);
}


iarf_e &sp_cmt_cpp_start()
{
   return(cpd.settings[UO_sp_cmt_cpp_start].a);
}


bool &sp_cmt_cpp_doxygen()
{
   return(cpd.settings[UO_sp_cmt_cpp_doxygen].b);
}


bool &sp_cmt_cpp_qttr()
{
   return(cpd.settings[UO_sp_cmt_cpp_qttr].b);
}


iarf_e &sp_endif_cmt()
{
   return(cpd.settings[UO_sp_endif_cmt].a);
}


iarf_e &sp_after_new()
{
   return(cpd.settings[UO_sp_after_new].a);
}


iarf_e &sp_between_new_paren()
{
   return(cpd.settings[UO_sp_between_new_paren].a);
}


iarf_e &sp_after_newop_paren()
{
   return(cpd.settings[UO_sp_after_newop_paren].a);
}


iarf_e &sp_inside_newop_paren()
{
   return(cpd.settings[UO_sp_inside_newop_paren].a);
}


iarf_e &sp_inside_newop_paren_open()
{
   return(cpd.settings[UO_sp_inside_newop_paren_open].a);
}


iarf_e &sp_inside_newop_paren_close()
{
   return(cpd.settings[UO_sp_inside_newop_paren_close].a);
}


iarf_e &sp_before_tr_emb_cmt()
{
   return(cpd.settings[UO_sp_before_tr_emb_cmt].a);
}


size_t &sp_num_before_tr_emb_cmt()
{
   return(cpd.settings[UO_sp_num_before_tr_emb_cmt].u);
}


iarf_e &sp_annotation_paren()
{
   return(cpd.settings[UO_sp_annotation_paren].a);
}


bool &sp_skip_vbrace_tokens()
{
   return(cpd.settings[UO_sp_skip_vbrace_tokens].b);
}


iarf_e &sp_after_noexcept()
{
   return(cpd.settings[UO_sp_after_noexcept].a);
}


bool &force_tab_after_define()
{
   return(cpd.settings[UO_force_tab_after_define].b);
}


size_t &indent_columns()
{
   return(cpd.settings[UO_indent_columns].u);
}


int &indent_continue()
{
   return(cpd.settings[UO_indent_continue].n);
}


size_t &indent_continue_class_head()
{
   return(cpd.settings[UO_indent_continue_class_head].u);
}


bool &indent_single_newlines()
{
   return(cpd.settings[UO_indent_single_newlines].b);
}


size_t &indent_param()
{
   return(cpd.settings[UO_indent_param].u);
}


size_t &indent_with_tabs()
{
   return(cpd.settings[UO_indent_with_tabs].u);
}


bool &indent_cmt_with_tabs()
{
   return(cpd.settings[UO_indent_cmt_with_tabs].b);
}


bool &indent_align_string()
{
   return(cpd.settings[UO_indent_align_string].b);
}


size_t &indent_xml_string()
{
   return(cpd.settings[UO_indent_xml_string].u);
}


size_t &indent_brace()
{
   return(cpd.settings[UO_indent_brace].u);
}


bool &indent_braces()
{
   return(cpd.settings[UO_indent_braces].b);
}


bool &indent_braces_no_func()
{
   return(cpd.settings[UO_indent_braces_no_func].b);
}


bool &indent_braces_no_class()
{
   return(cpd.settings[UO_indent_braces_no_class].b);
}


bool &indent_braces_no_struct()
{
   return(cpd.settings[UO_indent_braces_no_struct].b);
}


bool &indent_brace_parent()
{
   return(cpd.settings[UO_indent_brace_parent].b);
}


bool &indent_paren_open_brace()
{
   return(cpd.settings[UO_indent_paren_open_brace].b);
}


bool &indent_cs_delegate_brace()
{
   return(cpd.settings[UO_indent_cs_delegate_brace].b);
}


bool &indent_cs_delegate_body()
{
   return(cpd.settings[UO_indent_cs_delegate_body].b);
}


bool &indent_namespace()
{
   return(cpd.settings[UO_indent_namespace].b);
}


bool &indent_namespace_single_indent()
{
   return(cpd.settings[UO_indent_namespace_single_indent].b);
}


size_t &indent_namespace_level()
{
   return(cpd.settings[UO_indent_namespace_level].u);
}


size_t &indent_namespace_limit()
{
   return(cpd.settings[UO_indent_namespace_limit].u);
}


bool &indent_extern()
{
   return(cpd.settings[UO_indent_extern].b);
}


bool &indent_class()
{
   return(cpd.settings[UO_indent_class].b);
}


bool &indent_class_colon()
{
   return(cpd.settings[UO_indent_class_colon].b);
}


bool &indent_class_on_colon()
{
   return(cpd.settings[UO_indent_class_on_colon].b);
}


bool &indent_constr_colon()
{
   return(cpd.settings[UO_indent_constr_colon].b);
}


size_t &indent_ctor_init_leading()
{
   return(cpd.settings[UO_indent_ctor_init_leading].u);
}


int &indent_ctor_init()
{
   return(cpd.settings[UO_indent_ctor_init].n);
}


bool &indent_else_if()
{
   return(cpd.settings[UO_indent_else_if].b);
}


int &indent_var_def_blk()
{
   return(cpd.settings[UO_indent_var_def_blk].n);
}


bool &indent_var_def_cont()
{
   return(cpd.settings[UO_indent_var_def_cont].b);
}


bool &indent_shift()
{
   return(cpd.settings[UO_indent_shift].b);
}


bool &indent_func_def_force_col1()
{
   return(cpd.settings[UO_indent_func_def_force_col1].b);
}


bool &indent_func_call_param()
{
   return(cpd.settings[UO_indent_func_call_param].b);
}


bool &indent_func_def_param()
{
   return(cpd.settings[UO_indent_func_def_param].b);
}


bool &indent_func_proto_param()
{
   return(cpd.settings[UO_indent_func_proto_param].b);
}


bool &indent_func_class_param()
{
   return(cpd.settings[UO_indent_func_class_param].b);
}


bool &indent_func_ctor_var_param()
{
   return(cpd.settings[UO_indent_func_ctor_var_param].b);
}


bool &indent_template_param()
{
   return(cpd.settings[UO_indent_template_param].b);
}


bool &indent_func_param_double()
{
   return(cpd.settings[UO_indent_func_param_double].b);
}


size_t &indent_func_const()
{
   return(cpd.settings[UO_indent_func_const].u);
}


size_t &indent_func_throw()
{
   return(cpd.settings[UO_indent_func_throw].u);
}


size_t &indent_member()
{
   return(cpd.settings[UO_indent_member].u);
}


bool &indent_member_single()
{
   return(cpd.settings[UO_indent_member_single].b);
}


size_t &indent_sing_line_comments()
{
   return(cpd.settings[UO_indent_sing_line_comments].u);
}


bool &indent_relative_single_line_comments()
{
   return(cpd.settings[UO_indent_relative_single_line_comments].b);
}


size_t &indent_switch_case()
{
   return(cpd.settings[UO_indent_switch_case].u);
}


bool &indent_switch_pp()
{
   return(cpd.settings[UO_indent_switch_pp].b);
}


size_t &indent_case_shift()
{
   return(cpd.settings[UO_indent_case_shift].u);
}


int &indent_case_brace()
{
   return(cpd.settings[UO_indent_case_brace].n);
}


bool &indent_col1_comment()
{
   return(cpd.settings[UO_indent_col1_comment].b);
}


int &indent_label()
{
   return(cpd.settings[UO_indent_label].n);
}


int &indent_access_spec()
{
   return(cpd.settings[UO_indent_access_spec].n);
}


bool &indent_access_spec_body()
{
   return(cpd.settings[UO_indent_access_spec_body].b);
}


bool &indent_paren_nl()
{
   return(cpd.settings[UO_indent_paren_nl].b);
}


size_t &indent_paren_close()
{
   return(cpd.settings[UO_indent_paren_close].u);
}


bool &indent_paren_after_func_def()
{
   return(cpd.settings[UO_indent_paren_after_func_def].b);
}


bool &indent_paren_after_func_decl()
{
   return(cpd.settings[UO_indent_paren_after_func_decl].b);
}


bool &indent_paren_after_func_call()
{
   return(cpd.settings[UO_indent_paren_after_func_call].b);
}


bool &indent_comma_paren()
{
   return(cpd.settings[UO_indent_comma_paren].b);
}


bool &indent_bool_paren()
{
   return(cpd.settings[UO_indent_bool_paren].b);
}


bool &indent_semicolon_for_paren()
{
   return(cpd.settings[UO_indent_semicolon_for_paren].b);
}


bool &indent_first_bool_expr()
{
   return(cpd.settings[UO_indent_first_bool_expr].b);
}


bool &indent_first_for_expr()
{
   return(cpd.settings[UO_indent_first_for_expr].b);
}


bool &indent_square_nl()
{
   return(cpd.settings[UO_indent_square_nl].b);
}


bool &indent_preserve_sql()
{
   return(cpd.settings[UO_indent_preserve_sql].b);
}


bool &indent_align_assign()
{
   return(cpd.settings[UO_indent_align_assign].b);
}


bool &indent_align_paren()
{
   return(cpd.settings[UO_indent_align_paren].b);
}


bool &indent_oc_block()
{
   return(cpd.settings[UO_indent_oc_block].b);
}


size_t &indent_oc_block_msg()
{
   return(cpd.settings[UO_indent_oc_block_msg].u);
}


size_t &indent_oc_msg_colon()
{
   return(cpd.settings[UO_indent_oc_msg_colon].u);
}


bool &indent_oc_msg_prioritize_first_colon()
{
   return(cpd.settings[UO_indent_oc_msg_prioritize_first_colon].b);
}


bool &indent_oc_block_msg_xcode_style()
{
   return(cpd.settings[UO_indent_oc_block_msg_xcode_style].b);
}


bool &indent_oc_block_msg_from_keyword()
{
   return(cpd.settings[UO_indent_oc_block_msg_from_keyword].b);
}


bool &indent_oc_block_msg_from_colon()
{
   return(cpd.settings[UO_indent_oc_block_msg_from_colon].b);
}


bool &indent_oc_block_msg_from_caret()
{
   return(cpd.settings[UO_indent_oc_block_msg_from_caret].b);
}


bool &indent_oc_block_msg_from_brace()
{
   return(cpd.settings[UO_indent_oc_block_msg_from_brace].b);
}


size_t &indent_min_vbrace_open()
{
   return(cpd.settings[UO_indent_min_vbrace_open].u);
}


bool &indent_vbrace_open_on_tabstop()
{
   return(cpd.settings[UO_indent_vbrace_open_on_tabstop].b);
}


bool &indent_token_after_brace()
{
   return(cpd.settings[UO_indent_token_after_brace].b);
}


bool &indent_cpp_lambda_body()
{
   return(cpd.settings[UO_indent_cpp_lambda_body].b);
}


bool &indent_using_block()
{
   return(cpd.settings[UO_indent_using_block].b);
}


size_t &indent_ternary_operator()
{
   return(cpd.settings[UO_indent_ternary_operator].u);
}


bool &indent_off_after_return_new()
{
   return(cpd.settings[UO_indent_off_after_return_new].b);
}


bool &indent_single_after_return()
{
   return(cpd.settings[UO_indent_single_after_return].b);
}


bool &indent_ignore_asm_block()
{
   return(cpd.settings[UO_indent_ignore_asm_block].b);
}


bool &nl_collapse_empty_body()
{
   return(cpd.settings[UO_nl_collapse_empty_body].b);
}


bool &nl_assign_leave_one_liners()
{
   return(cpd.settings[UO_nl_assign_leave_one_liners].b);
}


bool &nl_class_leave_one_liners()
{
   return(cpd.settings[UO_nl_class_leave_one_liners].b);
}


bool &nl_enum_leave_one_liners()
{
   return(cpd.settings[UO_nl_enum_leave_one_liners].b);
}


bool &nl_getset_leave_one_liners()
{
   return(cpd.settings[UO_nl_getset_leave_one_liners].b);
}


bool &nl_cs_property_leave_one_liners()
{
   return(cpd.settings[UO_nl_cs_property_leave_one_liners].b);
}


bool &nl_func_leave_one_liners()
{
   return(cpd.settings[UO_nl_func_leave_one_liners].b);
}


bool &nl_cpp_lambda_leave_one_liners()
{
   return(cpd.settings[UO_nl_cpp_lambda_leave_one_liners].b);
}


bool &nl_if_leave_one_liners()
{
   return(cpd.settings[UO_nl_if_leave_one_liners].b);
}


bool &nl_while_leave_one_liners()
{
   return(cpd.settings[UO_nl_while_leave_one_liners].b);
}


bool &nl_oc_msg_leave_one_liner()
{
   return(cpd.settings[UO_nl_oc_msg_leave_one_liner].b);
}


iarf_e &nl_oc_mdef_brace()
{
   return(cpd.settings[UO_nl_oc_mdef_brace].a);
}


iarf_e &nl_oc_block_brace()
{
   return(cpd.settings[UO_nl_oc_block_brace].a);
}


iarf_e &nl_oc_interface_brace()
{
   return(cpd.settings[UO_nl_oc_interface_brace].a);
}


iarf_e &nl_oc_implementation_brace()
{
   return(cpd.settings[UO_nl_oc_implementation_brace].a);
}


iarf_e &nl_start_of_file()
{
   return(cpd.settings[UO_nl_start_of_file].a);
}


size_t &nl_start_of_file_min()
{
   return(cpd.settings[UO_nl_start_of_file_min].u);
}


iarf_e &nl_end_of_file()
{
   return(cpd.settings[UO_nl_end_of_file].a);
}


size_t &nl_end_of_file_min()
{
   return(cpd.settings[UO_nl_end_of_file_min].u);
}


iarf_e &nl_assign_brace()
{
   return(cpd.settings[UO_nl_assign_brace].a);
}


iarf_e &nl_assign_square()
{
   return(cpd.settings[UO_nl_assign_square].a);
}


iarf_e &nl_tsquare_brace()
{
   return(cpd.settings[UO_nl_tsquare_brace].a);
}


iarf_e &nl_after_square_assign()
{
   return(cpd.settings[UO_nl_after_square_assign].a);
}


size_t &nl_func_var_def_blk()
{
   return(cpd.settings[UO_nl_func_var_def_blk].u);
}


size_t &nl_typedef_blk_start()
{
   return(cpd.settings[UO_nl_typedef_blk_start].u);
}


size_t &nl_typedef_blk_end()
{
   return(cpd.settings[UO_nl_typedef_blk_end].u);
}


size_t &nl_typedef_blk_in()
{
   return(cpd.settings[UO_nl_typedef_blk_in].u);
}


size_t &nl_var_def_blk_start()
{
   return(cpd.settings[UO_nl_var_def_blk_start].u);
}


size_t &nl_var_def_blk_end()
{
   return(cpd.settings[UO_nl_var_def_blk_end].u);
}


size_t &nl_var_def_blk_in()
{
   return(cpd.settings[UO_nl_var_def_blk_in].u);
}


iarf_e &nl_fcall_brace()
{
   return(cpd.settings[UO_nl_fcall_brace].a);
}


iarf_e &nl_enum_brace()
{
   return(cpd.settings[UO_nl_enum_brace].a);
}


iarf_e &nl_enum_class()
{
   return(cpd.settings[UO_nl_enum_class].a);
}


iarf_e &nl_enum_class_identifier()
{
   return(cpd.settings[UO_nl_enum_class_identifier].a);
}


iarf_e &nl_enum_identifier_colon()
{
   return(cpd.settings[UO_nl_enum_identifier_colon].a);
}


iarf_e &nl_enum_colon_type()
{
   return(cpd.settings[UO_nl_enum_colon_type].a);
}


iarf_e &nl_struct_brace()
{
   return(cpd.settings[UO_nl_struct_brace].a);
}


iarf_e &nl_union_brace()
{
   return(cpd.settings[UO_nl_union_brace].a);
}


iarf_e &nl_if_brace()
{
   return(cpd.settings[UO_nl_if_brace].a);
}


iarf_e &nl_brace_else()
{
   return(cpd.settings[UO_nl_brace_else].a);
}


iarf_e &nl_elseif_brace()
{
   return(cpd.settings[UO_nl_elseif_brace].a);
}


iarf_e &nl_else_brace()
{
   return(cpd.settings[UO_nl_else_brace].a);
}


iarf_e &nl_else_if()
{
   return(cpd.settings[UO_nl_else_if].a);
}


iarf_e &nl_before_if_closing_paren()
{
   return(cpd.settings[UO_nl_before_if_closing_paren].a);
}


iarf_e &nl_brace_finally()
{
   return(cpd.settings[UO_nl_brace_finally].a);
}


iarf_e &nl_finally_brace()
{
   return(cpd.settings[UO_nl_finally_brace].a);
}


iarf_e &nl_try_brace()
{
   return(cpd.settings[UO_nl_try_brace].a);
}


iarf_e &nl_getset_brace()
{
   return(cpd.settings[UO_nl_getset_brace].a);
}


iarf_e &nl_for_brace()
{
   return(cpd.settings[UO_nl_for_brace].a);
}


iarf_e &nl_catch_brace()
{
   return(cpd.settings[UO_nl_catch_brace].a);
}


iarf_e &nl_oc_catch_brace()
{
   return(cpd.settings[UO_nl_oc_catch_brace].a);
}


iarf_e &nl_brace_catch()
{
   return(cpd.settings[UO_nl_brace_catch].a);
}


iarf_e &nl_oc_brace_catch()
{
   return(cpd.settings[UO_nl_oc_brace_catch].a);
}


iarf_e &nl_brace_square()
{
   return(cpd.settings[UO_nl_brace_square].a);
}


iarf_e &nl_brace_fparen()
{
   return(cpd.settings[UO_nl_brace_fparen].a);
}


iarf_e &nl_while_brace()
{
   return(cpd.settings[UO_nl_while_brace].a);
}


iarf_e &nl_scope_brace()
{
   return(cpd.settings[UO_nl_scope_brace].a);
}


iarf_e &nl_unittest_brace()
{
   return(cpd.settings[UO_nl_unittest_brace].a);
}


iarf_e &nl_version_brace()
{
   return(cpd.settings[UO_nl_version_brace].a);
}


iarf_e &nl_using_brace()
{
   return(cpd.settings[UO_nl_using_brace].a);
}


iarf_e &nl_brace_brace()
{
   return(cpd.settings[UO_nl_brace_brace].a);
}


iarf_e &nl_do_brace()
{
   return(cpd.settings[UO_nl_do_brace].a);
}


iarf_e &nl_brace_while()
{
   return(cpd.settings[UO_nl_brace_while].a);
}


iarf_e &nl_switch_brace()
{
   return(cpd.settings[UO_nl_switch_brace].a);
}


iarf_e &nl_synchronized_brace()
{
   return(cpd.settings[UO_nl_synchronized_brace].a);
}


bool &nl_multi_line_cond()
{
   return(cpd.settings[UO_nl_multi_line_cond].b);
}


bool &nl_multi_line_define()
{
   return(cpd.settings[UO_nl_multi_line_define].b);
}


bool &nl_before_case()
{
   return(cpd.settings[UO_nl_before_case].b);
}


iarf_e &nl_before_throw()
{
   return(cpd.settings[UO_nl_before_throw].a);
}


bool &nl_after_case()
{
   return(cpd.settings[UO_nl_after_case].b);
}


iarf_e &nl_case_colon_brace()
{
   return(cpd.settings[UO_nl_case_colon_brace].a);
}


iarf_e &nl_namespace_brace()
{
   return(cpd.settings[UO_nl_namespace_brace].a);
}


iarf_e &nl_template_class()
{
   return(cpd.settings[UO_nl_template_class].a);
}


iarf_e &nl_class_brace()
{
   return(cpd.settings[UO_nl_class_brace].a);
}


iarf_e &nl_class_init_args()
{
   return(cpd.settings[UO_nl_class_init_args].a);
}


iarf_e &nl_constr_init_args()
{
   return(cpd.settings[UO_nl_constr_init_args].a);
}


iarf_e &nl_enum_own_lines()
{
   return(cpd.settings[UO_nl_enum_own_lines].a);
}


iarf_e &nl_func_type_name()
{
   return(cpd.settings[UO_nl_func_type_name].a);
}


iarf_e &nl_func_type_name_class()
{
   return(cpd.settings[UO_nl_func_type_name_class].a);
}


iarf_e &nl_func_class_scope()
{
   return(cpd.settings[UO_nl_func_class_scope].a);
}


iarf_e &nl_func_scope_name()
{
   return(cpd.settings[UO_nl_func_scope_name].a);
}


iarf_e &nl_func_proto_type_name()
{
   return(cpd.settings[UO_nl_func_proto_type_name].a);
}


iarf_e &nl_func_paren()
{
   return(cpd.settings[UO_nl_func_paren].a);
}


iarf_e &nl_func_paren_empty()
{
   return(cpd.settings[UO_nl_func_paren_empty].a);
}


iarf_e &nl_func_def_paren()
{
   return(cpd.settings[UO_nl_func_def_paren].a);
}


iarf_e &nl_func_def_paren_empty()
{
   return(cpd.settings[UO_nl_func_def_paren_empty].a);
}


iarf_e &nl_func_call_paren()
{
   return(cpd.settings[UO_nl_func_call_paren].a);
}


iarf_e &nl_func_call_paren_empty()
{
   return(cpd.settings[UO_nl_func_call_paren_empty].a);
}


iarf_e &nl_func_decl_start()
{
   return(cpd.settings[UO_nl_func_decl_start].a);
}


iarf_e &nl_func_def_start()
{
   return(cpd.settings[UO_nl_func_def_start].a);
}


iarf_e &nl_func_decl_start_single()
{
   return(cpd.settings[UO_nl_func_decl_start_single].a);
}


iarf_e &nl_func_def_start_single()
{
   return(cpd.settings[UO_nl_func_def_start_single].a);
}


bool &nl_func_decl_start_multi_line()
{
   return(cpd.settings[UO_nl_func_decl_start_multi_line].b);
}


bool &nl_func_def_start_multi_line()
{
   return(cpd.settings[UO_nl_func_def_start_multi_line].b);
}


iarf_e &nl_func_decl_args()
{
   return(cpd.settings[UO_nl_func_decl_args].a);
}


iarf_e &nl_func_def_args()
{
   return(cpd.settings[UO_nl_func_def_args].a);
}


bool &nl_func_decl_args_multi_line()
{
   return(cpd.settings[UO_nl_func_decl_args_multi_line].b);
}


bool &nl_func_def_args_multi_line()
{
   return(cpd.settings[UO_nl_func_def_args_multi_line].b);
}


iarf_e &nl_func_decl_end()
{
   return(cpd.settings[UO_nl_func_decl_end].a);
}


iarf_e &nl_func_def_end()
{
   return(cpd.settings[UO_nl_func_def_end].a);
}


iarf_e &nl_func_decl_end_single()
{
   return(cpd.settings[UO_nl_func_decl_end_single].a);
}


iarf_e &nl_func_def_end_single()
{
   return(cpd.settings[UO_nl_func_def_end_single].a);
}


bool &nl_func_decl_end_multi_line()
{
   return(cpd.settings[UO_nl_func_decl_end_multi_line].b);
}


bool &nl_func_def_end_multi_line()
{
   return(cpd.settings[UO_nl_func_def_end_multi_line].b);
}


iarf_e &nl_func_decl_empty()
{
   return(cpd.settings[UO_nl_func_decl_empty].a);
}


iarf_e &nl_func_def_empty()
{
   return(cpd.settings[UO_nl_func_def_empty].a);
}


iarf_e &nl_func_call_empty()
{
   return(cpd.settings[UO_nl_func_call_empty].a);
}


bool &nl_func_call_start_multi_line()
{
   return(cpd.settings[UO_nl_func_call_start_multi_line].b);
}


bool &nl_func_call_args_multi_line()
{
   return(cpd.settings[UO_nl_func_call_args_multi_line].b);
}


bool &nl_func_call_end_multi_line()
{
   return(cpd.settings[UO_nl_func_call_end_multi_line].b);
}


bool &nl_oc_msg_args()
{
   return(cpd.settings[UO_nl_oc_msg_args].b);
}


iarf_e &nl_fdef_brace()
{
   return(cpd.settings[UO_nl_fdef_brace].a);
}


iarf_e &nl_cpp_ldef_brace()
{
   return(cpd.settings[UO_nl_cpp_ldef_brace].a);
}


iarf_e &nl_return_expr()
{
   return(cpd.settings[UO_nl_return_expr].a);
}


bool &nl_after_semicolon()
{
   return(cpd.settings[UO_nl_after_semicolon].b);
}


iarf_e &nl_paren_dbrace_open()
{
   return(cpd.settings[UO_nl_paren_dbrace_open].a);
}


iarf_e &nl_type_brace_init_lst()
{
   return(cpd.settings[UO_nl_type_brace_init_lst].a);
}


iarf_e &nl_type_brace_init_lst_open()
{
   return(cpd.settings[UO_nl_type_brace_init_lst_open].a);
}


iarf_e &nl_type_brace_init_lst_close()
{
   return(cpd.settings[UO_nl_type_brace_init_lst_close].a);
}


bool &nl_after_brace_open()
{
   return(cpd.settings[UO_nl_after_brace_open].b);
}


bool &nl_after_brace_open_cmt()
{
   return(cpd.settings[UO_nl_after_brace_open_cmt].b);
}


bool &nl_after_vbrace_open()
{
   return(cpd.settings[UO_nl_after_vbrace_open].b);
}


bool &nl_after_vbrace_open_empty()
{
   return(cpd.settings[UO_nl_after_vbrace_open_empty].b);
}


bool &nl_after_brace_close()
{
   return(cpd.settings[UO_nl_after_brace_close].b);
}


bool &nl_after_vbrace_close()
{
   return(cpd.settings[UO_nl_after_vbrace_close].b);
}


iarf_e &nl_brace_struct_var()
{
   return(cpd.settings[UO_nl_brace_struct_var].a);
}


bool &nl_define_macro()
{
   return(cpd.settings[UO_nl_define_macro].b);
}


bool &nl_squeeze_paren_close()
{
   return(cpd.settings[UO_nl_squeeze_paren_close].b);
}


bool &nl_squeeze_ifdef()
{
   return(cpd.settings[UO_nl_squeeze_ifdef].b);
}


bool &nl_squeeze_ifdef_top_level()
{
   return(cpd.settings[UO_nl_squeeze_ifdef_top_level].b);
}


iarf_e &nl_before_if()
{
   return(cpd.settings[UO_nl_before_if].a);
}


iarf_e &nl_after_if()
{
   return(cpd.settings[UO_nl_after_if].a);
}


iarf_e &nl_before_for()
{
   return(cpd.settings[UO_nl_before_for].a);
}


iarf_e &nl_after_for()
{
   return(cpd.settings[UO_nl_after_for].a);
}


iarf_e &nl_before_while()
{
   return(cpd.settings[UO_nl_before_while].a);
}


iarf_e &nl_after_while()
{
   return(cpd.settings[UO_nl_after_while].a);
}


iarf_e &nl_before_switch()
{
   return(cpd.settings[UO_nl_before_switch].a);
}


iarf_e &nl_after_switch()
{
   return(cpd.settings[UO_nl_after_switch].a);
}


iarf_e &nl_before_synchronized()
{
   return(cpd.settings[UO_nl_before_synchronized].a);
}


iarf_e &nl_after_synchronized()
{
   return(cpd.settings[UO_nl_after_synchronized].a);
}


iarf_e &nl_before_do()
{
   return(cpd.settings[UO_nl_before_do].a);
}


iarf_e &nl_after_do()
{
   return(cpd.settings[UO_nl_after_do].a);
}


bool &nl_ds_struct_enum_cmt()
{
   return(cpd.settings[UO_nl_ds_struct_enum_cmt].b);
}


bool &nl_ds_struct_enum_close_brace()
{
   return(cpd.settings[UO_nl_ds_struct_enum_close_brace].b);
}


size_t &nl_before_func_class_def()
{
   return(cpd.settings[UO_nl_before_func_class_def].u);
}


size_t &nl_before_func_class_proto()
{
   return(cpd.settings[UO_nl_before_func_class_proto].u);
}


iarf_e &nl_class_colon()
{
   return(cpd.settings[UO_nl_class_colon].a);
}


iarf_e &nl_constr_colon()
{
   return(cpd.settings[UO_nl_constr_colon].a);
}


bool &nl_namespace_two_to_one_liner()
{
   return(cpd.settings[UO_nl_namespace_two_to_one_liner].b);
}


bool &nl_create_if_one_liner()
{
   return(cpd.settings[UO_nl_create_if_one_liner].b);
}


bool &nl_create_for_one_liner()
{
   return(cpd.settings[UO_nl_create_for_one_liner].b);
}


bool &nl_create_while_one_liner()
{
   return(cpd.settings[UO_nl_create_while_one_liner].b);
}


bool &nl_create_func_def_one_liner()
{
   return(cpd.settings[UO_nl_create_func_def_one_liner].b);
}


bool &nl_split_if_one_liner()
{
   return(cpd.settings[UO_nl_split_if_one_liner].b);
}


bool &nl_split_for_one_liner()
{
   return(cpd.settings[UO_nl_split_for_one_liner].b);
}


bool &nl_split_while_one_liner()
{
   return(cpd.settings[UO_nl_split_while_one_liner].b);
}


size_t &nl_max()
{
   return(cpd.settings[UO_nl_max].u);
}


size_t &nl_max_blank_in_func()
{
   return(cpd.settings[UO_nl_max_blank_in_func].u);
}


size_t &nl_after_func_proto()
{
   return(cpd.settings[UO_nl_after_func_proto].u);
}


size_t &nl_after_func_proto_group()
{
   return(cpd.settings[UO_nl_after_func_proto_group].u);
}


size_t &nl_after_func_class_proto()
{
   return(cpd.settings[UO_nl_after_func_class_proto].u);
}


size_t &nl_after_func_class_proto_group()
{
   return(cpd.settings[UO_nl_after_func_class_proto_group].u);
}


size_t &nl_before_func_body_def()
{
   return(cpd.settings[UO_nl_before_func_body_def].u);
}


size_t &nl_before_func_body_proto()
{
   return(cpd.settings[UO_nl_before_func_body_proto].u);
}


size_t &nl_after_func_body()
{
   return(cpd.settings[UO_nl_after_func_body].u);
}


size_t &nl_after_func_body_class()
{
   return(cpd.settings[UO_nl_after_func_body_class].u);
}


size_t &nl_after_func_body_one_liner()
{
   return(cpd.settings[UO_nl_after_func_body_one_liner].u);
}


size_t &nl_before_block_comment()
{
   return(cpd.settings[UO_nl_before_block_comment].u);
}


size_t &nl_before_c_comment()
{
   return(cpd.settings[UO_nl_before_c_comment].u);
}


size_t &nl_before_cpp_comment()
{
   return(cpd.settings[UO_nl_before_cpp_comment].u);
}


bool &nl_after_multiline_comment()
{
   return(cpd.settings[UO_nl_after_multiline_comment].b);
}


bool &nl_after_label_colon()
{
   return(cpd.settings[UO_nl_after_label_colon].b);
}


size_t &nl_after_struct()
{
   return(cpd.settings[UO_nl_after_struct].u);
}


size_t &nl_before_class()
{
   return(cpd.settings[UO_nl_before_class].u);
}


size_t &nl_after_class()
{
   return(cpd.settings[UO_nl_after_class].u);
}


size_t &nl_before_access_spec()
{
   return(cpd.settings[UO_nl_before_access_spec].u);
}


size_t &nl_after_access_spec()
{
   return(cpd.settings[UO_nl_after_access_spec].u);
}


size_t &nl_comment_func_def()
{
   return(cpd.settings[UO_nl_comment_func_def].u);
}


size_t &nl_after_try_catch_finally()
{
   return(cpd.settings[UO_nl_after_try_catch_finally].u);
}


size_t &nl_around_cs_property()
{
   return(cpd.settings[UO_nl_around_cs_property].u);
}


size_t &nl_between_get_set()
{
   return(cpd.settings[UO_nl_between_get_set].u);
}


iarf_e &nl_property_brace()
{
   return(cpd.settings[UO_nl_property_brace].a);
}


bool &eat_blanks_after_open_brace()
{
   return(cpd.settings[UO_eat_blanks_after_open_brace].b);
}


bool &eat_blanks_before_close_brace()
{
   return(cpd.settings[UO_eat_blanks_before_close_brace].b);
}


size_t &nl_remove_extra_newlines()
{
   return(cpd.settings[UO_nl_remove_extra_newlines].u);
}


bool &nl_before_return()
{
   return(cpd.settings[UO_nl_before_return].b);
}


bool &nl_after_return()
{
   return(cpd.settings[UO_nl_after_return].b);
}


iarf_e &nl_after_annotation()
{
   return(cpd.settings[UO_nl_after_annotation].a);
}


iarf_e &nl_between_annotation()
{
   return(cpd.settings[UO_nl_between_annotation].a);
}


tokenpos_e &pos_arith()
{
   return(cpd.settings[UO_pos_arith].tp);
}


tokenpos_e &pos_assign()
{
   return(cpd.settings[UO_pos_assign].tp);
}


tokenpos_e &pos_bool()
{
   return(cpd.settings[UO_pos_bool].tp);
}


tokenpos_e &pos_compare()
{
   return(cpd.settings[UO_pos_compare].tp);
}


tokenpos_e &pos_conditional()
{
   return(cpd.settings[UO_pos_conditional].tp);
}


tokenpos_e &pos_comma()
{
   return(cpd.settings[UO_pos_comma].tp);
}


tokenpos_e &pos_enum_comma()
{
   return(cpd.settings[UO_pos_enum_comma].tp);
}


tokenpos_e &pos_class_comma()
{
   return(cpd.settings[UO_pos_class_comma].tp);
}


tokenpos_e &pos_constr_comma()
{
   return(cpd.settings[UO_pos_constr_comma].tp);
}


tokenpos_e &pos_class_colon()
{
   return(cpd.settings[UO_pos_class_colon].tp);
}


tokenpos_e &pos_constr_colon()
{
   return(cpd.settings[UO_pos_constr_colon].tp);
}


size_t &code_width()
{
   return(cpd.settings[UO_code_width].u);
}


bool &ls_for_split_full()
{
   return(cpd.settings[UO_ls_for_split_full].b);
}


bool &ls_func_split_full()
{
   return(cpd.settings[UO_ls_func_split_full].b);
}


bool &ls_code_width()
{
   return(cpd.settings[UO_ls_code_width].b);
}


bool &align_keep_tabs()
{
   return(cpd.settings[UO_align_keep_tabs].b);
}


bool &align_with_tabs()
{
   return(cpd.settings[UO_align_with_tabs].b);
}


bool &align_on_tabstop()
{
   return(cpd.settings[UO_align_on_tabstop].b);
}


bool &align_number_right()
{
   return(cpd.settings[UO_align_number_right].b);
}


bool &align_keep_extra_space()
{
   return(cpd.settings[UO_align_keep_extra_space].b);
}


bool &align_func_params()
{
   return(cpd.settings[UO_align_func_params].b);
}


size_t &align_func_params_span()
{
   return(cpd.settings[UO_align_func_params_span].u);
}


size_t &align_func_params_thresh()
{
   return(cpd.settings[UO_align_func_params_thresh].u);
}


size_t &align_func_params_gap()
{
   return(cpd.settings[UO_align_func_params_gap].u);
}


bool &align_same_func_call_params()
{
   return(cpd.settings[UO_align_same_func_call_params].b);
}


size_t &align_var_def_span()
{
   return(cpd.settings[UO_align_var_def_span].u);
}


size_t &align_var_def_star_style()
{
   return(cpd.settings[UO_align_var_def_star_style].u);
}


size_t &align_var_def_amp_style()
{
   return(cpd.settings[UO_align_var_def_amp_style].u);
}


size_t &align_var_def_thresh()
{
   return(cpd.settings[UO_align_var_def_thresh].u);
}


size_t &align_var_def_gap()
{
   return(cpd.settings[UO_align_var_def_gap].u);
}


bool &align_var_def_colon()
{
   return(cpd.settings[UO_align_var_def_colon].b);
}


size_t &align_var_def_colon_gap()
{
   return(cpd.settings[UO_align_var_def_colon_gap].u);
}


bool &align_var_def_attribute()
{
   return(cpd.settings[UO_align_var_def_attribute].b);
}


bool &align_var_def_inline()
{
   return(cpd.settings[UO_align_var_def_inline].b);
}


size_t &align_assign_span()
{
   return(cpd.settings[UO_align_assign_span].u);
}


size_t &align_assign_thresh()
{
   return(cpd.settings[UO_align_assign_thresh].u);
}


size_t &align_assign_decl_func()
{
   return(cpd.settings[UO_align_assign_decl_func].u);
}


size_t &align_enum_equ_span()
{
   return(cpd.settings[UO_align_enum_equ_span].u);
}


size_t &align_enum_equ_thresh()
{
   return(cpd.settings[UO_align_enum_equ_thresh].u);
}


size_t &align_var_class_span()
{
   return(cpd.settings[UO_align_var_class_span].u);
}


size_t &align_var_class_thresh()
{
   return(cpd.settings[UO_align_var_class_thresh].u);
}


size_t &align_var_class_gap()
{
   return(cpd.settings[UO_align_var_class_gap].u);
}


size_t &align_var_struct_span()
{
   return(cpd.settings[UO_align_var_struct_span].u);
}


size_t &align_var_struct_thresh()
{
   return(cpd.settings[UO_align_var_struct_thresh].u);
}


size_t &align_var_struct_gap()
{
   return(cpd.settings[UO_align_var_struct_gap].u);
}


size_t &align_struct_init_span()
{
   return(cpd.settings[UO_align_struct_init_span].u);
}


size_t &align_typedef_gap()
{
   return(cpd.settings[UO_align_typedef_gap].u);
}


size_t &align_typedef_span()
{
   return(cpd.settings[UO_align_typedef_span].u);
}


size_t &align_typedef_func()
{
   return(cpd.settings[UO_align_typedef_func].u);
}


size_t &align_typedef_star_style()
{
   return(cpd.settings[UO_align_typedef_star_style].u);
}


size_t &align_typedef_amp_style()
{
   return(cpd.settings[UO_align_typedef_amp_style].u);
}


size_t &align_right_cmt_span()
{
   return(cpd.settings[UO_align_right_cmt_span].u);
}


bool &align_right_cmt_mix()
{
   return(cpd.settings[UO_align_right_cmt_mix].b);
}


bool &align_right_cmt_same_level()
{
   return(cpd.settings[UO_align_right_cmt_same_level].b);
}


size_t &align_right_cmt_gap()
{
   return(cpd.settings[UO_align_right_cmt_gap].u);
}


size_t &align_right_cmt_at_col()
{
   return(cpd.settings[UO_align_right_cmt_at_col].u);
}


size_t &align_func_proto_span()
{
   return(cpd.settings[UO_align_func_proto_span].u);
}


size_t &align_func_proto_gap()
{
   return(cpd.settings[UO_align_func_proto_gap].u);
}


bool &align_on_operator()
{
   return(cpd.settings[UO_align_on_operator].b);
}


bool &align_mix_var_proto()
{
   return(cpd.settings[UO_align_mix_var_proto].b);
}


bool &align_single_line_func()
{
   return(cpd.settings[UO_align_single_line_func].b);
}


bool &align_single_line_brace()
{
   return(cpd.settings[UO_align_single_line_brace].b);
}


size_t &align_single_line_brace_gap()
{
   return(cpd.settings[UO_align_single_line_brace_gap].u);
}


size_t &align_oc_msg_spec_span()
{
   return(cpd.settings[UO_align_oc_msg_spec_span].u);
}


bool &align_nl_cont()
{
   return(cpd.settings[UO_align_nl_cont].b);
}


bool &align_pp_define_together()
{
   return(cpd.settings[UO_align_pp_define_together].b);
}


size_t &align_pp_define_gap()
{
   return(cpd.settings[UO_align_pp_define_gap].u);
}


size_t &align_pp_define_span()
{
   return(cpd.settings[UO_align_pp_define_span].u);
}


bool &align_left_shift()
{
   return(cpd.settings[UO_align_left_shift].b);
}


bool &align_asm_colon()
{
   return(cpd.settings[UO_align_asm_colon].b);
}


size_t &align_oc_msg_colon_span()
{
   return(cpd.settings[UO_align_oc_msg_colon_span].u);
}


bool &align_oc_msg_colon_first()
{
   return(cpd.settings[UO_align_oc_msg_colon_first].b);
}


bool &align_oc_decl_colon()
{
   return(cpd.settings[UO_align_oc_decl_colon].b);
}


size_t &cmt_width()
{
   return(cpd.settings[UO_cmt_width].u);
}


size_t &cmt_reflow_mode()
{
   return(cpd.settings[UO_cmt_reflow_mode].u);
}


bool &cmt_convert_tab_to_spaces()
{
   return(cpd.settings[UO_cmt_convert_tab_to_spaces].b);
}


bool &cmt_indent_multi()
{
   return(cpd.settings[UO_cmt_indent_multi].b);
}


bool &cmt_c_group()
{
   return(cpd.settings[UO_cmt_c_group].b);
}


bool &cmt_c_nl_start()
{
   return(cpd.settings[UO_cmt_c_nl_start].b);
}


bool &cmt_c_nl_end()
{
   return(cpd.settings[UO_cmt_c_nl_end].b);
}


bool &cmt_cpp_group()
{
   return(cpd.settings[UO_cmt_cpp_group].b);
}


bool &cmt_cpp_nl_start()
{
   return(cpd.settings[UO_cmt_cpp_nl_start].b);
}


bool &cmt_cpp_nl_end()
{
   return(cpd.settings[UO_cmt_cpp_nl_end].b);
}


bool &cmt_cpp_to_c()
{
   return(cpd.settings[UO_cmt_cpp_to_c].b);
}


bool &cmt_star_cont()
{
   return(cpd.settings[UO_cmt_star_cont].b);
}


size_t &cmt_sp_before_star_cont()
{
   return(cpd.settings[UO_cmt_sp_before_star_cont].u);
}


int &cmt_sp_after_star_cont()
{
   return(cpd.settings[UO_cmt_sp_after_star_cont].n);
}


bool &cmt_multi_check_last()
{
   return(cpd.settings[UO_cmt_multi_check_last].b);
}


size_t &cmt_multi_first_len_minimum()
{
   return(cpd.settings[UO_cmt_multi_first_len_minimum].u);
}


std::string cmt_insert_file_header()
{
   if (cpd.settings[UO_cmt_insert_file_header].str)
   {
      return(cpd.settings[UO_cmt_insert_file_header].str);
   }
   return(std::string{});
}


std::string cmt_insert_file_footer()
{
   if (cpd.settings[UO_cmt_insert_file_footer].str)
   {
      return(cpd.settings[UO_cmt_insert_file_footer].str);
   }
   return(std::string{});
}


std::string cmt_insert_func_header()
{
   if (cpd.settings[UO_cmt_insert_func_header].str)
   {
      return(cpd.settings[UO_cmt_insert_func_header].str);
   }
   return(std::string{});
}


std::string cmt_insert_class_header()
{
   if (cpd.settings[UO_cmt_insert_class_header].str)
   {
      return(cpd.settings[UO_cmt_insert_class_header].str);
   }
   return(std::string{});
}


std::string cmt_insert_oc_msg_header()
{
   if (cpd.settings[UO_cmt_insert_oc_msg_header].str)
   {
      return(cpd.settings[UO_cmt_insert_oc_msg_header].str);
   }
   return(std::string{});
}


bool &cmt_insert_before_preproc()
{
   return(cpd.settings[UO_cmt_insert_before_preproc].b);
}


bool &cmt_insert_before_inlines()
{
   return(cpd.settings[UO_cmt_insert_before_inlines].b);
}


bool &cmt_insert_before_ctor_dtor()
{
   return(cpd.settings[UO_cmt_insert_before_ctor_dtor].b);
}


iarf_e &mod_full_brace_do()
{
   return(cpd.settings[UO_mod_full_brace_do].a);
}


iarf_e &mod_full_brace_for()
{
   return(cpd.settings[UO_mod_full_brace_for].a);
}


iarf_e &mod_full_brace_function()
{
   return(cpd.settings[UO_mod_full_brace_function].a);
}


iarf_e &mod_full_brace_if()
{
   return(cpd.settings[UO_mod_full_brace_if].a);
}


bool &mod_full_brace_if_chain()
{
   return(cpd.settings[UO_mod_full_brace_if_chain].b);
}


bool &mod_full_brace_if_chain_only()
{
   return(cpd.settings[UO_mod_full_brace_if_chain_only].b);
}


size_t &mod_full_brace_nl()
{
   return(cpd.settings[UO_mod_full_brace_nl].u);
}


bool &mod_full_brace_nl_block_rem_mlcond()
{
   return(cpd.settings[UO_mod_full_brace_nl_block_rem_mlcond].b);
}


iarf_e &mod_full_brace_while()
{
   return(cpd.settings[UO_mod_full_brace_while].a);
}


iarf_e &mod_full_brace_using()
{
   return(cpd.settings[UO_mod_full_brace_using].a);
}


iarf_e &mod_paren_on_return()
{
   return(cpd.settings[UO_mod_paren_on_return].a);
}


bool &mod_pawn_semicolon()
{
   return(cpd.settings[UO_mod_pawn_semicolon].b);
}


bool &mod_full_paren_if_bool()
{
   return(cpd.settings[UO_mod_full_paren_if_bool].b);
}


bool &mod_remove_extra_semicolon()
{
   return(cpd.settings[UO_mod_remove_extra_semicolon].b);
}


size_t &mod_add_long_function_closebrace_comment()
{
   return(cpd.settings[UO_mod_add_long_function_closebrace_comment].u);
}


size_t &mod_add_long_namespace_closebrace_comment()
{
   return(cpd.settings[UO_mod_add_long_namespace_closebrace_comment].u);
}


size_t &mod_add_long_class_closebrace_comment()
{
   return(cpd.settings[UO_mod_add_long_class_closebrace_comment].u);
}


size_t &mod_add_long_switch_closebrace_comment()
{
   return(cpd.settings[UO_mod_add_long_switch_closebrace_comment].u);
}


size_t &mod_add_long_ifdef_endif_comment()
{
   return(cpd.settings[UO_mod_add_long_ifdef_endif_comment].u);
}


size_t &mod_add_long_ifdef_else_comment()
{
   return(cpd.settings[UO_mod_add_long_ifdef_else_comment].u);
}


bool &mod_sort_import()
{
   return(cpd.settings[UO_mod_sort_import].b);
}


bool &mod_sort_using()
{
   return(cpd.settings[UO_mod_sort_using].b);
}


bool &mod_sort_include()
{
   return(cpd.settings[UO_mod_sort_include].b);
}


bool &mod_move_case_break()
{
   return(cpd.settings[UO_mod_move_case_break].b);
}


iarf_e &mod_case_brace()
{
   return(cpd.settings[UO_mod_case_brace].a);
}


bool &mod_remove_empty_return()
{
   return(cpd.settings[UO_mod_remove_empty_return].b);
}


bool &mod_sort_oc_properties()
{
   return(cpd.settings[UO_mod_sort_oc_properties].b);
}


int &mod_sort_oc_property_class_weight()
{
   return(cpd.settings[UO_mod_sort_oc_property_class_weight].n);
}


int &mod_sort_oc_property_thread_safe_weight()
{
   return(cpd.settings[UO_mod_sort_oc_property_thread_safe_weight].n);
}


int &mod_sort_oc_property_readwrite_weight()
{
   return(cpd.settings[UO_mod_sort_oc_property_readwrite_weight].n);
}


int &mod_sort_oc_property_reference_weight()
{
   return(cpd.settings[UO_mod_sort_oc_property_reference_weight].n);
}


int &mod_sort_oc_property_getter_weight()
{
   return(cpd.settings[UO_mod_sort_oc_property_getter_weight].n);
}


int &mod_sort_oc_property_setter_weight()
{
   return(cpd.settings[UO_mod_sort_oc_property_setter_weight].n);
}


int &mod_sort_oc_property_nullability_weight()
{
   return(cpd.settings[UO_mod_sort_oc_property_nullability_weight].n);
}


iarf_e &mod_enum_last_comma()
{
   return(cpd.settings[UO_mod_enum_last_comma].a);
}


iarf_e &pp_indent()
{
   return(cpd.settings[UO_pp_indent].a);
}


bool &pp_indent_at_level()
{
   return(cpd.settings[UO_pp_indent_at_level].b);
}


size_t &pp_indent_count()
{
   return(cpd.settings[UO_pp_indent_count].u);
}


iarf_e &pp_space()
{
   return(cpd.settings[UO_pp_space].a);
}


size_t &pp_space_count()
{
   return(cpd.settings[UO_pp_space_count].u);
}


int &pp_indent_region()
{
   return(cpd.settings[UO_pp_indent_region].n);
}


bool &pp_region_indent_code()
{
   return(cpd.settings[UO_pp_region_indent_code].b);
}


int &pp_indent_if()
{
   return(cpd.settings[UO_pp_indent_if].n);
}


bool &pp_if_indent_code()
{
   return(cpd.settings[UO_pp_if_indent_code].b);
}


bool &pp_define_at_level()
{
   return(cpd.settings[UO_pp_define_at_level].b);
}


bool &pp_ignore_define_body()
{
   return(cpd.settings[UO_pp_ignore_define_body].b);
}


bool &pp_indent_case()
{
   return(cpd.settings[UO_pp_indent_case].b);
}


bool &pp_indent_func_def()
{
   return(cpd.settings[UO_pp_indent_func_def].b);
}


bool &pp_indent_extern()
{
   return(cpd.settings[UO_pp_indent_extern].b);
}


bool &pp_indent_brace()
{
   return(cpd.settings[UO_pp_indent_brace].b);
}


std::string include_category_0()
{
   if (cpd.settings[UO_include_category_0].str)
   {
      return(cpd.settings[UO_include_category_0].str);
   }
   return(std::string{});
}


std::string include_category_1()
{
   if (cpd.settings[UO_include_category_1].str)
   {
      return(cpd.settings[UO_include_category_1].str);
   }
   return(std::string{});
}


std::string include_category_2()
{
   if (cpd.settings[UO_include_category_2].str)
   {
      return(cpd.settings[UO_include_category_2].str);
   }
   return(std::string{});
}


bool &use_indent_func_call_param()
{
   return(cpd.settings[UO_use_indent_func_call_param].b);
}


bool &use_indent_continue_only_once()
{
   return(cpd.settings[UO_use_indent_continue_only_once].b);
}


bool &indent_cpp_lambda_only_once()
{
   return(cpd.settings[UO_indent_cpp_lambda_only_once].b);
}


bool &use_options_overriding_for_qt_macros()
{
   return(cpd.settings[UO_use_options_overriding_for_qt_macros].b);
}


size_t &warn_level_tabs_found_in_verbatim_string_literals()
{
   return(cpd.settings[UO_warn_level_tabs_found_in_verbatim_string_literals].u);
}
} // namespace option
} // namespace uncrustify
