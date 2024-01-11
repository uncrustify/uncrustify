/**
 * @file newlines_if_for_while_switch_pre_blank_lines.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "newlines_if_for_while_switch_pre_blank_lines.h"

#include "double_newline.h"
#include "log_rules.h"
#include "mark_change.h"
#include "newline_add.h"
#include "uncrustify.h"

constexpr static auto LCURRENT = LNEWLINE;

#define MARK_CHANGE()    mark_change(__func__, __LINE__)


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
