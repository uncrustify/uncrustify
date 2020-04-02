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

#include "align_stack.h"
#include "chunk_list.h"
#include "combine.h"
#include "flag_parens.h"
#include "indent.h"
#include "keywords.h"
#include "language_tools.h"
#include "log_rules.h"
#include "options.h"
#include "prototypes.h"
#include "space.h"
#include "unc_ctype.h"
#include "uncrustify.h"
#include "uncrustify_types.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>


using namespace std;
using namespace uncrustify;


static void mark_change(const char *func, size_t line);


/**
 * Check to see if we are allowed to increase the newline count.
 * We can't increase the newline count:
 *  - if nl_squeeze_ifdef and a preproc is after the newline.
 *  - if eat_blanks_before_close_brace and the next is '}'
 *  - if eat_blanks_after_open_brace and the prev is '{'
 *    - unless the brace belongs to a namespace
 *      and nl_inside_namespace is non-zero
 */
static bool can_increase_nl(chunk_t *nl);


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
static void newlines_enum_entries(chunk_t *open_brace, iarf_e av);


/**
 * Checks to see if it is OK to add a newline around the chunk.
 * Don't want to break one-liners...
 * return value:
 *  true: a new line may be added
 * false: a new line may NOT be added
 */
static bool one_liner_nl_ok(chunk_t *pc);


static void nl_create_one_liner(chunk_t *vbrace_open);


static void nl_create_list_liner(chunk_t *brace_open);


/**
 * Test if a chunk belongs to a one-liner method definition inside a class body
 */
static bool is_class_one_liner(chunk_t *pc);


/**
 * Test if a chunk may be combined with a function prototype group.
 *
 * If nl_class_leave_one_liner_groups is enabled, a chunk may be combined with
 * a function prototype group if it is a one-liner inside a class body, and is
 * a definition of the same sort as surrounding prototypes. This checks against
 * either the function name, or the function closing brace.
 */
bool is_func_proto_group(chunk_t *pc, c_token_t one_liner_type);


//! Find the next newline or nl_cont
static void nl_handle_define(chunk_t *pc);


/**
 * Does the Ignore, Add, Remove, or Force thing between two chunks
 *
 * @param before  The first chunk
 * @param after   The second chunk
 * @param av      The IARF value
 */
static void newline_iarf_pair(chunk_t *before, chunk_t *after, iarf_e av, bool check_nl_assign_leave_one_liners = false);


/**
 * Adds newlines to multi-line function call/decl/def
 * Start points to the open paren
 */
static void newline_func_multi_line(chunk_t *start);


static void newline_template(chunk_t *start);


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
static bool newlines_if_for_while_switch(chunk_t *start, iarf_e nl_opt);


/**
 * Add or remove extra newline before the chunk.
 * Adds before comments
 * Doesn't do anything if open brace before it
 * "code\n\ncomment\nif (...)" or "code\ncomment\nif (...)"
 */
static void newlines_if_for_while_switch_pre_blank_lines(chunk_t *start, iarf_e nl_opt);


static void blank_line_set(chunk_t *pc, Option<unsigned> &opt);


/**
 * Add one/two newline(s) before the chunk.
 * Adds before comments
 * Adds before destructor
 * Doesn't do anything if open brace before it
 * "code\n\ncomment\nif (...)" or "code\ncomment\nif (...)"
 */
static void newlines_func_pre_blank_lines(chunk_t *start, c_token_t start_type);


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
static void newlines_if_for_while_switch_post_blank_lines(chunk_t *start, iarf_e nl_opt);


/**
 * Adds or removes a newline between the keyword and the open brace.
 * If there is something after the '{' on the same line, then
 * the newline is removed unconditionally.
 * If there is a '=' between the keyword and '{', do nothing.
 *
 * "struct [name] {" or "struct [name] \n {"
 */
static void newlines_struct_union(chunk_t *start, iarf_e nl_opt, bool leave_trailing);
static void newlines_enum(chunk_t *start);
static void newlines_namespace(chunk_t *start); // Issue #2186


/**
 * Cuddles or un-cuddles a chunk with a previous close brace
 *
 * "} while" vs "} \n while"
 * "} else" vs "} \n else"
 *
 * @param start  The chunk - should be CT_ELSE or CT_WHILE_OF_DO
 */
static void newlines_cuddle_uncuddle(chunk_t *start, iarf_e nl_opt);


/**
 * Adds/removes a newline between else and '{'.
 * "else {" or "else \n {"
 */
static void newlines_do_else(chunk_t *start, iarf_e nl_opt);


//! Check if token starts a variable declaration
static bool is_var_def(chunk_t *pc, chunk_t *next);


//! Put newline(s) before and/or after a block of variable definitions
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


static void blank_line_max(chunk_t *pc, Option<unsigned> &opt);


static iarf_e newline_template_option(chunk_t *pc, iarf_e special, iarf_e base, iarf_e fallback);


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

   if (options::nl_squeeze_ifdef())
   {
      log_rule_B("nl_squeeze_ifdef");

      if (  chunk_is_token(prev, CT_PREPROC)
         && get_chunk_parent_type(prev) == CT_PP_ENDIF
         && (prev->level > 0 || options::nl_squeeze_ifdef_top_level()))
      {
         log_rule_B("nl_squeeze_ifdef_top_level");
         LOG_FMT(LBLANKD, "%s(%d): nl_squeeze_ifdef %zu (prev) pp_lvl=%zu rv=0\n",
                 __func__, __LINE__, nl->orig_line, nl->pp_level);
         return(false);
      }

      if (  chunk_is_token(next, CT_PREPROC)
         && get_chunk_parent_type(next) == CT_PP_ENDIF
         && (next->level > 0 || options::nl_squeeze_ifdef_top_level()))
      {
         bool rv = ifdef_over_whole_file() && next->flags.test(PCF_WF_ENDIF);
         LOG_FMT(LBLANKD, "%s(%d): nl_squeeze_ifdef %zu (next) pp_lvl=%zu rv=%d\n",
                 __func__, __LINE__, nl->orig_line, nl->pp_level, rv);
         return(rv);
      }
   }

   if (chunk_is_token(next, CT_BRACE_CLOSE))
   {
      if (options::nl_inside_namespace() && get_chunk_parent_type(next) == CT_NAMESPACE)
      {
         log_rule_B("nl_inside_namespace");
         LOG_FMT(LBLANKD, "%s(%d): nl_inside_namespace %zu\n",
                 __func__, __LINE__, nl->orig_line);
         return(true);
      }

      if (options::eat_blanks_before_close_brace())
      {
         log_rule_B("eat_blanks_before_close_brace");
         LOG_FMT(LBLANKD, "%s(%d): eat_blanks_before_close_brace %zu\n",
                 __func__, __LINE__, nl->orig_line);
         return(false);
      }
   }

   if (chunk_is_token(prev, CT_BRACE_OPEN))
   {
      if (options::nl_inside_namespace() && get_chunk_parent_type(prev) == CT_NAMESPACE)
      {
         log_rule_B("nl_inside_namespace");
         LOG_FMT(LBLANKD, "%s(%d): nl_inside_namespace %zu\n",
                 __func__, __LINE__, nl->orig_line);
         return(true);
      }

      if (options::eat_blanks_after_open_brace())
      {
         log_rule_B("eat_blanks_after_open_brace");
         LOG_FMT(LBLANKD, "%s(%d): eat_blanks_after_open_brace %zu\n",
                 __func__, __LINE__, nl->orig_line);
         return(false);
      }
   }
   log_rule_B("nl_start_of_file");

   if (!pcmt && (options::nl_start_of_file() != IARF_IGNORE))
   {
      LOG_FMT(LBLANKD, "%s(%d): SOF no prev %zu\n", __func__, __LINE__, nl->orig_line);
      return(false);
   }
   log_rule_B("nl_end_of_file");

   if (!next && (options::nl_end_of_file() != IARF_IGNORE))
   {
      LOG_FMT(LBLANKD, "%s(%d): EOF no next %zu\n", __func__, __LINE__, nl->orig_line);
      return(false);
   }
   return(true);
} // can_increase_nl


static void setup_newline_add(chunk_t *prev, chunk_t *nl, chunk_t *next)
{
   LOG_FUNC_ENTRY();

   if (  prev == nullptr
      || nl == nullptr
      || next == nullptr)
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

   if (  prev->flags.test(PCF_IN_PREPROC)
      && next->flags.test(PCF_IN_PREPROC))
   {
      chunk_flags_set(nl, PCF_IN_PREPROC);
   }

   if (nl->flags.test(PCF_IN_PREPROC))
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


void double_newline(chunk_t *nl)
{
   LOG_FUNC_ENTRY();
   chunk_t *prev = chunk_get_prev(nl);

   if (prev == nullptr)
   {
      return;
   }
   LOG_FMT(LNEWLINE, "%s(%d): add newline after ", __func__, __LINE__);

   if (chunk_is_token(prev, CT_VBRACE_CLOSE))
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
   LOG_FMT(LNEWLINE, "%s(%d): text() '%s', on orig_line is %zu, orig_col is %zu, pc->column is %zu",
           __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col, pc->column);
   log_func_stack_inline(LNEWLINE);

   setup_newline_add(prev, &nl, pc);
   nl.orig_col = pc->orig_col;
   LOG_FMT(LNEWLINE, "%s(%d): nl.column is %zu\n",
           __func__, __LINE__, nl.column);

   MARK_CHANGE();
   return(chunk_add_before(&nl, pc));
}


chunk_t *newline_force_before(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *nl = newline_add_before(pc);

   if (nl != nullptr && nl->nl_count > 1)
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

   nl.orig_line = pc->orig_line;
   nl.orig_col  = pc->orig_col;
   setup_newline_add(pc, &nl, next);

   MARK_CHANGE();
   // TO DO: check why the next statement is necessary
   nl.orig_col = pc->orig_col;
   return(chunk_add_after(&nl, pc));
}


chunk_t *newline_force_after(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *nl = newline_add_after(pc);   // add a newline

   if (nl != nullptr && nl->nl_count > 1) // check if there are more than 1 newline
   {
      nl->nl_count = 1;                   // if so change the newline count back to 1
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
      nl.orig_col  = br_close->orig_col;
      nl.nl_count  = 1;
      nl.flags     = (br_close->flags & PCF_COPY_FLAGS) & ~PCF_IN_PREPROC;

      if (  br_close->flags.test(PCF_IN_PREPROC)
         && next != nullptr
         && next->flags.test(PCF_IN_PREPROC))
      {
         nl.flags |= PCF_IN_PREPROC;
      }

      if (nl.flags.test(PCF_IN_PREPROC))
      {
         set_chunk_type(&nl, CT_NL_CONT);
         nl.str = "\\\n";
      }
      else
      {
         set_chunk_type(&nl, CT_NEWLINE);
         nl.str = "\n";
      }
      MARK_CHANGE();
      LOG_FMT(LNEWLINE, "%s(%d): %zu:%zu add newline after '%s'\n",
              __func__, __LINE__, br_close->orig_line, br_close->orig_col, br_close->text());
      chunk_add_after(&nl, br_close);
   }
}


static void newline_min_after(chunk_t *ref, size_t count, pcf_flag_e flag)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNEWLINE, "%s(%d): for '%s', at orig_line %zu, count is %zu, flag is %s:",
           __func__, __LINE__, ref->text(), ref->orig_line, count,
           pcf_flags_str(flag).c_str());
   log_func_stack_inline(LNEWLINE);

   chunk_t *pc = ref;

   do
   {
      pc = chunk_get_next(pc);
   } while (pc != nullptr && !chunk_is_newline(pc));

   if (pc != nullptr)                 // Coverity CID 76002
   {
      LOG_FMT(LNEWLINE, "%s(%d): type is %s, orig_line %zu, orig_col %zu\n",
              __func__, __LINE__, get_token_name(pc->type), pc->orig_line, pc->orig_col);
   }
   chunk_t *next = chunk_get_next(pc);

   if (next == nullptr)
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

   if (  start == nullptr
      || end == nullptr
      || chunk_is_token(end, CT_IGNORED))
   {
      return(nullptr);
   }
   LOG_FMT(LNEWLINE, "%s(%d): start->text() is '%s', type is %s, orig_line is %zu, orig_col is %zu\n",
           __func__, __LINE__, start->text(), get_token_name(start->type),
           start->orig_line, start->orig_col);
   LOG_FMT(LNEWLINE, "%s(%d): and end->text() is '%s', orig_line is %zu, orig_col is %zu\n  ",
           __func__, __LINE__, end->text(), end->orig_line, end->orig_col);
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
   if (chunk_is_token(end, CT_BRACE_OPEN))
   {
      chunk_t *pc = chunk_get_next(end);

      if (chunk_is_comment(pc))
      {
         pc = chunk_get_next(pc);

         if (chunk_is_newline(pc))
         {
            // are there some more (comment + newline)s ?
            chunk_t *pc1 = chunk_get_next_ncnl(end);

            if (!chunk_is_newline(pc1))
            {
               // yes, go back
               chunk_t *pc2 = chunk_get_prev(pc1);
               pc = pc2;
            }
         }

         if (end == pc)
         {
            LOG_FMT(LNEWLINE, "%s(%d): pc1 and pc are identical\n",
                    __func__, __LINE__);
         }
         else
         {
            // Move the open brace to after the newline
            chunk_move_after(end, pc);
         }
         LOG_FMT(LNEWLINE, "%s(%d):\n", __func__, __LINE__);
         newline_add_after(end);
         return(pc);
      }
      else
      {
         LOG_FMT(LNEWLINE, "%s(%d):\n", __func__, __LINE__);
      }
   }
   else
   {
      LOG_FMT(LNEWLINE, "%s(%d):\n", __func__, __LINE__);
   }
   chunk_t *tmp = newline_add_before(end);

   return(tmp);
} // newline_add_between


void newline_del_between(chunk_t *start, chunk_t *end)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNEWLINE, "%s(%d): start->text() is '%s', orig_line is %zu, orig_col is %zu\n",
           __func__, __LINE__, start->text(), start->orig_line, start->orig_col);
   LOG_FMT(LNEWLINE, "%s(%d): and end->text() is '%s', orig_line is %zu, orig_col is %zu: preproc=%c/%c\n",
           __func__, __LINE__, end->text(), end->orig_line, end->orig_col,
           start->flags.test(PCF_IN_PREPROC) ? 'y' : 'n',
           end->flags.test(PCF_IN_PREPROC) ? 'y' : 'n');
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
         || chunk_is_token(start, CT_DO)
         || chunk_is_token(start, CT_ELSE)))
   {
      chunk_move_after(end, start);
   }
} // newline_del_between


void newlines_sparens()
{
   LOG_FUNC_ENTRY();

   chunk_t *sparen_open;

   for (sparen_open = chunk_get_next_type(chunk_get_head(), CT_SPAREN_OPEN, ANY_LEVEL);
        sparen_open != nullptr; sparen_open = chunk_get_next_type(
           sparen_open, CT_SPAREN_OPEN, ANY_LEVEL))
   {
      chunk_t *sparen_close = chunk_get_next_type(sparen_open, CT_SPAREN_CLOSE, sparen_open->level);

      if (sparen_close == nullptr)
      {
         continue;
      }
      chunk_t *sparen_content_start = chunk_get_next_nnl(sparen_open);
      chunk_t *sparen_content_end   = chunk_get_prev_nnl(sparen_close);
      bool    is_multiline          = (
         sparen_content_start != sparen_content_end
                                      && !are_chunks_in_same_line(sparen_content_start, sparen_content_end));

      // Add a newline after '(' if an if/for/while/switch condition spans multiple lines,
      // as e.g. required by the ROS 2 development style guidelines:
      // https://index.ros.org/doc/ros2/Contributing/Developer-Guide/#open-versus-cuddled-braces
      if (is_multiline)
      {
         log_rule_B("nl_multi_line_sparen_open");
         newline_iarf(sparen_open, options::nl_multi_line_sparen_open());
      }

      // Add a newline before ')' if an if/for/while/switch condition spans multiple lines. Overrides nl_before_if_closing_paren if both are specified.
      if (is_multiline && options::nl_multi_line_sparen_close() != IARF_IGNORE)
      {
         log_rule_B("nl_multi_line_sparen_close");
         newline_iarf(sparen_content_end, options::nl_multi_line_sparen_close());
      }
      else
      {
         // add/remove trailing newline in an if condition
         chunk_t *ctrl_structure = chunk_get_prev_ncnl(sparen_open);

         if (ctrl_structure->type == CT_IF || ctrl_structure->type == CT_ELSEIF)
         {
            log_rule_B("nl_before_if_closing_paren");
            newline_iarf_pair(sparen_content_end, sparen_close, options::nl_before_if_closing_paren());
         }
      }
   }
} // newlines_sparens


