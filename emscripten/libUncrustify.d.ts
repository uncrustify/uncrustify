/**
*  Emscriptens interface for bound std::vectors
*/
interface EmscriptenVector< T >
{
    get( i : number ) : T
    push_back( elem : T );
    resize( size : number, elem : T );
    size() : number;
    get() : T;
    set( elem : T );
//TODO:
//    isAliasOf();
//    clone();
//    delete();
//    isDeleted() : boolean;
//    deleteLater();
}

/**
*  Emscriptens interface for bound std::maps
*/
interface EmscriptenMap< K, V >
{
    size() : number;
    get( elem : K ) : V;

//TODO:
    //et() : void;
    //isAliasOf() : void;
    //clone() : void;
    //delete() : void;
    //isDeleted() : void;
    //deleteLater() : void;
}

/**
*  Emscriptens interface for bound enum types
*/
interface EmscriptenEnumType
{
    //! returns list with value objects of an enum
    values() : EmscriptenVector<EmscriptenEnumTypeObject>;
}

/**
*  Emscriptens interface for bound enum type value objects
*/
interface EmscriptenEnumTypeObject
{
    //! return value of an enum value object
    value : number;
}

declare namespace LibUncrustify
{
    // <editor-fold desc="enums">

    // Example how to iterate below options : forin iterate Options,
    // skip 'values' key, [ s : Options_STRING ] : EmscriptenEnumTypeObject;

