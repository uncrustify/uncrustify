/**
 * @file newlines.cpp
 * Adds or removes newlines.
 *
 * Informations
 *   "Ignore" means do not change it.
 *   "Add" in the context of spaces means make sure there is at least 1.
 *   "Add" elsewhere means make sure one is present.
 *   "Remove" mean remove the space/brace/newline/etc.
 *   "Force" in the context of spaces means ensure that there is exactly 1.
 *   "Force" in other contexts means the same as "add".
 *
 *   Rmk: spaces = space + nl
 *
 * @author  Ben Gardner
 * @author  Guy Maurel since version 0.62 for uncrustify4Qt
 *          October 2015, 2016
 * @license GPL v2+
 */
#include "newlines.h"
#include "uncrustify_types.h"
#include "chunk_list.h"
#include "prototypes.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "unc_ctype.h"
#include "unc_tools.h"
#include "uncrustify.h"
#include "indent.h"
#include "space.h"
#include "combine.h"
#include "keywords.h"
#include "options.h"

#include <algorithm>


using namespace std;


static void mark_change(const char *func, size_t line);


/**
 * Check to see if we are allowed to increase the newline count.
 * We can't increase the newline count:
 *  - if nl_squeeze_ifdef and a preproc is after the newline.
 *  - if eat_blanks_before_close_brace and the next is '}'
 *  - if eat_blanks_after_open_brace and the prev is '{'
 */
static bool can_increase_nl(chunk_t *nl);


//! Double the newline, if allowed.
static void double_newline(chunk_t *nl);


/**
 * Basic approach:
 * 1. Find next open brace
 * 2. Find next close brace
 * 3. Determine why the braces are there
 * a. struct/union/enum "enum [name] {"
 * c. assignment "= {"
 * b. if/while/switch/for/etc ") {"
 * d. else "} else {"
 */
static void setup_newline_add(chunk_t *prev, chunk_t *nl, chunk_t *next);


//! Make sure there is a blank line after a commented group of values
static void newlines_double_space_struct_enum_union(chunk_t *open_brace);


//! If requested, make sure each entry in an enum is on its own line
static void newlines_enum_entries(chunk_t *open_brace, argval_t av);


/**
 * Checks to see if it is OK to add a newline around the chunk.
 * Don't want to break one-liners...
 * return value:
 *  true: a new line may be added
 * false: a new line may NOT be added
 */
static bool one_liner_nl_ok(chunk_t *pc);


static void nl_create_one_liner(chunk_t *vbrace_open);


//! Find the next newline or nl_cont
static void nl_handle_define(chunk_t *pc);


/**
 * Does the Ignore, Add, Remove, or Force thing between two chunks
 *
 * @param before  The first chunk
 * @param after   The second chunk
 * @param av      The IARF value
 */
static void newline_iarf_pair(chunk_t *before, chunk_t *after, argval_t av);


/**
 * Adds newlines to multi-line function call/decl/def
 * Start points to the open paren
 */
static void newline_func_multi_line(chunk_t *start);


/**
 * Formats a function declaration
 * Start points to the open paren
 */
static void newline_func_def_or_call(chunk_t *start);


/**
 * Formats a message, adding newlines before the item before the colons.
 *
 * Start points to the open '[' in:
 * [myObject doFooWith:arg1 name:arg2  // some lines with >1 arg
 *            error:arg3];
 */
static void newline_oc_msg(chunk_t *start);


//! Ensure that the next non-comment token after close brace is a newline
static void newline_end_newline(chunk_t *br_close);


/**
 * Add or remove a newline between the closing paren and opening brace.
 * Also uncuddles anything on the closing brace. (may get fixed later)
 *
 * "if (...) { \n" or "if (...) \n { \n"
 *
 * For virtual braces, we can only add a newline after the vbrace open.
 * If we do so, also add a newline after the vbrace close.
 */
static bool newlines_if_for_while_switch(chunk_t *start, argval_t nl_opt);


/**
 * Add or remove extra newline before the chunk.
 * Adds before comments
 * Doesn't do anything if open brace before it
 * "code\n\ncomment\nif (...)" or "code\ncomment\nif (...)"
 */
static void newlines_if_for_while_switch_pre_blank_lines(chunk_t *start, argval_t nl_opt);


static void _blank_line_set(chunk_t *pc, const char *text, uncrustify_options uo);


/**
 * Add one/two newline(s) before the chunk.
 * Adds before comments
 * Adds before destructor
 * Doesn't do anything if open brace before it
 * "code\n\ncomment\nif (...)" or "code\ncomment\nif (...)"
 */
static void newlines_func_pre_blank_lines(chunk_t *start);


static chunk_t *get_closing_brace(chunk_t *start);


/**
 * remove any consecutive newlines following this chunk
 * skip vbraces
 */
static void remove_next_newlines(chunk_t *start);


/**
 * Add or remove extra newline after end of the block started in chunk.
 * Doesn't do anything if close brace after it
 * Interesting issue is that at this point, nls can be before or after vbraces
 * VBraces will stay VBraces, conversion to real ones should have already happened
 * "if (...)\ncode\ncode" or "if (...)\ncode\n\ncode"
 */
static void newlines_if_for_while_switch_post_blank_lines(chunk_t *start, argval_t nl_opt);


/**
 * Adds or removes a newline between the keyword and the open brace.
 * If there is something after the '{' on the same line, then
 * the newline is removed unconditionally.
 * If there is a '=' between the keyword and '{', do nothing.
 *
 * "struct [name] {" or "struct [name] \n {"
 */
static void newlines_struct_union(chunk_t *start, argval_t nl_opt, bool leave_trailing);
static void newlines_enum(chunk_t *start);


/**
 * Cuddles or un-cuddles a chunk with a previous close brace
 *
 * "} while" vs "} \n while"
 * "} else" vs "} \n else"
 *
 * @param start  The chunk - should be CT_ELSE or CT_WHILE_OF_DO
 */
static void newlines_cuddle_uncuddle(chunk_t *start, argval_t nl_opt);


/**
 * Adds/removes a newline between else and '{'.
 * "else {" or "else \n {"
 */
static void newlines_do_else(chunk_t *start, argval_t nl_opt);


//! Put a newline before and after a block of variable definitions
static chunk_t *newline_def_blk(chunk_t *start, bool fn_top);


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
static void newlines_brace_pair(chunk_t *br_open);


/**
 * Put a empty line between the 'case' statement and the previous case colon
 * or semicolon.
 * Does not work with PAWN (?)
 */
static void newline_case(chunk_t *start);


static void newline_case_colon(chunk_t *start);


//! Put a blank line before a return statement, unless it is after an open brace
static void newline_before_return(chunk_t *start);


/**
 * Put a empty line after a return statement, unless it is followed by a
 * close brace.
 *
 * May not work with PAWN
 */
static void newline_after_return(chunk_t *start);


static void _blank_line_max(chunk_t *pc, const char *text, uncrustify_options uo);


#define MARK_CHANGE()    mark_change(__func__, __LINE__)


static void mark_change(const char *func, size_t line)
{
   LOG_FUNC_ENTRY();
   cpd.changes++;
   if (cpd.pass_count == 0)
   {
      LOG_FMT(LCHANGE, "%s(%d): change %d on %s:%zu\n",
              __func__, __LINE__, cpd.changes, func, line);
   }
}


static bool can_increase_nl(chunk_t *nl)
{
   LOG_FUNC_ENTRY();
   chunk_t *prev = chunk_get_prev_nc(nl);
   chunk_t *pcmt = chunk_get_prev(nl);
   chunk_t *next = chunk_get_next(nl);

   if (cpd.settings[UO_nl_squeeze_ifdef].b)
   {
      if (  chunk_is_token(prev, CT_PREPROC)
         && prev->parent_type == CT_PP_ENDIF
         && (prev->level > 0 || cpd.settings[UO_nl_squeeze_ifdef_top_level].b))
      {
         LOG_FMT(LBLANKD, "%s(%d): nl_squeeze_ifdef %zu (prev) pp_lvl=%zu rv=0\n",
                 __func__, __LINE__, nl->orig_line, nl->pp_level);
         return(false);
      }
      if (  next
         && next->type == CT_PREPROC
         && next->parent_type == CT_PP_ENDIF
         && (next->level > 0 || cpd.settings[UO_nl_squeeze_ifdef_top_level].b))
      {
         bool rv = ifdef_over_whole_file()
                   && (next->flags & PCF_WF_ENDIF);
         LOG_FMT(LBLANKD, "%s(%d): nl_squeeze_ifdef %zu (next) pp_lvl=%zu rv=%d\n",
                 __func__, __LINE__, nl->orig_line, nl->pp_level, rv);
         return(rv);
      }
   }

   if (  cpd.settings[UO_eat_blanks_before_close_brace].b
      && chunk_is_token(next, CT_BRACE_CLOSE))
   {
      LOG_FMT(LBLANKD, "%s(%d): eat_blanks_before_close_brace %zu\n",
              __func__, __LINE__, nl->orig_line);
      return(false);
   }

   if (  cpd.settings[UO_eat_blanks_after_open_brace].b
      && chunk_is_token(prev, CT_BRACE_OPEN))
   {
      LOG_FMT(LBLANKD, "%s(%d): eat_blanks_after_open_brace %zu\n",
              __func__, __LINE__, nl->orig_line);
      return(false);
   }

   if (!pcmt && (cpd.settings[UO_nl_start_of_file].a != AV_IGNORE))
   {
      LOG_FMT(LBLANKD, "%s(%d): SOF no prev %zu\n", __func__, __LINE__, nl->orig_line);
      return(false);
   }

   if (!next && (cpd.settings[UO_nl_end_of_file].a != AV_IGNORE))
   {
      LOG_FMT(LBLANKD, "%s(%d): EOF no next %zu\n", __func__, __LINE__, nl->orig_line);
      return(false);
   }

   return(true);
} // can_increase_nl


static void double_newline(chunk_t *nl)
{
   LOG_FUNC_ENTRY();
   chunk_t *prev = chunk_get_prev(nl);
   if (prev == nullptr)
   {
      return;
   }
   LOG_FMT(LNEWLINE, "%s(%d): add newline after ", __func__, __LINE__);
   if (prev->type == CT_VBRACE_CLOSE)
   {
      LOG_FMT(LNEWLINE, "VBRACE_CLOSE ");
   }
   else
   {
      LOG_FMT(LNEWLINE, "'%s' ", prev->text());
   }
   LOG_FMT(LNEWLINE, "on line %zu", prev->orig_line);

   if (!can_increase_nl(nl))
   {
      LOG_FMT(LNEWLINE, " - denied\n");
      return;
   }
   LOG_FMT(LNEWLINE, " - done\n");
   if (nl->nl_count != 2)
   {
      nl->nl_count = 2;
      MARK_CHANGE();
   }
}


static void setup_newline_add(chunk_t *prev, chunk_t *nl, chunk_t *next)
{
   LOG_FUNC_ENTRY();
   if (  !prev
      || !nl
      || !next)
   {
      return;
   }

   undo_one_liner(prev);

   nl->orig_line   = prev->orig_line;
   nl->level       = prev->level;
   nl->brace_level = prev->brace_level;
   nl->pp_level    = prev->pp_level;
   nl->nl_count    = 1;
   nl->flags       = (prev->flags & PCF_COPY_FLAGS) & ~PCF_IN_PREPROC;
   nl->orig_col    = prev->orig_col_end;
   nl->column      = prev->orig_col;
   if (  (prev->flags & PCF_IN_PREPROC)
      && (next->flags & PCF_IN_PREPROC))
   {
      chunk_flags_set(nl, PCF_IN_PREPROC);
   }
   if (nl->flags & PCF_IN_PREPROC)
   {
      set_chunk_type(nl, CT_NL_CONT);
      nl->str = "\\\n";
   }
   else
   {
      set_chunk_type(nl, CT_NEWLINE);
      nl->str = "\n";
   }
}


chunk_t *newline_add_before(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t nl;
   chunk_t *prev;

   prev = chunk_get_prev_nvb(pc);
   if (chunk_is_newline(prev))
   {
      // Already has a newline before this chunk
      return(prev);
   }

   LOG_FMT(LNEWLINE, "%s(%d): '%s' on orig_line is %zu, orig_col is %zu, pc->column is %zu",
           __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col, pc->column);
   log_func_stack_inline(LNEWLINE);

   setup_newline_add(prev, &nl, pc);
   LOG_FMT(LNEWLINE, "%s(%d): '%s' on nl.orig_line is %zu, nl.orig_col is %zu, nl.column is %zu\n",
           __func__, __LINE__, nl.text(), nl.orig_line, nl.orig_col, nl.column);

   MARK_CHANGE();
   return(chunk_add_before(&nl, pc));
}


chunk_t *newline_force_before(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *nl = newline_add_before(pc);
   if (nl && nl->nl_count > 1)
   {
      nl->nl_count = 1;
      MARK_CHANGE();
   }
   return(nl);
}


chunk_t *newline_add_after(chunk_t *pc)
{
   LOG_FUNC_ENTRY();

   if (pc == nullptr)
   {
      return(nullptr);
   }

   chunk_t *next = chunk_get_next_nvb(pc);
   if (chunk_is_newline(next))
   {
      // Already has a newline after this chunk
      return(next);
   }

   LOG_FMT(LNEWLINE, "%s(%d): '%s' on line %zu",
           __func__, __LINE__, pc->text(), pc->orig_line);
   log_func_stack_inline(LNEWLINE);

   chunk_t nl;
   setup_newline_add(pc, &nl, next);

   MARK_CHANGE();
   return(chunk_add_after(&nl, pc));
}


chunk_t *newline_force_after(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *nl = newline_add_after(pc); // add a newline
   if (nl && nl->nl_count > 1)          // check if there are more than 1 newline
   {
      nl->nl_count = 1;                 // if so change the newline count back to 1
      MARK_CHANGE();
   }
   return(nl);
}


static void newline_end_newline(chunk_t *br_close)
{
   LOG_FUNC_ENTRY();
   chunk_t *next = chunk_get_next(br_close);
   chunk_t nl;

   if (!chunk_is_newline(next) && !chunk_is_comment(next))
   {
      nl.orig_line = br_close->orig_line;
      nl.nl_count  = 1;
      nl.flags     = (br_close->flags & PCF_COPY_FLAGS) & ~PCF_IN_PREPROC;
      if (  (br_close->flags & PCF_IN_PREPROC)
         && next != nullptr
         && (next->flags & PCF_IN_PREPROC))
      {
         nl.flags |= PCF_IN_PREPROC;
      }
      if (nl.flags & PCF_IN_PREPROC)
      {
         nl.type = CT_NL_CONT;
         nl.str  = "\\\n";
      }
      else
      {
         nl.type = CT_NEWLINE;
         nl.str  = "\n";
      }
      MARK_CHANGE();
      LOG_FMT(LNEWLINE, "%s(%d): %zu:%zu add newline after '%s'\n",
              __func__, __LINE__, br_close->orig_line, br_close->orig_col, br_close->text());
      chunk_add_after(&nl, br_close);
   }
}


static void newline_min_after(chunk_t *ref, size_t count, UINT64 flag)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNEWLINE, "%s(%d): '%s' line %zu - count=%zu flg=0x%" PRIx64 ":",
           __func__, __LINE__, ref->text(), ref->orig_line, count, flag);
   log_func_stack_inline(LNEWLINE);

   chunk_t *pc = ref;
   do
   {
      pc = chunk_get_next(pc);
   } while (pc != nullptr && !chunk_is_newline(pc));

   if (pc != nullptr)                 // Coverity CID 76002
   {
      LOG_FMT(LNEWLINE, "%s(%d): on %s, line %zu, col %zu\n",
              __func__, __LINE__, get_token_name(pc->type), pc->orig_line, pc->orig_col);
   }

   chunk_t *next = chunk_get_next(pc);
   if (!next)
   {
      return;
   }
   if (  chunk_is_comment(next)
      && next->nl_count == 1
      && chunk_is_comment(chunk_get_prev(pc)))
   {
      newline_min_after(next, count, flag);
      return;
   }

   chunk_flags_set(pc, flag);
   if (chunk_is_newline(pc) && can_increase_nl(pc))
   {
      if (pc->nl_count < count)
      {
         pc->nl_count = count;
         MARK_CHANGE();
      }
   }
} // newline_min_after


chunk_t *newline_add_between(chunk_t *start, chunk_t *end)
{
   LOG_FUNC_ENTRY();

   if (start == nullptr || end == nullptr)
   {
      return(nullptr);
   }

   LOG_FMT(LNEWLINE, "%s(%d): '%s'[%s] line %zu:%zu and '%s' line %zu:%zu :",
           __func__, __LINE__, start->text(), get_token_name(start->type),
           start->orig_line, start->orig_col,
           end->text(), end->orig_line, end->orig_col);
   log_func_stack_inline(LNEWLINE);

   // Back-up check for one-liners (should never be true!)
   if (!one_liner_nl_ok(start))
   {
      return(nullptr);
   }

   /*
    * Scan for a line break, if there is a line break between start and end
    * we won't add another one
    */
   for (chunk_t *pc = start; pc != end; pc = chunk_get_next(pc))
   {
      if (chunk_is_newline(pc))
      {
         return(pc);
      }
   }

   /*
    * If the second one is a brace open, then check to see
    * if a comment + newline follows
    */
   if (end->type == CT_BRACE_OPEN)
   {
      chunk_t *pc = chunk_get_next(end);
      if (chunk_is_comment(pc))
      {
         pc = chunk_get_next(pc);
         if (chunk_is_newline(pc))
         {
            // Move the open brace to after the newline
            chunk_move_after(end, pc);
            return(pc);
         }
      }
   }

   return(newline_add_before(end));
} // newline_add_between


