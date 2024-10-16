/**
 * @file blank_line.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#include "blank_line.h"

#include "chunk.h"
#include "ifdef_over_whole_file.h"
#include "is_func_proto_group.h"
#include "log_rules.h"
#include "mark_change.h"
#include "newlines/can_increase_nl.h"
#include "newlines/func_pre_blank_lines.h"
#include "newlines/if_for_while_switch.h"
#include "newlines/one_liner.h"
#include "uncrustify.h"


constexpr static auto LCURRENT = LNEWLINE;


using namespace uncrustify;


void blank_line_max(Chunk *pc, Option<unsigned> &opt)
{
   LOG_FUNC_ENTRY();

   if (pc->IsNullChunk())
   {
      return;
   }
   const auto optval = opt();

   if (  (optval > 0)
      && (pc->GetNlCount() > optval))
   {
      LOG_FMT(LBLANKD, "%s(%d): blank lines: %s max line %zu\n",
              __func__, __LINE__, opt.name(), pc->GetOrigLine());
      pc->SetNlCount(optval);
      MARK_CHANGE();
   }
} // blank_line_max


void blank_line_set(Chunk *pc, Option<unsigned> &opt)
{
   LOG_FUNC_ENTRY();

   if (pc->IsNullChunk())
   {
      return;
   }
   const unsigned optval = opt();

   if (  (optval > 0)
      && (pc->GetNlCount() != optval))
   {
      LOG_FMT(LBLANKD, "%s(%d): %s set line %zu to %u\n",
              __func__, __LINE__, opt.name(), pc->GetOrigLine(), optval);
      pc->SetNlCount(optval);
      MARK_CHANGE();
   }
} // blank_line_set


void do_blank_lines()
{
   LOG_FUNC_ENTRY();

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (pc->Is(CT_NEWLINE))
      {
         LOG_FMT(LBLANKD, "%s(%d): orig line is %zu, orig col is %zu, <Newline>, nl is %zu\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetNlCount());
      }
      else
      {
         char copy[1000];
         LOG_FMT(LBLANKD, "%s(%d): orig line is %zu, orig col is %zu, Text() '%s', type is %s\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->ElidedText(copy), get_token_name(pc->GetType()));
      }
      LOG_FMT(LBLANK, "%s(%d): new line count is %zu\n",
              __func__, __LINE__, pc->GetNlCount());

      if (pc->IsNot(CT_NEWLINE))
      {
         continue;
      }
      Chunk *prev = pc->GetPrevNc();

      if (prev->IsNotNullChunk())
      {
         LOG_FMT(LBLANK, "%s(%d): prev orig line is %zu, prev->Text() '%s', prev->GetType() is %s\n",
                 __func__, __LINE__, pc->GetOrigLine(),
                 prev->Text(), get_token_name(prev->GetType()));

         if (prev->Is(CT_IGNORED))
         {
            continue;
         }
      }
      Chunk *next = pc->GetNext();
      Chunk *pcmt = pc->GetPrev();

      bool  line_added = false;

      /*
       * If this is the first or the last token, pretend that there is an extra
       * line. It will be removed at the end.
       */
      if (  pc == Chunk::GetHead()
         || next->IsNullChunk())
      {
         line_added = true;
         pc->SetNlCount(pc->GetNlCount() + 1);
         LOG_FMT(LBLANK, "%s(%d): orig line is %zu, orig col is %zu, text is '%s', new line count is now %zu\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), pc->GetNlCount());
      }

      // Limit consecutive newlines
      if (  (options::nl_max() > 0)
         && (pc->GetNlCount() > options::nl_max()))
      {
         log_rule_B("nl_max");
         blank_line_max(pc, options::nl_max);
      }

      if (!can_increase_nl(pc))
      {
         LOG_FMT(LBLANKD, "%s(%d): force to 1 orig line is %zu, orig col is %zu\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol());

         if (pc->GetNlCount() != 1)
         {
            pc->SetNlCount(1);
            MARK_CHANGE();
         }
         continue;
      }

      // Control blanks before multi-line comments
      if (  (options::nl_before_block_comment() > pc->GetNlCount())
         && next->Is(CT_COMMENT_MULTI))
      {
         log_rule_B("nl_before_block_comment");

         // Don't add blanks after an open brace or a case statement
         if (  (  prev->IsNullChunk()
               || (  prev->IsNot(CT_BRACE_OPEN)
                  && prev->IsNot(CT_VBRACE_OPEN)
                  && prev->IsNot(CT_CASE_COLON)))
            && pcmt->IsNot(CT_COMMENT_MULTI))    // Issue #2383
         {
            blank_line_set(pc, options::nl_before_block_comment);
            log_rule_B("nl_before_block_comment");
         }
      }

      // Control blanks before single line C comments
      if (  (options::nl_before_c_comment() > pc->GetNlCount())
         && next->Is(CT_COMMENT))
      {
         log_rule_B("nl_before_c_comment");

         // Don't add blanks after an open brace, a case stamement, or a comment
         if (  (  prev->IsNullChunk()
               || (  prev->IsNot(CT_BRACE_OPEN)
                  && prev->IsNot(CT_VBRACE_OPEN)
                  && prev->IsNot(CT_CASE_COLON)))
            && pcmt->IsNot(CT_COMMENT))    // Issue #2383
         {
            blank_line_set(pc, options::nl_before_c_comment);
            log_rule_B("nl_before_c_comment");
         }
      }

      // Control blanks before CPP comments
      if (  (options::nl_before_cpp_comment() > pc->GetNlCount())
         && next->Is(CT_COMMENT_CPP))
      {
         log_rule_B("nl_before_cpp_comment");

         // Don't add blanks after an open brace or a case statement
         if (  (  prev->IsNullChunk()
               || (  prev->IsNot(CT_BRACE_OPEN)
                  && prev->IsNot(CT_VBRACE_OPEN)
                  && prev->IsNot(CT_CASE_COLON)))
            && pcmt->IsNot(CT_COMMENT_CPP))    // Issue #2383
         {
            blank_line_set(pc, options::nl_before_cpp_comment);
            log_rule_B("nl_before_cpp_comment");
         }
      }

      // Control blanks before a class/struct
      if (  (  prev->Is(CT_SEMICOLON)
            || prev->Is(CT_BRACE_CLOSE))
         && (  prev->GetParentType() == CT_CLASS
            || prev->GetParentType() == CT_STRUCT))
      {
         E_Token parent_type = prev->GetParentType();
         Chunk   *start      = prev->GetPrevType(parent_type, prev->GetLevel());
         Chunk   *tmp        = start;

         // Is this a class/struct template?
         if (tmp->GetParentType() == CT_TEMPLATE)
         {
            tmp = tmp->GetPrevType(CT_TEMPLATE, prev->GetLevel());
            tmp = tmp->GetPrevNc();
         }
         else
         {
            tmp = tmp->GetPrevNc();

            while (  tmp->Is(CT_NEWLINE)
                  && tmp->GetPrev()->IsComment())
            {
               tmp = tmp->GetPrev()->GetPrevNc();
            }

            if (tmp->Is(CT_FRIEND))
            {
               // Account for a friend declaration
               tmp = tmp->GetPrevNc();
            }
         }

         while (  tmp->Is(CT_NEWLINE)
               && tmp->GetPrev()->IsComment())
         {
            tmp = tmp->GetPrev()->GetPrevNc();
         }

         if (  tmp->IsNotNullChunk()
            && !start->TestFlags(PCF_INCOMPLETE))
         {
            if (parent_type == CT_CLASS && options::nl_before_class() > tmp->GetNlCount())
            {
               log_rule_B("nl_before_class");
               blank_line_set(tmp, options::nl_before_class);
            }
            else if (parent_type == CT_STRUCT && options::nl_before_struct() > tmp->GetNlCount())
            {
               log_rule_B("nl_before_struct");
               blank_line_set(tmp, options::nl_before_struct);
            }
         }
      }

      if (  prev->Is(CT_BRACE_CLOSE)
         && prev->GetParentType() == CT_NAMESPACE)
      {
         // Control blanks before a namespace
         Chunk *tmp = prev->GetPrevType(CT_NAMESPACE, prev->GetLevel());
         tmp = tmp->GetPrevNc();

         while (  tmp->Is(CT_NEWLINE)
               && tmp->GetPrev()->IsComment())
         {
            tmp = tmp->GetPrev()->GetPrevNc();
         }

         if (  tmp->IsNotNullChunk()
            && options::nl_before_namespace() > tmp->GetNlCount())
         {
            log_rule_B("nl_before_namespace");
            blank_line_set(tmp, options::nl_before_namespace);
         }

         // Add blanks after namespace
         if (options::nl_after_namespace() > pc->GetNlCount())
         {
            log_rule_B("nl_after_namespace");
            blank_line_set(pc, options::nl_after_namespace);
         }
      }

      // Control blanks inside empty function body
      if (  prev->Is(CT_BRACE_OPEN)
         && next->Is(CT_BRACE_CLOSE)
         && (  prev->GetParentType() == CT_FUNC_DEF
            || prev->GetParentType() == CT_FUNC_CLASS_DEF)
         && options::nl_inside_empty_func() > pc->GetNlCount()
         && prev->TestFlags(PCF_EMPTY_BODY))
      {
         blank_line_set(pc, options::nl_inside_empty_func);
         log_rule_B("nl_inside_empty_func");
      }

      // Control blanks after an access spec
      if (  (options::nl_after_access_spec() > 0)
         && (options::nl_after_access_spec() != pc->GetNlCount())
         && prev->Is(CT_ACCESS_COLON))
      {
         log_rule_B("nl_after_access_spec");

         // Don't add blanks before a closing brace
         if (  next->IsNullChunk()
            || !next->IsBraceClose())
         {
            log_rule_B("nl_after_access_spec");
            blank_line_set(pc, options::nl_after_access_spec);
         }
      }

      // Add blanks after function bodies
      if (  prev->Is(CT_BRACE_CLOSE)
         && (  prev->GetParentType() == CT_FUNC_DEF
            || prev->GetParentType() == CT_FUNC_CLASS_DEF
            || prev->GetParentType() == CT_OC_MSG_DECL
            || prev->GetParentType() == CT_ASSIGN))
      {
         if (prev->TestFlags(PCF_ONE_LINER))
         {
            if (options::nl_after_func_body_one_liner() > pc->GetNlCount())
            {
               log_rule_B("nl_after_func_body_one_liner");
               blank_line_set(pc, options::nl_after_func_body_one_liner);
            }
         }
         else
         {
            if (  prev->TestFlags(PCF_IN_CLASS)
               && (options::nl_after_func_body_class() > 0))
            {
               log_rule_B("nl_after_func_body_class");

               if (options::nl_after_func_body_class() != pc->GetNlCount())
               {
                  log_rule_B("nl_after_func_body_class");
                  blank_line_set(pc, options::nl_after_func_body_class);
               }
            }
            else
            {
               if (!(pc->GetPrev()->TestFlags(PCF_IN_TRY_BLOCK))) // Issue #1734
               {
                  if (options::nl_after_func_body() > 0)
                  {
                     log_rule_B("nl_after_func_body");

                     if (options::nl_after_func_body() != pc->GetNlCount())
                     {
                        log_rule_B("nl_after_func_body");
                        blank_line_set(pc, options::nl_after_func_body);
                     }
                  }
                  else
                  {
                     if (options::nl_min_after_func_body() > 0) // Issue #2787
                     {
                        log_rule_B("nl_min_after_func_body");

                        if (options::nl_min_after_func_body() > pc->GetNlCount())
                        {
                           log_rule_B("nl_min_after_func_body");
                           blank_line_set(pc, options::nl_min_after_func_body);
                        }
                     }

                     if (options::nl_max_after_func_body() > 0)
                     {
                        log_rule_B("nl_max_after_func_body");

                        if (options::nl_max_after_func_body() < pc->GetNlCount())
                        {
                           log_rule_B("nl_max_after_func_body");
                           blank_line_set(pc, options::nl_max_after_func_body);
                        }
                     }
                  }
               }
            }
         }
      }

      // Add blanks after function prototypes
      if (  (  prev->Is(CT_SEMICOLON)
            && prev->GetParentType() == CT_FUNC_PROTO)
         || is_func_proto_group(prev, CT_FUNC_DEF))
      {
         if (options::nl_after_func_proto() > pc->GetNlCount())
         {
            log_rule_B("nl_after_func_proto");
            pc->SetNlCount(options::nl_after_func_proto());
            MARK_CHANGE();
         }

         if (  (options::nl_after_func_proto_group() > pc->GetNlCount())
            && next->IsNotNullChunk()
            && next->GetParentType() != CT_FUNC_PROTO
            && !is_func_proto_group(next, CT_FUNC_DEF))
         {
            log_rule_B("nl_after_func_proto_group");
            blank_line_set(pc, options::nl_after_func_proto_group);
         }
      }

      // Issue #411: Add blanks after function class prototypes
      if (  (  prev->Is(CT_SEMICOLON)
            && prev->GetParentType() == CT_FUNC_CLASS_PROTO)
         || is_func_proto_group(prev, CT_FUNC_CLASS_DEF))
      {
         if (options::nl_after_func_class_proto() > pc->GetNlCount())
         {
            log_rule_B("nl_after_func_class_proto");
            pc->SetNlCount(options::nl_after_func_class_proto());
            MARK_CHANGE();
         }

         if (  (options::nl_after_func_class_proto_group() > pc->GetNlCount())
            && next->IsNot(CT_FUNC_CLASS_PROTO)
            && next->GetParentType() != CT_FUNC_CLASS_PROTO
            && !is_func_proto_group(next, CT_FUNC_CLASS_DEF))
         {
            log_rule_B("nl_after_func_class_proto_group");
            blank_line_set(pc, options::nl_after_func_class_proto_group);
         }
      }

      // Add blanks after struct/enum/union/class
      if (  (  prev->Is(CT_SEMICOLON)
            || prev->Is(CT_BRACE_CLOSE))
         && (  prev->GetParentType() == CT_STRUCT
            || prev->GetParentType() == CT_ENUM
            || prev->GetParentType() == CT_UNION
            || prev->GetParentType() == CT_CLASS))
      {
         auto &opt = (prev->GetParentType() == CT_CLASS
         ? options::nl_after_class
         : options::nl_after_struct);
         log_rule_B("nl_after_class");
         log_rule_B("nl_after_struct");

         if (opt() > pc->GetNlCount())
         {
            // Issue #1702
            // look back if we have a variable
            Chunk *tmp        = pc;
            bool  is_var_def  = false;
            bool  is_fwd_decl = false;

            while ((tmp = tmp->GetPrev())->IsNotNullChunk())
            {
               if (tmp->GetLevel() > pc->GetLevel())
               {
                  continue;
               }
               LOG_FMT(LBLANK, "%s(%d): %zu:%zu token is '%s'\n",
                       __func__, __LINE__, tmp->GetOrigLine(), tmp->GetOrigCol(), tmp->Text());

               if (tmp->TestFlags(PCF_VAR_DEF))
               {
                  is_var_def = true;
                  break;
               }

               if (tmp->Is(prev->GetParentType()))
               {
                  is_fwd_decl = tmp->TestFlags(PCF_INCOMPLETE);
                  break;
               }
            }
            LOG_FMT(LBLANK, "%s(%d): var_def = %s, fwd_decl = %s\n",
                    __func__, __LINE__,
                    is_var_def ? "yes" : "no",
                    is_fwd_decl ? "yes" : "no");

            if (  !is_var_def
               && !is_fwd_decl)
            {
               blank_line_set(pc, opt);
            }
         }
      }

      // Change blanks between a function comment and body
      if (  (options::nl_comment_func_def() != 0)
         && pcmt->Is(CT_COMMENT_MULTI)
         && pcmt->GetParentType() == CT_COMMENT_WHOLE
         && next->IsNotNullChunk()
         && (  next->GetParentType() == CT_FUNC_DEF
            || next->GetParentType() == CT_FUNC_CLASS_DEF))
      {
         log_rule_B("nl_comment_func_def");

         if (options::nl_comment_func_def() != pc->GetNlCount())
         {
            log_rule_B("nl_comment_func_def");
            blank_line_set(pc, options::nl_comment_func_def);
         }
      }

      // Change blanks after a try-catch-finally block
      if (  (options::nl_after_try_catch_finally() != 0)
         && (options::nl_after_try_catch_finally() != pc->GetNlCount())
         && prev->IsNotNullChunk()
         && next->IsNotNullChunk())
      {
         log_rule_B("nl_after_try_catch_finally");

         if (  prev->Is(CT_BRACE_CLOSE)
            && (  prev->GetParentType() == CT_CATCH
               || prev->GetParentType() == CT_FINALLY))
         {
            if (  next->IsNot(CT_BRACE_CLOSE)
               && next->IsNot(CT_CATCH)
               && next->IsNot(CT_FINALLY))
            {
               blank_line_set(pc, options::nl_after_try_catch_finally);
               log_rule_B("nl_after_try_catch_finally");
            }
         }
      }

      // Change blanks after a try-catch-finally block
      if (  (options::nl_between_get_set() != 0)
         && (options::nl_between_get_set() != pc->GetNlCount())
         && prev->IsNotNullChunk()
         && next->IsNotNullChunk())
      {
         log_rule_B("nl_between_get_set");

         if (  prev->GetParentType() == CT_GETSET
            && next->IsNot(CT_BRACE_CLOSE)
            && (  prev->Is(CT_BRACE_CLOSE)
               || prev->Is(CT_SEMICOLON)))
         {
            blank_line_set(pc, options::nl_between_get_set);
            log_rule_B("nl_between_get_set");
         }
      }

      // Change blanks after a try-catch-finally block
      if (  (options::nl_around_cs_property() != 0)
         && (options::nl_around_cs_property() != pc->GetNlCount())
         && prev->IsNotNullChunk()
         && next->IsNotNullChunk())
      {
         log_rule_B("nl_around_cs_property");

         if (  prev->Is(CT_BRACE_CLOSE)
            && prev->GetParentType() == CT_CS_PROPERTY
            && next->IsNot(CT_BRACE_CLOSE))
         {
            blank_line_set(pc, options::nl_around_cs_property);
            log_rule_B("nl_around_cs_property");
         }
         else if (  next->GetParentType() == CT_CS_PROPERTY
                 && next->TestFlags(PCF_STMT_START))
         {
            blank_line_set(pc, options::nl_around_cs_property);
            log_rule_B("nl_around_cs_property");
         }
      }

      // Control blanks before an access spec
      if (  (options::nl_before_access_spec() > 0)
         && (options::nl_before_access_spec() != pc->GetNlCount())
         && next->Is(CT_ACCESS))
      {
         log_rule_B("nl_before_access_spec");

         // Don't add blanks after an open brace
         if (  prev->IsNullChunk()
            || (  prev->IsNot(CT_BRACE_OPEN)
               && prev->IsNot(CT_VBRACE_OPEN)))
         {
            log_rule_B("nl_before_access_spec");
            blank_line_set(pc, options::nl_before_access_spec);
         }
      }

      // Change blanks inside namespace braces
      if (  (options::nl_inside_namespace() != 0)
         && (options::nl_inside_namespace() != pc->GetNlCount())
         && (  (  prev->Is(CT_BRACE_OPEN)
               && prev->GetParentType() == CT_NAMESPACE)
            || (  next->Is(CT_BRACE_CLOSE)
               && next->GetParentType() == CT_NAMESPACE)))
      {
         log_rule_B("nl_inside_namespace");
         blank_line_set(pc, options::nl_inside_namespace);
      }

      // Control blanks before a whole-file #ifdef
      if (  options::nl_before_whole_file_ifdef() != 0
         && options::nl_before_whole_file_ifdef() != pc->GetNlCount()
         && next->Is(CT_PREPROC)
         && next->GetParentType() == CT_PP_IF
         && ifdef_over_whole_file()
         && next->TestFlags(PCF_WF_IF))
      {
         log_rule_B("nl_before_whole_file_ifdef");
         blank_line_set(pc, options::nl_before_whole_file_ifdef);
      }

      // Control blanks after a whole-file #ifdef
      if (  options::nl_after_whole_file_ifdef() != 0
         && options::nl_after_whole_file_ifdef() != pc->GetNlCount())
      {
         Chunk *pp_start = prev->GetPpStart();

         if (  pp_start->IsNotNullChunk()
            && pp_start->GetParentType() == CT_PP_IF
            && ifdef_over_whole_file()
            && pp_start->TestFlags(PCF_WF_IF))
         {
            log_rule_B("nl_after_whole_file_ifdef");
            blank_line_set(pc, options::nl_after_whole_file_ifdef);
         }
      }

      // Control blanks before a whole-file #endif
      if (  options::nl_before_whole_file_endif() != 0
         && options::nl_before_whole_file_endif() != pc->GetNlCount()
         && next->Is(CT_PREPROC)
         && next->GetParentType() == CT_PP_ENDIF
         && ifdef_over_whole_file()
         && next->TestFlags(PCF_WF_ENDIF))
      {
         log_rule_B("nl_before_whole_file_endif");
         blank_line_set(pc, options::nl_before_whole_file_endif);
      }

      // Control blanks after a whole-file #endif
      if (  options::nl_after_whole_file_endif() != 0
         && options::nl_after_whole_file_endif() != pc->GetNlCount())
      {
         Chunk *pp_start = prev->GetPpStart();

         if (  pp_start->IsNotNullChunk()
            && pp_start->GetParentType() == CT_PP_ENDIF
            && ifdef_over_whole_file()
            && pp_start->TestFlags(PCF_WF_ENDIF))
         {
            log_rule_B("nl_after_whole_file_endif");
            blank_line_set(pc, options::nl_after_whole_file_endif);
         }
      }

      if (  line_added
         && pc->GetNlCount() > 1)
      {
         pc->SetNlCount(pc->GetNlCount() - 1);
         LOG_FMT(LBLANK, "%s(%d): orig line is %zu, orig col is %zu, text is '%s', new line count is now %zu\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), pc->GetNlCount());
      }
      LOG_FMT(LBLANK, "%s(%d): orig line is %zu, orig col is %zu, text is '%s', end new line count is now %zu\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), pc->GetNlCount());
   }
} // do_blank_lines


void newlines_insert_blank_lines()
{
   LOG_FUNC_ENTRY();

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      //LOG_FMT(LNEWLINE, "%s(%d): orig line is %zu, orig col is %zu, Text() '%s', type is %s\n",
      //        __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), get_token_name(pc->GetType()));
      if (pc->Is(CT_IF))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_before_if());
         log_rule_B("nl_before_if");
         newlines_if_for_while_switch_post_blank_lines(pc, options::nl_after_if());
         log_rule_B("nl_after_if");
      }
      else if (pc->Is(CT_FOR))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_before_for());
         log_rule_B("nl_before_for");
         newlines_if_for_while_switch_post_blank_lines(pc, options::nl_after_for());
         log_rule_B("nl_after_for");
      }
      else if (pc->Is(CT_WHILE))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_before_while());
         log_rule_B("nl_before_while");
         newlines_if_for_while_switch_post_blank_lines(pc, options::nl_after_while());
         log_rule_B("nl_after_while");
      }
      else if (pc->Is(CT_SWITCH))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_before_switch());
         log_rule_B("nl_before_switch");
         newlines_if_for_while_switch_post_blank_lines(pc, options::nl_after_switch());
         log_rule_B("nl_after_switch");
      }
      else if (pc->Is(CT_SYNCHRONIZED))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_before_synchronized());
         log_rule_B("nl_before_synchronized");
         newlines_if_for_while_switch_post_blank_lines(pc, options::nl_after_synchronized());
         log_rule_B("nl_after_synchronized");
      }
      else if (pc->Is(CT_DO))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_before_do());
         log_rule_B("nl_before_do");
         newlines_if_for_while_switch_post_blank_lines(pc, options::nl_after_do());
         log_rule_B("nl_after_do");
      }
      else if (pc->Is(CT_OC_INTF))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_oc_before_interface());
         log_rule_B("nl_oc_before_interface");
      }
      else if (pc->Is(CT_OC_END))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_oc_before_end());
         log_rule_B("nl_oc_before_end");
      }
      else if (pc->Is(CT_OC_IMPL))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_oc_before_implementation());
         log_rule_B("nl_oc_before_implementation");
      }
      else if (  pc->Is(CT_FUNC_CLASS_DEF)
              || pc->Is(CT_FUNC_DEF)
              || pc->Is(CT_FUNC_CLASS_PROTO)
              || pc->Is(CT_FUNC_PROTO))
      {
         if (  options::nl_class_leave_one_liner_groups()
            && is_class_one_liner(pc))
         {
            log_rule_B("nl_class_leave_one_liner_groups");
            newlines_func_pre_blank_lines(pc, CT_FUNC_PROTO);
         }
         else
         {
            newlines_func_pre_blank_lines(pc, pc->GetType());
         }
      }
      else
      {
         // ignore it
         //LOG_FMT(LNEWLINE, "%s(%d): ignore it\n", __func__, __LINE__);
      }
   }
} // newlines_insert_blank_lines