    // region enum bindings
    export interface Option extends EmscriptenEnumType
    {
        UO_newlines : EmscriptenEnumTypeObject;
        UO_input_tab_size : EmscriptenEnumTypeObject;
        UO_output_tab_size : EmscriptenEnumTypeObject;
        UO_string_escape_char : EmscriptenEnumTypeObject;
        UO_string_escape_char2 : EmscriptenEnumTypeObject;
        UO_string_replace_tab_chars : EmscriptenEnumTypeObject;
        UO_tok_split_gte : EmscriptenEnumTypeObject;
        UO_disable_processing_cmt : EmscriptenEnumTypeObject;
        UO_enable_processing_cmt : EmscriptenEnumTypeObject;
        UO_enable_digraphs : EmscriptenEnumTypeObject;
        UO_utf8_bom : EmscriptenEnumTypeObject;
        UO_utf8_byte : EmscriptenEnumTypeObject;
        UO_utf8_force : EmscriptenEnumTypeObject;
        UO_sp_arith : EmscriptenEnumTypeObject;
        UO_sp_arith_additive : EmscriptenEnumTypeObject;
        UO_sp_assign : EmscriptenEnumTypeObject;
        UO_sp_cpp_lambda_assign : EmscriptenEnumTypeObject;
        UO_sp_cpp_lambda_paren : EmscriptenEnumTypeObject;
        UO_sp_assign_default : EmscriptenEnumTypeObject;
        UO_sp_before_assign : EmscriptenEnumTypeObject;
        UO_sp_after_assign : EmscriptenEnumTypeObject;
        UO_sp_enum_paren : EmscriptenEnumTypeObject;
        UO_sp_enum_assign : EmscriptenEnumTypeObject;
        UO_sp_enum_before_assign : EmscriptenEnumTypeObject;
        UO_sp_enum_after_assign : EmscriptenEnumTypeObject;
        UO_sp_enum_colon : EmscriptenEnumTypeObject;
        UO_sp_pp_concat : EmscriptenEnumTypeObject;
        UO_sp_pp_stringify : EmscriptenEnumTypeObject;
        UO_sp_before_pp_stringify : EmscriptenEnumTypeObject;
        UO_sp_bool : EmscriptenEnumTypeObject;
        UO_sp_compare : EmscriptenEnumTypeObject;
        UO_sp_inside_paren : EmscriptenEnumTypeObject;
        UO_sp_paren_paren : EmscriptenEnumTypeObject;
        UO_sp_cparen_oparen : EmscriptenEnumTypeObject;
        UO_sp_balance_nested_parens : EmscriptenEnumTypeObject;
        UO_sp_paren_brace : EmscriptenEnumTypeObject;
        UO_sp_before_ptr_star : EmscriptenEnumTypeObject;
        UO_sp_before_unnamed_ptr_star : EmscriptenEnumTypeObject;
        UO_sp_between_ptr_star : EmscriptenEnumTypeObject;
        UO_sp_after_ptr_star : EmscriptenEnumTypeObject;
        UO_sp_after_ptr_star_qualifier : EmscriptenEnumTypeObject;
        UO_sp_after_ptr_star_func : EmscriptenEnumTypeObject;
        UO_sp_ptr_star_paren : EmscriptenEnumTypeObject;
        UO_sp_before_ptr_star_func : EmscriptenEnumTypeObject;
        UO_sp_before_byref : EmscriptenEnumTypeObject;
        UO_sp_before_unnamed_byref : EmscriptenEnumTypeObject;
        UO_sp_after_byref : EmscriptenEnumTypeObject;
        UO_sp_after_byref_func : EmscriptenEnumTypeObject;
        UO_sp_before_byref_func : EmscriptenEnumTypeObject;
        UO_sp_after_type : EmscriptenEnumTypeObject;
        UO_sp_before_template_paren : EmscriptenEnumTypeObject;
        UO_sp_template_angle : EmscriptenEnumTypeObject;
        UO_sp_before_angle : EmscriptenEnumTypeObject;
        UO_sp_inside_angle : EmscriptenEnumTypeObject;
        UO_sp_angle_colon : EmscriptenEnumTypeObject;
        UO_sp_after_angle : EmscriptenEnumTypeObject;
        UO_sp_angle_paren : EmscriptenEnumTypeObject;
        UO_sp_angle_paren_empty : EmscriptenEnumTypeObject;
        UO_sp_angle_word : EmscriptenEnumTypeObject;
        UO_sp_angle_shift : EmscriptenEnumTypeObject;
        UO_sp_permit_cpp11_shift : EmscriptenEnumTypeObject;
        UO_sp_before_sparen : EmscriptenEnumTypeObject;
        UO_sp_inside_sparen : EmscriptenEnumTypeObject;
        UO_sp_inside_sparen_close : EmscriptenEnumTypeObject;
        UO_sp_inside_sparen_open : EmscriptenEnumTypeObject;
        UO_sp_after_sparen : EmscriptenEnumTypeObject;
        UO_sp_sparen_brace : EmscriptenEnumTypeObject;
        UO_sp_invariant_paren : EmscriptenEnumTypeObject;
        UO_sp_after_invariant_paren : EmscriptenEnumTypeObject;
        UO_sp_special_semi : EmscriptenEnumTypeObject;
        UO_sp_before_semi : EmscriptenEnumTypeObject;
        UO_sp_before_semi_for : EmscriptenEnumTypeObject;
        UO_sp_before_semi_for_empty : EmscriptenEnumTypeObject;
        UO_sp_after_semi : EmscriptenEnumTypeObject;
        UO_sp_after_semi_for : EmscriptenEnumTypeObject;
        UO_sp_after_semi_for_empty : EmscriptenEnumTypeObject;
        UO_sp_before_square : EmscriptenEnumTypeObject;
        UO_sp_before_squares : EmscriptenEnumTypeObject;
        UO_sp_inside_square : EmscriptenEnumTypeObject;
        UO_sp_after_comma : EmscriptenEnumTypeObject;
        UO_sp_before_comma : EmscriptenEnumTypeObject;
        UO_sp_after_mdatype_commas : EmscriptenEnumTypeObject;
        UO_sp_before_mdatype_commas : EmscriptenEnumTypeObject;
        UO_sp_between_mdatype_commas : EmscriptenEnumTypeObject;
        UO_sp_paren_comma : EmscriptenEnumTypeObject;
        UO_sp_before_ellipsis : EmscriptenEnumTypeObject;
        UO_sp_after_class_colon : EmscriptenEnumTypeObject;
        UO_sp_before_class_colon : EmscriptenEnumTypeObject;
        UO_sp_after_constr_colon : EmscriptenEnumTypeObject;
        UO_sp_before_constr_colon : EmscriptenEnumTypeObject;
        UO_sp_before_case_colon : EmscriptenEnumTypeObject;
        UO_sp_after_operator : EmscriptenEnumTypeObject;
        UO_sp_after_operator_sym : EmscriptenEnumTypeObject;
        UO_sp_after_operator_sym_empty : EmscriptenEnumTypeObject;
        UO_sp_after_cast : EmscriptenEnumTypeObject;
        UO_sp_inside_paren_cast : EmscriptenEnumTypeObject;
        UO_sp_cpp_cast_paren : EmscriptenEnumTypeObject;
        UO_sp_sizeof_paren : EmscriptenEnumTypeObject;
        UO_sp_after_tag : EmscriptenEnumTypeObject;
        UO_sp_inside_braces_enum : EmscriptenEnumTypeObject;
        UO_sp_inside_braces_struct : EmscriptenEnumTypeObject;
        UO_sp_after_type_brace_init_lst_open : EmscriptenEnumTypeObject;
        UO_sp_before_type_brace_init_lst_close : EmscriptenEnumTypeObject;
        UO_sp_inside_type_brace_init_lst : EmscriptenEnumTypeObject;
        UO_sp_inside_braces : EmscriptenEnumTypeObject;
        UO_sp_inside_braces_empty : EmscriptenEnumTypeObject;
        UO_sp_type_func : EmscriptenEnumTypeObject;
        UO_sp_type_brace_init_lst : EmscriptenEnumTypeObject;
        UO_sp_func_proto_paren : EmscriptenEnumTypeObject;
        UO_sp_func_proto_paren_empty : EmscriptenEnumTypeObject;
        UO_sp_func_def_paren : EmscriptenEnumTypeObject;
        UO_sp_func_def_paren_empty : EmscriptenEnumTypeObject;
        UO_sp_inside_fparens : EmscriptenEnumTypeObject;
        UO_sp_inside_fparen : EmscriptenEnumTypeObject;
        UO_sp_inside_tparen : EmscriptenEnumTypeObject;
        UO_sp_after_tparen_close : EmscriptenEnumTypeObject;
        UO_sp_square_fparen : EmscriptenEnumTypeObject;
        UO_sp_fparen_brace : EmscriptenEnumTypeObject;
        UO_sp_fparen_dbrace : EmscriptenEnumTypeObject;
        UO_sp_func_call_paren : EmscriptenEnumTypeObject;
        UO_sp_func_call_paren_empty : EmscriptenEnumTypeObject;
        UO_sp_func_call_user_paren : EmscriptenEnumTypeObject;
        UO_sp_func_class_paren : EmscriptenEnumTypeObject;
        UO_sp_func_class_paren_empty : EmscriptenEnumTypeObject;
        UO_sp_return_paren : EmscriptenEnumTypeObject;
        UO_sp_attribute_paren : EmscriptenEnumTypeObject;
        UO_sp_defined_paren : EmscriptenEnumTypeObject;
        UO_sp_throw_paren : EmscriptenEnumTypeObject;
        UO_sp_after_throw : EmscriptenEnumTypeObject;
        UO_sp_catch_paren : EmscriptenEnumTypeObject;
        UO_sp_version_paren : EmscriptenEnumTypeObject;
        UO_sp_scope_paren : EmscriptenEnumTypeObject;
        UO_sp_super_paren : EmscriptenEnumTypeObject;
        UO_sp_this_paren : EmscriptenEnumTypeObject;
        UO_sp_macro : EmscriptenEnumTypeObject;
        UO_sp_macro_func : EmscriptenEnumTypeObject;
        UO_sp_else_brace : EmscriptenEnumTypeObject;
        UO_sp_brace_else : EmscriptenEnumTypeObject;
        UO_sp_brace_typedef : EmscriptenEnumTypeObject;
        UO_sp_catch_brace : EmscriptenEnumTypeObject;
        UO_sp_brace_catch : EmscriptenEnumTypeObject;
        UO_sp_finally_brace : EmscriptenEnumTypeObject;
        UO_sp_brace_finally : EmscriptenEnumTypeObject;
        UO_sp_try_brace : EmscriptenEnumTypeObject;
        UO_sp_getset_brace : EmscriptenEnumTypeObject;
        UO_sp_word_brace : EmscriptenEnumTypeObject;
        UO_sp_word_brace_ns : EmscriptenEnumTypeObject;
        UO_sp_before_dc : EmscriptenEnumTypeObject;
        UO_sp_after_dc : EmscriptenEnumTypeObject;
        UO_sp_d_array_colon : EmscriptenEnumTypeObject;
        UO_sp_not : EmscriptenEnumTypeObject;
        UO_sp_inv : EmscriptenEnumTypeObject;
        UO_sp_addr : EmscriptenEnumTypeObject;
        UO_sp_member : EmscriptenEnumTypeObject;
        UO_sp_deref : EmscriptenEnumTypeObject;
        UO_sp_sign : EmscriptenEnumTypeObject;
        UO_sp_incdec : EmscriptenEnumTypeObject;
        UO_sp_before_nl_cont : EmscriptenEnumTypeObject;
        UO_sp_after_oc_scope : EmscriptenEnumTypeObject;
        UO_sp_after_oc_colon : EmscriptenEnumTypeObject;
        UO_sp_before_oc_colon : EmscriptenEnumTypeObject;
        UO_sp_after_oc_dict_colon : EmscriptenEnumTypeObject;
        UO_sp_before_oc_dict_colon : EmscriptenEnumTypeObject;
        UO_sp_after_send_oc_colon : EmscriptenEnumTypeObject;
        UO_sp_before_send_oc_colon : EmscriptenEnumTypeObject;
        UO_sp_after_oc_type : EmscriptenEnumTypeObject;
        UO_sp_after_oc_return_type : EmscriptenEnumTypeObject;
        UO_sp_after_oc_at_sel : EmscriptenEnumTypeObject;
        UO_sp_after_oc_at_sel_parens : EmscriptenEnumTypeObject;
        UO_sp_inside_oc_at_sel_parens : EmscriptenEnumTypeObject;
        UO_sp_before_oc_block_caret : EmscriptenEnumTypeObject;
        UO_sp_after_oc_block_caret : EmscriptenEnumTypeObject;
        UO_sp_after_oc_msg_receiver : EmscriptenEnumTypeObject;
        UO_sp_after_oc_property : EmscriptenEnumTypeObject;
        UO_sp_cond_colon : EmscriptenEnumTypeObject;
        UO_sp_cond_colon_before : EmscriptenEnumTypeObject;
        UO_sp_cond_colon_after : EmscriptenEnumTypeObject;
        UO_sp_cond_question : EmscriptenEnumTypeObject;
        UO_sp_cond_question_before : EmscriptenEnumTypeObject;
        UO_sp_cond_question_after : EmscriptenEnumTypeObject;
        UO_sp_cond_ternary_short : EmscriptenEnumTypeObject;
        UO_sp_case_label : EmscriptenEnumTypeObject;
        UO_sp_range : EmscriptenEnumTypeObject;
        UO_sp_after_for_colon : EmscriptenEnumTypeObject;
        UO_sp_before_for_colon : EmscriptenEnumTypeObject;
        UO_sp_extern_paren : EmscriptenEnumTypeObject;
        UO_sp_cmt_cpp_start : EmscriptenEnumTypeObject;
        UO_sp_cmt_cpp_doxygen : EmscriptenEnumTypeObject;
        UO_sp_cmt_cpp_qttr : EmscriptenEnumTypeObject;
        UO_sp_endif_cmt : EmscriptenEnumTypeObject;
        UO_sp_after_new : EmscriptenEnumTypeObject;
        UO_sp_between_new_paren : EmscriptenEnumTypeObject;
        UO_sp_after_newop_paren : EmscriptenEnumTypeObject;
        UO_sp_inside_newop_paren : EmscriptenEnumTypeObject;
        UO_sp_inside_newop_paren_open : EmscriptenEnumTypeObject;
        UO_sp_inside_newop_paren_close : EmscriptenEnumTypeObject;
        UO_sp_before_tr_emb_cmt : EmscriptenEnumTypeObject;
        UO_sp_num_before_tr_emb_cmt : EmscriptenEnumTypeObject;
        UO_sp_annotation_paren : EmscriptenEnumTypeObject;
        UO_sp_skip_vbrace_tokens : EmscriptenEnumTypeObject;
        UO_force_tab_after_define : EmscriptenEnumTypeObject;
        UO_indent_columns : EmscriptenEnumTypeObject;
        UO_indent_continue : EmscriptenEnumTypeObject;
        UO_indent_param : EmscriptenEnumTypeObject;
        UO_indent_with_tabs : EmscriptenEnumTypeObject;
        UO_indent_cmt_with_tabs : EmscriptenEnumTypeObject;
        UO_indent_align_string : EmscriptenEnumTypeObject;
        UO_indent_xml_string : EmscriptenEnumTypeObject;
        UO_indent_brace : EmscriptenEnumTypeObject;
        UO_indent_braces : EmscriptenEnumTypeObject;
        UO_indent_braces_no_func : EmscriptenEnumTypeObject;
        UO_indent_braces_no_class : EmscriptenEnumTypeObject;
        UO_indent_braces_no_struct : EmscriptenEnumTypeObject;
        UO_indent_brace_parent : EmscriptenEnumTypeObject;
        UO_indent_paren_open_brace : EmscriptenEnumTypeObject;
        UO_indent_cs_delegate_brace : EmscriptenEnumTypeObject;
        UO_indent_namespace : EmscriptenEnumTypeObject;
        UO_indent_namespace_single_indent : EmscriptenEnumTypeObject;
        UO_indent_namespace_level : EmscriptenEnumTypeObject;
        UO_indent_namespace_limit : EmscriptenEnumTypeObject;
        UO_indent_extern : EmscriptenEnumTypeObject;
        UO_indent_class : EmscriptenEnumTypeObject;
        UO_indent_class_colon : EmscriptenEnumTypeObject;
        UO_indent_class_on_colon : EmscriptenEnumTypeObject;
        UO_indent_constr_colon : EmscriptenEnumTypeObject;
        UO_indent_ctor_init_leading : EmscriptenEnumTypeObject;
        UO_indent_ctor_init : EmscriptenEnumTypeObject;
        UO_indent_else_if : EmscriptenEnumTypeObject;
        UO_indent_var_def_blk : EmscriptenEnumTypeObject;
        UO_indent_var_def_cont : EmscriptenEnumTypeObject;
        UO_indent_shift : EmscriptenEnumTypeObject;
        UO_indent_func_def_force_col1 : EmscriptenEnumTypeObject;
        UO_indent_func_call_param : EmscriptenEnumTypeObject;
        UO_indent_func_def_param : EmscriptenEnumTypeObject;
        UO_indent_func_proto_param : EmscriptenEnumTypeObject;
        UO_indent_func_class_param : EmscriptenEnumTypeObject;
        UO_indent_func_ctor_var_param : EmscriptenEnumTypeObject;
        UO_indent_template_param : EmscriptenEnumTypeObject;
        UO_indent_func_param_double : EmscriptenEnumTypeObject;
        UO_indent_func_const : EmscriptenEnumTypeObject;
        UO_indent_func_throw : EmscriptenEnumTypeObject;
        UO_indent_member : EmscriptenEnumTypeObject;
        UO_indent_sing_line_comments : EmscriptenEnumTypeObject;
        UO_indent_relative_single_line_comments : EmscriptenEnumTypeObject;
        UO_indent_switch_case : EmscriptenEnumTypeObject;
        UO_indent_switch_pp : EmscriptenEnumTypeObject;
        UO_indent_case_shift : EmscriptenEnumTypeObject;
        UO_indent_case_brace : EmscriptenEnumTypeObject;
        UO_indent_col1_comment : EmscriptenEnumTypeObject;
        UO_indent_label : EmscriptenEnumTypeObject;
        UO_indent_access_spec : EmscriptenEnumTypeObject;
        UO_indent_access_spec_body : EmscriptenEnumTypeObject;
        UO_indent_paren_nl : EmscriptenEnumTypeObject;
        UO_indent_paren_close : EmscriptenEnumTypeObject;
        UO_indent_paren_after_func_def : EmscriptenEnumTypeObject;
        UO_indent_paren_after_func_decl : EmscriptenEnumTypeObject;
        UO_indent_paren_after_func_call : EmscriptenEnumTypeObject;
        UO_indent_comma_paren : EmscriptenEnumTypeObject;
        UO_indent_bool_paren : EmscriptenEnumTypeObject;
        UO_indent_first_bool_expr : EmscriptenEnumTypeObject;
        UO_indent_square_nl : EmscriptenEnumTypeObject;
        UO_indent_preserve_sql : EmscriptenEnumTypeObject;
        UO_indent_align_assign : EmscriptenEnumTypeObject;
        UO_indent_oc_block : EmscriptenEnumTypeObject;
        UO_indent_oc_block_msg : EmscriptenEnumTypeObject;
        UO_indent_oc_msg_colon : EmscriptenEnumTypeObject;
        UO_indent_oc_msg_prioritize_first_colon : EmscriptenEnumTypeObject;
        UO_indent_oc_block_msg_xcode_style : EmscriptenEnumTypeObject;
        UO_indent_oc_block_msg_from_keyword : EmscriptenEnumTypeObject;
        UO_indent_oc_block_msg_from_colon : EmscriptenEnumTypeObject;
        UO_indent_oc_block_msg_from_caret : EmscriptenEnumTypeObject;
        UO_indent_oc_block_msg_from_brace : EmscriptenEnumTypeObject;
        UO_indent_min_vbrace_open : EmscriptenEnumTypeObject;
        UO_indent_vbrace_open_on_tabstop : EmscriptenEnumTypeObject;
        UO_indent_token_after_brace : EmscriptenEnumTypeObject;
        UO_indent_cpp_lambda_body : EmscriptenEnumTypeObject;
        UO_indent_using_block : EmscriptenEnumTypeObject;
        UO_indent_ternary_operator : EmscriptenEnumTypeObject;
        UO_indent_ignore_asm_block : EmscriptenEnumTypeObject;
        UO_nl_collapse_empty_body : EmscriptenEnumTypeObject;
        UO_nl_assign_leave_one_liners : EmscriptenEnumTypeObject;
        UO_nl_class_leave_one_liners : EmscriptenEnumTypeObject;
        UO_nl_enum_leave_one_liners : EmscriptenEnumTypeObject;
        UO_nl_getset_leave_one_liners : EmscriptenEnumTypeObject;
        UO_nl_func_leave_one_liners : EmscriptenEnumTypeObject;
        UO_nl_cpp_lambda_leave_one_liners : EmscriptenEnumTypeObject;
        UO_nl_if_leave_one_liners : EmscriptenEnumTypeObject;
        UO_nl_while_leave_one_liners : EmscriptenEnumTypeObject;
        UO_nl_oc_msg_leave_one_liner : EmscriptenEnumTypeObject;
        UO_nl_oc_block_brace : EmscriptenEnumTypeObject;
        UO_nl_start_of_file : EmscriptenEnumTypeObject;
        UO_nl_start_of_file_min : EmscriptenEnumTypeObject;
        UO_nl_end_of_file : EmscriptenEnumTypeObject;
        UO_nl_end_of_file_min : EmscriptenEnumTypeObject;
        UO_nl_assign_brace : EmscriptenEnumTypeObject;
        UO_nl_assign_square : EmscriptenEnumTypeObject;
        UO_nl_after_square_assign : EmscriptenEnumTypeObject;
        UO_nl_func_var_def_blk : EmscriptenEnumTypeObject;
        UO_nl_typedef_blk_start : EmscriptenEnumTypeObject;
        UO_nl_typedef_blk_end : EmscriptenEnumTypeObject;
        UO_nl_typedef_blk_in : EmscriptenEnumTypeObject;
        UO_nl_var_def_blk_start : EmscriptenEnumTypeObject;
        UO_nl_var_def_blk_end : EmscriptenEnumTypeObject;
        UO_nl_var_def_blk_in : EmscriptenEnumTypeObject;
        UO_nl_fcall_brace : EmscriptenEnumTypeObject;
        UO_nl_enum_brace : EmscriptenEnumTypeObject;
        UO_nl_enum_class : EmscriptenEnumTypeObject;
        UO_nl_enum_class_identifier : EmscriptenEnumTypeObject;
        UO_nl_enum_identifier_colon : EmscriptenEnumTypeObject;
        UO_nl_enum_colon_type : EmscriptenEnumTypeObject;
        UO_nl_struct_brace : EmscriptenEnumTypeObject;
        UO_nl_union_brace : EmscriptenEnumTypeObject;
        UO_nl_if_brace : EmscriptenEnumTypeObject;
        UO_nl_brace_else : EmscriptenEnumTypeObject;
        UO_nl_elseif_brace : EmscriptenEnumTypeObject;
        UO_nl_else_brace : EmscriptenEnumTypeObject;
        UO_nl_else_if : EmscriptenEnumTypeObject;
        UO_nl_before_if_closing_paren : EmscriptenEnumTypeObject;
        UO_nl_brace_finally : EmscriptenEnumTypeObject;
        UO_nl_finally_brace : EmscriptenEnumTypeObject;
        UO_nl_try_brace : EmscriptenEnumTypeObject;
        UO_nl_getset_brace : EmscriptenEnumTypeObject;
        UO_nl_for_brace : EmscriptenEnumTypeObject;
        UO_nl_catch_brace : EmscriptenEnumTypeObject;
        UO_nl_brace_catch : EmscriptenEnumTypeObject;
        UO_nl_brace_square : EmscriptenEnumTypeObject;
        UO_nl_brace_fparen : EmscriptenEnumTypeObject;
        UO_nl_while_brace : EmscriptenEnumTypeObject;
        UO_nl_scope_brace : EmscriptenEnumTypeObject;
        UO_nl_unittest_brace : EmscriptenEnumTypeObject;
        UO_nl_version_brace : EmscriptenEnumTypeObject;
        UO_nl_using_brace : EmscriptenEnumTypeObject;
        UO_nl_brace_brace : EmscriptenEnumTypeObject;
        UO_nl_do_brace : EmscriptenEnumTypeObject;
        UO_nl_brace_while : EmscriptenEnumTypeObject;
        UO_nl_switch_brace : EmscriptenEnumTypeObject;
        UO_nl_synchronized_brace : EmscriptenEnumTypeObject;
        UO_nl_multi_line_cond : EmscriptenEnumTypeObject;
        UO_nl_multi_line_define : EmscriptenEnumTypeObject;
        UO_nl_before_case : EmscriptenEnumTypeObject;
        UO_nl_before_throw : EmscriptenEnumTypeObject;
        UO_nl_after_case : EmscriptenEnumTypeObject;
        UO_nl_case_colon_brace : EmscriptenEnumTypeObject;
        UO_nl_namespace_brace : EmscriptenEnumTypeObject;
        UO_nl_template_class : EmscriptenEnumTypeObject;
        UO_nl_class_brace : EmscriptenEnumTypeObject;
        UO_nl_class_init_args : EmscriptenEnumTypeObject;
        UO_nl_constr_init_args : EmscriptenEnumTypeObject;
        UO_nl_enum_own_lines : EmscriptenEnumTypeObject;
        UO_nl_func_type_name : EmscriptenEnumTypeObject;
        UO_nl_func_type_name_class : EmscriptenEnumTypeObject;
        UO_nl_func_class_scope : EmscriptenEnumTypeObject;
        UO_nl_func_scope_name : EmscriptenEnumTypeObject;
        UO_nl_func_proto_type_name : EmscriptenEnumTypeObject;
        UO_nl_func_paren : EmscriptenEnumTypeObject;
        UO_nl_func_paren_empty : EmscriptenEnumTypeObject;
        UO_nl_func_def_paren : EmscriptenEnumTypeObject;
        UO_nl_func_def_paren_empty : EmscriptenEnumTypeObject;
        UO_nl_func_call_paren : EmscriptenEnumTypeObject;
        UO_nl_func_call_paren_empty : EmscriptenEnumTypeObject;
        UO_nl_func_decl_start : EmscriptenEnumTypeObject;
        UO_nl_func_def_start : EmscriptenEnumTypeObject;
        UO_nl_func_decl_start_single : EmscriptenEnumTypeObject;
        UO_nl_func_def_start_single : EmscriptenEnumTypeObject;
        UO_nl_func_decl_start_multi_line : EmscriptenEnumTypeObject;
        UO_nl_func_def_start_multi_line : EmscriptenEnumTypeObject;
        UO_nl_func_decl_args : EmscriptenEnumTypeObject;
        UO_nl_func_def_args : EmscriptenEnumTypeObject;
        UO_nl_func_decl_args_multi_line : EmscriptenEnumTypeObject;
        UO_nl_func_def_args_multi_line : EmscriptenEnumTypeObject;
        UO_nl_func_decl_end : EmscriptenEnumTypeObject;
        UO_nl_func_def_end : EmscriptenEnumTypeObject;
        UO_nl_func_decl_end_single : EmscriptenEnumTypeObject;
        UO_nl_func_def_end_single : EmscriptenEnumTypeObject;
        UO_nl_func_decl_end_multi_line : EmscriptenEnumTypeObject;
        UO_nl_func_def_end_multi_line : EmscriptenEnumTypeObject;
        UO_nl_func_decl_empty : EmscriptenEnumTypeObject;
        UO_nl_func_def_empty : EmscriptenEnumTypeObject;
        UO_nl_func_call_empty : EmscriptenEnumTypeObject;
        UO_nl_func_call_start_multi_line : EmscriptenEnumTypeObject;
        UO_nl_func_call_args_multi_line : EmscriptenEnumTypeObject;
        UO_nl_func_call_end_multi_line : EmscriptenEnumTypeObject;
        UO_nl_oc_msg_args : EmscriptenEnumTypeObject;
        UO_nl_fdef_brace : EmscriptenEnumTypeObject;
        UO_nl_cpp_ldef_brace : EmscriptenEnumTypeObject;
        UO_nl_return_expr : EmscriptenEnumTypeObject;
        UO_nl_after_semicolon : EmscriptenEnumTypeObject;
        UO_nl_paren_dbrace_open : EmscriptenEnumTypeObject;
        UO_nl_type_brace_init_lst : EmscriptenEnumTypeObject;
        UO_nl_type_brace_init_lst_open : EmscriptenEnumTypeObject;
        UO_nl_type_brace_init_lst_close : EmscriptenEnumTypeObject;
        UO_nl_after_brace_open : EmscriptenEnumTypeObject;
        UO_nl_after_brace_open_cmt : EmscriptenEnumTypeObject;
        UO_nl_after_vbrace_open : EmscriptenEnumTypeObject;
        UO_nl_after_vbrace_open_empty : EmscriptenEnumTypeObject;
        UO_nl_after_brace_close : EmscriptenEnumTypeObject;
        UO_nl_after_vbrace_close : EmscriptenEnumTypeObject;
        UO_nl_brace_struct_var : EmscriptenEnumTypeObject;
        UO_nl_define_macro : EmscriptenEnumTypeObject;
        UO_nl_squeeze_ifdef : EmscriptenEnumTypeObject;
        UO_nl_squeeze_ifdef_top_level : EmscriptenEnumTypeObject;
        UO_nl_before_if : EmscriptenEnumTypeObject;
        UO_nl_after_if : EmscriptenEnumTypeObject;
        UO_nl_before_for : EmscriptenEnumTypeObject;
        UO_nl_after_for : EmscriptenEnumTypeObject;
        UO_nl_before_while : EmscriptenEnumTypeObject;
        UO_nl_after_while : EmscriptenEnumTypeObject;
        UO_nl_before_switch : EmscriptenEnumTypeObject;
        UO_nl_after_switch : EmscriptenEnumTypeObject;
        UO_nl_before_synchronized : EmscriptenEnumTypeObject;
        UO_nl_after_synchronized : EmscriptenEnumTypeObject;
        UO_nl_before_do : EmscriptenEnumTypeObject;
        UO_nl_after_do : EmscriptenEnumTypeObject;
        UO_nl_ds_struct_enum_cmt : EmscriptenEnumTypeObject;
        UO_nl_ds_struct_enum_close_brace : EmscriptenEnumTypeObject;
        UO_nl_before_func_class_def : EmscriptenEnumTypeObject;
        UO_nl_before_func_class_proto : EmscriptenEnumTypeObject;
        UO_nl_class_colon : EmscriptenEnumTypeObject;
        UO_nl_constr_colon : EmscriptenEnumTypeObject;
        UO_nl_create_if_one_liner : EmscriptenEnumTypeObject;
        UO_nl_create_for_one_liner : EmscriptenEnumTypeObject;
        UO_nl_create_while_one_liner : EmscriptenEnumTypeObject;
        UO_nl_split_if_one_liner : EmscriptenEnumTypeObject;
        UO_nl_split_for_one_liner : EmscriptenEnumTypeObject;
        UO_nl_split_while_one_liner : EmscriptenEnumTypeObject;
        UO_nl_max : EmscriptenEnumTypeObject;
        UO_nl_max_blank_in_func : EmscriptenEnumTypeObject;
        UO_nl_after_func_proto : EmscriptenEnumTypeObject;
        UO_nl_after_func_proto_group : EmscriptenEnumTypeObject;
        UO_nl_after_func_class_proto : EmscriptenEnumTypeObject;
        UO_nl_after_func_class_proto_group : EmscriptenEnumTypeObject;
        UO_nl_before_func_body_def : EmscriptenEnumTypeObject;
        UO_nl_before_func_body_proto : EmscriptenEnumTypeObject;
        UO_nl_after_func_body : EmscriptenEnumTypeObject;
        UO_nl_after_func_body_class : EmscriptenEnumTypeObject;
        UO_nl_after_func_body_one_liner : EmscriptenEnumTypeObject;
        UO_nl_before_block_comment : EmscriptenEnumTypeObject;
        UO_nl_before_c_comment : EmscriptenEnumTypeObject;
        UO_nl_before_cpp_comment : EmscriptenEnumTypeObject;
        UO_nl_after_multiline_comment : EmscriptenEnumTypeObject;
        UO_nl_after_label_colon : EmscriptenEnumTypeObject;
        UO_nl_after_struct : EmscriptenEnumTypeObject;
        UO_nl_before_class : EmscriptenEnumTypeObject;
        UO_nl_after_class : EmscriptenEnumTypeObject;
        UO_nl_before_access_spec : EmscriptenEnumTypeObject;
        UO_nl_after_access_spec : EmscriptenEnumTypeObject;
        UO_nl_comment_func_def : EmscriptenEnumTypeObject;
        UO_nl_after_try_catch_finally : EmscriptenEnumTypeObject;
        UO_nl_around_cs_property : EmscriptenEnumTypeObject;
        UO_nl_between_get_set : EmscriptenEnumTypeObject;
        UO_nl_property_brace : EmscriptenEnumTypeObject;
        UO_eat_blanks_after_open_brace : EmscriptenEnumTypeObject;
        UO_eat_blanks_before_close_brace : EmscriptenEnumTypeObject;
        UO_nl_remove_extra_newlines : EmscriptenEnumTypeObject;
        UO_nl_before_return : EmscriptenEnumTypeObject;
        UO_nl_after_return : EmscriptenEnumTypeObject;
        UO_nl_after_annotation : EmscriptenEnumTypeObject;
        UO_nl_between_annotation : EmscriptenEnumTypeObject;
        UO_pos_arith : EmscriptenEnumTypeObject;
        UO_pos_assign : EmscriptenEnumTypeObject;
        UO_pos_bool : EmscriptenEnumTypeObject;
        UO_pos_compare : EmscriptenEnumTypeObject;
        UO_pos_conditional : EmscriptenEnumTypeObject;
        UO_pos_comma : EmscriptenEnumTypeObject;
        UO_pos_enum_comma : EmscriptenEnumTypeObject;
        UO_pos_class_comma : EmscriptenEnumTypeObject;
        UO_pos_constr_comma : EmscriptenEnumTypeObject;
        UO_pos_class_colon : EmscriptenEnumTypeObject;
        UO_pos_constr_colon : EmscriptenEnumTypeObject;
        UO_code_width : EmscriptenEnumTypeObject;
        UO_ls_for_split_full : EmscriptenEnumTypeObject;
        UO_ls_func_split_full : EmscriptenEnumTypeObject;
        UO_ls_code_width : EmscriptenEnumTypeObject;
        UO_align_keep_tabs : EmscriptenEnumTypeObject;
        UO_align_with_tabs : EmscriptenEnumTypeObject;
        UO_align_on_tabstop : EmscriptenEnumTypeObject;
        UO_align_number_right : EmscriptenEnumTypeObject;
        UO_align_keep_extra_space : EmscriptenEnumTypeObject;
        UO_align_func_params : EmscriptenEnumTypeObject;
        UO_align_func_params_span : EmscriptenEnumTypeObject;
        UO_align_func_params_thresh : EmscriptenEnumTypeObject;
        UO_align_func_params_gap : EmscriptenEnumTypeObject;
        UO_align_same_func_call_params : EmscriptenEnumTypeObject;
        UO_align_var_def_span : EmscriptenEnumTypeObject;
        UO_align_var_def_star_style : EmscriptenEnumTypeObject;
        UO_align_var_def_amp_style : EmscriptenEnumTypeObject;
        UO_align_var_def_thresh : EmscriptenEnumTypeObject;
        UO_align_var_def_gap : EmscriptenEnumTypeObject;
        UO_align_var_def_colon : EmscriptenEnumTypeObject;
        UO_align_var_def_colon_gap : EmscriptenEnumTypeObject;
        UO_align_var_def_attribute : EmscriptenEnumTypeObject;
        UO_align_var_def_inline : EmscriptenEnumTypeObject;
        UO_align_assign_span : EmscriptenEnumTypeObject;
        UO_align_assign_thresh : EmscriptenEnumTypeObject;
        UO_align_enum_equ_span : EmscriptenEnumTypeObject;
        UO_align_enum_equ_thresh : EmscriptenEnumTypeObject;
        UO_align_var_class_span : EmscriptenEnumTypeObject;
        UO_align_var_class_thresh : EmscriptenEnumTypeObject;
        UO_align_var_class_gap : EmscriptenEnumTypeObject;
        UO_align_var_struct_span : EmscriptenEnumTypeObject;
        UO_align_var_struct_thresh : EmscriptenEnumTypeObject;
        UO_align_var_struct_gap : EmscriptenEnumTypeObject;
        UO_align_struct_init_span : EmscriptenEnumTypeObject;
        UO_align_typedef_gap : EmscriptenEnumTypeObject;
        UO_align_typedef_span : EmscriptenEnumTypeObject;
        UO_align_typedef_func : EmscriptenEnumTypeObject;
        UO_align_typedef_star_style : EmscriptenEnumTypeObject;
        UO_align_typedef_amp_style : EmscriptenEnumTypeObject;
        UO_align_right_cmt_span : EmscriptenEnumTypeObject;
        UO_align_right_cmt_mix : EmscriptenEnumTypeObject;
        UO_align_right_cmt_gap : EmscriptenEnumTypeObject;
        UO_align_right_cmt_at_col : EmscriptenEnumTypeObject;
        UO_align_func_proto_span : EmscriptenEnumTypeObject;
        UO_align_func_proto_gap : EmscriptenEnumTypeObject;
        UO_align_on_operator : EmscriptenEnumTypeObject;
        UO_align_mix_var_proto : EmscriptenEnumTypeObject;
        UO_align_single_line_func : EmscriptenEnumTypeObject;
        UO_align_single_line_brace : EmscriptenEnumTypeObject;
        UO_align_single_line_brace_gap : EmscriptenEnumTypeObject;
        UO_align_oc_msg_spec_span : EmscriptenEnumTypeObject;
        UO_align_nl_cont : EmscriptenEnumTypeObject;
        UO_align_pp_define_together : EmscriptenEnumTypeObject;
        UO_align_pp_define_gap : EmscriptenEnumTypeObject;
        UO_align_pp_define_span : EmscriptenEnumTypeObject;
        UO_align_left_shift : EmscriptenEnumTypeObject;
        UO_align_asm_colon : EmscriptenEnumTypeObject;
        UO_align_oc_msg_colon_span : EmscriptenEnumTypeObject;
        UO_align_oc_msg_colon_first : EmscriptenEnumTypeObject;
        UO_align_oc_decl_colon : EmscriptenEnumTypeObject;
        UO_cmt_width : EmscriptenEnumTypeObject;
        UO_cmt_reflow_mode : EmscriptenEnumTypeObject;
        UO_cmt_convert_tab_to_spaces : EmscriptenEnumTypeObject;
        UO_cmt_indent_multi : EmscriptenEnumTypeObject;
        UO_cmt_c_group : EmscriptenEnumTypeObject;
        UO_cmt_c_nl_start : EmscriptenEnumTypeObject;
        UO_cmt_c_nl_end : EmscriptenEnumTypeObject;
        UO_cmt_cpp_group : EmscriptenEnumTypeObject;
        UO_cmt_cpp_nl_start : EmscriptenEnumTypeObject;
        UO_cmt_cpp_nl_end : EmscriptenEnumTypeObject;
        UO_cmt_cpp_to_c : EmscriptenEnumTypeObject;
        UO_cmt_star_cont : EmscriptenEnumTypeObject;
        UO_cmt_sp_before_star_cont : EmscriptenEnumTypeObject;
        UO_cmt_sp_after_star_cont : EmscriptenEnumTypeObject;
        UO_cmt_multi_check_last : EmscriptenEnumTypeObject;
        UO_cmt_multi_first_len_minimum : EmscriptenEnumTypeObject;
        UO_cmt_insert_file_header : EmscriptenEnumTypeObject;
        UO_cmt_insert_file_footer : EmscriptenEnumTypeObject;
        UO_cmt_insert_func_header : EmscriptenEnumTypeObject;
        UO_cmt_insert_class_header : EmscriptenEnumTypeObject;
        UO_cmt_insert_oc_msg_header : EmscriptenEnumTypeObject;
        UO_cmt_insert_before_preproc : EmscriptenEnumTypeObject;
        UO_cmt_insert_before_inlines : EmscriptenEnumTypeObject;
        UO_cmt_insert_before_ctor_dtor : EmscriptenEnumTypeObject;
        UO_mod_full_brace_do : EmscriptenEnumTypeObject;
        UO_mod_full_brace_for : EmscriptenEnumTypeObject;
        UO_mod_full_brace_function : EmscriptenEnumTypeObject;
        UO_mod_full_brace_if : EmscriptenEnumTypeObject;
        UO_mod_full_brace_if_chain : EmscriptenEnumTypeObject;
        UO_mod_full_brace_if_chain_only : EmscriptenEnumTypeObject;
        UO_mod_full_brace_nl : EmscriptenEnumTypeObject;
        UO_mod_full_brace_nl_block_rem_mlcond : EmscriptenEnumTypeObject;
        UO_mod_full_brace_while : EmscriptenEnumTypeObject;
        UO_mod_full_brace_using : EmscriptenEnumTypeObject;
        UO_mod_paren_on_return : EmscriptenEnumTypeObject;
        UO_mod_pawn_semicolon : EmscriptenEnumTypeObject;
        UO_mod_full_paren_if_bool : EmscriptenEnumTypeObject;
        UO_mod_remove_extra_semicolon : EmscriptenEnumTypeObject;
        UO_mod_add_long_function_closebrace_comment : EmscriptenEnumTypeObject;
        UO_mod_add_long_namespace_closebrace_comment : EmscriptenEnumTypeObject;
        UO_mod_add_long_class_closebrace_comment : EmscriptenEnumTypeObject;
        UO_mod_add_long_switch_closebrace_comment : EmscriptenEnumTypeObject;
        UO_mod_add_long_ifdef_endif_comment : EmscriptenEnumTypeObject;
        UO_mod_add_long_ifdef_else_comment : EmscriptenEnumTypeObject;
        UO_mod_sort_import : EmscriptenEnumTypeObject;
        UO_mod_sort_using : EmscriptenEnumTypeObject;
        UO_mod_sort_include : EmscriptenEnumTypeObject;
        UO_mod_move_case_break : EmscriptenEnumTypeObject;
        UO_mod_case_brace : EmscriptenEnumTypeObject;
        UO_mod_remove_empty_return : EmscriptenEnumTypeObject;
        UO_mod_sort_oc_properties : EmscriptenEnumTypeObject;
        UO_mod_sort_oc_property_class_weight : EmscriptenEnumTypeObject;
        UO_mod_sort_oc_property_thread_safe_weight : EmscriptenEnumTypeObject;
        UO_mod_sort_oc_property_readwrite_weight : EmscriptenEnumTypeObject;
        UO_mod_sort_oc_property_reference_weight : EmscriptenEnumTypeObject;
        UO_mod_sort_oc_property_getter_weight : EmscriptenEnumTypeObject;
        UO_mod_sort_oc_property_setter_weight : EmscriptenEnumTypeObject;
        UO_mod_sort_oc_property_nullability_weight : EmscriptenEnumTypeObject;
        UO_pp_indent : EmscriptenEnumTypeObject;
        UO_pp_indent_at_level : EmscriptenEnumTypeObject;
        UO_pp_indent_count : EmscriptenEnumTypeObject;
        UO_pp_space : EmscriptenEnumTypeObject;
        UO_pp_space_count : EmscriptenEnumTypeObject;
        UO_pp_indent_region : EmscriptenEnumTypeObject;
        UO_pp_region_indent_code : EmscriptenEnumTypeObject;
        UO_pp_indent_if : EmscriptenEnumTypeObject;
        UO_pp_if_indent_code : EmscriptenEnumTypeObject;
        UO_pp_define_at_level : EmscriptenEnumTypeObject;
        UO_pp_ignore_define_body : EmscriptenEnumTypeObject;
        UO_pp_indent_case : EmscriptenEnumTypeObject;
        UO_pp_indent_func_def : EmscriptenEnumTypeObject;
        UO_pp_indent_extern : EmscriptenEnumTypeObject;
        UO_pp_indent_brace : EmscriptenEnumTypeObject;
        UO_include_category_0 : EmscriptenEnumTypeObject;
        UO_include_category_1 : EmscriptenEnumTypeObject;
        UO_include_category_2 : EmscriptenEnumTypeObject;
        UO_use_indent_func_call_param : EmscriptenEnumTypeObject;
        UO_use_indent_continue_only_once : EmscriptenEnumTypeObject;
        UO_use_options_overriding_for_qt_macros : EmscriptenEnumTypeObject;
        UO_warn_level_tabs_found_in_verbatim_string_literals : EmscriptenEnumTypeObject;
        UO_option_count : EmscriptenEnumTypeObject;
    }

