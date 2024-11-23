/**
 * @file cleanup.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#include "newlines/cleanup.h"

#include "blank_line.h"
#include "chunk.h"
#include "log_rules.h"
#include "mark_change.h"
#include "newlines/add.h"
#include "newlines/after.h"
#include "newlines/before_return.h"
#include "newlines/brace_pair.h"
#include "newlines/case.h"
#include "newlines/cuddle_uncuddle.h"
#include "newlines/do_else.h"
#include "newlines/double_newline.h"
#include "newlines/double_space_struct_enum_union.h"
#include "newlines/end_newline.h"
#include "newlines/enum.h"
#include "newlines/func.h"
#include "newlines/iarf.h"
#include "newlines/if_for_while_switch.h"
#include "newlines/namespace.h"
#include "newlines/oc_msg.h"
#include "newlines/one_liner.h"
#include "newlines/struct_union.h"
#include "newlines/template.h"
#include "newlines/var_def_blk.h"
#include "tokenizer/flag_parens.h"

#ifdef WIN32
#include <algorithm>                   // to get max
#endif // ifdef WIN32

constexpr static auto LCURRENT = LNEWLINE;


using namespace uncrustify;


void newlines_cleanup_angles()
{
   // Issue #1167
   LOG_FUNC_ENTRY();

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      char copy[1000];
      LOG_FMT(LBLANK, "%s(%d): orig line is %zu, orig col is %zu, Text() is '%s'\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->ElidedText(copy));

      if (pc->Is(CT_ANGLE_OPEN))
      {
         newline_template(pc);
      }
   }
} // newlines_cleanup_angles


void newlines_cleanup_braces(bool first)
{
   LOG_FUNC_ENTRY();

   // Get the first token that's not an empty line:
   Chunk *pc = Chunk::GetHead();

   if (pc->IsNewline())
   {
      pc = pc->GetNextNcNnl();
   }

   for ( ; pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      char copy[1000];
      LOG_FMT(LBLANK, "%s(%d): orig line is %zu, orig col is %zu, Text() is '%s'\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->ElidedText(copy));

      if (  pc->Is(CT_IF)
         || pc->Is(CT_CONSTEXPR))
      {
         log_rule_B("nl_if_brace");
         newlines_if_for_while_switch(pc, options::nl_if_brace());
      }
      else if (pc->Is(CT_ELSEIF))
      {
         log_rule_B("nl_elseif_brace");
         iarf_e arg = options::nl_elseif_brace();
         log_rule_B("nl_if_brace");
         newlines_if_for_while_switch(
            pc, (arg != IARF_IGNORE) ? arg : options::nl_if_brace());
      }
      else if (pc->Is(CT_FOR))
      {
         log_rule_B("nl_for_brace");
         newlines_if_for_while_switch(pc, options::nl_for_brace());
      }
      else if (pc->Is(CT_CATCH))
      {
         log_rule_B("nl_oc_brace_catch");

         if (  language_is_set(lang_flag_e::LANG_OC)
            && (pc->GetStr()[0] == '@')
            && (options::nl_oc_brace_catch() != IARF_IGNORE))
         {
            newlines_cuddle_uncuddle(pc, options::nl_oc_brace_catch());
         }
         else
         {
            log_rule_B("nl_brace_catch");
            newlines_cuddle_uncuddle(pc, options::nl_brace_catch());
         }
         Chunk *next = pc->GetNextNcNnl();

         if (next->Is(CT_BRACE_OPEN))
         {
            log_rule_B("nl_oc_catch_brace");

            if (  language_is_set(lang_flag_e::LANG_OC)
               && (options::nl_oc_catch_brace() != IARF_IGNORE))
            {
               log_rule_B("nl_oc_catch_brace");
               newlines_do_else(pc, options::nl_oc_catch_brace());
            }
            else
            {
               log_rule_B("nl_catch_brace");
               newlines_do_else(pc, options::nl_catch_brace());
            }
         }
         else
         {
            log_rule_B("nl_oc_catch_brace");

            if (  language_is_set(lang_flag_e::LANG_OC)
               && (options::nl_oc_catch_brace() != IARF_IGNORE))
            {
               newlines_if_for_while_switch(pc, options::nl_oc_catch_brace());
            }
            else
            {
               log_rule_B("nl_catch_brace");
               newlines_if_for_while_switch(pc, options::nl_catch_brace());
            }
         }
      }
      else if (pc->Is(CT_WHILE))
      {
         log_rule_B("nl_while_brace");
         newlines_if_for_while_switch(pc, options::nl_while_brace());
      }
      else if (pc->Is(CT_USING_STMT))
      {
         log_rule_B("nl_using_brace");
         newlines_if_for_while_switch(pc, options::nl_using_brace());
      }
      else if (pc->Is(CT_D_SCOPE_IF))
      {
         log_rule_B("nl_scope_brace");
         newlines_if_for_while_switch(pc, options::nl_scope_brace());
      }
      else if (pc->Is(CT_UNITTEST))
      {
         log_rule_B("nl_unittest_brace");
         newlines_do_else(pc, options::nl_unittest_brace());
      }
      else if (pc->Is(CT_D_VERSION_IF))
      {
         log_rule_B("nl_version_brace");
         newlines_if_for_while_switch(pc, options::nl_version_brace());
      }
      else if (pc->Is(CT_SWITCH))
      {
         log_rule_B("nl_switch_brace");
         newlines_if_for_while_switch(pc, options::nl_switch_brace());
      }
      else if (pc->Is(CT_SYNCHRONIZED))
      {
         log_rule_B("nl_synchronized_brace");
         newlines_if_for_while_switch(pc, options::nl_synchronized_brace());
      }
      else if (pc->Is(CT_DO))
      {
         log_rule_B("nl_do_brace");
         newlines_do_else(pc, options::nl_do_brace());
      }
      else if (pc->Is(CT_ELSE))
      {
         log_rule_B("nl_brace_else");
         newlines_cuddle_uncuddle(pc, options::nl_brace_else());
         Chunk *next = pc->GetNextNcNnl();

         if (next->Is(CT_ELSEIF))
         {
            log_rule_B("nl_else_if");
            newline_iarf_pair(pc, next, options::nl_else_if());
         }
         log_rule_B("nl_else_brace");
         newlines_do_else(pc, options::nl_else_brace());
      }
      else if (pc->Is(CT_TRY))
      {
         log_rule_B("nl_try_brace");
         newlines_do_else(pc, options::nl_try_brace());
         // Issue #1734
         Chunk *po = pc->GetNextNcNnl();
         flag_parens(po, PCF_IN_TRY_BLOCK, po->GetType(), CT_NONE, false);
      }
      else if (pc->Is(CT_GETSET))
      {
         log_rule_B("nl_getset_brace");
         newlines_do_else(pc, options::nl_getset_brace());
      }
      else if (pc->Is(CT_FINALLY))
      {
         log_rule_B("nl_brace_finally");
         newlines_cuddle_uncuddle(pc, options::nl_brace_finally());
         log_rule_B("nl_finally_brace");
         newlines_do_else(pc, options::nl_finally_brace());
      }
      else if (pc->Is(CT_WHILE_OF_DO))
      {
         log_rule_B("nl_brace_while");
         newlines_cuddle_uncuddle(pc, options::nl_brace_while());
      }
      else if (pc->Is(CT_BRACE_OPEN))
      {
         switch (pc->GetParentType())
         {
         case CT_DOUBLE_BRACE:
         {
            log_rule_B("nl_paren_dbrace_open");

            if (options::nl_paren_dbrace_open() != IARF_IGNORE)
            {
               Chunk *prev = pc->GetPrevNcNnlNi(E_Scope::PREPROC);    // Issue #2279

               if (prev->IsParenClose())
               {
                  log_rule_B("nl_paren_dbrace_open");
                  newline_iarf_pair(prev, pc, options::nl_paren_dbrace_open());
               }
            }
            break;
         }

         case CT_ENUM:
         {
            log_rule_B("nl_enum_own_lines");

            if (options::nl_enum_own_lines() != IARF_IGNORE)
            {
               newlines_enum_entries(pc, options::nl_enum_own_lines());
            }
            log_rule_B("nl_ds_struct_enum_cmt");

            if (options::nl_ds_struct_enum_cmt())
            {
               newlines_double_space_struct_enum_union(pc);
            }
            break;
         }

         case CT_STRUCT:
         case CT_UNION:
         {
            log_rule_B("nl_ds_struct_enum_cmt");

            if (options::nl_ds_struct_enum_cmt())
            {
               newlines_double_space_struct_enum_union(pc);
            }
            break;
         }

         case CT_CLASS:
         {
            if (pc->GetLevel() == pc->GetBraceLevel())
            {
               log_rule_B("nl_class_brace");
               log_ruleNL("nl_class_brace", pc);
               newlines_do_else(pc->GetPrevNnl(), options::nl_class_brace());
            }
            break;
         }

         case CT_OC_CLASS:
         {
            if (pc->GetLevel() == pc->GetBraceLevel())
            {
               // Request #126
               // introduce two new options
               // look back if we have a @interface or a @implementation
               for (Chunk *tmp = pc->GetPrev(); tmp->IsNotNullChunk(); tmp = tmp->GetPrev())
               {
                  LOG_FMT(LBLANK, "%s(%d): orig line is %zu, orig col is %zu, Text() is '%s'\n",
                          __func__, __LINE__, tmp->GetOrigLine(), tmp->GetOrigCol(), tmp->Text());

                  if (  tmp->Is(CT_OC_INTF)
                     || tmp->Is(CT_OC_IMPL))
                  {
                     LOG_FMT(LBLANK, "%s(%d): orig line is %zu, orig col is %zu, may be remove/force newline before {\n",
                             __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol());

                     if (tmp->Is(CT_OC_INTF))
                     {
                        log_rule_B("nl_oc_interface_brace");
                        newlines_do_else(pc->GetPrevNnl(), options::nl_oc_interface_brace());
                     }
                     else
                     {
                        log_rule_B("nl_oc_implementation_brace");
                        newlines_do_else(pc->GetPrevNnl(), options::nl_oc_implementation_brace());
                     }
                     break;
                  }
               }
            }
            break;
         }

         case CT_BRACED_INIT_LIST:
         {
            // Issue #1052
            log_rule_B("nl_create_list_one_liner");

            if (options::nl_create_list_one_liner())
            {
               nl_create_list_liner(pc);
               break;
            }
            Chunk *prev = pc->GetPrevNnl();

            if (  prev->IsNotNullChunk()
               && (  prev->GetType() == CT_TYPE
                  || prev->GetType() == CT_WORD
                  || prev->GetType() == CT_ASSIGN                      // Issue #2957
                  || prev->GetParentType() == CT_TEMPLATE
                  || prev->GetParentType() == CT_DECLTYPE))
            {
               log_rule_B("nl_type_brace_init_lst");
               newline_iarf_pair(prev, pc, options::nl_type_brace_init_lst(), true);
            }
            break;
         }

         case CT_OC_BLOCK_EXPR:
         {
            // issue # 477
            log_rule_B("nl_oc_block_brace");
            newline_iarf_pair(pc->GetPrev(), pc, options::nl_oc_block_brace());
            break;
         }

         case CT_FUNC_CLASS_DEF:                             // Issue #2343
         {
            if (!one_liner_nl_ok(pc))
            {
               LOG_FMT(LNL1LINE, "a new line may NOT be added\n");
               // no change - preserve one liner body
            }
            else
            {
               log_rule_B("nl_before_opening_brace_func_class_def");

               if (options::nl_before_opening_brace_func_class_def() != IARF_IGNORE)
               {
                  newline_iarf_pair(pc->GetPrev(), pc, options::nl_before_opening_brace_func_class_def());
               }
            }
            break;
         }

         default:
         {
            break;
         }
         } // switch

         log_rule_B("nl_brace_brace");

         if (options::nl_brace_brace() != IARF_IGNORE)
         {
            Chunk *next = pc->GetNextNc(E_Scope::PREPROC);

            if (next->Is(CT_BRACE_OPEN))
            {
               newline_iarf_pair(pc, next, options::nl_brace_brace());
            }
         }
         Chunk *next = pc->GetNextNnl();

         if (next->IsNullChunk())
         {
            // do nothing
         }
         else if (next->Is(CT_BRACE_CLOSE))
         {
            // TODO: add an option to split open empty statements? { };
         }
         else if (next->Is(CT_BRACE_OPEN))
         {
            // already handled
         }
         else
         {
            next = pc->GetNextNcNnl();

            // Handle unnamed temporary direct-list-initialization
            if (pc->GetParentType() == CT_BRACED_INIT_LIST)
            {
               log_rule_B("nl_type_brace_init_lst_open");
               newline_iarf_pair(pc, pc->GetNextNnl(),
                                 options::nl_type_brace_init_lst_open(), true);
            }
            // Handle nl_after_brace_open
            else if (  (  pc->GetParentType() == CT_CPP_LAMBDA
                       || pc->GetLevel() == pc->GetBraceLevel())
                    && options::nl_after_brace_open())
            {
               log_rule_B("nl_after_brace_open");

               if (!one_liner_nl_ok(pc))
               {
                  LOG_FMT(LNL1LINE, "a new line may NOT be added (nl_after_brace_open)\n");
                  // no change - preserve one liner body
               }
               else if (  pc->TestFlags(PCF_IN_PREPROC)
                       || (  pc->TestFlags(PCF_ONE_LINER)
                          && pc->TestFlags(PCF_IN_ARRAY_ASSIGN)
                          && options::nl_assign_leave_one_liners()))
               {
                  // no change - don't break up preprocessors
               }
               else
               {
                  // Step back from next to the first non-newline item
                  Chunk *tmp = next->GetPrev();

                  while (tmp != pc)
                  {
                     if (tmp->IsComment())
                     {
                        log_rule_B("nl_after_brace_open_cmt");

                        if (  !options::nl_after_brace_open_cmt()
                           && tmp->IsNot(CT_COMMENT_MULTI))
                        {
                           break;
                        }
                     }
                     tmp = tmp->GetPrev();
                  }
                  // Add the newline
                  newline_iarf(tmp, IARF_ADD);
               }
            }
         }
         // braced-init-list is more like a function call with arguments,
         // than curly braces that determine a structure of a source code,
         // so, don't add a newline before a closing brace. Issue #1405.
         log_rule_B("nl_type_brace_init_lst_open");
         log_rule_B("nl_type_brace_init_lst_close");

         if (!(  pc->GetParentType() == CT_BRACED_INIT_LIST
              && options::nl_type_brace_init_lst_open() == IARF_IGNORE
              && options::nl_type_brace_init_lst_close() == IARF_IGNORE))
         {
            log_ruleNL("nl_type_brace_init_lst_close", pc);
            newlines_brace_pair(pc);
         }

         // Handle nl_before_brace_open
         if (  pc->Is(CT_BRACE_OPEN)
            && pc->GetLevel() == pc->GetBraceLevel()
            && options::nl_before_brace_open())
         {
            log_rule_B("nl_before_brace_open");

            if (!one_liner_nl_ok(pc))
            {
               LOG_FMT(LNL1LINE, "a new line may NOT be added (nl_before_brace_open)\n");
               // no change - preserve one liner body
            }
            else if (  pc->TestFlags(PCF_IN_PREPROC)
                    || pc->TestFlags(PCF_IN_ARRAY_ASSIGN))
            {
               // no change - don't break up array assignments or preprocessors
            }
            else
            {
               // Step back to previous non-newline item
               Chunk *tmp = pc->GetPrev();

               if (!tmp->Is(CT_NEWLINE))
               {
                  newline_iarf(tmp, IARF_ADD);
               }
            }
         }
      }
      else if (pc->Is(CT_BRACE_CLOSE))
      {
         // newline between a close brace and x
         log_rule_B("nl_brace_brace");

         if (options::nl_brace_brace() != IARF_IGNORE)
         {
            Chunk *next = pc->GetNextNc(E_Scope::PREPROC);

            if (next->Is(CT_BRACE_CLOSE))
            {
               log_rule_B("nl_brace_brace");
               newline_iarf_pair(pc, next, options::nl_brace_brace());
            }
         }
         log_rule_B("nl_brace_square");

         if (options::nl_brace_square() != IARF_IGNORE)
         {
            Chunk *next = pc->GetNextNc(E_Scope::PREPROC);

            if (next->Is(CT_SQUARE_CLOSE))
            {
               log_rule_B("nl_brace_square");
               newline_iarf_pair(pc, next, options::nl_brace_square());
            }
         }
         log_rule_B("nl_brace_fparen");

         if (options::nl_brace_fparen() != IARF_IGNORE)
         {
            Chunk *next = pc->GetNextNc(E_Scope::PREPROC);

            log_rule_B("nl_brace_fparen");

            if (  next->Is(CT_NEWLINE)
               && (options::nl_brace_fparen() == IARF_REMOVE))
            {
               next = next->GetNextNc(E_Scope::PREPROC);  // Issue #1000
            }

            if (next->Is(CT_FPAREN_CLOSE))
            {
               log_rule_B("nl_brace_fparen");
               newline_iarf_pair(pc, next, options::nl_brace_fparen());
            }
         }
         // newline before a close brace
         log_rule_B("nl_type_brace_init_lst_close");

         if (  pc->GetParentType() == CT_BRACED_INIT_LIST
            && options::nl_type_brace_init_lst_close() != IARF_IGNORE)
         {
            // Handle unnamed temporary direct-list-initialization
            log_ruleNL("nl_after_brace_close", pc);
            newline_iarf_pair(pc->GetPrevNnl(), pc,
                              options::nl_type_brace_init_lst_close(), true);
         }
         // blanks before a close brace
         log_rule_B("eat_blanks_before_close_brace");

         if (options::eat_blanks_before_close_brace())
         {
            // Limit the newlines before the close brace to 1
            Chunk *prev = pc->GetPrev();

            if (prev->IsNewline())
            {
               log_rule_B("nl_inside_namespace");
               log_rule_B("nl_inside_empty_func");

               if (  options::nl_inside_empty_func() > 0
                  && pc->GetPrevNnl()->Is(CT_BRACE_OPEN)
                  && (  pc->GetParentType() == CT_FUNC_CLASS_DEF
                     || pc->GetParentType() == CT_FUNC_DEF))
               {
                  blank_line_set(prev, options::nl_inside_empty_func);
               }
               else if (  options::nl_inside_namespace() > 0
                       && pc->GetParentType() == CT_NAMESPACE)
               {
                  blank_line_set(prev, options::nl_inside_namespace);
               }
               else if (prev->GetNlCount() != 1)
               {
                  prev->SetNlCount(1);
                  LOG_FMT(LBLANKD, "%s(%d): eat_blanks_before_close_brace %zu\n",
                          __func__, __LINE__, prev->GetOrigLine());
                  MARK_CHANGE();
               }
            }
         }
         else if (  options::nl_ds_struct_enum_close_brace()
                 && (  pc->GetParentType() == CT_ENUM
                    || pc->GetParentType() == CT_STRUCT
                    || pc->GetParentType() == CT_UNION))
         {
            log_rule_B("nl_ds_struct_enum_close_brace");

            if (!pc->TestFlags(PCF_ONE_LINER))
            {
               // Make sure the brace is preceded by two newlines
               Chunk *prev = pc->GetPrev();

               if (!prev->IsNewline())
               {
                  prev = newline_add_before(pc);
               }

               if (prev->GetNlCount() < 2)
               {
                  double_newline(prev);
               }
            }
         }
         // Force a newline after a close brace
         log_rule_B("nl_brace_struct_var");

         if (  (options::nl_brace_struct_var() != IARF_IGNORE)
            && (  pc->GetParentType() == CT_STRUCT
               || pc->GetParentType() == CT_ENUM
               || pc->GetParentType() == CT_UNION))
         {
            Chunk *next = pc->GetNextNcNnl(E_Scope::PREPROC);

            if (  next->IsNot(CT_SEMICOLON)
               && next->IsNot(CT_COMMA))
            {
               log_rule_B("nl_brace_struct_var");
               newline_iarf(pc, options::nl_brace_struct_var());
            }
         }
         else if (  pc->GetParentType() != CT_OC_AT
                 && pc->GetParentType() != CT_BRACED_INIT_LIST
                 && (  options::nl_after_brace_close()
                    || pc->GetParentType() == CT_FUNC_CLASS_DEF
                    || pc->GetParentType() == CT_FUNC_DEF
                    || pc->GetParentType() == CT_OC_MSG_DECL))
         {
            log_rule_B("nl_after_brace_close");
            Chunk *next = pc->GetNext();

            if (  next->IsNot(CT_SEMICOLON)
               && next->IsNot(CT_COMMA)
               && next->IsNot(CT_SPAREN_CLOSE)    // Issue #664
               && next->IsNot(CT_SQUARE_CLOSE)
               && next->IsNot(CT_FPAREN_CLOSE)
               && next->IsNot(CT_PAREN_CLOSE)
               && next->IsNot(CT_WHILE_OF_DO)
               && next->IsNot(CT_VBRACE_CLOSE)        // Issue #666
               && (  next->IsNot(CT_BRACE_CLOSE)
                  || !next->TestFlags(PCF_ONE_LINER)) // #1258
               && !pc->TestFlags(PCF_IN_ARRAY_ASSIGN)
               && !pc->TestFlags(PCF_IN_TYPEDEF)
               && !next->IsCommentOrNewline()
               && next->IsNotNullChunk())
            {
               // #1258
               // dont add newline between two consecutive braces closes, if the second is a part of one liner.
               newline_end_newline(pc);
            }
         }
         else if (pc->GetParentType() == CT_NAMESPACE)
         {
            log_rule_B("nl_after_namespace");

            if (options::nl_after_namespace() > 0)
            {
               Chunk *next = pc->GetNextNcNnl(E_Scope::PREPROC);

               if (next->IsNotNullChunk())
               {
                  newline_add_before(next);
                  // newline_iarf(next, IARF_ADD);
               }
            }
         }
      }
      else if (pc->Is(CT_VBRACE_OPEN))
      {
         log_rule_B("nl_after_vbrace_open");
         log_rule_B("nl_after_vbrace_open_empty");

         if (  options::nl_after_vbrace_open()
            || options::nl_after_vbrace_open_empty())
         {
            Chunk *next = pc->GetNext(E_Scope::PREPROC);
            bool  add_it;

            if (next->IsSemicolon())
            {
               log_rule_B("nl_after_vbrace_open_empty");
               add_it = options::nl_after_vbrace_open_empty();
            }
            else
            {
               log_rule_B("nl_after_vbrace_open");
               add_it = (  options::nl_after_vbrace_open()
                        && next->IsNot(CT_VBRACE_CLOSE)
                        && !next->IsCommentOrNewline());
            }

            if (add_it)
            {
               newline_iarf(pc, IARF_ADD);
            }
         }
         log_rule_B("nl_create_if_one_liner");
         log_rule_B("nl_create_for_one_liner");
         log_rule_B("nl_create_while_one_liner");

         if (  (  (  pc->GetParentType() == CT_IF
                  || pc->GetParentType() == CT_ELSEIF
                  || pc->GetParentType() == CT_ELSE)
               && options::nl_create_if_one_liner())
            || (  pc->GetParentType() == CT_FOR
               && options::nl_create_for_one_liner())
            || (  pc->GetParentType() == CT_WHILE
               && options::nl_create_while_one_liner()))
         {
            nl_create_one_liner(pc);
         }
         log_rule_B("nl_split_if_one_liner");
         log_rule_B("nl_split_for_one_liner");
         log_rule_B("nl_split_while_one_liner");

         if (  (  (  pc->GetParentType() == CT_IF
                  || pc->GetParentType() == CT_ELSEIF
                  || pc->GetParentType() == CT_ELSE)
               && options::nl_split_if_one_liner())
            || (  pc->GetParentType() == CT_FOR
               && options::nl_split_for_one_liner())
            || (  pc->GetParentType() == CT_WHILE
               && options::nl_split_while_one_liner()))
         {
            if (pc->TestFlags(PCF_ONE_LINER))
            {
               // split one-liner
               Chunk *end = pc->GetNext()->GetNextType(CT_SEMICOLON)->GetNext();
               // Scan for clear flag
               LOG_FMT(LNEWLINE, "(%d) ", __LINE__);
               LOG_FMT(LNEWLINE, "\n");

               for (Chunk *temp = pc; temp != end; temp = temp->GetNext())
               {
                  LOG_FMT(LNEWLINE, "%s(%d): Text() is '%s', type is %s, level is %zu\n",
                          __func__, __LINE__, temp->Text(), get_token_name(temp->GetType()), temp->GetLevel());
                  // produces much more log output. Use it only debugging purpose
                  //log_pcf_flags(LNEWLINE, temp->GetFlags());
                  temp->ResetFlagBits(PCF_ONE_LINER);
               }

               // split
               newline_add_between(pc, pc->GetNext());
            }
         }
      }
      else if (pc->Is(CT_VBRACE_CLOSE))
      {
         log_rule_B("nl_after_vbrace_close");

         if (options::nl_after_vbrace_close())
         {
            if (!pc->GetNextNc()->IsNewline())
            {
               newline_iarf(pc, IARF_ADD);
            }
         }
      }
      else if (  pc->Is(CT_SQUARE_OPEN)
              && pc->GetParentType() == CT_OC_MSG)
      {
         log_rule_B("nl_oc_msg_args");

         if (options::nl_oc_msg_args())
         {
            newline_oc_msg(pc);
         }
      }
      else if (pc->Is(CT_STRUCT))
      {
         log_rule_B("nl_struct_brace");
         newlines_struct_union(pc, options::nl_struct_brace(), true);
      }
      else if (pc->Is(CT_UNION))
      {
         log_rule_B("nl_union_brace");
         newlines_struct_union(pc, options::nl_union_brace(), true);
      }
      else if (pc->Is(CT_ENUM))
      {
         newlines_enum(pc);
      }
      else if (pc->Is(CT_CASE))
      {
         // Note: 'default' also maps to CT_CASE
         log_rule_B("nl_before_case");

         if (options::nl_before_case())
         {
            newline_case(pc);
         }
      }
      else if (pc->Is(CT_THROW))
      {
         Chunk *prev = pc->GetPrev();

         if (  prev->Is(CT_PAREN_CLOSE)
            || prev->Is(CT_FPAREN_CLOSE))         // Issue #1122
         {
            log_rule_B("nl_before_throw");
            newline_iarf(pc->GetPrevNcNnlNi(), options::nl_before_throw());   // Issue #2279
         }
      }
      else if (  pc->Is(CT_QUALIFIER)
              && !strcmp(pc->Text(), "throws"))
      {
         Chunk *prev = pc->GetPrev();

         if (  prev->Is(CT_PAREN_CLOSE)
            || prev->Is(CT_FPAREN_CLOSE))         // Issue #1122
         {
            log_rule_B("nl_before_throw");
            newline_iarf(pc->GetPrevNcNnlNi(), options::nl_before_throw());   // Issue #2279
         }
      }
      else if (pc->Is(CT_CASE_COLON))
      {
         Chunk *next = pc->GetNextNnl();

         log_rule_B("nl_case_colon_brace");

         if (  next->Is(CT_BRACE_OPEN)
            && options::nl_case_colon_brace() != IARF_IGNORE)
         {
            newline_iarf(pc, options::nl_case_colon_brace());
         }
         else if (options::nl_after_case())
         {
            log_rule_B("nl_after_case");
            newline_case_colon(pc);
         }
      }
      else if (pc->Is(CT_SPAREN_CLOSE))
      {
         Chunk *next = pc->GetNextNcNnl();

         if (next->Is(CT_BRACE_OPEN))
         {
            /*
             * TODO: this could be used to control newlines between the
             * the if/while/for/switch close parenthesis and the open brace, but
             * that is currently handled elsewhere.
             */
         }
      }
      else if (pc->Is(CT_RETURN))
      {
         log_rule_B("nl_before_return");

         if (options::nl_before_return())
         {
            newline_before_return(pc);
         }
         log_rule_B("nl_after_return");

         if (options::nl_after_return())
         {
            newline_after_return(pc);
         }
      }
      else if (pc->Is(CT_SEMICOLON))
      {
         log_rule_B("nl_after_semicolon");
         //log_rule_NL("nl_after_semicolon");                      // this is still a beta test

         if (  !pc->TestFlags(PCF_IN_SPAREN)
            && !pc->TestFlags(PCF_IN_PREPROC)
            && options::nl_after_semicolon())
         {
            Chunk *next = pc->GetNext();

            while (next->Is(CT_VBRACE_CLOSE))
            {
               next = next->GetNext();
            }

            if (  next->IsNotNullChunk()
               && !next->IsCommentOrNewline())
            {
               if (one_liner_nl_ok(next))
               {
                  LOG_FMT(LNL1LINE, "%s(%d): a new line may be added\n", __func__, __LINE__);
                  newline_iarf(pc, IARF_ADD);
               }
               else
               {
                  LOG_FMT(LNL1LINE, "%s(%d): a new line may NOT be added\n", __func__, __LINE__);
               }
            }
         }
         else if (pc->GetParentType() == CT_CLASS)
         {
            log_rule_B("nl_after_class");

            if (options::nl_after_class() > 0)
            {
               /*
                * If there is already a "class" comment, then don't add a newline if
                * one exists after the comment. or else this will interfere with the
                * mod_add_long_class_closebrace_comment option.
                */
               iarf_e mode  = IARF_ADD;
               Chunk  *next = pc->GetNext();

               if (next->IsComment())
               {
                  pc   = next;
                  next = pc->GetNext();

                  if (next->IsNewline())
                  {
                     mode = IARF_IGNORE;
                  }
               }
               newline_iarf(pc, mode);
            }
         }
      }
      else if (pc->Is(CT_FPAREN_OPEN))
      {
         log_rule_B("nl_func_decl_start");
         log_rule_B("nl_func_def_start");
         log_rule_B("nl_func_decl_start_single");
         log_rule_B("nl_func_def_start_single");
         log_rule_B("nl_func_decl_start_multi_line");
         log_rule_B("nl_func_def_start_multi_line");
         log_rule_B("nl_func_decl_args");
         log_rule_B("nl_func_def_args");
         log_rule_B("nl_func_decl_args_multi_line");
         log_rule_B("nl_func_def_args_multi_line");
         log_rule_B("nl_func_decl_end");
         log_rule_B("nl_func_def_end");
         log_rule_B("nl_func_decl_end_single");
         log_rule_B("nl_func_def_end_single");
         log_rule_B("nl_func_decl_end_multi_line");
         log_rule_B("nl_func_def_end_multi_line");
         log_rule_B("nl_func_decl_empty");
         log_rule_B("nl_func_def_empty");
         log_rule_B("nl_func_type_name");
         log_rule_B("nl_func_type_name_class");
         log_rule_B("nl_func_class_scope");
         log_rule_B("nl_func_scope_name");
         log_rule_B("nl_func_proto_type_name");
         log_rule_B("nl_func_paren");
         log_rule_B("nl_func_def_paren");
         log_rule_B("nl_func_def_paren_empty");
         log_rule_B("nl_func_paren_empty");

         if (  (  pc->GetParentType() == CT_FUNC_DEF
               || pc->GetParentType() == CT_FUNC_PROTO
               || pc->GetParentType() == CT_FUNC_CLASS_DEF
               || pc->GetParentType() == CT_FUNC_CLASS_PROTO
               || pc->GetParentType() == CT_OPERATOR)
            && (  options::nl_func_decl_start() != IARF_IGNORE
               || options::nl_func_def_start() != IARF_IGNORE
               || options::nl_func_decl_start_single() != IARF_IGNORE
               || options::nl_func_def_start_single() != IARF_IGNORE
               || options::nl_func_decl_start_multi_line()
               || options::nl_func_def_start_multi_line()
               || options::nl_func_decl_args() != IARF_IGNORE
               || options::nl_func_def_args() != IARF_IGNORE
               || options::nl_func_decl_args_multi_line()
               || options::nl_func_def_args_multi_line()
               || options::nl_func_decl_end() != IARF_IGNORE
               || options::nl_func_def_end() != IARF_IGNORE
               || options::nl_func_decl_end_single() != IARF_IGNORE
               || options::nl_func_def_end_single() != IARF_IGNORE
               || options::nl_func_decl_end_multi_line()
               || options::nl_func_def_end_multi_line()
               || options::nl_func_decl_empty() != IARF_IGNORE
               || options::nl_func_def_empty() != IARF_IGNORE
               || options::nl_func_type_name() != IARF_IGNORE
               || options::nl_func_type_name_class() != IARF_IGNORE
               || options::nl_func_class_scope() != IARF_IGNORE
               || options::nl_func_scope_name() != IARF_IGNORE
               || options::nl_func_proto_type_name() != IARF_IGNORE
               || options::nl_func_paren() != IARF_IGNORE
               || options::nl_func_def_paren() != IARF_IGNORE
               || options::nl_func_def_paren_empty() != IARF_IGNORE
               || options::nl_func_paren_empty() != IARF_IGNORE))
         {
            newline_func_def_or_call(pc);
         }
         else if (  (  pc->GetParentType() == CT_FUNC_CALL
                    || pc->GetParentType() == CT_FUNC_CALL_USER)
                 && (  (options::nl_func_call_start_multi_line())
                    || (options::nl_func_call_args_multi_line())
                    || (options::nl_func_call_end_multi_line())
                    || (options::nl_func_call_start() != IARF_IGNORE)        // Issue #2020
                    || (options::nl_func_call_args() != IARF_IGNORE)         // Issue #2604
                    || (options::nl_func_call_paren() != IARF_IGNORE)
                    || (options::nl_func_call_paren_empty() != IARF_IGNORE)
                    || (options::nl_func_call_empty() != IARF_IGNORE)))
         {
            log_rule_B("nl_func_call_start_multi_line");
            log_rule_B("nl_func_call_args_multi_line");
            log_rule_B("nl_func_call_end_multi_line");
            log_rule_B("nl_func_call_start");
            log_rule_B("nl_func_call_args");
            log_rule_B("nl_func_call_paren");
            log_rule_B("nl_func_call_paren_empty");
            log_rule_B("nl_func_call_empty");

            if (options::nl_func_call_start() != IARF_IGNORE)
            {
               newline_iarf(pc, options::nl_func_call_start());
            }
            // note that newline_func_def_or_call() calls newline_func_multi_line()
            newline_func_def_or_call(pc);
         }
         else if (  first
                 && (options::nl_remove_extra_newlines() == 1))
         {
            log_rule_B("nl_remove_extra_newlines");
            newline_iarf(pc, IARF_REMOVE);
         }
      }
      else if (pc->Is(CT_FPAREN_CLOSE))                          // Issue #2758
      {
         if (  (  pc->GetParentType() == CT_FUNC_CALL
               || pc->GetParentType() == CT_FUNC_CALL_USER)
            && options::nl_func_call_end() != IARF_IGNORE)
         {
            log_rule_B("nl_func_call_end");
            newline_iarf(pc->GetPrev(), options::nl_func_call_end());
         }
      }
      else if (pc->Is(CT_ANGLE_CLOSE))
      {
         if (pc->GetParentType() == CT_TEMPLATE)
         {
            Chunk *next = pc->GetNextNcNnl();

            if (  next->IsNotNullChunk()
               && next->GetLevel() == next->GetBraceLevel())
            {
               Chunk *tmp = pc->GetPrevType(CT_ANGLE_OPEN, pc->GetLevel())->GetPrevNcNnlNi();   // Issue #2279

               if (tmp->Is(CT_TEMPLATE))
               {
                  if (next->Is(CT_USING))
                  {
                     newline_iarf(pc, options::nl_template_using());
                     log_rule_B("nl_template_using");
                  }
                  else if (next->GetParentType() == CT_FUNC_DEF) // function definition
                  {
                     iarf_e const action =
                        newline_template_option(
                           pc,
                           options::nl_template_func_def_special(),
                           options::nl_template_func_def(),
                           options::nl_template_func());
                     log_rule_B("nl_template_func_def_special");
                     log_rule_B("nl_template_func_def");
                     log_rule_B("nl_template_func");
                     newline_iarf(pc, action);
                  }
                  else if (next->GetParentType() == CT_FUNC_PROTO) // function declaration
                  {
                     iarf_e const action =
                        newline_template_option(
                           pc,
                           options::nl_template_func_decl_special(),
                           options::nl_template_func_decl(),
                           options::nl_template_func());
                     log_rule_B("nl_template_func_decl_special");
                     log_rule_B("nl_template_func_decl");
                     log_rule_B("nl_template_func");
                     newline_iarf(pc, action);
                  }
                  else if (  next->Is(CT_TYPE)
                          || next->Is(CT_QUALIFIER)) // variable
                  {
                     newline_iarf(pc, options::nl_template_var());
                     log_rule_B("nl_template_var");
                  }
                  else if (next->TestFlags(PCF_INCOMPLETE)) // class declaration
                  {
                     iarf_e const action =
                        newline_template_option(
                           pc,
                           options::nl_template_class_decl_special(),
                           options::nl_template_class_decl(),
                           options::nl_template_class());
                     log_rule_B("nl_template_class_decl_special");
                     log_rule_B("nl_template_class_decl");
                     log_rule_B("nl_template_class");
                     newline_iarf(pc, action);
                  }
                  else // class definition
                  {
                     iarf_e const action =
                        newline_template_option(
                           pc,
                           options::nl_template_class_def_special(),
                           options::nl_template_class_def(),
                           options::nl_template_class());
                     log_rule_B("nl_template_class_def_special");
                     log_rule_B("nl_template_class_def");
                     log_rule_B("nl_template_class");
                     newline_iarf(pc, action);
                  }
               }
            }
         }
      }
      else if (  pc->Is(CT_NAMESPACE)
              && pc->GetParentType() != CT_USING)
      {
         // Issue #2387
         Chunk *next = pc->GetNextNcNnl();

         if (next->IsNotNullChunk())
         {
            next = next->GetNextNcNnl();

            if (!next->Is(CT_ASSIGN))
            {
               // Issue #1235
               // Issue #2186
               Chunk *braceOpen = pc->GetNextType(CT_BRACE_OPEN, pc->GetLevel());

               if (braceOpen->IsNullChunk())
               {
                  // fatal error
                  LOG_FMT(LERR, "%s(%d): Missing BRACE_OPEN after namespace\n   orig line is %zu, orig col is %zu\n",
                          __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol());
                  exit(EXIT_FAILURE);
               }
               LOG_FMT(LNEWLINE, "%s(%d): braceOpen orig line is %zu, orig col is %zu, Text() is '%s'\n",
                       __func__, __LINE__, braceOpen->GetOrigLine(), braceOpen->GetOrigCol(), braceOpen->Text());
               // produces much more log output. Use it only debugging purpose
               //log_pcf_flags(LNEWLINE, braceOpen->GetFlags());
               newlines_namespace(pc);
            }
         }
      }
      else if (pc->Is(CT_SQUARE_OPEN))
      {
         if (  pc->GetParentType() == CT_ASSIGN
            && !pc->TestFlags(PCF_ONE_LINER))
         {
            Chunk *tmp = pc->GetPrevNcNnlNi();   // Issue #2279
            newline_iarf(tmp, options::nl_assign_square());
            log_rule_B("nl_assign_square");

            iarf_e arg = options::nl_after_square_assign();
            log_rule_B("nl_after_square_assign");

            if (options::nl_assign_square() & IARF_ADD)
            {
               log_rule_B("nl_assign_square");
               arg = IARF_ADD;
            }
            newline_iarf(pc, arg);

            /*
             * if there is a newline after the open, then force a newline
             * before the close
             */
            tmp = pc->GetNextNc();

            if (tmp->IsNewline())
            {
               tmp = pc->GetNextType(CT_SQUARE_CLOSE, pc->GetLevel());

               if (tmp->IsNotNullChunk())
               {
                  newline_add_before(tmp);
               }
            }
         }
      }
      else if (pc->Is(CT_ACCESS))
      {
         // Make sure there is a newline before an access spec
         if (options::nl_before_access_spec() > 0)
         {
            log_rule_B("nl_before_access_spec");
            Chunk *prev = pc->GetPrev();

            if (!prev->IsNewline())
            {
               newline_add_before(pc);
            }
         }
      }
      else if (pc->Is(CT_ACCESS_COLON))
      {
         // Make sure there is a newline after an access spec
         if (options::nl_after_access_spec() > 0)
         {
            log_rule_B("nl_after_access_spec");
            Chunk *next = pc->GetNext();

            if (!next->IsNewline())
            {
               newline_add_before(next);
            }
         }
      }
      else if (pc->Is(CT_PP_DEFINE))
      {
         if (options::nl_multi_line_define())
         {
            log_rule_B("nl_multi_line_define");
            nl_handle_define(pc);
         }
      }
      else if (  first
              && (options::nl_remove_extra_newlines() == 1)
              && !pc->TestFlags(PCF_IN_PREPROC))
      {
         log_rule_B("nl_remove_extra_newlines");
         //log_rule_NL("nl_remove_extra_newlines");                          // this is still a beta test
         newline_iarf(pc, IARF_REMOVE);
      }
      else if (  pc->Is(CT_MEMBER)
              && (  language_is_set(lang_flag_e::LANG_JAVA)
                 || language_is_set(lang_flag_e::LANG_CPP)))                 // Issue #2574
      {
         // Issue #1124
         if (pc->GetParentType() != CT_FUNC_DEF)
         {
            newline_iarf(pc->GetPrevNnl(), options::nl_before_member());
            log_rule_B("nl_before_member");
            newline_iarf(pc, options::nl_after_member());
            log_rule_B("nl_after_member");
         }
      }
      else
      {
         // ignore it
      }
   }

   newline_var_def_blk(Chunk::GetHead());
} // newlines_cleanup_braces


void newlines_cleanup_dup()
{
   LOG_FUNC_ENTRY();

   Chunk *pc   = Chunk::GetHead();
   Chunk *next = pc;

   while (pc->IsNotNullChunk())
   {
      next = next->GetNext();

      if (  pc->Is(CT_NEWLINE)
         && next->Is(CT_NEWLINE))
      {
         next->SetNlCount(std::max(pc->GetNlCount(), next->GetNlCount()));
         Chunk::Delete(pc);
         MARK_CHANGE();
      }
      pc = next;
   }
} // newlines_cleanup_dup
