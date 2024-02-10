/**
 * @file func.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/func.h"

#include "log_rules.h"
#include "newlines/iarf.h"
#include "options.h"
#include "tokenizer/combine_skip.h"
#include "uncrustify.h"


using namespace uncrustify;


constexpr static auto LCURRENT = LNEWLINE;


/**
 * Formats a function declaration
 * Start points to the open paren
 */
void newline_func_def_or_call(Chunk *start)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNFD, "%s(%d): called on start->Text() is '%s', orig line is %zu, orig col is %zu, [%s/%s]\n",
           __func__, __LINE__, start->Text(), start->GetOrigLine(), start->GetOrigCol(),
           get_token_name(start->GetType()), get_token_name(start->GetParentType()));

   bool is_def = (start->GetParentType() == CT_FUNC_DEF)
                 || start->GetParentType() == CT_FUNC_CLASS_DEF;
   bool is_call = (start->GetParentType() == CT_FUNC_CALL)
                  || start->GetParentType() == CT_FUNC_CALL_USER;

   LOG_FMT(LNFD, "%s(%d): is_def is %s, is_call is %s\n",
           __func__, __LINE__, is_def ? "TRUE" : "FALSE", is_call ? "TRUE" : "FALSE");

   if (is_call)
   {
      log_rule_B("nl_func_call_paren");
      iarf_e atmp = options::nl_func_call_paren();

      if (atmp != IARF_IGNORE)
      {
         Chunk *prev = start->GetPrevNcNnlNi();   // Issue #2279

         if (prev->IsNotNullChunk())
         {
            newline_iarf(prev, atmp);
         }
      }
      Chunk *pc = start->GetNextNcNnl();

      if (pc->IsString(")"))
      {
         log_rule_B("nl_func_call_paren_empty");
         atmp = options::nl_func_call_paren_empty();

         if (atmp != IARF_IGNORE)
         {
            Chunk *prev = start->GetPrevNcNnlNi();   // Issue #2279

            if (prev->IsNotNullChunk())
            {
               newline_iarf(prev, atmp);
            }
         }
         log_rule_B("nl_func_call_empty");
         atmp = options::nl_func_call_empty();

         if (atmp != IARF_IGNORE)
         {
            newline_iarf(start, atmp);
         }
         return;
      }
   }
   else
   {
      log_rule_B("nl_func_def_paren");
      log_rule_B("nl_func_paren");
      iarf_e atmp = is_def ? options::nl_func_def_paren()
                  : options::nl_func_paren();
      LOG_FMT(LSPACE, "%s(%d): atmp is %s\n",
              __func__, __LINE__,
              (atmp == IARF_IGNORE) ? "IGNORE" :
              (atmp == IARF_ADD) ? "ADD" :
              (atmp == IARF_REMOVE) ? "REMOVE" : "FORCE");

      if (atmp != IARF_IGNORE)
      {
         Chunk *prev = start->GetPrevNcNnlNi();      // Issue #2279

         if (prev->IsNotNullChunk())
         {
            newline_iarf(prev, atmp);
         }
      }
      // Handle break newlines type and function
      Chunk *prev = start->GetPrevNcNnlNi();   // Issue #2279
      prev = skip_template_prev(prev);
      // Don't split up a function variable
      prev = prev->IsParenClose() ? Chunk::NullChunkPtr : prev->GetPrevNcNnlNi();   // Issue #2279

      log_rule_B("nl_func_class_scope");

      if (  prev->Is(CT_DC_MEMBER)
         && (options::nl_func_class_scope() != IARF_IGNORE))
      {
         newline_iarf(prev->GetPrevNcNnlNi(), options::nl_func_class_scope());   // Issue #2279
      }

      if (prev->IsNot(CT_ACCESS_COLON))
      {
         Chunk *tmp;

         if (prev->Is(CT_OPERATOR))
         {
            tmp  = prev;
            prev = prev->GetPrevNcNnlNi();   // Issue #2279
         }
         else
         {
            tmp = start;
         }

         if (prev->Is(CT_DC_MEMBER))
         {
            log_rule_B("nl_func_scope_name");

            if (  options::nl_func_scope_name() != IARF_IGNORE
               && !start->TestFlags(PCF_IN_DECLTYPE))
            {
               newline_iarf(prev, options::nl_func_scope_name());
            }
         }
         const Chunk *tmp_next = prev->GetNextNcNnl();

         if (tmp_next->IsNot(CT_FUNC_CLASS_DEF))
         {
            Chunk  *closing = tmp->GetClosingParen();
            Chunk  *brace   = closing->GetNextNcNnl();
            iarf_e a;                                            // Issue #2561

            if (  tmp->GetParentType() == CT_FUNC_PROTO
               || tmp->GetParentType() == CT_FUNC_CLASS_PROTO)
            {
               // proto
               log_rule_B("nl_func_proto_type_name");
               a = options::nl_func_proto_type_name();
            }
            else
            {
               // def

               log_rule_B("nl_func_leave_one_liners");

               if (  options::nl_func_leave_one_liners()
                  && (  brace->IsNullChunk()
                     || brace->TestFlags(PCF_ONE_LINER)))   // Issue #1511 and #3274
               {
                  a = IARF_IGNORE;
               }
               else
               {
                  log_rule_B("nl_func_type_name");
                  a = options::nl_func_type_name();
               }
            }
            log_rule_B("nl_func_type_name_class");

            if (  tmp->TestFlags(PCF_IN_CLASS)
               && (options::nl_func_type_name_class() != IARF_IGNORE))
            {
               a = options::nl_func_type_name_class();
            }

            if (  (a != IARF_IGNORE)
               && prev->IsNotNullChunk())
            {
               LOG_FMT(LNFD, "%s(%d): prev->Text() '%s', orig line is %zu, orig col is %zu, [%s/%s]\n",
                       __func__, __LINE__, prev->Text(), prev->GetOrigLine(), prev->GetOrigCol(),
                       get_token_name(prev->GetType()),
                       get_token_name(prev->GetParentType()));

               if (prev->Is(CT_DESTRUCTOR))
               {
                  prev = prev->GetPrevNcNnlNi();   // Issue #2279
               }

               /*
                * If we are on a '::', step back two tokens
                * TODO: do we also need to check for '.' ?
                */
               while (prev->Is(CT_DC_MEMBER))
               {
                  prev = prev->GetPrevNcNnlNi();   // Issue #2279
                  prev = skip_template_prev(prev);
                  prev = prev->GetPrevNcNnlNi();   // Issue #2279
               }

               if (  !prev->IsBraceClose()
                  && prev->IsNot(CT_BRACE_OPEN)
                  && prev->IsNot(CT_SEMICOLON)
                  && prev->IsNot(CT_ACCESS_COLON)
                     // #1008: if we landed on an operator check that it is having
                     // a type before it, in order to not apply nl_func_type_name
                     // on conversion operators as they don't have a normal
                     // return type syntax
                  && (tmp_next->IsNot(CT_OPERATOR) ? true : prev->IsTypeDefinition()))
               {
                  newline_iarf(prev, a);
               }
            }
         }
      }
      Chunk *pc = start->GetNextNcNnl();

      if (pc->IsString(")"))
      {
         log_rule_B("nl_func_def_empty");
         log_rule_B("nl_func_decl_empty");
         atmp = is_def ? options::nl_func_def_empty()
                : options::nl_func_decl_empty();

         if (atmp != IARF_IGNORE)
         {
            newline_iarf(start, atmp);
         }
         log_rule_B("nl_func_def_paren_empty");
         log_rule_B("nl_func_paren_empty");
         atmp = is_def ? options::nl_func_def_paren_empty()
                : options::nl_func_paren_empty();

         if (atmp != IARF_IGNORE)
         {
            prev = start->GetPrevNcNnlNi();   // Issue #2279

            if (prev->IsNotNullChunk())
            {
               newline_iarf(prev, atmp);
            }
         }
         return;
      }
   }
   // Now scan for commas
   size_t comma_count = 0;
   Chunk  *tmp;
   Chunk  *pc;

   for (pc = start->GetNextNcNnl();
        pc->IsNotNullChunk() && pc->GetLevel() > start->GetLevel();
        pc = pc->GetNextNcNnl())
   {
      if (  pc->Is(CT_COMMA)
         && (pc->GetLevel() == (start->GetLevel() + 1)))
      {
         comma_count++;
         tmp = pc->GetNext();

         if (tmp->IsComment())
         {
            pc = tmp;
         }

         if (is_def)
         {
            log_rule_B("nl_func_def_args");
            newline_iarf(pc, options::nl_func_def_args());
         }
         else if (is_call)
         {
            // Issue #2604
            log_rule_B("nl_func_call_args");
            newline_iarf(pc, options::nl_func_call_args());
         }
         else // start->GetParentType() == CT_FUNC_DECL
         {
            log_rule_B("nl_func_decl_args");
            newline_iarf(pc, options::nl_func_decl_args());
         }
      }
   }

   log_rule_B("nl_func_def_start");
   log_rule_B("nl_func_decl_start");
   iarf_e as = is_def ? options::nl_func_def_start() : options::nl_func_decl_start();

   log_rule_B("nl_func_def_end");
   log_rule_B("nl_func_decl_end");
   iarf_e ae = is_def ? options::nl_func_def_end() : options::nl_func_decl_end();

   if (comma_count == 0)
   {
      iarf_e atmp;
      log_rule_B("nl_func_def_start_single");
      log_rule_B("nl_func_decl_start_single");
      atmp = is_def ? options::nl_func_def_start_single() :
             options::nl_func_decl_start_single();

      if (atmp != IARF_IGNORE)
      {
         as = atmp;
      }
      log_rule_B("nl_func_def_end_single");
      log_rule_B("nl_func_decl_end_single");
      atmp = is_def ? options::nl_func_def_end_single() :
             options::nl_func_decl_end_single();

      if (atmp != IARF_IGNORE)
      {
         ae = atmp;
      }
   }

   if (!is_call)
   {
      newline_iarf(start, as);
   }

   // and fix up the close parenthesis
   if (pc->Is(CT_FPAREN_CLOSE))
   {
      Chunk *prev = pc->GetPrevNnl();

      if (  prev->IsNot(CT_FPAREN_OPEN)
         && !is_call)
      {
         newline_iarf(prev, ae);
      }
      newline_func_multi_line(start);
   }
} // newline_func_def_or_call