    export interface Group extends EmscriptenEnumType
    {
        UG_general : EmscriptenEnumTypeObject;
        UG_space : EmscriptenEnumTypeObject;
        UG_indent : EmscriptenEnumTypeObject;
        UG_newline : EmscriptenEnumTypeObject;
        UG_blankline : EmscriptenEnumTypeObject;
        UG_position : EmscriptenEnumTypeObject;
        UG_linesplit : EmscriptenEnumTypeObject;
        UG_align : EmscriptenEnumTypeObject;
        UG_comment : EmscriptenEnumTypeObject;
        UG_codemodify : EmscriptenEnumTypeObject;
        UG_preprocessor : EmscriptenEnumTypeObject;
        UG_sort_includes : EmscriptenEnumTypeObject;
        UG_Use_Ext : EmscriptenEnumTypeObject;
        UG_warnlevels : EmscriptenEnumTypeObject;
        UG_group_count : EmscriptenEnumTypeObject;
    }

    export interface Argtype extends EmscriptenEnumType
    {
        AT_BOOL : EmscriptenEnumTypeObject;
        AT_IARF : EmscriptenEnumTypeObject;
        AT_NUM : EmscriptenEnumTypeObject;
        AT_LINE : EmscriptenEnumTypeObject;
        AT_POS : EmscriptenEnumTypeObject;
        AT_STRING : EmscriptenEnumTypeObject;
        AT_UNUM : EmscriptenEnumTypeObject;
        AT_TFI : EmscriptenEnumTypeObject;
    }