void newline_del_between(chunk_t *start, chunk_t *end)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNEWLINE, "%s(%d): '%s' line %zu:%zu and '%s' line %zu:%zu : preproc=%d/%d ",
           __func__, __LINE__, start->text(), start->orig_line, start->orig_col,
           end->text(), end->orig_line, end->orig_col,
           ((start->flags & PCF_IN_PREPROC) != 0),
           ((end->flags & PCF_IN_PREPROC) != 0));
   log_func_stack_inline(LNEWLINE);

   // Can't remove anything if the preproc status differs
   if (!chunk_same_preproc(start, end))
   {
      return;
   }

   chunk_t *pc           = start;
   bool    start_removed = false;
   do
   {
      chunk_t *next = chunk_get_next(pc);
      if (chunk_is_newline(pc))
      {
         chunk_t *prev = chunk_get_prev(pc);
         if (  (!chunk_is_comment(prev) && !chunk_is_comment(next))
            || chunk_is_newline(prev)
            || chunk_is_newline(next))
         {
            if (chunk_safe_to_del_nl(pc))
            {
               if (pc == start)
               {
                  start_removed = true;
               }

               chunk_del(pc);
               MARK_CHANGE();
               if (prev != nullptr)
               {
                  align_to_column(next, prev->column + space_col_align(prev, next));
               }
            }
         }
         else
         {
            if (pc->nl_count > 1)
            {
               pc->nl_count = 1;
               MARK_CHANGE();
            }
         }
      }
      pc = next;
   } while (pc != end);

   if (  !start_removed
      && chunk_is_str(end, "{", 1)
      && (  chunk_is_str(start, ")", 1)
         || start->type == CT_DO
         || start->type == CT_ELSE))
   {
      chunk_move_after(end, start);
   }
} // newline_del_between


static bool newlines_if_for_while_switch(chunk_t *start, argval_t nl_opt)
{
   LOG_FUNC_ENTRY();

   if (  nl_opt == AV_IGNORE
      || (  (start->flags & PCF_IN_PREPROC)
         && !cpd.settings[UO_nl_define_macro].b))
   {
      return(false);
   }

   bool    retval = false;
   chunk_t *pc    = chunk_get_next_ncnl(start);
   if (chunk_is_token(pc, CT_SPAREN_OPEN))
   {
      chunk_t *close_paren = chunk_get_next_type(pc, CT_SPAREN_CLOSE, pc->level);
      chunk_t *brace_open  = chunk_get_next_ncnl(close_paren);

      if (  brace_open != nullptr
         && (  brace_open->type == CT_BRACE_OPEN
            || brace_open->type == CT_VBRACE_OPEN)
         && one_liner_nl_ok(brace_open))
      {
         if (cpd.settings[UO_nl_multi_line_cond].b)
         {
            while ((pc = chunk_get_next(pc)) != close_paren)
            {
               if (chunk_is_newline(pc))
               {
                  nl_opt = AV_ADD;
                  break;
               }
            }
         }

         if (brace_open->type == CT_VBRACE_OPEN)
         {
            // Can only add - we don't want to create a one-line here
            if (nl_opt & AV_ADD)
            {
               newline_iarf_pair(close_paren, chunk_get_next_ncnl(brace_open), nl_opt);
               pc = chunk_get_next_type(brace_open, CT_VBRACE_CLOSE, brace_open->level);
               if (  !chunk_is_newline(chunk_get_prev_nc(pc))
                  && !chunk_is_newline(chunk_get_next_nc(pc)))
               {
                  newline_add_after(pc);
                  retval = true;
               }
            }
         }
         else
         {
            newline_iarf_pair(close_paren, brace_open, nl_opt);

            newline_add_between(brace_open, chunk_get_next_ncnl(brace_open));

            // Make sure nothing is cuddled with the closing brace
            pc = chunk_get_next_type(brace_open, CT_BRACE_CLOSE, brace_open->level);
            newline_add_between(pc, chunk_get_next_nblank(pc));
            retval = true;
         }
      }
   }
   return(retval);
} // newlines_if_for_while_switch


static void newlines_if_for_while_switch_pre_blank_lines(chunk_t *start, argval_t nl_opt)
{
   LOG_FUNC_ENTRY();

   if (  nl_opt == AV_IGNORE
      || (  (start->flags & PCF_IN_PREPROC)
         && !cpd.settings[UO_nl_define_macro].b))
   {
      return;
   }

   chunk_t *prev;
   chunk_t *next;
   chunk_t *last_nl = nullptr;
   size_t  level    = start->level;
   bool    do_add   = nl_opt & AV_ADD;

   /*
    * look backwards until we find
    *   open brace (don't add or remove)
    *   2 newlines in a row (don't add)
    *   something else (don't remove)
    */
   for (chunk_t *pc = chunk_get_prev(start); pc != nullptr; pc = chunk_get_prev(pc))
   {
      if (chunk_is_newline(pc))
      {
         last_nl = pc;
         // if we found 2 or more in a row
         if (pc->nl_count > 1 || chunk_is_newline(chunk_get_prev_nvb(pc)))
         {
            // need to remove
            if ((nl_opt & AV_REMOVE) && ((pc->flags & PCF_VAR_DEF) == 0))
            {
               // if we're also adding, take care of that here
               size_t nl_count = do_add ? 2 : 1;
               if (nl_count != pc->nl_count)
               {
                  pc->nl_count = nl_count;
                  MARK_CHANGE();
               }
               // can keep using pc because anything other than newline stops loop, and we delete if newline
               while (chunk_is_newline(prev = chunk_get_prev_nvb(pc)))
               {
                  // Make sure we don't combine a preproc and non-preproc
                  if (!chunk_safe_to_del_nl(prev))
                  {
                     break;
                  }
                  chunk_del(prev);
                  MARK_CHANGE();
               }
            }

            return;
         }
      }
      else if (chunk_is_opening_brace(pc) || pc->level < level)
      {
         return;
      }
      else if (chunk_is_comment(pc))
      {
         // vbrace close is ok because it won't go into output, so we should skip it
         last_nl = nullptr;
         continue;
      }
      else
      {
         if (do_add) // we found something previously besides a comment or a new line
         {
            // if we have run across a newline
            if (last_nl != nullptr)
            {
               if (last_nl->nl_count < 2)
               {
                  double_newline(last_nl);
               }
            }
            else
            {
               // we didn't run into a newline, so we need to add one
               if (  ((next = chunk_get_next(pc)) != nullptr)
                  && chunk_is_comment(next))
               {
                  pc = next;
               }
               if ((last_nl = newline_add_after(pc)) != nullptr)
               {
                  double_newline(last_nl);
               }
            }
         }

         return;
      }
   }
} // newlines_if_for_while_switch_pre_blank_lines


static void _blank_line_set(chunk_t *pc, const char *text, uncrustify_options uo)
{
   LOG_FUNC_ENTRY();
   if (pc == nullptr)
   {
      return;
   }
   const option_map_value *option = get_option_name(uo);
   if (option->type != AT_UNUM)
   {
      fprintf(stderr, "Program error for UO_=%d\n", static_cast<int>(uo));
      fprintf(stderr, "Please make a report\n");
      log_flush(true);
      exit(2);
   }

   if ((cpd.settings[uo].u > 0) && (pc->nl_count != cpd.settings[uo].u))
   {
      LOG_FMT(LBLANKD, "%s(%d): do_blank_lines: %s set line %zu to %zu\n",
              __func__, __LINE__, text + 3, pc->orig_line, cpd.settings[uo].u);
      pc->nl_count = cpd.settings[uo].u;
      MARK_CHANGE();
   }
}


#define blank_line_set(pc, op)    _blank_line_set(pc, #op, op)


static void newlines_func_pre_blank_lines(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   if (  start == nullptr
      || (  (  start->type != CT_FUNC_CLASS_DEF
            || cpd.settings[UO_nl_before_func_class_def].u == 0)
         && (  start->type != CT_FUNC_CLASS_PROTO
            || cpd.settings[UO_nl_before_func_class_proto].u == 0)
         && (  start->type != CT_FUNC_DEF
            || cpd.settings[UO_nl_before_func_body_def].u == 0)
         && (  start->type != CT_FUNC_PROTO
            || cpd.settings[UO_nl_before_func_body_proto].u == 0)))
   {
      return;
   }

   LOG_FMT(LNLFUNCT, "\n%s(%d): set blank line(s): for %s at line %zu\n",
           __func__, __LINE__, start->text(), start->orig_line);
   /*
    * look backwards until we find:
    *   - open brace (don't add or remove)
    *   - two newlines in a row (don't add)
    *   - a destructor
    *   - something else (don't remove)
    */
   chunk_t *pc           = nullptr;
   chunk_t *last_nl      = nullptr;
   chunk_t *last_comment = nullptr;
   bool    do_it         = false;
   for (pc = chunk_get_prev(start); pc != nullptr; pc = chunk_get_prev(pc))
   {
      LOG_FMT(LNLFUNCT, "   O%zu:%zu %s '%s'\n", pc->orig_line, pc->orig_col,
              get_token_name(pc->type), pc->text());

      if (chunk_is_newline(pc))
      {
         last_nl = pc;
         continue;
      }

      if (chunk_is_comment(pc))
      {
         if (  (  pc->orig_line < start->orig_line
               && ((start->orig_line - pc->orig_line
                    - (pc->type == CT_COMMENT_MULTI ? pc->nl_count : 0))) < 2)
            || (  last_comment != nullptr
               && pc->type == CT_COMMENT_CPP       // combine only cpp comments
               && last_comment->type == pc->type   // don't mix comment types
               && last_comment->orig_line > pc->orig_line
               && (last_comment->orig_line - pc->orig_line) < 2))
         {
            last_comment = pc;
            continue;
         }

         do_it = true;
         break;
      }

      if (  pc->type == CT_DESTRUCTOR
         || pc->type == CT_TYPE
         || pc->type == CT_QUALIFIER
         || pc->type == CT_PTR_TYPE
         || pc->type == CT_DC_MEMBER
         || pc->type == CT_TYPE
         || pc->type == CT_TYPE)
      {
         continue;
      }
      // skip template stuff to add newlines before it
      if (pc->type == CT_ANGLE_CLOSE && pc->parent_type == CT_TEMPLATE)
      {
         pc = chunk_get_prev_type(pc, CT_TEMPLATE, -1);
         continue;
      }

      // else
      do_it = true;
      break;
   }
   if (!do_it || last_nl == nullptr)
   {
      return;
   }

   LOG_FMT(LNLFUNCT, "   set blank line(s): for <NL> at O%zu:%zu\n",
           last_nl->orig_line, last_nl->orig_col);

   switch (start->type)
   {
   case CT_FUNC_CLASS_DEF:
   {
      if (cpd.settings[UO_nl_before_func_class_def].u != last_nl->nl_count)
      {
         LOG_FMT(LNLFUNCT, "   set blank line(s) to %zu\n",
                 cpd.settings[UO_nl_before_func_class_def].u);
         blank_line_set(last_nl, UO_nl_before_func_class_def);
      }
      break;
   }

   case CT_FUNC_CLASS_PROTO:
   {
      if (cpd.settings[UO_nl_before_func_class_proto].u != last_nl->nl_count)
      {
         LOG_FMT(LNLFUNCT, "   set blank line(s) to %zu\n",
                 cpd.settings[UO_nl_before_func_class_proto].u);
         blank_line_set(last_nl, UO_nl_before_func_class_proto);
      }
      break;
   }

   case CT_FUNC_DEF:
   {
      if (cpd.settings[UO_nl_before_func_body_def].u != last_nl->nl_count)
      {
         LOG_FMT(LNLFUNCT, "   set blank line(s) to %zu\n",
                 cpd.settings[UO_nl_before_func_body_def].u);
         blank_line_set(last_nl, UO_nl_before_func_body_def);
      }
      break;
   }

   case CT_FUNC_PROTO:
   {
      if (cpd.settings[UO_nl_before_func_body_proto].u != last_nl->nl_count)
      {
         LOG_FMT(LNLFUNCT, "   set blank line(s) to %zu\n",
                 cpd.settings[UO_nl_before_func_body_proto].u);
         blank_line_set(last_nl, UO_nl_before_func_body_proto);
      }
      break;
   }

   default:
   {
      LOG_FMT(LERR, "   setting to blank line(s) at line %zu not possible\n",
              pc->orig_line);
      break;
   }
   }   // switch
} // newlines_func_pre_blank_lines


static chunk_t *get_closing_brace(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;
   size_t  level = start->level;

   for (pc = start; (pc = chunk_get_next(pc)) != nullptr;)
   {
      if (  (pc->type == CT_BRACE_CLOSE || pc->type == CT_VBRACE_CLOSE)
         && pc->level == level)
      {
         return(pc);
      }
      // for some reason, we can have newlines between if and opening brace that are lower level than either
      if (!chunk_is_newline(pc) && pc->level < level)
      {
         return(nullptr);
      }
   }

   return(nullptr);
}


static void remove_next_newlines(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t *next;

   while ((next = chunk_get_next(start)) != nullptr)
   {
      if (chunk_is_newline(next) && chunk_safe_to_del_nl(next))
      {
         chunk_del(next);
         MARK_CHANGE();
      }
      else if (chunk_is_vbrace(next))
      {
         start = next;
      }
      else
      {
         break;
      }
   }
}


