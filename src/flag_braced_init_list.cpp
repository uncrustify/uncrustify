/**
 * @file flag_braced_init_list.cpp
 *
 * @license GPL v2+
 */

#include "chunk_list.h"

#include "flag_braced_init_list.h"

#include "uncrustify.h"


bool detect_cpp_braced_init_list(chunk_t *pc, chunk_t *next)
{
   LOG_FUNC_ENTRY();
   // Issue #2332
   bool we_have_a_case_before = false;

   if (chunk_is_token(pc, CT_COLON))
   {
      // check if we have a case before
      chunk_t *switch_before = chunk_get_prev_type(pc, CT_CASE, pc->level);

      if (switch_before != nullptr)
      {
         LOG_FMT(LFCNR, "%s(%d): switch_before->orig_line is %zu, orig_col is %zu, text() is '%s', type is %s\n",
                 __func__, __LINE__, switch_before->orig_line, switch_before->orig_col,
                 switch_before->text(), get_token_name(switch_before->type));
         we_have_a_case_before = true;
      }
   }

   // Detect a braced-init-list
   if (  chunk_is_token(pc, CT_WORD)
      || chunk_is_token(pc, CT_TYPE)
      || chunk_is_assign_token(pc)
      || chunk_is_token(pc, CT_RETURN)
      || chunk_is_comma_token(pc)
      || chunk_is_angle_close_token(pc)
      || chunk_is_square_close_token(pc)
      || chunk_is_subscript_token(pc)
      || chunk_is_token(pc, CT_FPAREN_OPEN)
      || chunk_is_question_token(pc)
      || (  chunk_is_token(pc, CT_COLON)
         && !we_have_a_case_before)
      || (  chunk_is_brace_open_token(pc)
         && (  get_chunk_parent_type(pc) == CT_NONE
            || get_chunk_parent_type(pc) == CT_BRACED_INIT_LIST)))
   {
      log_pcf_flags(LFCNR, pc->flags);
      auto brace_open = chunk_get_next_ncnnl(pc);

      if (  chunk_is_brace_open_token(brace_open)
         && (  get_chunk_parent_type(brace_open) == CT_NONE
            || get_chunk_parent_type(brace_open) == CT_ASSIGN
            || get_chunk_parent_type(brace_open) == CT_RETURN
            || get_chunk_parent_type(brace_open) == CT_BRACED_INIT_LIST))
      {
         log_pcf_flags(LFCNR, brace_open->flags);
         auto brace_close = chunk_skip_to_match(next);

         if (chunk_is_brace_close_token(brace_close))
         {
            return(true);
         }
      }
   }
   return(false);
} // detect_cpp_braced_init_list


void flag_cpp_braced_init_list(chunk_t *pc, chunk_t *next)
{
   auto brace_open  = chunk_get_next_ncnnl(pc);
   auto brace_close = chunk_skip_to_match(next);

   set_chunk_parent(brace_open, CT_BRACED_INIT_LIST);
   set_chunk_parent(brace_close, CT_BRACED_INIT_LIST);

   auto *tmp = chunk_get_next_ncnnl(brace_close);

   if (tmp != nullptr)
   {
      chunk_flags_clr(tmp, PCF_EXPR_START | PCF_STMT_START);

      // Flag call operator
      if (chunk_is_token(tmp, CT_PAREN_OPEN))
      {
         if (auto *const c = chunk_skip_to_match(tmp))
         {
            set_chunk_type(tmp, CT_FPAREN_OPEN);
            set_chunk_parent(tmp, CT_FUNC_CALL);
            set_chunk_type(c, CT_FPAREN_CLOSE);
            set_chunk_parent(c, CT_FUNC_CALL);
         }
      }
   }
   // TODO: Change pc->type CT_WORD -> CT_TYPE
   // for the case CT_ASSIGN (and others).

   // TODO: Move this block to the fix_fcn_call_args function.
   if (  chunk_is_token(pc, CT_WORD)
      && pc->flags.test(PCF_IN_FCN_CALL))
   {
      set_chunk_type(pc, CT_TYPE);
   }
} // flag_cpp_braced_init_list