    export interface LogSev extends EmscriptenEnumType
    {
        LSYS : EmscriptenEnumTypeObject;
        LERR : EmscriptenEnumTypeObject;
        LWARN : EmscriptenEnumTypeObject;
        LNOTE : EmscriptenEnumTypeObject;
        LINFO : EmscriptenEnumTypeObject;
        LDATA : EmscriptenEnumTypeObject;
        LFILELIST : EmscriptenEnumTypeObject;
        LLINEENDS : EmscriptenEnumTypeObject;
        LCASTS : EmscriptenEnumTypeObject;
        LALBR : EmscriptenEnumTypeObject;
        LALTD : EmscriptenEnumTypeObject;
        LALPP : EmscriptenEnumTypeObject;
        LALPROTO : EmscriptenEnumTypeObject;
        LALNLC : EmscriptenEnumTypeObject;
        LALTC : EmscriptenEnumTypeObject;
        LALADD : EmscriptenEnumTypeObject;
        LALASS : EmscriptenEnumTypeObject;
        LFVD : EmscriptenEnumTypeObject;
        LFVD2 : EmscriptenEnumTypeObject;
        LINDENT : EmscriptenEnumTypeObject;
        LINDENT2 : EmscriptenEnumTypeObject;
        LINDPSE : EmscriptenEnumTypeObject;
        LINDPC : EmscriptenEnumTypeObject;
        LNEWLINE : EmscriptenEnumTypeObject;
        LPF : EmscriptenEnumTypeObject;
        LSTMT : EmscriptenEnumTypeObject;
        LTOK : EmscriptenEnumTypeObject;
        LALRC : EmscriptenEnumTypeObject;
        LCMTIND : EmscriptenEnumTypeObject;
        LINDLINE : EmscriptenEnumTypeObject;
        LSIB : EmscriptenEnumTypeObject;
        LRETURN : EmscriptenEnumTypeObject;
        LBRDEL : EmscriptenEnumTypeObject;
        LFCN : EmscriptenEnumTypeObject;
        LFCNP : EmscriptenEnumTypeObject;
        LPCU : EmscriptenEnumTypeObject;
        LDYNKW : EmscriptenEnumTypeObject;
        LOUTIND : EmscriptenEnumTypeObject;
        LBCSAFTER : EmscriptenEnumTypeObject;
        LBCSPOP : EmscriptenEnumTypeObject;
        LBCSPUSH : EmscriptenEnumTypeObject;
        LBCSSWAP : EmscriptenEnumTypeObject;
        LFTOR : EmscriptenEnumTypeObject;
        LAS : EmscriptenEnumTypeObject;
        LPPIS : EmscriptenEnumTypeObject;
        LTYPEDEF : EmscriptenEnumTypeObject;
        LVARDEF : EmscriptenEnumTypeObject;
        LDEFVAL : EmscriptenEnumTypeObject;
        LPVSEMI : EmscriptenEnumTypeObject;
        LPFUNC : EmscriptenEnumTypeObject;
        LSPLIT : EmscriptenEnumTypeObject;
        LFTYPE : EmscriptenEnumTypeObject;
        LTEMPL : EmscriptenEnumTypeObject;
        LPARADD : EmscriptenEnumTypeObject;
        LPARADD2 : EmscriptenEnumTypeObject;
        LBLANKD : EmscriptenEnumTypeObject;
        LTEMPFUNC : EmscriptenEnumTypeObject;
        LSCANSEMI : EmscriptenEnumTypeObject;
        LDELSEMI : EmscriptenEnumTypeObject;
        LFPARAM : EmscriptenEnumTypeObject;
        LNL1LINE : EmscriptenEnumTypeObject;
        LPFCHK : EmscriptenEnumTypeObject;
        LAVDB : EmscriptenEnumTypeObject;
        LSORT : EmscriptenEnumTypeObject;
        LSPACE : EmscriptenEnumTypeObject;
        LALIGN : EmscriptenEnumTypeObject;
        LALAGAIN : EmscriptenEnumTypeObject;
        LOPERATOR : EmscriptenEnumTypeObject;
        LASFCP : EmscriptenEnumTypeObject;
        LINDLINED : EmscriptenEnumTypeObject;
        LBCTRL : EmscriptenEnumTypeObject;
        LRMRETURN : EmscriptenEnumTypeObject;
        LPPIF : EmscriptenEnumTypeObject;
        LMCB : EmscriptenEnumTypeObject;
        LBRCH : EmscriptenEnumTypeObject;
        LFCNR : EmscriptenEnumTypeObject;
        LOCCLASS : EmscriptenEnumTypeObject;
        LOCMSG : EmscriptenEnumTypeObject;
        LBLANK : EmscriptenEnumTypeObject;
        LOBJCWORD : EmscriptenEnumTypeObject;
        LCHANGE : EmscriptenEnumTypeObject;
        LCONTTEXT : EmscriptenEnumTypeObject;
        LANNOT : EmscriptenEnumTypeObject;
        LOCBLK : EmscriptenEnumTypeObject;
        LFLPAREN : EmscriptenEnumTypeObject;
        LOCMSGD : EmscriptenEnumTypeObject;
        LINDENTAG : EmscriptenEnumTypeObject;
        LNFD : EmscriptenEnumTypeObject;
        LJDBI : EmscriptenEnumTypeObject;
        LSETPAR : EmscriptenEnumTypeObject;
        LSETTYP : EmscriptenEnumTypeObject;
        LSETFLG : EmscriptenEnumTypeObject;
        LNLFUNCT : EmscriptenEnumTypeObject;
        LCHUNK : EmscriptenEnumTypeObject;
        LGUY98 : EmscriptenEnumTypeObject;
        LGUY : EmscriptenEnumTypeObject;
    }

