/**
 * @file options.cpp
 * Parses the options from the config file.
 *
 * $Id$
 */

#define DEFINE_OPTION_NAME_MAP

#include "uncrustify_types.h"
#include "args.h"
#include "prototypes.h"
#include <cstring>
#ifdef HAVE_STRINGS_H
#include <strings.h> /* strcasecmp() */
#endif
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cctype>


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
                    const char *short_desc, const char *long_desc)
{
   group_map[current_group].options.push_back(id);

   option_map_value value;

   value.id              = id;
   value.group_id        = current_group;
   value.type            = type;
   value.name            = name;
   value.short_desc      = short_desc;
   value.long_desc       = long_desc;
   option_name_map[name] = value;

   int name_len = strlen(name);
   if (name_len > cpd.max_option_name_len)
   {
      cpd.max_option_name_len = name_len;
   }
}

const option_map_value *unc_find_option(const char *name)
{
   if (option_name_map.find(name) == option_name_map.end())
   {
      return(NULL);
   }
   return(&option_name_map[name]);
}

void register_options(void)
{
   unc_begin_group(UG_general, "General options");
   unc_add_option("input_tab_size", UO_input_tab_size, AT_NUM, "Sets the size of the tab on input. Usually 8.");
   unc_add_option("newlines", UO_newlines, AT_LINE);
   unc_add_option("output_tab_size", UO_output_tab_size, AT_NUM, "Sets the output tab size. Only used if using tabs for aligning. Usually 8.");
   unc_add_option("string_escape_char", UO_string_escape_char, AT_NUM);

   unc_begin_group(UG_align, "Code alignment (not left column spaces/tabs)");
   unc_add_option("align_assign_span", UO_align_assign_span, AT_NUM);
   unc_add_option("align_assign_thresh", UO_align_assign_thresh, AT_NUM);
   unc_add_option("align_enum_equ_span", UO_align_enum_equ_span, AT_NUM);
   unc_add_option("align_enum_equ_thresh", UO_align_enum_equ_thresh, AT_NUM);
   unc_add_option("align_func_proto_span", UO_align_func_proto_span, AT_NUM);
   unc_add_option("align_keep_tabs", UO_align_keep_tabs, AT_BOOL);
   unc_add_option("align_nl_cont", UO_align_nl_cont, AT_BOOL);
   unc_add_option("align_number_left", UO_align_number_left, AT_BOOL);
   unc_add_option("align_on_tabstop", UO_align_on_tabstop, AT_BOOL);
   unc_add_option("align_pp_define_gap", UO_align_pp_define_gap, AT_NUM);
   unc_add_option("align_pp_define_span", UO_align_pp_define_span, AT_NUM);
   unc_add_option("align_right_cmt_span", UO_align_right_cmt_span, AT_NUM);
   unc_add_option("align_struct_init_span", UO_align_struct_init_span, AT_NUM);
   unc_add_option("align_typedef_gap", UO_align_typedef_gap, AT_NUM);
   unc_add_option("align_typedef_span", UO_align_typedef_span, AT_NUM);
   unc_add_option("align_typedef_star_style", UO_align_typedef_star_style, AT_NUM);
   unc_add_option("align_var_def_colon", UO_align_var_def_colon, AT_BOOL);
   unc_add_option("align_var_def_inline", UO_align_var_def_inline, AT_BOOL);
   unc_add_option("align_var_def_span", UO_align_var_def_span, AT_NUM);
   unc_add_option("align_var_def_star", UO_align_var_def_star, AT_BOOL);
   unc_add_option("align_var_def_thresh", UO_align_var_def_thresh, AT_NUM);
   unc_add_option("align_var_struct_span", UO_align_var_struct_span, AT_NUM);
   unc_add_option("align_with_tabs", UO_align_with_tabs, AT_BOOL);

   unc_begin_group(UG_comment, "Comment modifications");
   unc_add_option("cmt_cpp_group", UO_cmt_cpp_group, AT_BOOL);
   unc_add_option("cmt_cpp_nl_end", UO_cmt_cpp_nl_end, AT_BOOL);
   unc_add_option("cmt_cpp_nl_start", UO_cmt_cpp_nl_start, AT_BOOL);
   unc_add_option("cmt_cpp_to_c", UO_cmt_cpp_to_c, AT_BOOL);
   unc_add_option("cmt_star_cont", UO_cmt_star_cont, AT_BOOL);

   unc_begin_group(UG_blankline, "Blank line options", "Note that it takes 2 newlines to get a blank line.");
   unc_add_option("eat_blanks_after_open_brace", UO_eat_blanks_after_open_brace, AT_BOOL);
   unc_add_option("eat_blanks_before_close_brace", UO_eat_blanks_before_close_brace, AT_BOOL);
   unc_add_option("nl_after_func_body", UO_nl_after_func_body, AT_NUM);
   unc_add_option("nl_after_func_proto", UO_nl_after_func_proto, AT_NUM);
   unc_add_option("nl_after_func_proto_group", UO_nl_after_func_proto_group, AT_NUM);
   unc_add_option("nl_before_block_comment", UO_nl_before_block_comment, AT_NUM);
   unc_add_option("nl_max", UO_nl_max, AT_NUM);

   unc_begin_group(UG_indent, "Indenting");
   unc_add_option("indent_align_string", UO_indent_align_string, AT_BOOL);
   unc_add_option("indent_brace", UO_indent_brace, AT_NUM);
   unc_add_option("indent_braces", UO_indent_braces, AT_BOOL);
   unc_add_option("indent_case_body", UO_indent_case_body, AT_NUM);
   unc_add_option("indent_case_brace", UO_indent_case_brace, AT_NUM);
   unc_add_option("indent_class", UO_indent_class, AT_BOOL);
   unc_add_option("indent_class_colon", UO_indent_class_colon, AT_BOOL);
   unc_add_option("indent_col1_comment", UO_indent_col1_comment, AT_BOOL);
   unc_add_option("indent_columns", UO_indent_columns, AT_NUM, "The number of columns to indent per level. Usually 2, 3, 4, or 8.");
   unc_add_option("indent_func_call_param", UO_indent_func_call_param, AT_BOOL);
   unc_add_option("indent_label", UO_indent_label, AT_NUM);
   unc_add_option("indent_member", UO_indent_member, AT_NUM);
   unc_add_option("indent_namespace", UO_indent_namespace, AT_BOOL);
   unc_add_option("indent_paren_nl", UO_indent_paren_nl, AT_BOOL);
   unc_add_option("indent_square_nl", UO_indent_square_nl, AT_BOOL);
   unc_add_option("indent_switch_case", UO_indent_switch_case, AT_NUM);
   unc_add_option("indent_with_tabs", UO_indent_with_tabs, AT_NUM, "0=Spaces only. 1=Indent with tabs, align with spaces. 2=indent and align with tabs.");

   unc_begin_group(UG_codemodify, "Code modifying options (non-whitespace)");
   unc_add_option("mod_full_brace_do", UO_mod_full_brace_do, AT_IARF);
   unc_add_option("mod_full_brace_for", UO_mod_full_brace_for, AT_IARF);
   unc_add_option("mod_full_brace_function", UO_mod_full_brace_function, AT_IARF);
   unc_add_option("mod_full_brace_if", UO_mod_full_brace_if, AT_IARF);
   unc_add_option("mod_full_brace_nl", UO_mod_full_brace_nl, AT_NUM);
   unc_add_option("mod_full_brace_while", UO_mod_full_brace_while, AT_IARF);
   unc_add_option("mod_paren_on_return", UO_mod_paren_on_return, AT_IARF);
   unc_add_option("mod_pawn_semicolon", UO_mod_pawn_semicolon, AT_BOOL);

   unc_begin_group(UG_newline, "Newline adding and removing options");
   unc_add_option("code_width", UO_code_width, AT_NUM, "Try to limit code width to this number of columns");
   unc_add_option("nl_after_case", UO_nl_after_case, AT_BOOL);
   unc_add_option("nl_after_return", UO_nl_after_return, AT_BOOL);
   unc_add_option("nl_assign_brace", UO_nl_assign_brace, AT_IARF);
   unc_add_option("nl_before_case", UO_nl_before_case, AT_BOOL);
   unc_add_option("nl_brace_else", UO_nl_brace_else, AT_IARF);
   unc_add_option("nl_brace_while", UO_nl_brace_while, AT_IARF);
   unc_add_option("nl_class_brace", UO_nl_class_brace, AT_IARF);
   unc_add_option("nl_class_init_args", UO_nl_class_init_args, AT_IARF);
   unc_add_option("nl_collapse_empty_body", UO_nl_collapse_empty_body, AT_BOOL);
   unc_add_option("nl_define_macro", UO_nl_define_macro, AT_BOOL);
   unc_add_option("nl_do_brace", UO_nl_do_brace, AT_IARF);
   unc_add_option("nl_else_brace", UO_nl_else_brace, AT_IARF);
   unc_add_option("nl_elseif_brace", UO_nl_elseif_brace, AT_IARF);
   unc_add_option("nl_end_of_file", UO_nl_end_of_file, AT_IARF);
   unc_add_option("nl_end_of_file_min", UO_nl_end_of_file_min, AT_NUM);
   unc_add_option("nl_enum_brace", UO_nl_enum_brace, AT_IARF);
   unc_add_option("nl_fcall_brace", UO_nl_fcall_brace, AT_IARF);
   unc_add_option("nl_fdef_brace", UO_nl_fdef_brace, AT_IARF);
   unc_add_option("nl_for_brace", UO_nl_for_brace, AT_IARF);
   unc_add_option("nl_func_decl_args", UO_nl_func_decl_args, AT_IARF);
   unc_add_option("nl_func_decl_end", UO_nl_func_decl_end, AT_IARF);
   unc_add_option("nl_func_decl_start", UO_nl_func_decl_start, AT_IARF);
   unc_add_option("nl_func_type_name", UO_nl_func_type_name, AT_IARF);
   unc_add_option("nl_func_var_def_blk", UO_nl_func_var_def_blk, AT_NUM);
   unc_add_option("nl_if_brace", UO_nl_if_brace, AT_IARF);
   unc_add_option("nl_namespace_brace", UO_nl_namespace_brace, AT_IARF);
   unc_add_option("nl_squeeze_ifdef", UO_nl_squeeze_ifdef, AT_BOOL);
   unc_add_option("nl_start_of_file", UO_nl_start_of_file, AT_IARF);
   unc_add_option("nl_start_of_file_min", UO_nl_start_of_file_min, AT_NUM);
   unc_add_option("nl_struct_brace", UO_nl_struct_brace, AT_IARF);
   unc_add_option("nl_switch_brace", UO_nl_switch_brace, AT_IARF);
   unc_add_option("nl_template_class", UO_nl_template_class, AT_IARF);
   unc_add_option("nl_union_brace", UO_nl_union_brace, AT_IARF);
   unc_add_option("nl_while_brace", UO_nl_while_brace, AT_IARF);

   unc_begin_group(UG_position, "Positioning options");
   unc_add_option("pos_bool", UO_pos_bool, AT_POS);
   unc_add_option("pos_class_colon", UO_pos_class_colon, AT_POS);

   unc_begin_group(UG_preprocessor, "Preprocessor options");
   unc_add_option("pp_indent", UO_pp_indent, AT_IARF);
   unc_add_option("pp_space", UO_pp_space, AT_IARF);

   unc_begin_group(UG_space, "Spacing options");
   unc_add_option("sp_after_angle", UO_sp_after_angle, AT_IARF);
   unc_add_option("sp_after_byref", UO_sp_after_byref, AT_IARF);
   unc_add_option("sp_after_cast", UO_sp_after_cast, AT_IARF);
   unc_add_option("sp_after_comma", UO_sp_after_comma, AT_IARF);
   unc_add_option("sp_after_operator", UO_sp_after_operator, AT_IARF);
   unc_add_option("sp_after_ptr_star", UO_sp_after_ptr_star, AT_IARF);
   unc_add_option("sp_after_sparen", UO_sp_after_sparen, AT_IARF);
   unc_add_option("sp_after_tag", UO_sp_after_tag, AT_IARF);
   unc_add_option("sp_arith", UO_sp_arith, AT_IARF);
   unc_add_option("sp_assign", UO_sp_assign, AT_IARF);
   unc_add_option("sp_before_angle", UO_sp_before_angle, AT_IARF);
   unc_add_option("sp_before_byref", UO_sp_before_byref, AT_IARF);
   unc_add_option("sp_before_ptr_star", UO_sp_before_ptr_star, AT_IARF);
   unc_add_option("sp_before_semi", UO_sp_before_semi, AT_IARF);
   unc_add_option("sp_before_sparen", UO_sp_before_sparen, AT_IARF);
   unc_add_option("sp_before_square", UO_sp_before_square, AT_IARF);
   unc_add_option("sp_before_squares", UO_sp_before_squares, AT_IARF);
   unc_add_option("sp_between_ptr_star", UO_sp_between_ptr_star, AT_IARF);
   unc_add_option("sp_bool", UO_sp_bool, AT_IARF);
   unc_add_option("sp_compare", UO_sp_compare, AT_IARF);
   unc_add_option("sp_fparen_brace", UO_sp_fparen_brace, AT_IARF, "Space between ')' and '{' of function");
   unc_add_option("sp_func_call_paren", UO_sp_func_call_paren, AT_IARF);
   unc_add_option("sp_func_class_paren", UO_sp_func_class_paren, AT_IARF);
   unc_add_option("sp_func_def_paren", UO_sp_func_def_paren, AT_IARF);
   unc_add_option("sp_func_proto_paren", UO_sp_func_proto_paren, AT_IARF);
   unc_add_option("sp_inside_angle", UO_sp_inside_angle, AT_IARF);
   unc_add_option("sp_inside_braces", UO_sp_inside_braces, AT_IARF);
   unc_add_option("sp_inside_braces_enum", UO_sp_inside_braces_enum, AT_IARF);
   unc_add_option("sp_inside_braces_struct", UO_sp_inside_braces_struct, AT_IARF);
   unc_add_option("sp_inside_fparen", UO_sp_inside_fparen, AT_IARF);
   unc_add_option("sp_inside_fparens", UO_sp_inside_fparens, AT_IARF);
   unc_add_option("sp_inside_paren", UO_sp_inside_paren, AT_IARF);
   unc_add_option("sp_inside_sparen", UO_sp_inside_sparen, AT_IARF);
   unc_add_option("sp_inside_square", UO_sp_inside_square, AT_IARF);
   unc_add_option("sp_macro", UO_sp_macro, AT_IARF);
   unc_add_option("sp_macro_func", UO_sp_macro_func, AT_IARF);
   unc_add_option("sp_paren_brace", UO_sp_paren_brace, AT_IARF, "Space between ')' and '{'");
   unc_add_option("sp_paren_paren", UO_sp_paren_paren, AT_IARF);
   unc_add_option("sp_return_paren", UO_sp_return_paren, AT_IARF);
   unc_add_option("sp_sizeof_paren", UO_sp_sizeof_paren, AT_IARF);
   unc_add_option("sp_sparen_brace", UO_sp_sparen_brace, AT_IARF, "Space between ')' and '{' of if, while, etc");
   unc_add_option("sp_special_semi", UO_sp_special_semi, AT_IARF);
   unc_add_option("sp_square_fparen", UO_sp_square_fparen, AT_IARF);
   unc_add_option("sp_type_func", UO_sp_type_func, AT_IARF);
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
 * Convert the value string to a number.
 */
static int convert_value(const option_map_value *entry, const char *val)
{
   const option_map_value *tmp;
   bool btrue;
   int  mult;

   if (entry->type == AT_LINE)
   {
      if (strcasecmp(val, "CRLF") == 0)
      {
         return(LE_CRLF);
      }
      if (strcasecmp(val, "LF") == 0)
      {
         return(LE_LF);
      }
      if (strcasecmp(val, "CR") == 0)
      {
         return(LE_CR);
      }
      if (strcasecmp(val, "AUTO") != 0)
      {
         LOG_FMT(LWARN, "%s:%d Expected AUTO, LF, CRLF, or CR for %s, got %s\n",
                 cpd.filename, cpd.line_number, entry->name, val);
         cpd.error_count++;
      }
      return(LE_AUTO);
   }

   if (entry->type == AT_POS)
   {
      if ((strcasecmp(val, "LEAD") == 0) ||
          (strcasecmp(val, "START") == 0))
      {
         return(TP_LEAD);
      }
      if ((strcasecmp(val, "TRAIL") == 0) ||
          (strcasecmp(val, "END") == 0))
      {
         return(TP_TRAIL);
      }
      if (strcasecmp(val, "IGNORE") != 0)
      {
         LOG_FMT(LWARN, "%s:%d Expected IGNORE, LEAD/START, or TRAIL/END for %s, got %s\n",
                 cpd.filename, cpd.line_number, entry->name, val);
         cpd.error_count++;
      }
      return(TP_IGNORE);
   }

   if (entry->type == AT_NUM)
   {
      if (isdigit(*val) ||
          (isdigit(val[1]) && ((*val == '-') || (*val == '+'))))
      {
         return(strtol(val, NULL, 0));
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
            return(cpd.settings[tmp->id].n * mult);
         }
      }
      LOG_FMT(LWARN, "%s:%d Expected a number for %s, got %s\n",
              cpd.filename, cpd.line_number, entry->name, val);
      cpd.error_count++;
      return(0);
   }

   if (entry->type == AT_BOOL)
   {
      if ((strcasecmp(val, "true") == 0) ||
          (strcasecmp(val, "t") == 0) ||
          (strcmp(val, "1") == 0))
      {
         return(1);
      }

      if ((strcasecmp(val, "false") == 0) ||
          (strcasecmp(val, "f") == 0) ||
          (strcmp(val, "0") == 0))
      {
         return(0);
      }

      btrue = true;
      if ((*val == '-') || (*val == '~'))
      {
         btrue = false;
         val++;
      }

      if (((tmp = unc_find_option(val)) != NULL) && (tmp->type == entry->type))
      {
         return(cpd.settings[tmp->id].b ? btrue : !btrue);
      }
      LOG_FMT(LWARN, "%s:%d Expected 'True' or 'False' for %s, got %s\n",
              cpd.filename, cpd.line_number, entry->name, val);
      cpd.error_count++;
      return(0);
   }

   /* Must be AT_IARF */

   if ((strcasecmp(val, "add") == 0) || (strcasecmp(val, "a") == 0))
   {
      return(AV_ADD);
   }
   if ((strcasecmp(val, "remove") == 0) || (strcasecmp(val, "r") == 0))
   {
      return(AV_REMOVE);
   }
   if ((strcasecmp(val, "force") == 0) || (strcasecmp(val, "f") == 0))
   {
      return(AV_FORCE);
   }
   if ((strcasecmp(val, "ignore") == 0) || (strcasecmp(val, "i") == 0))
   {
      return(AV_IGNORE);
   }
   if (((tmp = unc_find_option(val)) != NULL) && (tmp->type == entry->type))
   {
      return(cpd.settings[tmp->id].a);
   }
   LOG_FMT(LWARN, "%s:%d Expected 'Add', 'Remove', 'Force', or 'Ignore' for %s, got %s\n",
           cpd.filename, cpd.line_number, entry->name, val);
   cpd.error_count++;
   return(0);
}