static void newlines_if_for_while_switch_post_blank_lines(chunk_t *start, argval_t nl_opt)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;
   chunk_t *next;
   chunk_t *prev;

   LOG_FMT(LNEWLINE, "%s:\n   (%d):start->..., type %s, line %zu, column %zu,\n",
           __func__, __LINE__, get_token_name(start->type), start->orig_line, start->orig_col);
   if (  nl_opt == AV_IGNORE
      || (  (start->flags & PCF_IN_PREPROC)
         && !cpd.settings[UO_nl_define_macro].b))
   {
      return;
   }

   // first find ending brace
   if ((pc = get_closing_brace(start)) == nullptr)
   {
      return;
   }
   LOG_FMT(LNEWLINE, "   (%d):pc->...   , type %s, line %zu, column %zu,\n",
           __LINE__, get_token_name(pc->type), pc->orig_line, pc->orig_col);

   /*
    * if we're dealing with an if, we actually want to add or remove
    * blank lines after any else
    */
   if (start->type == CT_IF)
   {
      while (true)
      {
         next = chunk_get_next_ncnl(pc);
         if (  next != nullptr
            && (next->type == CT_ELSE || next->type == CT_ELSEIF))
         {
            // point to the closing brace of the else
            if ((pc = get_closing_brace(next)) == nullptr)
            {
               return;
            }
            LOG_FMT(LNEWLINE, "   (%d):pc->...   , type %s, line %zu, column %zu,\n",
                    __LINE__, get_token_name(pc->type), pc->orig_line, pc->orig_col);
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
   if (start->type == CT_DO)
   {
      // point to the next semicolon
      if ((pc = chunk_get_next_type(pc, CT_SEMICOLON, start->level)) == nullptr)
      {
         return;
      }
      LOG_FMT(LNEWLINE, "   (%d):pc->...   , type %s, line %zu, column %zu,\n",
              __LINE__, get_token_name(pc->type), pc->orig_line, pc->orig_col);
   }

   bool isVBrace = (pc->type == CT_VBRACE_CLOSE);
   if (isVBrace)
   {
      LOG_FMT(LNEWLINE, "   (%d): isVBrace is TRUE\n", __LINE__);
   }
   else
   {
      LOG_FMT(LNEWLINE, "   (%d): isVBrace is FALSE\n", __LINE__);
   }

   if ((prev = chunk_get_prev_nvb(pc)) == nullptr)
   {
      return;
   }

   bool have_pre_vbrace_nl = isVBrace && chunk_is_newline(prev);
   if (have_pre_vbrace_nl)
   {
      LOG_FMT(LNEWLINE, "   (%d): have_pre_vbrace_nl is TRUE\n", __LINE__);
   }
   else
   {
      LOG_FMT(LNEWLINE, "   (%d): have_pre_vbrace_nl is FALSE\n", __LINE__);
   }
   if (nl_opt & AV_REMOVE)
   {
      // if chunk before is a vbrace, remove any newlines after it
      if (have_pre_vbrace_nl)
      {
         if (prev->nl_count != 1)
         {
            prev->nl_count = 1;
            MARK_CHANGE();
         }
         remove_next_newlines(pc);
      }
      else if (  (chunk_is_newline(next = chunk_get_next_nvb(pc)))
              && !(next->flags & PCF_VAR_DEF))
      {
         // otherwise just deal with newlines after brace
         if (next->nl_count != 1)
         {
            next->nl_count = 1;
            MARK_CHANGE();
         }
         remove_next_newlines(next);
      }
   }

   // may have a newline before and after vbrace
   // don't do anything with it if the next non newline chunk is a closing brace
   if (nl_opt & AV_ADD)
   {
      chunk_t *nextNNL = chunk_get_next_nnl(pc);
      do
      {
         if (nextNNL == nullptr)
         {
            return;
         }
         if (nextNNL->type != CT_VBRACE_CLOSE)
         {
            next = nextNNL;
            break;
         }
         nextNNL = chunk_get_next_nnl(nextNNL);
      } while (true);

      LOG_FMT(LNEWLINE, "   (%d): next->... , type %s, line %zu, column %zu,\n",
              __LINE__, get_token_name(next->type), next->orig_line, next->orig_col);
      if (next->type != CT_BRACE_CLOSE)
      {
         // if vbrace, have to check before and after
         // if chunk before vbrace, check its count
         size_t nl_count = have_pre_vbrace_nl ? prev->nl_count : 0;
         LOG_FMT(LNEWLINE, "   (%d): nl_count %zu\n", __LINE__, nl_count);
         if (chunk_is_newline(next = chunk_get_next_nvb(pc)))
         {
            LOG_FMT(LNEWLINE, "   (%d): next->... , type %s, line %zu, column %zu,\n",
                    __LINE__, get_token_name(next->type), next->orig_line, next->orig_col);
            nl_count += next->nl_count;
            LOG_FMT(LNEWLINE, "   (%d): nl_count is %zu\n", __LINE__, nl_count);
         }

         // if we have no newlines, add one and make it double
         if (nl_count == 0)
         {
            LOG_FMT(LNEWLINE, "   (%d): nl_count is 0\n", __LINE__);
            if (  ((next = chunk_get_next(pc)) != nullptr)
               && chunk_is_comment(next))
            {
               LOG_FMT(LNEWLINE, "   (%d): next->... , type %s, line %zu, column %zu,\n",
                       __LINE__, get_token_name(next->type), next->orig_line, next->orig_col);
               pc = next;
               LOG_FMT(LNEWLINE, "   (%d): pc->...   , type %s, line %zu, column %zu,\n",
                       __LINE__, get_token_name(pc->type), pc->orig_line, pc->orig_col);
            }

            if ((next = newline_add_after(pc)) == nullptr)
            {
               return;
            }
            LOG_FMT(LNEWLINE, "   (%d): next->... , type %s, line %zu, column %zu,\n",
                    __LINE__, get_token_name(next->type), next->orig_line, next->orig_col);
            double_newline(next);
         }
         else if (nl_count == 1) // if we don't have enough newlines
         {
            LOG_FMT(LNEWLINE, "   (%d): nl_count is 1\n", __LINE__);
            // if we have a preceeding vbrace, add one after it
            if (have_pre_vbrace_nl)
            {
               LOG_FMT(LNEWLINE, "   (%d): have_pre_vbrace_nl is TRUE\n", __LINE__);
               next = newline_add_after(pc);
               LOG_FMT(LNEWLINE, "   (%d): next->... , type %s, line %zu, column %zu,\n",
                       __LINE__, get_token_name(next->type), next->orig_line, next->orig_col);
            }
            else
            {
               LOG_FMT(LNEWLINE, "   (%d): have_pre_vbrace_nl is FALSE\n", __LINE__);
               prev = chunk_get_prev_nnl(next);
               LOG_FMT(LNEWLINE, "   (%d): prev->... , type %s, line %zu, column %zu,\n",
                       __LINE__, get_token_name(prev->type), prev->orig_line, prev->orig_col);
               pc = chunk_get_next_nl(next);
               LOG_FMT(LNEWLINE, "   (%d): pc->...   , type %s, line %zu, column %zu,\n",
                       __LINE__, get_token_name(pc->type), pc->orig_line, pc->orig_col);
               chunk_t *pc2 = chunk_get_next(pc);
               if (pc2 != nullptr)
               {
                  pc = pc2;
                  LOG_FMT(LNEWLINE, "   (%d): pc->...   , type %s, line %zu, column %zu,\n",
                          __LINE__, get_token_name(pc->type), pc->orig_line, pc->orig_col);
               }
               else
               {
                  LOG_FMT(LNEWLINE, "   (%d): no next found: <EOF>\n", __LINE__);
               }
               if (  chunk_is_token(pc, CT_PREPROC)
                  && pc->parent_type == CT_PP_ENDIF
                  && cpd.settings[UO_nl_squeeze_ifdef].b)
               {
                  LOG_FMT(LNEWLINE, "%s(%d): cannot add newline after line %zu due to nl_squeeze_ifdef\n",
                          __func__, __LINE__, prev->orig_line);
               }
               else
               {
                  // make newline after double
                  LOG_FMT(LNEWLINE, "   (%d): call double_newline\n", __LINE__);
                  double_newline(next);
               }
            }
         }
      }
   }
} // newlines_if_for_while_switch_post_blank_lines


static void newlines_struct_union(chunk_t *start, argval_t nl_opt, bool leave_trailing)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;

   if (  nl_opt == AV_IGNORE
      || (  (start->flags & PCF_IN_PREPROC)
         && !cpd.settings[UO_nl_define_macro].b))
   {
      return;
   }

   /*
    * step past any junk between the keyword and the open brace
    * Quit if we hit a semicolon or '=', which are not expected.
    */
   size_t level = start->level;
   pc = start;
   while (((pc = chunk_get_next_ncnl(pc)) != nullptr) && pc->level >= level)
   {
      if (  pc->level == level
         && (  pc->type == CT_BRACE_OPEN
            || chunk_is_semicolon(pc)
            || pc->type == CT_ASSIGN))
      {
         break;
      }
      start = pc;
   }

   // If we hit a brace open, then we need to toy with the newlines
   if (chunk_is_token(pc, CT_BRACE_OPEN))
   {
      // Skip over embedded C comments
      chunk_t *next = chunk_get_next(pc);
      while (chunk_is_token(next, CT_COMMENT))
      {
         next = chunk_get_next(next);
      }
      if (  leave_trailing
         && !chunk_is_comment(next)
         && !chunk_is_newline(next))
      {
         nl_opt = AV_IGNORE;
      }

      newline_iarf_pair(start, pc, nl_opt);
   }
} // newlines_struct_union


// enum {
// enum class angle_state_e : unsigned int {
// enum-key attr(optional) identifier(optional) enum-base(optional) { enumerator-list(optional) }
// enum-key attr(optional) nested-name-specifier(optional) identifier enum-base(optional) ; TODO
// enum-key         - one of enum, enum class or enum struct  TODO
// identifier       - the name of the enumeration that's being declared
// enum-base(C++11) - colon (:), followed by a type-specifier-seq


// enumerator-list  - comma-separated list of enumerator definitions
static void newlines_enum(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t  *pc;
   chunk_t  *pcClass;
   chunk_t  *pcType;
   chunk_t  *pcColon;
   chunk_t  *pcType1;
   chunk_t  *pcType2;
   argval_t nl_opt;

   if ((start->flags & PCF_IN_PREPROC) && !cpd.settings[UO_nl_define_macro].b)
   {
      return;
   }

   // look for 'enum class'
   pcClass = chunk_get_next_ncnl(start);
   if (chunk_is_token(pcClass, CT_ENUM_CLASS))
   {
      newline_iarf_pair(start, pcClass, cpd.settings[UO_nl_enum_class].a);
      // look for 'identifier'/ 'type'
      pcType = chunk_get_next_ncnl(pcClass);
      if (chunk_is_token(pcType, CT_TYPE))
      {
         newline_iarf_pair(pcClass, pcType, cpd.settings[UO_nl_enum_class_identifier].a);
         // look for ':'
         pcColon = chunk_get_next_ncnl(pcType);
         if (chunk_is_token(pcColon, CT_BIT_COLON))
         {
            newline_iarf_pair(pcType, pcColon, cpd.settings[UO_nl_enum_identifier_colon].a);
            // look for 'type' i.e. unsigned
            pcType1 = chunk_get_next_ncnl(pcColon);
            if (chunk_is_token(pcType1, CT_TYPE))
            {
               newline_iarf_pair(pcColon, pcType1, cpd.settings[UO_nl_enum_colon_type].a);
               // look for 'type' i.e. int
               pcType2 = chunk_get_next_ncnl(pcType1);
               if (chunk_is_token(pcType2, CT_TYPE))
               {
                  newline_iarf_pair(pcType1, pcType2, cpd.settings[UO_nl_enum_colon_type].a);
               }
            }
         }
      }
   }

   /*
    * step past any junk between the keyword and the open brace
    * Quit if we hit a semicolon or '=', which are not expected.
    */
   size_t level = start->level;
   pc = start;
   while (((pc = chunk_get_next_ncnl(pc)) != nullptr) && pc->level >= level)
   {
      if (  pc->level == level
         && (  pc->type == CT_BRACE_OPEN
            || chunk_is_semicolon(pc)
            || pc->type == CT_ASSIGN))
      {
         break;
      }
      start = pc;
   }

   // If we hit a brace open, then we need to toy with the newlines
   if (chunk_is_token(pc, CT_BRACE_OPEN))
   {
      // Skip over embedded C comments
      chunk_t *next = chunk_get_next(pc);
      while (chunk_is_token(next, CT_COMMENT))
      {
         next = chunk_get_next(next);
      }
      if (!chunk_is_comment(next) && !chunk_is_newline(next))
      {
         nl_opt = AV_IGNORE;
      }
      else
      {
         nl_opt = cpd.settings[UO_nl_enum_brace].a;
      }

      newline_iarf_pair(start, pc, nl_opt);
   }
} // newlines_enum


static void newlines_cuddle_uncuddle(chunk_t *start, argval_t nl_opt)
{
   LOG_FUNC_ENTRY();
   chunk_t *br_close;

   if ((start->flags & PCF_IN_PREPROC) && !cpd.settings[UO_nl_define_macro].b)
   {
      return;
   }

   br_close = chunk_get_prev_ncnl(start);
   if (chunk_is_token(br_close, CT_BRACE_CLOSE))
   {
      newline_iarf_pair(br_close, start, nl_opt);
   }
}


static void newlines_do_else(chunk_t *start, argval_t nl_opt)
{
   LOG_FUNC_ENTRY();
   chunk_t *next;

   if (  nl_opt == AV_IGNORE
      || (  (start->flags & PCF_IN_PREPROC)
         && !cpd.settings[UO_nl_define_macro].b))
   {
      return;
   }

   next = chunk_get_next_ncnl(start);
   if (  next != nullptr
      && (next->type == CT_BRACE_OPEN || next->type == CT_VBRACE_OPEN))
   {
      if (!one_liner_nl_ok(next))
      {
         LOG_FMT(LNL1LINE, "%s(%d): a new line may NOT be added\n", __func__, __LINE__);
         return;
      }

      LOG_FMT(LNL1LINE, "%s(%d): a new line may be added\n", __func__, __LINE__);

      if (next->type == CT_VBRACE_OPEN)
      {
         // Can only add - we don't want to create a one-line here
         if (nl_opt & AV_ADD)
         {
            newline_iarf_pair(start, chunk_get_next_ncnl(next), nl_opt);
            chunk_t *tmp = chunk_get_next_type(next, CT_VBRACE_CLOSE, next->level);
            if (  !chunk_is_newline(chunk_get_next_nc(tmp))
               && !chunk_is_newline(chunk_get_prev_nc(tmp)))
            {
               newline_add_after(tmp);
            }
         }
      }
      else
      {
         newline_iarf_pair(start, next, nl_opt);

         newline_add_between(next, chunk_get_next_ncnl(next));
      }
   }
} // newlines_do_else


static chunk_t *newline_def_blk(chunk_t *start, bool fn_top)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;
   bool    did_this_line = false;
   bool    first_var_blk = true;
   bool    typedef_blk   = false;
   bool    var_blk       = false;

   chunk_t *prev = chunk_get_prev_ncnl(start);
   // can't be any variable definitions in a "= {" block
   if (chunk_is_token(prev, CT_ASSIGN))
   {
      pc = chunk_get_next_type(start, CT_BRACE_CLOSE, start->level);
      return(chunk_get_next_ncnl(pc));
   }
   pc = chunk_get_next(start);
   while (  pc != nullptr
         && (pc->level >= start->level || pc->level == 0))
   {
      if (chunk_is_comment(pc))
      {
         pc = chunk_get_next(pc);
         continue;
      }

      // process nested braces
      if (pc->type == CT_BRACE_OPEN)
      {
         pc = newline_def_blk(pc, false);
         continue;
      }

      // Done with this brace set?
      if (pc->type == CT_BRACE_CLOSE)
      {
         pc = chunk_get_next(pc);
         break;
      }

      // skip vbraces
      if (pc->type == CT_VBRACE_OPEN)
      {
         pc = chunk_get_next_type(pc, CT_VBRACE_CLOSE, pc->level);
         if (pc != nullptr)
         {
            pc = chunk_get_next(pc);
         }
         continue;
      }

      // Ignore stuff inside parenthesis/squares/angles
      if (pc->level > pc->brace_level)
      {
         pc = chunk_get_next(pc);
         continue;
      }

      if (chunk_is_newline(pc))
      {
         did_this_line = false;
         pc            = chunk_get_next(pc);
         continue;
      }

      // Determine if this is a variable def or code
      if (  !did_this_line
         && pc->type != CT_FUNC_CLASS_DEF
         && pc->type != CT_FUNC_CLASS_PROTO
         && ((pc->level == (start->level + 1)) || pc->level == 0))
      {
         chunk_t *next = chunk_get_next_ncnl(pc);
         if (next == nullptr)
         {
            break;
         }
         prev = chunk_get_prev_ncnl(pc);
         if (pc->type == CT_TYPEDEF)
         {
            // set newlines before typedef block
            if (  !typedef_blk
               && prev != nullptr
               && (cpd.settings[UO_nl_typedef_blk_start].u > 0))
            {
               newline_min_after(prev, cpd.settings[UO_nl_typedef_blk_start].u, PCF_VAR_DEF);
            }
            // set newlines within typedef block
            else if (  typedef_blk
                    && (cpd.settings[UO_nl_typedef_blk_in].u > 0))
            {
               prev = chunk_get_prev(pc);
               if (chunk_is_newline(prev))
               {
                  if (prev->nl_count > cpd.settings[UO_nl_typedef_blk_in].u)
                  {
                     prev->nl_count = cpd.settings[UO_nl_typedef_blk_in].u;
                     MARK_CHANGE();
                  }
               }
            }
            // set blank lines after first var def block
            if (  var_blk
               && first_var_blk
               && fn_top
               && (cpd.settings[UO_nl_func_var_def_blk].u > 0))
            {
               newline_min_after(prev, 1 + cpd.settings[UO_nl_func_var_def_blk].u, PCF_VAR_DEF);
            }
            // set newlines after var def block
            else if (  var_blk
                    && (cpd.settings[UO_nl_var_def_blk_end].u > 0))
            {
               newline_min_after(prev, cpd.settings[UO_nl_var_def_blk_end].u, PCF_VAR_DEF);
            }
            pc            = chunk_get_next_type(pc, CT_SEMICOLON, pc->level);
            typedef_blk   = true;
            first_var_blk = false;
            var_blk       = false;
         }
         else if (  chunk_is_type(pc)
                 && ((  chunk_is_type(next)
                     || next->type == CT_WORD
                     || next->type == CT_FUNC_CTOR_VAR))
                 && !(next->type == CT_DC_MEMBER))  // DbConfig::configuredDatabase()->apply(db);
                                                    // is NOT a declaration of a variable
                                                    // guy 2015-09-22
         {
            // set newlines before var def block
            if (  !var_blk
               && !first_var_blk
               && (cpd.settings[UO_nl_var_def_blk_start].u > 0))
            {
               newline_min_after(prev, cpd.settings[UO_nl_var_def_blk_start].u, PCF_VAR_DEF);
            }
            // set newlines within var def block
            else if (var_blk && (cpd.settings[UO_nl_var_def_blk_in].u > 0))
            {
               prev = chunk_get_prev(pc);
               if (chunk_is_newline(prev))
               {
                  if (prev->nl_count > cpd.settings[UO_nl_var_def_blk_in].u)
                  {
                     prev->nl_count = cpd.settings[UO_nl_var_def_blk_in].u;
                     MARK_CHANGE();
                  }
               }
            }
            // set newlines after typedef block
            else if (  typedef_blk
                    && (cpd.settings[UO_nl_typedef_blk_end].u > 0))
            {
               newline_min_after(prev, cpd.settings[UO_nl_typedef_blk_end].u, PCF_VAR_DEF);
            }
            pc          = chunk_get_next_type(pc, CT_SEMICOLON, pc->level);
            typedef_blk = false;
            var_blk     = true;
         }
         else
         {
            // set newlines after typedef block
            if (typedef_blk && (cpd.settings[UO_nl_var_def_blk_end].u > 0))
            {
               newline_min_after(prev, cpd.settings[UO_nl_var_def_blk_end].u, PCF_VAR_DEF);
            }
            // set blank lines after first var def block
            if (  var_blk
               && first_var_blk
               && fn_top
               && (cpd.settings[UO_nl_func_var_def_blk].u > 0))
            {
               newline_min_after(prev, 1 + cpd.settings[UO_nl_func_var_def_blk].u, PCF_VAR_DEF);
            }
            // set newlines after var def block
            else if (var_blk && (cpd.settings[UO_nl_var_def_blk_end].u > 0))
            {
               newline_min_after(prev, cpd.settings[UO_nl_var_def_blk_end].u, PCF_VAR_DEF);
            }
            typedef_blk   = false;
            first_var_blk = false;
            var_blk       = false;
         }
      }
      did_this_line = true;
      pc            = chunk_get_next(pc);
   }

   return(pc);
} // newline_def_blk


