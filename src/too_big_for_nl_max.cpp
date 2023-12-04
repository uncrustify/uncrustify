/**
 * @file too_big_for_nl_max.cpp
 *
 * @author  Guy Maurel Decembe 2023
 * @license GPL v2+
 */

#include "too_big_for_nl_max.h"

#include "log_levels.h"
#include "log_rules.h"
#include "logger.h"
#include "options.h"

#include <cstdio>

constexpr static auto LCURRENT = LINDENT;

using namespace std;
using namespace uncrustify;


void too_big_for_nl_max()
{
   log_rule_B("nl_var_def_blk_end_func_top");
   if (options::nl_var_def_blk_end_func_top() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_var_def_blk_end_func_top' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_var_def_blk_end");
   if (options::nl_var_def_blk_end() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_var_def_blk_end' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_var_def_blk_start");
   if (options::nl_var_def_blk_start() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_var_def_blk_start' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_var_def_blk_in");
   if (options::nl_var_def_blk_in() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_var_def_blk_in' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_after_access_spec");
   if (options::nl_after_access_spec() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_after_access_spec' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_start_of_file_min");
   if (options::nl_start_of_file_min() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_start_of_file_min' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_end_of_file_min");
   if (options::nl_end_of_file_min() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_end_of_file_min' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_inside_empty_func");
   if (options::nl_inside_empty_func() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_inside_empty_func' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_before_func_body_proto");
   if (options::nl_before_func_body_proto() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_before_func_body_proto' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_before_func_body_def");
   if (options::nl_before_func_body_def() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_before_func_body_def' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_before_func_class_proto");
   if (options::nl_before_func_class_proto() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_before_func_class_proto' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_before_func_class_def");
   if (options::nl_before_func_class_def() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_before_func_class_def' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_after_func_proto");
   if (options::nl_after_func_proto() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_after_func_proto' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_after_func_proto_group");
   if (options::nl_after_func_proto_group() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_after_func_proto_group' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_after_func_class_proto");
   if (options::nl_after_func_class_proto() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_after_func_class_proto' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_after_func_class_proto_group");
   if (options::nl_after_func_class_proto_group() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_after_func_class_proto_group' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_after_func_body");
   if (options::nl_after_func_body() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_after_func_body' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_min_after_func_body");
   if (options::nl_min_after_func_body() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_min_after_func_body' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_after_func_body_class");
   if (options::nl_after_func_body_class() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_after_func_body_class' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_after_func_body_one_liner");
   if (options::nl_after_func_body_one_liner() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_after_func_body_one_liner' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_typedef_blk_start");
   if (options::nl_typedef_blk_start() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_typedef_blk_start' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_typedef_blk_end");
   if (options::nl_typedef_blk_end() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_typedef_blk_end' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_before_block_comment");
   if (options::nl_before_block_comment() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_before_block_comment' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_before_c_comment");
   if (options::nl_before_c_comment() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_before_c_comment' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_before_cpp_comment");
   if (options::nl_before_cpp_comment() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_before_cpp_comment' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_before_struct");
   if (options::nl_before_struct() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_before_struct' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_after_struct");
   if (options::nl_after_struct() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_after_struct' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_before_class");
   if (options::nl_before_class() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_before_class' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_after_class");
   if (options::nl_after_class() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_after_class' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_before_namespace");
   if (options::nl_before_namespace() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_before_namespace' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_inside_namespace");
   if (options::nl_inside_namespace() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_inside_namespace' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_after_namespace");
   if (options::nl_after_namespace() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_after_namespace' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_before_access_spec");
   if (options::nl_before_access_spec() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_before_access_spec' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_after_access_spec");
   if (options::nl_after_access_spec() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_after_access_spec' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_comment_func_def");
   if (options::nl_comment_func_def() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_comment_func_def' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_after_try_catch_finally");
   if (options::nl_after_try_catch_finally() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_after_try_catch_finally' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_around_cs_property");
   if (options::nl_around_cs_property() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_around_cs_property' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_between_get_set");
   if (options::nl_between_get_set() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_between_get_set' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_before_whole_file_ifdef");
   if (options::nl_before_whole_file_ifdef() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_before_whole_file_ifdef' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_after_whole_file_ifdef");
   if (options::nl_after_whole_file_ifdef() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_after_whole_file_ifdef' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_before_whole_file_endif");
   if (options::nl_before_whole_file_endif() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_before_whole_file_endif' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }
   
   log_rule_B("nl_after_whole_file_endif");
   if (options::nl_after_whole_file_endif() >= options::nl_max())
   {
      fprintf(stderr, "The option 'nl_after_whole_file_endif' is too big against the option 'nl_max'");
      log_flush(true);
      exit(EX_CONFIG);
   }

} // too_big_for_nl_max