int set_option_value(const char *name, const char *value)
{
   const option_map_value *entry;

   if ((entry = unc_find_option(name)) != NULL)
   {
      cpd.settings[entry->id].n = convert_value(entry, value);
      return(entry->id);
   }
   return(-1);
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
            add_keyword(args[idx], CT_TYPE, LANG_ALL);
         }
      }
      else if (strcasecmp(args[0], "define") == 0)
      {
         add_define(args[1], args[2]);
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
   const char *val_str;
   int        val_len;
   int        name_len;

   /* Print the all out */
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
            fprintf(pfile, "%s# %s\n", first ? "" : "\n", option->short_desc);
         }
         first   = false;
         val_str = op_val_to_string(option->type, cpd.settings[option->id]).c_str();
         val_len = strlen(val_str);
         name_len = strlen(option->name);

         fprintf(pfile, "%s %*.s= %s",
                 option->name, cpd.max_option_name_len - name_len, " ",
                 val_str);
         if (withDoc)
         {
            fprintf(pfile, "%*.s # %s",
                    8 - val_len, " ",
                    argtype_to_string(option->type).c_str());
         }
         fputs("\n", pfile);
      }
   }
   fclose(pfile);
   return(0);
}

void print_options(FILE *pfile, bool verbose)
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
 * TODO: select from various sets? - ie, K&R, GNU, Linux, Ben
 */
