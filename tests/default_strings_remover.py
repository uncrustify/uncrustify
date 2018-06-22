#!/usr/bin/python
from __future__ import print_function  # python >= 2.6
from glob import glob
from sys import argv
import editdistance
from re import search

STRINGS_FILE_PATH = "default_strings.txt"
MATCH_MAX_DELTA_PERCENT = 5
# OR
MATCH_MAX_DELTA_CHARS = 3

OPTIONS_LIST = [
    "newlines", "input_tab_size", "output_tab_size", "string_escape_char",
    "string_escape_char2", "string_replace_tab_chars", "tok_split_gte",
    "disable_processing_cmt", "enable_processing_cmt", "enable_digraphs",
    "utf8_bom", "utf8_byte", "utf8_force", "sp_arith", "sp_arith_additive",
    "sp_assign", "sp_cpp_lambda_assign", "sp_cpp_lambda_paren",
    "sp_assign_default", "sp_before_assign", "sp_after_assign", "sp_enum_paren",
    "sp_enum_assign", "sp_enum_before_assign", "sp_enum_after_assign",
    "sp_enum_colon", "sp_pp_concat", "sp_pp_stringify",
    "sp_before_pp_stringify", "sp_bool", "sp_compare", "sp_inside_paren",
    "sp_paren_paren", "sp_cparen_oparen", "sp_balance_nested_parens",
    "sp_paren_brace", "sp_before_ptr_star", "sp_before_unnamed_ptr_star",
    "sp_between_ptr_star", "sp_after_ptr_star", "sp_after_ptr_star_qualifier",
    "sp_after_ptr_star_func", "sp_ptr_star_paren", "sp_before_ptr_star_func",
    "sp_before_byref", "sp_before_unnamed_byref", "sp_after_byref",
    "sp_after_byref_func", "sp_before_byref_func", "sp_after_type",
    "sp_before_template_paren", "sp_template_angle", "sp_before_angle",
    "sp_inside_angle", "sp_angle_colon", "sp_after_angle", "sp_angle_paren",
    "sp_angle_paren_empty",
    "sp_angle_word",
    "sp_angle_shift",
    "sp_permit_cpp11_shift",
    "sp_before_sparen",
    "sp_inside_sparen",
    "sp_inside_sparen_close",
    "sp_inside_sparen_open",
    "sp_after_sparen",
    "sp_sparen_brace",
    "sp_invariant_paren",
    "sp_after_invariant_paren",
    "sp_special_semi",
    "sp_before_semi",
    "sp_before_semi_for",
    "sp_before_semi_for_empty",
    "sp_after_semi",
    "sp_after_semi_for",
    "sp_after_semi_for_empty",
    "sp_before_square",
    "sp_before_squares",
    "sp_inside_square",
    "sp_after_comma",
    "sp_before_comma",
    "sp_after_mdatype_commas",
    "sp_before_mdatype_commas",
    "sp_between_mdatype_commas",
    "sp_paren_comma",
    "sp_before_ellipsis",
    "sp_after_class_colon",
    "sp_before_class_colon",
    "sp_after_constr_colon",
    "sp_before_constr_colon",
    "sp_before_case_colon",
    "sp_after_operator",
    "sp_after_operator_sym",
    "sp_after_operator_sym_empty",
    "sp_after_cast",
    "sp_inside_paren_cast",
    "sp_cpp_cast_paren",
    "sp_sizeof_paren",
    "sp_after_tag",
    "sp_inside_braces_enum",
    "sp_inside_braces_struct",
    "sp_after_type_brace_init_lst_open",
    "sp_before_type_brace_init_lst_close",
    "sp_inside_type_brace_init_lst",
    "sp_inside_braces",
    "sp_inside_braces_empty",
    "sp_type_func",
    "sp_type_brace_init_lst",
    "sp_func_proto_paren",
    "sp_func_proto_paren_empty",
    "sp_func_def_paren",
    "sp_func_def_paren_empty",
    "sp_inside_fparens",
    "sp_inside_fparen",
    "sp_inside_tparen",
    "sp_after_tparen_close",
    "sp_square_fparen",
    "sp_fparen_brace",
    "sp_fparen_dbrace",
    "sp_func_call_paren",
    "sp_func_call_paren_empty",
    "sp_func_call_user_paren",
    "sp_func_class_paren",
    "sp_func_class_paren_empty",
    "sp_return_paren",
    "sp_attribute_paren",
    "sp_defined_paren",
    "sp_throw_paren",
    "sp_after_throw",
    "sp_catch_paren",
    "sp_version_paren",
    "sp_scope_paren",
    "sp_super_paren",
    "sp_this_paren",
    "sp_macro",
    "sp_macro_func",
    "sp_else_brace",
    "sp_brace_else",
    "sp_brace_typedef",
    "sp_catch_brace",
    "sp_brace_catch",
    "sp_finally_brace",
    "sp_brace_finally",
    "sp_try_brace",
    "sp_getset_brace",
    "sp_word_brace",
    "sp_word_brace_ns",
    "sp_before_dc",
    "sp_after_dc",
    "sp_d_array_colon",
    "sp_not",
    "sp_inv",
    "sp_addr",
    "sp_member",
    "sp_deref",
    "sp_sign",
    "sp_incdec",
    "sp_before_nl_cont",
    "sp_after_oc_scope",
    "sp_after_oc_colon",
    "sp_before_oc_colon",
    "sp_after_oc_dict_colon",
    "sp_before_oc_dict_colon",
    "sp_after_send_oc_colon",
    "sp_before_send_oc_colon",
    "sp_after_oc_type",
    "sp_after_oc_return_type",
    "sp_after_oc_at_sel",
    "sp_after_oc_at_sel_parens",
    "sp_inside_oc_at_sel_parens",
    "sp_before_oc_block_caret",
    "sp_after_oc_block_caret",
    "sp_after_oc_msg_receiver",
    "sp_after_oc_property",
    "sp_cond_colon",
    "sp_cond_colon_before",
    "sp_cond_colon_after",
    "sp_cond_question",
    "sp_cond_question_before",
    "sp_cond_question_after",
    "sp_cond_ternary_short",
    "sp_case_label",
    "sp_range",
    "sp_after_for_colon",
    "sp_before_for_colon",
    "sp_extern_paren",
    "sp_cmt_cpp_start",
    "sp_cmt_cpp_doxygen",
    "sp_cmt_cpp_qttr",
    "sp_endif_cmt",
    "sp_after_new",
    "sp_between_new_paren",
    "sp_after_newop_paren",
    "sp_inside_newop_paren",
    "sp_inside_newop_paren_open",
    "sp_inside_newop_paren_close",
    "sp_before_tr_emb_cmt",
    "sp_num_before_tr_emb_cmt",
    "sp_annotation_paren",
    "sp_skip_vbrace_tokens",
    "force_tab_after_define",
    "indent_columns",
    "indent_continue",
    "indent_param",
    "indent_with_tabs",
    "indent_cmt_with_tabs",
    "indent_align_string",
    "indent_xml_string",
    "indent_brace",
    "indent_braces",
    "indent_braces_no_func",
    "indent_braces_no_class",
    "indent_braces_no_struct",
    "indent_brace_parent",
    "indent_paren_open_brace",
    "indent_cs_delegate_brace",
    "indent_namespace",
    "indent_namespace_single_indent",
    "indent_namespace_level",
    "indent_namespace_limit",
    "indent_extern",
    "indent_class",
    "indent_class_colon",
    "indent_class_on_colon",
    "indent_constr_colon",
    "indent_ctor_init_leading",
    "indent_ctor_init",
    "indent_else_if",
    "indent_var_def_blk",
    "indent_var_def_cont",
    "indent_shift",
    "indent_func_def_force_col1",
    "indent_func_call_param",
    "indent_func_def_param",
    "indent_func_proto_param",
    "indent_func_class_param",
    "indent_func_ctor_var_param",
    "indent_template_param",
    "indent_func_param_double",
    "indent_func_const",
    "indent_func_throw",
    "indent_member",
    "indent_sing_line_comments",
    "indent_relative_single_line_comments",
    "indent_switch_case",
    "indent_switch_pp",
    "indent_case_shift",
    "indent_case_brace",
    "indent_col1_comment",
    "indent_label",
    "indent_access_spec",
    "indent_access_spec_body",
    "indent_paren_nl",
    "indent_paren_close",
    "indent_paren_after_func_def",
    "indent_paren_after_func_decl",
    "indent_paren_after_func_call",
    "indent_comma_paren",
    "indent_bool_paren",
    "indent_first_bool_expr",
    "indent_square_nl",
    "indent_preserve_sql",
    "indent_align_assign",
    "indent_oc_block",
    "indent_oc_block_msg",
    "indent_oc_msg_colon",
    "indent_oc_msg_prioritize_first_colon",
    "indent_oc_block_msg_xcode_style",
    "indent_oc_block_msg_from_keyword",
    "indent_oc_block_msg_from_colon",
    "indent_oc_block_msg_from_caret",
    "indent_oc_block_msg_from_brace",
    "indent_min_vbrace_open",
    "indent_vbrace_open_on_tabstop",
    "indent_token_after_brace",
    "indent_cpp_lambda_body",
    "indent_using_block",
    "indent_ternary_operator",
    "nl_collapse_empty_body",
    "nl_assign_leave_one_liners",
    "nl_class_leave_one_liners",
    "nl_enum_leave_one_liners",
    "nl_getset_leave_one_liners",
    "nl_func_leave_one_liners",
    "nl_cpp_lambda_leave_one_liners",
    "nl_if_leave_one_liners",
    "nl_while_leave_one_liners",
    "nl_oc_msg_leave_one_liner",
    "nl_oc_block_brace",
    "nl_start_of_file",
    "nl_start_of_file_min",
    "nl_end_of_file",
    "nl_end_of_file_min",
    "nl_assign_brace",
    "nl_assign_square",
    "nl_after_square_assign",
    "nl_func_var_def_blk",
    "nl_typedef_blk_start",
    "nl_typedef_blk_end",
    "nl_typedef_blk_in",
    "nl_var_def_blk_start",
    "nl_var_def_blk_end",
    "nl_var_def_blk_in",
    "nl_fcall_brace",
    "nl_enum_brace",
    "nl_enum_class",
    "nl_enum_class_identifier",
    "nl_enum_identifier_colon",
    "nl_enum_colon_type",
    "nl_struct_brace",
    "nl_union_brace",
    "nl_if_brace",
    "nl_brace_else",
    "nl_elseif_brace",
    "nl_else_brace",
    "nl_else_if",
    "nl_before_if_closing_paren",
    "nl_brace_finally",
    "nl_finally_brace",
    "nl_try_brace",
    "nl_getset_brace",
    "nl_for_brace",
    "nl_catch_brace",
    "nl_brace_catch",
    "nl_brace_square",
    "nl_brace_fparen",
    "nl_while_brace",
    "nl_scope_brace",
    "nl_unittest_brace",
    "nl_version_brace",
    "nl_using_brace",
    "nl_brace_brace",
    "nl_do_brace",
    "nl_brace_while",
    "nl_switch_brace",
    "nl_synchronized_brace",
    "nl_multi_line_cond",
    "nl_multi_line_define",
    "nl_before_case",
    "nl_before_throw",
    "nl_after_case",
    "nl_case_colon_brace",
    "nl_namespace_brace",
    "nl_template_class",
    "nl_class_brace",
    "nl_class_init_args",
    "nl_constr_init_args",
    "nl_enum_own_lines",
    "nl_func_type_name",
    "nl_func_type_name_class",
    "nl_func_class_scope",
    "nl_func_scope_name",
    "nl_func_proto_type_name",
    "nl_func_paren",
    "nl_func_paren_empty",
    "nl_func_def_paren",
    "nl_func_def_paren_empty",
    "nl_func_call_paren",
    "nl_func_call_paren_empty",
    "nl_func_decl_start",
    "nl_func_def_start",
    "nl_func_decl_start_single",
    "nl_func_def_start_single",
    "nl_func_decl_start_multi_line",
    "nl_func_def_start_multi_line",
    "nl_func_decl_args",
    "nl_func_def_args",
    "nl_func_decl_args_multi_line",
    "nl_func_def_args_multi_line",
    "nl_func_decl_end",
    "nl_func_def_end",
    "nl_func_decl_end_single",
    "nl_func_def_end_single",
    "nl_func_decl_end_multi_line",
    "nl_func_def_end_multi_line",
    "nl_func_decl_empty",
    "nl_func_def_empty",
    "nl_func_call_empty",
    "nl_func_call_start_multi_line",
    "nl_func_call_args_multi_line",
    "nl_func_call_end_multi_line",
    "nl_oc_msg_args",
    "nl_fdef_brace",
    "nl_cpp_ldef_brace",
    "nl_return_expr",
    "nl_after_semicolon",
    "nl_paren_dbrace_open",
    "nl_type_brace_init_lst",
    "nl_type_brace_init_lst_open",
    "nl_type_brace_init_lst_close",
    "nl_after_brace_open",
    "nl_after_brace_open_cmt",
    "nl_after_vbrace_open",
    "nl_after_vbrace_open_empty",
    "nl_after_brace_close",
    "nl_after_vbrace_close",
    "nl_brace_struct_var",
    "nl_define_macro",
    "nl_squeeze_ifdef",
    "nl_squeeze_ifdef_top_level",
    "nl_before_if",
    "nl_after_if",
    "nl_before_for",
    "nl_after_for",
    "nl_before_while",
    "nl_after_while",
    "nl_before_switch",
    "nl_after_switch",
    "nl_before_synchronized",
    "nl_after_synchronized",
    "nl_before_do",
    "nl_after_do",
    "nl_ds_struct_enum_cmt",
    "nl_ds_struct_enum_close_brace",
    "nl_before_func_class_def",
    "nl_before_func_class_proto",
    "nl_class_colon",
    "nl_constr_colon",
    "nl_create_if_one_liner",
    "nl_create_for_one_liner",
    "nl_create_while_one_liner",
    "nl_split_if_one_liner",
    "nl_split_for_one_liner",
    "nl_split_while_one_liner",
    "nl_max",
    "nl_max_blank_in_func",
    "nl_after_func_proto",
    "nl_after_func_proto_group",
    "nl_after_func_class_proto",
    "nl_after_func_class_proto_group",
    "nl_before_func_body_def",
    "nl_before_func_body_proto",
    "nl_after_func_body",
    "nl_after_func_body_class",
    "nl_after_func_body_one_liner",
    "nl_before_block_comment",
    "nl_before_c_comment",
    "nl_before_cpp_comment",
    "nl_after_multiline_comment",
    "nl_after_label_colon",
    "nl_after_struct",
    "nl_before_class",
    "nl_after_class",
    "nl_before_access_spec",
    "nl_after_access_spec",
    "nl_comment_func_def",
    "nl_after_try_catch_finally",
    "nl_around_cs_property",
    "nl_between_get_set",
    "nl_property_brace",
    "eat_blanks_after_open_brace",
    "eat_blanks_before_close_brace",
    "nl_remove_extra_newlines",
    "nl_before_return",
    "nl_after_return",
    "nl_after_annotation",
    "nl_between_annotation",
    "pos_arith",
    "pos_assign",
    "pos_bool",
    "pos_compare",
    "pos_conditional",
    "pos_comma",
    "pos_enum_comma",
    "pos_class_comma",
    "pos_constr_comma",
    "pos_class_colon",
    "pos_constr_colon",
    "code_width",
    "ls_for_split_full",
    "ls_func_split_full",
    "ls_code_width",
    "align_keep_tabs",
    "align_with_tabs",
    "align_on_tabstop",
    "align_number_right",
    "align_keep_extra_space",
    "align_func_params",
    "align_func_params_span",
    "align_func_params_thresh",
    "align_func_params_gap",
    "align_same_func_call_params",
    "align_var_def_span",
    "align_var_def_star_style",
    "align_var_def_amp_style",
    "align_var_def_thresh",
    "align_var_def_gap",
    "align_var_def_colon",
    "align_var_def_colon_gap",
    "align_var_def_attribute",
    "align_var_def_inline",
    "align_assign_span",
    "align_assign_thresh",
    "align_enum_equ_span",
    "align_enum_equ_thresh",
    "align_var_class_span",
    "align_var_class_thresh",
    "align_var_class_gap",
    "align_var_struct_span",
    "align_var_struct_thresh",
    "align_var_struct_gap",
    "align_struct_init_span",
    "align_typedef_gap",
    "align_typedef_span",
    "align_typedef_func",
    "align_typedef_star_style",
    "align_typedef_amp_style",
    "align_right_cmt_span",
    "align_right_cmt_mix",
    "align_right_cmt_gap",
    "align_right_cmt_at_col",
    "align_func_proto_span",
    "align_func_proto_gap",
    "align_on_operator",
    "align_mix_var_proto",
    "align_single_line_func",
    "align_single_line_brace",
    "align_single_line_brace_gap",
    "align_oc_msg_spec_span",
    "align_nl_cont",
    "align_pp_define_together",
    "align_pp_define_gap",
    "align_pp_define_span",
    "align_left_shift",
    "align_asm_colon",
    "align_oc_msg_colon_span",
    "align_oc_msg_colon_first",
    "align_oc_decl_colon",
    "cmt_width",
    "cmt_reflow_mode",
    "cmt_convert_tab_to_spaces",
    "cmt_indent_multi",
    "cmt_c_group",
    "cmt_c_nl_start",
    "cmt_c_nl_end",
    "cmt_cpp_group",
    "cmt_cpp_nl_start",
    "cmt_cpp_nl_end",
    "cmt_cpp_to_c",
    "cmt_star_cont",
    "cmt_sp_before_star_cont",
    "cmt_sp_after_star_cont",
    "cmt_multi_check_last",
    "cmt_multi_first_len_minimum",
    "cmt_insert_file_header",
    "cmt_insert_file_footer",
    "cmt_insert_func_header",
    "cmt_insert_class_header",
    "cmt_insert_oc_msg_header",
    "cmt_insert_before_preproc",
    "cmt_insert_before_inlines",
    "cmt_insert_before_ctor_dtor",
    "mod_full_brace_do",
    "mod_full_brace_for",
    "mod_full_brace_function",
    "mod_full_brace_if",
    "mod_full_brace_if_chain",
    "mod_full_brace_if_chain_only",
    "mod_full_brace_nl",
    "mod_full_brace_nl_block_rem_mlcond",
    "mod_full_brace_while",
    "mod_full_brace_using",
    "mod_paren_on_return",
    "mod_pawn_semicolon",
    "mod_full_paren_if_bool",
    "mod_remove_extra_semicolon",
    "mod_add_long_function_closebrace_comment",
    "mod_add_long_namespace_closebrace_comment",
    "mod_add_long_class_closebrace_comment",
    "mod_add_long_switch_closebrace_comment",
    "mod_add_long_ifdef_endif_comment",
    "mod_add_long_ifdef_else_comment",
    "mod_sort_import",
    "mod_sort_using",
    "mod_sort_include",
    "mod_move_case_break",
    "mod_case_brace",
    "mod_remove_empty_return",
    "mod_sort_oc_properties",
    "mod_sort_oc_property_class_weight",
    "mod_sort_oc_property_thread_safe_weight",
    "mod_sort_oc_property_readwrite_weight",
    "mod_sort_oc_property_reference_weight",
    "mod_sort_oc_property_getter_weight",
    "mod_sort_oc_property_setter_weight",
    "mod_sort_oc_property_nullability_weight",
    "pp_indent",
    "pp_indent_at_level",
    "pp_indent_count",
    "pp_space",
    "pp_space_count",
    "pp_indent_region",
    "pp_region_indent_code",
    "pp_indent_if",
    "pp_if_indent_code",
    "pp_define_at_level",
    "pp_ignore_define_body",
    "pp_indent_case",
    "pp_indent_func_def",
    "pp_indent_extern",
    "pp_indent_brace",
    "include_category_0",
    "include_category_1",
    "include_category_2",
    "use_indent_func_call_param",
    "use_indent_continue_only_once",
    "use_options_overriding_for_qt_macros",
    "warn_level_tabs_found_in_verbatim_string_literals",
    "option_count",
]