static void newlines_brace_pair(chunk_t *br_open)
{
   LOG_FUNC_ENTRY();

   if ((br_open->flags & PCF_IN_PREPROC) && !cpd.settings[UO_nl_define_macro].b)
   {
      return;
   }

   chunk_t *next;
   chunk_t *pc;

   if (cpd.settings[UO_nl_collapse_empty_body].b)
   {
      next = chunk_get_next_nnl(br_open);
      if (chunk_is_token(next, CT_BRACE_CLOSE))
      {
         pc = chunk_get_next(br_open);

         while (pc != nullptr && pc->type != CT_BRACE_CLOSE)
         {
            next = chunk_get_next(pc);
            if (pc->type == CT_NEWLINE)
            {
               if (chunk_safe_to_del_nl(pc))
               {
                  chunk_del(pc);
                  MARK_CHANGE();
               }
            }
            pc = next;
         }
         return;
      }
   }

   //fixes 1235 Add single line namespace support

   if (  br_open->type == CT_BRACE_OPEN
      && (br_open->parent_type == CT_NAMESPACE)
      && chunk_is_newline(chunk_get_prev(br_open)))
   {
      chunk_t *chunk_brace_close = chunk_skip_to_match(br_open, scope_e::ALL);
      if (chunk_brace_close != nullptr)
      {
         if (are_chunks_in_same_line(br_open, chunk_brace_close))
         {
            if (cpd.settings[UO_nl_namespace_two_to_one_liner].b)
            {
               chunk_t *prev = chunk_get_prev_nnl(br_open);
               newline_del_between(prev, br_open);
            }
            /* Below code is to support conversion of 2 liner to 4 liners
             * else
             * {
             *  chunk_t *nxt = chunk_get_next(br_open);
             *  newline_add_between(br_open, nxt);
             * }*/
         }
      }
   }

   // fix 1247 oneliner function support - converts 4,3,2  liners to oneliner

   if (  br_open->parent_type == CT_FUNC_DEF
      && cpd.settings[UO_nl_create_func_def_one_liner].b)
   {
      chunk_t *br_close = chunk_skip_to_match(br_open, scope_e::ALL);

      chunk_t *tmp = chunk_get_prev_ncnl(br_open);

      if (((br_close->orig_line - br_open->orig_line) <= 2) && chunk_is_paren_close(tmp))
      {
         while (  tmp != nullptr
               && (tmp = chunk_get_next(tmp)) != nullptr
               && !chunk_is_closing_brace(tmp)
               && (chunk_get_next(tmp) != nullptr))
         {
            if (chunk_is_newline(tmp))
            {
               tmp = chunk_get_prev(tmp);
               newline_iarf_pair(tmp, chunk_get_next_ncnl(tmp), AV_REMOVE);
            }

            chunk_flags_set(br_open, PCF_ONE_LINER);
            chunk_flags_set(br_close, PCF_ONE_LINER);
         }
      }
   }


   // Make sure we don't break a one-liner

   if (!one_liner_nl_ok(br_open))
   {
      LOG_FMT(LNL1LINE, "%s(%d): br_open->orig_line is %zu, br_open->orig_col is %zu, a new line may NOT be added\n",
              __func__, __LINE__, br_open->orig_line, br_open->orig_col);
      return;
   }

   LOG_FMT(LNL1LINE, "%s(%d): a new line may be added\n", __func__, __LINE__);

   next = chunk_get_next_nc(br_open);
   chunk_t *prev;
   // Insert a newline between the '=' and open brace, if needed
   LOG_FMT(LNL1LINE, "%s(%d): br_open->text() '%s', br_open->type [%s], br_open->parent_type [%s]\n",
           __func__, __LINE__, br_open->text(), get_token_name(br_open->type), get_token_name(br_open->parent_type));
   if (br_open->parent_type == CT_ASSIGN)
   {
      // Only mess with it if the open brace is followed by a newline
      if (chunk_is_newline(next))
      {
         prev = chunk_get_prev_ncnl(br_open);

         newline_iarf_pair(prev, br_open, cpd.settings[UO_nl_assign_brace].a);
      }
   }

   //fixes #1245 will add new line between tsquare and brace open based on UO_nl_tsquare_brace

   if (br_open->type == CT_BRACE_OPEN)
   {
      chunk_t *chunk_closeing_brace = chunk_skip_to_match(br_open, scope_e::ALL);
      if (chunk_closeing_brace != nullptr)
      {
         if (chunk_closeing_brace->orig_line > br_open->orig_line)
         {
            prev = chunk_get_prev_nc(br_open);
            if (  prev != nullptr
               && prev->type == CT_TSQUARE
               && chunk_is_newline(next))
            {
               newline_iarf_pair(prev, br_open, cpd.settings[UO_nl_tsquare_brace].a);
            }
         }
      }
   }

   // Eat any extra newlines after the brace open
   if (cpd.settings[UO_eat_blanks_after_open_brace].b)
   {
      if (chunk_is_newline(next))
      {
         if (next->nl_count > 1)
         {
            next->nl_count = 1;
            LOG_FMT(LBLANKD, "%s(%d): eat_blanks_after_open_brace %zu\n",
                    __func__, __LINE__, next->orig_line);
            MARK_CHANGE();
         }
      }
   }

   argval_t val            = AV_IGNORE;
   bool     nl_close_brace = false;
   // Handle the cases where the brace is part of a function call or definition
   if (  br_open->parent_type == CT_FUNC_DEF
      || br_open->parent_type == CT_FUNC_CALL
      || br_open->parent_type == CT_FUNC_CALL_USER
      || br_open->parent_type == CT_FUNC_CLASS_DEF
      || br_open->parent_type == CT_OC_CLASS
      || br_open->parent_type == CT_OC_MSG_DECL
      || br_open->parent_type == CT_CS_PROPERTY
      || br_open->parent_type == CT_CPP_LAMBDA)
   {
      // Need to force a newline before the close brace, if not in a class body
      if ((br_open->flags & PCF_IN_CLASS) == 0)
      {
         nl_close_brace = true;
      }

      // handle newlines after the open brace
      pc = chunk_get_next_ncnl(br_open);
      newline_add_between(br_open, pc);

      val = ((  br_open->parent_type == CT_FUNC_DEF
             || br_open->parent_type == CT_FUNC_CLASS_DEF
             || br_open->parent_type == CT_OC_CLASS
             || br_open->parent_type == CT_OC_MSG_DECL) ?
             cpd.settings[UO_nl_fdef_brace].a :
             ((br_open->parent_type == CT_CS_PROPERTY) ?
              cpd.settings[UO_nl_property_brace].a :
              ((br_open->parent_type == CT_CPP_LAMBDA) ?
               cpd.settings[UO_nl_cpp_ldef_brace].a :
               cpd.settings[UO_nl_fcall_brace].a)));

      if (val != AV_IGNORE)
      {
         // Grab the chunk before the open brace
         prev = chunk_get_prev_ncnl(br_open);

         newline_iarf_pair(prev, br_open, val);
      }

      newline_def_blk(br_open, true);
   }

   // Handle the cases where the brace is part of a class or struct
   if (br_open->parent_type == CT_CLASS || br_open->parent_type == CT_STRUCT)
   {
      newline_def_blk(br_open, false);
   }

   // Grab the matching brace close
   chunk_t *br_close;
   br_close = chunk_get_next_type(br_open, CT_BRACE_CLOSE, br_open->level);
   if (br_close == nullptr)
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
      pc = chunk_get_next(br_open);
      while (chunk_is_token(pc, CT_COMMENT))
      {
         pc = chunk_get_next(pc);
      }
      if (chunk_is_newline(pc) || chunk_is_comment(pc))
      {
         nl_close_brace = true;
      }
   }

   prev = chunk_get_prev_nblank(br_close);
   if (nl_close_brace)
   {
      newline_add_between(prev, br_close);
   }
   else
   {
      newline_del_between(prev, br_close);
   }
} // newlines_brace_pair


static void newline_case(chunk_t *start)
{
   LOG_FUNC_ENTRY();

   //   printf("%s case (%s) on line %d col %d\n",
   //          __func__, c_chunk_names[start->type],
   //          start->orig_line, start->orig_col);

   // Scan backwards until a '{' or ';' or ':'. Abort if a multi-newline is found
   chunk_t *prev = start;
   do
   {
      prev = chunk_get_prev_nc(prev);
      if (  prev != nullptr
         && chunk_is_newline(prev)
         && prev->nl_count > 1)
      {
         return;
      }
   } while (  prev != nullptr
           && prev->type != CT_BRACE_OPEN
           && prev->type != CT_BRACE_CLOSE
           && prev->type != CT_SEMICOLON
           && prev->type != CT_CASE_COLON);

   if (prev == nullptr)
   {
      return;
   }

   chunk_t *pc = newline_add_between(prev, start);
   if (pc == nullptr)
   {
      return;
   }

   // Only add an extra line after a semicolon or brace close
   if (prev->type == CT_SEMICOLON || prev->type == CT_BRACE_CLOSE)
   {
      if (chunk_is_newline(pc) && pc->nl_count < 2)
      {
         double_newline(pc);
      }
   }
} // newline_case


static void newline_case_colon(chunk_t *start)
{
   LOG_FUNC_ENTRY();

   // Scan forwards until a non-comment is found
   chunk_t *pc = start;
   do
   {
      pc = chunk_get_next(pc);
   } while (chunk_is_comment(pc));

   if (pc != nullptr && !chunk_is_newline(pc))
   {
      newline_add_before(pc);
   }
}


static void newline_before_return(chunk_t *start)
{
   LOG_FUNC_ENTRY();

   chunk_t *nl = chunk_get_prev(start);
   if (!chunk_is_newline(nl))
   {
      // Don't mess with lines that don't start with 'return'
      return;
   }

   // Do we already have a blank line?
   if (nl->nl_count > 1)
   {
      return;
   }

   chunk_t *pc = chunk_get_prev(nl);
   if (  pc == nullptr
      || (  pc->type == CT_BRACE_OPEN
         || pc->type == CT_VBRACE_OPEN
         || start->parent_type == CT_CASE)) // Issue #1257
   {
      return;
   }
   if (chunk_is_comment(pc))
   {
      pc = chunk_get_prev(pc);
      if (!chunk_is_newline(pc))
      {
         return;
      }
      nl = pc;
   }
   if (nl->nl_count < 2)
   {
      nl->nl_count++;
      MARK_CHANGE();
   }
}


static void newline_after_return(chunk_t *start)
{
   LOG_FUNC_ENTRY();

   chunk_t *semi  = chunk_get_next_type(start, CT_SEMICOLON, start->level);
   chunk_t *after = chunk_get_next_nblank(semi);

   // If we hit a brace or an 'else', then a newline isn't needed
   if (  after == nullptr
      || after->type == CT_BRACE_CLOSE
      || after->type == CT_VBRACE_CLOSE
      || after->type == CT_ELSE)
   {
      return;
   }

   chunk_t *pc;
   for (pc = chunk_get_next(semi); pc != after; pc = chunk_get_next(pc))
   {
      if (pc->type == CT_NEWLINE)
      {
         if (pc->nl_count < 2)
         {
            double_newline(pc);
         }
         return;
      }
   }
}


static void newline_iarf_pair(chunk_t *before, chunk_t *after, argval_t av)
{
   LOG_FUNC_ENTRY();
   log_func_stack(LNEWLINE, "Call Stack:");

   if (before != nullptr && after != nullptr)
   {
      if ((av & AV_ADD) != 0)
      {
         chunk_t *nl = newline_add_between(before, after);
         if (  nl
            && av == AV_FORCE
            && nl->nl_count > 1)
         {
            nl->nl_count = 1;
         }
      }
      else if ((av & AV_REMOVE) != 0)
      {
         newline_del_between(before, after);
      }
   }
}


void newline_iarf(chunk_t *pc, argval_t av)
{
   LOG_FUNC_ENTRY();
   log_func_stack(LNEWLINE, "CallStack:");

   newline_iarf_pair(pc, chunk_get_next_nnl(pc), av);
}


static void newline_func_multi_line(chunk_t *start)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNFD, "%s(%d): called on %zu:%zu '%s' [%s/%s]\n",
           __func__, __LINE__, start->orig_line, start->orig_col,
           start->text(), get_token_name(start->type), get_token_name(start->parent_type));

   bool add_start;
   bool add_args;
   bool add_end;

   if (  start->parent_type == CT_FUNC_DEF
      || start->parent_type == CT_FUNC_CLASS_DEF)
   {
      add_start = cpd.settings[UO_nl_func_def_start_multi_line].b;
      add_args  = cpd.settings[UO_nl_func_def_args_multi_line].b;
      add_end   = cpd.settings[UO_nl_func_def_end_multi_line].b;
   }
   else if (  start->parent_type == CT_FUNC_CALL
           || start->parent_type == CT_FUNC_CALL_USER)
   {
      add_start = cpd.settings[UO_nl_func_call_start_multi_line].b;
      add_args  = cpd.settings[UO_nl_func_call_args_multi_line].b;
      add_end   = cpd.settings[UO_nl_func_call_end_multi_line].b;
   }
   else
   {
      add_start = cpd.settings[UO_nl_func_decl_start_multi_line].b;
      add_args  = cpd.settings[UO_nl_func_decl_args_multi_line].b;
      add_end   = cpd.settings[UO_nl_func_decl_end_multi_line].b;
   }

   if (  !add_start
      && !add_args
      && !add_end)
   {
      return;
   }

   chunk_t *pc = chunk_get_next_ncnl(start);
   while (pc != nullptr && pc->level > start->level)
   {
      pc = chunk_get_next_ncnl(pc);
   }

   if (  chunk_is_token(pc, CT_FPAREN_CLOSE)
      && chunk_is_newline_between(start, pc))
   {
      if (add_start && !chunk_is_newline(chunk_get_next(start)))
      {
         newline_iarf(start, AV_ADD);
      }

      if (add_end && !chunk_is_newline(chunk_get_prev(pc)))
      {
         newline_iarf(chunk_get_prev(pc), AV_ADD);
      }

      if (add_args)
      {
         for (pc = chunk_get_next_ncnl(start);
              pc != nullptr && pc->level > start->level;
              pc = chunk_get_next_ncnl(pc))
         {
            if (pc->type == CT_COMMA && (pc->level == (start->level + 1)))
            {
               chunk_t *tmp = chunk_get_next(pc);
               if (chunk_is_comment(tmp))
               {
                  pc = tmp;
               }

               if (!chunk_is_newline(chunk_get_next(pc)))
               {
                  newline_iarf(pc, AV_ADD);
               }
            }
         }
      }
   }
} // newline_func_multi_line


static void newline_func_def_or_call(chunk_t *start)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNFD, "%s(%d): called on %zu:%zu '%s' [%s/%s]\n",
           __func__, __LINE__, start->orig_line, start->orig_col,
           start->text(), get_token_name(start->type), get_token_name(start->parent_type));

   chunk_t *prev  = nullptr;
   bool    is_def = (start->parent_type == CT_FUNC_DEF)
                    || start->parent_type == CT_FUNC_CLASS_DEF;
   bool    is_call = (start->parent_type == CT_FUNC_CALL)
                     || start->parent_type == CT_FUNC_CALL_USER;

   if (is_call)
   {
      argval_t atmp = cpd.settings[UO_nl_func_call_paren].a;
      if (atmp != AV_IGNORE)
      {
         prev = chunk_get_prev_ncnl(start);
         if (prev != nullptr)
         {
            newline_iarf(prev, atmp);
         }
      }

      chunk_t *pc = chunk_get_next_ncnl(start);
      if (chunk_is_str(pc, ")", 1))
      {
         atmp = cpd.settings[UO_nl_func_call_paren_empty].a;
         if (atmp != AV_IGNORE)
         {
            prev = chunk_get_prev_ncnl(start);
            if (prev != nullptr)
            {
               newline_iarf(prev, atmp);
            }
         }

         atmp = cpd.settings[UO_nl_func_call_empty].a;
         if (atmp != AV_IGNORE)
         {
            newline_iarf(start, atmp);
         }
         return;
      }
   }
   else
   {
      argval_t atmp = cpd.settings[is_def ? UO_nl_func_def_paren : UO_nl_func_paren].a;
      if (atmp != AV_IGNORE)
      {
         prev = chunk_get_prev_ncnl(start);
         if (prev != nullptr)
         {
            newline_iarf(prev, atmp);
         }
      }

      // Handle break newlines type and function
      prev = chunk_get_prev_ncnl(start);
      prev = skip_template_prev(prev);
      // Don't split up a function variable
      prev = chunk_is_paren_close(prev) ? nullptr : chunk_get_prev_ncnl(prev);

      if (  chunk_is_token(prev, CT_DC_MEMBER)
         && (cpd.settings[UO_nl_func_class_scope].a != AV_IGNORE))
      {
         newline_iarf(chunk_get_prev_ncnl(prev), cpd.settings[UO_nl_func_class_scope].a);
      }

      if (prev != nullptr && prev->type != CT_PRIVATE_COLON)
      {
         chunk_t *tmp;
         if (prev->type == CT_OPERATOR)
         {
            tmp  = prev;
            prev = chunk_get_prev_ncnl(prev);
         }
         else
         {
            tmp = start;
         }

         if (chunk_is_token(prev, CT_DC_MEMBER))
         {
            if (cpd.settings[UO_nl_func_scope_name].a != AV_IGNORE)
            {
               newline_iarf(prev, cpd.settings[UO_nl_func_scope_name].a);
            }
         }

         const chunk_t *tmp_next = chunk_get_next_ncnl(prev);
         if (tmp_next != nullptr && tmp_next->type != CT_FUNC_CLASS_DEF)
         {
            argval_t a = (tmp->parent_type == CT_FUNC_PROTO) ?
                         cpd.settings[UO_nl_func_proto_type_name].a :
                         cpd.settings[UO_nl_func_type_name].a;
            if (  (tmp->flags & PCF_IN_CLASS)
               && (cpd.settings[UO_nl_func_type_name_class].a != AV_IGNORE))
            {
               a = cpd.settings[UO_nl_func_type_name_class].a;
            }

            if (a != AV_IGNORE && prev != nullptr)
            {
               LOG_FMT(LNFD, "%s(%d): prev %zu:%zu '%s' [%s/%s]\n",
                       __func__, __LINE__, prev->orig_line, prev->orig_col,
                       prev->text(), get_token_name(prev->type),
                       get_token_name(prev->parent_type));

               if (prev->type == CT_DESTRUCTOR)
               {
                  prev = chunk_get_prev_ncnl(prev);
               }

               /*
                * If we are on a '::', step back two tokens
                * TODO: do we also need to check for '.' ?
                */
               while (chunk_is_token(prev, CT_DC_MEMBER))
               {
                  prev = chunk_get_prev_ncnl(prev);
                  prev = skip_template_prev(prev);
                  prev = chunk_get_prev_ncnl(prev);
               }

               if (  prev != nullptr
                  && prev->type != CT_BRACE_CLOSE
                  && prev->type != CT_VBRACE_CLOSE
                  && prev->type != CT_BRACE_OPEN
                  && prev->type != CT_SEMICOLON
                  && prev->type != CT_PRIVATE_COLON)
               {
                  newline_iarf(prev, a);
               }
            }
         }
      }

      chunk_t *pc = chunk_get_next_ncnl(start);
      if (chunk_is_str(pc, ")", 1))
      {
         atmp = cpd.settings[is_def ? UO_nl_func_def_empty : UO_nl_func_decl_empty].a;
         if (atmp != AV_IGNORE)
         {
            newline_iarf(start, atmp);
         }

         atmp = cpd.settings[is_def ? UO_nl_func_def_paren_empty : UO_nl_func_paren_empty].a;
         if (atmp != AV_IGNORE)
         {
            prev = chunk_get_prev_ncnl(start);
            if (prev != NULL)
            {
               newline_iarf(prev, atmp);
            }
         }
         return;
      }
   }

   // Now scan for commas
   size_t  comma_count = 0;
   chunk_t *tmp;
   chunk_t *pc;
   for (pc = chunk_get_next_ncnl(start);
        pc != nullptr && pc->level > start->level;
        pc = chunk_get_next_ncnl(pc))
   {
      if (pc->type == CT_COMMA && (pc->level == (start->level + 1)))
      {
         comma_count++;
         tmp = chunk_get_next(pc);
         if (chunk_is_comment(tmp))
         {
            pc = tmp;
         }
         newline_iarf(pc, cpd.settings[(  start->parent_type == CT_FUNC_DEF
                                       || start->parent_type == CT_FUNC_CLASS_DEF) ?
                                       UO_nl_func_def_args :
                                       UO_nl_func_decl_args].a);
      }
   }

   argval_t as = cpd.settings[is_def ? UO_nl_func_def_start : UO_nl_func_decl_start].a;
   argval_t ae = cpd.settings[is_def ? UO_nl_func_def_end : UO_nl_func_decl_end].a;
   if (comma_count == 0)
   {
      argval_t atmp;
      atmp = cpd.settings[is_def ? UO_nl_func_def_start_single :
                          UO_nl_func_decl_start_single].a;
      if (atmp != AV_IGNORE)
      {
         as = atmp;
      }
      atmp = cpd.settings[is_def ? UO_nl_func_def_end_single :
                          UO_nl_func_decl_end_single].a;
      if (atmp != AV_IGNORE)
      {
         ae = atmp;
      }
   }
   newline_iarf(start, as);

   // and fix up the close parenthesis
   if (chunk_is_token(pc, CT_FPAREN_CLOSE))
   {
      prev = chunk_get_prev_nnl(pc);
      if (prev != nullptr && prev->type != CT_FPAREN_OPEN)
      {
         newline_iarf(prev, ae);
      }

      newline_func_multi_line(start);
   }
} // newline_func_def_or_call