void set_option_defaults(void)
{
   cpd.settings[UO_newlines].le          = LE_AUTO;
   cpd.settings[UO_input_tab_size].n     = 8;
   cpd.settings[UO_output_tab_size].n    = 8;
   cpd.settings[UO_indent_columns].n     = 8;
   cpd.settings[UO_indent_with_tabs].n   = 1;
   cpd.settings[UO_indent_label].n       = 1;
   cpd.settings[UO_string_escape_char].n = '\\';
}

std::string argtype_to_string(argtype_e argtype)
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
      return("ignore/lead/trail");

   default:
      LOG_FMT(LWARN, "Unknown argtype '%d'\n", argtype);
      return("");
   }
}

std::string bool_to_string(bool val)
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

std::string argval_to_string(argval_t argval)
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

std::string number_to_string(int number)
{
   char buffer[12]; // 11 + 1

   sprintf(buffer, "%d", number);
   return(buffer);
}

std::string lineends_to_string(lineends_e linends)
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

std::string tokenpos_to_string(tokenpos_e tokenpos)
{
   switch (tokenpos)
   {
   case TP_IGNORE:
      return("ignore");

   case TP_LEAD:
      return("lead");

   case TP_TRAIL:
      return("trail");

   default:
      LOG_FMT(LWARN, "Unknown tokenpos '%d'\n", tokenpos);
      return("");
   }
}

std::string op_val_to_string(argtype_e argtype, op_val_t op_val)
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

   default:
      LOG_FMT(LWARN, "Unknown argtype '%d'\n", argtype);
      return("");
   }
}