# ------------------------------------------------------------------------------


def enum(**enums):
    return type('Enum', (), enums)


MODE = enum(ALL=0, TRAILING=1, STARTING=2)


def get_strings(fp):
    lines = []
    with open(fp, 'r') as f:
        lines = f.read().splitlines()
    return lines


def process(in_lines, out_lines, strings_list, mode):
    for in_line in in_lines:
        flag_continue = False
        comment_start = in_line.find('#')

        if comment_start == -1:
            out_lines.append(in_line)
            continue

        if mode == MODE.ALL:
            pass
        elif mode == MODE.TRAILING:
            if comment_start == 0:
                flag_continue = True
            else:
                before_comment_lstripped = in_line[:comment_start + 1].lstrip()
                trailing_test_start = before_comment_lstripped.find('#')
                if trailing_test_start == 0:
                    flag_continue = True

        elif mode == MODE.STARTING:
            in_line_lstripped = in_line.lstrip()
            comment_start = in_line_lstripped.find('#')
            if comment_start != 0:
                flag_continue = True
        else:
            raise Exception("Unknown Mode")

        if flag_continue:
            out_lines.append(in_line)
            continue

        comment_string = in_line[comment_start + 1:].strip()
        if not comment_string:
            continue  # removes '#' in line ending with it

        flag_matched = False

        ns_comment_string = comment_string.lower().replace(' ', '')
        for search_str in strings_list:
            ns_search_str = search_str.lower().replace(' ', '')

            edits = editdistance.eval(ns_search_str, ns_comment_string)
            p = 100 * edits / len(ns_comment_string)

            if p <= MATCH_MAX_DELTA_PERCENT or edits <= MATCH_MAX_DELTA_CHARS:
                flag_matched = True
                break

        if flag_matched:
            if mode == MODE.STARTING:
                continue

            out_lines.append(in_line[:comment_start].rstrip())
        else:
            out_lines.append(in_line)


