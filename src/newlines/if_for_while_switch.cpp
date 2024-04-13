/**
 * @file if_for_while_switch.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#include "newlines/if_for_while_switch.h"

#include "chunk.h"
#include "log_rules.h"
#include "mark_change.h"
#include "newlines/add.h"
#include "newlines/double_newline.h"
#include "newlines/get_closing_brace.h"
#include "newlines/iarf.h"
#include "newlines/one_liner.h"
#include "newlines/remove_next_newlines.h"
#include "option.h"


using namespace uncrustify;


constexpr static auto LCURRENT = LNEWLINE;


bool newlines_if_for_while_switch(Chunk *start, iarf_e nl_opt)
{
   LOG_FUNC_ENTRY();

   log_rule_B("nl_define_macro");

   if (  nl_opt == IARF_IGNORE
      || (  start->TestFlags(PCF_IN_PREPROC)
         && !options::nl_define_macro()))
   {
      return(false);
   }
   bool  retval = false;
   Chunk *pc    = start->GetNextNcNnl();

   if (pc->Is(CT_SPAREN_OPEN))
   {
      Chunk *close_paren = pc->GetNextType(CT_SPAREN_CLOSE, pc->GetLevel());
      Chunk *brace_open  = close_paren->GetNextNcNnl();

      if (  (  brace_open->Is(CT_BRACE_OPEN)
            || brace_open->Is(CT_VBRACE_OPEN))
         && one_liner_nl_ok(brace_open))
      {
         log_rule_B("nl_multi_line_cond");

         if (options::nl_multi_line_cond())
         {
            while ((pc = pc->GetNext()) != close_paren)
            {
               if (pc->IsNewline())
               {
                  nl_opt = IARF_ADD;
                  break;
               }
            }
         }

         if (brace_open->Is(CT_VBRACE_OPEN))
         {
            // Can only add - we don't want to create a one-line here
            if (nl_opt & IARF_ADD)
            {
               newline_iarf_pair(close_paren, brace_open->GetNextNcNnl(), nl_opt);
               pc = brace_open->GetNextType(CT_VBRACE_CLOSE, brace_open->GetLevel());

               if (  !pc->GetPrevNc()->IsNewline()
                  && !pc->GetNextNc()->IsNewline()
                  && pc->GetNextNc()->IsNot(CT_VBRACE_CLOSE))
               {
                  newline_add_after(pc);
                  retval = true;
               }
            }
         }
         else
         {
            newline_iarf_pair(close_paren, brace_open, nl_opt);
            Chunk *next = brace_open->GetNextNcNnl();

            if (brace_open->GetType() != next->GetType())                       // Issue #2836
            {
               newline_add_between(brace_open, brace_open->GetNextNcNnl());
            }
            // Make sure nothing is cuddled with the closing brace
            pc = brace_open->GetNextType(CT_BRACE_CLOSE, brace_open->GetLevel());
            newline_add_between(pc, pc->GetNextNcNnlNet());
            retval = true;
         }
      }
   }
   return(retval);
} // newlines_if_for_while_switch


/**
 * Add or remove extra newline after end of the block started in chunk.
 * Doesn't do anything if close brace after it
 * Interesting issue is that at this point, nls can be before or after vbraces
 * VBraces will stay VBraces, conversion to real ones should have already happened
 * "if (...)\ncode\ncode" or "if (...)\ncode\n\ncode"
 */