    export interface Token extends EmscriptenEnumType
    {
        CT_NONE : EmscriptenEnumTypeObject;
        CT_EOF : EmscriptenEnumTypeObject;
        CT_UNKNOWN : EmscriptenEnumTypeObject;
        CT_JUNK : EmscriptenEnumTypeObject;
        CT_WHITESPACE : EmscriptenEnumTypeObject;
        CT_SPACE : EmscriptenEnumTypeObject;
        CT_NEWLINE : EmscriptenEnumTypeObject;
        CT_NL_CONT : EmscriptenEnumTypeObject;
        CT_COMMENT_CPP : EmscriptenEnumTypeObject;
        CT_COMMENT : EmscriptenEnumTypeObject;
        CT_COMMENT_MULTI : EmscriptenEnumTypeObject;
        CT_COMMENT_EMBED : EmscriptenEnumTypeObject;
        CT_COMMENT_START : EmscriptenEnumTypeObject;
        CT_COMMENT_END : EmscriptenEnumTypeObject;
        CT_COMMENT_WHOLE : EmscriptenEnumTypeObject;
        CT_COMMENT_ENDIF : EmscriptenEnumTypeObject;
        CT_IGNORED : EmscriptenEnumTypeObject;
        CT_WORD : EmscriptenEnumTypeObject;
        CT_NUMBER : EmscriptenEnumTypeObject;
        CT_NUMBER_FP : EmscriptenEnumTypeObject;
        CT_STRING : EmscriptenEnumTypeObject;
        CT_STRING_MULTI : EmscriptenEnumTypeObject;
        CT_IF : EmscriptenEnumTypeObject;
        CT_ELSE : EmscriptenEnumTypeObject;
        CT_ELSEIF : EmscriptenEnumTypeObject;
        CT_FOR : EmscriptenEnumTypeObject;
        CT_WHILE : EmscriptenEnumTypeObject;
        CT_WHILE_OF_DO : EmscriptenEnumTypeObject;
        CT_SWITCH : EmscriptenEnumTypeObject;
        CT_CASE : EmscriptenEnumTypeObject;
        CT_DO : EmscriptenEnumTypeObject;
        CT_SYNCHRONIZED : EmscriptenEnumTypeObject;
        CT_VOLATILE : EmscriptenEnumTypeObject;
        CT_TYPEDEF : EmscriptenEnumTypeObject;
        CT_STRUCT : EmscriptenEnumTypeObject;
        CT_ENUM : EmscriptenEnumTypeObject;
        CT_ENUM_CLASS : EmscriptenEnumTypeObject;
        CT_SIZEOF : EmscriptenEnumTypeObject;
        CT_RETURN : EmscriptenEnumTypeObject;
        CT_BREAK : EmscriptenEnumTypeObject;
        CT_UNION : EmscriptenEnumTypeObject;
        CT_GOTO : EmscriptenEnumTypeObject;
        CT_CONTINUE : EmscriptenEnumTypeObject;
        CT_C_CAST : EmscriptenEnumTypeObject;
        CT_CPP_CAST : EmscriptenEnumTypeObject;
        CT_D_CAST : EmscriptenEnumTypeObject;
        CT_TYPE_CAST : EmscriptenEnumTypeObject;
        CT_TYPENAME : EmscriptenEnumTypeObject;
        CT_TEMPLATE : EmscriptenEnumTypeObject;
        CT_ASSIGN : EmscriptenEnumTypeObject;
        CT_ASSIGN_NL : EmscriptenEnumTypeObject;
        CT_SASSIGN : EmscriptenEnumTypeObject;
        CT_COMPARE : EmscriptenEnumTypeObject;
        CT_SCOMPARE : EmscriptenEnumTypeObject;
        CT_BOOL : EmscriptenEnumTypeObject;
        CT_SBOOL : EmscriptenEnumTypeObject;
        CT_ARITH : EmscriptenEnumTypeObject;
        CT_SARITH : EmscriptenEnumTypeObject;
        CT_CARET : EmscriptenEnumTypeObject;
        CT_DEREF : EmscriptenEnumTypeObject;
        CT_INCDEC_BEFORE : EmscriptenEnumTypeObject;
        CT_INCDEC_AFTER : EmscriptenEnumTypeObject;
        CT_MEMBER : EmscriptenEnumTypeObject;
        CT_DC_MEMBER : EmscriptenEnumTypeObject;
        CT_C99_MEMBER : EmscriptenEnumTypeObject;
        CT_INV : EmscriptenEnumTypeObject;
        CT_DESTRUCTOR : EmscriptenEnumTypeObject;
        CT_NOT : EmscriptenEnumTypeObject;
        CT_D_TEMPLATE : EmscriptenEnumTypeObject;
        CT_ADDR : EmscriptenEnumTypeObject;
        CT_NEG : EmscriptenEnumTypeObject;
        CT_POS : EmscriptenEnumTypeObject;
        CT_STAR : EmscriptenEnumTypeObject;
        CT_PLUS : EmscriptenEnumTypeObject;
        CT_MINUS : EmscriptenEnumTypeObject;
        CT_AMP : EmscriptenEnumTypeObject;
        CT_BYREF : EmscriptenEnumTypeObject;
        CT_POUND : EmscriptenEnumTypeObject;
        CT_PREPROC : EmscriptenEnumTypeObject;
        CT_PREPROC_INDENT : EmscriptenEnumTypeObject;
        CT_PREPROC_BODY : EmscriptenEnumTypeObject;
        CT_PP : EmscriptenEnumTypeObject;
        CT_ELLIPSIS : EmscriptenEnumTypeObject;
        CT_RANGE : EmscriptenEnumTypeObject;
        CT_NULLCOND : EmscriptenEnumTypeObject;
        CT_SEMICOLON : EmscriptenEnumTypeObject;
        CT_VSEMICOLON : EmscriptenEnumTypeObject;
        CT_COLON : EmscriptenEnumTypeObject;
        CT_ASM_COLON : EmscriptenEnumTypeObject;
        CT_CASE_COLON : EmscriptenEnumTypeObject;
        CT_CLASS_COLON : EmscriptenEnumTypeObject;
        CT_CONSTR_COLON : EmscriptenEnumTypeObject;
        CT_D_ARRAY_COLON : EmscriptenEnumTypeObject;
        CT_COND_COLON : EmscriptenEnumTypeObject;
        CT_QUESTION : EmscriptenEnumTypeObject;
        CT_COMMA : EmscriptenEnumTypeObject;
        CT_ASM : EmscriptenEnumTypeObject;
        CT_ATTRIBUTE : EmscriptenEnumTypeObject;
        CT_CATCH : EmscriptenEnumTypeObject;
        CT_WHEN : EmscriptenEnumTypeObject;
        CT_CLASS : EmscriptenEnumTypeObject;
        CT_DELETE : EmscriptenEnumTypeObject;
        CT_EXPORT : EmscriptenEnumTypeObject;
        CT_FRIEND : EmscriptenEnumTypeObject;
        CT_NAMESPACE : EmscriptenEnumTypeObject;
        CT_PACKAGE : EmscriptenEnumTypeObject;
        CT_NEW : EmscriptenEnumTypeObject;
        CT_OPERATOR : EmscriptenEnumTypeObject;
        CT_OPERATOR_VAL : EmscriptenEnumTypeObject;
        CT_PRIVATE : EmscriptenEnumTypeObject;
        CT_PRIVATE_COLON : EmscriptenEnumTypeObject;
        CT_THROW : EmscriptenEnumTypeObject;
        CT_NOEXCEPT : EmscriptenEnumTypeObject;
        CT_TRY : EmscriptenEnumTypeObject;
        CT_BRACED_INIT_LIST : EmscriptenEnumTypeObject;
        CT_USING : EmscriptenEnumTypeObject;
        CT_USING_STMT : EmscriptenEnumTypeObject;
        CT_D_WITH : EmscriptenEnumTypeObject;
        CT_D_MODULE : EmscriptenEnumTypeObject;
        CT_SUPER : EmscriptenEnumTypeObject;
        CT_DELEGATE : EmscriptenEnumTypeObject;
        CT_BODY : EmscriptenEnumTypeObject;
        CT_DEBUG : EmscriptenEnumTypeObject;
        CT_DEBUGGER : EmscriptenEnumTypeObject;
        CT_INVARIANT : EmscriptenEnumTypeObject;
        CT_UNITTEST : EmscriptenEnumTypeObject;
        CT_UNSAFE : EmscriptenEnumTypeObject;
        CT_FINALLY : EmscriptenEnumTypeObject;
        CT_IMPORT : EmscriptenEnumTypeObject;
        CT_D_SCOPE : EmscriptenEnumTypeObject;
        CT_D_SCOPE_IF : EmscriptenEnumTypeObject;
        CT_LAZY : EmscriptenEnumTypeObject;
        CT_D_MACRO : EmscriptenEnumTypeObject;
        CT_D_VERSION : EmscriptenEnumTypeObject;
        CT_D_VERSION_IF : EmscriptenEnumTypeObject;
        CT_PAREN_OPEN : EmscriptenEnumTypeObject;
        CT_PAREN_CLOSE : EmscriptenEnumTypeObject;
        CT_ANGLE_OPEN : EmscriptenEnumTypeObject;
        CT_ANGLE_CLOSE : EmscriptenEnumTypeObject;
        CT_SPAREN_OPEN : EmscriptenEnumTypeObject;
        CT_SPAREN_CLOSE : EmscriptenEnumTypeObject;
        CT_FPAREN_OPEN : EmscriptenEnumTypeObject;
        CT_FPAREN_CLOSE : EmscriptenEnumTypeObject;
        CT_TPAREN_OPEN : EmscriptenEnumTypeObject;
        CT_TPAREN_CLOSE : EmscriptenEnumTypeObject;
        CT_BRACE_OPEN : EmscriptenEnumTypeObject;
        CT_BRACE_CLOSE : EmscriptenEnumTypeObject;
        CT_VBRACE_OPEN : EmscriptenEnumTypeObject;
        CT_VBRACE_CLOSE : EmscriptenEnumTypeObject;
        CT_SQUARE_OPEN : EmscriptenEnumTypeObject;
        CT_SQUARE_CLOSE : EmscriptenEnumTypeObject;
        CT_TSQUARE : EmscriptenEnumTypeObject;
        CT_MACRO_OPEN : EmscriptenEnumTypeObject;
        CT_MACRO_CLOSE : EmscriptenEnumTypeObject;
        CT_MACRO_ELSE : EmscriptenEnumTypeObject;
        CT_LABEL : EmscriptenEnumTypeObject;
        CT_LABEL_COLON : EmscriptenEnumTypeObject;
        CT_FUNCTION : EmscriptenEnumTypeObject;
        CT_FUNC_CALL : EmscriptenEnumTypeObject;
        CT_FUNC_CALL_USER : EmscriptenEnumTypeObject;
        CT_FUNC_DEF : EmscriptenEnumTypeObject;
        CT_FUNC_TYPE : EmscriptenEnumTypeObject;
        CT_FUNC_VAR : EmscriptenEnumTypeObject;
        CT_FUNC_PROTO : EmscriptenEnumTypeObject;
        CT_FUNC_CLASS_DEF : EmscriptenEnumTypeObject;
        CT_FUNC_CLASS_PROTO : EmscriptenEnumTypeObject;
        CT_FUNC_CTOR_VAR : EmscriptenEnumTypeObject;
        CT_FUNC_WRAP : EmscriptenEnumTypeObject;
        CT_PROTO_WRAP : EmscriptenEnumTypeObject;
        CT_MACRO_FUNC : EmscriptenEnumTypeObject;
        CT_MACRO : EmscriptenEnumTypeObject;
        CT_QUALIFIER : EmscriptenEnumTypeObject;
        CT_EXTERN : EmscriptenEnumTypeObject;
        CT_ALIGN : EmscriptenEnumTypeObject;
        CT_TYPE : EmscriptenEnumTypeObject;
        CT_PTR_TYPE : EmscriptenEnumTypeObject;
        CT_TYPE_WRAP : EmscriptenEnumTypeObject;
        CT_CPP_LAMBDA : EmscriptenEnumTypeObject;
        CT_CPP_LAMBDA_RET : EmscriptenEnumTypeObject;
        CT_BIT_COLON : EmscriptenEnumTypeObject;
        CT_OC_DYNAMIC : EmscriptenEnumTypeObject;
        CT_OC_END : EmscriptenEnumTypeObject;
        CT_OC_IMPL : EmscriptenEnumTypeObject;
        CT_OC_INTF : EmscriptenEnumTypeObject;
        CT_OC_PROTOCOL : EmscriptenEnumTypeObject;
        CT_OC_PROTO_LIST : EmscriptenEnumTypeObject;
        CT_OC_GENERIC_SPEC : EmscriptenEnumTypeObject;
        CT_OC_PROPERTY : EmscriptenEnumTypeObject;
        CT_OC_CLASS : EmscriptenEnumTypeObject;
        CT_OC_CLASS_EXT : EmscriptenEnumTypeObject;
        CT_OC_CATEGORY : EmscriptenEnumTypeObject;
        CT_OC_SCOPE : EmscriptenEnumTypeObject;
        CT_OC_MSG : EmscriptenEnumTypeObject;
        CT_OC_MSG_CLASS : EmscriptenEnumTypeObject;
        CT_OC_MSG_FUNC : EmscriptenEnumTypeObject;
        CT_OC_MSG_NAME : EmscriptenEnumTypeObject;
        CT_OC_MSG_SPEC : EmscriptenEnumTypeObject;
        CT_OC_MSG_DECL : EmscriptenEnumTypeObject;
        CT_OC_RTYPE : EmscriptenEnumTypeObject;
        CT_OC_ATYPE : EmscriptenEnumTypeObject;
        CT_OC_COLON : EmscriptenEnumTypeObject;
        CT_OC_DICT_COLON : EmscriptenEnumTypeObject;
        CT_OC_SEL : EmscriptenEnumTypeObject;
        CT_OC_SEL_NAME : EmscriptenEnumTypeObject;
        CT_OC_BLOCK : EmscriptenEnumTypeObject;
        CT_OC_BLOCK_ARG : EmscriptenEnumTypeObject;
        CT_OC_BLOCK_TYPE : EmscriptenEnumTypeObject;
        CT_OC_BLOCK_EXPR : EmscriptenEnumTypeObject;
        CT_OC_BLOCK_CARET : EmscriptenEnumTypeObject;
        CT_OC_AT : EmscriptenEnumTypeObject;
        CT_OC_PROPERTY_ATTR : EmscriptenEnumTypeObject;
        CT_PP_DEFINE : EmscriptenEnumTypeObject;
        CT_PP_DEFINED : EmscriptenEnumTypeObject;
        CT_PP_INCLUDE : EmscriptenEnumTypeObject;
        CT_PP_IF : EmscriptenEnumTypeObject;
        CT_PP_ELSE : EmscriptenEnumTypeObject;
        CT_PP_ENDIF : EmscriptenEnumTypeObject;
        CT_PP_ASSERT : EmscriptenEnumTypeObject;
        CT_PP_EMIT : EmscriptenEnumTypeObject;
        CT_PP_ENDINPUT : EmscriptenEnumTypeObject;
        CT_PP_ERROR : EmscriptenEnumTypeObject;
        CT_PP_FILE : EmscriptenEnumTypeObject;
        CT_PP_LINE : EmscriptenEnumTypeObject;
        CT_PP_SECTION : EmscriptenEnumTypeObject;
        CT_PP_ASM : EmscriptenEnumTypeObject;
        CT_PP_UNDEF : EmscriptenEnumTypeObject;
        CT_PP_PROPERTY : EmscriptenEnumTypeObject;
        CT_PP_BODYCHUNK : EmscriptenEnumTypeObject;
        CT_PP_PRAGMA : EmscriptenEnumTypeObject;
        CT_PP_REGION : EmscriptenEnumTypeObject;
        CT_PP_ENDREGION : EmscriptenEnumTypeObject;
        CT_PP_REGION_INDENT : EmscriptenEnumTypeObject;
        CT_PP_IF_INDENT : EmscriptenEnumTypeObject;
        CT_PP_IGNORE : EmscriptenEnumTypeObject;
        CT_PP_OTHER : EmscriptenEnumTypeObject;
        CT_CHAR : EmscriptenEnumTypeObject;
        CT_DEFINED : EmscriptenEnumTypeObject;
        CT_FORWARD : EmscriptenEnumTypeObject;
        CT_NATIVE : EmscriptenEnumTypeObject;
        CT_STATE : EmscriptenEnumTypeObject;
        CT_STOCK : EmscriptenEnumTypeObject;
        CT_TAGOF : EmscriptenEnumTypeObject;
        CT_DOT : EmscriptenEnumTypeObject;
        CT_TAG : EmscriptenEnumTypeObject;
        CT_TAG_COLON : EmscriptenEnumTypeObject;
        CT_LOCK : EmscriptenEnumTypeObject;
        CT_AS : EmscriptenEnumTypeObject;
        CT_IN : EmscriptenEnumTypeObject;
        CT_BRACED : EmscriptenEnumTypeObject;
        CT_THIS : EmscriptenEnumTypeObject;
        CT_BASE : EmscriptenEnumTypeObject;
        CT_DEFAULT : EmscriptenEnumTypeObject;
        CT_GETSET : EmscriptenEnumTypeObject;
        CT_GETSET_EMPTY : EmscriptenEnumTypeObject;
        CT_CONCAT : EmscriptenEnumTypeObject;
        CT_CS_SQ_STMT : EmscriptenEnumTypeObject;
        CT_CS_SQ_COLON : EmscriptenEnumTypeObject;
        CT_CS_PROPERTY : EmscriptenEnumTypeObject;
        CT_SQL_EXEC : EmscriptenEnumTypeObject;
        CT_SQL_BEGIN : EmscriptenEnumTypeObject;
        CT_SQL_END : EmscriptenEnumTypeObject;
        CT_SQL_WORD : EmscriptenEnumTypeObject;
        CT_CONSTRUCT : EmscriptenEnumTypeObject;
        CT_LAMBDA : EmscriptenEnumTypeObject;
        CT_ASSERT : EmscriptenEnumTypeObject;
        CT_ANNOTATION : EmscriptenEnumTypeObject;
        CT_FOR_COLON : EmscriptenEnumTypeObject;
        CT_DOUBLE_BRACE : EmscriptenEnumTypeObject;
        CT_Q_EMIT : EmscriptenEnumTypeObject;
        CT_Q_FOREACH : EmscriptenEnumTypeObject;
        CT_Q_FOREVER : EmscriptenEnumTypeObject;
        CT_Q_GADGET : EmscriptenEnumTypeObject;
        CT_Q_OBJECT : EmscriptenEnumTypeObject;
        CT_MODE : EmscriptenEnumTypeObject;
        CT_DI : EmscriptenEnumTypeObject;
        CT_HI : EmscriptenEnumTypeObject;
        CT_QI : EmscriptenEnumTypeObject;
        CT_SI : EmscriptenEnumTypeObject;
        CT_NOTHROW : EmscriptenEnumTypeObject;
        CT_WORD_ : EmscriptenEnumTypeObject;
        CT_TOKEN_COUNT_ : EmscriptenEnumTypeObject;
    }