static bool newlines_if_for_while_switch(chunk_t *start, iarf_e nl_opt)
{
   LOG_FUNC_ENTRY();

   log_rule_B("nl_define_macro");

   if (  nl_opt == IARF_IGNORE
      || (  start->flags.test(PCF_IN_PREPROC)
         && !options::nl_define_macro()))
   {
      return(false);
   }
   bool    retval = false;
   chunk_t *pc    = chunk_get_next_ncnl(start);

   if (chunk_is_token(pc, CT_SPAREN_OPEN))
   {
      chunk_t *close_paren = chunk_get_next_type(pc, CT_SPAREN_CLOSE, pc->level);
      chunk_t *brace_open  = chunk_get_next_ncnl(close_paren);

      if (  (  chunk_is_token(brace_open, CT_BRACE_OPEN)
            || chunk_is_token(brace_open, CT_VBRACE_OPEN))
         && one_liner_nl_ok(brace_open))
      {
         log_rule_B("nl_multi_line_cond");

         if (options::nl_multi_line_cond())
         {
            while ((pc = chunk_get_next(pc)) != close_paren)
            {
               if (chunk_is_newline(pc))
               {
                  nl_opt = IARF_ADD;
                  break;
               }
            }
         }

         if (chunk_is_token(brace_open, CT_VBRACE_OPEN))
         {
            // Can only add - we don't want to create a one-line here
            if (nl_opt & IARF_ADD)
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


static void newlines_if_for_while_switch_pre_blank_lines(chunk_t *start, iarf_e nl_opt)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNEWLINE, "%s(%d): start->text() is '%s', type is %s, orig_line is %zu, orig_column is %zu\n",
           __func__, __LINE__, start->text(), get_token_name(start->type), start->orig_line, start->orig_col);

   log_rule_B("nl_define_macro");

   if (  nl_opt == IARF_IGNORE
      || (  start->flags.test(PCF_IN_PREPROC)
         && !options::nl_define_macro()))
   {
      return;
   }
   chunk_t *prev;
   chunk_t *next;
   chunk_t *last_nl = nullptr;
   size_t  level    = start->level;
   bool    do_add   = (nl_opt & IARF_ADD) != IARF_IGNORE; // forcing value to bool

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
            if ((nl_opt & IARF_REMOVE) && !pc->flags.test(PCF_VAR_DEF))
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


static void blank_line_set(chunk_t *pc, Option<unsigned> &opt)
{
   LOG_FUNC_ENTRY();

   if (pc == nullptr)
   {
      return;
   }
   const auto optval = opt();

   if ((optval > 0) && (pc->nl_count != optval))
   {
      LOG_FMT(LBLANKD, "%s(%d): do_blank_lines: %s set line %zu to %u\n",
              __func__, __LINE__, opt.name(), pc->orig_line, optval);
      pc->nl_count = optval;
      MARK_CHANGE();
   }
}


bool do_it_newlines_func_pre_blank_lines(chunk_t *last_nl, c_token_t start_type)
{
   LOG_FUNC_ENTRY();

   if (last_nl == nullptr)
   {
      return(false);
   }
   LOG_FMT(LNLFUNCT, "%s(%d): orig_line is %zu, orig_col is %zu, type is %s, text() is '%s'\n",
           __func__, __LINE__,
           last_nl->orig_line, last_nl->orig_col, get_token_name(last_nl->type), last_nl->text());

   switch (start_type)
   {
   case CT_FUNC_CLASS_DEF:
   {
      log_rule_B("nl_before_func_class_def");
      bool diff = options::nl_before_func_class_def() <= last_nl->nl_count;
      LOG_FMT(LNLFUNCT, "%s(%d): is %s\n",
              __func__, __LINE__, diff ? "TRUE" : "FALSE");

      log_rule_B("nl_before_func_class_def");

      if (options::nl_before_func_class_def() != last_nl->nl_count)
      {
         LOG_FMT(LNLFUNCT, "%s(%d):   set blank line(s) to %u\n",
                 __func__, __LINE__, options::nl_before_func_class_def());
         blank_line_set(last_nl, options::nl_before_func_class_def);
      }
      return(diff);
   }

   case CT_FUNC_CLASS_PROTO:
   {
      log_rule_B("nl_before_func_class_proto");
      bool diff = options::nl_before_func_class_proto() <= last_nl->nl_count;
      LOG_FMT(LNLFUNCT, "%s(%d): is %s\n",
              __func__, __LINE__, diff ? "TRUE" : "FALSE");

      log_rule_B("nl_before_func_class_proto");

      if (options::nl_before_func_class_proto() != last_nl->nl_count)
      {
         LOG_FMT(LNLFUNCT, "%s(%d):   set blank line(s) to %u\n",
                 __func__, __LINE__, options::nl_before_func_class_proto());
         blank_line_set(last_nl, options::nl_before_func_class_proto);
      }
      return(diff);
   }

   case CT_FUNC_DEF:
   {
      LOG_FMT(LNLFUNCT, "%s(%d): nl_before_func_body_def() is %u, last_nl->nl_count is %zu\n",
              __func__, __LINE__, options::nl_before_func_body_def(), last_nl->nl_count);
      log_rule_B("nl_before_func_body_def");
      bool diff = options::nl_before_func_body_def() <= last_nl->nl_count;
      LOG_FMT(LNLFUNCT, "%s(%d): is %s\n",
              __func__, __LINE__, diff ? "TRUE" : "FALSE");

      log_rule_B("nl_before_func_body_def");

      if (options::nl_before_func_body_def() != last_nl->nl_count)
      {
         LOG_FMT(LNLFUNCT, "%s(%d):    set blank line(s) to %u\n",
                 __func__, __LINE__, options::nl_before_func_body_def());
         log_rule_B("nl_before_func_body_def");
         blank_line_set(last_nl, options::nl_before_func_body_def);
      }
      LOG_FMT(LNLFUNCT, "%s(%d): nl_before_func_body_def() is %u, last_nl->nl_count is %zu\n",
              __func__, __LINE__, options::nl_before_func_body_def(), last_nl->nl_count);
      return(diff);
   }

   case CT_FUNC_PROTO:
   {
      log_rule_B("nl_before_func_body_proto");
      bool diff = options::nl_before_func_body_proto() <= last_nl->nl_count;
      LOG_FMT(LNLFUNCT, "%s(%d): is %s\n",
              __func__, __LINE__, diff ? "TRUE" : "FALSE");

      log_rule_B("nl_before_func_body_proto");

      if (options::nl_before_func_body_proto() != last_nl->nl_count)
      {
         LOG_FMT(LNLFUNCT, "%s(%d):   set blank line(s) to %u\n",
                 __func__, __LINE__, options::nl_before_func_body_proto());
         log_rule_B("nl_before_func_body_proto");
         blank_line_set(last_nl, options::nl_before_func_body_proto);
      }
      return(diff);
   }

   default:
   {
      LOG_FMT(LERR, "%s(%d):   setting to blank line(s) at line %zu not possible\n",
              __func__, __LINE__, last_nl->orig_line);
      return(false);
   }
   } // switch
} // do_it_newlines_func_pre_blank_lines


static void newlines_func_pre_blank_lines(chunk_t *start, c_token_t start_type)
{
   LOG_FUNC_ENTRY();

   log_rule_B("nl_before_func_class_def");
   log_rule_B("nl_before_func_class_proto");
   log_rule_B("nl_before_func_body_def");
   log_rule_B("nl_before_func_body_proto");

   if (  start == nullptr
      || (  (  start_type != CT_FUNC_CLASS_DEF
            || options::nl_before_func_class_def() == 0)
         && (  start_type != CT_FUNC_CLASS_PROTO
            || options::nl_before_func_class_proto() == 0)
         && (  start_type != CT_FUNC_DEF
            || options::nl_before_func_body_def() == 0)
         && (  start_type != CT_FUNC_PROTO
            || options::nl_before_func_body_proto() == 0)))
   {
      return;
   }
   LOG_FMT(LNLFUNCT, "%s(%d):    set blank line(s): for <NL> at line %zu, column %zu, start_type is %s\n",
           __func__, __LINE__, start->orig_line, start->orig_col, get_token_name(start_type));
   LOG_FMT(LNLFUNCT, "%s(%d): BEGIN set blank line(s) for '%s' at line %zu\n",
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
   size_t  first_line    = start->orig_line;

   for (pc = chunk_get_prev(start); pc != nullptr; pc = chunk_get_prev(pc))
   {
      LOG_FMT(LNLFUNCT, "%s(%d): orig_line is %zu, orig_col is %zu, type is %s, text() is '%s', nl_count is %zu\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, get_token_name(pc->type), pc->text(), pc->nl_count);

      if (chunk_is_newline(pc))
      {
         last_nl = pc;
         LOG_FMT(LNLFUNCT, "%s(%d):    <chunk_is_newline> found at line %zu, column %zu, nl_count is %zu\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->nl_count);
         LOG_FMT(LNLFUNCT, "%s(%d):    last_nl set to %zu\n",
                 __func__, __LINE__, last_nl->orig_line);
         bool break_now = false;

         if (pc->nl_count > 1)
         {
            break_now = do_it_newlines_func_pre_blank_lines(last_nl, start_type);
            LOG_FMT(LNLFUNCT, "%s(%d): break_now is %s\n",
                    __func__, __LINE__, break_now ? "TRUE" : "FALSE");
         }

         if (break_now)
         {
            break;
         }
         else
         {
            continue;
         }
      }
      else if (chunk_is_comment(pc))
      {
         LOG_FMT(LNLFUNCT, "%s(%d):    <chunk_is_comment> found at line %zu, column %zu\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col);

         if (  (  pc->orig_line < first_line
               && ((first_line - pc->orig_line
                    - (chunk_is_token(pc, CT_COMMENT_MULTI) ? pc->nl_count : 0))) < 2)
            || (  last_comment != nullptr
               && chunk_is_token(pc, CT_COMMENT_CPP) // combine only cpp comments
               && last_comment->type == pc->type     // don't mix comment types
               && last_comment->orig_line > pc->orig_line
               && (last_comment->orig_line - pc->orig_line) < 2))
         {
            last_comment = pc;
            continue;
         }
         bool break_now = do_it_newlines_func_pre_blank_lines(last_nl, start_type);
         LOG_FMT(LNLFUNCT, "%s(%d): break_now is %s\n",
                 __func__, __LINE__, break_now ? "TRUE" : "FALSE");
         continue;
      }
      else if (  chunk_is_token(pc, CT_DESTRUCTOR)
              || chunk_is_token(pc, CT_TYPE)
              || chunk_is_token(pc, CT_TEMPLATE)
              || chunk_is_token(pc, CT_QUALIFIER)
              || chunk_is_token(pc, CT_PTR_TYPE)
              || chunk_is_token(pc, CT_BYREF)                  // Issue #2163
              || chunk_is_token(pc, CT_DC_MEMBER)
              || chunk_is_token(pc, CT_EXTERN)
              || (chunk_is_token(pc, CT_STRING) && get_chunk_parent_type(pc) == CT_EXTERN))
      {
         LOG_FMT(LNLFUNCT, "%s(%d): first_line set to %zu\n",
                 __func__, __LINE__, pc->orig_line);
         first_line = pc->orig_line;
         continue;
      }
      else if (chunk_is_token(pc, CT_ANGLE_CLOSE) && get_chunk_parent_type(pc) == CT_TEMPLATE)
      {
         LOG_FMT(LNLFUNCT, "%s(%d):\n", __func__, __LINE__);
         // skip template stuff to add newlines before it
         pc = chunk_skip_to_match_rev(pc);

         if (pc != nullptr)
         {
            first_line = pc->orig_line;
         }
         continue;
      }
      else
      {
         LOG_FMT(LNLFUNCT, "%s(%d): else ==================================\n",
                 __func__, __LINE__);
         bool break_now = do_it_newlines_func_pre_blank_lines(last_nl, start_type);
         LOG_FMT(LNLFUNCT, "%s(%d): break_now is %s\n",
                 __func__, __LINE__, break_now ? "TRUE" : "FALSE");
         break;
      }
   }
} // newlines_func_pre_blank_lines


static chunk_t *get_closing_brace(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;
   size_t  level = start->level;

   for (pc = start; (pc = chunk_get_next(pc)) != nullptr;)
   {
      if (  (chunk_is_token(pc, CT_BRACE_CLOSE) || chunk_is_token(pc, CT_VBRACE_CLOSE))
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


static void newlines_if_for_while_switch_post_blank_lines(chunk_t *start, iarf_e nl_opt)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;
   chunk_t *next;
   chunk_t *prev;

   LOG_FMT(LNEWLINE, "%s(%d): start->text() is '%s', type is %s, orig_line is %zu, orig_column is %zu\n",
           __func__, __LINE__, start->text(), get_token_name(start->type), start->orig_line, start->orig_col);

   log_rule_B("nl_define_macro");

   if (  nl_opt == IARF_IGNORE
      || (  start->flags.test(PCF_IN_PREPROC)
         && !options::nl_define_macro()))
   {
      return;
   }

   // first find ending brace
   if ((pc = get_closing_brace(start)) == nullptr)
   {
      return;
   }
   LOG_FMT(LNEWLINE, "%s(%d): pc->text() is '%s', type is %s, orig_line is %zu, orig_column is %zu\n",
           __func__, __LINE__, pc->text(), get_token_name(pc->type), pc->orig_line, pc->orig_col);

   /*
    * if we're dealing with an if, we actually want to add or remove
    * blank lines after any else
    */
   if (chunk_is_token(start, CT_IF))
   {
      while (true)
      {
         next = chunk_get_next_ncnl(pc);

         if (  next != nullptr
            && (chunk_is_token(next, CT_ELSE) || chunk_is_token(next, CT_ELSEIF)))
         {
            // point to the closing brace of the else
            if ((pc = get_closing_brace(next)) == nullptr)
            {
               return;
            }
            LOG_FMT(LNEWLINE, "%s(%d): pc->text() is '%s', type %s, orig_line %zu, orig_column %zu\n",
                    __func__, __LINE__, pc->text(), get_token_name(pc->type), pc->orig_line, pc->orig_col);
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
   if (chunk_is_token(start, CT_DO))
   {
      // point to the next semicolon
      if ((pc = chunk_get_next_type(pc, CT_SEMICOLON, start->level)) == nullptr)
      {
         return;
      }
      LOG_FMT(LNEWLINE, "%s(%d): pc->text() is '%s', type %s, orig_line %zu, orig_column %zu\n",
              __func__, __LINE__, pc->text(), get_token_name(pc->type), pc->orig_line, pc->orig_col);
   }
   bool isVBrace = (chunk_is_token(pc, CT_VBRACE_CLOSE));

   if (isVBrace)
   {
      LOG_FMT(LNEWLINE, "%s(%d): isVBrace is TRUE\n", __func__, __LINE__);
   }
   else
   {
      LOG_FMT(LNEWLINE, "%s(%d): isVBrace is FALSE\n", __func__, __LINE__);
   }

   if ((prev = chunk_get_prev_nvb(pc)) == nullptr)
   {
      return;
   }
   bool have_pre_vbrace_nl = isVBrace && chunk_is_newline(prev);

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
              && !next->flags.test(PCF_VAR_DEF))
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
   if (nl_opt & IARF_ADD)
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

      LOG_FMT(LNEWLINE, "%s(%d): next->text() is '%s', type %s, orig_line %zu, orig_column %zu\n",
              __func__, __LINE__, next->text(), get_token_name(next->type), next->orig_line, next->orig_col);

      if (next->type != CT_BRACE_CLOSE)
      {
         // if vbrace, have to check before and after
         // if chunk before vbrace, check its count
         size_t nl_count = have_pre_vbrace_nl ? prev->nl_count : 0;
         LOG_FMT(LNEWLINE, "%s(%d): nl_count %zu\n", __func__, __LINE__, nl_count);

         if (chunk_is_newline(next = chunk_get_next_nvb(pc)))
         {
            LOG_FMT(LNEWLINE, "%s(%d): next->text() is '%s', type %s, orig_line %zu, orig_column %zu\n",
                    __func__, __LINE__, next->text(), get_token_name(next->type), next->orig_line, next->orig_col);
            nl_count += next->nl_count;
            LOG_FMT(LNEWLINE, "%s(%d): nl_count is %zu\n", __func__, __LINE__, nl_count);
         }

         // if we have no newlines, add one and make it double
         if (nl_count == 0)
         {
            LOG_FMT(LNEWLINE, "%s(%d): nl_count is 0\n", __func__, __LINE__);

            if (  ((next = chunk_get_next(pc)) != nullptr)
               && chunk_is_comment(next))
            {
               LOG_FMT(LNEWLINE, "%s(%d): next->text() is '%s', type %s, orig_line %zu, orig_column %zu\n",
                       __func__, __LINE__, next->text(), get_token_name(next->type), next->orig_line, next->orig_col);
               pc = next;
               LOG_FMT(LNEWLINE, "%s(%d): pc->text() is '%s', type %s, orig_line %zu, orig_column %zu\n",
                       __func__, __LINE__, pc->text(), get_token_name(pc->type), pc->orig_line, pc->orig_col);
            }

            if ((next = newline_add_after(pc)) == nullptr)
            {
               return;
            }
            LOG_FMT(LNEWLINE, "%s(%d): next->text() is '%s', type %s, orig_line %zu, orig_column %zu\n",
                    __func__, __LINE__, next->text(), get_token_name(next->type), next->orig_line, next->orig_col);
            double_newline(next);
         }
         else if (nl_count == 1) // if we don't have enough newlines
         {
            LOG_FMT(LNEWLINE, "%s(%d): nl_count is 1\n", __func__, __LINE__);

            // if we have a preceeding vbrace, add one after it
            if (have_pre_vbrace_nl)
            {
               LOG_FMT(LNEWLINE, "%s(%d): have_pre_vbrace_nl is TRUE\n", __func__, __LINE__);
               next = newline_add_after(pc);
               LOG_FMT(LNEWLINE, "%s(%d): next->text() is '%s', type %s, orig_line %zu, orig_column %zu\n",
                       __func__, __LINE__, next->text(), get_token_name(next->type), next->orig_line, next->orig_col);
            }
            else
            {
               LOG_FMT(LNEWLINE, "%s(%d): have_pre_vbrace_nl is FALSE\n", __func__, __LINE__);
               prev = chunk_get_prev_nnl(next);
               LOG_FMT(LNEWLINE, "%s(%d): prev->text() is '%s', type %s, orig_line %zu, orig_column %zu\n",
                       __func__, __LINE__, prev->text(), get_token_name(prev->type), prev->orig_line, prev->orig_col);
               pc = chunk_get_next_nl(next);
               LOG_FMT(LNEWLINE, "%s(%d): pc->text() is '%s', type %s, orig_line %zu, orig_column %zu\n",
                       __func__, __LINE__, pc->text(), get_token_name(pc->type), pc->orig_line, pc->orig_col);
               chunk_t *pc2 = chunk_get_next(pc);

               if (pc2 != nullptr)
               {
                  pc = pc2;
                  LOG_FMT(LNEWLINE, "%s(%d): pc->text() is '%s', type %s, orig_line %zu, orig_column %zu\n",
                          __func__, __LINE__, pc->text(), get_token_name(pc->type), pc->orig_line, pc->orig_col);
               }
               else
               {
                  LOG_FMT(LNEWLINE, "%s(%d): no next found: <EOF>\n", __func__, __LINE__);
               }
               log_rule_B("nl_squeeze_ifdef");

               if (  chunk_is_token(pc, CT_PREPROC)
                  && get_chunk_parent_type(pc) == CT_PP_ENDIF
                  && options::nl_squeeze_ifdef())
               {
                  LOG_FMT(LNEWLINE, "%s(%d): cannot add newline after orig_line %zu due to nl_squeeze_ifdef\n",
                          __func__, __LINE__, prev->orig_line);
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


static void newlines_struct_union(chunk_t *start, iarf_e nl_opt, bool leave_trailing)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;

   log_rule_B("nl_define_macro");

   if (  nl_opt == IARF_IGNORE
      || (  start->flags.test(PCF_IN_PREPROC)
         && !options::nl_define_macro()))
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
         && (  chunk_is_token(pc, CT_BRACE_OPEN)
            || chunk_is_semicolon(pc)
            || chunk_is_token(pc, CT_ASSIGN)))
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
         nl_opt = IARF_IGNORE;
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
   chunk_t *pc;
   chunk_t *pcClass;
   chunk_t *pcType;
   chunk_t *pcColon;
   chunk_t *pcType1;
   chunk_t *pcType2;
   iarf_e  nl_opt;

   log_rule_B("nl_define_macro");

   if (start->flags.test(PCF_IN_PREPROC) && !options::nl_define_macro())
   {
      return;
   }
   // look for 'enum class'
   pcClass = chunk_get_next_ncnl(start);

   if (chunk_is_token(pcClass, CT_ENUM_CLASS))
   {
      log_rule_B("nl_enum_class");
      newline_iarf_pair(start, pcClass, options::nl_enum_class());
      // look for 'identifier'/ 'type'
      pcType = chunk_get_next_ncnl(pcClass);

      if (chunk_is_token(pcType, CT_TYPE))
      {
         log_rule_B("nl_enum_class_identifier");
         newline_iarf_pair(pcClass, pcType, options::nl_enum_class_identifier());
         // look for ':'
         pcColon = chunk_get_next_ncnl(pcType);

         if (chunk_is_token(pcColon, CT_BIT_COLON))
         {
            log_rule_B("nl_enum_identifier_colon");
            newline_iarf_pair(pcType, pcColon, options::nl_enum_identifier_colon());
            // look for 'type' i.e. unsigned
            pcType1 = chunk_get_next_ncnl(pcColon);

            if (chunk_is_token(pcType1, CT_TYPE))
            {
               log_rule_B("nl_enum_colon_type");
               newline_iarf_pair(pcColon, pcType1, options::nl_enum_colon_type());
               // look for 'type' i.e. int
               pcType2 = chunk_get_next_ncnl(pcType1);

               if (chunk_is_token(pcType2, CT_TYPE))
               {
                  log_rule_B("nl_enum_colon_type");
                  newline_iarf_pair(pcType1, pcType2, options::nl_enum_colon_type());
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
         && (  chunk_is_token(pc, CT_BRACE_OPEN)
            || chunk_is_semicolon(pc)
            || chunk_is_token(pc, CT_ASSIGN)))
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
         nl_opt = IARF_IGNORE;
      }
      else
      {
         log_rule_B("nl_enum_brace");
         nl_opt = options::nl_enum_brace();
      }
      newline_iarf_pair(start, pc, nl_opt);
   }
} // newlines_enum


// namespace {
// namespace word {
// namespace type::word {
static void newlines_namespace(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   log_rule_B("nl_namespace_brace");
   iarf_e nl_opt = options::nl_namespace_brace();

   // Add or remove newline between 'namespace' and '{'.

   log_rule_B("nl_define_macro");

   if (  nl_opt == IARF_IGNORE
      || (  start->flags.test(PCF_IN_PREPROC)
         && !options::nl_define_macro()))
   {
      return;
   }
   chunk_t *braceOpen = chunk_get_next_type(start, CT_BRACE_OPEN, start->level);

   LOG_FMT(LNEWLINE, "%s(%d): braceOpen->orig_line is %zu, orig_col is %zu, text() is '%s'\n",
           __func__, __LINE__, braceOpen->orig_line, braceOpen->orig_col, braceOpen->text());
   log_pcf_flags(LNEWLINE, braceOpen->flags);

   if (braceOpen->flags.test(PCF_ONE_LINER))
   {
      LOG_FMT(LNEWLINE, "%s(%d): is one_liner\n",
              __func__, __LINE__);
      return;
   }
   chunk_t *beforeBrace = chunk_get_prev(braceOpen);

   LOG_FMT(LNEWLINE, "%s(%d): beforeBrace->orig_line is %zu, orig_col is %zu, text() is '%s'\n",
           __func__, __LINE__, beforeBrace->orig_line, beforeBrace->orig_col, beforeBrace->text());
   // namespace {
   newline_iarf_pair(beforeBrace, braceOpen, nl_opt);
} // newlines_namespace


static void newlines_cuddle_uncuddle(chunk_t *start, iarf_e nl_opt)
{
   LOG_FUNC_ENTRY();
   chunk_t *br_close;

   log_rule_B("nl_define_macro");

   if (start->flags.test(PCF_IN_PREPROC) && !options::nl_define_macro())
   {
      return;
   }
   br_close = chunk_get_prev_ncnlni(start);   // Issue #2279

   if (chunk_is_token(br_close, CT_BRACE_CLOSE))
   {
      newline_iarf_pair(br_close, start, nl_opt);
   }
}


static void newlines_do_else(chunk_t *start, iarf_e nl_opt)
{
   LOG_FUNC_ENTRY();
   chunk_t *next;

   log_rule_B("nl_define_macro");

   if (  nl_opt == IARF_IGNORE
      || (  start->flags.test(PCF_IN_PREPROC)
         && !options::nl_define_macro()))
   {
      return;
   }
   next = chunk_get_next_ncnl(start);

   if (  next != nullptr
      && (chunk_is_token(next, CT_BRACE_OPEN) || chunk_is_token(next, CT_VBRACE_OPEN)))
   {
      if (!one_liner_nl_ok(next))
      {
         LOG_FMT(LNL1LINE, "%s(%d): a new line may NOT be added\n", __func__, __LINE__);
         return;
      }
      LOG_FMT(LNL1LINE, "%s(%d): a new line may be added\n", __func__, __LINE__);

      if (chunk_is_token(next, CT_VBRACE_OPEN))
      {
         // Can only add - we don't want to create a one-line here
         if (nl_opt & IARF_ADD)
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


static bool is_var_def(chunk_t *pc, chunk_t *next)
{
   if (chunk_is_token(pc, CT_DECLTYPE) && chunk_is_token(next, CT_PAREN_OPEN))
   {
      // If current token starts a decltype expression, skip it
      next = chunk_skip_to_match(next);
      next = chunk_get_next_ncnl(next);
   }
   else if (!chunk_is_type(pc))
   {
      // Otherwise, if the current token is not a type --> not a declaration
      return(false);
   }
   else if (chunk_is_token(next, CT_DC_MEMBER))
   {
      // If next token is CT_DC_MEMBER, skip it
      next = chunk_skip_dc_member(next);
   }
   else if (chunk_is_token(next, CT_ANGLE_OPEN))
   {
      // If we have a template type, skip it
      next = chunk_skip_to_match(next);
      next = chunk_get_next_ncnl(next);
   }
   bool is = (  (  chunk_is_type(next)
                && get_chunk_parent_type(next) != CT_FUNC_DEF)           // Issue #2639
             || chunk_is_token(next, CT_WORD)
             || chunk_is_token(next, CT_FUNC_CTOR_VAR));

   return(is);
} // is_var_def


// Put newline(s) before and/or after a block of variable definitions
static chunk_t *newline_def_blk(chunk_t *start, bool fn_top)
{
   LOG_FUNC_ENTRY();
   bool    did_this_line = false;
   bool    first_var_blk = true;
   bool    var_blk       = false;

   chunk_t *prev = chunk_get_prev_ncnlni(start);   // Issue #2279

   // can't be any variable definitions in a "= {" block
   if (chunk_is_token(prev, CT_ASSIGN))
   {
      chunk_t *tmp = chunk_get_next_type(start, CT_BRACE_CLOSE, start->level);
      return(chunk_get_next_ncnl(tmp));
   }
   chunk_t *pc = chunk_get_next(start);

   while (  pc != nullptr
         && (pc->level >= start->level || pc->level == 0))
   {
      LOG_FMT(LNL1LINE, "%s(%d): pc->orig_line is %zu, pc->orig_col is %zu, text() is '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());

      chunk_t *next_pc = chunk_get_next(pc);

      if (chunk_is_token(next_pc, CT_DC_MEMBER))
      {
         // If next_pc token is CT_DC_MEMBER, skip it
         pc = chunk_skip_dc_member(pc);
      }

      if (chunk_is_comment(pc))
      {
         pc = chunk_get_next(pc);
         continue;
      }

      // process nested braces
      if (chunk_is_token(pc, CT_BRACE_OPEN))
      {
         pc = newline_def_blk(pc, false);
         continue;
      }

      // Done with this brace set?
      if (chunk_is_token(pc, CT_BRACE_CLOSE))
      {
         pc = chunk_get_next(pc);
         break;
      }

      if (chunk_is_preproc(pc))
      {
         if (!var_blk)
         {
            pc = chunk_get_next(pc);
            break;
         }
      }

      // skip vbraces
      if (chunk_is_token(pc, CT_VBRACE_OPEN))
      {
         pc = chunk_get_next_type(pc, CT_VBRACE_CLOSE, pc->level);
         pc = chunk_get_next(pc);
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

      // Determine if this is a variable definition or code
      if (  !did_this_line
         && pc->type != CT_FUNC_CLASS_DEF
         && pc->type != CT_FUNC_CLASS_PROTO
         && ((pc->level == (start->level + 1)) || pc->level == 0))
      {
         chunk_t *next = chunk_get_next_ncnl(pc);

         if (chunk_is_token(next, CT_PTR_TYPE))               // Issue #2692
         {
            next = chunk_get_next_ncnl(next);
         }

         if (next == nullptr)
         {
            break;
         }
         LOG_FMT(LNL1LINE, "%s(%d): next->orig_line is %zu, next->orig_col is %zu, text() is '%s'\n",
                 __func__, __LINE__, next->orig_line, next->orig_col, next->text());
         prev = chunk_get_prev_type(pc, CT_SEMICOLON, pc->level);

         if (prev == nullptr)
         {
            prev = chunk_get_prev_type(pc, CT_BRACE_OPEN, pc->level - 1);      // Issue #2692
         }

         if (  chunk_is_token(prev, CT_STRING)
            && get_chunk_parent_type(prev) == CT_EXTERN
            && chunk_is_token(prev->prev, CT_EXTERN))
         {
            prev = chunk_get_prev_ncnlni(prev->prev);   // Issue #2279
         }

         if (is_var_def(pc, next))
         {
            LOG_FMT(LBLANKD, "%s(%d): 'typ==var' found: '%s %s' at line %zu\n",
                    __func__, __LINE__, pc->text(), next->text(), pc->orig_line);
            // Put newline(s) before a block of variable definitions
            log_rule_B("nl_var_def_blk_start");

            if (  !var_blk
               && first_var_blk
               && options::nl_var_def_blk_start() > 0)
            {
               newline_min_after(prev, options::nl_var_def_blk_start(), PCF_VAR_DEF);
            }
            // set newlines within var def block
            else if (var_blk && (options::nl_var_def_blk_in() > 0))
            {
               log_rule_B("nl_var_def_blk_in");
               prev = chunk_get_prev(pc);
               LOG_FMT(LNL1LINE, "%s(%d): prev->orig_line is %zu, prev->orig_col is %zu, text() is '%s'\n",
                       __func__, __LINE__, prev->orig_line, prev->orig_col, prev->text());

               if (chunk_is_newline(prev))
               {
                  log_rule_B("nl_var_def_blk_in");

                  if (prev->nl_count > options::nl_var_def_blk_in())
                  {
                     prev->nl_count = options::nl_var_def_blk_in();
                     MARK_CHANGE();
                  }
               }
            }
            pc      = chunk_get_next_type(pc, CT_SEMICOLON, pc->level);
            var_blk = true;
         }
         else if (var_blk)
         {
            log_rule_B("nl_var_def_blk_end");

            if (options::nl_var_def_blk_end() > 0)
            {
               newline_min_after(prev, options::nl_var_def_blk_end(), PCF_VAR_DEF);
            }
            // set blank lines after first var def block
            log_rule_B("nl_func_var_def_blk");

            if (  var_blk
               && first_var_blk
               && fn_top
               && (options::nl_func_var_def_blk() > 0))
            {
               LOG_FMT(LBLANKD, "%s(%d): nl_func_var_def_blk at line %zu\n",
                       __func__, __LINE__, prev->orig_line);
               log_rule_B("nl_func_var_def_blk");
               newline_min_after(prev, 1 + options::nl_func_var_def_blk(), PCF_VAR_DEF);
            }
            // reset the variables for the next block
            first_var_blk = true;
            var_blk       = false;
         }
      }
      else
      {
         //if (pc->type == CT_FUNC_CLASS_DEF)
         if (chunk_is_token(pc, CT_FUNC_CLASS_DEF))
         {
            if (  var_blk
               && options::nl_var_def_blk_end() > 0)
            {
               prev = chunk_get_prev(pc);
               prev = chunk_get_prev(prev);
               newline_min_after(prev, options::nl_var_def_blk_end(), PCF_VAR_DEF);
               pc = chunk_get_next(pc);
               //typedef_blk   = false;
               first_var_blk = false;
               var_blk       = false;
            }
         }
      }
      did_this_line = true;
      pc            = chunk_get_next(pc);
   }
   return(pc);
} // newline_def_blk


static bool collapse_empty_body(chunk_t *br_open)
{
   log_rule_B("nl_collapse_empty_body");

   if (  !options::nl_collapse_empty_body()
      || !chunk_is_token(chunk_get_next_nnl(br_open), CT_BRACE_CLOSE))
   {
      return(false);
   }

   for (chunk_t *pc = chunk_get_next(br_open)
        ; pc != nullptr && pc->type != CT_BRACE_CLOSE
        ; pc = chunk_get_next(pc))
   {
      if (chunk_is_token(pc, CT_NEWLINE) && chunk_safe_to_del_nl(pc))
      {
         pc = pc->prev;
         chunk_t *next = pc->next;
         chunk_del(next);
         MARK_CHANGE();
      }
   }

   return(true);
}


static void newlines_brace_pair(chunk_t *br_open)
{
   LOG_FUNC_ENTRY();

   log_rule_B("nl_define_macro");

   if (br_open->flags.test(PCF_IN_PREPROC) && !options::nl_define_macro())
   {
      return;
   }

   //fixes 1235 Add single line namespace support
   if (  chunk_is_token(br_open, CT_BRACE_OPEN)
      && (get_chunk_parent_type(br_open) == CT_NAMESPACE)
      && chunk_is_newline(chunk_get_prev(br_open)))
   {
      chunk_t *chunk_brace_close = chunk_skip_to_match(br_open, scope_e::ALL);

      if (chunk_brace_close != nullptr)
      {
         if (are_chunks_in_same_line(br_open, chunk_brace_close))
         {
            log_rule_B("nl_namespace_two_to_one_liner");

            if (options::nl_namespace_two_to_one_liner())
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
   log_rule_B("nl_create_func_def_one_liner");

   if (  get_chunk_parent_type(br_open) == CT_FUNC_DEF
      && options::nl_create_func_def_one_liner())
   {
      chunk_t *br_close = chunk_skip_to_match(br_open, scope_e::ALL);
      chunk_t *tmp      = chunk_get_prev_ncnlni(br_open); // Issue #2279

      if (  br_close != nullptr                           // Issue #2594
         && ((br_close->orig_line - br_open->orig_line) <= 2)
         && chunk_is_paren_close(tmp))                    // need to check the conditions.
      {
         // Issue #1825
         bool is_it_possible = true;

         while (  tmp != nullptr
               && (tmp = chunk_get_next(tmp)) != nullptr
               && !chunk_is_closing_brace(tmp)
               && (chunk_get_next(tmp) != nullptr))
         {
            LOG_FMT(LNL1LINE, "%s(%d): tmp->orig_line is %zu, tmp->orig_col is %zu, text() is '%s'\n",
                    __func__, __LINE__, tmp->orig_line, tmp->orig_col, tmp->text());

            if (chunk_is_comment(tmp))
            {
               is_it_possible = false;
               break;
            }
         }

         if (is_it_possible)
         {
            tmp = chunk_get_prev_ncnlni(br_open);

            while (  tmp != nullptr
                  && (tmp = chunk_get_next(tmp)) != nullptr
                  && !chunk_is_closing_brace(tmp)
                  && (chunk_get_next(tmp) != nullptr))
            {
               LOG_FMT(LNL1LINE, "%s(%d): tmp->orig_line is %zu, tmp->orig_col is %zu, text() is '%s'\n",
                       __func__, __LINE__, tmp->orig_line, tmp->orig_col, tmp->text());

               if (chunk_is_newline(tmp))
               {
                  tmp = chunk_get_prev(tmp);                 // Issue #1825
                  newline_iarf_pair(tmp, chunk_get_next_ncnl(tmp), IARF_REMOVE);
               }
            }
            chunk_flags_set(br_open, PCF_ONE_LINER);         // set the one liner flag if needed
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

   chunk_t *next = chunk_get_next_nc(br_open);

   // Insert a newline between the '=' and open brace, if needed
   LOG_FMT(LNL1LINE, "%s(%d): br_open->text() '%s', br_open->type [%s], br_open->parent_type [%s]\n",
           __func__, __LINE__, br_open->text(), get_token_name(br_open->type),
           get_token_name(get_chunk_parent_type(br_open)));

   if (get_chunk_parent_type(br_open) == CT_ASSIGN)
   {
      // Only mess with it if the open brace is followed by a newline
      if (chunk_is_newline(next))
      {
         chunk_t *prev = chunk_get_prev_ncnlni(br_open);   // Issue #2279
         log_rule_B("nl_assign_brace");
         newline_iarf_pair(prev, br_open, options::nl_assign_brace());
      }
   }

   if (  get_chunk_parent_type(br_open) == CT_OC_MSG_DECL
      || get_chunk_parent_type(br_open) == CT_FUNC_DEF
      || get_chunk_parent_type(br_open) == CT_FUNC_CLASS_DEF
      || get_chunk_parent_type(br_open) == CT_OC_CLASS
      || get_chunk_parent_type(br_open) == CT_CS_PROPERTY
      || get_chunk_parent_type(br_open) == CT_CPP_LAMBDA
      || get_chunk_parent_type(br_open) == CT_FUNC_CALL
      || get_chunk_parent_type(br_open) == CT_FUNC_CALL_USER)
   {
      chunk_t *prev = nullptr;
      iarf_e  val;

      if (get_chunk_parent_type(br_open) == CT_OC_MSG_DECL)
      {
         log_rule_B("nl_oc_mdef_brace");
         val = options::nl_oc_mdef_brace();
      }
      else
      {
         if (  get_chunk_parent_type(br_open) == CT_FUNC_DEF
            || get_chunk_parent_type(br_open) == CT_FUNC_CLASS_DEF
            || get_chunk_parent_type(br_open) == CT_OC_CLASS)
         {
            val = iarf_e::NOT_DEFINED;
            log_rule_B("nl_fdef_brace_cond");
            const iarf_e nl_fdef_brace_cond_v = options::nl_fdef_brace_cond();

            if (nl_fdef_brace_cond_v != IARF_IGNORE)
            {
               prev = chunk_get_prev_ncnlni(br_open);   // Issue #2279

               if (chunk_is_token(prev, CT_FPAREN_CLOSE))
               {
                  val = nl_fdef_brace_cond_v;
               }
            }

            if (val == iarf_e::NOT_DEFINED)
            {
               log_rule_B("nl_fdef_brace");
               val = options::nl_fdef_brace();
            }
         }
         else
         {
            log_rule_B("nl_property_brace");
            log_rule_B("nl_cpp_ldef_brace");
            log_rule_B("nl_fcall_brace");
            val = ((get_chunk_parent_type(br_open) == CT_CS_PROPERTY) ?
                   options::nl_property_brace() :
                   ((get_chunk_parent_type(br_open) == CT_CPP_LAMBDA) ?
                    options::nl_cpp_ldef_brace() :
                    options::nl_fcall_brace()));
         }
      }

      if (val != IARF_IGNORE)
      {
         if (prev == nullptr)
         {
            // Grab the chunk before the open brace
            prev = chunk_get_prev_ncnlni(br_open);   // Issue #2279
         }
         newline_iarf_pair(prev, br_open, val);
      }
   }

   if (collapse_empty_body(br_open))
   {
      return;
   }
   //fixes #1245 will add new line between tsquare and brace open based on nl_tsquare_brace

   if (chunk_is_token(br_open, CT_BRACE_OPEN))
   {
      chunk_t *chunk_closeing_brace = chunk_skip_to_match(br_open, scope_e::ALL);

      if (chunk_closeing_brace != nullptr)
      {
         if (chunk_closeing_brace->orig_line > br_open->orig_line)
         {
            chunk_t *prev = chunk_get_prev_nc(br_open);

            if (  chunk_is_token(prev, CT_TSQUARE)
               && chunk_is_newline(next))
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
      if (chunk_is_newline(next))
      {
         log_rule_B("nl_inside_namespace");

         if (options::nl_inside_namespace() && get_chunk_parent_type(br_open) == CT_NAMESPACE)
         {
            blank_line_set(next, options::nl_inside_namespace);
         }
         else if (next->nl_count > 1)
         {
            next->nl_count = 1;
            LOG_FMT(LBLANKD, "%s(%d): eat_blanks_after_open_brace %zu\n",
                    __func__, __LINE__, next->orig_line);
            MARK_CHANGE();
         }
      }
   }
   bool nl_close_brace = false;

   // Handle the cases where the brace is part of a function call or definition
   if (  get_chunk_parent_type(br_open) == CT_FUNC_DEF
      || get_chunk_parent_type(br_open) == CT_FUNC_CALL
      || get_chunk_parent_type(br_open) == CT_FUNC_CALL_USER
      || get_chunk_parent_type(br_open) == CT_FUNC_CLASS_DEF
      || get_chunk_parent_type(br_open) == CT_OC_CLASS
      || get_chunk_parent_type(br_open) == CT_OC_MSG_DECL
      || get_chunk_parent_type(br_open) == CT_CS_PROPERTY
      || get_chunk_parent_type(br_open) == CT_CPP_LAMBDA)
   {
      // Need to force a newline before the close brace, if not in a class body
      if (!br_open->flags.test(PCF_IN_CLASS))
      {
         nl_close_brace = true;
      }
      // handle newlines after the open brace
      chunk_t *pc = chunk_get_next_ncnl(br_open);
      newline_add_between(br_open, pc);

      newline_def_blk(br_open, true);
   }

   // Handle the cases where the brace is part of a class or struct
   if (get_chunk_parent_type(br_open) == CT_CLASS || get_chunk_parent_type(br_open) == CT_STRUCT)
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
      chunk_t *pc = chunk_get_next(br_open);

      while (chunk_is_token(pc, CT_COMMENT))
      {
         pc = chunk_get_next(pc);
      }

      if (chunk_is_newline(pc) || chunk_is_comment(pc))
      {
         nl_close_brace = true;
      }
   }
   chunk_t *prev = chunk_get_prev_nblank(br_close);

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
   if (chunk_is_token(prev, CT_SEMICOLON) || chunk_is_token(prev, CT_BRACE_CLOSE))
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
      || (  chunk_is_token(pc, CT_BRACE_OPEN)
         || chunk_is_token(pc, CT_VBRACE_OPEN)
         || chunk_is_token(pc, CT_CASE_COLON)))
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
} // newline_before_return


static void newline_after_return(chunk_t *start)
{
   LOG_FUNC_ENTRY();

   chunk_t *semi  = chunk_get_next_type(start, CT_SEMICOLON, start->level);
   chunk_t *after = chunk_get_next_nblank(semi);

   // If we hit a brace or an 'else', then a newline isn't needed
   if (  after == nullptr
      || chunk_is_token(after, CT_BRACE_CLOSE)
      || chunk_is_token(after, CT_VBRACE_CLOSE)
      || chunk_is_token(after, CT_ELSE))
   {
      return;
   }
   chunk_t *pc;

   for (pc = chunk_get_next(semi); pc != after; pc = chunk_get_next(pc))
   {
      if (chunk_is_token(pc, CT_NEWLINE))
      {
         if (pc->nl_count < 2)
         {
            double_newline(pc);
         }
         return;
      }
   }
}


static void newline_iarf_pair(chunk_t *before, chunk_t *after, iarf_e av, bool check_nl_assign_leave_one_liners)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LNEWLINE, "%s(%d): ", __func__, __LINE__);
   log_func_stack(LNEWLINE, " CallStack:");

   if (  before == nullptr
      || after == nullptr
      || chunk_is_token(after, CT_IGNORED))
   {
      return;
   }

   if (av & IARF_ADD)
   {
      log_rule_B("nl_assign_leave_one_liners");

      if (  check_nl_assign_leave_one_liners
         && options::nl_assign_leave_one_liners()
         && after->flags.test(PCF_ONE_LINER))
      {
         return;
      }
      chunk_t *nl = newline_add_between(before, after);

      if (  nl != nullptr
         && av == IARF_FORCE
         && nl->nl_count > 1)
      {
         nl->nl_count = 1;
      }
   }
   else if (av & IARF_REMOVE)
   {
      newline_del_between(before, after);
   }
}


void newline_iarf(chunk_t *pc, iarf_e av)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LNFD, "%s(%d): ", __func__, __LINE__);
   log_func_stack(LNFD, " CallStack:");

   newline_iarf_pair(pc, chunk_get_next_nnl(pc), av);
}


static void newline_func_multi_line(chunk_t *start)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNFD, "%s(%d): called on %zu:%zu '%s' [%s/%s]\n",
           __func__, __LINE__, start->orig_line, start->orig_col,
           start->text(), get_token_name(start->type), get_token_name(get_chunk_parent_type(start)));

   bool add_start;
   bool add_args;
   bool add_end;

   if (  get_chunk_parent_type(start) == CT_FUNC_DEF
      || get_chunk_parent_type(start) == CT_FUNC_CLASS_DEF)
   {
      log_rule_B("nl_func_def_start_multi_line");
      add_start = options::nl_func_def_start_multi_line();
      log_rule_B("nl_func_def_args_multi_line");
      add_args = options::nl_func_def_args_multi_line();
      log_rule_B("nl_func_def_end_multi_line");
      add_end = options::nl_func_def_end_multi_line();
   }
   else if (  get_chunk_parent_type(start) == CT_FUNC_CALL
           || get_chunk_parent_type(start) == CT_FUNC_CALL_USER)
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
   chunk_t *pc = chunk_get_next_ncnl(start);

   while (pc != nullptr && pc->level > start->level)
   {
      pc = chunk_get_next_ncnl(pc);
   }

   if (  chunk_is_token(pc, CT_FPAREN_CLOSE)
      && chunk_is_newline_between(start, pc))
   {
      chunk_t *start_next         = chunk_get_next_ncnl(start);
      bool    has_leading_closure = (  start_next->parent_type == CT_OC_BLOCK_EXPR
                                    || start_next->parent_type == CT_CPP_LAMBDA
                                    || start_next->type == CT_BRACE_OPEN);

      chunk_t *prev_end            = chunk_get_prev_ncnl(pc);
      bool    has_trailing_closure = (  prev_end->parent_type == CT_OC_BLOCK_EXPR
                                     || prev_end->parent_type == CT_CPP_LAMBDA
                                     || prev_end->type == CT_BRACE_OPEN);

      if (add_start && !chunk_is_newline(chunk_get_next(start)))
      {
         if (options::nl_func_call_args_multi_line_ignore_closures())
         {
            if (!has_leading_closure && !has_trailing_closure)
            {
               newline_iarf(start, IARF_ADD);
            }
         }
         else
         {
            newline_iarf(start, IARF_ADD);
         }
      }

      if (add_end && !chunk_is_newline(chunk_get_prev(pc)))
      {
         if (options::nl_func_call_args_multi_line_ignore_closures())
         {
            if (!has_leading_closure && !has_trailing_closure)
            {
               newline_iarf(chunk_get_prev(pc), IARF_ADD);
            }
         }
         else
         {
            newline_iarf(chunk_get_prev(pc), IARF_ADD);
         }
      }

      if (add_args)
      {
         // process the function in reverse and leave the first comma if the option to leave trailing closure
         // is on. nl_func_call_args_multi_line_ignore_trailing_closure
         for (pc = chunk_get_next_ncnl(start);
              pc != nullptr && pc->level > start->level;
              pc = chunk_get_next_ncnl(pc))
         {
            if (chunk_is_token(pc, CT_COMMA) && (pc->level == (start->level + 1)))
            {
               chunk_t *tmp = chunk_get_next(pc);

               if (chunk_is_comment(tmp))
               {
                  pc = tmp;
               }

               if (!chunk_is_newline(chunk_get_next(pc)))
               {
                  if (options::nl_func_call_args_multi_line_ignore_closures())
                  {
                     chunk_t *prev_comma  = chunk_get_prev_ncnl(pc);
                     chunk_t *after_comma = chunk_get_next_ncnl(pc);

                     if (!(  (  prev_comma->parent_type == CT_OC_BLOCK_EXPR
                             || prev_comma->parent_type == CT_CPP_LAMBDA
                             || prev_comma->type == CT_BRACE_OPEN)
                          || (  after_comma->parent_type == CT_OC_BLOCK_EXPR
                             || after_comma->parent_type == CT_CPP_LAMBDA
                             || after_comma->type == CT_BRACE_OPEN)))
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


static void newline_template(chunk_t *start)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNFD, "%s(%d): called on %zu:%zu '%s' [%s/%s]\n",
           __func__, __LINE__, start->orig_line, start->orig_col,
           start->text(), get_token_name(start->type), get_token_name(get_chunk_parent_type(start)));

   log_rule_B("nl_template_start");
   bool add_start = options::nl_template_start();

   log_rule_B("nl_template_args");
   bool add_args = options::nl_template_args();

   log_rule_B("nl_template_end");
   bool add_end = options::nl_template_end();

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

   if (chunk_is_token(pc, CT_ANGLE_CLOSE))
   {
      if (add_start)
      {
         newline_iarf(start, IARF_ADD);
      }

      if (add_end)
      {
         newline_iarf(chunk_get_prev(pc), IARF_ADD);
      }

      if (add_args)
      {
         for (pc = chunk_get_next_ncnl(start);
              pc != nullptr && pc->level > start->level;
              pc = chunk_get_next_ncnl(pc))
         {
            if (chunk_is_token(pc, CT_COMMA) && (pc->level == (start->level + 1)))
            {
               chunk_t *tmp = chunk_get_next(pc);

               if (chunk_is_comment(tmp))
               {
                  pc = tmp;
               }

               if (!chunk_is_newline(chunk_get_next(pc)))
               {
                  newline_iarf(pc, IARF_ADD);
               }
            }
         }
      }
   }
} // newline_template


static void newline_func_def_or_call(chunk_t *start)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNFD, "%s(%d): called on start->text() is '%s', orig_line is %zu, orig_col is %zu, [%s/%s]\n",
           __func__, __LINE__, start->text(), start->orig_line, start->orig_col,
           get_token_name(start->type), get_token_name(get_chunk_parent_type(start)));

   chunk_t *prev  = nullptr;
   bool    is_def = (get_chunk_parent_type(start) == CT_FUNC_DEF)
                    || get_chunk_parent_type(start) == CT_FUNC_CLASS_DEF;
   bool    is_call = (get_chunk_parent_type(start) == CT_FUNC_CALL)
                     || get_chunk_parent_type(start) == CT_FUNC_CALL_USER;

   LOG_FMT(LNFD, "%s(%d): is_def is %s, is_call is %s\n",
           __func__, __LINE__, is_def ? "TRUE" : "FALSE", is_call ? "TRUE" : "FALSE");

   if (is_call)
   {
      log_rule_B("nl_func_call_paren");
      iarf_e atmp = options::nl_func_call_paren();

      if (atmp != IARF_IGNORE)
      {
         prev = chunk_get_prev_ncnlni(start);   // Issue #2279

         if (prev != nullptr)
         {
            newline_iarf(prev, atmp);
         }
      }
      chunk_t *pc = chunk_get_next_ncnl(start);

      if (chunk_is_str(pc, ")", 1))
      {
         log_rule_B("nl_func_call_paren_empty");
         atmp = options::nl_func_call_paren_empty();

         if (atmp != IARF_IGNORE)
         {
            prev = chunk_get_prev_ncnlni(start);   // Issue #2279

            if (prev != nullptr)
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
      auto atmp = is_def ? options::nl_func_def_paren()
                  : options::nl_func_paren();
      LOG_FMT(LSPACE, "%s(%d): atmp is %s\n",
              __func__, __LINE__,
              (atmp == IARF_IGNORE) ? "IGNORE" :
              (atmp == IARF_ADD) ? "ADD" :
              (atmp == IARF_REMOVE) ? "REMOVE" : "FORCE");

      if (atmp != IARF_IGNORE)
      {
         prev = chunk_get_prev_ncnlni(start);   // Issue #2279

         if (prev != nullptr)
         {
            newline_iarf(prev, atmp);
         }
      }
      // Handle break newlines type and function
      prev = chunk_get_prev_ncnlni(start);   // Issue #2279
      prev = skip_template_prev(prev);
      // Don't split up a function variable
      prev = chunk_is_paren_close(prev) ? nullptr : chunk_get_prev_ncnlni(prev);   // Issue #2279

      log_rule_B("nl_func_class_scope");

      if (  chunk_is_token(prev, CT_DC_MEMBER)
         && (options::nl_func_class_scope() != IARF_IGNORE))
      {
         newline_iarf(chunk_get_prev_ncnlni(prev), options::nl_func_class_scope());   // Issue #2279
      }

      if (prev != nullptr && prev->type != CT_ACCESS_COLON)
      {
         chunk_t *tmp;

         if (chunk_is_token(prev, CT_OPERATOR))
         {
            tmp  = prev;
            prev = chunk_get_prev_ncnlni(prev);   // Issue #2279
         }
         else
         {
            tmp = start;
         }

         if (chunk_is_token(prev, CT_DC_MEMBER))
         {
            log_rule_B("nl_func_scope_name");

            if (options::nl_func_scope_name() != IARF_IGNORE)
            {
               newline_iarf(prev, options::nl_func_scope_name());
            }
         }
         const chunk_t *tmp_next = chunk_get_next_ncnl(prev);

         if (tmp_next != nullptr && tmp_next->type != CT_FUNC_CLASS_DEF)
         {
            chunk_t *closing = chunk_skip_to_match(tmp);
            chunk_t *brace   = chunk_get_next_ncnl(closing);
            iarf_e  a;                                           // Issue #2561

            if (  get_chunk_parent_type(tmp) == CT_FUNC_PROTO
               || get_chunk_parent_type(tmp) == CT_FUNC_CLASS_PROTO)
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
                  && brace->flags.test(PCF_ONE_LINER))                 // Issue #1511
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

            if (  tmp->flags.test(PCF_IN_CLASS)
               && (options::nl_func_type_name_class() != IARF_IGNORE))
            {
               a = options::nl_func_type_name_class();
            }

            if (a != IARF_IGNORE && prev != nullptr)
            {
               LOG_FMT(LNFD, "%s(%d): prev->text() '%s', orig_line is %zu, orig_col is %zu, [%s/%s]\n",
                       __func__, __LINE__, prev->text(), prev->orig_line, prev->orig_col,
                       get_token_name(prev->type),
                       get_token_name(get_chunk_parent_type(prev)));

               if (chunk_is_token(prev, CT_DESTRUCTOR))
               {
                  prev = chunk_get_prev_ncnlni(prev);   // Issue #2279
               }

               /*
                * If we are on a '::', step back two tokens
                * TODO: do we also need to check for '.' ?
                */
               while (chunk_is_token(prev, CT_DC_MEMBER))
               {
                  prev = chunk_get_prev_ncnlni(prev);   // Issue #2279
                  prev = skip_template_prev(prev);
                  prev = chunk_get_prev_ncnlni(prev);   // Issue #2279
               }

               if (  prev != nullptr
                  && prev->type != CT_BRACE_CLOSE
                  && prev->type != CT_VBRACE_CLOSE
                  && prev->type != CT_BRACE_OPEN
                  && prev->type != CT_SEMICOLON
                  && prev->type != CT_ACCESS_COLON
                     // #1008: if we landed on an operator check that it is having
                     // a type before it, in order to not apply nl_func_type_name
                     // on conversion operators as they don't have a normal
                     // return type syntax
                  && (tmp_next->type != CT_OPERATOR ? true : chunk_is_type(prev)))
               {
                  newline_iarf(prev, a);
               }
            }
         }
      }
      chunk_t *pc = chunk_get_next_ncnl(start);

      if (chunk_is_str(pc, ")", 1))
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
            prev = chunk_get_prev_ncnlni(start);   // Issue #2279

            if (prev != nullptr)
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
      if (chunk_is_token(pc, CT_COMMA) && (pc->level == (start->level + 1)))
      {
         comma_count++;
         tmp = chunk_get_next(pc);

         if (chunk_is_comment(tmp))
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
         else // get_chunk_parent_type(start) == CT_FUNC_DECL
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
   if (chunk_is_token(pc, CT_FPAREN_CLOSE))
   {
      prev = chunk_get_prev_nnl(pc);

      if (prev != nullptr && prev->type != CT_FPAREN_OPEN && !is_call)
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

   if (sq_c == nullptr)
   {
      return;
   }
   log_rule_B("nl_oc_msg_leave_one_liner");

   if (options::nl_oc_msg_leave_one_liner())
   {
      return;
   }

   for (chunk_t *pc = chunk_get_next_ncnl(start); pc; pc = chunk_get_next_ncnl(pc))
   {
      if (pc->level <= start->level)
      {
         break;
      }

      if (chunk_is_token(pc, CT_OC_MSG_NAME))
      {
         newline_add_before(pc);
      }
   }
} // newline_oc_msg


static bool one_liner_nl_ok(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LNL1LINE, "%s(%d): check type is %s, parent is %s, flag is %s, orig_line is %zu, orig_col is %zu\n",
           __func__, __LINE__, get_token_name(pc->type), get_token_name(get_chunk_parent_type(pc)),
           pcf_flags_str(pc->flags).c_str(), pc->orig_line, pc->orig_col);

   if (!pc->flags.test(PCF_ONE_LINER))
   {
      LOG_FMT(LNL1LINE, "%s(%d): true (not 1-liner), a new line may be added\n", __func__, __LINE__);
      return(true);
   }
   // Step back to find the opening brace
   chunk_t *br_open = pc;

   if (chunk_is_closing_brace(br_open))
   {
      br_open = chunk_get_prev_type(br_open,
                                    chunk_is_token(br_open, CT_BRACE_CLOSE) ? CT_BRACE_OPEN : CT_VBRACE_OPEN,
                                    br_open->level, scope_e::ALL);
   }
   else
   {
      while (  br_open != nullptr
            && br_open->flags.test(PCF_ONE_LINER)
            && !chunk_is_opening_brace(br_open)
            && !chunk_is_closing_brace(br_open))
      {
         br_open = chunk_get_prev(br_open);
      }
   }
   pc = br_open;

   if (  pc != nullptr
      && pc->flags.test(PCF_ONE_LINER)
      && (  chunk_is_token(pc, CT_BRACE_OPEN)
         || chunk_is_token(pc, CT_BRACE_CLOSE)
         || chunk_is_token(pc, CT_VBRACE_OPEN)
         || chunk_is_token(pc, CT_VBRACE_CLOSE)))
   {
      log_rule_B("nl_class_leave_one_liners");

      if (  options::nl_class_leave_one_liners()
         && pc->flags.test(PCF_IN_CLASS))
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (class)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_assign_leave_one_liners");

      if (  options::nl_assign_leave_one_liners()
         && get_chunk_parent_type(pc) == CT_ASSIGN)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (assign)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_enum_leave_one_liners");

      if (  options::nl_enum_leave_one_liners()
         && get_chunk_parent_type(pc) == CT_ENUM)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (enum)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_getset_leave_one_liners");

      if (  options::nl_getset_leave_one_liners()
         && get_chunk_parent_type(pc) == CT_GETSET)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (get/set), a new line may NOT be added\n", __func__, __LINE__);
         return(false);
      }
      // Issue #UT-98
      log_rule_B("nl_cs_property_leave_one_liners");

      if (  options::nl_cs_property_leave_one_liners()
         && get_chunk_parent_type(pc) == CT_CS_PROPERTY)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (c# property), a new line may NOT be added\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_func_leave_one_liners");

      if (  options::nl_func_leave_one_liners()
         && (  get_chunk_parent_type(pc) == CT_FUNC_DEF
            || get_chunk_parent_type(pc) == CT_FUNC_CLASS_DEF))
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (func def)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_func_leave_one_liners");

      if (  options::nl_func_leave_one_liners()
         && get_chunk_parent_type(pc) == CT_OC_MSG_DECL)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (method def)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_cpp_lambda_leave_one_liners");

      if (  options::nl_cpp_lambda_leave_one_liners()
         && ((get_chunk_parent_type(pc) == CT_CPP_LAMBDA)))
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (lambda)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_oc_msg_leave_one_liner");

      if (  options::nl_oc_msg_leave_one_liner()
         && pc->flags.test(PCF_IN_OC_MSG))
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (message)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_if_leave_one_liners");

      if (  options::nl_if_leave_one_liners()
         && (  get_chunk_parent_type(pc) == CT_IF
            || get_chunk_parent_type(pc) == CT_ELSEIF
            || get_chunk_parent_type(pc) == CT_ELSE))
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (if/else)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_while_leave_one_liners");

      if (  options::nl_while_leave_one_liners()
         && get_chunk_parent_type(pc) == CT_WHILE)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (while)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_for_leave_one_liners");

      if (  options::nl_for_leave_one_liners()
         && get_chunk_parent_type(pc) == CT_FOR)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (for)\n", __func__, __LINE__);
         return(false);
      }
   }
   LOG_FMT(LNL1LINE, "%s(%d): true, a new line may be added\n", __func__, __LINE__);
   return(true);
} // one_liner_nl_ok


void undo_one_liner(chunk_t *pc)
{
   LOG_FUNC_ENTRY();

   if (pc != nullptr && pc->flags.test(PCF_ONE_LINER))
   {
      LOG_FMT(LNL1LINE, "%s(%d): pc->text() '%s', orig_line is %zu, orig_col is %zu",
              __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
      chunk_flags_clr(pc, PCF_ONE_LINER);

      // scan backward
      LOG_FMT(LNL1LINE, "%s(%d): scan backward\n", __func__, __LINE__);
      chunk_t *tmp = pc;

      while ((tmp = chunk_get_prev(tmp)) != nullptr)
      {
         if (!tmp->flags.test(PCF_ONE_LINER))
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
         if (!tmp->flags.test(PCF_ONE_LINER))
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

   if (first == nullptr || get_token_pattern_class(first->type) != pattern_class_e::NONE)
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


static void nl_create_list_liner(chunk_t *brace_open)
{
   LOG_FUNC_ENTRY();

   // See if we get a newline between the next text and the vbrace_close
   chunk_t *closing = chunk_get_next_type(brace_open, CT_BRACE_CLOSE, brace_open->level);
   chunk_t *tmp     = brace_open;

   if (brace_open == nullptr)
   {
      return;
   }

   do
   {
      if (chunk_is_token(tmp, CT_COMMA))
      {
         return;
      }
      tmp = chunk_get_next(tmp);
   } while (tmp != closing);

   newline_del_between(brace_open, closing);
} // nl_create_list_liner


void newlines_remove_newlines(void)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LBLANK, "%s(%d):\n", __func__, __LINE__);
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
      if (!pc->flags.test(PCF_IN_PREPROC))
      {
         next = pc->next;
         prev = pc->prev;
         newline_iarf(pc, IARF_REMOVE);

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


void newlines_cleanup_angles()
{
   // Issue #1167
   LOG_FUNC_ENTRY();

   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next_ncnl(pc))
   {
      LOG_FMT(LBLANK, "%s(%d): orig_line is %zu, orig_col is %zu, text() is '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());

      if (chunk_is_token(pc, CT_ANGLE_OPEN))
      {
         newline_template(pc);
      }
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
      LOG_FMT(LBLANK, "%s(%d): orig_line is %zu, orig_col is %zu, text() is '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());

      if (chunk_is_token(pc, CT_IF))
      {
         log_rule_B("nl_if_brace");
         newlines_if_for_while_switch(pc, options::nl_if_brace());
      }
      else if (chunk_is_token(pc, CT_ELSEIF))
      {
         log_rule_B("nl_elseif_brace");
         iarf_e arg = options::nl_elseif_brace();
         log_rule_B("nl_if_brace");
         newlines_if_for_while_switch(
            pc, (arg != IARF_IGNORE) ? arg : options::nl_if_brace());
      }
      else if (chunk_is_token(pc, CT_FOR))
      {
         log_rule_B("nl_for_brace");
         newlines_if_for_while_switch(pc, options::nl_for_brace());
      }
      else if (chunk_is_token(pc, CT_CATCH))
      {
         log_rule_B("nl_oc_brace_catch");

         if (  language_is_set(LANG_OC)
            && (pc->str[0] == '@')
            && (options::nl_oc_brace_catch() != IARF_IGNORE))
         {
            newlines_cuddle_uncuddle(pc, options::nl_oc_brace_catch());
         }
         else
         {
            log_rule_B("nl_brace_catch");
            newlines_cuddle_uncuddle(pc, options::nl_brace_catch());
         }
         next = chunk_get_next_ncnl(pc);

         if (chunk_is_token(next, CT_BRACE_OPEN))
         {
            log_rule_B("nl_oc_catch_brace");

            if (  language_is_set(LANG_OC)
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

            if (  language_is_set(LANG_OC)
               && (options::nl_oc_catch_brace() != IARF_IGNORE))
            {
               newlines_if_for_while_switch(pc, options::nl_oc_catch_brace());
            }
            else
            {
               log_rule_B("nl_oc_catch_brace");
               newlines_if_for_while_switch(pc, options::nl_catch_brace());
            }
         }
      }
      else if (chunk_is_token(pc, CT_WHILE))
      {
         log_rule_B("nl_while_brace");
         newlines_if_for_while_switch(pc, options::nl_while_brace());
      }
      else if (chunk_is_token(pc, CT_USING_STMT))
      {
         log_rule_B("nl_using_brace");
         newlines_if_for_while_switch(pc, options::nl_using_brace());
      }
      else if (chunk_is_token(pc, CT_D_SCOPE_IF))
      {
         log_rule_B("nl_scope_brace");
         newlines_if_for_while_switch(pc, options::nl_scope_brace());
      }
      else if (chunk_is_token(pc, CT_UNITTEST))
      {
         log_rule_B("nl_unittest_brace");
         newlines_do_else(pc, options::nl_unittest_brace());
      }
      else if (chunk_is_token(pc, CT_D_VERSION_IF))
      {
         log_rule_B("nl_version_brace");
         newlines_if_for_while_switch(pc, options::nl_version_brace());
      }
      else if (chunk_is_token(pc, CT_SWITCH))
      {
         log_rule_B("nl_switch_brace");
         newlines_if_for_while_switch(pc, options::nl_switch_brace());
      }
      else if (chunk_is_token(pc, CT_SYNCHRONIZED))
      {
         log_rule_B("nl_synchronized_brace");
         newlines_if_for_while_switch(pc, options::nl_synchronized_brace());
      }
      else if (chunk_is_token(pc, CT_DO))
      {
         log_rule_B("nl_do_brace");
         newlines_do_else(pc, options::nl_do_brace());
      }
      else if (chunk_is_token(pc, CT_ELSE))
      {
         log_rule_B("nl_brace_else");
         newlines_cuddle_uncuddle(pc, options::nl_brace_else());
         next = chunk_get_next_ncnl(pc);

         if (chunk_is_token(next, CT_ELSEIF))
         {
            log_rule_B("nl_else_if");
            newline_iarf_pair(pc, next, options::nl_else_if());
         }
         log_rule_B("nl_else_brace");
         newlines_do_else(pc, options::nl_else_brace());
      }
      else if (chunk_is_token(pc, CT_TRY))
      {
         log_rule_B("nl_try_brace");
         newlines_do_else(pc, options::nl_try_brace());
         // Issue #1734
         chunk_t *po = chunk_get_next_ncnl(pc);
         flag_parens(po, PCF_IN_TRY_BLOCK, po->type, CT_NONE, false);
      }
      else if (chunk_is_token(pc, CT_GETSET))
      {
         log_rule_B("nl_getset_brace");
         newlines_do_else(pc, options::nl_getset_brace());
      }
      else if (chunk_is_token(pc, CT_FINALLY))
      {
         log_rule_B("nl_brace_finally");
         newlines_cuddle_uncuddle(pc, options::nl_brace_finally());
         log_rule_B("nl_finally_brace");
         newlines_do_else(pc, options::nl_finally_brace());
      }
      else if (chunk_is_token(pc, CT_WHILE_OF_DO))
      {
         log_rule_B("nl_brace_while");
         newlines_cuddle_uncuddle(pc, options::nl_brace_while());
      }
      else if (chunk_is_token(pc, CT_BRACE_OPEN))
      {
         switch (get_chunk_parent_type(pc))
         {
         case CT_DOUBLE_BRACE:
         {
            log_rule_B("nl_paren_dbrace_open");

            if (options::nl_paren_dbrace_open() != IARF_IGNORE)
            {
               prev = chunk_get_prev_ncnlni(pc, scope_e::PREPROC);   // Issue #2279

               if (chunk_is_paren_close(prev))
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
            if (pc->level == pc->brace_level)
            {
               log_rule_B("nl_class_brace");
               newlines_do_else(chunk_get_prev_nnl(pc), options::nl_class_brace());
            }
            break;
         }

         case CT_OC_CLASS:
         {
            if (pc->level == pc->brace_level)
            {
               // Request #126
               // introduce two new options
               // look back if we have a @interface or a @implementation
               for (tmp = chunk_get_prev(pc); tmp != nullptr; tmp = chunk_get_prev(tmp))
               {
                  LOG_FMT(LBLANK, "%s(%d): orig_line is %zu, orig_col is %zu, text() is '%s'\n",
                          __func__, __LINE__, tmp->orig_line, tmp->orig_col, tmp->text());

                  if (  chunk_is_token(tmp, CT_OC_INTF)
                     || chunk_is_token(tmp, CT_OC_IMPL))
                  {
                     LOG_FMT(LBLANK, "%s(%d): orig_line is %zu, orig_col is %zu, may be remove/force newline before {\n",
                             __func__, __LINE__, pc->orig_line, pc->orig_col);

                     if (chunk_is_token(tmp, CT_OC_INTF))
                     {
                        log_rule_B("nl_oc_interface_brace");
                        newlines_do_else(chunk_get_prev_nnl(pc), options::nl_oc_interface_brace());
                     }
                     else
                     {
                        log_rule_B("nl_oc_implementation_brace");
                        newlines_do_else(chunk_get_prev_nnl(pc), options::nl_oc_implementation_brace());
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
            log_rule_B("nl_type_brace_init_lst");
            newline_iarf_pair(chunk_get_prev_nnl(pc), pc, options::nl_type_brace_init_lst(), true);
            break;
         }

         case CT_OC_BLOCK_EXPR:
         {
            // issue # 477
            log_rule_B("nl_oc_block_brace");
            newline_iarf_pair(chunk_get_prev(pc), pc, options::nl_oc_block_brace());
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
            next = chunk_get_next_nc(pc, scope_e::PREPROC);

            if (chunk_is_token(next, CT_BRACE_OPEN))
            {
               newline_iarf_pair(pc, next, options::nl_brace_brace());
            }
         }
         next = chunk_get_next_nnl(pc);

         if (next == nullptr)
         {
            // do nothing
         }
         else if (chunk_is_token(next, CT_BRACE_CLOSE))
         {
            // TODO: add an option to split open empty statements? { };
         }
         else if (chunk_is_token(next, CT_BRACE_OPEN))
         {
            // already handled
         }
         else
         {
            next = chunk_get_next_ncnl(pc);

            // Handle unnamed temporary direct-list-initialization
            if (get_chunk_parent_type(pc) == CT_BRACED_INIT_LIST)
            {
               log_rule_B("nl_type_brace_init_lst_open");
               newline_iarf_pair(pc, chunk_get_next_nnl(pc),
                                 options::nl_type_brace_init_lst_open(), true);
            }
            // Handle nl_after_brace_open
            else if (  (  get_chunk_parent_type(pc) == CT_CPP_LAMBDA
                       || pc->level == pc->brace_level)
                    && options::nl_after_brace_open())
            {
               log_rule_B("nl_after_brace_open");

               if (!one_liner_nl_ok(pc))
               {
                  LOG_FMT(LNL1LINE, "a new line may NOT be added\n");
                  // no change - preserve one liner body
               }
               else if (  pc->flags.test(PCF_IN_ARRAY_ASSIGN)
                       || pc->flags.test(PCF_IN_PREPROC))
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
                        log_rule_B("nl_after_brace_open_cmt");

                        if (  !options::nl_after_brace_open_cmt()
                           && tmp->type != CT_COMMENT_MULTI)
                        {
                           break;
                        }
                     }
                     tmp = chunk_get_prev(tmp);
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

         if (!(  get_chunk_parent_type(pc) == CT_BRACED_INIT_LIST
              && options::nl_type_brace_init_lst_open() == IARF_IGNORE
              && options::nl_type_brace_init_lst_close() == IARF_IGNORE))
         {
            newlines_brace_pair(pc);
         }
      }
      else if (chunk_is_token(pc, CT_BRACE_CLOSE))
      {
         // newline between a close brace and x
         log_rule_B("nl_brace_brace");

         if (options::nl_brace_brace() != IARF_IGNORE)
         {
            next = chunk_get_next_nc(pc, scope_e::PREPROC);

            if (chunk_is_token(next, CT_BRACE_CLOSE))
            {
               log_rule_B("nl_brace_brace");
               newline_iarf_pair(pc, next, options::nl_brace_brace());
            }
         }
         log_rule_B("nl_brace_square");

         if (options::nl_brace_square() != IARF_IGNORE)
         {
            next = chunk_get_next_nc(pc, scope_e::PREPROC);

            if (chunk_is_token(next, CT_SQUARE_CLOSE))
            {
               log_rule_B("nl_brace_square");
               newline_iarf_pair(pc, next, options::nl_brace_square());
            }
         }
         log_rule_B("nl_brace_fparen");

         if (options::nl_brace_fparen() != IARF_IGNORE)
         {
            next = chunk_get_next_nc(pc, scope_e::PREPROC);

            log_rule_B("nl_brace_fparen");

            if (  chunk_is_token(next, CT_NEWLINE)
               && (options::nl_brace_fparen() == IARF_REMOVE))
            {
               next = chunk_get_next_nc(next, scope_e::PREPROC);  // Issue #1000
            }

            if (chunk_is_token(next, CT_FPAREN_CLOSE))
            {
               log_rule_B("nl_brace_fparen");
               newline_iarf_pair(pc, next, options::nl_brace_fparen());
            }
         }
         // newline before a close brace
         log_rule_B("nl_type_brace_init_lst_close");

         if (  get_chunk_parent_type(pc) == CT_BRACED_INIT_LIST
            && options::nl_type_brace_init_lst_close() != IARF_IGNORE)
         {
            // Handle unnamed temporary direct-list-initialization
            newline_iarf_pair(chunk_get_prev_nnl(pc), pc,
                              options::nl_type_brace_init_lst_close(), true);
         }

         // blanks before a close brace
         if (options::eat_blanks_before_close_brace())
         {
            // Limit the newlines before the close brace to 1
            prev = chunk_get_prev(pc);

            if (chunk_is_newline(prev))
            {
               log_rule_B("nl_inside_namespace");

               if (options::nl_inside_namespace() && get_chunk_parent_type(pc) == CT_NAMESPACE)
               {
                  blank_line_set(prev, options::nl_inside_namespace);
               }
               else if (prev->nl_count != 1)
               {
                  prev->nl_count = 1;
                  LOG_FMT(LBLANKD, "%s(%d): eat_blanks_before_close_brace %zu\n",
                          __func__, __LINE__, prev->orig_line);
                  MARK_CHANGE();
               }
            }
         }
         else if (  options::nl_ds_struct_enum_close_brace()
                 && (  get_chunk_parent_type(pc) == CT_ENUM
                    || get_chunk_parent_type(pc) == CT_STRUCT
                    || get_chunk_parent_type(pc) == CT_UNION))
         {
            log_rule_B("nl_ds_struct_enum_close_brace");

            if (!pc->flags.test(PCF_ONE_LINER))
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
         log_rule_B("nl_brace_struct_var");

         if (  (options::nl_brace_struct_var() != IARF_IGNORE)
            && (  get_chunk_parent_type(pc) == CT_STRUCT
               || get_chunk_parent_type(pc) == CT_ENUM
               || get_chunk_parent_type(pc) == CT_UNION))
         {
            next = chunk_get_next_ncnl(pc, scope_e::PREPROC);

            if (  next != nullptr
               && next->type != CT_SEMICOLON
               && next->type != CT_COMMA)
            {
               log_rule_B("nl_brace_struct_var");
               newline_iarf(pc, options::nl_brace_struct_var());
            }
         }
         else if (  get_chunk_parent_type(pc) != CT_OC_AT
                 && get_chunk_parent_type(pc) != CT_BRACED_INIT_LIST
                 && (  options::nl_after_brace_close()
                    || get_chunk_parent_type(pc) == CT_FUNC_CLASS_DEF
                    || get_chunk_parent_type(pc) == CT_FUNC_DEF
                    || get_chunk_parent_type(pc) == CT_OC_MSG_DECL))
         {
            log_rule_B("nl_after_brace_close");
            next = chunk_get_next(pc);

            if (  next != nullptr
               && next->type != CT_SEMICOLON
               && next->type != CT_COMMA
               && next->type != CT_SPAREN_CLOSE    // Issue #664
               && next->type != CT_SQUARE_CLOSE
               && next->type != CT_FPAREN_CLOSE
               && next->type != CT_PAREN_CLOSE
               && next->type != CT_WHILE_OF_DO
               && next->type != CT_VBRACE_CLOSE                                      // Issue #666
               && (next->type != CT_BRACE_CLOSE || !next->flags.test(PCF_ONE_LINER)) // #1258
               && !pc->flags.test(PCF_IN_ARRAY_ASSIGN)
               && !pc->flags.test(PCF_IN_TYPEDEF)
               && !chunk_is_newline(next)
               && !chunk_is_comment(next))
            {
               // #1258
               // dont add newline between two consecutive braces closes, if the second is a part of one liner.
               newline_end_newline(pc);
            }
         }
         else if (get_chunk_parent_type(pc) == CT_NAMESPACE)
         {
            log_rule_B("nl_after_namespace");

            if (options::nl_after_namespace() > 0)
            {
               next = chunk_get_next_ncnl(pc, scope_e::PREPROC);

               if (next)
               {
                  newline_add_before(next);
                  // newline_iarf(next, IARF_ADD);
               }
            }
         }
      }
      else if (chunk_is_token(pc, CT_VBRACE_OPEN))
      {
         log_rule_B("nl_after_vbrace_open");
         log_rule_B("nl_after_vbrace_open_empty");

         if (  options::nl_after_vbrace_open()
            || options::nl_after_vbrace_open_empty())
         {
            next = chunk_get_next(pc, scope_e::PREPROC);
            bool add_it;

            if (chunk_is_semicolon(next))
            {
               log_rule_B("nl_after_vbrace_open_empty");
               add_it = options::nl_after_vbrace_open_empty();
            }
            else
            {
               log_rule_B("nl_after_vbrace_open");
               add_it = (  options::nl_after_vbrace_open()
                        && next->type != CT_VBRACE_CLOSE
                        && !chunk_is_comment(next)
                        && !chunk_is_newline(next));
            }

            if (add_it)
            {
               newline_iarf(pc, IARF_ADD);
            }
         }
         log_rule_B("nl_create_if_one_liner");
         log_rule_B("nl_create_for_one_liner");
         log_rule_B("nl_create_while_one_liner");

         if (  (  (  get_chunk_parent_type(pc) == CT_IF
                  || get_chunk_parent_type(pc) == CT_ELSEIF
                  || get_chunk_parent_type(pc) == CT_ELSE)
               && options::nl_create_if_one_liner())
            || (  get_chunk_parent_type(pc) == CT_FOR
               && options::nl_create_for_one_liner())
            || (  get_chunk_parent_type(pc) == CT_WHILE
               && options::nl_create_while_one_liner()))
         {
            nl_create_one_liner(pc);
         }
         log_rule_B("nl_split_if_one_liner");
         log_rule_B("nl_split_for_one_liner");
         log_rule_B("nl_split_while_one_liner");

         if (  (  (  get_chunk_parent_type(pc) == CT_IF
                  || get_chunk_parent_type(pc) == CT_ELSEIF
                  || get_chunk_parent_type(pc) == CT_ELSE)
               && options::nl_split_if_one_liner())
            || (  get_chunk_parent_type(pc) == CT_FOR
               && options::nl_split_for_one_liner())
            || (  get_chunk_parent_type(pc) == CT_WHILE
               && options::nl_split_while_one_liner()))
         {
            if (pc->flags.test(PCF_ONE_LINER))
            {
               // split one-liner
               chunk_t *end = chunk_get_next(chunk_get_next_type(pc->next, CT_SEMICOLON, -1));
               // Scan for clear flag
               D_LOG_FMT(LNEWLINE, "(%d) ", __LINE__);
               LOG_FMT(LNEWLINE, "\n");

               for (chunk_t *temp = pc; temp != end; temp = chunk_get_next(temp))
               {
                  LOG_FMT(LNEWLINE, "%s(%d): text() is '%s', type is %s, level is %zu\n   ",
                          __func__, __LINE__, temp->text(), get_token_name(temp->type), temp->level);
                  log_pcf_flags(LNEWLINE, temp->flags);
                  chunk_flags_clr(temp, PCF_ONE_LINER);
               }

               // split
               newline_add_between(pc, pc->next);
            }
         }
      }
      else if (chunk_is_token(pc, CT_VBRACE_CLOSE))
      {
         log_rule_B("nl_after_vbrace_close");

         if (options::nl_after_vbrace_close())
         {
            if (!chunk_is_newline(chunk_get_next_nc(pc)))
            {
               newline_iarf(pc, IARF_ADD);
            }
         }
      }
      else if (chunk_is_token(pc, CT_SQUARE_OPEN) && get_chunk_parent_type(pc) == CT_OC_MSG)
      {
         log_rule_B("nl_oc_msg_args");

         if (options::nl_oc_msg_args())
         {
            newline_oc_msg(pc);
         }
      }
      else if (chunk_is_token(pc, CT_STRUCT))
      {
         log_rule_B("nl_struct_brace");
         newlines_struct_union(pc, options::nl_struct_brace(), true);
      }
      else if (chunk_is_token(pc, CT_UNION))
      {
         log_rule_B("nl_union_brace");
         newlines_struct_union(pc, options::nl_union_brace(), true);
      }
      else if (chunk_is_token(pc, CT_ENUM))
      {
         newlines_enum(pc);
      }
      else if (chunk_is_token(pc, CT_CASE))
      {
         // Note: 'default' also maps to CT_CASE
         log_rule_B("nl_before_case");

         if (options::nl_before_case())
         {
            newline_case(pc);
         }
      }
      else if (chunk_is_token(pc, CT_THROW))
      {
         prev = chunk_get_prev(pc);

         if (  chunk_is_token(prev, CT_PAREN_CLOSE)
            || chunk_is_token(prev, CT_FPAREN_CLOSE))         // Issue #1122
         {
            log_rule_B("nl_before_throw");
            newline_iarf(chunk_get_prev_ncnlni(pc), options::nl_before_throw());   // Issue #2279
         }
      }
      else if (  chunk_is_token(pc, CT_QUALIFIER)
              && !strcmp(pc->text(), "throws"))
      {
         prev = chunk_get_prev(pc);

         if (  chunk_is_token(prev, CT_PAREN_CLOSE)
            || chunk_is_token(prev, CT_FPAREN_CLOSE))         // Issue #1122
         {
            log_rule_B("nl_before_throw");
            newline_iarf(chunk_get_prev_ncnlni(pc), options::nl_before_throw());   // Issue #2279
         }
      }
      else if (chunk_is_token(pc, CT_CASE_COLON))
      {
         next = chunk_get_next_nnl(pc);

         log_rule_B("nl_case_colon_brace");

         if (  chunk_is_token(next, CT_BRACE_OPEN)
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
      else if (chunk_is_token(pc, CT_SPAREN_CLOSE))
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
      else if (chunk_is_token(pc, CT_RETURN))
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
      else if (chunk_is_token(pc, CT_SEMICOLON))
      {
         log_rule_B("nl_after_semicolon");

         if (  !pc->flags.test(PCF_IN_SPAREN)
            && !pc->flags.test(PCF_IN_PREPROC)
            && options::nl_after_semicolon())
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
                  newline_iarf(pc, IARF_ADD);
               }
               else
               {
                  LOG_FMT(LNL1LINE, "%s(%d): a new line may NOT be added\n", __func__, __LINE__);
               }
            }
         }
         else if (get_chunk_parent_type(pc) == CT_CLASS)
         {
            log_rule_B("nl_after_class");

            if (options::nl_after_class() > 0)
            {
               newline_iarf(pc, IARF_ADD);
            }
         }
      }
      else if (chunk_is_token(pc, CT_FPAREN_OPEN))
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

         if (  (  (  get_chunk_parent_type(pc) == CT_FUNC_DEF
                  || get_chunk_parent_type(pc) == CT_FUNC_PROTO
                  || get_chunk_parent_type(pc) == CT_FUNC_CLASS_DEF
                  || get_chunk_parent_type(pc) == CT_FUNC_CLASS_PROTO
                  || get_chunk_parent_type(pc) == CT_OPERATOR)
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

            || (  get_chunk_parent_type(pc) == CT_FUNC_CALL                // Issue #2604
               && options::nl_func_call_args() != IARF_IGNORE))
         {
            newline_func_def_or_call(pc);
         }
         else if (  (  get_chunk_parent_type(pc) == CT_FUNC_CALL                // Issue #2020
                    || get_chunk_parent_type(pc) == CT_FUNC_CALL_USER)
                 && options::nl_func_call_start() != IARF_IGNORE)
         {
            log_rule_B("nl_func_call_start");
            newline_iarf(pc, options::nl_func_call_start());
         }
         else if (  (  get_chunk_parent_type(pc) == CT_FUNC_CALL
                    || get_chunk_parent_type(pc) == CT_FUNC_CALL_USER)
                 && (  (options::nl_func_call_start_multi_line())
                    || (options::nl_func_call_args_multi_line())
                    || (options::nl_func_call_end_multi_line())
                    || (options::nl_func_call_paren() != IARF_IGNORE)
                    || (options::nl_func_call_paren_empty() != IARF_IGNORE)
                    || (options::nl_func_call_empty() != IARF_IGNORE)))
         {
            log_rule_B("nl_func_call_start_multi_line");
            log_rule_B("nl_func_call_args_multi_line");
            log_rule_B("nl_func_call_end_multi_line");
            log_rule_B("nl_func_call_paren");
            log_rule_B("nl_func_call_paren_empty");
            log_rule_B("nl_func_call_empty");

            if (  options::nl_func_call_paren() != IARF_IGNORE
               || options::nl_func_call_paren_empty() != IARF_IGNORE
               || options::nl_func_call_empty() != IARF_IGNORE)
            {
               newline_func_def_or_call(pc);
            }
            newline_func_multi_line(pc);
         }
         else if (first && (options::nl_remove_extra_newlines() == 1))
         {
            log_rule_B("nl_remove_extra_newlines");
            newline_iarf(pc, IARF_REMOVE);
         }
      }
      else if (chunk_is_token(pc, CT_ANGLE_CLOSE))
      {
         if (get_chunk_parent_type(pc) == CT_TEMPLATE)
         {
            next = chunk_get_next_ncnl(pc);

            if (next != nullptr && next->level == next->brace_level)
            {
               tmp = chunk_get_prev_ncnlni(chunk_get_prev_type(pc, CT_ANGLE_OPEN, pc->level));   // Issue #2279

               if (chunk_is_token(tmp, CT_TEMPLATE))
               {
                  if (chunk_is_token(next, CT_USING))
                  {
                     newline_iarf(pc, options::nl_template_using());
                     log_rule_B("nl_template_using");
                  }
                  else if (get_chunk_parent_type(next) == CT_FUNC_DEF) // function definition
                  {
                     auto const action =
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
                  else if (get_chunk_parent_type(next) == CT_FUNC_PROTO) // function declaration
                  {
                     auto const action =
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
                  else if (  chunk_is_token(next, CT_TYPE)
                          || chunk_is_token(next, CT_QUALIFIER)) // variable
                  {
                     newline_iarf(pc, options::nl_template_var());
                     log_rule_B("nl_template_var");
                  }
                  else if (next->flags.test(PCF_INCOMPLETE)) // class declaration
                  {
                     auto const action =
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
                     auto const action =
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
      else if (  chunk_is_token(pc, CT_NAMESPACE)
              && get_chunk_parent_type(pc) != CT_USING)
      {
         // Issue #2387
         next = chunk_get_next_ncnl(pc);

         if (next != nullptr)
         {
            next = chunk_get_next_ncnl(next);

            if (!chunk_is_token(next, CT_ASSIGN))
            {
               // Issue #1235
               // Issue #2186
               chunk_t *braceOpen = chunk_get_next_type(pc, CT_BRACE_OPEN, pc->level);

               if (braceOpen == nullptr)
               {
                  // fatal error
                  LOG_FMT(LERR, "%s(%d): Missing BRACE_OPEN after namespace\n   orig_line is %zu, orig_col is %zu\n",
                          __func__, __LINE__, pc->orig_line, pc->orig_col);
                  exit(EXIT_FAILURE);
               }
               LOG_FMT(LNEWLINE, "%s(%d): braceOpen->orig_line is %zu, orig_col is %zu, text() is '%s'\n",
                       __func__, __LINE__, braceOpen->orig_line, braceOpen->orig_col, braceOpen->text());
               log_pcf_flags(LNEWLINE, braceOpen->flags);
               newlines_namespace(pc);
            }
         }
      }
      else if (chunk_is_token(pc, CT_SQUARE_OPEN))
      {
         if (get_chunk_parent_type(pc) == CT_ASSIGN && !pc->flags.test(PCF_ONE_LINER))
         {
            tmp = chunk_get_prev_ncnlni(pc);   // Issue #2279
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
      else if (chunk_is_token(pc, CT_ACCESS))
      {
         // Make sure there is a newline before an access spec
         if (options::nl_before_access_spec() > 0)
         {
            log_rule_B("nl_before_access_spec");
            prev = chunk_get_prev(pc);

            if (!chunk_is_newline(prev))
            {
               newline_add_before(pc);
            }
         }
      }
      else if (chunk_is_token(pc, CT_ACCESS_COLON))
      {
         // Make sure there is a newline after an access spec
         if (options::nl_after_access_spec() > 0)
         {
            log_rule_B("nl_after_access_spec");
            next = chunk_get_next(pc);

            if (!chunk_is_newline(next))
            {
               newline_add_before(next);
            }
         }
      }
      else if (chunk_is_token(pc, CT_PP_DEFINE))
      {
         if (options::nl_multi_line_define())
         {
            log_rule_B("nl_multi_line_define");
            nl_handle_define(pc);
         }
      }
      else if (  first
              && (options::nl_remove_extra_newlines() == 1)
              && !pc->flags.test(PCF_IN_PREPROC))
      {
         log_rule_B("nl_remove_extra_newlines");
         newline_iarf(pc, IARF_REMOVE);
      }
      else if (  chunk_is_token(pc, CT_MEMBER)
              && (  language_is_set(LANG_JAVA)
                 || language_is_set(LANG_CPP)))                 // Issue #2574
      {
         // Issue #1124
         if (pc->parent_type != CT_FUNC_DEF)
         {
            newline_iarf(chunk_get_prev_nnl(pc), options::nl_before_member());
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

   newline_def_blk(chunk_get_head(), false);
} // newlines_cleanup_braces


static void nl_handle_define(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *nl  = pc;
   chunk_t *ref = nullptr;

   while ((nl = chunk_get_next(nl)) != nullptr)
   {
      if (chunk_is_token(nl, CT_NEWLINE))
      {
         return;
      }

      if (  chunk_is_token(nl, CT_MACRO)
         || (chunk_is_token(nl, CT_FPAREN_CLOSE) && get_chunk_parent_type(nl) == CT_MACRO_FUNC))
      {
         ref = nl;
      }

      if (chunk_is_token(nl, CT_NL_CONT))
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


static bool is_class_one_liner(chunk_t *pc)
{
   if (  (  chunk_is_token(pc, CT_FUNC_CLASS_DEF)
         || chunk_is_token(pc, CT_FUNC_DEF))
      && pc->flags.test(PCF_IN_CLASS))
   {
      // Find opening brace
      pc = chunk_get_next_type(pc, CT_BRACE_OPEN, pc->level);
      return(pc != nullptr && pc->flags.test(PCF_ONE_LINER));
   }
   return(false);
}


void newlines_insert_blank_lines(void)
{
   LOG_FUNC_ENTRY();

   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next_ncnl(pc))
   {
      //LOG_FMT(LNEWLINE, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s', type is %s\n",
      //        __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
      if (chunk_is_token(pc, CT_IF))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_before_if());
         log_rule_B("nl_before_if");
         newlines_if_for_while_switch_post_blank_lines(pc, options::nl_after_if());
         log_rule_B("nl_after_if");
      }
      else if (chunk_is_token(pc, CT_FOR))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_before_for());
         log_rule_B("nl_before_for");
         newlines_if_for_while_switch_post_blank_lines(pc, options::nl_after_for());
         log_rule_B("nl_after_for");
      }
      else if (chunk_is_token(pc, CT_WHILE))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_before_while());
         log_rule_B("nl_before_while");
         newlines_if_for_while_switch_post_blank_lines(pc, options::nl_after_while());
         log_rule_B("nl_after_while");
      }
      else if (chunk_is_token(pc, CT_SWITCH))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_before_switch());
         log_rule_B("nl_before_switch");
         newlines_if_for_while_switch_post_blank_lines(pc, options::nl_after_switch());
         log_rule_B("nl_after_switch");
      }
      else if (chunk_is_token(pc, CT_SYNCHRONIZED))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_before_synchronized());
         log_rule_B("nl_before_synchronized");
         newlines_if_for_while_switch_post_blank_lines(pc, options::nl_after_synchronized());
         log_rule_B("nl_after_synchronized");
      }
      else if (chunk_is_token(pc, CT_DO))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_before_do());
         log_rule_B("nl_before_do");
         newlines_if_for_while_switch_post_blank_lines(pc, options::nl_after_do());
         log_rule_B("nl_after_do");
      }
      else if (chunk_is_token(pc, CT_OC_INTF))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_oc_before_interface());
         log_rule_B("nl_oc_before_interface");
      }
      else if (chunk_is_token(pc, CT_OC_END))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_oc_before_end());
         log_rule_B("nl_oc_before_end");
      }
      else if (chunk_is_token(pc, CT_OC_IMPL))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_oc_before_implementation());
         log_rule_B("nl_oc_before_implementation");
      }
      else if (  chunk_is_token(pc, CT_FUNC_CLASS_DEF)
              || chunk_is_token(pc, CT_FUNC_DEF)
              || chunk_is_token(pc, CT_FUNC_CLASS_PROTO)
              || chunk_is_token(pc, CT_FUNC_PROTO))
      {
         if (  options::nl_class_leave_one_liner_groups()
            && is_class_one_liner(pc))
         {
            log_rule_B("nl_class_leave_one_liner_groups");
            newlines_func_pre_blank_lines(pc, CT_FUNC_PROTO);
         }
         else
         {
            newlines_func_pre_blank_lines(pc, pc->type);
         }
      }
      else
      {
         // ignore it
         //LOG_FMT(LNEWLINE, "%s(%d): ignore it\n", __func__, __LINE__);
      }
   }
} // newlines_insert_blank_lines


void newlines_functions_remove_extra_blank_lines(void)
{
   LOG_FUNC_ENTRY();

   const size_t nl_max_blank_in_func = options::nl_max_blank_in_func();

   log_rule_B("nl_max_blank_in_func");

   if (nl_max_blank_in_func == 0)
   {
      return;
   }

   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next(pc))
   {
      if (  pc->type != CT_BRACE_OPEN
         || (get_chunk_parent_type(pc) != CT_FUNC_DEF && get_chunk_parent_type(pc) != CT_CPP_LAMBDA))
      {
         continue;
      }
      const size_t startMoveLevel = pc->level;

      while (pc != nullptr)
      {
         if (chunk_is_token(pc, CT_BRACE_CLOSE) && pc->level == startMoveLevel)
         {
            break;
         }

         // delete newlines
         if (  !chunk_is_token(pc, CT_COMMENT_MULTI)   // Issue #2195
            && pc->nl_count > nl_max_blank_in_func)
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
} // newlines_functions_remove_extra_blank_lines


void newlines_squeeze_ifdef(void)
{
   LOG_FUNC_ENTRY();

   chunk_t *pc;

   for (pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next_ncnl(pc))
   {
      if (  chunk_is_token(pc, CT_PREPROC)
         && (pc->level > 0 || options::nl_squeeze_ifdef_top_level()))
      {
         log_rule_B("nl_squeeze_ifdef_top_level");
         chunk_t *ppr = chunk_get_next(pc);

         if (  chunk_is_token(ppr, CT_PP_IF)
            || chunk_is_token(ppr, CT_PP_ELSE)
            || chunk_is_token(ppr, CT_PP_ENDIF))
         {
            chunk_t *pnl = nullptr;
            chunk_t *nnl = chunk_get_next_nl(ppr);

            if (chunk_is_token(ppr, CT_PP_ELSE) || chunk_is_token(ppr, CT_PP_ENDIF))
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

               if (chunk_is_token(ppr, CT_PP_IF) || chunk_is_token(ppr, CT_PP_ELSE))
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

      if (chunk_is_token(pc, CT_NEWLINE))
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
               if (chunk_is_token(pc, CT_NEWLINE))
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
      && (  (options::nl_start_of_file() & IARF_REMOVE)
         || (  (options::nl_start_of_file() & IARF_ADD)
            && (options::nl_start_of_file_min() > 0))))
   {
      log_rule_B("nl_start_of_file");
      log_rule_B("nl_start_of_file_min");
      pc = chunk_get_head();

      if (pc != nullptr)
      {
         if (chunk_is_token(pc, CT_NEWLINE))
         {
            if (options::nl_start_of_file() == IARF_REMOVE)
            {
               log_rule_B("nl_start_of_file");
               LOG_FMT(LBLANKD, "%s(%d): eat_blanks_start_of_file %zu\n",
                       __func__, __LINE__, pc->orig_line);
               chunk_del(pc);
               MARK_CHANGE();
            }
            else if (  options::nl_start_of_file() == IARF_FORCE
                    || (pc->nl_count < options::nl_start_of_file_min()))
            {
               log_rule_B("nl_start_of_file");
               LOG_FMT(LBLANKD, "%s(%d): set_blanks_start_of_file %zu\n",
                       __func__, __LINE__, pc->orig_line);
               pc->nl_count = options::nl_start_of_file_min();
               log_rule_B("nl_start_of_file_min");
               MARK_CHANGE();
            }
         }
         else if (  (options::nl_start_of_file() & IARF_ADD)
                 && (options::nl_start_of_file_min() > 0))
         {
            log_rule_B("nl_start_of_file");
            log_rule_B("nl_start_of_file_min");
            chunk_t chunk;
            set_chunk_type(&chunk, CT_NEWLINE);
            chunk.orig_line = pc->orig_line;
            chunk.orig_col  = pc->orig_col;
            chunk.nl_count  = options::nl_start_of_file_min();
            log_rule_B("nl_start_of_file_min");
            chunk_add_before(&chunk, pc);
            LOG_FMT(LNEWLINE, "%s(%d): %zu:%zu add newline before '%s'\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
            MARK_CHANGE();
         }
      }
   }

   // Process newlines at the end of the file
   if (  cpd.frag_cols == 0
      && (  (options::nl_end_of_file() & IARF_REMOVE)
         || (  (options::nl_end_of_file() & IARF_ADD)
            && (options::nl_end_of_file_min() > 0))))
   {
      log_rule_B("nl_end_of_file");
      log_rule_B("nl_end_of_file_min");
      pc = chunk_get_tail();

      if (pc != nullptr)
      {
         if (chunk_is_token(pc, CT_NEWLINE))
         {
            if (options::nl_end_of_file() == IARF_REMOVE)
            {
               log_rule_B("nl_end_of_file");
               LOG_FMT(LBLANKD, "%s(%d): eat_blanks_end_of_file %zu\n",
                       __func__, __LINE__, pc->orig_line);
               chunk_del(pc);
               MARK_CHANGE();
            }
            else if (  options::nl_end_of_file() == IARF_FORCE
                    || (pc->nl_count < options::nl_end_of_file_min()))
            {
               log_rule_B("nl_end_of_file");
               log_rule_B("nl_end_of_file_min");

               if (pc->nl_count != options::nl_end_of_file_min())
               {
                  log_rule_B("nl_end_of_file_min");
                  LOG_FMT(LBLANKD, "%s(%d): set_blanks_end_of_file %zu\n",
                          __func__, __LINE__, pc->orig_line);
                  pc->nl_count = options::nl_end_of_file_min();
                  log_rule_B("nl_end_of_file_min");
                  MARK_CHANGE();
               }
            }
         }
         else if (  (options::nl_end_of_file() & IARF_ADD)
                 && (options::nl_end_of_file_min() > 0))
         {
            log_rule_B("nl_end_of_file");
            log_rule_B("nl_end_of_file_min");
            chunk_t chunk;
            set_chunk_type(&chunk, CT_NEWLINE);
            chunk.orig_line = pc->orig_line;
            chunk.orig_col  = pc->orig_col;
            chunk.nl_count  = options::nl_end_of_file_min();
            log_rule_B("nl_end_of_file_min");
            chunk_add_before(&chunk, nullptr);
            LOG_FMT(LNEWLINE, "%s(%d): %zu:%zu add newline before '%s'\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
            MARK_CHANGE();
         }
      }
   }
} // newlines_eat_start_end


void newlines_chunk_pos(c_token_t chunk_type, token_pos_e mode)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNEWLINE, "%s(%d): mode is %s\n",
           __func__, __LINE__, to_string(mode));

   if (  !(mode & (TP_JOIN | TP_LEAD | TP_TRAIL))
      && chunk_type != CT_COMMA)
   {
      return;
   }

   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next_ncnl(pc))
   {
      LOG_FMT(LNEWLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, text() is '%s'\n   ",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
      log_pcf_flags(LNEWLINE, pc->flags);

      if (pc->type == chunk_type)
      {
         token_pos_e mode_local;

         if (chunk_type == CT_COMMA)
         {
            LOG_FMT(LNEWLINE, "%s(%d): orig_line is %zu, orig_col is %zu, text() is '%s', type is %s\n   ",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
            log_pcf_flags(LNEWLINE, pc->flags);

            if (pc->flags.test(PCF_IN_CONST_ARGS)) // Issue #2250
            {
               continue;
            }

            /*
             * for chunk_type == CT_COMMA
             * we get 'mode' from options::pos_comma()
             * BUT we must take care of options::pos_class_comma()
             * TODO and options::pos_constr_comma()
             */
            if (pc->flags.test(PCF_IN_CLASS_BASE))
            {
               // change mode
               mode_local = options::pos_class_comma();
            }
            else if (pc->flags.test(PCF_IN_ENUM))
            {
               mode_local = options::pos_enum_comma();
            }
            else
            {
               mode_local = mode;
            }
            LOG_FMT(LNEWLINE, "%s(%d): mode_local is %s\n",
                    __func__, __LINE__, to_string(mode_local));
         }
         else
         {
            mode_local = mode;
         }
         chunk_t *prev = chunk_get_prev_nc(pc);
         chunk_t *next = chunk_get_next_nc(pc);

         LOG_FMT(LNEWLINE, "%s(%d): mode_local is %s\n",
                 __func__, __LINE__, to_string(mode_local));

         LOG_FMT(LNEWLINE, "%s(%d): prev->orig_line is %zu, orig_col is %zu, text() is '%s'\n",
                 __func__, __LINE__, prev->orig_line, prev->orig_col, prev->text());
         LOG_FMT(LNEWLINE, "%s(%d): next->orig_line is %zu, orig_col is %zu, text() is '%s'\n",
                 __func__, __LINE__, next->orig_line, next->orig_col, next->text());
         size_t nl_flag = ((chunk_is_newline(prev) ? 1 : 0) |
                           (chunk_is_newline(next) ? 2 : 0));
         LOG_FMT(LNEWLINE, "%s(%d): nl_flag is %zu\n",
                 __func__, __LINE__, nl_flag);

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

         if (  (nl_flag == 0 && !(mode_local & (TP_FORCE | TP_BREAK)))
            || (nl_flag == 3 && !(mode_local & TP_FORCE)))
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
               remove_next_newlines(chunk_get_prev_ncnlni(pc));   // Issue #2279
            }
            continue;
         }

         // we need to move the newline
         if (mode_local & TP_LEAD)
         {
            chunk_t *next2 = chunk_get_next(next);

            if (  chunk_is_token(next2, CT_PREPROC)
               || (  chunk_type == CT_ASSIGN
                  && chunk_is_token(next2, CT_BRACE_OPEN)))
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
            LOG_FMT(LNEWLINE, "%s(%d): prev->orig_line is %zu, orig_col is %zu, text() is '%s', nl_count is %zu\n",
                    __func__, __LINE__, prev->orig_line, prev->orig_col, prev->text(), prev->nl_count);

            if (prev->nl_count == 1)
            {
               // Back up to the next non-comment item
               prev = chunk_get_prev_nc(prev);
               LOG_FMT(LNEWLINE, "%s(%d): prev->orig_line is %zu, orig_col is %zu, text() is '%s'\n",
                       __func__, __LINE__, prev->orig_line, prev->orig_col, prev->text());

               if (  prev != nullptr
                  && !chunk_is_newline(prev)
                  && !prev->flags.test(PCF_IN_PREPROC)
                  && !prev->flags.test(PCF_IN_OC_MSG))
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

   token_pos_e tpc;
   token_pos_e pcc;
   iarf_e      anc;
   iarf_e      ncia;

   if (tok == CT_CLASS_COLON)
   {
      tpc = options::pos_class_colon();
      log_rule_B("pos_class_colon");
      anc = options::nl_class_colon();
      log_rule_B("nl_class_colon");
      ncia = options::nl_class_init_args();
      log_rule_B("nl_class_init_args");
      pcc = options::pos_class_comma();
      log_rule_B("pos_class_comma");
   }
   else // tok == CT_CONSTR_COLON
   {
      tpc = options::pos_constr_colon();
      log_rule_B("pos_constr_colon");
      anc = options::nl_constr_colon();
      log_rule_B("nl_constr_colon");
      ncia = options::nl_constr_init_args();
      log_rule_B("nl_constr_init_args");
      pcc = options::pos_constr_comma();
      log_rule_B("pos_constr_comma");
   }
   chunk_t *ccolon  = nullptr;
   size_t  acv_span = options::align_constr_value_span();

   log_rule_B("align_constr_value_span");
   bool       with_acv = (acv_span > 0) && language_is_set(LANG_CPP);
   AlignStack constructorValue;    // ABC_Member(abc_value)

   if (with_acv)
   {
      int    acv_thresh = options::align_constr_value_thresh();
      log_rule_B("align_constr_value_thresh");
      size_t acv_gap = options::align_constr_value_gap();
      log_rule_B("align_constr_value_gap");
      constructorValue.Start(acv_span, acv_thresh);
      constructorValue.m_gap         = acv_gap;
      constructorValue.m_right_align = !options::align_on_tabstop();
      log_rule_B("align_on_tabstop");
   }

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
         LOG_FMT(LBLANKD, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s', type is %s\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
         ccolon = pc;
         prev   = chunk_get_prev_nc(pc);
         next   = chunk_get_next_nc(pc);

         if (chunk_is_token(pc, CT_CONSTR_COLON))
         {
            LOG_FMT(LBLANKD, "%s(%d): pc->orig_line is %zu, orig_col is %zu, text() '%s', type is %s\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
            chunk_t *paren_vor_value = chunk_get_next_type(pc, CT_FPAREN_OPEN, pc->level);

            if (with_acv && paren_vor_value != nullptr)
            {
               LOG_FMT(LBLANKD, "%s(%d): paren_vor_value->orig_line is %zu, orig_col is %zu, text() '%s', type is %s\n",
                       __func__, __LINE__, paren_vor_value->orig_line, paren_vor_value->orig_col,
                       paren_vor_value->text(), get_token_name(paren_vor_value->type));
               constructorValue.NewLines(paren_vor_value->nl_count);
               constructorValue.Add(paren_vor_value);
            }
         }

         if (  !chunk_is_newline(prev)
            && !chunk_is_newline(next)
            && (anc & IARF_ADD))
         {
            newline_add_after(pc);
            prev = chunk_get_prev_nc(pc);
            next = chunk_get_next_nc(pc);
         }

         if (anc == IARF_REMOVE)
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
         if (chunk_is_token(pc, CT_BRACE_OPEN) || chunk_is_token(pc, CT_SEMICOLON))
         {
            ccolon = nullptr;

            if (with_acv)
            {
               constructorValue.End();
            }
            continue;
         }

         if (chunk_is_token(pc, CT_COMMA) && pc->level == ccolon->level)
         {
            LOG_FMT(LBLANKD, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s', type is %s\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
            chunk_t *paren_vor_value = chunk_get_next_type(pc, CT_FPAREN_OPEN, pc->level);

            if (with_acv && paren_vor_value != nullptr)
            {
               LOG_FMT(LBLANKD, "%s(%d): paren_vor_value->orig_line is %zu, orig_col is %zu, text() '%s', type is %s\n",
                       __func__, __LINE__, paren_vor_value->orig_line, paren_vor_value->orig_col,
                       paren_vor_value->text(), get_token_name(paren_vor_value->type));
               constructorValue.NewLines(paren_vor_value->nl_count);
               constructorValue.Add(paren_vor_value);
            }

            if (ncia & IARF_ADD)
            {
               if (pcc & TP_TRAIL)
               {
                  if (ncia == IARF_FORCE)
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
                  if (ncia == IARF_FORCE)
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
            else if (ncia == IARF_REMOVE)
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


static void blank_line_max(chunk_t *pc, Option<unsigned> &opt)
{
   LOG_FUNC_ENTRY();

   if (pc == nullptr)
   {
      return;
   }
   const auto optval = opt();

   if ((optval > 0) && (pc->nl_count > optval))
   {
      LOG_FMT(LBLANKD, "%s(%d): do_blank_lines: %s max line %zu\n",
              __func__, __LINE__, opt.name(), pc->orig_line);
      pc->nl_count = optval;
      MARK_CHANGE();
   }
}


iarf_e newline_template_option(chunk_t *pc, iarf_e special, iarf_e base, iarf_e fallback)
{
   auto *const prev = chunk_get_prev_ncnl(pc);

   if (chunk_is_token(prev, CT_ANGLE_OPEN) && special != IARF_IGNORE)
   {
      return(special);
   }
   else if (base != IARF_IGNORE)
   {
      return(base);
   }
   else
   {
      return(fallback);
   }
}


bool is_func_proto_group(chunk_t *pc, c_token_t one_liner_type)
{
   if (  pc && options::nl_class_leave_one_liner_groups()
      && (pc->type == one_liner_type || get_chunk_parent_type(pc) == one_liner_type)
      && pc->flags.test(PCF_IN_CLASS))
   {
      log_rule_B("nl_class_leave_one_liner_groups");

      if (pc->type == CT_BRACE_CLOSE)
      {
         return(pc->flags.test(PCF_ONE_LINER));
      }
      else
      {
         // Find opening brace
         pc = chunk_get_next_type(pc, CT_BRACE_OPEN, pc->level);
         return(pc != nullptr && pc->flags.test(PCF_ONE_LINER));
      }
   }
   return(false);
}


void do_blank_lines(void)
{
   LOG_FUNC_ENTRY();

   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next(pc))
   {
      if (chunk_is_token(pc, CT_NEWLINE))
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

         if (chunk_is_token(prev, CT_IGNORED))
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
      if (  (options::nl_max() > 0)
         && (pc->nl_count > options::nl_max()))
      {
         log_rule_B("nl_max");
         blank_line_max(pc, options::nl_max);
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
      if (  (options::nl_before_block_comment() > pc->nl_count)
         && chunk_is_token(next, CT_COMMENT_MULTI))
      {
         log_rule_B("nl_before_block_comment");

         // Don't add blanks after a open brace
         if (  (  prev == nullptr
               || (  prev->type != CT_BRACE_OPEN
                  && prev->type != CT_VBRACE_OPEN))
            && pcmt != nullptr                          // Issue #2383
            && pcmt->type != CT_COMMENT_MULTI)
         {
            blank_line_set(pc, options::nl_before_block_comment);
            log_rule_B("nl_before_block_comment");
         }
      }

      // Control blanks before single line C comments
      if (  (options::nl_before_c_comment() > pc->nl_count)
         && chunk_is_token(next, CT_COMMENT))
      {
         log_rule_B("nl_before_c_comment");

         // Don't add blanks after a open brace or a comment
         if (  (  prev == nullptr
               || (  prev->type != CT_BRACE_OPEN
                  && prev->type != CT_VBRACE_OPEN))
            && pcmt != nullptr                          // Issue #2383
            && pcmt->type != CT_COMMENT)
         {
            blank_line_set(pc, options::nl_before_c_comment);
            log_rule_B("nl_before_c_comment");
         }
      }

      // Control blanks before CPP comments
      if (  (options::nl_before_cpp_comment() > pc->nl_count)
         && chunk_is_token(next, CT_COMMENT_CPP))
      {
         log_rule_B("nl_before_cpp_comment");

         // Don't add blanks after a open brace
         if (  (  prev == nullptr
               || (  prev->type != CT_BRACE_OPEN
                  && prev->type != CT_VBRACE_OPEN))
            && pcmt != nullptr                          // Issue #2383
            && pcmt->type != CT_COMMENT_CPP)
         {
            blank_line_set(pc, options::nl_before_cpp_comment);
            log_rule_B("nl_before_cpp_comment");
         }
      }

      // Control blanks before an access spec
      if (  (options::nl_before_access_spec() > 0)
         && (options::nl_before_access_spec() != pc->nl_count)
         && chunk_is_token(next, CT_ACCESS))
      {
         log_rule_B("nl_before_access_spec");

         // Don't add blanks after a open brace
         if (  prev == nullptr
            || (prev->type != CT_BRACE_OPEN && prev->type != CT_VBRACE_OPEN))
         {
            blank_line_set(pc, options::nl_before_access_spec);
            log_rule_B("nl_before_access_spec");
         }
      }

      // Control blanks before a class
      if (  (chunk_is_token(prev, CT_SEMICOLON) || chunk_is_token(prev, CT_BRACE_CLOSE))
         && get_chunk_parent_type(prev) == CT_CLASS)
      {
         chunk_t *tmp = chunk_get_prev_type(prev, CT_CLASS, prev->level);
         tmp = chunk_get_prev_nc(tmp);

         while (  chunk_is_token(tmp, CT_NEWLINE)
               && chunk_is_comment(tmp->prev))
         {
            tmp = chunk_get_prev_nc(tmp->prev);
         }

         if (tmp != nullptr && options::nl_before_class() > tmp->nl_count)
         {
            log_rule_B("nl_before_class");
            blank_line_set(tmp, options::nl_before_class);
         }
      }

      if (  chunk_is_token(prev, CT_BRACE_CLOSE)
         && get_chunk_parent_type(prev) == CT_NAMESPACE)
      {
         // Control blanks before a namespace
         chunk_t *tmp = chunk_get_prev_type(prev, CT_NAMESPACE, prev->level);
         tmp = chunk_get_prev_nc(tmp);

         while (  chunk_is_token(tmp, CT_NEWLINE)
               && chunk_is_comment(tmp->prev))
         {
            tmp = chunk_get_prev_nc(tmp->prev);
         }

         if (tmp != nullptr && options::nl_before_namespace() > tmp->nl_count)
         {
            log_rule_B("nl_before_namespace");
            blank_line_set(tmp, options::nl_before_namespace);
         }

         // Add blanks after namespace
         if (options::nl_after_namespace() > pc->nl_count)
         {
            log_rule_B("nl_after_namespace");
            blank_line_set(pc, options::nl_after_namespace);
         }
      }

      // Control blanks after an access spec
      if (  (options::nl_after_access_spec() > 0)
         && (options::nl_after_access_spec() != pc->nl_count)
         && chunk_is_token(prev, CT_ACCESS_COLON))
      {
         log_rule_B("nl_after_access_spec");
         blank_line_set(pc, options::nl_after_access_spec);
      }

      // Add blanks after function bodies
      if (  chunk_is_token(prev, CT_BRACE_CLOSE)
         && (  get_chunk_parent_type(prev) == CT_FUNC_DEF
            || get_chunk_parent_type(prev) == CT_FUNC_CLASS_DEF
            || get_chunk_parent_type(prev) == CT_OC_MSG_DECL
            || get_chunk_parent_type(prev) == CT_ASSIGN))
      {
         if (prev->flags.test(PCF_ONE_LINER))
         {
            if (options::nl_after_func_body_one_liner() > pc->nl_count)
            {
               log_rule_B("nl_after_func_body_one_liner");
               blank_line_set(pc, options::nl_after_func_body_one_liner);
            }
         }
         else
         {
            if (  prev->flags.test(PCF_IN_CLASS)
               && (options::nl_after_func_body_class() > 0))
            {
               log_rule_B("nl_after_func_body_class");

               if (options::nl_after_func_body_class() != pc->nl_count)
               {
                  log_rule_B("nl_after_func_body_class");
                  blank_line_set(pc, options::nl_after_func_body_class);
               }
            }
            else if (options::nl_after_func_body() > 0)
            {
               log_rule_B("nl_after_func_body");

               // Issue #1734
               if (!(pc->prev->flags.test(PCF_IN_TRY_BLOCK)))
               {
                  if (options::nl_after_func_body() != pc->nl_count)
                  {
                     log_rule_B("nl_after_func_body");
                     blank_line_set(pc, options::nl_after_func_body);
                  }
               }
            }
         }
      }

      // Add blanks after function prototypes
      if (  (  chunk_is_token(prev, CT_SEMICOLON)
            && get_chunk_parent_type(prev) == CT_FUNC_PROTO)
         || is_func_proto_group(prev, CT_FUNC_DEF))
      {
         if (options::nl_after_func_proto() > pc->nl_count)
         {
            log_rule_B("nl_after_func_proto");
            pc->nl_count = options::nl_after_func_proto();
            MARK_CHANGE();
         }

         if (  (options::nl_after_func_proto_group() > pc->nl_count)
            && next != nullptr
            && get_chunk_parent_type(next) != CT_FUNC_PROTO
            && !is_func_proto_group(next, CT_FUNC_DEF))
         {
            log_rule_B("nl_after_func_proto_group");
            blank_line_set(pc, options::nl_after_func_proto_group);
         }
      }

      // Issue #411: Add blanks after function class prototypes
      if (  (  chunk_is_token(prev, CT_SEMICOLON)
            && get_chunk_parent_type(prev) == CT_FUNC_CLASS_PROTO)
         || is_func_proto_group(prev, CT_FUNC_CLASS_DEF))
      {
         if (options::nl_after_func_class_proto() > pc->nl_count)
         {
            log_rule_B("nl_after_func_class_proto");
            pc->nl_count = options::nl_after_func_class_proto();
            MARK_CHANGE();
         }

         if (  (options::nl_after_func_class_proto_group() > pc->nl_count)
            && next != nullptr
            && next->type != CT_FUNC_CLASS_PROTO
            && get_chunk_parent_type(next) != CT_FUNC_CLASS_PROTO
            && !is_func_proto_group(next, CT_FUNC_CLASS_DEF))
         {
            log_rule_B("nl_after_func_class_proto_group");
            blank_line_set(pc, options::nl_after_func_class_proto_group);
         }
      }

      // Add blanks after struct/enum/union/class
      if (  (chunk_is_token(prev, CT_SEMICOLON) || chunk_is_token(prev, CT_BRACE_CLOSE))
         && (  get_chunk_parent_type(prev) == CT_STRUCT
            || get_chunk_parent_type(prev) == CT_ENUM
            || get_chunk_parent_type(prev) == CT_UNION
            || get_chunk_parent_type(prev) == CT_CLASS))
      {
         auto &opt = (get_chunk_parent_type(prev) == CT_CLASS
         ? options::nl_after_class
         : options::nl_after_struct);
         log_rule_B("nl_after_class");
         log_rule_B("nl_after_struct");

         if (opt() > pc->nl_count)
         {
            // Issue #1702
            // look back if we have a variable
            auto tmp         = pc;
            bool is_var_def  = false;
            bool is_fwd_decl = false;

            while ((tmp = chunk_get_prev(tmp)) != nullptr)
            {
               if (tmp->level > pc->level)
               {
                  continue;
               }
               LOG_FMT(LBLANK, "%s(%d): %zu:%zu token is '%s'\n",
                       __func__, __LINE__, tmp->orig_line, tmp->orig_col, tmp->text());

               if (tmp->flags.test(PCF_VAR_DEF))
               {
                  is_var_def = true;
                  break;
               }

               if (chunk_is_token(tmp, get_chunk_parent_type(prev)))
               {
                  is_fwd_decl = tmp->flags.test(PCF_INCOMPLETE);
                  break;
               }
            }
            LOG_FMT(LBLANK, "%s(%d): var_def = %s, fwd_decl = %s\n",
                    __func__, __LINE__,
                    is_var_def ? "yes" : "no",
                    is_fwd_decl ? "yes" : "no");

            if (!is_var_def && !is_fwd_decl)
            {
               blank_line_set(pc, opt);
            }
         }
      }

      // Change blanks between a function comment and body
      if (  (options::nl_comment_func_def() != 0)
         && chunk_is_token(pcmt, CT_COMMENT_MULTI)
         && get_chunk_parent_type(pcmt) == CT_COMMENT_WHOLE
         && next != nullptr
         && (  get_chunk_parent_type(next) == CT_FUNC_DEF
            || get_chunk_parent_type(next) == CT_FUNC_CLASS_DEF))
      {
         log_rule_B("nl_comment_func_def");

         if (options::nl_comment_func_def() != pc->nl_count)
         {
            log_rule_B("nl_comment_func_def");
            blank_line_set(pc, options::nl_comment_func_def);
         }
      }

      // Change blanks after a try-catch-finally block
      if (  (options::nl_after_try_catch_finally() != 0)
         && (options::nl_after_try_catch_finally() != pc->nl_count)
         && prev != nullptr
         && next != nullptr)
      {
         log_rule_B("nl_after_try_catch_finally");

         if (  chunk_is_token(prev, CT_BRACE_CLOSE)
            && (  get_chunk_parent_type(prev) == CT_CATCH
               || get_chunk_parent_type(prev) == CT_FINALLY))
         {
            if (  next->type != CT_BRACE_CLOSE
               && next->type != CT_CATCH
               && next->type != CT_FINALLY)
            {
               blank_line_set(pc, options::nl_after_try_catch_finally);
               log_rule_B("nl_after_try_catch_finally");
            }
         }
      }

      // Change blanks after a try-catch-finally block
      if (  (options::nl_between_get_set() != 0)
         && (options::nl_between_get_set() != pc->nl_count)
         && prev != nullptr
         && next != nullptr)
      {
         log_rule_B("nl_between_get_set");

         if (  get_chunk_parent_type(prev) == CT_GETSET
            && next->type != CT_BRACE_CLOSE
            && (chunk_is_token(prev, CT_BRACE_CLOSE) || chunk_is_token(prev, CT_SEMICOLON)))
         {
            blank_line_set(pc, options::nl_between_get_set);
            log_rule_B("nl_between_get_set");
         }
      }

      // Change blanks after a try-catch-finally block
      if (  (options::nl_around_cs_property() != 0)
         && (options::nl_around_cs_property() != pc->nl_count)
         && prev != nullptr
         && next != nullptr)
      {
         log_rule_B("nl_around_cs_property");

         if (  chunk_is_token(prev, CT_BRACE_CLOSE)
            && get_chunk_parent_type(prev) == CT_CS_PROPERTY
            && next->type != CT_BRACE_CLOSE)
         {
            blank_line_set(pc, options::nl_around_cs_property);
            log_rule_B("nl_around_cs_property");
         }
         else if (  get_chunk_parent_type(next) == CT_CS_PROPERTY
                 && next->flags.test(PCF_STMT_START))
         {
            blank_line_set(pc, options::nl_around_cs_property);
            log_rule_B("nl_around_cs_property");
         }
      }

      // Change blanks inside namespace braces
      if (  (options::nl_inside_namespace() != 0)
         && (options::nl_inside_namespace() != pc->nl_count)
         && (  (  chunk_is_token(prev, CT_BRACE_OPEN)
               && get_chunk_parent_type(prev) == CT_NAMESPACE)
            || (  chunk_is_token(next, CT_BRACE_CLOSE)
               && get_chunk_parent_type(next) == CT_NAMESPACE)))
      {
         log_rule_B("nl_inside_namespace");
         blank_line_set(pc, options::nl_inside_namespace);
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

      if (chunk_is_token(pc, CT_NEWLINE) && chunk_is_token(next, CT_NEWLINE))
      {
         next->nl_count = max(pc->nl_count, next->nl_count);
         chunk_del(pc);
         MARK_CHANGE();
      }
      pc = next;
   }
}


static void newlines_enum_entries(chunk_t *open_brace, iarf_e av)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc = open_brace;

   while (  (pc = chunk_get_next_nc(pc)) != nullptr
         && pc->level > open_brace->level)
   {
      if (  (pc->level != (open_brace->level + 1))
         || pc->type != CT_COMMA
         || (  pc->type == CT_COMMA && pc->next
            && (  pc->next->type == CT_COMMENT_CPP
               || pc->next->type == CT_COMMENT)))
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
   chunk_t *prev;
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
      LOG_FMT(LANNOT, "%s(%d): orig_line is %zu, orig_col is %zu, annotation is '%s',  end @ orig_line %zu, orig_col %zu, is '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(),
              ae->orig_line, ae->orig_col, ae->text());

      prev = chunk_get_prev(ae);             // Issue #1845
      LOG_FMT(LANNOT, "%s(%d): prev->orig_line is %zu, orig_col is %zu, text() is '%s'\n",
              __func__, __LINE__, prev->orig_line, prev->orig_col, prev->text());
      next = chunk_get_next_nnl(ae);

      if (chunk_is_token(next, CT_ANNOTATION))
      {
         LOG_FMT(LANNOT, "%s(%d):  -- nl_between_annotation\n",
                 __func__, __LINE__);
         newline_iarf(ae, options::nl_between_annotation());
         log_rule_B("nl_between_annotation");
      }

      if (chunk_is_token(next, CT_NEWLINE))
      {
         if (chunk_is_token(next, CT_ANNOTATION))
         {
            LOG_FMT(LANNOT, "%s(%d):  -- nl_after_annotation\n",
                    __func__, __LINE__);
            newline_iarf(ae, options::nl_after_annotation());
            log_rule_B("nl_after_annotation");
         }
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