void newlines_if_for_while_switch_post_blank_lines(Chunk *start, uncrustify::iarf_e nl_opt)
{
   LOG_FUNC_ENTRY();

   Chunk *prev;

   LOG_FMT(LNEWLINE, "%s(%d): start->Text() is '%s', type is %s, orig line is %zu, orig col is %zu\n",
           __func__, __LINE__, start->Text(), get_token_name(start->GetType()), start->GetOrigLine(), start->GetOrigCol());

   log_rule_B("nl_define_macro");

   if (  nl_opt == IARF_IGNORE
      || (  start->TestFlags(PCF_IN_PREPROC)
         && !options::nl_define_macro()))
   {
      return;
   }
   Chunk *pc = get_closing_brace(start);

   // first find ending brace
   if (pc->IsNullChunk())
   {
      return;
   }
   LOG_FMT(LNEWLINE, "%s(%d): pc->Text() is '%s', type is %s, orig line is %zu, orig col is %zu\n",
           __func__, __LINE__, pc->Text(), get_token_name(pc->GetType()), pc->GetOrigLine(), pc->GetOrigCol());

   /*
    * if we're dealing with an if, we actually want to add or remove
    * blank lines after any else
    */
   if (start->Is(CT_IF))
   {
      Chunk *next;

      while (true)
      {
         next = pc->GetNextNcNnl();

         if (  next->IsNotNullChunk()
            && (  next->Is(CT_ELSE)
               || next->Is(CT_ELSEIF)))
         {
            // point to the closing brace of the else
            if ((pc = get_closing_brace(next))->IsNullChunk())
            {
               return;
            }
            LOG_FMT(LNEWLINE, "%s(%d): pc->Text() is '%s', type %s, orig line %zu, orig col %zu\n",
                    __func__, __LINE__, pc->Text(), get_token_name(pc->GetType()), pc->GetOrigLine(), pc->GetOrigCol());
         }
         else
         {
            break;
         }
      }
   }

   /*
    * if we're dealing with a do/while, we actually want to add or
    * remove blank lines after while and its condition
    */
   if (start->Is(CT_DO))
   {
      // point to the next semicolon
      if ((pc = pc->GetNextType(CT_SEMICOLON, start->GetLevel()))->IsNullChunk())
      {
         return;
      }
      LOG_FMT(LNEWLINE, "%s(%d): pc->Text() is '%s', type %s, orig line %zu, orig col %zu\n",
              __func__, __LINE__, pc->Text(), get_token_name(pc->GetType()), pc->GetOrigLine(), pc->GetOrigCol());
   }
   bool isVBrace = (pc->Is(CT_VBRACE_CLOSE));

   if (isVBrace)
   {
      LOG_FMT(LNEWLINE, "%s(%d): isVBrace is TRUE\n", __func__, __LINE__);
   }
   else
   {
      LOG_FMT(LNEWLINE, "%s(%d): isVBrace is FALSE\n", __func__, __LINE__);
   }

   if ((prev = pc->GetPrevNvb())->IsNullChunk())
   {
      return;
   }
   bool have_pre_vbrace_nl = isVBrace && prev->IsNewline();

   if (have_pre_vbrace_nl)
   {
      LOG_FMT(LNEWLINE, "%s(%d): have_pre_vbrace_nl is TRUE\n", __func__, __LINE__);
   }
   else
   {
      LOG_FMT(LNEWLINE, "%s(%d): have_pre_vbrace_nl is FALSE\n", __func__, __LINE__);
   }

   if (nl_opt & IARF_REMOVE)
   {
      Chunk *next;

      // if chunk before is a vbrace, remove any newlines after it
      if (have_pre_vbrace_nl)
      {
         if (prev->GetNlCount() != 1)
         {
            prev->SetNlCount(1);
            MARK_CHANGE();
         }
         remove_next_newlines(pc);
      }
      else if (  ((next = pc->GetNextNvb())->IsNewline())
              && !next->TestFlags(PCF_VAR_DEF))
      {
         // otherwise just deal with newlines after brace
         if (next->GetNlCount() != 1)
         {
            next->SetNlCount(1);
            MARK_CHANGE();
         }
         remove_next_newlines(next);
      }
   }

   // may have a newline before and after vbrace
   // don't do anything with it if the next non newline chunk is a closing brace
   if (nl_opt & IARF_ADD)
   {
      Chunk *next = pc->GetNextNnl();

      do
      {
         if (next->IsNullChunk())
         {
            return;
         }

         if (next->IsNot(CT_VBRACE_CLOSE))
         {
            break;
         }
         next = next->GetNextNnl();
      } while (true);

      LOG_FMT(LNEWLINE, "%s(%d): next->Text() is '%s', type %s, orig line %zu, orig col %zu\n",
              __func__, __LINE__, next->Text(), get_token_name(next->GetType()), next->GetOrigLine(), next->GetOrigCol());

      if (next->IsNot(CT_BRACE_CLOSE))
      {
         // if vbrace, have to check before and after
         // if chunk before vbrace, check its count
         size_t nl_count = have_pre_vbrace_nl ? prev->GetNlCount() : 0;
         LOG_FMT(LNEWLINE, "%s(%d): new line count %zu\n", __func__, __LINE__, nl_count);

         if ((next = pc->GetNextNvb())->IsNewline())
         {
            LOG_FMT(LNEWLINE, "%s(%d): next->Text() is '%s', type %s, orig line %zu, orig col %zu\n",
                    __func__, __LINE__, next->Text(), get_token_name(next->GetType()), next->GetOrigLine(), next->GetOrigCol());
            nl_count += next->GetNlCount();
            LOG_FMT(LNEWLINE, "%s(%d): new line count is %zu\n", __func__, __LINE__, nl_count);
         }

         // if we have no newlines, add one and make it double
         if (nl_count == 0)
         {
            LOG_FMT(LNEWLINE, "%s(%d): new line count is 0\n", __func__, __LINE__);

            if (  ((next = pc->GetNext())->IsNotNullChunk())
               && next->IsComment())
            {
               LOG_FMT(LNEWLINE, "%s(%d): next->Text() is '%s', type %s, orig line %zu, orig col %zu\n",
                       __func__, __LINE__, next->Text(), get_token_name(next->GetType()), next->GetOrigLine(), next->GetOrigCol());
               pc = next;
               LOG_FMT(LNEWLINE, "%s(%d): pc->Text() is '%s', type %s, orig line %zu, orig col %zu\n",
                       __func__, __LINE__, pc->Text(), get_token_name(pc->GetType()), pc->GetOrigLine(), pc->GetOrigCol());
            }

            if ((next = newline_add_after(pc))->IsNullChunk())
            {
               return;
            }
            LOG_FMT(LNEWLINE, "%s(%d): next->Text() is '%s', type %s, orig line %zu, orig col %zu\n",
                    __func__, __LINE__, next->Text(), get_token_name(next->GetType()), next->GetOrigLine(), next->GetOrigCol());
            double_newline(next);
         }
         else if (nl_count == 1) // if we don't have enough newlines
         {
            LOG_FMT(LNEWLINE, "%s(%d): new line count is 1\n", __func__, __LINE__);

            // if we have a preceding vbrace, add one after it
            if (have_pre_vbrace_nl)
            {
               LOG_FMT(LNEWLINE, "%s(%d): have_pre_vbrace_nl is TRUE\n", __func__, __LINE__);
               next = newline_add_after(pc);
               LOG_FMT(LNEWLINE, "%s(%d): next->Text() is '%s', type %s, orig line %zu, orig col %zu\n",
                       __func__, __LINE__, next->Text(), get_token_name(next->GetType()), next->GetOrigLine(), next->GetOrigCol());
            }
            else
            {
               LOG_FMT(LNEWLINE, "%s(%d): have_pre_vbrace_nl is FALSE\n", __func__, __LINE__);
               prev = next->GetPrevNnl();
               LOG_FMT(LNEWLINE, "%s(%d): prev->Text() is '%s', type %s, orig line %zu, orig col %zu\n",
                       __func__, __LINE__, prev->Text(), get_token_name(prev->GetType()), prev->GetOrigLine(), prev->GetOrigCol());
               pc = next->GetNextNl();
               LOG_FMT(LNEWLINE, "%s(%d): pc->Text() is '%s', type %s, orig line %zu, orig col %zu\n",
                       __func__, __LINE__, pc->Text(), get_token_name(pc->GetType()), pc->GetOrigLine(), pc->GetOrigCol());
               Chunk *pc2 = pc->GetNext();

               if (pc2->IsNotNullChunk())
               {
                  pc = pc2;
                  LOG_FMT(LNEWLINE, "%s(%d): pc->Text() is '%s', type %s, orig line %zu, orig col %zu\n",
                          __func__, __LINE__, pc->Text(), get_token_name(pc->GetType()), pc->GetOrigLine(), pc->GetOrigCol());
               }
               else
               {
                  LOG_FMT(LNEWLINE, "%s(%d): no next found: <EOF>\n", __func__, __LINE__);
               }
               log_rule_B("nl_squeeze_ifdef");

               if (  pc->Is(CT_PREPROC)
                  && pc->GetParentType() == CT_PP_ENDIF
                  && options::nl_squeeze_ifdef())
               {
                  LOG_FMT(LNEWLINE, "%s(%d): cannot add newline after orig line %zu due to nl_squeeze_ifdef\n",
                          __func__, __LINE__, prev->GetOrigLine());
               }
               else
               {
                  // make newline after double
                  LOG_FMT(LNEWLINE, "%s(%d): call double_newline\n", __func__, __LINE__);
                  double_newline(next);
               }
            }
         }
      }
   }
} // newlines_if_for_while_switch_post_blank_lines