    export interface LangFlag extends EmscriptenEnumType
    {
        LANG_C : EmscriptenEnumTypeObject;
        LANG_CPP : EmscriptenEnumTypeObject;
        LANG_D : EmscriptenEnumTypeObject;
        LANG_CS : EmscriptenEnumTypeObject;
        LANG_JAVA : EmscriptenEnumTypeObject;
        LANG_OC : EmscriptenEnumTypeObject;
        LANG_VALA : EmscriptenEnumTypeObject;
        LANG_PAWN : EmscriptenEnumTypeObject;
        LANG_ECMA : EmscriptenEnumTypeObject;
        LANG_ALLC : EmscriptenEnumTypeObject;
        LANG_ALL : EmscriptenEnumTypeObject;
        FLAG_DIG : EmscriptenEnumTypeObject;
        FLAG_PP : EmscriptenEnumTypeObject;
    }

    // endregion enum bindings
    // </editor-fold>

    //! interface for Emscriptens group_map value pair
    export interface group_map_value
    {
        //! group enum value object
        id : EmscriptenEnumTypeObject;
        //! list of options enum value objects
        options : EmscriptenVector<EmscriptenEnumTypeObject>;
    }

    //! interface for Emscriptens group_map value pair
    export interface option_map_value
    {
        //! option enum value object
        id : EmscriptenEnumTypeObject;
        //! group enum value object
        group_id :EmscriptenEnumTypeObject;