static void newline_oc_msg(chunk_t *start)
{
   LOG_FUNC_ENTRY();

   chunk_t *sq_c = chunk_skip_to_match(start);
   if (!sq_c)
   {
      return;
   }

   // mark one-liner
   bool    one_liner = true;
   chunk_t *pc;
   for (pc = chunk_get_next(start);
        pc && pc != sq_c;
        pc = chunk_get_next(pc))
   {
      if (pc->level <= start->level)
      {
         break;
      }
      if (chunk_is_newline(pc))
      {
         one_liner = false;
      }
   }

   // we don't use the 1-liner flag, but set it anyway
   UINT64 flags = one_liner ? PCF_ONE_LINER : 0;
   flag_series(start, sq_c, flags, flags ^ PCF_ONE_LINER);

   if (cpd.settings[UO_nl_oc_msg_leave_one_liner].b && one_liner)
   {
      return;
   }

   for (pc = chunk_get_next_ncnl(start); pc; pc = chunk_get_next_ncnl(pc))
   {
      if (pc->level <= start->level)
      {
         break;
      }
      if (pc->type == CT_OC_MSG_NAME)
      {
         newline_add_before(pc);
      }
   }
} // newline_oc_msg


static bool one_liner_nl_ok(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LNL1LINE, "%s(%d): check type is %s, parent is %s, flag is %" PRIx64 ", orig_line is %zu, orig_col is %zu\n",
           __func__, __LINE__, get_token_name(pc->type), get_token_name(pc->parent_type),
           pc->flags, pc->orig_line, pc->orig_col);


   if (!(pc->flags & PCF_ONE_LINER))
   {
      LOG_FMT(LNL1LINE, "%s(%d): true (not 1-liner), a new line may be added\n", __func__, __LINE__);
      return(true);
   }

   // Step back to find the opening brace
   chunk_t *br_open = pc;
   if (chunk_is_closing_brace(br_open))
   {
      br_open = chunk_get_prev_type(br_open,
                                    br_open->type == CT_BRACE_CLOSE ? CT_BRACE_OPEN : CT_VBRACE_OPEN,
                                    br_open->level, scope_e::ALL);
   }
   else
   {
      while (  br_open
            && (br_open->flags & PCF_ONE_LINER)
            && !chunk_is_opening_brace(br_open)
            && !chunk_is_closing_brace(br_open))
      {
         br_open = chunk_get_prev(br_open);
      }
   }
   pc = br_open;
   if (  pc
      && (pc->flags & PCF_ONE_LINER)
      && (  pc->type == CT_BRACE_OPEN
         || pc->type == CT_BRACE_CLOSE
         || pc->type == CT_VBRACE_OPEN
         || pc->type == CT_VBRACE_CLOSE))
   {
      if (  cpd.settings[UO_nl_class_leave_one_liners].b
         && (pc->flags & PCF_IN_CLASS))
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (class)\n", __func__, __LINE__);
         return(false);
      }

      if (  cpd.settings[UO_nl_assign_leave_one_liners].b
         && pc->parent_type == CT_ASSIGN)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (assign)\n", __func__, __LINE__);
         return(false);
      }

      if (  cpd.settings[UO_nl_enum_leave_one_liners].b
         && pc->parent_type == CT_ENUM)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (enum)\n", __func__, __LINE__);
         return(false);
      }

      if (  cpd.settings[UO_nl_getset_leave_one_liners].b
         && pc->parent_type == CT_GETSET)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (get/set), a new line may NOT be added\n", __func__, __LINE__);
         return(false);
      }

      if (  cpd.settings[UO_nl_cs_property_leave_one_liners].b
         && pc->parent_type == CT_CS_PROPERTY)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (c# property), a new line may NOT be added\n", __func__, __LINE__);
         return(false);
      }

      if (  cpd.settings[UO_nl_func_leave_one_liners].b
         && (  pc->parent_type == CT_FUNC_DEF
            || pc->parent_type == CT_FUNC_CLASS_DEF))
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (func def)\n", __func__, __LINE__);
         return(false);
      }

      if (  cpd.settings[UO_nl_func_leave_one_liners].b
         && pc->parent_type == CT_OC_MSG_DECL)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (method def)\n", __func__, __LINE__);
         return(false);
      }

      if (  cpd.settings[UO_nl_cpp_lambda_leave_one_liners].b
         && ((pc->parent_type == CT_CPP_LAMBDA)))
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (lambda)\n", __func__, __LINE__);
         return(false);
      }

      if (  cpd.settings[UO_nl_oc_msg_leave_one_liner].b
         && (pc->flags & PCF_IN_OC_MSG))
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (message)\n", __func__, __LINE__);
         return(false);
      }

      if (  cpd.settings[UO_nl_if_leave_one_liners].b
         && (  pc->parent_type == CT_IF
            || pc->parent_type == CT_ELSEIF
            || pc->parent_type == CT_ELSE))
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (if/else)\n", __func__, __LINE__);
         return(false);
      }

      if (  cpd.settings[UO_nl_while_leave_one_liners].b
         && pc->parent_type == CT_WHILE)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (while)\n", __func__, __LINE__);
         return(false);
      }
   }
   LOG_FMT(LNL1LINE, "%s(%d): true, a new line may be added\n", __func__, __LINE__);
   return(true);
} // one_liner_nl_ok


