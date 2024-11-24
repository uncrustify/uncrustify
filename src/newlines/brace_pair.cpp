/**
 * @file brace_pair.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#include "newlines/brace_pair.h"

#include "blank_line.h"
#include "log_rules.h"
#include "mark_change.h"
#include "newlines/add.h"
#include "newlines/collapse_empty_body.h"
#include "newlines/del_between.h"
#include "newlines/iarf.h"
#include "newlines/is_func_call_or_def.h"
#include "newlines/one_liner.h"


constexpr static auto LCURRENT = LNEWLINE;


using namespace uncrustify;


/**
 * Handles the brace_on_func_line setting and decides if the closing brace
 * of a pair should be right after a newline.
 * The only cases where the closing brace shouldn't be the first thing on a line
 * is where the opening brace has junk after it AND where a one-liner in a
 * class is supposed to be preserved.
 *
 * General rule for break before close brace:
 * If the brace is part of a function (call or definition) OR if the only
 * thing after the opening brace is comments, the there must be a newline
 * before the close brace.
 *
 * Example of no newline before close
 * struct mystring { int  len;
 *                   char str[]; };
 * while (*(++ptr) != 0) { }
 *
 * Examples of newline before close
 * void foo() {
 * }
 */
void newlines_brace_pair(Chunk *br_open)
{
   LOG_FUNC_ENTRY();

   log_rule_B("nl_define_macro");

   if (  br_open->TestFlags(PCF_IN_PREPROC)
      && !options::nl_define_macro())
   {
      return;
   }

   //fixes 1235 Add single line namespace support
   if (  br_open->Is(CT_BRACE_OPEN)
      && (br_open->GetParentType() == CT_NAMESPACE)
      && br_open->GetPrev()->IsNewline())
   {
      Chunk *chunk_brace_close = br_open->GetClosingParen();

      if (chunk_brace_close->IsNotNullChunk())
      {
         if (br_open->IsOnSameLine(chunk_brace_close))
         {
            log_rule_B("nl_namespace_two_to_one_liner - 1");

            if (options::nl_namespace_two_to_one_liner())
            {
               Chunk *prev = br_open->GetPrevNnl();
               newline_del_between(prev, br_open);
            }
            /* Below code is to support conversion of 2 liner to 4 liners
             * else
             * {
             *  Chunk *nxt = br_open->GetNext();
             *  newline_add_between(br_open, nxt);
             * }*/
         }
      }
   }
   // fix 1247 oneliner function support - converts 4,3,2  liners to oneliner
   log_rule_B("nl_create_func_def_one_liner");

   if (  br_open->GetParentType() == CT_FUNC_DEF
      && options::nl_create_func_def_one_liner()
      && !br_open->TestFlags(PCF_NOT_POSSIBLE))          // Issue #2795
   {
      Chunk *br_close = br_open->GetClosingParen();
      Chunk *tmp      = br_open->GetPrevNcNnlNi(); // Issue #2279

      if (  br_close->IsNotNullChunk()             // Issue #2594
         && ((br_close->GetOrigLine() - br_open->GetOrigLine()) <= 2)
         && tmp->IsParenClose())                   // need to check the conditions.
      {
         // Issue #1825
         bool is_it_possible = true;

         while (  tmp->IsNotNullChunk()
               && (tmp = tmp->GetNext())->IsNotNullChunk()
               && !tmp->IsBraceClose()
               && (tmp->GetNext()->IsNotNullChunk()))
         {
            LOG_FMT(LNL1LINE, "%s(%d): tmp orig line is %zu, orig col is %zu, Text() is '%s'\n",
                    __func__, __LINE__, tmp->GetOrigLine(), tmp->GetOrigCol(), tmp->Text());

            if (tmp->IsComment())
            {
               is_it_possible = false;
               break;
            }
         }

         if (is_it_possible)
         {
            // Issue 2795
            // we have to check if it could be too long for code_width
            // make a vector to save the chunk
            std::vector<Chunk> saved_chunk;
            log_rule_B("code_width");

            if (options::code_width() > 0)
            {
               saved_chunk.reserve(16);
               Chunk *current       = br_open->GetPrevNcNnlNi();
               Chunk *next_br_close = br_close->GetNext();
               current = current->GetNext();

               while (current->IsNotNullChunk())
               {
                  LOG_FMT(LNL1LINE, "%s(%d): zu  kopieren: current orig line is %zu, orig col is %zu, Text() is '%s'\n",
                          __func__, __LINE__, current->GetOrigLine(), current->GetOrigCol(), current->Text());
                  saved_chunk.push_back(*current);
                  Chunk *the_next = current->GetNext();

                  if (  the_next->IsNullChunk()
                     || the_next == next_br_close)
                  {
                     break;
                  }
                  current = the_next;
               }
            }
            Chunk *tmp_1 = br_open->GetPrevNcNnlNi();

            while (  tmp_1->IsNotNullChunk()
                  && (tmp_1 = tmp_1->GetNext())->IsNotNullChunk()
                  && !tmp_1->IsBraceClose()
                  && (tmp_1->GetNext()->IsNotNullChunk()))
            {
               LOG_FMT(LNL1LINE, "%s(%d): tmp_1 orig line is %zu, orig col is %zu, Text() is '%s'\n",
                       __func__, __LINE__, tmp_1->GetOrigLine(), tmp_1->GetOrigCol(), tmp_1->Text());

               if (tmp_1->IsNewline())
               {
                  tmp_1 = tmp_1->GetPrev();                 // Issue #1825
                  newline_iarf_pair(tmp_1, tmp_1->GetNextNcNnl(), IARF_REMOVE);
               }
            }
            br_open->SetFlagBits(PCF_ONE_LINER);         // set the one liner flag if needed
            br_close->SetFlagBits(PCF_ONE_LINER);
            log_rule_B("code_width");

            if (  options::code_width() > 0
               && br_close->GetColumn() > options::code_width())
            {
               // the created line is too long
               // it is not possible to make an one_liner
               // because the line would be too long
               br_open->SetFlagBits(PCF_NOT_POSSIBLE);
               // restore the code
               size_t count;
               Chunk  tmp_2;
               Chunk  *current = br_open;

               for (count = 0; count < saved_chunk.size(); count++)
               {
                  tmp_2 = saved_chunk.at(count);

                  if (tmp_2.GetOrigLine() != current->GetOrigLine())
                  {
                     // restore the newline
                     Chunk chunk;
                     chunk.SetType(CT_NEWLINE);
                     chunk.SetOrigLine(current->GetOrigLine());
                     chunk.SetOrigCol(current->GetOrigCol());
                     chunk.SetPpLevel(current->GetPpLevel());
                     chunk.SetNlCount(1);
                     chunk.CopyAndAddBefore(current);
                     LOG_FMT(LNEWLINE, "%s(%d): %zu:%zu add newline before '%s'\n",
                             __func__, __LINE__, current->GetOrigLine(), current->GetOrigCol(), current->Text());
                  }
                  else
                  {
                     current = current->GetNext();
                  }
               }
            }
         }
      }
   }

   // Make sure we don't break a one-liner
   if (!one_liner_nl_ok(br_open))
   {
      LOG_FMT(LNL1LINE, "%s(%d): br_open orig line is %zu, orig col is %zu, a new line may NOT be added\n",
              __func__, __LINE__, br_open->GetOrigLine(), br_open->GetOrigCol());
      return;
   }
   LOG_FMT(LNL1LINE, "%s(%d): a new line may be added\n", __func__, __LINE__);

   Chunk *next = br_open->GetNextNc();

   // Insert a newline between the '=' and open brace, if needed
   LOG_FMT(LNL1LINE, "%s(%d): br_open->Text() '%s', br_open->GetType() [%s], br_open->GetParentType() [%s]\n",
           __func__, __LINE__, br_open->Text(), get_token_name(br_open->GetType()),
           get_token_name(br_open->GetParentType()));

   if (br_open->GetParentType() == CT_ASSIGN)
   {
      // Only mess with it if the open brace is followed by a newline
      if (next->IsNewline())
      {
         Chunk *prev = br_open->GetPrevNcNnlNi();   // Issue #2279
         log_rule_B("nl_assign_brace");
         newline_iarf_pair(prev, br_open, options::nl_assign_brace());
      }
   }

   if (  br_open->GetParentType() == CT_OC_MSG_DECL
      || br_open->GetParentType() == CT_FUNC_DEF
      || br_open->GetParentType() == CT_FUNC_CLASS_DEF
      || br_open->GetParentType() == CT_OC_CLASS
      || br_open->GetParentType() == CT_CS_PROPERTY
      || br_open->GetParentType() == CT_CPP_LAMBDA
      || br_open->GetParentType() == CT_FUNC_CALL
      || br_open->GetParentType() == CT_FUNC_CALL_USER)
   {
      Chunk  *prev = Chunk::NullChunkPtr;
      iarf_e val;

      if (br_open->GetParentType() == CT_OC_MSG_DECL)
      {
         log_rule_B("nl_oc_mdef_brace");
         val = options::nl_oc_mdef_brace();
      }
      else
      {
         if (  br_open->GetParentType() == CT_FUNC_DEF
            || br_open->GetParentType() == CT_FUNC_CLASS_DEF
            || br_open->GetParentType() == CT_OC_CLASS)
         {
            const iarf_e nl_fdef_brace_v      = options::nl_fdef_brace();
            const iarf_e nl_fdef_brace_cond_v = options::nl_fdef_brace_cond();

            if (nl_fdef_brace_cond_v == IARF_IGNORE)
            {
               val = nl_fdef_brace_v;
            }
            else // nl_fdef_brace_cond_v != IARF_IGNORE
            {
               prev = br_open->GetPrevNcNnlNi();   // Issue #2279

               if (prev->Is(CT_FPAREN_CLOSE))
               {
                  // Add or remove newline between function signature and '{',
                  // if signature ends with ')'. Overrides nl_fdef_brace.
                  log_rule_B("nl_fdef_brace_cond");
                  val = nl_fdef_brace_cond_v;
               }
               else
               {
                  // Add or remove newline between function signature and '{'.
                  log_rule_B("nl_fdef_brace");
                  val = nl_fdef_brace_v;
               }
            }
         }
         else
         {
            log_rule_B("nl_property_brace");
            log_rule_B("nl_cpp_ldef_brace");
            log_rule_B("nl_fcall_brace");
            val = ((br_open->GetParentType() == CT_CS_PROPERTY) ?
                   options::nl_property_brace() :
                   ((br_open->GetParentType() == CT_CPP_LAMBDA) ?
                    options::nl_cpp_ldef_brace() :
                    options::nl_fcall_brace()));
         }
      }

      if (val != IARF_IGNORE)
      {
         if (prev->IsNullChunk())
         {
            // Grab the chunk before the open brace
            prev = br_open->GetPrevNcNnlNi();   // Issue #2279
         }
         newline_iarf_pair(prev, br_open, val);
      }
   }

   if (br_open->GetNextNnl()->Is(CT_BRACE_CLOSE))
   {
      // Chunk is "{" and "}" with only whitespace/newlines in between

      if (br_open->GetParentType() == CT_FUNC_DEF)
      {
         // Braces belong to a function definition
         log_rule_B("nl_collapse_empty_body_functions");
         log_ruleNL("nl_collapse_empty_body_functions", br_open);

         if (options::nl_collapse_empty_body_functions())
         {
            collapse_empty_body(br_open);
            return;
         }
      }
      else
      {
         log_rule_B("nl_collapse_empty_body");
         log_ruleNL("nl_collapse_empty_body", br_open);

         if (options::nl_collapse_empty_body())
         {
            collapse_empty_body(br_open);
            return;
         }
      }
   }
   //fixes #1245 will add new line between tsquare and brace open based on nl_tsquare_brace

   if (br_open->Is(CT_BRACE_OPEN))
   {
      Chunk *chunk_closing_brace = br_open->GetClosingParen();

      if (chunk_closing_brace->IsNotNullChunk())
      {
         if (chunk_closing_brace->GetOrigLine() > br_open->GetOrigLine())
         {
            Chunk *prev = br_open->GetPrevNc();

            if (  prev->Is(CT_TSQUARE)
               && next->IsNewline())
            {
               log_rule_B("nl_tsquare_brace");
               newline_iarf_pair(prev, br_open, options::nl_tsquare_brace());
            }
         }
      }
   }
   // Eat any extra newlines after the brace open
   log_rule_B("eat_blanks_after_open_brace");

   if (options::eat_blanks_after_open_brace())
   {
      if (next->IsNewline())
      {
         log_rule_B("nl_inside_empty_func");
         log_rule_B("nl_inside_namespace");

         if (  options::nl_inside_empty_func() > 0
            && br_open->GetNextNnl()->Is(CT_BRACE_CLOSE)
            && (  br_open->GetParentType() == CT_FUNC_CLASS_DEF
               || br_open->GetParentType() == CT_FUNC_DEF))
         {
            blank_line_set(next, options::nl_inside_empty_func);
         }
         else if (  options::nl_inside_namespace() > 0
                 && br_open->GetParentType() == CT_NAMESPACE)
         {
            blank_line_set(next, options::nl_inside_namespace);
         }
         else if (next->GetNlCount() > 1)
         {
            next->SetNlCount(1);
            LOG_FMT(LBLANKD, "%s(%d): eat_blanks_after_open_brace %zu\n",
                    __func__, __LINE__, next->GetOrigLine());
            MARK_CHANGE();
         }
      }
   }
   bool nl_close_brace = false;

   // Handle the cases where the brace is part of a function call or definition
   if (is_func_call_or_def(br_open))
   {
      // Need to force a newline before the close brace, if not in a class body
      if (!br_open->TestFlags(PCF_IN_CLASS))
      {
         nl_close_brace = true;
      }
      // handle newlines after the open brace
      Chunk *pc = br_open->GetNextNcNnl();
      newline_add_between(br_open, pc);
   }
   // Grab the matching brace close
   Chunk *br_close = br_open->GetNextType(CT_BRACE_CLOSE, br_open->GetLevel());

   if (br_close->IsNullChunk())
   {
      return;
   }

   if (!nl_close_brace)
   {
      /*
       * If the open brace hits a CT_NEWLINE, CT_NL_CONT, CT_COMMENT_MULTI, or
       * CT_COMMENT_CPP without hitting anything other than CT_COMMENT, then
       * there should be a newline before the close brace.
       */
      Chunk *pc = br_open->GetNext();

      while (pc->Is(CT_COMMENT))
      {
         pc = pc->GetNext();
      }

      if (pc->IsCommentOrNewline())
      {
         nl_close_brace = true;
      }
   }
   Chunk *prev = br_close->GetPrevNcNnlNet();

   if (nl_close_brace)
   {
      newline_add_between(prev, br_close);
   }
   else
   {
      newline_del_between(prev, br_close);
   }
} // newlines_brace_pair