/**
 * Adds newlines to multi-line function call/decl/def
 * Start points to the open paren
 */
void newline_func_multi_line(Chunk *start)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNFD, "%s(%d): called on %zu:%zu '%s' [%s/%s]\n",
           __func__, __LINE__, start->GetOrigLine(), start->GetOrigCol(),
           start->Text(), get_token_name(start->GetType()), get_token_name(start->GetParentType()));

   bool add_start;
   bool add_args;
   bool add_end;

   if (  start->GetParentType() == CT_FUNC_DEF
      || start->GetParentType() == CT_FUNC_CLASS_DEF)
   {
      log_rule_B("nl_func_def_start_multi_line");
      add_start = options::nl_func_def_start_multi_line();
      log_rule_B("nl_func_def_args_multi_line");
      add_args = options::nl_func_def_args_multi_line();
      log_rule_B("nl_func_def_end_multi_line");
      add_end = options::nl_func_def_end_multi_line();
   }
   else if (  start->GetParentType() == CT_FUNC_CALL
           || start->GetParentType() == CT_FUNC_CALL_USER)
   {
      log_rule_B("nl_func_call_start_multi_line");
      add_start = options::nl_func_call_start_multi_line();
      log_rule_B("nl_func_call_args_multi_line");
      add_args = options::nl_func_call_args_multi_line();
      log_rule_B("nl_func_call_end_multi_line");
      add_end = options::nl_func_call_end_multi_line();
   }
   else
   {
      log_rule_B("nl_func_decl_start_multi_line");
      add_start = options::nl_func_decl_start_multi_line();
      log_rule_B("nl_func_decl_args_multi_line");
      add_args = options::nl_func_decl_args_multi_line();
      log_rule_B("nl_func_decl_end_multi_line");
      add_end = options::nl_func_decl_end_multi_line();
   }

   if (  !add_start
      && !add_args
      && !add_end)
   {
      return;
   }
   Chunk *pc = start->GetNextNcNnl();

   while (  pc->IsNotNullChunk()
         && pc->GetLevel() > start->GetLevel())
   {
      pc = pc->GetNextNcNnl();
   }

   if (  pc->Is(CT_FPAREN_CLOSE)
      && start->IsNewlineBetween(pc))
   {
      Chunk *start_next         = start->GetNextNcNnl();
      bool  has_leading_closure = (  start_next->GetParentType() == CT_OC_BLOCK_EXPR
                                  || start_next->GetParentType() == CT_CPP_LAMBDA
                                  || start_next->Is(CT_BRACE_OPEN));

      Chunk *prev_end            = pc->GetPrevNcNnl();
      bool  has_trailing_closure = (  prev_end->GetParentType() == CT_OC_BLOCK_EXPR
                                   || prev_end->GetParentType() == CT_CPP_LAMBDA
                                   || prev_end->Is(CT_BRACE_OPEN));

      if (  add_start
         && !start->GetNext()->IsNewline())
      {
         log_rule_B("nl_func_call_args_multi_line_ignore_closures");

         if (options::nl_func_call_args_multi_line_ignore_closures())
         {
            if (  !has_leading_closure
               && !has_trailing_closure)
            {
               newline_iarf(start, IARF_ADD);
            }
         }
         else
         {
            newline_iarf(start, IARF_ADD);
         }
      }

      if (  add_end
         && !pc->GetPrev()->IsNewline())
      {
         log_rule_B("nl_func_call_args_multi_line_ignore_closures");

         if (options::nl_func_call_args_multi_line_ignore_closures())
         {
            if (  !has_leading_closure
               && !has_trailing_closure)
            {
               newline_iarf(pc->GetPrev(), IARF_ADD);
            }
         }
         else
         {
            newline_iarf(pc->GetPrev(), IARF_ADD);
         }
      }

      if (add_args)
      {
         // process the function in reverse and leave the first comma if the option to leave trailing closure
         // is on. nl_func_call_args_multi_line_ignore_trailing_closure
         for (pc = start->GetNextNcNnl();
              pc->IsNotNullChunk() && pc->GetLevel() > start->GetLevel();
              pc = pc->GetNextNcNnl())
         {
            if (  pc->Is(CT_COMMA)
               && (pc->GetLevel() == (start->GetLevel() + 1)))
            {
               Chunk *tmp = pc->GetNext();

               if (tmp->IsComment())
               {
                  pc = tmp;
               }

               if (!pc->GetNext()->IsNewline())
               {
                  log_rule_B("nl_func_call_args_multi_line_ignore_closures");

                  if (options::nl_func_call_args_multi_line_ignore_closures())
                  {
                     Chunk *prev_comma  = pc->GetPrevNcNnl();
                     Chunk *after_comma = pc->GetNextNcNnl();

                     if (!(  (  prev_comma->GetParentType() == CT_OC_BLOCK_EXPR
                             || prev_comma->GetParentType() == CT_CPP_LAMBDA
                             || prev_comma->Is(CT_BRACE_OPEN))
                          || (  after_comma->GetParentType() == CT_OC_BLOCK_EXPR
                             || after_comma->GetParentType() == CT_CPP_LAMBDA
                             || after_comma->Is(CT_BRACE_OPEN))))
                     {
                        newline_iarf(pc, IARF_ADD);
                     }
                  }
                  else
                  {
                     newline_iarf(pc, IARF_ADD);
                  }
               }
            }
         }
      }
   }
} // newline_func_multi_line