void undo_one_liner(chunk_t *pc)
{
   LOG_FUNC_ENTRY();

   if (pc && (pc->flags & PCF_ONE_LINER))
   {
      LOG_FMT(LNL1LINE, "%s(%d): pc->text() '%s', orig_line is %zu, orig_col is %zu",
              __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
      chunk_flags_clr(pc, PCF_ONE_LINER);

      // scan backward
      LOG_FMT(LNL1LINE, "%s(%d): scan backward\n", __func__, __LINE__);
      chunk_t *tmp = pc;
      while ((tmp = chunk_get_prev(tmp)) != nullptr)
      {
         if (!(tmp->flags & PCF_ONE_LINER))
         {
            LOG_FMT(LNL1LINE, "%s(%d): tmp->text() '%s', orig_line is %zu, orig_col is %zu, --> break\n",
                    __func__, __LINE__, tmp->text(), tmp->orig_line, tmp->orig_col);
            break;
         }
         LOG_FMT(LNL1LINE, "%s(%d): clear for tmp->text() '%s', orig_line is %zu, orig_col is %zu",
                 __func__, __LINE__, tmp->text(), tmp->orig_line, tmp->orig_col);
         chunk_flags_clr(tmp, PCF_ONE_LINER);
      }

      // scan forward
      LOG_FMT(LNL1LINE, "%s(%d): scan forward\n", __func__, __LINE__);
      tmp = pc;
      LOG_FMT(LNL1LINE, "%s(%d): - \n", __func__, __LINE__);
      while ((tmp = chunk_get_next(tmp)) != nullptr)
      {
         if (!(tmp->flags & PCF_ONE_LINER))
         {
            LOG_FMT(LNL1LINE, "%s(%d): tmp->text() '%s', orig_line is %zu, orig_col is %zu, --> break\n",
                    __func__, __LINE__, tmp->text(), tmp->orig_line, tmp->orig_col);
            break;
         }
         LOG_FMT(LNL1LINE, "%s(%d): clear for tmp->text() '%s', orig_line is %zu, orig_col is %zu",
                 __func__, __LINE__, tmp->text(), tmp->orig_line, tmp->orig_col);
         chunk_flags_clr(tmp, PCF_ONE_LINER);
      }
      LOG_FMT(LNL1LINE, "\n");
   }
} // undo_one_liner


static void nl_create_one_liner(chunk_t *vbrace_open)
{
   LOG_FUNC_ENTRY();

   // See if we get a newline between the next text and the vbrace_close
   chunk_t *tmp   = chunk_get_next_ncnl(vbrace_open);
   chunk_t *first = tmp;
   if (!first || get_token_pattern_class(first->type) != pattern_class_e::NONE)
   {
      return;
   }

   size_t nl_total = 0;
   while (tmp != nullptr && tmp->type != CT_VBRACE_CLOSE)
   {
      if (chunk_is_newline(tmp))
      {
         nl_total += tmp->nl_count;
         if (nl_total > 1)
         {
            return;
         }
      }
      tmp = chunk_get_next(tmp);
   }

   if (tmp != nullptr && first != nullptr)
   {
      newline_del_between(vbrace_open, first);
   }
}


void newlines_remove_newlines(void)
{
   LOG_FUNC_ENTRY();

   chunk_t *pc = chunk_get_head();
   if (!chunk_is_newline(pc))
   {
      pc = chunk_get_next_nl(pc);
   }

   chunk_t *next;
   chunk_t *prev;
   while (pc != nullptr)
   {
      // Remove all newlines not in preproc
      if (!(pc->flags & PCF_IN_PREPROC))
      {
         next = pc->next;
         prev = pc->prev;
         newline_iarf(pc, AV_REMOVE);
         if (next == chunk_get_head())
         {
            pc = next;
            continue;
         }
         else if (prev && !chunk_is_newline(prev->next))
         {
            pc = prev;
         }
      }
      pc = chunk_get_next_nl(pc);
   }
}


void newlines_cleanup_braces(bool first)
{
   LOG_FUNC_ENTRY();

   // Get the first token that's not an empty line:
   chunk_t *pc;
   if (chunk_is_newline(pc = chunk_get_head()))
   {
      pc = chunk_get_next_ncnl(pc);
   }

   chunk_t *next;
   chunk_t *prev;
   chunk_t *tmp;
   for ( ; pc != nullptr; pc = chunk_get_next_ncnl(pc))
   {
      if (pc->type == CT_IF)
      {
         newlines_if_for_while_switch(pc, cpd.settings[UO_nl_if_brace].a);
         tmp = chunk_get_next_type(pc, CT_SPAREN_CLOSE, pc->level);
         if (tmp != nullptr)
         {
            prev = chunk_get_prev(tmp);
            if (prev != nullptr)
            {
               // Issue #1139
               newline_iarf_pair(prev, tmp, cpd.settings[UO_nl_before_if_closing_paren].a);
            }
         }
      }
      else if (pc->type == CT_ELSEIF)
      {
         argval_t arg = cpd.settings[UO_nl_elseif_brace].a;
         newlines_if_for_while_switch(
            pc, (arg != AV_IGNORE) ? arg : cpd.settings[UO_nl_if_brace].a);
         tmp = chunk_get_next_type(pc, CT_SPAREN_CLOSE, pc->level);
         if (tmp != nullptr)
         {
            prev = chunk_get_prev(tmp);
            if (prev != nullptr)
            {
               // Issue #1139
               newline_iarf_pair(prev, tmp, cpd.settings[UO_nl_before_if_closing_paren].a);
            }
         }
      }
      else if (pc->type == CT_FOR)
      {
         newlines_if_for_while_switch(pc, cpd.settings[UO_nl_for_brace].a);
      }
      else if (pc->type == CT_CATCH)
      {
         newlines_cuddle_uncuddle(pc, cpd.settings[UO_nl_brace_catch].a);
         next = chunk_get_next_ncnl(pc);
         if (chunk_is_token(next, CT_BRACE_OPEN))
         {
            newlines_do_else(pc, cpd.settings[UO_nl_catch_brace].a);
         }
         else
         {
            newlines_if_for_while_switch(pc, cpd.settings[UO_nl_catch_brace].a);
         }
      }
      else if (pc->type == CT_WHILE)
      {
         newlines_if_for_while_switch(pc, cpd.settings[UO_nl_while_brace].a);
      }
      else if (pc->type == CT_USING_STMT)
      {
         newlines_if_for_while_switch(pc, cpd.settings[UO_nl_using_brace].a);
      }
      else if (pc->type == CT_D_SCOPE_IF)
      {
         newlines_if_for_while_switch(pc, cpd.settings[UO_nl_scope_brace].a);
      }
      else if (pc->type == CT_UNITTEST)
      {
         newlines_do_else(pc, cpd.settings[UO_nl_unittest_brace].a);
      }
      else if (pc->type == CT_D_VERSION_IF)
      {
         newlines_if_for_while_switch(pc, cpd.settings[UO_nl_version_brace].a);
      }
      else if (pc->type == CT_SWITCH)
      {
         newlines_if_for_while_switch(pc, cpd.settings[UO_nl_switch_brace].a);
      }
      else if (pc->type == CT_SYNCHRONIZED)
      {
         newlines_if_for_while_switch(pc,
                                      cpd.settings[UO_nl_synchronized_brace].a);
      }
      else if (pc->type == CT_DO)
      {
         newlines_do_else(pc, cpd.settings[UO_nl_do_brace].a);
      }
      else if (pc->type == CT_ELSE)
      {
         newlines_cuddle_uncuddle(pc, cpd.settings[UO_nl_brace_else].a);
         next = chunk_get_next_ncnl(pc);
         if (chunk_is_token(next, CT_ELSEIF))
         {
            newline_iarf_pair(pc, next, cpd.settings[UO_nl_else_if].a);
         }
         newlines_do_else(pc, cpd.settings[UO_nl_else_brace].a);
      }
      else if (pc->type == CT_TRY)
      {
         newlines_do_else(pc, cpd.settings[UO_nl_try_brace].a);
      }
      else if (pc->type == CT_GETSET)
      {
         newlines_do_else(pc, cpd.settings[UO_nl_getset_brace].a);
      }
      else if (pc->type == CT_FINALLY)
      {
         newlines_cuddle_uncuddle(pc, cpd.settings[UO_nl_brace_finally].a);
         newlines_do_else(pc, cpd.settings[UO_nl_finally_brace].a);
      }
      else if (pc->type == CT_WHILE_OF_DO)
      {
         newlines_cuddle_uncuddle(pc, cpd.settings[UO_nl_brace_while].a);
      }
      else if (pc->type == CT_BRACE_OPEN)
      {
         switch (pc->parent_type)
         {
         case CT_DOUBLE_BRACE:
         {
            if (cpd.settings[UO_nl_paren_dbrace_open].a != AV_IGNORE)
            {
               prev = chunk_get_prev_ncnl(pc, scope_e::PREPROC);
               if (chunk_is_paren_close(prev))
               {
                  newline_iarf_pair(prev, pc, cpd.settings[UO_nl_paren_dbrace_open].a);
               }
            }
            break;
         }

         case CT_ENUM:
         {
            if (cpd.settings[UO_nl_enum_own_lines].a != AV_IGNORE)
            {
               newlines_enum_entries(pc, cpd.settings[UO_nl_enum_own_lines].a);
            }
            if (cpd.settings[UO_nl_ds_struct_enum_cmt].b)
            {
               newlines_double_space_struct_enum_union(pc);
            }
            break;
         }

         case CT_STRUCT:
         case CT_UNION:
         {
            if (cpd.settings[UO_nl_ds_struct_enum_cmt].b)
            {
               newlines_double_space_struct_enum_union(pc);
            }
            break;
         }

         case CT_CLASS:
         {
            if (pc->level == pc->brace_level)
            {
               newlines_do_else(chunk_get_prev_nnl(pc), cpd.settings[UO_nl_class_brace].a);
            }
            break;
         }

         case CT_BRACED_INIT_LIST:
         {
            newline_iarf_pair(chunk_get_prev_nnl(pc), pc, cpd.settings[UO_nl_type_brace_init_lst].a);
            break;
         }

         case CT_OC_BLOCK_EXPR:
         {
            // issue # 477
            newline_iarf_pair(chunk_get_prev(pc), pc, cpd.settings[UO_nl_oc_block_brace].a);
            break;
         }

         default:
         {
            break;
         }
         } // switch

         if (cpd.settings[UO_nl_brace_brace].a != AV_IGNORE)
         {
            next = chunk_get_next_nc(pc, scope_e::PREPROC);
            if (chunk_is_token(next, CT_BRACE_OPEN))
            {
               newline_iarf_pair(pc, next, cpd.settings[UO_nl_brace_brace].a);
            }
         }

         next = chunk_get_next_nnl(pc);
         if (next == nullptr)
         {
            // do nothing
         }
         else if (next->type == CT_BRACE_CLOSE)
         {
            // TODO: add an option to split open empty statements? { };
         }
         else if (next->type == CT_BRACE_OPEN)
         {
            // already handled
         }
         else
         {
            next = chunk_get_next_ncnl(pc);

            // Handle unnamed temporary direct-list-initialization
            if (pc->parent_type == CT_BRACED_INIT_LIST)
            {
               newline_iarf_pair(pc, chunk_get_next_nnl(pc),
                                 cpd.settings[UO_nl_type_brace_init_lst_open].a);
            }
            // Handle nl_after_brace_open
            else if (  (  pc->parent_type == CT_CPP_LAMBDA
                       || pc->level == pc->brace_level)
                    && cpd.settings[UO_nl_after_brace_open].b)
            {
               if (!one_liner_nl_ok(pc))
               {
                  LOG_FMT(LNL1LINE, "a new line may NOT be added\n");
                  // no change - preserve one liner body
               }
               else if (pc->flags & (PCF_IN_ARRAY_ASSIGN | PCF_IN_PREPROC))
               {
                  // no change - don't break up array assignments or preprocessors
               }
               else
               {
                  // Step back from next to the first non-newline item
                  tmp = chunk_get_prev(next);
                  while (tmp != pc)
                  {
                     if (chunk_is_comment(tmp))
                     {
                        if (  !cpd.settings[UO_nl_after_brace_open_cmt].b
                           && tmp->type != CT_COMMENT_MULTI)
                        {
                           break;
                        }
                     }
                     tmp = chunk_get_prev(tmp);
                  }
                  // Add the newline
                  newline_iarf(tmp, AV_ADD);
               }
            }
         }

         // braced-init-list is more like a function call with arguments,
         // than curly braces that determine a structure of a source code,
         // so, don't add a newline before a closing brace. Issue #1405.
         if (!(  pc->parent_type == CT_BRACED_INIT_LIST
              && cpd.settings[UO_nl_type_brace_init_lst_open].a == AV_IGNORE
              && cpd.settings[UO_nl_type_brace_init_lst_close].a == AV_IGNORE))
         {
            newlines_brace_pair(pc);
         }
      }
      else if (pc->type == CT_BRACE_CLOSE)
      {
         // newline between a close brace and x
         if (cpd.settings[UO_nl_brace_brace].a != AV_IGNORE)
         {
            next = chunk_get_next_nc(pc, scope_e::PREPROC);
            if (chunk_is_token(next, CT_BRACE_CLOSE))
            {
               newline_iarf_pair(pc, next, cpd.settings[UO_nl_brace_brace].a);
            }
         }

         if (cpd.settings[UO_nl_brace_square].a != AV_IGNORE)
         {
            next = chunk_get_next_nc(pc, scope_e::PREPROC);
            if (chunk_is_token(next, CT_SQUARE_CLOSE))
            {
               newline_iarf_pair(pc, next, cpd.settings[UO_nl_brace_square].a);
            }
         }

         if (cpd.settings[UO_nl_brace_fparen].a != AV_IGNORE)
         {
            next = chunk_get_next_nc(pc, scope_e::PREPROC);
            if (  chunk_is_token(next, CT_NEWLINE)
               && (cpd.settings[UO_nl_brace_fparen].a == AV_REMOVE))
            {
               next = chunk_get_next_nc(next, scope_e::PREPROC);  // Issue #1000
            }
            if (chunk_is_token(next, CT_FPAREN_CLOSE))
            {
               newline_iarf_pair(pc, next, cpd.settings[UO_nl_brace_fparen].a);
            }
         }

         // newline before a close brace
         if (  pc->parent_type == CT_BRACED_INIT_LIST
            && cpd.settings[UO_nl_type_brace_init_lst_close].a != AV_IGNORE)
         {
            // Handle unnamed temporary direct-list-initialization
            newline_iarf_pair(chunk_get_prev_nnl(pc), pc,
                              cpd.settings[UO_nl_type_brace_init_lst_close].a);
         }

         // blanks before a close brace
         if (cpd.settings[UO_eat_blanks_before_close_brace].b)
         {
            // Limit the newlines before the close brace to 1
            prev = chunk_get_prev(pc);
            if (chunk_is_newline(prev))
            {
               if (prev->nl_count != 1)
               {
                  prev->nl_count = 1;
                  LOG_FMT(LBLANKD, "%s(%d): eat_blanks_before_close_brace %zu\n",
                          __func__, __LINE__, prev->orig_line);
                  MARK_CHANGE();
               }
            }
         }
         else if (  cpd.settings[UO_nl_ds_struct_enum_close_brace].b
                 && (  pc->parent_type == CT_ENUM
                    || pc->parent_type == CT_STRUCT
                    || pc->parent_type == CT_UNION))
         {
            if ((pc->flags & PCF_ONE_LINER) == 0)
            {
               // Make sure the brace is preceded by two newlines
               prev = chunk_get_prev(pc);
               if (!chunk_is_newline(prev))
               {
                  prev = newline_add_before(pc);
               }
               if (prev->nl_count < 2)
               {
                  double_newline(prev);
               }
            }
         }

         // Force a newline after a close brace
         if (  (cpd.settings[UO_nl_brace_struct_var].a != AV_IGNORE)
            && (  pc->parent_type == CT_STRUCT
               || pc->parent_type == CT_ENUM
               || pc->parent_type == CT_UNION))
         {
            next = chunk_get_next_ncnl(pc, scope_e::PREPROC);
            if (  next
               && next->type != CT_SEMICOLON
               && next->type != CT_COMMA)
            {
               newline_iarf(pc, cpd.settings[UO_nl_brace_struct_var].a);
            }
         }
         else if (  cpd.settings[UO_nl_after_brace_close].b
                 || pc->parent_type == CT_FUNC_CLASS_DEF
                 || pc->parent_type == CT_FUNC_DEF
                 || pc->parent_type == CT_OC_MSG_DECL)
         {
            next = chunk_get_next(pc);
            if (  next != nullptr
               && next->type != CT_SEMICOLON
               && next->type != CT_COMMA
               && next->type != CT_SPAREN_CLOSE    // Issue #664
               && next->type != CT_SQUARE_CLOSE
               && next->type != CT_FPAREN_CLOSE
               && next->type != CT_PAREN_CLOSE
               && next->type != CT_WHILE_OF_DO
               && next->type != CT_VBRACE_CLOSE                                    // Issue #666
               && (next->type != CT_BRACE_CLOSE || !(next->flags & PCF_ONE_LINER)) // #1258
               && (pc->flags & (PCF_IN_ARRAY_ASSIGN | PCF_IN_TYPEDEF)) == 0
               && !chunk_is_newline(next)
               && !chunk_is_comment(next))
            {
               // #1258
               // dont add newline between two consecutive braces closes, if the second is a part of one liner.
               newline_end_newline(pc);
            }
         }
      }
      else if (pc->type == CT_VBRACE_OPEN)
      {
         if (  cpd.settings[UO_nl_after_vbrace_open].b
            || cpd.settings[UO_nl_after_vbrace_open_empty].b)
         {
            next = chunk_get_next(pc, scope_e::PREPROC);
            bool add_it;
            if (chunk_is_semicolon(next))
            {
               add_it = cpd.settings[UO_nl_after_vbrace_open_empty].b;
            }
            else
            {
               add_it = (  cpd.settings[UO_nl_after_vbrace_open].b
                        && next->type != CT_VBRACE_CLOSE
                        && !chunk_is_comment(next)
                        && !chunk_is_newline(next));
            }
            if (add_it)
            {
               newline_iarf(pc, AV_ADD);
            }
         }

         if (  (  (  pc->parent_type == CT_IF
                  || pc->parent_type == CT_ELSEIF
                  || pc->parent_type == CT_ELSE)
               && cpd.settings[UO_nl_create_if_one_liner].b)
            || (  pc->parent_type == CT_FOR
               && cpd.settings[UO_nl_create_for_one_liner].b)
            || (  pc->parent_type == CT_WHILE
               && cpd.settings[UO_nl_create_while_one_liner].b))
         {
            nl_create_one_liner(pc);
         }
         if (  (  (  pc->parent_type == CT_IF
                  || pc->parent_type == CT_ELSEIF
                  || pc->parent_type == CT_ELSE)
               && cpd.settings[UO_nl_split_if_one_liner].b)
            || (  pc->parent_type == CT_FOR
               && cpd.settings[UO_nl_split_for_one_liner].b)
            || (  pc->parent_type == CT_WHILE
               && cpd.settings[UO_nl_split_while_one_liner].b))
         {
            if (pc->flags & PCF_ONE_LINER)
            {
               // split one-liner
               chunk_t *end = chunk_get_next(chunk_get_next_type(pc->next, CT_SEMICOLON, -1));
               // Scan for clear flag
               D_LOG_FMT(LNEWLINE, "(%d) ", __LINE__);
               LOG_FMT(LNEWLINE, "\n");
               for (chunk_t *temp = pc; temp != end; temp = chunk_get_next(temp))
               {
                  LOG_FMT(LNEWLINE, "%s type=%s , level=%zu", temp->text(), get_token_name(temp->type), temp->level);
                  log_pcf_flags(LNEWLINE, temp->flags);
                  chunk_flags_clr(temp, PCF_ONE_LINER);
               }
               // split
               newline_add_between(pc, pc->next);
            }
         }
      }
      else if (pc->type == CT_VBRACE_CLOSE)
      {
         if (cpd.settings[UO_nl_after_vbrace_close].b)
         {
            if (!chunk_is_newline(chunk_get_next_nc(pc)))
            {
               newline_iarf(pc, AV_ADD);
            }
         }
      }
      else if (pc->type == CT_SQUARE_OPEN && pc->parent_type == CT_OC_MSG)
      {
         if (cpd.settings[UO_nl_oc_msg_args].b)
         {
            newline_oc_msg(pc);
         }
      }
      else if (pc->type == CT_STRUCT)
      {
         newlines_struct_union(pc, cpd.settings[UO_nl_struct_brace].a, true);
      }
      else if (pc->type == CT_UNION)
      {
         newlines_struct_union(pc, cpd.settings[UO_nl_union_brace].a, true);
      }
      else if (pc->type == CT_ENUM)
      {
         newlines_enum(pc);
      }
      else if (pc->type == CT_CASE)
      {
         // Note: 'default' also maps to CT_CASE
         if (cpd.settings[UO_nl_before_case].b)
         {
            newline_case(pc);
         }
      }
      else if (pc->type == CT_THROW)
      {
         prev = chunk_get_prev(pc);
         if (chunk_is_token(prev, CT_PAREN_CLOSE))
         {
            newline_iarf(chunk_get_prev_ncnl(pc), cpd.settings[UO_nl_before_throw].a);
         }
      }
      else if (pc->type == CT_CASE_COLON)
      {
         next = chunk_get_next_nnl(pc);
         if (  chunk_is_token(next, CT_BRACE_OPEN)
            && cpd.settings[UO_nl_case_colon_brace].a != AV_IGNORE)
         {
            newline_iarf(pc, cpd.settings[UO_nl_case_colon_brace].a);
         }
         else if (cpd.settings[UO_nl_after_case].b)
         {
            newline_case_colon(pc);
         }
      }
      else if (pc->type == CT_SPAREN_CLOSE)
      {
         next = chunk_get_next_ncnl(pc);
         if (chunk_is_token(next, CT_BRACE_OPEN))
         {
            /*
             * TODO: this could be used to control newlines between the
             * the if/while/for/switch close parenthesis and the open brace, but
             * that is currently handled elsewhere.
             */
         }
      }
      else if (pc->type == CT_RETURN)
      {
         if (cpd.settings[UO_nl_before_return].b)
         {
            newline_before_return(pc);
         }
         if (cpd.settings[UO_nl_after_return].b)
         {
            newline_after_return(pc);
         }
      }
      else if (pc->type == CT_SEMICOLON)
      {
         if (  ((pc->flags & (PCF_IN_SPAREN | PCF_IN_PREPROC)) == 0)
            && cpd.settings[UO_nl_after_semicolon].b)
         {
            next = chunk_get_next(pc);
            while (chunk_is_token(next, CT_VBRACE_CLOSE))
            {
               next = chunk_get_next(next);
            }
            if (  next != nullptr
               && !chunk_is_comment(next)
               && !chunk_is_newline(next))
            {
               if (one_liner_nl_ok(next))
               {
                  LOG_FMT(LNL1LINE, "%s(%d): a new line may be added\n", __func__, __LINE__);
                  newline_iarf(pc, AV_ADD);
               }
               else
               {
                  LOG_FMT(LNL1LINE, "%s(%d): a new line may NOT be added\n", __func__, __LINE__);
               }
            }
         }
         else if (pc->parent_type == CT_CLASS)
         {
            if (cpd.settings[UO_nl_after_class].u > 0)
            {
               newline_iarf(pc, AV_ADD);
            }
         }
      }
      else if (pc->type == CT_FPAREN_OPEN)
      {
         if (  (  pc->parent_type == CT_FUNC_DEF
               || pc->parent_type == CT_FUNC_PROTO
               || pc->parent_type == CT_FUNC_CLASS_DEF
               || pc->parent_type == CT_FUNC_CLASS_PROTO
               || pc->parent_type == CT_OPERATOR)
            && (  cpd.settings[UO_nl_func_decl_start].a != AV_IGNORE
               || cpd.settings[UO_nl_func_def_start].a != AV_IGNORE
               || cpd.settings[UO_nl_func_decl_start_single].a != AV_IGNORE
               || cpd.settings[UO_nl_func_def_start_single].a != AV_IGNORE
               || cpd.settings[UO_nl_func_decl_start_multi_line].b
               || cpd.settings[UO_nl_func_def_start_multi_line].b
               || cpd.settings[UO_nl_func_decl_args].a != AV_IGNORE
               || cpd.settings[UO_nl_func_def_args].a != AV_IGNORE
               || cpd.settings[UO_nl_func_decl_args_multi_line].b
               || cpd.settings[UO_nl_func_def_args_multi_line].b
               || cpd.settings[UO_nl_func_decl_end].a != AV_IGNORE
               || cpd.settings[UO_nl_func_def_end].a != AV_IGNORE
               || cpd.settings[UO_nl_func_decl_end_single].a != AV_IGNORE
               || cpd.settings[UO_nl_func_def_end_single].a != AV_IGNORE
               || cpd.settings[UO_nl_func_decl_end_multi_line].b
               || cpd.settings[UO_nl_func_def_end_multi_line].b
               || cpd.settings[UO_nl_func_decl_empty].a != AV_IGNORE
               || cpd.settings[UO_nl_func_def_empty].a != AV_IGNORE
               || cpd.settings[UO_nl_func_type_name].a != AV_IGNORE
               || cpd.settings[UO_nl_func_type_name_class].a != AV_IGNORE
               || cpd.settings[UO_nl_func_class_scope].a != AV_IGNORE
               || cpd.settings[UO_nl_func_scope_name].a != AV_IGNORE
               || cpd.settings[UO_nl_func_proto_type_name].a != AV_IGNORE
               || cpd.settings[UO_nl_func_paren].a != AV_IGNORE
               || cpd.settings[UO_nl_func_def_paren].a != AV_IGNORE
               || cpd.settings[UO_nl_func_def_paren_empty].a != AV_IGNORE
               || cpd.settings[UO_nl_func_paren_empty].a != AV_IGNORE))
         {
            newline_func_def_or_call(pc);
         }
         else if (  (  pc->parent_type == CT_FUNC_CALL
                    || pc->parent_type == CT_FUNC_CALL_USER)
                 && (  (cpd.settings[UO_nl_func_call_start_multi_line].b)
                    || (cpd.settings[UO_nl_func_call_args_multi_line].b)
                    || (cpd.settings[UO_nl_func_call_end_multi_line].b)
                    || (cpd.settings[UO_nl_func_call_paren].a != AV_IGNORE)
                    || (cpd.settings[UO_nl_func_call_paren_empty].a != AV_IGNORE)
                    || (cpd.settings[UO_nl_func_call_empty].a != AV_IGNORE)))
         {
            if (  cpd.settings[UO_nl_func_call_paren].a != AV_IGNORE
               || cpd.settings[UO_nl_func_call_paren_empty].a != AV_IGNORE
               || cpd.settings[UO_nl_func_call_empty].a != AV_IGNORE)
            {
               newline_func_def_or_call(pc);
            }
            newline_func_multi_line(pc);
         }
         else if (first && (cpd.settings[UO_nl_remove_extra_newlines].u == 1))
         {
            newline_iarf(pc, AV_REMOVE);
         }
      }
      else if (pc->type == CT_ANGLE_CLOSE)
      {
         if (pc->parent_type == CT_TEMPLATE)
         {
            next = chunk_get_next_ncnl(pc);
            if (next != nullptr && next->level == next->brace_level)
            {
               tmp = chunk_get_prev_ncnl(chunk_get_prev_type(pc, CT_ANGLE_OPEN, pc->level));
               if (chunk_is_token(tmp, CT_TEMPLATE))
               {
                  newline_iarf(pc, cpd.settings[UO_nl_template_class].a);
               }
            }
         }
      }
      else if (pc->type == CT_NAMESPACE)
      {
         // Issue #1235
         if ((pc->next->next->flags & PCF_ONE_LINER) == 0)
         {
            newlines_struct_union(pc, cpd.settings[UO_nl_namespace_brace].a, false);
         }
      }
      else if (pc->type == CT_SQUARE_OPEN)
      {
         if (  pc->parent_type == CT_ASSIGN
            && ((pc->flags & PCF_ONE_LINER) == 0))
         {
            tmp = chunk_get_prev_ncnl(pc);
            newline_iarf(tmp, cpd.settings[UO_nl_assign_square].a);

            argval_t arg = cpd.settings[UO_nl_after_square_assign].a;

            if (cpd.settings[UO_nl_assign_square].a & AV_ADD)
            {
               arg = AV_ADD;
            }
            newline_iarf(pc, arg);

            /*
             * if there is a newline after the open, then force a newline
             * before the close
             */
            tmp = chunk_get_next_nc(pc);
            if (chunk_is_newline(tmp))
            {
               tmp = chunk_get_next_type(pc, CT_SQUARE_CLOSE, pc->level);
               if (tmp != nullptr)
               {
                  newline_add_before(tmp);
               }
            }
         }
      }
      else if (pc->type == CT_PRIVATE)
      {
         // Make sure there is a newline before an access spec
         if (cpd.settings[UO_nl_before_access_spec].u > 0)
         {
            prev = chunk_get_prev(pc);
            if (!chunk_is_newline(prev))
            {
               newline_add_before(pc);
            }
         }
      }
      else if (pc->type == CT_PRIVATE_COLON)
      {
         // Make sure there is a newline after an access spec
         if (cpd.settings[UO_nl_after_access_spec].u > 0)
         {
            next = chunk_get_next(pc);
            if (!chunk_is_newline(next))
            {
               newline_add_before(next);
            }
         }
      }
      else if (pc->type == CT_PP_DEFINE)
      {
         if (cpd.settings[UO_nl_multi_line_define].b)
         {
            nl_handle_define(pc);
         }
      }
      else if (  first
              && (cpd.settings[UO_nl_remove_extra_newlines].u == 1)
              && !(pc->flags & PCF_IN_PREPROC))
      {
         newline_iarf(pc, AV_REMOVE);
      }
      else
      {
         // ignore it
      }
   }
   newline_def_blk(chunk_get_head(), false);
} // newlines_cleanup_braces


static void nl_handle_define(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *nl  = pc;
   chunk_t *ref = nullptr;

   while ((nl = chunk_get_next(nl)) != nullptr)
   {
      if (nl->type == CT_NEWLINE)
      {
         return;
      }
      if (  nl->type == CT_MACRO
         || (nl->type == CT_FPAREN_CLOSE && nl->parent_type == CT_MACRO_FUNC))
      {
         ref = nl;
      }
      if (nl->type == CT_NL_CONT)
      {
         if (ref != nullptr)
         {
            newline_add_after(ref);
         }
         return;
      }
   }
}


void newline_after_multiline_comment(void)
{
   LOG_FUNC_ENTRY();

   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next(pc))
   {
      if (pc->type != CT_COMMENT_MULTI)
      {
         continue;
      }

      chunk_t *tmp = pc;
      while (((tmp = chunk_get_next(tmp)) != nullptr) && !chunk_is_newline(tmp))
      {
         if (!chunk_is_comment(tmp))
         {
            newline_add_before(tmp);
            break;
         }
      }
   }
}