def rewrite_lineends(in_lines, out_lines):
    out_lines.clear()
    out_lines.extend(in_lines)


def rem_option_comments(in_lines, out_lines, options_list):
    for in_line in in_lines:
        s_in_line = in_line.strip()
        comment_start = s_in_line.find('#')
        equals_start = s_in_line.find('=')
        if comment_start != 0 or equals_start < comment_start:
            out_lines.append(in_line)
            continue

        comment_string = s_in_line[comment_start + 1:].strip()

        # ignore lines: #<option>   = value #<comment>
        if comment_string.find('#') != -1:
            out_lines.append(in_line)
            continue

        # search first space in: #<option>    = value
        space_start = search('\s', comment_string)
        if not space_start:
            out_lines.append(in_line)
            continue

        comment_string = comment_string[:space_start.span()[0]]
        if not comment_string:
            continue  # removes '#' in line ending with it

        flag_matched = False

        ns_comment_string = comment_string.lower()
        for search_str in options_list:
            ns_search_str = search_str.lower().replace(' ', '')

            edits = editdistance.eval(ns_search_str, ns_comment_string)
            if edits == 0:
                flag_matched = True
                break

        if flag_matched:
            continue
        else:
            out_lines.append(in_line)


def main():
    strings_list = get_strings(STRINGS_FILE_PATH)
    strings_list.sort(key=len)

    target_files = glob("config/*.cfg")
    for target_file in target_files:
        in_lines = get_strings(target_file)
        out_lines = []

        # rewrite_lineends(in_lines, out_lines)
        # rem_option_comments(in_lines, out_lines, OPTIONS_LIST)
        # process(in_lines, out_lines, strings_list, MODE.TRAILING)
        process(in_lines, out_lines, strings_list, MODE.STARTING)

        with open(target_file, "w") as f:
            for out_line in out_lines:
                print(out_line, file=f, end='\n')


if __name__ == "__main__":
    main()