/**
 * Add or remove extra newline before the chunk.
 * Adds before comments
 * Doesn't do anything if open brace before it
 * "code\n\ncomment\nif (...)" or "code\ncomment\nif (...)"
 */
void newlines_if_for_while_switch_pre_blank_lines(Chunk *start, uncrustify::iarf_e nl_opt)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNEWLINE, "%s(%d): start->Text() is '%s', type is %s, orig line is %zu, orig col is %zu\n",
           __func__, __LINE__, start->Text(), get_token_name(start->GetType()), start->GetOrigLine(), start->GetOrigCol());

   log_rule_B("nl_define_macro");

   if (  nl_opt == IARF_IGNORE
      || (  start->TestFlags(PCF_IN_PREPROC)
         && !options::nl_define_macro()))
   {
      return;
   }

   /*
    * look backwards until we find
    *   open brace (don't add or remove)
    *   2 newlines in a row (don't add)
    *   something else (don't remove)
    */
   for (Chunk *pc = start->GetPrev(); pc->IsNotNullChunk(); pc = pc->GetPrev())
   {
      size_t level    = start->GetLevel();
      bool   do_add   = (nl_opt & IARF_ADD) != IARF_IGNORE;  // forcing value to bool
      Chunk  *last_nl = Chunk::NullChunkPtr;

      if (pc->IsNewline())
      {
         last_nl = pc;

         // if we found 2 or more in a row
         if (  pc->GetNlCount() > 1
            || pc->GetPrevNvb()->IsNewline())
         {
            // need to remove
            if (  (nl_opt & IARF_REMOVE)
               && !pc->TestFlags(PCF_VAR_DEF))
            {
               // if we're also adding, take care of that here
               size_t nl_count = do_add ? 2 : 1;

               if (nl_count != pc->GetNlCount())
               {
                  pc->SetNlCount(nl_count);
                  MARK_CHANGE();
               }
               Chunk *prev;

               // can keep using pc because anything other than newline stops loop, and we delete if newline
               while ((prev = pc->GetPrevNvb())->IsNewline())
               {
                  // Make sure we don't combine a preproc and non-preproc
                  if (!prev->SafeToDeleteNl())
                  {
                     break;
                  }
                  Chunk::Delete(prev);
                  MARK_CHANGE();
               }
            }
            return;
         }
      }
      else if (  pc->IsBraceOpen()
              || pc->GetLevel() < level)
      {
         return;
      }
      else if (pc->IsComment())
      {
         // vbrace close is ok because it won't go into output, so we should skip it
         last_nl = Chunk::NullChunkPtr;
         continue;
      }
      else
      {
         if (  pc->Is(CT_CASE_COLON)
            && options::nl_before_ignore_after_case())
         {
            return;
         }

         if (do_add) // we found something previously besides a comment or a new line
         {
            // if we have run across a newline
            if (last_nl->IsNotNullChunk())
            {
               if (last_nl->GetNlCount() < 2)
               {
                  double_newline(last_nl);
               }
            }
            else
            {
               Chunk *next;

               // we didn't run into a newline, so we need to add one
               if (  ((next = pc->GetNext())->IsNotNullChunk())
                  && next->IsComment())
               {
                  pc = next;
               }

               if ((last_nl = newline_add_after(pc))->IsNotNullChunk())
               {
                  double_newline(last_nl);
               }
            }
         }
         return;
      }
   }
} // newlines_if_for_while_switch_pre_blank_lines