void newline_after_label_colon(void)
{
   LOG_FUNC_ENTRY();

   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next(pc))
   {
      if (pc->type != CT_LABEL_COLON)
      {
         continue;
      }

      newline_add_after(pc);
   }
}


void newlines_insert_blank_lines(void)
{
   LOG_FUNC_ENTRY();

   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next_ncnl(pc))
   {
      LOG_FMT(LNEWLINE, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s', type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
      if (pc->type == CT_IF)
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, cpd.settings[UO_nl_before_if].a);
         newlines_if_for_while_switch_post_blank_lines(pc, cpd.settings[UO_nl_after_if].a);
      }
      else if (pc->type == CT_FOR)
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, cpd.settings[UO_nl_before_for].a);
         newlines_if_for_while_switch_post_blank_lines(pc, cpd.settings[UO_nl_after_for].a);
      }
      else if (pc->type == CT_WHILE)
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, cpd.settings[UO_nl_before_while].a);
         newlines_if_for_while_switch_post_blank_lines(pc, cpd.settings[UO_nl_after_while].a);
      }
      else if (pc->type == CT_SWITCH)
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, cpd.settings[UO_nl_before_switch].a);
         newlines_if_for_while_switch_post_blank_lines(pc, cpd.settings[UO_nl_after_switch].a);
      }
      else if (pc->type == CT_SYNCHRONIZED)
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, cpd.settings[UO_nl_before_synchronized].a);
         newlines_if_for_while_switch_post_blank_lines(pc, cpd.settings[UO_nl_after_synchronized].a);
      }
      else if (pc->type == CT_DO)
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, cpd.settings[UO_nl_before_do].a);
         newlines_if_for_while_switch_post_blank_lines(pc, cpd.settings[UO_nl_after_do].a);
      }
      else if (  pc->type == CT_FUNC_CLASS_DEF
              || pc->type == CT_FUNC_DEF
              || pc->type == CT_FUNC_CLASS_PROTO
              || pc->type == CT_FUNC_PROTO)
      {
         newlines_func_pre_blank_lines(pc);
      }
      else
      {
         // ignore it
         LOG_FMT(LNEWLINE, "%s(%d): ignore it\n", __func__, __LINE__);
      }
   }
} // newlines_insert_blank_lines


void newlines_functions_remove_extra_blank_lines(void)
{
   LOG_FUNC_ENTRY();

   const size_t nl_max_blank_in_func = cpd.settings[UO_nl_max_blank_in_func].u;
   if (nl_max_blank_in_func <= 0)
   {
      return;
   }

   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next(pc))
   {
      if (  pc->type != CT_BRACE_OPEN
         || (pc->parent_type != CT_FUNC_DEF && pc->parent_type != CT_CPP_LAMBDA))
      {
         continue;
      }

      const size_t startMoveLevel = pc->level;

      while (pc != nullptr)
      {
         if (pc->type == CT_BRACE_CLOSE && pc->level == startMoveLevel)
         {
            break;
         }

         // delete newlines
         if (pc->nl_count > nl_max_blank_in_func)
         {
            pc->nl_count = nl_max_blank_in_func;
            MARK_CHANGE();
            remove_next_newlines(pc);
         }
         else
         {
            pc = chunk_get_next(pc);
         }
      }
   }
}


void newlines_squeeze_ifdef(void)
{
   LOG_FUNC_ENTRY();

   chunk_t *pc;
   for (pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next_ncnl(pc))
   {
      if (  pc->type == CT_PREPROC
         && (pc->level > 0 || cpd.settings[UO_nl_squeeze_ifdef_top_level].b))
      {
         chunk_t *ppr = chunk_get_next(pc);

         if (  ppr->type == CT_PP_IF
            || ppr->type == CT_PP_ELSE
            || ppr->type == CT_PP_ENDIF)
         {
            chunk_t *pnl = nullptr;
            chunk_t *nnl = chunk_get_next_nl(ppr);
            if (ppr->type == CT_PP_ELSE || ppr->type == CT_PP_ENDIF)
            {
               pnl = chunk_get_prev_nl(pc);
            }

            chunk_t *tmp1;
            chunk_t *tmp2;
            if (nnl != nullptr)
            {
               if (pnl != nullptr)
               {
                  if (pnl->nl_count > 1)
                  {
                     //nnl->nl_count += pnl->nl_count - 1;
                     pnl->nl_count = 1;
                     MARK_CHANGE();

                     tmp1 = chunk_get_prev_nnl(pnl);
                     tmp2 = chunk_get_prev_nnl(nnl);

                     LOG_FMT(LNEWLINE, "%s(%d): moved from after line %zu to after %zu\n",
                             __func__, __LINE__, tmp1->orig_line, tmp2->orig_line);
                  }
               }

               if (ppr->type == CT_PP_IF || ppr->type == CT_PP_ELSE)
               {
                  if (nnl->nl_count > 1)
                  {
                     tmp1 = chunk_get_prev_nnl(nnl);
                     LOG_FMT(LNEWLINE, "%s(%d): trimmed newlines after line %zu from %zu\n",
                             __func__, __LINE__, tmp1->orig_line, nnl->nl_count);
                     nnl->nl_count = 1;
                     MARK_CHANGE();
                  }
               }
            }
         }
      }
   }
} // newlines_squeeze_ifdef


void newlines_squeeze_paren_close(void)
{
   LOG_FUNC_ENTRY();

   chunk_t *pc;
   for (pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next(pc))
   {
      chunk_t *next;
      chunk_t *prev;
      if (pc->type == CT_NEWLINE)
      {
         prev = chunk_get_prev(pc);
      }
      else
      {
         prev = pc;
      }
      next = chunk_get_next(pc);
      if (next != nullptr && prev != nullptr && chunk_is_paren_close(next) && chunk_is_paren_close(prev))
      {
         chunk_t *prev_op = chunk_skip_to_match_rev(prev);
         chunk_t *next_op = chunk_skip_to_match_rev(next);
         bool    flag     = true;
         if (true)
         {
            chunk_t *tmp = prev;
            while (chunk_is_paren_close(tmp))
            {
               tmp = chunk_get_prev(tmp);
            }
            if (tmp->type != CT_NEWLINE)
            {
               flag = false;
            }
         }
         if (flag)
         {
            if (are_chunks_in_same_line(next_op, prev_op))
            {
               if (pc->type == CT_NEWLINE)
               {
                  pc = next;
               }
               newline_del_between(prev, next);
            }
            else
            {
               newline_add_between(prev, next);
            }
         }
      }
   }
} // newlines_squeeze_paren_close


void newlines_eat_start_end(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;

   // Process newlines at the start of the file
   if (  cpd.frag_cols == 0
      && (  (cpd.settings[UO_nl_start_of_file].a & AV_REMOVE)
         || (  (cpd.settings[UO_nl_start_of_file].a & AV_ADD)
            && (cpd.settings[UO_nl_start_of_file_min].u > 0))))
   {
      pc = chunk_get_head();
      if (pc != nullptr)
      {
         if (pc->type == CT_NEWLINE)
         {
            if (cpd.settings[UO_nl_start_of_file].a == AV_REMOVE)
            {
               LOG_FMT(LBLANKD, "%s(%d): eat_blanks_start_of_file %zu\n",
                       __func__, __LINE__, pc->orig_line);
               chunk_del(pc);
               MARK_CHANGE();
            }
            else if (  cpd.settings[UO_nl_start_of_file].a == AV_FORCE
                    || (pc->nl_count < cpd.settings[UO_nl_start_of_file_min].u))
            {
               LOG_FMT(LBLANKD, "%s(%d): set_blanks_start_of_file %zu\n",
                       __func__, __LINE__, pc->orig_line);
               pc->nl_count = cpd.settings[UO_nl_start_of_file_min].u;
               MARK_CHANGE();
            }
         }
         else if (  (cpd.settings[UO_nl_start_of_file].a & AV_ADD)
                 && (cpd.settings[UO_nl_start_of_file_min].u > 0))
         {
            chunk_t chunk;
            chunk.orig_line = pc->orig_line;
            chunk.type      = CT_NEWLINE;
            chunk.nl_count  = cpd.settings[UO_nl_start_of_file_min].u;
            chunk_add_before(&chunk, pc);
            LOG_FMT(LNEWLINE, "%s(%d): %zu:%zu add newline before '%s'\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
            MARK_CHANGE();
         }
      }
   }

   // Process newlines at the end of the file
   if (  cpd.frag_cols == 0
      && (  (cpd.settings[UO_nl_end_of_file].a & AV_REMOVE)
         || (  (cpd.settings[UO_nl_end_of_file].a & AV_ADD)
            && (cpd.settings[UO_nl_end_of_file_min].u > 0))))
   {
      pc = chunk_get_tail();
      if (pc != nullptr)
      {
         if (pc->type == CT_NEWLINE)
         {
            if (cpd.settings[UO_nl_end_of_file].a == AV_REMOVE)
            {
               LOG_FMT(LBLANKD, "%s(%d): eat_blanks_end_of_file %zu\n",
                       __func__, __LINE__, pc->orig_line);
               chunk_del(pc);
               MARK_CHANGE();
            }
            else if (  cpd.settings[UO_nl_end_of_file].a == AV_FORCE
                    || (pc->nl_count < cpd.settings[UO_nl_end_of_file_min].u))
            {
               if (pc->nl_count != cpd.settings[UO_nl_end_of_file_min].u)
               {
                  LOG_FMT(LBLANKD, "%s(%d): set_blanks_end_of_file %zu\n",
                          __func__, __LINE__, pc->orig_line);
                  pc->nl_count = cpd.settings[UO_nl_end_of_file_min].u;
                  MARK_CHANGE();
               }
            }
         }
         else if (  (cpd.settings[UO_nl_end_of_file].a & AV_ADD)
                 && (cpd.settings[UO_nl_end_of_file_min].u > 0))
         {
            chunk_t chunk;
            chunk.orig_line = pc->orig_line;
            chunk.type      = CT_NEWLINE;
            chunk.nl_count  = cpd.settings[UO_nl_end_of_file_min].u;
            chunk_add_before(&chunk, nullptr);
            LOG_FMT(LNEWLINE, "%s(%d): %zu:%zu add newline before '%s'\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
            MARK_CHANGE();
         }
      }
   }
} // newlines_eat_start_end


void newlines_chunk_pos(c_token_t chunk_type, tokenpos_e mode)
{
   LOG_FUNC_ENTRY();

   if (  (mode & (TP_JOIN | TP_LEAD | TP_TRAIL)) == 0
      && chunk_type != CT_COMMA)
   {
      return;
   }

   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next_ncnl(pc))
   {
      if (pc->type == chunk_type)
      {
         tokenpos_e mode_local;
         if (chunk_type == CT_COMMA)
         {
            /*
             * for chunk_type == CT_COMMA
             * we get 'mode' from cpd.settings[UO_pos_comma].tp
             * BUT we must take care of cpd.settings[UO_pos_class_comma].tp
             * TODO and cpd.settings[UO_pos_constr_comma].tp
             */
            if (pc->flags & PCF_IN_CLASS_BASE)
            {
               // change mode
               mode_local = cpd.settings[UO_pos_class_comma].tp;
            }
            else if (pc->flags & PCF_IN_ENUM)
            {
               mode_local = cpd.settings[UO_pos_enum_comma].tp;
            }
            else
            {
               mode_local = mode;
            }
         }
         else
         {
            mode_local = mode;
         }
         chunk_t *prev = chunk_get_prev_nc(pc);
         chunk_t *next = chunk_get_next_nc(pc);

         size_t  nl_flag = ((chunk_is_newline(prev) ? 1 : 0) |
                            (chunk_is_newline(next) ? 2 : 0));

         if (mode_local & TP_JOIN)
         {
            if (nl_flag & 1)
            {
               // remove newline if not preceded by a comment
               chunk_t *prev2 = chunk_get_prev(prev);

               if (prev2 != nullptr && !(chunk_is_comment(prev2)))
               {
                  remove_next_newlines(prev2);
               }
            }
            if (nl_flag & 2)
            {
               // remove newline if not followed by a comment
               chunk_t *next2 = chunk_get_next(next);

               if (next2 != nullptr && !(chunk_is_comment(next2)))
               {
                  remove_next_newlines(pc);
               }
            }
            continue;
         }

         if (  (nl_flag == 0 && ((mode_local & (TP_FORCE | TP_BREAK)) == 0))
            || (nl_flag == 3 && ((mode_local & TP_FORCE) == 0)))
         {
            // No newlines and not adding any or both and not forcing
            continue;
         }

         if (  ((mode_local & TP_LEAD) && nl_flag == 1)
            || ((mode_local & TP_TRAIL) && nl_flag == 2))
         {
            // Already a newline before (lead) or after (trail)
            continue;
         }

         // If there were no newlines, we need to add one
         if (nl_flag == 0)
         {
            if (mode_local & TP_LEAD)
            {
               newline_add_before(pc);
            }
            else
            {
               newline_add_after(pc);
            }
            continue;
         }

         // If there were both newlines, we need to remove one
         if (nl_flag == 3)
         {
            if (mode_local & TP_LEAD)
            {
               remove_next_newlines(pc);
            }
            else
            {
               remove_next_newlines(chunk_get_prev_ncnl(pc));
            }
            continue;
         }

         // we need to move the newline
         if (mode_local & TP_LEAD)
         {
            chunk_t *next2 = chunk_get_next(next);
            if (  next2 != nullptr
               && (  next2->type == CT_PREPROC
                  || (  chunk_type == CT_ASSIGN
                     && next2->type == CT_BRACE_OPEN)))
            {
               continue;
            }
            if (next->nl_count == 1)
            {
               // move the CT_BOOL to after the newline
               chunk_move_after(pc, next);
            }
         }
         else
         {
            if (prev->nl_count == 1)
            {
               // Back up to the next non-comment item
               prev = chunk_get_prev_nc(prev);
               if (  prev != nullptr
                  && !chunk_is_newline(prev)
                  && !(prev->flags & PCF_IN_PREPROC))
               {
                  chunk_move_after(pc, prev);
               }
            }
         }
      }
   }
} // newlines_chunk_pos


void newlines_class_colon_pos(c_token_t tok)
{
   LOG_FUNC_ENTRY();

   tokenpos_e tpc, pcc;
   argval_t   anc, ncia;

   if (tok == CT_CLASS_COLON)
   {
      tpc  = cpd.settings[UO_pos_class_colon].tp;
      anc  = cpd.settings[UO_nl_class_colon].a;
      ncia = cpd.settings[UO_nl_class_init_args].a;
      pcc  = cpd.settings[UO_pos_class_comma].tp;
   }
   else // tok == CT_CONSTR_COLON
   {
      tpc  = cpd.settings[UO_pos_constr_colon].tp;
      anc  = cpd.settings[UO_nl_constr_colon].a;
      ncia = cpd.settings[UO_nl_constr_init_args].a;
      pcc  = cpd.settings[UO_pos_constr_comma].tp;
   }

   chunk_t *ccolon = nullptr;
   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next_ncnl(pc))
   {
      if (!ccolon && pc->type != tok)
      {
         continue;
      }

      chunk_t *prev;
      chunk_t *next;
      if (pc->type == tok)
      {
         ccolon = pc;
         prev   = chunk_get_prev_nc(pc);
         next   = chunk_get_next_nc(pc);

         if (  !chunk_is_newline(prev)
            && !chunk_is_newline(next)
            && ((anc & AV_ADD) != 0))
         {
            newline_add_after(pc);
            prev = chunk_get_prev_nc(pc);
            next = chunk_get_next_nc(pc);
         }

         if (anc == AV_REMOVE)
         {
            if (chunk_is_newline(prev) && chunk_safe_to_del_nl(prev))
            {
               chunk_del(prev);
               MARK_CHANGE();
               prev = chunk_get_prev_nc(pc);
            }
            if (chunk_is_newline(next) && chunk_safe_to_del_nl(next))
            {
               chunk_del(next);
               MARK_CHANGE();
               next = chunk_get_next_nc(pc);
            }
         }

         if (tpc & TP_TRAIL)
         {
            if (  chunk_is_newline(prev)
               && prev->nl_count == 1
               && chunk_safe_to_del_nl(prev))
            {
               chunk_swap(pc, prev);
            }
         }
         else if (tpc & TP_LEAD)
         {
            if (  chunk_is_newline(next)
               && next->nl_count == 1
               && chunk_safe_to_del_nl(next))
            {
               chunk_swap(pc, next);
            }
         }
      }
      else
      {
         if (pc->type == CT_BRACE_OPEN || pc->type == CT_SEMICOLON)
         {
            ccolon = nullptr;
            continue;
         }

         if (pc->type == CT_COMMA && pc->level == ccolon->level)
         {
            if ((ncia & AV_ADD) != 0)
            {
               if (pcc & TP_TRAIL)
               {
                  if (ncia == AV_FORCE)
                  {
                     newline_force_after(pc);
                  }
                  else
                  {
                     newline_add_after(pc);
                  }
                  prev = chunk_get_prev_nc(pc);
                  if (chunk_is_newline(prev) && chunk_safe_to_del_nl(prev))
                  {
                     chunk_del(prev);
                     MARK_CHANGE();
                  }
               }
               else if (pcc & TP_LEAD)
               {
                  if (ncia == AV_FORCE)
                  {
                     newline_force_before(pc);
                  }
                  else
                  {
                     newline_add_before(pc);
                  }
                  next = chunk_get_next_nc(pc);
                  if (chunk_is_newline(next) && chunk_safe_to_del_nl(next))
                  {
                     chunk_del(next);
                     MARK_CHANGE();
                  }
               }
            }
            else if (ncia == AV_REMOVE)
            {
               next = chunk_get_next(pc);
               if (chunk_is_newline(next) && chunk_safe_to_del_nl(next))
               {
                  chunk_del(next);
                  MARK_CHANGE();
               }
            }
         }
      }
   }
} // newlines_class_colon_pos