        //! option argument type enum value object
        type : EmscriptenEnumTypeObject;

        //! option min value
        min_val : number;
        //! option max value
        max_val : number;
        //! option name
        name : string;
        //! option short description
        short_desc : string;
        //! option extra description
        long_desc : string;
    }

    export interface Uncrustify
    {
        lang_flag_e : LangFlag;
        argtype_e : Argtype;
        uncrustify_options : Option;
        uncrustify_groups : Group;
        log_sev_t : LogSev;

        //! sets all option values to their default values
        set_option_defaults() : void;

        //! adds a new keyword to Uncrustifys dynamic keyword map (dkwm, keywords.cpp)
        add_keyword( tag : string, type : Token ) : void

        //! removes a keyword from Uncrustifys dynamic keyword map (dkwm, keywords.cpp)
        remove_keyword( tag : string )

        // clears Uncrustifys dynamic keyword map (dkwm, keywords.cpp)
        clear_keywords() : void;

        /**
        * Adds an entry to the define list
        *
        * @param tag        tag string
        * @param value      value of the define
        */
        add_define( tag : string, value : string ) : void;

        /**
        * Adds an entry to the define list
        *
        * @param tag        tag string
        */
        add_define( tag : string ) : void;

        /**
        * Show or hide the severity prefix "<1>"
        *
        * @param b true=show  false=hide
        */
        show_log_type( b : boolean ) : void;

