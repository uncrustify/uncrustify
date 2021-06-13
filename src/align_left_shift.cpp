/**
 * @file align_left_shift.cpp
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align_left_shift.h"

#include "align_stack.h"
#include "indent.h"
#include "log_rules.h"

constexpr static auto LCURRENT = LALIGN;

using namespace uncrustify;


void align_left_shift(void)
{
   LOG_FUNC_ENTRY();

   chunk_t    *start = nullptr;
   AlignStack as;

   as.Start(255);

   chunk_t *pc = chunk_get_head();

   while (pc != nullptr)
   {
      if (chunk_is_newline(pc))
      {
         LOG_FMT(LALIGN, "%s(%d): orig_line is %zu, <Newline>\n", __func__, __LINE__, pc->orig_line);
      }
      else
      {
         char copy[1000];
         LOG_FMT(LALIGN, "%s(%d): orig_line is %zu, orig_col is %zu, pc->text() '%s'\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->elided_text(copy));
      }

      if (  start != nullptr
         && ((pc->flags & PCF_IN_PREPROC) != (start->flags & PCF_IN_PREPROC)))
      {
         // a change in preproc status restarts the aligning
         as.Flush();
         start = nullptr;
      }
      else if (chunk_is_newline(pc))
      {
         as.NewLines(pc->nl_count);
      }
      else if (  start != nullptr
              && pc->level < start->level)
      {
         // A drop in level restarts the aligning
         as.Flush();
         start = nullptr;
      }
      else if (  start != nullptr
              && pc->level > start->level)
      {
         // Ignore any deeper levels when aligning
      }
      else if (chunk_is_token(pc, CT_SEMICOLON))
      {
         // A semicolon at the same level flushes
         as.Flush();
         start = nullptr;
      }
      else if (  !pc->flags.test(PCF_IN_ENUM)
              && !pc->flags.test(PCF_IN_TYPEDEF)
              && chunk_is_lshift_str(pc))
      {
         if (get_chunk_parent_type(pc) == CT_OPERATOR)
         {
            // Ignore operator<<
         }
         else if (as.m_aligned.Empty())
         {
            /*
             * check if the first one is actually on a blank line and then
             * indent it. Eg:
             *
             *      cout
             *          << "something";
             */
            chunk_t *prev = chunk_get_prev(pc);

            if (  prev != nullptr
               && chunk_is_newline(prev))
            {
               log_rule_B("indent_columns");
               indent_to_column(pc, pc->column_indent + options::indent_columns());
               pc->column_indent = pc->column;
               chunk_flags_set(pc, PCF_DONT_INDENT);
            }
            // first one can be anywhere
            as.Add(pc);
            start = pc;
         }
         else if (chunk_is_newline(chunk_get_prev(pc)))
         {
            // subsequent ones must be after a newline
            as.Add(pc);
         }
      }
      else if (!as.m_aligned.Empty())
      {
         /*
          * check if the given statement is on a line of its own, immediately following <<
          * and then it. Eg:
          *
          *      cout <<
          *          "something";
          */
         chunk_t *prev = chunk_get_prev(pc);

         if (  prev != nullptr
            && chunk_is_newline(prev))
         {
            log_rule_B("indent_columns");
            indent_to_column(pc, pc->column_indent + options::indent_columns());
            pc->column_indent = pc->column;
            chunk_flags_set(pc, PCF_DONT_INDENT);
         }
      }
      pc = chunk_get_next(pc);
   }
   as.End();
} // align_left_shift