static void _blank_line_max(chunk_t *pc, const char *text, uncrustify_options uo)
{
   LOG_FUNC_ENTRY();
   if (pc == nullptr)
   {
      return;
   }
   const option_map_value *option = get_option_name(uo);
   if (option->type != AT_UNUM)
   {
      fprintf(stderr, "Program error for UO_=%d\n", static_cast<int>(uo));
      fprintf(stderr, "Please make a report\n");
      log_flush(true);
      exit(2);
   }
   if ((cpd.settings[uo].u > 0) && (pc->nl_count > cpd.settings[uo].u))
   {
      LOG_FMT(LBLANKD, "%s(%d): do_blank_lines: %s max line %zu\n",
              __func__, __LINE__, text + 3, pc->orig_line);
      pc->nl_count = cpd.settings[uo].u;
      MARK_CHANGE();
   }
}


#define blank_line_max(pc, op)    _blank_line_max(pc, # op, op)


void do_blank_lines(void)
{
   LOG_FUNC_ENTRY();

   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next(pc))
   {
      if (pc->type == CT_NEWLINE)
      {
         LOG_FMT(LBLANKD, "%s(%d): orig_line is %zu, orig_col is %zu, <Newline>, nl is %zu\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->nl_count);
      }
      else
      {
         LOG_FMT(LBLANKD, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s', type is %s\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
      }
      bool line_added = false;

      if (pc->type != CT_NEWLINE)
      {
         continue;
      }

      chunk_t *prev = chunk_get_prev_nc(pc);
      if (prev != nullptr)
      {
         LOG_FMT(LBLANK, "%s(%d): prev->orig_line is %zu, prev->text() '%s', prev->type is %s\n",
                 __func__, __LINE__, pc->orig_line,
                 prev->text(), get_token_name(prev->type));
         if (prev->type == CT_IGNORED)
         {
            continue;
         }
      }

      chunk_t *next  = chunk_get_next(pc);
      chunk_t *pcmt  = chunk_get_prev(pc);
      size_t  old_nl = pc->nl_count;

      /*
       * If this is the first or the last token, pretend that there is an extra
       * line. It will be removed at the end.
       */
      if (pc == chunk_get_head() || next == nullptr)
      {
         line_added = true;
         ++pc->nl_count;
      }

      // Limit consecutive newlines
      if (  (cpd.settings[UO_nl_max].u > 0)
         && (pc->nl_count > cpd.settings[UO_nl_max].u))
      {
         blank_line_max(pc, UO_nl_max);
      }

      if (!can_increase_nl(pc))
      {
         LOG_FMT(LBLANKD, "%s(%d): force to 1 orig_line is %zu, orig_col is %zu\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col);
         if (pc->nl_count != 1)
         {
            pc->nl_count = 1;
            MARK_CHANGE();
         }
         continue;
      }

      // Control blanks before multi-line comments
      if (  (cpd.settings[UO_nl_before_block_comment].u > pc->nl_count)
         && chunk_is_token(next, CT_COMMENT_MULTI))
      {
         // Don't add blanks after a open brace
         if (  prev == nullptr
            || (prev->type != CT_BRACE_OPEN && prev->type != CT_VBRACE_OPEN))
         {
            blank_line_set(pc, UO_nl_before_block_comment);
         }
      }

      // Control blanks before single line C comments
      if (  (cpd.settings[UO_nl_before_c_comment].u > pc->nl_count)
         && chunk_is_token(next, CT_COMMENT))
      {
         // Don't add blanks after a open brace or a comment
         if (  prev == nullptr
            || (  prev->type != CT_BRACE_OPEN
               && prev->type != CT_VBRACE_OPEN
               && pcmt != nullptr
               && pcmt->type != CT_COMMENT))
         {
            blank_line_set(pc, UO_nl_before_c_comment);
         }
      }

      // Control blanks before CPP comments
      if (  (cpd.settings[UO_nl_before_cpp_comment].u > pc->nl_count)
         && chunk_is_token(next, CT_COMMENT_CPP))
      {
         // Don't add blanks after a open brace
         if (  prev == nullptr
            || (  prev->type != CT_BRACE_OPEN
               && prev->type != CT_VBRACE_OPEN
               && pcmt != nullptr
               && pcmt->type != CT_COMMENT_CPP))
         {
            blank_line_set(pc, UO_nl_before_cpp_comment);
         }
      }

      // Control blanks before an access spec
      if (  (cpd.settings[UO_nl_before_access_spec].u > 0)
         && (cpd.settings[UO_nl_before_access_spec].u != pc->nl_count)
         && chunk_is_token(next, CT_PRIVATE))
      {
         // Don't add blanks after a open brace
         if (  prev == nullptr
            || (prev->type != CT_BRACE_OPEN && prev->type != CT_VBRACE_OPEN))
         {
            blank_line_set(pc, UO_nl_before_access_spec);
         }
      }

      // Control blanks before a class
      if (  prev != nullptr
         && (prev->type == CT_SEMICOLON || prev->type == CT_BRACE_CLOSE)
         && prev->parent_type == CT_CLASS)
      {
         chunk_t *tmp = chunk_get_prev_type(prev, CT_CLASS, prev->level);
         tmp = chunk_get_prev_nc(tmp);
         if (cpd.settings[UO_nl_before_class].u > pc->nl_count)
         {
            blank_line_set(tmp, UO_nl_before_class);
         }
      }

      // Control blanks after an access spec
      if (  (cpd.settings[UO_nl_after_access_spec].u > 0)
         && (cpd.settings[UO_nl_after_access_spec].u != pc->nl_count)
         && chunk_is_token(prev, CT_PRIVATE_COLON))
      {
         blank_line_set(pc, UO_nl_after_access_spec);
      }

      // Add blanks after function bodies
      if (  chunk_is_token(prev, CT_BRACE_CLOSE)
         && (  prev->parent_type == CT_FUNC_DEF
            || prev->parent_type == CT_FUNC_CLASS_DEF
            || prev->parent_type == CT_OC_MSG_DECL
            || prev->parent_type == CT_ASSIGN))
      {
         if (prev->flags & PCF_ONE_LINER)
         {
            if (cpd.settings[UO_nl_after_func_body_one_liner].u > pc->nl_count)
            {
               blank_line_set(pc, UO_nl_after_func_body_one_liner);
            }
         }
         else
         {
            if (  (prev->flags & PCF_IN_CLASS)
               && (cpd.settings[UO_nl_after_func_body_class].u > 0))
            {
               if (cpd.settings[UO_nl_after_func_body_class].u != pc->nl_count)
               {
                  blank_line_set(pc, UO_nl_after_func_body_class);
               }
            }
            else if (cpd.settings[UO_nl_after_func_body].u > 0)
            {
               if (cpd.settings[UO_nl_after_func_body].u != pc->nl_count)
               {
                  blank_line_set(pc, UO_nl_after_func_body);
               }
            }
         }
      }

      // Add blanks after function prototypes
      if (  chunk_is_token(prev, CT_SEMICOLON)
         && prev->parent_type == CT_FUNC_PROTO)
      {
         if (cpd.settings[UO_nl_after_func_proto].u > pc->nl_count)
         {
            pc->nl_count = cpd.settings[UO_nl_after_func_proto].u;
            MARK_CHANGE();
         }
         if (  (cpd.settings[UO_nl_after_func_proto_group].u > pc->nl_count)
            && next != nullptr
            && next->parent_type != CT_FUNC_PROTO)
         {
            blank_line_set(pc, UO_nl_after_func_proto_group);
         }
      }

      // Issue #411: Add blanks after function class prototypes
      if (  chunk_is_token(prev, CT_SEMICOLON)
         && prev->parent_type == CT_FUNC_CLASS_PROTO)
      {
         if (cpd.settings[UO_nl_after_func_class_proto].u > pc->nl_count)
         {
            pc->nl_count = cpd.settings[UO_nl_after_func_class_proto].u;
            MARK_CHANGE();
         }
         if (  (cpd.settings[UO_nl_after_func_class_proto_group].u > pc->nl_count)
            && next != nullptr
            && next->parent_type != CT_FUNC_CLASS_PROTO)
         {
            blank_line_set(pc, UO_nl_after_func_class_proto_group);
         }
      }

      // Add blanks after struct/enum/union/class
      if (  prev != nullptr
         && (prev->type == CT_SEMICOLON || prev->type == CT_BRACE_CLOSE)
         && (  prev->parent_type == CT_STRUCT
            || prev->parent_type == CT_ENUM
            || prev->parent_type == CT_UNION
            || prev->parent_type == CT_CLASS))
      {
         if (prev->parent_type == CT_CLASS)
         {
            if (cpd.settings[UO_nl_after_class].u > pc->nl_count)
            {
               blank_line_set(pc, UO_nl_after_class);
            }
         }
         else
         {
            if (cpd.settings[UO_nl_after_struct].u > pc->nl_count)
            {
               blank_line_set(pc, UO_nl_after_struct);
            }
         }
      }

      // Change blanks between a function comment and body
      if (  (cpd.settings[UO_nl_comment_func_def].u != 0)
         && chunk_is_token(pcmt, CT_COMMENT_MULTI)
         && pcmt->parent_type == CT_COMMENT_WHOLE
         && next != nullptr
         && (  next->parent_type == CT_FUNC_DEF
            || next->parent_type == CT_FUNC_CLASS_DEF))
      {
         if (cpd.settings[UO_nl_comment_func_def].u != pc->nl_count)
         {
            blank_line_set(pc, UO_nl_comment_func_def);
         }
      }

      // Change blanks after a try-catch-finally block
      if (  (cpd.settings[UO_nl_after_try_catch_finally].u != 0)
         && (cpd.settings[UO_nl_after_try_catch_finally].u != pc->nl_count)
         && prev != nullptr
         && next != nullptr)
      {
         if (  prev->type == CT_BRACE_CLOSE
            && (  prev->parent_type == CT_CATCH
               || prev->parent_type == CT_FINALLY))
         {
            if (  next->type != CT_BRACE_CLOSE
               && next->type != CT_CATCH
               && next->type != CT_FINALLY)
            {
               blank_line_set(pc, UO_nl_after_try_catch_finally);
            }
         }
      }

      // Change blanks after a try-catch-finally block
      if (  (cpd.settings[UO_nl_between_get_set].u != 0)
         && (cpd.settings[UO_nl_between_get_set].u != pc->nl_count)
         && prev != nullptr
         && next != nullptr)
      {
         if (  prev->parent_type == CT_GETSET
            && next->type != CT_BRACE_CLOSE
            && (prev->type == CT_BRACE_CLOSE || prev->type == CT_SEMICOLON))
         {
            blank_line_set(pc, UO_nl_between_get_set);
         }
      }

      // Change blanks after a try-catch-finally block
      if (  (cpd.settings[UO_nl_around_cs_property].u != 0)
         && (cpd.settings[UO_nl_around_cs_property].u != pc->nl_count)
         && prev != nullptr
         && next != nullptr)
      {
         if (  prev->type == CT_BRACE_CLOSE
            && prev->parent_type == CT_CS_PROPERTY
            && next->type != CT_BRACE_CLOSE)
         {
            blank_line_set(pc, UO_nl_around_cs_property);
         }
         else if (  next->parent_type == CT_CS_PROPERTY
                 && (next->flags & PCF_STMT_START))
         {
            blank_line_set(pc, UO_nl_around_cs_property);
         }
      }

      if (line_added && pc->nl_count > 1)
      {
         --pc->nl_count;
      }

      if (old_nl != pc->nl_count)
      {
         LOG_FMT(LBLANK, "   -=> changed to %zu\n", pc->nl_count);
      }
   }
} // do_blank_lines


void newlines_cleanup_dup(void)
{
   LOG_FUNC_ENTRY();

   chunk_t *pc   = chunk_get_head();
   chunk_t *next = pc;

   while (pc != nullptr)
   {
      next = chunk_get_next(next);
      if (pc->type == CT_NEWLINE && chunk_is_token(next, CT_NEWLINE))
      {
         next->nl_count = max(pc->nl_count, next->nl_count);
         chunk_del(pc);
         MARK_CHANGE();
      }
      pc = next;
   }
}


static void newlines_enum_entries(chunk_t *open_brace, argval_t av)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc = open_brace;

   while (  (pc = chunk_get_next_nc(pc)) != nullptr
         && pc->level > open_brace->level)
   {
      if ((pc->level != (open_brace->level + 1)) || pc->type != CT_COMMA)
      {
         continue;
      }

      newline_iarf(pc, av);
   }

   newline_iarf(open_brace, av);
}


static void newlines_double_space_struct_enum_union(chunk_t *open_brace)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc = open_brace;

   while (  (pc = chunk_get_next_nc(pc)) != nullptr
         && pc->level > open_brace->level)
   {
      if (pc->level != (open_brace->level + 1) || pc->type != CT_NEWLINE)
      {
         continue;
      }

      /*
       * If the newline is NOT after a comment or a brace open and
       * it is before a comment, then make sure that the newline is
       * at least doubled
       */
      chunk_t *prev = chunk_get_prev(pc);
      if (  !chunk_is_comment(prev)
         && prev->type != CT_BRACE_OPEN
         && chunk_is_comment(chunk_get_next(pc)))
      {
         if (pc->nl_count < 2)
         {
            double_newline(pc);
         }
      }
   }
}


void annotations_newlines(void)
{
   LOG_FUNC_ENTRY();

   chunk_t *next;
   chunk_t *ae;   // last token of the annotation
   chunk_t *pc = chunk_get_head();
   while (  (pc = chunk_get_next_type(pc, CT_ANNOTATION, -1)) != nullptr
         && (next = chunk_get_next_nnl(pc)) != nullptr)
   {
      // find the end of this annotation
      if (chunk_is_paren_open(next))
      {
         // TODO: control newline between annotation and '(' ?
         ae = chunk_skip_to_match(next);
      }
      else
      {
         ae = pc;
      }
      if (!ae)
      {
         break;
      }

      LOG_FMT(LANNOT, "%s(%d): %zu:%zu annotation '%s' end@%zu:%zu '%s'",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(),
              ae->orig_line, ae->orig_col, ae->text());

      next = chunk_get_next_nnl(ae);
      if (chunk_is_token(next, CT_ANNOTATION))
      {
         LOG_FMT(LANNOT, " -- nl_between_annotation\n");
         newline_iarf(ae, cpd.settings[UO_nl_between_annotation].a);
      }
      else
      {
         LOG_FMT(LANNOT, " -- nl_after_annotation\n");
         newline_iarf(ae, cpd.settings[UO_nl_after_annotation].a);
      }
   }
} // annotations_newlines


bool newlines_between(chunk_t *pc_start, chunk_t *pc_end, size_t &newlines, scope_e scope)
{
   if (pc_start == nullptr || pc_end == nullptr)
   {
      return(false);
   }

   newlines = 0;

   auto it = pc_start;
   for ( ; it != nullptr && it != pc_end; it = chunk_get_next(it, scope))
   {
      newlines += it->nl_count;
   }

   // newline count is valid if search stopped on expected chunk
   return(it == pc_end);
}