        //! returns the UNCRUSTIFY_VERSION string
        get_version() : string;

        //! disables all logging messages
        set_quiet() : void;

        /**
        * sets value of an option
        *
        * @param name   name of the option
        * @param value  value that is going to be set
        * @return options enum value of the found option or -1 if option was not found
        */
        set_option( name : string, value : string ) : number;

        /**
        * returns value of an option
        *
        * @param name   name of the option
        * @return currently set value of the option
        */
        get_option( name : string ) : string;

        //! returns a string with option documentation
        show_options() : string;

        /**
        * returns the config file string based on the current configuration
        *
        * @param withDoc  false= without documentation true=with documentation text lines
        * @param only_not_default  false=containing all options true=containing only options with non default values
        * @return returns the config file string based on the current configuration
        */
        show_config( withDoc : boolean, only_not_default : boolean ) : string;

        /**
        * returns the config file string with all options based on the current configuration
        *
        * @param withDoc  false= without documentation true=with documentation text lines
        * @return returns the config file string with all options based on the current configuration
        */
        show_config( withDoc : boolean ) : string;

        /**
        * returns the config file string with all options and without documentation based on the current configuration
        *
        * @return returns the config file string with all options without documentation based on the current configuration
        */
        show_config() : string;

        //! initializes the current libUncrustify instance
        initialize() : void;

        //! destroys the current libUncrustify instance
        destruct() : void;

        /**
        * reads option file string, sets the defined options
        *
        * @return returns EXIT_SUCCESS on success
        */
        loadConfig( cfg : string ) : number;

        /**
         * format text
         *
         * @param file file string that is going to be formated
         * @param lang specifies in which language the input file is written (see LangFlag)
         * @param frag [optional] true=fragmented code input
         *                        false=unfragmented code input [default]
         *
         * @return formatted file string
         */
        uncrustify( file : string, lang : EmscriptenEnumTypeObject, frag : boolean ) : string;
        uncrustify( file : string, lang : EmscriptenEnumTypeObject ) : string;

        /**
         * generate debug output
         *
         * @param file file string that is going to be formated
         * @param lang specifies in which language the input file is written (see LangFlag)
         * @param frag [optional] true=fragmented code input
         *                        false=unfragmented code input [default]
         *
         * @return debug output string
         */
        debug( file : string, lang : EmscriptenEnumTypeObject, frag : boolean ) : string;
        debug( file : string, lang : EmscriptenEnumTypeObject ) : string;

        //! clears defines map  (defines, defines.cpp)
        clear_defines() : void;

        //! returns a copy of the current option_name_map
        getOptionNameMap() : EmscriptenMap<EmscriptenEnumTypeObject, option_map_value>;

        //! returns a copy of the current group_map
        getGroupMap() : EmscriptenMap<EmscriptenEnumTypeObject, group_map_value>;
    }

    var Uncrustify : {
        (module?: Object): Uncrustify;
        new (module?: Object): Uncrustify;
    };
}

declare var uncrustify : LibUncrustify.Uncrustify;

declare module "libUncrustify"
{
    export = uncrustify;
}
