/**
 * @file newlines.cpp
 * Adds or removes newlines.
 *
 * Information
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
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines.h"

#include "align_stack.h"
#include "combine_skip.h"
#include "flag_parens.h"
#include "indent.h"
#include "keywords.h"
#include "prototypes.h"
#include "space.h"
#include "unc_tools.h"

#ifdef WIN32
#include <algorithm>                   // to get max
#endif // ifdef WIN32


constexpr static auto LCURRENT = LNEWLINE;

using namespace std;
using namespace uncrustify;


static void mark_change(const char *func, size_t line);


/**
 * Check to see if we are allowed to increase the newline count.
 * We can't increase the newline count:
 *  - if nl_squeeze_ifdef and a preproc is after the newline.
 *  - if eat_blanks_before_close_brace and the next is '}'
 *    - unless function contains an empty body and
 *      nl_inside_empty_func is non-zero
 *  - if eat_blanks_after_open_brace and the prev is '{'
 *    - unless the brace belongs to a namespace
 *      and nl_inside_namespace is non-zero
 */
static bool can_increase_nl(Chunk *nl);


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
static void setup_newline_add(Chunk *prev, Chunk *nl, Chunk *next);


//! Make sure there is a blank line after a commented group of values
static void newlines_double_space_struct_enum_union(Chunk *open_brace);


//! If requested, make sure each entry in an enum is on its own line
static void newlines_enum_entries(Chunk *open_brace, iarf_e av);


/**
 * Checks to see if it is OK to add a newline around the chunk.
 * Don't want to break one-liners...
 * return value:
 *  true: a new line may be added
 * false: a new line may NOT be added
 */
static bool one_liner_nl_ok(Chunk *pc);


static void nl_create_one_liner(Chunk *vbrace_open);


static void nl_create_list_liner(Chunk *brace_open);


/**
 * Test if a chunk belongs to a one-liner method definition inside a class body
 */
static bool is_class_one_liner(Chunk *pc);


/**
 * Test if a chunk may be combined with a function prototype group.
 *
 * If nl_class_leave_one_liner_groups is enabled, a chunk may be combined with
 * a function prototype group if it is a one-liner inside a class body, and is
 * a definition of the same sort as surrounding prototypes. This checks against
 * either the function name, or the function closing brace.
 */
bool is_func_proto_group(Chunk *pc, E_Token one_liner_type);

/**
 * Test if an opening brace is part of a function call or definition.
 */
static bool is_func_call_or_def(Chunk *pc);


//! Find the next newline or nl_cont
static void nl_handle_define(Chunk *pc);


/**
 * Does the Ignore, Add, Remove, or Force thing between two chunks
 *
 * @param before  The first chunk
 * @param after   The second chunk
 * @param av      The IARF value
 */
static void newline_iarf_pair(Chunk *before, Chunk *after, iarf_e av, bool check_nl_assign_leave_one_liners = false);


/**
 * Adds newlines to multi-line function call/decl/def
 * Start points to the open paren
 */
static void newline_func_multi_line(Chunk *start);


static void newline_template(Chunk *start);


/**
 * Formats a function declaration
 * Start points to the open paren
 */
static void newline_func_def_or_call(Chunk *start);


/**
 * Formats a message, adding newlines before the item before the colons.
 *
 * Start points to the open '[' in:
 * [myObject doFooWith:arg1 name:arg2  // some lines with >1 arg
 *            error:arg3];
 */
static void newline_oc_msg(Chunk *start);


//! Ensure that the next non-comment token after close brace is a newline
static void newline_end_newline(Chunk *br_close);


/**
 * Add or remove a newline between the closing paren and opening brace.
 * Also uncuddles anything on the closing brace. (may get fixed later)
 *
 * "if (...) { \n" or "if (...) \n { \n"
 *
 * For virtual braces, we can only add a newline after the vbrace open.
 * If we do so, also add a newline after the vbrace close.
 */
static bool newlines_if_for_while_switch(Chunk *start, iarf_e nl_opt);


/**
 * Add or remove extra newline before the chunk.
 * Adds before comments
 * Doesn't do anything if open brace before it
 * "code\n\ncomment\nif (...)" or "code\ncomment\nif (...)"
 */
static void newlines_if_for_while_switch_pre_blank_lines(Chunk *start, iarf_e nl_opt);


static void blank_line_set(Chunk *pc, Option<unsigned> &opt);


/**
 * Add one/two newline(s) before the chunk.
 * Adds before comments
 * Adds before destructor
 * Doesn't do anything if open brace before it
 * "code\n\ncomment\nif (...)" or "code\ncomment\nif (...)"
 */
static void newlines_func_pre_blank_lines(Chunk *start, E_Token start_type);


static Chunk *get_closing_brace(Chunk *start);


/**
 * remove any consecutive newlines following this chunk
 * skip vbraces
 */
static void remove_next_newlines(Chunk *start);


/**
 * Add or remove extra newline after end of the block started in chunk.
 * Doesn't do anything if close brace after it
 * Interesting issue is that at this point, nls can be before or after vbraces
 * VBraces will stay VBraces, conversion to real ones should have already happened
 * "if (...)\ncode\ncode" or "if (...)\ncode\n\ncode"
 */
static void newlines_if_for_while_switch_post_blank_lines(Chunk *start, iarf_e nl_opt);


/**
 * Adds or removes a newline between the keyword and the open brace.
 * If there is something after the '{' on the same line, then
 * the newline is removed unconditionally.
 * If there is a '=' between the keyword and '{', do nothing.
 *
 * "struct [name] {" or "struct [name] \n {"
 */
static void newlines_struct_union(Chunk *start, iarf_e nl_opt, bool leave_trailing);
static void newlines_enum(Chunk *start);
static void newlines_namespace(Chunk *start); // Issue #2186


/**
 * Cuddles or un-cuddles a chunk with a previous close brace
 *
 * "} while" vs "} \n while"
 * "} else" vs "} \n else"
 *
 * @param start  The chunk - should be CT_ELSE or CT_WHILE_OF_DO
 */
static void newlines_cuddle_uncuddle(Chunk *start, iarf_e nl_opt);


/**
 * Adds/removes a newline between else and '{'.
 * "else {" or "else \n {"
 */
static void newlines_do_else(Chunk *start, iarf_e nl_opt);


//! Check if token starts a variable declaration
static bool is_var_def(Chunk *pc, Chunk *next);


//! Put newline(s) before and/or after a block of variable definitions
static Chunk *newline_var_def_blk(Chunk *start);


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
static void newlines_brace_pair(Chunk *br_open);


/**
 * Put a empty line between the 'case' statement and the previous case colon
 * or semicolon.
 * Does not work with PAWN (?)
 */
static void newline_case(Chunk *start);


static void newline_case_colon(Chunk *start);


//! Put a blank line before a return statement, unless it is after an open brace
static void newline_before_return(Chunk *start);


/**
 * Put a empty line after a return statement, unless it is followed by a
 * close brace.
 *
 * May not work with PAWN
 */
static void newline_after_return(Chunk *start);


static void blank_line_max(Chunk *pc, Option<unsigned> &opt);


static iarf_e newline_template_option(Chunk *pc, iarf_e special, iarf_e base, iarf_e fallback);


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
} // mark_change


static bool can_increase_nl(Chunk *nl)
{
   LOG_FUNC_ENTRY();

   Chunk *prev = nl->GetPrevNc();

   Chunk *pcmt = nl->GetPrev();
   Chunk *next = nl->GetNext();

   if (options::nl_squeeze_ifdef())
   {
      log_rule_B("nl_squeeze_ifdef");

      Chunk *pp_start = prev->GetPpStart();

      if (  pp_start->IsNotNullChunk()
         && (  pp_start->GetParentType() == CT_PP_IF
            || pp_start->GetParentType() == CT_PP_ELSE)
         && (  pp_start->GetLevel() > 0
            || options::nl_squeeze_ifdef_top_level()))
      {
         log_rule_B("nl_squeeze_ifdef_top_level");
         bool rv = ifdef_over_whole_file() && pp_start->TestFlags(PCF_WF_IF);
         LOG_FMT(LBLANKD, "%s(%d): nl_squeeze_ifdef %zu (prev) pp_lvl=%zu rv=%d\n",
                 __func__, __LINE__, nl->GetOrigLine(), nl->GetPpLevel(), rv);
         return(rv);
      }

      if (  next->Is(CT_PREPROC)
         && (  next->GetParentType() == CT_PP_ELSE
            || next->GetParentType() == CT_PP_ENDIF)
         && (  next->GetLevel() > 0
            || options::nl_squeeze_ifdef_top_level()))
      {
         log_rule_B("nl_squeeze_ifdef_top_level");
         bool rv = ifdef_over_whole_file() && next->TestFlags(PCF_WF_ENDIF);
         LOG_FMT(LBLANKD, "%s(%d): nl_squeeze_ifdef %zu (next) pp_lvl=%zu rv=%d\n",
                 __func__, __LINE__, nl->GetOrigLine(), nl->GetPpLevel(), rv);
         return(rv);
      }
   }

   if (next->Is(CT_BRACE_CLOSE))
   {
      if (  options::nl_inside_namespace() > 0
         && next->GetParentType() == CT_NAMESPACE)
      {
         log_rule_B("nl_inside_namespace");
         LOG_FMT(LBLANKD, "%s(%d): nl_inside_namespace %zu\n",
                 __func__, __LINE__, nl->GetOrigLine());
         return(true);
      }

      if (  options::nl_inside_empty_func() > 0
         && prev->Is(CT_BRACE_OPEN)
         && (  next->GetParentType() == CT_FUNC_DEF
            || next->GetParentType() == CT_FUNC_CLASS_DEF))
      {
         log_rule_B("nl_inside_empty_func");
         LOG_FMT(LBLANKD, "%s(%d): nl_inside_empty_func %zu\n",
                 __func__, __LINE__, nl->GetOrigLine());
         return(true);
      }

      if (options::eat_blanks_before_close_brace())
      {
         log_rule_B("eat_blanks_before_close_brace");
         LOG_FMT(LBLANKD, "%s(%d): eat_blanks_before_close_brace %zu\n",
                 __func__, __LINE__, nl->GetOrigLine());
         return(false);
      }
   }

   if (prev->Is(CT_BRACE_CLOSE))
   {
      if (  options::nl_before_namespace()
         && prev->GetParentType() == CT_NAMESPACE)
      {
         log_rule_B("nl_before_namespace");
         LOG_FMT(LBLANKD, "%s(%d): nl_before_namespace %zu\n",
                 __func__, __LINE__, nl->GetOrigLine());
         return(true);
      }
   }

   if (prev->Is(CT_BRACE_OPEN))
   {
      if (  options::nl_inside_namespace() > 0
         && prev->GetParentType() == CT_NAMESPACE)
      {
         log_rule_B("nl_inside_namespace");
         LOG_FMT(LBLANKD, "%s(%d): nl_inside_namespace %zu\n",
                 __func__, __LINE__, nl->GetOrigLine());
         return(true);
      }

      if (  options::nl_inside_empty_func() > 0
         && next->Is(CT_BRACE_CLOSE)
         && (  prev->GetParentType() == CT_FUNC_DEF
            || prev->GetParentType() == CT_FUNC_CLASS_DEF))
      {
         log_rule_B("nl_inside_empty_func");
         LOG_FMT(LBLANKD, "%s(%d): nl_inside_empty_func %zu\n",
                 __func__, __LINE__, nl->GetOrigLine());
         return(true);
      }

      if (options::eat_blanks_after_open_brace())
      {
         log_rule_B("eat_blanks_after_open_brace");
         LOG_FMT(LBLANKD, "%s(%d): eat_blanks_after_open_brace %zu\n",
                 __func__, __LINE__, nl->GetOrigLine());
         return(false);
      }
   }
   log_rule_B("nl_start_of_file");

   if (  !pcmt
      && (options::nl_start_of_file() != IARF_IGNORE))
   {
      LOG_FMT(LBLANKD, "%s(%d): SOF no prev %zu\n", __func__, __LINE__, nl->GetOrigLine());
      return(false);
   }
   log_rule_B("nl_end_of_file");

   if (  next->IsNullChunk()
      && (options::nl_end_of_file() != IARF_IGNORE))
   {
      LOG_FMT(LBLANKD, "%s(%d): EOF no next %zu\n", __func__, __LINE__, nl->GetOrigLine());
      return(false);
   }
   return(true);
} // can_increase_nl


static void setup_newline_add(Chunk *prev, Chunk *nl, Chunk *next)
{
   LOG_FUNC_ENTRY();

   if (  prev->IsNullChunk()
      || nl->IsNullChunk()
      || next->IsNullChunk())
   {
      return;
   }
   undo_one_liner(prev);

   nl->SetOrigLine(prev->GetOrigLine());
   nl->SetLevel(prev->GetLevel());
   nl->SetPpLevel(prev->GetPpLevel());
   nl->SetBraceLevel(prev->GetBraceLevel());
   nl->SetPpLevel(prev->GetPpLevel());
   nl->SetNlCount(1);
   nl->SetFlags((prev->GetFlags() & PCF_COPY_FLAGS) & ~PCF_IN_PREPROC);
   nl->SetOrigCol(prev->GetOrigColEnd());
   nl->SetColumn(prev->GetOrigCol());

   if (  prev->TestFlags(PCF_IN_PREPROC)
      && next->TestFlags(PCF_IN_PREPROC))
   {
      nl->SetFlagBits(PCF_IN_PREPROC);
   }

   if (nl->TestFlags(PCF_IN_PREPROC))
   {
      nl->SetType(CT_NL_CONT);
      nl->Str() = "\\\n";
   }
   else
   {
      nl->SetType(CT_NEWLINE);
      nl->Str() = "\n";
   }
} // setup_newline_add


void double_newline(Chunk *nl)
{
   LOG_FUNC_ENTRY();

   Chunk *prev = nl->GetPrev();

   if (prev->IsNullChunk())
   {
      return;
   }
   LOG_FMT(LNEWLINE, "%s(%d): add newline after ", __func__, __LINE__);

   if (prev->Is(CT_VBRACE_CLOSE))
   {
      LOG_FMT(LNEWLINE, "VBRACE_CLOSE ");
   }
   else
   {
      LOG_FMT(LNEWLINE, "'%s' ", prev->Text());
   }
   LOG_FMT(LNEWLINE, "on line %zu", prev->GetOrigLine());

   if (!can_increase_nl(nl))
   {
      LOG_FMT(LNEWLINE, " - denied\n");
      return;
   }
   LOG_FMT(LNEWLINE, " - done\n");

   if (nl->GetNlCount() != 2)
   {
      nl->SetNlCount(2);
      MARK_CHANGE();
   }
} // double_newline


Chunk *newline_add_before(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   Chunk nl;
   Chunk *prev = pc->GetPrevNvb();

   if (prev->IsNewline())
   {
      // Already has a newline before this chunk
      return(prev);
   }
   LOG_FMT(LNEWLINE, "%s(%d): Text() '%s', on orig line is %zu, orig col is %zu, pc column is %zu",
           __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol(), pc->GetColumn());
   log_func_stack_inline(LNEWLINE);

   setup_newline_add(prev, &nl, pc);
   nl.SetOrigCol(pc->GetOrigCol());
   nl.SetPpLevel(pc->GetPpLevel());
   LOG_FMT(LNEWLINE, "%s(%d): nl column is %zu\n",
           __func__, __LINE__, nl.GetColumn());

   MARK_CHANGE();
   return(nl.CopyAndAddBefore(pc));
} // newline_add_before


Chunk *newline_force_before(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   Chunk *nl = newline_add_before(pc);

   if (  nl->IsNotNullChunk()
      && nl->GetNlCount() > 1)
   {
      nl->SetNlCount(1);
      MARK_CHANGE();
   }
   return(nl);
} // newline_force_before


Chunk *newline_add_after(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   if (pc->IsNullChunk())
   {
      return(Chunk::NullChunkPtr);
   }
   Chunk *next = pc->GetNextNvb();

   if (next->IsNewline())
   {
      // Already has a newline after this chunk
      return(next);
   }
   LOG_FMT(LNEWLINE, "%s(%d): '%s' on line %zu",
           __func__, __LINE__, pc->Text(), pc->GetOrigLine());
   log_func_stack_inline(LNEWLINE);

   Chunk nl;

   nl.SetOrigLine(pc->GetOrigLine());
   nl.SetOrigCol(pc->GetOrigCol());
   setup_newline_add(pc, &nl, next);

   MARK_CHANGE();
   // TO DO: check why the next statement is necessary
   nl.SetOrigCol(pc->GetOrigCol());
   nl.SetPpLevel(pc->GetPpLevel());
   return(nl.CopyAndAddAfter(pc));
} // newline_add_after


Chunk *newline_force_after(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   Chunk *nl = newline_add_after(pc);   // add a newline

   if (  nl->IsNotNullChunk()
      && nl->GetNlCount() > 1) // check if there are more than 1 newline
   {
      nl->SetNlCount(1);                   // if so change the newline count back to 1
      MARK_CHANGE();
   }
   return(nl);
} // newline_force_after


static void newline_end_newline(Chunk *br_close)
{
   LOG_FUNC_ENTRY();

   Chunk *next = br_close->GetNext();
   Chunk nl;

   if (!next->IsCommentOrNewline())
   {
      nl.SetOrigLine(br_close->GetOrigLine());
      nl.SetOrigCol(br_close->GetOrigCol());
      nl.SetNlCount(1);
      nl.SetPpLevel(0);
      nl.SetFlags((br_close->GetFlags() & PCF_COPY_FLAGS) & ~PCF_IN_PREPROC);

      if (  br_close->TestFlags(PCF_IN_PREPROC)
         && next->IsNotNullChunk()
         && next->TestFlags(PCF_IN_PREPROC))
      {
         nl.SetFlagBits(PCF_IN_PREPROC);
      }

      if (nl.TestFlags(PCF_IN_PREPROC))
      {
         nl.SetType(CT_NL_CONT);
         nl.Str() = "\\\n";
      }
      else
      {
         nl.SetType(CT_NEWLINE);
         nl.Str() = "\n";
      }
      MARK_CHANGE();
      LOG_FMT(LNEWLINE, "%s(%d): %zu:%zu add newline after '%s'\n",
              __func__, __LINE__, br_close->GetOrigLine(), br_close->GetOrigCol(), br_close->Text());
      nl.CopyAndAddAfter(br_close);
   }
} // newline_end_newline


static void newline_min_after(Chunk *ref, size_t count, E_PcfFlag flag)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNEWLINE, "%s(%d): for '%s', at orig line %zu, count is %zu,\n   flag is %s:",
           __func__, __LINE__, ref->Text(), ref->GetOrigLine(), count,
           pcf_flags_str(flag).c_str());
   log_func_stack_inline(LNEWLINE);

   Chunk *pc = ref;

   do
   {
      pc = pc->GetNext();
   } while (  pc->IsNotNullChunk()
           && !pc->IsNewline());

   if (pc->IsNotNullChunk())                 // Coverity CID 76002
   {
      LOG_FMT(LNEWLINE, "%s(%d): type is %s, orig line %zu, orig col %zu\n",
              __func__, __LINE__, get_token_name(pc->GetType()), pc->GetOrigLine(), pc->GetOrigCol());
   }
   Chunk *next = pc->GetNext();

   if (next->IsNullChunk())
   {
      return;
   }

   if (  next->IsComment()
      && next->GetNlCount() == 1
      && pc->GetPrev()->IsComment())
   {
      newline_min_after(next, count, flag);
      return;
   }
   pc->SetFlagBits(flag);

   if (  pc->IsNewline()
      && can_increase_nl(pc))
   {
      if (pc->GetNlCount() < count)
      {
         pc->SetNlCount(count);
         MARK_CHANGE();
      }
   }
} // newline_min_after


Chunk *newline_add_between(Chunk *start, Chunk *end)
{
   LOG_FUNC_ENTRY();

   if (  start->IsNullChunk()
      || end->IsNullChunk()
      || end->Is(CT_IGNORED))
   {
      return(Chunk::NullChunkPtr);
   }
   LOG_FMT(LNEWLINE, "%s(%d): start->Text() is '%s', type is %s, orig line is %zu, orig col is %zu\n",
           __func__, __LINE__, start->Text(), get_token_name(start->GetType()),
           start->GetOrigLine(), start->GetOrigCol());
   LOG_FMT(LNEWLINE, "%s(%d): and end->Text() is '%s', orig line is %zu, orig col is %zu\n  ",
           __func__, __LINE__, end->Text(), end->GetOrigLine(), end->GetOrigCol());
   log_func_stack_inline(LNEWLINE);

   // Back-up check for one-liners (should never be true!)
   if (!one_liner_nl_ok(start))
   {
      return(Chunk::NullChunkPtr);
   }

   /*
    * Scan for a line break, if there is a line break between start and end
    * we won't add another one
    */
   for (Chunk *pc = start; pc != end; pc = pc->GetNext())
   {
      if (pc->IsNewline())
      {
         return(pc);
      }
   }

   /*
    * If the second one is a brace open, then check to see
    * if a comment + newline follows
    */
   if (end->Is(CT_BRACE_OPEN))
   {
      Chunk *pc = end->GetNext();

      if (pc->IsComment())
      {
         pc = pc->GetNext();

         if (pc->IsNewline())
         {
            // are there some more (comment + newline)s ?
            Chunk *pc1 = end->GetNextNcNnl();

            if (!pc1->IsNewline())
            {
               // yes, go back
               Chunk *pc2 = pc1->GetPrev();
               pc = pc2;
            }

            if (end == pc)
            {
               LOG_FMT(LNEWLINE, "%s(%d): pc1 and pc are identical\n",
                       __func__, __LINE__);
            }
            else
            {
               // Move the open brace to after the newline
               end->MoveAfter(pc);
            }
            LOG_FMT(LNEWLINE, "%s(%d):\n", __func__, __LINE__);
            newline_add_after(end);
            return(pc);
         }
         else                  // Issue #3873
         {
            LOG_FMT(LNEWLINE, "%s(%d):\n", __func__, __LINE__);
         }
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
   Chunk *tmp = newline_add_before(end);

   return(tmp);
} // newline_add_between


void newline_del_between(Chunk *start, Chunk *end)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNEWLINE, "%s(%d): start->Text() is '%s', orig line is %zu, orig col is %zu\n",
           __func__, __LINE__, start->Text(), start->GetOrigLine(), start->GetOrigCol());
   LOG_FMT(LNEWLINE, "%s(%d): and end->Text() is '%s', orig line is %zu, orig col is %zu: preproc=%c/%c\n",
           __func__, __LINE__, end->Text(), end->GetOrigLine(), end->GetOrigCol(),
           start->TestFlags(PCF_IN_PREPROC) ? 'y' : 'n',
           end->TestFlags(PCF_IN_PREPROC) ? 'y' : 'n');
   log_func_stack_inline(LNEWLINE);

   // Can't remove anything if the preproc status differs
   if (!start->IsSamePreproc(end))
   {
      return;
   }
   Chunk *pc           = start;
   bool  start_removed = false;

   do
   {
      Chunk *next = pc->GetNext();

      if (pc->IsNewline())
      {
         Chunk *prev = pc->GetPrev();

         if (  (  !prev->IsComment()
               && !next->IsComment())
            || prev->IsNewline()
            || next->IsNewline())
         {
            if (pc->SafeToDeleteNl())
            {
               if (pc == start)
               {
                  start_removed = true;
               }
               Chunk::Delete(pc);
               MARK_CHANGE();

               if (prev->IsNotNullChunk())
               {
                  size_t temp = space_col_align(prev, next);
                  align_to_column(next, prev->GetColumn() + temp);
                  dump_step(dump_file_name, "del 1");
               }
            }
         }
         else
         {
            if (pc->GetNlCount() > 1)
            {
               pc->SetNlCount(1);
               MARK_CHANGE();
            }
         }
      }
      pc = next;
   } while (pc != end);

   if (  !start_removed
      && end->IsString("{")
      && (  start->IsString(")")
         || start->Is(CT_DO)
         || start->Is(CT_ELSE)))
   {
      end->MoveAfter(start);
   }
} // newline_del_between


void newlines_sparens()
{
   LOG_FUNC_ENTRY();

   //Chunk *sparen_open;

   for (Chunk *sparen_open = Chunk::GetHead()->GetNextType(CT_SPAREN_OPEN, ANY_LEVEL);
        sparen_open->IsNotNullChunk();
        sparen_open = sparen_open->GetNextType(CT_SPAREN_OPEN, ANY_LEVEL))
   {
      Chunk *sparen_close = sparen_open->GetNextType(CT_SPAREN_CLOSE, sparen_open->GetLevel());

      if (sparen_close->IsNullChunk())
      {
         continue;
      }
      Chunk *sparen_content_start = sparen_open->GetNextNnl();
      Chunk *sparen_content_end   = sparen_close->GetPrevNnl();
      bool  is_multiline          = (
         sparen_content_start != sparen_content_end
                                    && !sparen_content_start->IsOnSameLine(sparen_content_end));

      // Add a newline after '(' if an if/for/while/switch condition spans multiple lines,
      // as e.g. required by the ROS 2 development style guidelines:
      // https://index.ros.org/doc/ros2/Contributing/Developer-Guide/#open-versus-cuddled-braces
      if (is_multiline)
      {
         log_rule_B("nl_multi_line_sparen_open");
         newline_iarf(sparen_open, options::nl_multi_line_sparen_open());
      }

      // Add a newline before ')' if an if/for/while/switch condition spans multiple lines. Overrides nl_before_if_closing_paren if both are specified.
      if (  is_multiline
         && options::nl_multi_line_sparen_close() != IARF_IGNORE)
      {
         log_rule_B("nl_multi_line_sparen_close");
         newline_iarf(sparen_content_end, options::nl_multi_line_sparen_close());
      }
      else
      {
         // add/remove trailing newline in an if condition
         Chunk *ctrl_structure = sparen_open->GetPrevNcNnl();

         if (  ctrl_structure->Is(CT_IF)
            || ctrl_structure->Is(CT_ELSEIF))
         {
            log_rule_B("nl_before_if_closing_paren");
            newline_iarf_pair(sparen_content_end, sparen_close, options::nl_before_if_closing_paren());
         }
      }
   }
} // newlines_sparens


static bool newlines_if_for_while_switch(Chunk *start, iarf_e nl_opt)
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
                  && !pc->GetNextNc()->IsNewline())
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


static void newlines_if_for_while_switch_pre_blank_lines(Chunk *start, iarf_e nl_opt)
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


static void blank_line_set(Chunk *pc, Option<unsigned> &opt)
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
      LOG_FMT(LBLANKD, "%s(%d): do_blank_lines: %s set line %zu to %u\n",
              __func__, __LINE__, opt.name(), pc->GetOrigLine(), optval);
      pc->SetNlCount(optval);
      MARK_CHANGE();
   }
} // blank_line_set


bool do_it_newlines_func_pre_blank_lines(Chunk *last_nl, E_Token start_type)
{
   LOG_FUNC_ENTRY();

   if (last_nl->IsNullChunk())
   {
      return(false);
   }
   LOG_FMT(LNLFUNCT, "%s(%d): orig line is %zu, orig col is %zu, type is %s, Text() is '%s'\n",
           __func__, __LINE__,
           last_nl->GetOrigLine(), last_nl->GetOrigCol(), get_token_name(last_nl->GetType()), last_nl->Text());

   switch (start_type)
   {
   case CT_FUNC_CLASS_DEF:
   {
      log_rule_B("nl_before_func_class_def");
      bool diff = options::nl_before_func_class_def() <= last_nl->GetNlCount();
      LOG_FMT(LNLFUNCT, "%s(%d): is %s\n",
              __func__, __LINE__, diff ? "TRUE" : "FALSE");

      log_rule_B("nl_before_func_class_def");

      if (options::nl_before_func_class_def() != last_nl->GetNlCount())
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
      bool diff = options::nl_before_func_class_proto() <= last_nl->GetNlCount();
      LOG_FMT(LNLFUNCT, "%s(%d): is %s\n",
              __func__, __LINE__, diff ? "TRUE" : "FALSE");

      log_rule_B("nl_before_func_class_proto");

      if (options::nl_before_func_class_proto() != last_nl->GetNlCount())
      {
         LOG_FMT(LNLFUNCT, "%s(%d):   set blank line(s) to %u\n",
                 __func__, __LINE__, options::nl_before_func_class_proto());
         blank_line_set(last_nl, options::nl_before_func_class_proto);
      }
      return(diff);
   }

   case CT_FUNC_DEF:
   {
      LOG_FMT(LNLFUNCT, "%s(%d): nl_before_func_body_def() is %u, last_nl new line count is %zu\n",
              __func__, __LINE__, options::nl_before_func_body_def(), last_nl->GetNlCount());
      log_rule_B("nl_before_func_body_def");
      bool diff = options::nl_before_func_body_def() <= last_nl->GetNlCount();
      LOG_FMT(LNLFUNCT, "%s(%d): is %s\n",
              __func__, __LINE__, diff ? "TRUE" : "FALSE");

      log_rule_B("nl_before_func_body_def");

      if (options::nl_before_func_body_def() != last_nl->GetNlCount())
      {
         LOG_FMT(LNLFUNCT, "%s(%d):    set blank line(s) to %u\n",
                 __func__, __LINE__, options::nl_before_func_body_def());
         log_rule_B("nl_before_func_body_def");
         blank_line_set(last_nl, options::nl_before_func_body_def);
      }
      LOG_FMT(LNLFUNCT, "%s(%d): nl_before_func_body_def() is %u, last_nl new line count is %zu\n",
              __func__, __LINE__, options::nl_before_func_body_def(), last_nl->GetNlCount());
      return(diff);
   }

   case CT_FUNC_PROTO:
   {
      log_rule_B("nl_before_func_body_proto");
      bool diff = options::nl_before_func_body_proto() <= last_nl->GetNlCount();
      LOG_FMT(LNLFUNCT, "%s(%d): is %s\n",
              __func__, __LINE__, diff ? "TRUE" : "FALSE");

      log_rule_B("nl_before_func_body_proto");

      if (options::nl_before_func_body_proto() != last_nl->GetNlCount())
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
              __func__, __LINE__, last_nl->GetOrigLine());
      return(false);
   }
   } // switch
} // do_it_newlines_func_pre_blank_lines


static void newlines_func_pre_blank_lines(Chunk *start, E_Token start_type)
{
   LOG_FUNC_ENTRY();

   log_rule_B("nl_before_func_class_def");
   log_rule_B("nl_before_func_class_proto");
   log_rule_B("nl_before_func_body_def");
   log_rule_B("nl_before_func_body_proto");

   if (  start->IsNullChunk()
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
           __func__, __LINE__, start->GetOrigLine(), start->GetOrigCol(), get_token_name(start_type));
   LOG_FMT(LNLFUNCT, "%s(%d): BEGIN set blank line(s) for '%s' at line %zu\n",
           __func__, __LINE__, start->Text(), start->GetOrigLine());
   /*
    * look backwards until we find:
    *   - open brace (don't add or remove)
    *   - two newlines in a row (don't add)
    *   - a destructor
    *   - something else (don't remove)
    */
   Chunk  *pc           = Chunk::NullChunkPtr;
   Chunk  *last_nl      = Chunk::NullChunkPtr;
   Chunk  *last_comment = Chunk::NullChunkPtr;
   size_t first_line    = start->GetOrigLine();

   for (pc = start->GetPrev(); pc->IsNotNullChunk(); pc = pc->GetPrev())
   {
      LOG_FMT(LNLFUNCT, "%s(%d): orig line is %zu, orig col is %zu, type is %s, Text() is '%s', new line count is %zu\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()), pc->Text(), pc->GetNlCount());

      if (pc->IsNewline())
      {
         last_nl = pc;
         LOG_FMT(LNLFUNCT, "%s(%d):    <Chunk::IsNewline> found at line %zu, column %zu, new line count is %zu\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetNlCount());
         LOG_FMT(LNLFUNCT, "%s(%d):    last_nl set to %zu\n",
                 __func__, __LINE__, last_nl->GetOrigLine());
         bool break_now = false;

         if (pc->GetNlCount() > 1)
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
      else if (pc->IsComment())
      {
         LOG_FMT(LNLFUNCT, "%s(%d):    <Chunk::IsComment> found at line %zu, column %zu\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol());

         if (  (  pc->GetOrigLine() < first_line
               && ((first_line - pc->GetOrigLine()
                    - (pc->Is(CT_COMMENT_MULTI) ? pc->GetNlCount() : 0))) < 2)
            || (  last_comment->IsNotNullChunk()
               && pc->Is(CT_COMMENT_CPP)          // combine only cpp comments
               && last_comment->Is(pc->GetType()) // don't mix comment types
               && last_comment->GetOrigLine() > pc->GetOrigLine()
               && (last_comment->GetOrigLine() - pc->GetOrigLine()) < 2))
         {
            last_comment = pc;
            continue;
         }
         bool break_now = do_it_newlines_func_pre_blank_lines(last_nl, start_type);
         LOG_FMT(LNLFUNCT, "%s(%d): break_now is %s\n",
                 __func__, __LINE__, break_now ? "TRUE" : "FALSE");
         continue;
      }
      else if (  pc->Is(CT_DESTRUCTOR)
              || pc->Is(CT_TYPE)
              || pc->Is(CT_TEMPLATE)
              || pc->Is(CT_QUALIFIER)
              || pc->Is(CT_PTR_TYPE)
              || pc->Is(CT_BYREF)                  // Issue #2163
              || pc->Is(CT_DC_MEMBER)
              || pc->Is(CT_EXTERN)
              || (  pc->Is(CT_STRING)
                 && pc->GetParentType() == CT_EXTERN))
      {
         LOG_FMT(LNLFUNCT, "%s(%d): first_line set to %zu\n",
                 __func__, __LINE__, pc->GetOrigLine());
         first_line = pc->GetOrigLine();
         continue;
      }
      else if (  pc->Is(CT_ANGLE_CLOSE)
              && pc->GetParentType() == CT_TEMPLATE)
      {
         LOG_FMT(LNLFUNCT, "%s(%d):\n", __func__, __LINE__);
         // skip template stuff to add newlines before it
         pc = pc->GetOpeningParen();

         if (pc->IsNotNullChunk())
         {
            first_line = pc->GetOrigLine();
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


static Chunk *get_closing_brace(Chunk *start)
{
   LOG_FUNC_ENTRY();

   Chunk  *pc;
   size_t level = start->GetLevel();

   for (pc = start; (pc = pc->GetNext())->IsNotNullChunk();)
   {
      if (  (pc->IsBraceClose())
         && pc->GetLevel() == level)
      {
         return(pc);
      }

      // for some reason, we can have newlines between if and opening brace that are lower level than either
      if (  !pc->IsNewline()
         && pc->GetLevel() < level)
      {
         return(Chunk::NullChunkPtr);
      }
   }

   return(Chunk::NullChunkPtr);
} // get_closing_brace


static void remove_next_newlines(Chunk *start)
{
   LOG_FUNC_ENTRY();

   Chunk *next;

   while ((next = start->GetNext())->IsNotNullChunk())
   {
      if (  next->IsNewline()
         && next->SafeToDeleteNl())
      {
         Chunk::Delete(next);
         MARK_CHANGE();
      }
      else if (next->IsVBrace())
      {
         start = next;
      }
      else
      {
         break;
      }
   }
} // remove_next_newlines


static void newlines_if_for_while_switch_post_blank_lines(Chunk *start, iarf_e nl_opt)
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


static void newlines_struct_union(Chunk *start, iarf_e nl_opt, bool leave_trailing)
{
   LOG_FUNC_ENTRY();

   log_rule_B("nl_define_macro");

   if (  nl_opt == IARF_IGNORE
      || (  start->TestFlags(PCF_IN_PREPROC)
         && !options::nl_define_macro()))
   {
      return;
   }
   /*
    * step past any junk between the keyword and the open brace
    * Quit if we hit a semicolon or '=', which are not expected.
    */
   size_t level = start->GetLevel();
   Chunk  *pc   = start->GetNextNcNnl();

   while (  pc->IsNotNullChunk()
         && pc->GetLevel() >= level)
   {
      if (  pc->GetLevel() == level
         && (  pc->Is(CT_BRACE_OPEN)
            || pc->IsSemicolon()
            || pc->Is(CT_ASSIGN)))
      {
         break;
      }
      start = pc;
      pc    = pc->GetNextNcNnl();
   }

   // If we hit a brace open, then we need to toy with the newlines
   if (pc->Is(CT_BRACE_OPEN))
   {
      // Skip over embedded C comments
      Chunk *next = pc->GetNext();

      while (next->Is(CT_COMMENT))
      {
         next = next->GetNext();
      }

      if (  leave_trailing
         && !next->IsCommentOrNewline())
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
static void newlines_enum(Chunk *start)
{
   LOG_FUNC_ENTRY();

   log_rule_B("nl_define_macro");

   if (  start->TestFlags(PCF_IN_PREPROC)
      && !options::nl_define_macro())
   {
      return;
   }
   // look for 'enum class'
   Chunk *pcClass = start->GetNextNcNnl();

   if (pcClass->Is(CT_ENUM_CLASS))
   {
      log_rule_B("nl_enum_class");
      newline_iarf_pair(start, pcClass, options::nl_enum_class());
      // look for 'identifier'/ 'type'
      Chunk *pcType = pcClass->GetNextNcNnl();

      if (pcType->Is(CT_TYPE))
      {
         log_rule_B("nl_enum_class_identifier");
         newline_iarf_pair(pcClass, pcType, options::nl_enum_class_identifier());
         // look for ':'
         Chunk *pcColon = pcType->GetNextNcNnl();

         if (pcColon->Is(CT_ENUM_COLON))                       // Issue #4040
         {
            log_rule_B("nl_enum_identifier_colon");
            newline_iarf_pair(pcType, pcColon, options::nl_enum_identifier_colon());
            // look for 'type' i.e. unsigned
            Chunk *pcType1 = pcColon->GetNextNcNnl();

            if (pcType1->Is(CT_TYPE))
            {
               log_rule_B("nl_enum_colon_type");
               newline_iarf_pair(pcColon, pcType1, options::nl_enum_colon_type());
               // look for 'type' i.e. int
               Chunk *pcType2 = pcType1->GetNextNcNnl();

               if (pcType2->Is(CT_TYPE))
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
   size_t level = start->GetLevel();
   Chunk  *pc   = start->GetNextNcNnl();

   while (  pc->IsNotNullChunk()
         && pc->GetLevel() >= level)
   {
      if (  pc->GetLevel() == level
         && (  pc->Is(CT_BRACE_OPEN)
            || pc->IsSemicolon()
            || pc->Is(CT_ASSIGN)))
      {
         break;
      }
      start = pc;
      pc    = pc->GetNextNcNnl();
   }

   // If we hit a brace open, then we need to toy with the newlines
   if (pc->Is(CT_BRACE_OPEN))
   {
      // Skip over embedded C comments
      Chunk *next = pc->GetNext();

      while (next->Is(CT_COMMENT))
      {
         next = next->GetNext();
      }
      iarf_e nl_opt;

      if (!next->IsCommentOrNewline())
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
static void newlines_namespace(Chunk *start)
{
   LOG_FUNC_ENTRY();

   log_rule_B("nl_namespace_brace");

   // Add or remove newline between 'namespace' and 'BRACE_OPEN'
   log_rule_B("nl_define_macro");
   iarf_e nl_opt = options::nl_namespace_brace();

   if (  nl_opt == IARF_IGNORE
      || (  start->TestFlags(PCF_IN_PREPROC)
         && !options::nl_define_macro()))
   {
      return;
   }
   Chunk *braceOpen = start->GetNextType(CT_BRACE_OPEN, start->GetLevel());

   LOG_FMT(LNEWLINE, "%s(%d): braceOpen orig line is %zu, orig col is %zu, Text() is '%s'\n",
           __func__, __LINE__, braceOpen->GetOrigLine(), braceOpen->GetOrigCol(), braceOpen->Text());
   // produces much more log output. Use it only debugging purpose
   //log_pcf_flags(LNEWLINE, braceOpen->GetFlags());

   if (braceOpen->TestFlags(PCF_ONE_LINER))
   {
      LOG_FMT(LNEWLINE, "%s(%d): is one_liner\n",
              __func__, __LINE__);
      return;
   }
   Chunk *beforeBrace = braceOpen->GetPrev();

   LOG_FMT(LNEWLINE, "%s(%d): beforeBrace orig line is %zu, orig col is %zu, Text() is '%s'\n",
           __func__, __LINE__, beforeBrace->GetOrigLine(), beforeBrace->GetOrigCol(), beforeBrace->Text());
   // 'namespace' 'BRACE_OPEN'
   newline_iarf_pair(beforeBrace, braceOpen, nl_opt);
} // newlines_namespace


static void newlines_cuddle_uncuddle(Chunk *start, iarf_e nl_opt)
{
   LOG_FUNC_ENTRY();

   log_rule_B("nl_define_macro");

   if (  start->TestFlags(PCF_IN_PREPROC)
      && !options::nl_define_macro())
   {
      return;
   }
   Chunk *br_close = start->GetPrevNcNnlNi();   // Issue #2279

   if (br_close->Is(CT_BRACE_CLOSE))
   {
      newline_iarf_pair(br_close, start, nl_opt);
   }
} // newlines_cuddle_uncuddle


static void newlines_do_else(Chunk *start, iarf_e nl_opt)
{
   LOG_FUNC_ENTRY();

   log_rule_B("nl_define_macro");
   log_ruleNL("nl_define_macro", start);

   if (  nl_opt == IARF_IGNORE
      || (  start->TestFlags(PCF_IN_PREPROC)
         && !options::nl_define_macro()))
   {
      return;
   }
   Chunk *next = start->GetNextNcNnl();

   if (  next->IsNotNullChunk()
      && (  next->Is(CT_BRACE_OPEN)
         || next->Is(CT_VBRACE_OPEN)))
   {
      if (!one_liner_nl_ok(next))
      {
         LOG_FMT(LNL1LINE, "%s(%d): a new line may NOT be added\n", __func__, __LINE__);
         return;
      }
      LOG_FMT(LNL1LINE, "%s(%d): a new line may be added\n", __func__, __LINE__);

      if (next->Is(CT_VBRACE_OPEN))
      {
         // Can only add - we don't want to create a one-line here
         if (nl_opt & IARF_ADD)
         {
            newline_iarf_pair(start, next->GetNextNcNnl(), nl_opt);
            Chunk *tmp = next->GetNextType(CT_VBRACE_CLOSE, next->GetLevel());

            if (  !tmp->GetNextNc()->IsNewline()
               && !tmp->GetPrevNc()->IsNewline())
            {
               newline_add_after(tmp);
            }
         }
      }
      else
      {
         newline_iarf_pair(start, next, nl_opt);
         newline_add_between(next, next->GetNextNcNnl());
      }
   }
} // newlines_do_else


static bool is_var_def(Chunk *pc, Chunk *next)
{
   if (  pc->Is(CT_DECLTYPE)
      && next->Is(CT_PAREN_OPEN))
   {
      // If current token starts a decltype expression, skip it
      next = next->GetClosingParen();
      next = next->GetNextNcNnl();
   }
   else if (!pc->IsTypeDefinition())
   {
      // Otherwise, if the current token is not a type --> not a declaration
      return(false);
   }
   else if (next->Is(CT_DC_MEMBER))
   {
      // If next token is CT_DC_MEMBER, skip it
      next = next->SkipDcMember();
   }
   else if (next->Is(CT_ANGLE_OPEN))
   {
      // If we have a template type, skip it
      next = next->GetClosingParen();
      next = next->GetNextNcNnl();
   }
   bool is = (  (  next->IsTypeDefinition()
                && next->GetParentType() != CT_FUNC_DEF)           // Issue #2639
             || next->Is(CT_WORD)
             || next->Is(CT_FUNC_CTOR_VAR));

   return(is);
} // is_var_def


static bool is_func_call_or_def(Chunk *pc)
{
   if (  pc->GetParentType() == CT_FUNC_DEF
      || pc->GetParentType() == CT_FUNC_CALL
      || pc->GetParentType() == CT_FUNC_CALL_USER
      || pc->GetParentType() == CT_FUNC_CLASS_DEF
      || pc->GetParentType() == CT_OC_CLASS
      || pc->GetParentType() == CT_OC_MSG_DECL
      || pc->GetParentType() == CT_CS_PROPERTY
      || pc->GetParentType() == CT_CPP_LAMBDA)
   {
      return(true);
   }
   return(false);
} // is_func_call_or_def


// Put newline(s) before and/or after a block of variable definitions
static Chunk *newline_var_def_blk(Chunk *start)
{
   LOG_FUNC_ENTRY();

   Chunk *pc           = start;
   Chunk *prev         = start->GetPrevNcNnlNi(); // Issue #2279
   bool  did_this_line = false;
   bool  fn_top        = false;
   bool  var_blk       = false;
   bool  first_var_blk = true;

   LOG_FMT(LVARDFBLK, "%s(%d): start orig line is %zu, orig col is %zu, Text() is '%s'\n",
           __func__, __LINE__, start->GetOrigLine(), start->GetOrigCol(), start->Text());

   if (start->Is(CT_BRACE_OPEN))
   {
      // can't be any variable definitions in a "= {" block
      if (  prev->IsNotNullChunk()
         && prev->Is(CT_ASSIGN))
      {
         Chunk *tmp = start->GetClosingParen();
         return(tmp->GetNextNcNnl());
      }
      // check if we're at the top of a function definition, or function call with a
      // possible variable block
      fn_top = is_func_call_or_def(start);
      // opening brace is processed, start with next chunk
      pc = pc->GetNext();
   }

   while (  pc->IsNotNullChunk()
         && (  pc->GetLevel() >= start->GetLevel()
            || pc->GetLevel() == 0))
   {
      LOG_CHUNK(LTOK, pc);

      Chunk *next_pc = pc->GetNext();
      LOG_FMT(LVARDFBLK, "%s(%d): next_pc orig line is %zu, orig col is %zu, type is %s, Text() is '%s'\n",
              __func__, __LINE__, next_pc->GetOrigLine(), next_pc->GetOrigCol(), get_token_name(next_pc->GetType()), next_pc->Text());

      // If next_pc token is CT_DC_MEMBER, skip it
      if (next_pc->Is(CT_DC_MEMBER))
      {
         pc = pc->SkipDcMember();
      }

      // skip qualifiers
      if (pc->Is(CT_QUALIFIER))
      {
         pc = pc->GetNext();
         continue;
      }

      if (pc->IsComment())
      {
         pc = pc->GetNext();
         continue;
      }

      // process nested braces
      if (pc->Is(CT_BRACE_OPEN))
      {
         pc = newline_var_def_blk(pc);
         continue;
      }

      // Done with this brace set?
      if (pc->Is(CT_BRACE_CLOSE))
      {
         pc = pc->GetNext();
         break;
      }

      // skip vbraces
      if (pc->Is(CT_VBRACE_OPEN))
      {
         pc = pc->GetNextType(CT_VBRACE_CLOSE, pc->GetLevel());
         pc = pc->GetNext();
         continue;
      }

      // Ignore stuff inside parenthesis/squares/angles
      if (pc->GetLevel() > pc->GetBraceLevel())
      {
         pc = pc->GetNext();
         continue;
      }

      if (pc->IsNewline())
      {
         did_this_line = false;
         pc            = pc->GetNext();
         continue;
      }

      // Determine if this is a variable definition or code
      if (  !did_this_line
         && pc->IsNot(CT_FUNC_CLASS_DEF)
         && pc->IsNot(CT_FUNC_CLASS_PROTO)
         && (  (pc->GetLevel() == (start->GetLevel() + 1))
            || pc->GetLevel() == 0))
      {
         Chunk *next = pc->GetNextNcNnl();
         LOG_FMT(LVARDFBLK, "%s(%d): next orig line is %zu, orig col is %zu, Text() is '%s'\n",
                 __func__, __LINE__, next->GetOrigLine(), next->GetOrigCol(), next->Text());

         // skip over all other type-like things
         while (  next->Is(CT_PTR_TYPE)  // Issue #2692
               || next->Is(CT_BYREF)     // Issue #3018
               || next->Is(CT_QUALIFIER)
               || next->Is(CT_TSQUARE))
         {
            next = next->GetNextNcNnl();
            LOG_FMT(LVARDFBLK, "%s(%d): next orig line is %zu, orig col is %zu, Text() is '%s'\n",
                    __func__, __LINE__, next->GetOrigLine(), next->GetOrigCol(), next->Text());
         }

         if (next->IsNullChunk())
         {
            break;
         }
         LOG_FMT(LVARDFBLK, "%s(%d): next orig line is %zu, orig col is %zu, Text() is '%s'\n",
                 __func__, __LINE__, next->GetOrigLine(), next->GetOrigCol(), next->Text());

         prev = pc->GetPrevNcNnl();

         while (  prev->Is(CT_DC_MEMBER)
               || prev->Is(CT_QUALIFIER)
               || prev->Is(CT_TYPE))
         {
            prev = prev->GetPrevNcNnl();
         }

         if (!(  prev->IsBraceOpen()
              || prev->IsBraceClose()))
         {
            prev = pc->GetPrevType(CT_SEMICOLON, pc->GetLevel());
         }

         if (prev->IsNullChunk())
         {
            prev = pc->GetPrevType(CT_BRACE_OPEN, pc->GetLevel() - 1);      // Issue #2692
         }

         if (  prev->Is(CT_STRING)
            && prev->GetParentType() == CT_EXTERN
            && prev->GetPrev()->Is(CT_EXTERN))
         {
            prev = prev->GetPrev()->GetPrevNcNnlNi();   // Issue #2279
         }
         LOG_FMT(LVARDFBLK, "%s(%d): pc orig line is %zu, orig col is %zu, type is %s, Text() is '%s'\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()), pc->Text());
         LOG_FMT(LVARDFBLK, "%s(%d): next orig line is %zu, orig col is %zu, type is %s, Text() is '%s'\n",
                 __func__, __LINE__, next->GetOrigLine(), next->GetOrigCol(), get_token_name(next->GetType()), next->Text());

         if (is_var_def(pc, next))
         {
            LOG_FMT(LVARDFBLK, "%s(%d): 'typ==var' found: '%s %s' at line %zu\n",
                    __func__, __LINE__, pc->Text(), next->Text(), pc->GetOrigLine());
            LOG_FMT(LBLANKD, "%s(%d): var_blk %s, first_var_blk %s, fn_top %s\n",
                    __func__, __LINE__, var_blk ? "TRUE" : "FALSE",
                    first_var_blk ? "TRUE" : "FALSE", fn_top ? "TRUE" : "FALSE");
            // Put newline(s) before a block of variable definitions
            log_rule_B("nl_var_def_blk_start");

            if (  !var_blk
               && !first_var_blk
               && options::nl_var_def_blk_start() > 0)
            {
               LOG_FMT(LVARDFBLK, "%s(%d): pc is '%s', orig line is %zu\n",
                       __func__, __LINE__, pc->Text(), pc->GetOrigLine());

               if (prev->IsNullChunk())
               {
                  LOG_FMT(LVARDFBLK, "%s(%d): prev is a null chunk\n", __func__, __LINE__);
               }
               else
               {
                  LOG_FMT(LVARDFBLK, "%s(%d): prev is '%s', orig line is %zu\n",
                          __func__, __LINE__, prev->Text(), prev->GetOrigLine());

                  if (!prev->IsBraceOpen())
                  {
                     newline_min_after(prev, options::nl_var_def_blk_start() + 1, PCF_VAR_DEF);
                  }
               }
            }
            // set newlines within var def block
            log_rule_B("nl_var_def_blk_in");

            if (  var_blk
               && (options::nl_var_def_blk_in() > 0))
            {
               prev = pc->GetPrev();
               LOG_FMT(LVARDFBLK, "%s(%d): prev orig line is %zu, orig col is %zu, Text() is '%s'\n",
                       __func__, __LINE__, prev->GetOrigLine(), prev->GetOrigCol(), prev->Text());

               if (prev->IsNewline())
               {
                  if (prev->GetNlCount() > options::nl_var_def_blk_in())
                  {
                     prev->SetNlCount(options::nl_var_def_blk_in());
                     MARK_CHANGE();
                  }
               }
            }
            pc      = pc->GetNextType(CT_SEMICOLON, pc->GetLevel());
            var_blk = true;
         }
         else if (var_blk)
         {
            LOG_FMT(LVARDFBLK, "%s(%d): var_blk %s, first_var_blk %s, fn_top %s\n",
                    __func__, __LINE__, var_blk ? "TRUE" : "FALSE",
                    first_var_blk ? "TRUE" : "FALSE", fn_top ? "TRUE" : "FALSE");
            log_rule_B("nl_var_def_blk_end_func_top");
            log_rule_B("nl_var_def_blk_end");

            if (  first_var_blk
               && fn_top)
            {
               // set blank lines after first var def block at the top of a function
               if (options::nl_var_def_blk_end_func_top() > 0)
               {
                  LOG_FMT(LVARDFBLK, "%s(%d): nl_var_def_blk_end_func_top at line %zu\n",
                          __func__, __LINE__, prev->GetOrigLine());
                  newline_min_after(prev, options::nl_var_def_blk_end_func_top() + 1, PCF_VAR_DEF);
               }
            }
            else if (  !pc->IsPreproc()
                    && options::nl_var_def_blk_end() > 0)
            {
               // set blank lines after other var def blocks
               LOG_FMT(LVARDFBLK, "%s(%d): nl_var_def_blk_end at line %zu\n",
                       __func__, __LINE__, prev->GetOrigLine());
               // Issue #3516
               newline_min_after(prev, options::nl_var_def_blk_end() + 1, PCF_VAR_DEF);
            }
            // reset the variables for the next block
            first_var_blk = false;
            var_blk       = false;
         }
         else
         {
            first_var_blk = false;
            var_blk       = false;
         }
      }
      else
      {
         if (pc->Is(CT_FUNC_CLASS_DEF))
         {
            log_rule_B("nl_var_def_blk_end");

            if (  var_blk
               && options::nl_var_def_blk_end() > 0)
            {
               prev = pc->GetPrev();
               prev = prev->GetPrev();
               newline_min_after(prev, options::nl_var_def_blk_end() + 1, PCF_VAR_DEF);
               pc            = pc->GetNext();
               first_var_blk = false;
               var_blk       = false;
            }
         }
      }
      did_this_line = true;

      if (pc == nullptr)
      {
         pc = Chunk::NullChunkPtr;
      }
      pc = pc->GetNext();
   }
   LOG_FMT(LVARDFBLK, "%s(%d): pc orig line is %zu, orig col is %zu, Text() is '%s', level is %zu\n",
           __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), pc->GetLevel());
   LOG_FMT(LVARDFBLK, "%s(%d): start orig line is %zu, orig col is %zu, Text() is '%s', level is %zu\n",
           __func__, __LINE__, start->GetOrigLine(), start->GetOrigCol(), start->Text(), start->GetLevel());
   return(pc);
} // newline_var_def_blk


static void collapse_empty_body(Chunk *br_open)
{
   for (Chunk *pc = br_open->GetNext(); pc->IsNot(CT_BRACE_CLOSE); pc = pc->GetNext())
   {
      if (  pc->Is(CT_NEWLINE)
         && pc->SafeToDeleteNl())
      {
         pc = pc->GetPrev();
         Chunk *next = pc->GetNext();
         Chunk::Delete(next);
         MARK_CHANGE();
      }
   }
} // collapse_empty_body


static void newlines_brace_pair(Chunk *br_open)
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
            vector<Chunk> saved_chunk;
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
            val = IARF_NOT_DEFINED;
            log_rule_B("nl_fdef_brace_cond");
            const iarf_e nl_fdef_brace_cond_v = options::nl_fdef_brace_cond();

            if (nl_fdef_brace_cond_v != IARF_IGNORE)
            {
               prev = br_open->GetPrevNcNnlNi();   // Issue #2279

               if (prev->Is(CT_FPAREN_CLOSE))
               {
                  val = nl_fdef_brace_cond_v;
               }
            }

            if (val == IARF_NOT_DEFINED)
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

         if (options::nl_collapse_empty_body_functions())
         {
            collapse_empty_body(br_open);
            return;
         }
      }
      else
      {
         log_rule_B("nl_collapse_empty_body");

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


static void newline_case(Chunk *start)
{
   LOG_FUNC_ENTRY();

   //   printf("%s case (%s) on line %d col %d\n",
   //          __func__, c_chunk_names[start->GetType()],
   //          start->GetOrigLine(), start->GetOrigCol());

   // Scan backwards until a '{' or ';' or ':'. Abort if a multi-newline is found
   Chunk *prev = start;

   do
   {
      prev = prev->GetPrevNc();

      if (  prev->IsNotNullChunk()
         && prev->IsNewline()
         && prev->GetNlCount() > 1)
      {
         return;
      }
   } while (  prev->IsNot(CT_BRACE_OPEN)
           && prev->IsNot(CT_BRACE_CLOSE)
           && prev->IsNot(CT_SEMICOLON)
           && prev->IsNot(CT_CASE_COLON));

   if (prev->IsNullChunk())
   {
      return;
   }
   Chunk *pc = newline_add_between(prev, start);

   if (pc == nullptr)
   {
      return;
   }

   // Only add an extra line after a semicolon or brace close
   if (  prev->Is(CT_SEMICOLON)
      || prev->Is(CT_BRACE_CLOSE))
   {
      if (  pc->IsNewline()
         && pc->GetNlCount() < 2)
      {
         double_newline(pc);
      }
   }
} // newline_case


static void newline_case_colon(Chunk *start)
{
   LOG_FUNC_ENTRY();

   // Scan forwards until a non-comment is found
   Chunk *pc = start;

   do
   {
      pc = pc->GetNext();
   } while (pc->IsComment());

   if (  pc->IsNotNullChunk()
      && !pc->IsNewline())
   {
      newline_add_before(pc);
   }
} // newline_case_colon


static void newline_before_return(Chunk *start)
{
   LOG_FUNC_ENTRY();

   Chunk *pc = Chunk::NullChunkPtr;

   if (start != nullptr)
   {
      pc = start->GetPrev();
   }
   Chunk *nl = pc;

   // Skip over single preceding newline
   if (pc->IsNewline())
   {
      // Do we already have a blank line?
      if (nl->GetNlCount() > 1)
      {
         return;
      }
      pc = nl->GetPrev();
   }

   // Skip over preceding comments that are not a trailing comment, taking
   // into account that comment blocks may span multiple lines.
   // Trailing comments are considered part of the previous token, not the
   // return statement.  They are handled below.
   while (  pc->IsComment()
         && pc->GetParentType() != CT_COMMENT_END)
   {
      pc = pc->GetPrev();

      if (!pc->IsNewline())
      {
         return;
      }
      nl = pc;
      pc = pc->GetPrev();
   }
   pc = nl->GetPrev();

   // Peek over trailing comment of previous token
   if (  pc->IsComment()
      && pc->GetParentType() == CT_COMMENT_END)
   {
      pc = pc->GetPrev();
   }

   // Don't add extra blanks after an opening brace or a case statement
   if (  pc->IsNullChunk()
      || (  pc->Is(CT_BRACE_OPEN)
         || pc->Is(CT_VBRACE_OPEN)
         || pc->Is(CT_CASE_COLON)))
   {
      return;
   }

   if (  nl->IsNewline()
      && nl->GetNlCount() < 2)
   {
      nl->SetNlCount(nl->GetNlCount() + 1);
      MARK_CHANGE();
      LOG_FMT(LBLANK, "%s(%d): orig line is %zu, orig col is %zu, text is '%s', new line count is now %zu\n",
              __func__, __LINE__, nl->GetOrigLine(), nl->GetOrigCol(), nl->Text(), nl->GetNlCount());
   }
} // newline_before_return


static void newline_after_return(Chunk *start)
{
   LOG_FUNC_ENTRY();

   Chunk *semi  = start->GetNextType(CT_SEMICOLON, start->GetLevel());
   Chunk *after = semi->GetNextNcNnlNet();

   // If we hit a brace or an 'else', then a newline isn't needed
   if (  after->IsNullChunk()
      || after->IsBraceClose()
      || after->Is(CT_ELSE))
   {
      return;
   }
   Chunk *pc;

   for (pc = semi->GetNext(); pc != after; pc = pc->GetNext())
   {
      if (pc->Is(CT_NEWLINE))
      {
         if (pc->GetNlCount() < 2)
         {
            double_newline(pc);
         }
         return;
      }
   }
} // newline_after_return


static void newline_iarf_pair(Chunk *before, Chunk *after, iarf_e av, bool check_nl_assign_leave_one_liners)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNEWLINE, "%s(%d): ", __func__, __LINE__);
   log_func_stack(LNEWLINE, "CallStack:");

   if (  before == nullptr
      || before == Chunk::NullChunkPtr
      || after == nullptr
      || after == Chunk::NullChunkPtr
      || after->Is(CT_IGNORED))
   {
      return;
   }

   if (av & IARF_ADD)
   {
      if (  check_nl_assign_leave_one_liners
         && options::nl_assign_leave_one_liners()
         && after->TestFlags(PCF_ONE_LINER))
      {
         log_rule_B("nl_assign_leave_one_liners");
         return;
      }
      Chunk *nl = newline_add_between(before, after);
      LOG_FMT(LNEWLINE, "%s(%d): newline_add_between '%s' and '%s'\n",
              __func__, __LINE__, before->Text(), after->Text());

      if (  nl != nullptr
         && av == IARF_FORCE
         && nl->GetNlCount() > 1)
      {
         nl->SetNlCount(1);
      }
   }
   else if (av & IARF_REMOVE)
   {
      LOG_FMT(LNEWLINE, "%s(%d): newline_remove_between '%s' and '%s'\n",
              __func__, __LINE__, before->Text(), after->Text());
      newline_del_between(before, after);
   }
} // newline_iarf_pair


void newline_iarf(Chunk *pc, iarf_e av)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNFD, "%s(%d): ", __func__, __LINE__);
   log_func_stack(LNFD, "CallStack:");
   Chunk *after = Chunk::NullChunkPtr;

   if (pc != nullptr)
   {
      after = pc->GetNextNnl();
   }

   if (  (pc != nullptr && pc->Is(CT_FPAREN_OPEN))                         // Issue #2914
      && pc->GetParentType() == CT_FUNC_CALL
      && after->Is(CT_COMMENT_CPP)
      && options::donot_add_nl_before_cpp_comment())
   {
      return;
   }
   newline_iarf_pair(pc, after, av);
} // newline_iarf


static void newline_func_multi_line(Chunk *start)
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


static void newline_template(Chunk *start)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNFD, "%s(%d): called on %zu:%zu '%s' [%s/%s]\n",
           __func__, __LINE__, start->GetOrigLine(), start->GetOrigCol(),
           start->Text(), get_token_name(start->GetType()), get_token_name(start->GetParentType()));

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
   Chunk *pc = start->GetNextNcNnl();

   while (  pc->IsNotNullChunk()
         && pc->GetLevel() > start->GetLevel())
   {
      pc = pc->GetNextNcNnl();
   }

   if (pc->Is(CT_ANGLE_CLOSE))
   {
      if (add_start)
      {
         newline_iarf(start, IARF_ADD);
      }

      if (add_end)
      {
         newline_iarf(pc->GetPrev(), IARF_ADD);
      }

      if (add_args)
      {
         Chunk *pc_1;

         for (pc_1 = start->GetNextNcNnl();
              pc_1->IsNotNullChunk() && pc_1->GetLevel() > start->GetLevel();
              pc_1 = pc_1->GetNextNcNnl())
         {
            if (  pc_1->Is(CT_COMMA)
               && (pc_1->GetLevel() == (start->GetLevel() + 1)))
            {
               Chunk *tmp = pc_1->GetNext();

               if (tmp->IsComment())
               {
                  pc_1 = tmp;
               }

               if (!pc_1->GetNext()->IsNewline())
               {
                  newline_iarf(pc_1, IARF_ADD);
               }
            }
         }
      }
   }
} // newline_template


static void newline_func_def_or_call(Chunk *start)
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
                  && (  brace == nullptr
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


static void newline_oc_msg(Chunk *start)
{
   LOG_FUNC_ENTRY();

   Chunk *sq_c = start->GetClosingParen();

   if (sq_c->IsNullChunk())
   {
      return;
   }
   log_rule_B("nl_oc_msg_leave_one_liner");

   if (options::nl_oc_msg_leave_one_liner())
   {
      return;
   }
   bool should_nl_msg = false;

   // Get count of parameters
   size_t parameter_count = 0;

   for (Chunk *pc = start->GetNextNcNnl(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      if (pc->GetLevel() <= start->GetLevel())
      {
         break;
      }

      if (pc->Is(CT_OC_COLON) && pc->GetLevel() - 1 == start->GetLevel())
      {
         parameter_count++;
      }
   }

   size_t min_params = options::nl_oc_msg_args_min_params();

   if (  parameter_count >= min_params
      && min_params != 0)
   {
      should_nl_msg = true;
   }
   // Get length of longest line
   size_t longest_line = 0;

   for (Chunk *pc = start->GetNextNcNnl(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      if (pc->GetLevel() <= start->GetLevel())
      {
         break;
      }

      if (pc->GetOrigColEnd() > longest_line)
      {
         longest_line = pc->GetOrigColEnd();
      }
   }

   size_t max_code_width = options::nl_oc_msg_args_max_code_width();

   if (  longest_line > max_code_width
      && max_code_width != 0)
   {
      should_nl_msg = true;
   }

   // If both nl_oc_msg_args_min_params and nl_oc_msg_args_max_code_width are disabled
   // we should newline all messages.
   if (  max_code_width == 0
      && min_params == 0)
   {
      should_nl_msg = true;
   }

   if (!should_nl_msg)
   {
      return;
   }

   for (Chunk *pc = start->GetNextNcNnl(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      if (pc->GetLevel() <= start->GetLevel())
      {
         break;
      }

      if (pc->Is(CT_OC_MSG_NAME) && pc->GetLevel() - 1 == start->GetLevel())
      {
         newline_add_before(pc);
      }
   }
} // newline_oc_msg


static bool one_liner_nl_ok(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNL1LINE, "%s(%d): check type is %s, parent is %s, flag is %s, orig line is %zu, orig col is %zu\n",
           __func__, __LINE__, get_token_name(pc->GetType()), get_token_name(pc->GetParentType()),
           pcf_flags_str(pc->GetFlags()).c_str(), pc->GetOrigLine(), pc->GetOrigCol());

   if (!pc->TestFlags(PCF_ONE_LINER))
   {
      LOG_FMT(LNL1LINE, "%s(%d): true (not 1-liner), a new line may be added\n", __func__, __LINE__);
      return(true);
   }
   // Step back to find the opening brace
   Chunk *br_open = pc;

   if (br_open->IsBraceClose())
   {
      br_open = br_open->GetPrevType(br_open->Is(CT_BRACE_CLOSE) ? CT_BRACE_OPEN : CT_VBRACE_OPEN,
                                     br_open->GetLevel(), E_Scope::ALL);
   }
   else
   {
      while (  br_open->IsNotNullChunk()
            && br_open->TestFlags(PCF_ONE_LINER)
            && !br_open->IsBraceOpen()
            && !br_open->IsBraceClose())
      {
         br_open = br_open->GetPrev();
      }
   }
   pc = br_open;

   if (  pc->IsNotNullChunk()
      && pc->TestFlags(PCF_ONE_LINER)
      && (  pc->IsBraceOpen()
         || pc->IsBraceClose()))
   {
      log_rule_B("nl_class_leave_one_liners");

      if (  options::nl_class_leave_one_liners()
         && pc->TestFlags(PCF_IN_CLASS))
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (class)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_assign_leave_one_liners");

      if (  options::nl_assign_leave_one_liners()
         && pc->GetParentType() == CT_ASSIGN)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (assign)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_enum_leave_one_liners");

      if (  options::nl_enum_leave_one_liners()
         && pc->GetParentType() == CT_ENUM)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (enum)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_getset_leave_one_liners");

      if (  options::nl_getset_leave_one_liners()
         && pc->GetParentType() == CT_GETSET)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (get/set), a new line may NOT be added\n", __func__, __LINE__);
         return(false);
      }
      // Issue #UT-98
      log_rule_B("nl_cs_property_leave_one_liners");

      if (  options::nl_cs_property_leave_one_liners()
         && pc->GetParentType() == CT_CS_PROPERTY)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (c# property), a new line may NOT be added\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_func_leave_one_liners");

      if (  options::nl_func_leave_one_liners()
         && (  pc->GetParentType() == CT_FUNC_DEF
            || pc->GetParentType() == CT_FUNC_CLASS_DEF))
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (func def)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_func_leave_one_liners");

      if (  options::nl_func_leave_one_liners()
         && pc->GetParentType() == CT_OC_MSG_DECL)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (method def)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_cpp_lambda_leave_one_liners");

      if (  options::nl_cpp_lambda_leave_one_liners()
         && ((pc->GetParentType() == CT_CPP_LAMBDA)))
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (lambda)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_oc_msg_leave_one_liner");

      if (  options::nl_oc_msg_leave_one_liner()
         && pc->TestFlags(PCF_IN_OC_MSG))
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (message)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_if_leave_one_liners");

      if (  options::nl_if_leave_one_liners()
         && (  pc->GetParentType() == CT_IF
            || pc->GetParentType() == CT_ELSEIF
            || pc->GetParentType() == CT_ELSE))
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (if/else)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_while_leave_one_liners");

      if (  options::nl_while_leave_one_liners()
         && pc->GetParentType() == CT_WHILE)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (while)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_do_leave_one_liners");

      if (  options::nl_do_leave_one_liners()
         && pc->GetParentType() == CT_DO)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (do)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_for_leave_one_liners");

      if (  options::nl_for_leave_one_liners()
         && pc->GetParentType() == CT_FOR)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (for)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_namespace_two_to_one_liner - 2");

      if (  options::nl_namespace_two_to_one_liner()
         && pc->GetParentType() == CT_NAMESPACE)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (namespace)\n", __func__, __LINE__);
         return(false);
      }
   }
   LOG_FMT(LNL1LINE, "%s(%d): true, a new line may be added\n", __func__, __LINE__);
   return(true);
} // one_liner_nl_ok


void undo_one_liner(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   if (  pc != nullptr
      && pc->TestFlags(PCF_ONE_LINER))
   {
      LOG_FMT(LNL1LINE, "%s(%d): pc->Text() '%s', orig line is %zu, orig col is %zu",
              __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());
      pc->ResetFlagBits(PCF_ONE_LINER);

      // scan backward
      LOG_FMT(LNL1LINE, "%s(%d): scan backward\n", __func__, __LINE__);
      Chunk *tmp = pc;

      while ((tmp = tmp->GetPrev())->IsNotNullChunk())
      {
         if (!tmp->TestFlags(PCF_ONE_LINER))
         {
            LOG_FMT(LNL1LINE, "%s(%d): tmp->Text() '%s', orig line is %zu, orig col is %zu, --> break\n",
                    __func__, __LINE__, tmp->Text(), tmp->GetOrigLine(), tmp->GetOrigCol());
            break;
         }
         LOG_FMT(LNL1LINE, "%s(%d): clear for tmp->Text() '%s', orig line is %zu, orig col is %zu",
                 __func__, __LINE__, tmp->Text(), tmp->GetOrigLine(), tmp->GetOrigCol());
         tmp->ResetFlagBits(PCF_ONE_LINER);
      }
      // scan forward
      LOG_FMT(LNL1LINE, "%s(%d): scan forward\n", __func__, __LINE__);
      tmp = pc;
      LOG_FMT(LNL1LINE, "%s(%d): - \n", __func__, __LINE__);

      while ((tmp = tmp->GetNext())->IsNotNullChunk())
      {
         if (!tmp->TestFlags(PCF_ONE_LINER))
         {
            LOG_FMT(LNL1LINE, "%s(%d): tmp->Text() '%s', orig line is %zu, orig col is %zu, --> break\n",
                    __func__, __LINE__, tmp->Text(), tmp->GetOrigLine(), tmp->GetOrigCol());
            break;
         }
         LOG_FMT(LNL1LINE, "%s(%d): clear for tmp->Text() '%s', orig line is %zu, orig col is %zu",
                 __func__, __LINE__, tmp->Text(), tmp->GetOrigLine(), tmp->GetOrigCol());
         tmp->ResetFlagBits(PCF_ONE_LINER);
      }
      LOG_FMT(LNL1LINE, "\n");
   }
} // undo_one_liner


static void nl_create_one_liner(Chunk *vbrace_open)
{
   LOG_FUNC_ENTRY();

   // See if we get a newline between the next text and the vbrace_close
   Chunk *tmp   = vbrace_open->GetNextNcNnl();
   Chunk *first = tmp;

   if (  first->IsNullChunk()
      || get_token_pattern_class(first->GetType()) != pattern_class_e::NONE)
   {
      return;
   }
   size_t nl_total = 0;

   while (tmp->IsNot(CT_VBRACE_CLOSE))
   {
      if (tmp->IsNewline())
      {
         nl_total += tmp->GetNlCount();

         if (nl_total > 1)
         {
            return;
         }
      }
      tmp = tmp->GetNext();
   }

   if (  tmp->IsNotNullChunk()
      && first->IsNotNullChunk())
   {
      newline_del_between(vbrace_open, first);
   }
} // nl_create_one_liner


static void nl_create_list_liner(Chunk *brace_open)
{
   LOG_FUNC_ENTRY();

   // See if we get a newline between the next text and the vbrace_close
   if (brace_open == nullptr)
   {
      return;
   }
   Chunk *closing = brace_open->GetNextType(CT_BRACE_CLOSE, brace_open->GetLevel());
   Chunk *tmp     = brace_open;

   do
   {
      if (tmp->Is(CT_COMMA))
      {
         return;
      }
      tmp = tmp->GetNext();
   } while (tmp != closing);

   newline_del_between(brace_open, closing);
} // nl_create_list_liner


void newlines_remove_newlines()
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LBLANK, "%s(%d):\n", __func__, __LINE__);
   Chunk *pc = Chunk::GetHead();

   if (!pc->IsNewline())
   {
      pc = pc->GetNextNl();
   }
   Chunk *next;
   Chunk *prev;

   while (pc->IsNotNullChunk())
   {
      // Remove all newlines not in preproc
      if (!pc->TestFlags(PCF_IN_PREPROC))
      {
         next = pc->GetNext();
         prev = pc->GetPrev();
         newline_iarf(pc, IARF_REMOVE);

         if (next == Chunk::GetHead())
         {
            pc = next;
            continue;
         }
         else if (  prev->IsNotNullChunk()
                 && !prev->GetNext()->IsNewline())
         {
            pc = prev;
         }
      }
      pc = pc->GetNextNl();
   }
} // newlines_remove_newlines


void newlines_remove_disallowed()
{
   LOG_FUNC_ENTRY();

   Chunk *pc = Chunk::GetHead();
   Chunk *next;

   while ((pc = pc->GetNextNl())->IsNotNullChunk())
   {
      LOG_FMT(LBLANKD, "%s(%d): orig line is %zu, orig col is %zu, <Newline>, nl is %zu\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetNlCount());

      next = pc->GetNext();

      if (  next->IsNotNullChunk()
         && !next->Is(CT_NEWLINE)
         && !can_increase_nl(pc))
      {
         LOG_FMT(LBLANKD, "%s(%d): force to 1 orig line is %zu, orig col is %zu\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol());

         if (pc->GetNlCount() != 1)
         {
            pc->SetNlCount(1);
            MARK_CHANGE();
         }
      }
   }
} // newlines_remove_disallowed


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
   dump_step(dump_file_name, "new 2");

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

         if (  language_is_set(LANG_OC)
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
               log_ruleNL("nl_class_brace", pc->GetPrevNnl());
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
         log_rule_NL("nl_after_semicolon");

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
         log_rule_NL("nl_remove_extra_newlines");
         newline_iarf(pc, IARF_REMOVE);
      }
      else if (  pc->Is(CT_MEMBER)
              && (  language_is_set(LANG_JAVA)
                 || language_is_set(LANG_CPP)))                 // Issue #2574
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


static void nl_handle_define(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   Chunk *nl  = pc;
   Chunk *ref = Chunk::NullChunkPtr;

   while ((nl = nl->GetNext())->IsNotNullChunk())
   {
      if (nl->Is(CT_NEWLINE))
      {
         return;
      }

      if (  nl->Is(CT_MACRO)
         || (  nl->Is(CT_FPAREN_CLOSE)
            && nl->GetParentType() == CT_MACRO_FUNC))
      {
         ref = nl;
      }

      if (nl->Is(CT_NL_CONT))
      {
         if (ref->IsNotNullChunk())
         {
            newline_add_after(ref);
         }
         return;
      }
   }
} // nl_handle_define


void newline_after_multiline_comment()
{
   LOG_FUNC_ENTRY();

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (pc->IsNot(CT_COMMENT_MULTI))
      {
         continue;
      }
      Chunk *tmp = pc;

      while (  ((tmp = tmp->GetNext())->IsNotNullChunk())
            && !tmp->IsNewline())
      {
         if (!tmp->IsComment())
         {
            newline_add_before(tmp);
            break;
         }
      }
   }
} // newline_after_multiline_comment


void newline_after_label_colon()
{
   LOG_FUNC_ENTRY();

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (pc->IsNot(CT_LABEL_COLON))
      {
         continue;
      }
      newline_add_after(pc);
   }
} // newline_after_label_colon


static bool is_class_one_liner(Chunk *pc)
{
   if (  (  pc->Is(CT_FUNC_CLASS_DEF)
         || pc->Is(CT_FUNC_DEF))
      && pc->TestFlags(PCF_IN_CLASS))
   {
      // Find opening brace
      pc = pc->GetNextType(CT_BRACE_OPEN, pc->GetLevel());
      return(  pc->IsNotNullChunk()
            && pc->TestFlags(PCF_ONE_LINER));
   }
   return(false);
} // is_class_one_liner


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


void newlines_functions_remove_extra_blank_lines()
{
   LOG_FUNC_ENTRY();

   const size_t nl_max_blank_in_func = options::nl_max_blank_in_func();

   log_rule_B("nl_max_blank_in_func");

   if (nl_max_blank_in_func == 0)
   {
      LOG_FMT(LNEWLINE, "%s(%d): nl_max_blank_in_func is zero\n", __func__, __LINE__);
      return;
   }

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      LOG_FMT(LNEWLINE, "%s(%d): orig line is %zu, orig col is %zu, Text() '%s', type is %s\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), get_token_name(pc->GetType()));

      if (  pc->IsNot(CT_BRACE_OPEN)
         || (  pc->GetParentType() != CT_FUNC_DEF
            && pc->GetParentType() != CT_CPP_LAMBDA))
      {
         continue;
      }
      const size_t startMoveLevel = pc->GetLevel();

      while (pc->IsNotNullChunk())
      {
         if (  pc->Is(CT_BRACE_CLOSE)
            && pc->GetLevel() == startMoveLevel)
         {
            break;
         }

         // delete newlines
         if (  !pc->Is(CT_COMMENT_MULTI)   // Issue #2195
            && pc->GetNlCount() > nl_max_blank_in_func)
         {
            LOG_FMT(LNEWLINE, "%s(%d): orig line is %zu, orig col is %zu, Text() '%s', type is %s\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), get_token_name(pc->GetType()));
            pc->SetNlCount(nl_max_blank_in_func);
            MARK_CHANGE();
            remove_next_newlines(pc);
         }
         else
         {
            pc = pc->GetNext();
         }
      }
   }
} // newlines_functions_remove_extra_blank_lines


void newlines_squeeze_ifdef()
{
   LOG_FUNC_ENTRY();

   Chunk *pc;

   for (pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      if (  pc->Is(CT_PREPROC)
         && (  pc->GetLevel() > 0
            || options::nl_squeeze_ifdef_top_level()))
      {
         log_rule_B("nl_squeeze_ifdef_top_level");
         Chunk *ppr = pc->GetNext();

         if (  ppr->Is(CT_PP_IF)
            || ppr->Is(CT_PP_ELSE)
            || ppr->Is(CT_PP_ENDIF))
         {
            Chunk *pnl = Chunk::NullChunkPtr;
            Chunk *nnl = ppr->GetNextNl();

            if (  ppr->Is(CT_PP_ELSE)
               || ppr->Is(CT_PP_ENDIF))
            {
               pnl = pc->GetPrevNl();
            }
            Chunk *tmp1;
            Chunk *tmp2;

            if (nnl->IsNotNullChunk())
            {
               if (pnl->IsNotNullChunk())
               {
                  if (pnl->GetNlCount() > 1)
                  {
                     pnl->SetNlCount(1);
                     MARK_CHANGE();

                     tmp1 = pnl->GetPrevNnl();
                     tmp2 = nnl->GetPrevNnl();

                     LOG_FMT(LNEWLINE, "%s(%d): moved from after line %zu to after %zu\n",
                             __func__, __LINE__, tmp1->GetOrigLine(), tmp2->GetOrigLine());
                  }
               }

               if (  ppr->Is(CT_PP_IF)
                  || ppr->Is(CT_PP_ELSE))
               {
                  if (nnl->GetNlCount() > 1)
                  {
                     tmp1 = nnl->GetPrevNnl();
                     LOG_FMT(LNEWLINE, "%s(%d): trimmed newlines after line %zu from %zu\n",
                             __func__, __LINE__, tmp1->GetOrigLine(), nnl->GetNlCount());
                     nnl->SetNlCount(1);
                     MARK_CHANGE();
                  }
               }
            }
         }
      }
   }
} // newlines_squeeze_ifdef


void newlines_squeeze_paren_close()
{
   LOG_FUNC_ENTRY();

   Chunk *pc;

   for (pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      Chunk *next;
      Chunk *prev;

      if (pc->Is(CT_NEWLINE))
      {
         prev = pc->GetPrev();
      }
      else
      {
         prev = pc;
      }
      next = pc->GetNext();

      if (  next->IsNotNullChunk()
         && prev->IsNotNullChunk()
         && next->IsParenClose()
         && prev->IsParenClose())
      {
         Chunk *prev_op = prev->GetOpeningParen();
         Chunk *next_op = next->GetOpeningParen();
         bool  flag     = true;

         Chunk *tmp = prev;

         while (tmp->IsParenClose())
         {
            tmp = tmp->GetPrev();
         }

         if (tmp->IsNot(CT_NEWLINE))
         {
            flag = false;
         }

         if (flag)
         {
            if (next_op->IsOnSameLine(prev_op))
            {
               if (pc->Is(CT_NEWLINE))
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


void newlines_eat_start_end()
{
   LOG_FUNC_ENTRY();

   Chunk *pc;

   // Process newlines at the start of the file
   if (  cpd.frag_cols == 0
      && (  (options::nl_start_of_file() & IARF_REMOVE)
         || (  (options::nl_start_of_file() & IARF_ADD)
            && (options::nl_start_of_file_min() > 0))))
   {
      log_rule_B("nl_start_of_file");
      log_rule_B("nl_start_of_file_min");
      pc = Chunk::GetHead();

      if (pc->IsNotNullChunk())
      {
         if (pc->Is(CT_NEWLINE))
         {
            if (options::nl_start_of_file() == IARF_REMOVE)
            {
               log_rule_B("nl_start_of_file");
               LOG_FMT(LBLANKD, "%s(%d): eat_blanks_start_of_file %zu\n",
                       __func__, __LINE__, pc->GetOrigLine());
               Chunk::Delete(pc);
               MARK_CHANGE();
            }
            else if (  options::nl_start_of_file() == IARF_FORCE
                    || (pc->GetNlCount() < options::nl_start_of_file_min()))
            {
               log_rule_B("nl_start_of_file");
               LOG_FMT(LBLANKD, "%s(%d): set_blanks_start_of_file %zu\n",
                       __func__, __LINE__, pc->GetOrigLine());
               pc->SetNlCount(options::nl_start_of_file_min());
               log_rule_B("nl_start_of_file_min");
               MARK_CHANGE();
            }
         }
         else if (  (options::nl_start_of_file() & IARF_ADD)
                 && (options::nl_start_of_file_min() > 0))
         {
            log_rule_B("nl_start_of_file");
            log_rule_B("nl_start_of_file_min");
            Chunk chunk;
            chunk.SetType(CT_NEWLINE);
            chunk.SetOrigLine(pc->GetOrigLine());
            chunk.SetOrigCol(pc->GetOrigCol());
            chunk.SetPpLevel(pc->GetPpLevel());
            chunk.SetNlCount(options::nl_start_of_file_min());
            log_rule_B("nl_start_of_file_min");
            chunk.CopyAndAddBefore(pc);
            LOG_FMT(LNEWLINE, "%s(%d): %zu:%zu add newline before '%s'\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text());
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
      pc = Chunk::GetTail();

      if (pc->IsNotNullChunk())
      {
         if (pc->Is(CT_NEWLINE))
         {
            if (options::nl_end_of_file() == IARF_REMOVE)
            {
               log_rule_B("nl_end_of_file");
               LOG_FMT(LBLANKD, "%s(%d): eat_blanks_end_of_file %zu\n",
                       __func__, __LINE__, pc->GetOrigLine());
               Chunk::Delete(pc);
               MARK_CHANGE();
            }
            else if (  options::nl_end_of_file() == IARF_FORCE
                    || (pc->GetNlCount() < options::nl_end_of_file_min()))
            {
               log_rule_B("nl_end_of_file");
               log_rule_B("nl_end_of_file_min");

               if (pc->GetNlCount() != options::nl_end_of_file_min())
               {
                  log_rule_B("nl_end_of_file_min");
                  LOG_FMT(LBLANKD, "%s(%d): set_blanks_end_of_file %zu\n",
                          __func__, __LINE__, pc->GetOrigLine());
                  pc->SetNlCount(options::nl_end_of_file_min());
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
            Chunk chunk;
            chunk.SetType(CT_NEWLINE);
            chunk.SetOrigLine(pc->GetOrigLine());
            chunk.SetOrigCol(pc->GetOrigCol());
            chunk.SetPpLevel(pc->GetPpLevel());
            chunk.SetNlCount(options::nl_end_of_file_min());
            log_rule_B("nl_end_of_file_min");
            chunk.CopyAndAddBefore(Chunk::NullChunkPtr);
            LOG_FMT(LNEWLINE, "%s(%d): %zu:%zu add newline after '%s'\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text());
            MARK_CHANGE();
         }
      }
   }
} // newlines_eat_start_end


void newlines_chunk_pos(E_Token chunk_type, token_pos_e mode)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNEWLINE, "%s(%d): mode is %s\n",
           __func__, __LINE__, to_string(mode));

   if (  !(mode & (TP_JOIN | TP_LEAD | TP_TRAIL))
      && chunk_type != CT_COMMA)
   {
      return;
   }

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      char copy[1000];
      LOG_FMT(LNEWLINE, "%s(%d): pc orig line is %zu, orig col is %zu, Text() is '%s'\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->ElidedText(copy));
      // produces much more log output. Use it only debugging purpose
      //log_pcf_flags(LNEWLINE, pc->GetFlags());

      if (pc->Is(chunk_type))
      {
         token_pos_e mode_local;

         if (chunk_type == CT_COMMA)
         {
            LOG_FMT(LNEWLINE, "%s(%d): orig line is %zu, orig col is %zu, Text() is '%s', type is %s\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), get_token_name(pc->GetType()));
            // produces much more log output. Use it only debugging purpose
            //log_pcf_flags(LNEWLINE, pc->GetFlags());

            if (pc->TestFlags(PCF_IN_CONST_ARGS)) // Issue #2250
            {
               continue;
            }

            /*
             * for chunk_type == CT_COMMA
             * we get 'mode' from options::pos_comma()
             * BUT we must take care of options::pos_class_comma()
             * TODO and options::pos_constr_comma()
             */
            if (pc->TestFlags(PCF_IN_CLASS_BASE))
            {
               // change mode
               log_rule_B("pos_class_comma");
               mode_local = options::pos_class_comma();
            }
            else if (pc->TestFlags(PCF_IN_ENUM))
            {
               log_rule_B("pos_enum_comma");
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
         Chunk *prev = pc->GetPrevNc();
         Chunk *next = pc->GetNextNc();

         LOG_FMT(LNEWLINE, "%s(%d): mode_local is %s\n",
                 __func__, __LINE__, to_string(mode_local));

         LOG_FMT(LNEWLINE, "%s(%d): prev orig line is %zu, orig col is %zu, Text() is '%s'\n",
                 __func__, __LINE__, prev->GetOrigLine(), prev->GetOrigCol(), prev->Text());
         LOG_FMT(LNEWLINE, "%s(%d): next orig line is %zu, orig col is %zu, Text() is '%s'\n",
                 __func__, __LINE__, next->GetOrigLine(), next->GetOrigCol(), next->Text());
         size_t nl_flag = ((prev->IsNewline() ? 1 : 0) |
                           (next->IsNewline() ? 2 : 0));
         LOG_FMT(LNEWLINE, "%s(%d): nl_flag is %zu\n",
                 __func__, __LINE__, nl_flag);

         if (mode_local & TP_JOIN)
         {
            if (nl_flag & 1)
            {
               // remove newline if not preceded by a comment
               Chunk *prev2 = prev->GetPrev();

               if (  prev2->IsNotNullChunk()
                  && !(prev2->IsComment()))
               {
                  remove_next_newlines(prev2);
               }
            }

            if (nl_flag & 2)
            {
               // remove newline if not followed by a comment or by '{'
               Chunk *next2 = next->GetNext();

               if (  next2->IsNotNullChunk()
                  && !next2->IsComment()
                  && !next2->Is(CT_BRACE_OPEN))
               {
                  remove_next_newlines(pc);
               }
            }
            continue;
         }

         if (  (  nl_flag == 0
               && !(mode_local & (TP_FORCE | TP_BREAK)))
            || (  nl_flag == 3
               && !(mode_local & TP_FORCE)))
         {
            // No newlines and not adding any or both and not forcing
            continue;
         }

         if (  (  (mode_local & TP_LEAD)
               && nl_flag == 1)
            || (  (mode_local & TP_TRAIL)
               && nl_flag == 2))
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
               remove_next_newlines(pc->GetPrevNcNnlNi());   // Issue #2279
            }
            continue;
         }

         // we need to move the newline
         if (mode_local & TP_LEAD)
         {
            Chunk *next2 = next->GetNext();

            if (  next2->Is(CT_PREPROC)
               || (  chunk_type == CT_ASSIGN
                  && next2->Is(CT_BRACE_OPEN)))
            {
               continue;
            }

            if (next->GetNlCount() == 1)
            {
               if (  prev != nullptr
                  && !prev->TestFlags(PCF_IN_PREPROC))
               {
                  // move the CT_BOOL to after the newline
                  pc->MoveAfter(next);
               }
            }
         }
         else
         {
            LOG_FMT(LNEWLINE, "%s(%d): prev orig line is %zu, orig col is %zu, Text() is '%s', new line count is %zu\n",
                    __func__, __LINE__, prev->GetOrigLine(), prev->GetOrigCol(), prev->Text(), prev->GetNlCount());

            if (prev->GetNlCount() == 1)
            {
               // Back up to the next non-comment item
               prev = prev->GetPrevNc();
               LOG_FMT(LNEWLINE, "%s(%d): prev orig line is %zu, orig col is %zu, Text() is '%s'\n",
                       __func__, __LINE__, prev->GetOrigLine(), prev->GetOrigCol(), prev->Text());

               if (  prev->IsNotNullChunk()
                  && !prev->IsNewline()
                  && !prev->TestFlags(PCF_IN_PREPROC)
                  && !prev->TestFlags(PCF_IN_OC_MSG))
               {
                  pc->MoveAfter(prev);
               }
            }
         }
      }
   }
} // newlines_chunk_pos


void newlines_class_colon_pos(E_Token tok)
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
   Chunk  *ccolon  = nullptr;
   size_t acv_span = options::align_constr_value_span();

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

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      if (  ccolon == nullptr
         && pc->IsNot(tok))
      {
         continue;
      }
      Chunk *prev;
      Chunk *next;

      if (pc->Is(tok))
      {
         LOG_FMT(LBLANKD, "%s(%d): orig line is %zu, orig col is %zu, Text() '%s', type is %s\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), get_token_name(pc->GetType()));
         ccolon = pc;
         prev   = pc->GetPrevNc();
         next   = pc->GetNextNc();

         if (pc->Is(CT_CONSTR_COLON))
         {
            LOG_FMT(LBLANKD, "%s(%d): pc orig line is %zu, orig col is %zu, Text() '%s', type is %s\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), get_token_name(pc->GetType()));
            Chunk *paren_vor_value = pc->GetNextType(CT_FPAREN_OPEN, pc->GetLevel());

            if (  with_acv
               && paren_vor_value->IsNotNullChunk())
            {
               LOG_FMT(LBLANKD, "%s(%d): paren_vor_value orig line is %zu, orig col is %zu, Text() '%s', type is %s\n",
                       __func__, __LINE__, paren_vor_value->GetOrigLine(), paren_vor_value->GetOrigCol(),
                       paren_vor_value->Text(), get_token_name(paren_vor_value->GetType()));
               constructorValue.NewLines(paren_vor_value->GetNlCount());
               constructorValue.Add(paren_vor_value);
            }
         }

         if (  !prev->IsNewline()
            && !next->IsNewline()
            && (anc & IARF_ADD))                   // nl_class_colon, nl_constr_colon: 1

         {
            newline_add_after(pc);
            prev = pc->GetPrevNc();
            next = pc->GetNextNc();
         }

         if (anc == IARF_REMOVE)                   // nl_class_colon, nl_constr_colon: 2
         {
            if (  prev->IsNewline()
               && prev->SafeToDeleteNl())
            {
               Chunk::Delete(prev);
               MARK_CHANGE();
               prev = pc->GetPrevNc();
            }

            if (  next->IsNewline()
               && next->SafeToDeleteNl())
            {
               Chunk::Delete(next);
               MARK_CHANGE();
               next = pc->GetNextNc();
            }
         }

         if (tpc & TP_TRAIL)                       // pos_class_colon, pos_constr_colon: 4
         {
            if (  prev->IsNewline()
               && prev->GetNlCount() == 1
               && prev->SafeToDeleteNl())
            {
               pc->Swap(prev);
            }
         }
         else if (tpc & TP_LEAD)                   // pos_class_colon, pos_constr_colon: 3
         {
            if (  next->IsNewline()
               && next->GetNlCount() == 1
               && next->SafeToDeleteNl())
            {
               pc->Swap(next);
            }
         }
      }
      else
      {
         // (pc->GetType() != tok)
         if (  pc->Is(CT_BRACE_OPEN)
            || pc->Is(CT_SEMICOLON))
         {
            ccolon = nullptr;

            if (with_acv)
            {
               constructorValue.End();
            }
            continue;
         }

         if (  pc->Is(CT_COMMA)
            && pc->GetLevel() == ccolon->GetLevel())
         {
            LOG_FMT(LBLANKD, "%s(%d): orig line is %zu, orig col is %zu, Text() '%s', type is %s\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), get_token_name(pc->GetType()));
            Chunk *paren_vor_value = pc->GetNextType(CT_FPAREN_OPEN, pc->GetLevel());

            if (  with_acv
               && paren_vor_value->IsNotNullChunk())
            {
               LOG_FMT(LBLANKD, "%s(%d): paren_vor_value orig line is %zu, orig col is %zu, Text() '%s', type is %s\n",
                       __func__, __LINE__, paren_vor_value->GetOrigLine(), paren_vor_value->GetOrigCol(),
                       paren_vor_value->Text(), get_token_name(paren_vor_value->GetType()));
               constructorValue.NewLines(paren_vor_value->GetNlCount());
               constructorValue.Add(paren_vor_value);
            }

            if (ncia & IARF_ADD)                   // nl_class_init_args, nl_constr_init_args:
            {
               if (pcc & TP_TRAIL)                 // pos_class_comma, pos_constr_comma
               {
                  if (ncia == IARF_FORCE)          // nl_class_init_args, nl_constr_init_args: 5
                  {
                     Chunk *after = pc->GetNext();   // Issue #2759

                     if (after->IsNot(CT_COMMENT_CPP))
                     {
                        newline_force_after(pc);
                     }
                  }
                  else
                  {
                     // (ncia == IARF_ADD)         // nl_class_init_args, nl_constr_init_args: 8
                     newline_add_after(pc);
                  }
                  prev = pc->GetPrevNc();

                  if (  prev->IsNewline()
                     && prev->SafeToDeleteNl())
                  {
                     Chunk::Delete(prev);
                     MARK_CHANGE();
                  }
               }
               else if (pcc & TP_LEAD)             // pos_class_comma, pos_constr_comma
               {
                  if (ncia == IARF_FORCE)          // nl_class_init_args, nl_constr_init_args: 7
                  {
                     newline_force_before(pc);
                  }
                  else
                  {
                     // (ncia == IARF_ADD)         // nl_class_init_args, nl_constr_init_args: 9
                     newline_add_before(pc);
                  }
                  next = pc->GetNextNc();

                  if (  next->IsNewline()
                     && next->SafeToDeleteNl())
                  {
                     Chunk::Delete(next);
                     MARK_CHANGE();
                  }
               }
            }
            else if (ncia == IARF_REMOVE)          // nl_class_init_args, nl_constr_init_args: 6
            {
               next = pc->GetNext();

               if (  next->IsNewline()
                  && next->SafeToDeleteNl())
               {
                  // comma is after
                  Chunk::Delete(next);
                  MARK_CHANGE();
               }
               else
               {
                  prev = pc->GetPrev();

                  if (  prev->IsNewline()
                     && prev->SafeToDeleteNl())
                  {
                     // comma is before
                     Chunk::Delete(prev);
                     MARK_CHANGE();
                  }
               }
            }
         }
      }
   }
} // newlines_class_colon_pos


static void blank_line_max(Chunk *pc, Option<unsigned> &opt)
{
   LOG_FUNC_ENTRY();

   if (pc == nullptr)
   {
      return;
   }
   const auto optval = opt();

   if (  (optval > 0)
      && (pc->GetNlCount() > optval))
   {
      LOG_FMT(LBLANKD, "%s(%d): do_blank_lines: %s max line %zu\n",
              __func__, __LINE__, opt.name(), pc->GetOrigLine());
      pc->SetNlCount(optval);
      MARK_CHANGE();
   }
} // blank_line_max


iarf_e newline_template_option(Chunk *pc, iarf_e special, iarf_e base, iarf_e fallback)
{
   Chunk *const prev = pc->GetPrevNcNnl();

   if (  prev->Is(CT_ANGLE_OPEN)
      && special != IARF_IGNORE)
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
} // newline_template_option


bool is_func_proto_group(Chunk *pc, E_Token one_liner_type)
{
   if (  pc != nullptr
      && options::nl_class_leave_one_liner_groups()
      && (  pc->Is(one_liner_type)
         || pc->GetParentType() == one_liner_type)
      && pc->TestFlags(PCF_IN_CLASS))
   {
      log_rule_B("nl_class_leave_one_liner_groups");

      if (pc->Is(CT_BRACE_CLOSE))
      {
         return(pc->TestFlags(PCF_ONE_LINER));
      }
      else
      {
         // Find opening brace
         pc = pc->GetNextType(CT_BRACE_OPEN, pc->GetLevel());
         return(  pc->IsNotNullChunk()
               && pc->TestFlags(PCF_ONE_LINER));
      }
   }
   return(false);
} // is_func_proto_group


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
         next->SetNlCount(max(pc->GetNlCount(), next->GetNlCount()));
         Chunk::Delete(pc);
         MARK_CHANGE();
      }
      pc = next;
   }
} // newlines_cleanup_dup


static void newlines_enum_entries(Chunk *open_brace, iarf_e av)
{
   LOG_FUNC_ENTRY();

   for (Chunk *pc = open_brace->GetNextNc();
        pc->IsNotNullChunk() && pc->GetLevel() > open_brace->GetLevel();
        pc = pc->GetNextNc())
   {
      if (  (pc->GetLevel() != (open_brace->GetLevel() + 1))
         || pc->IsNot(CT_COMMA)
         || (  pc->Is(CT_COMMA)
            && (  pc->GetNext()->GetType() == CT_COMMENT_CPP
               || pc->GetNext()->GetType() == CT_COMMENT
               || pc->GetNext()->GetType() == CT_COMMENT_MULTI)))
      {
         continue;
      }
      newline_iarf(pc, av);
   }

   newline_iarf(open_brace, av);
} // newlines_enum_entries


static void newlines_double_space_struct_enum_union(Chunk *open_brace)
{
   LOG_FUNC_ENTRY();

   Chunk *pc = Chunk::NullChunkPtr;

   if (open_brace != nullptr)
   {
      pc = open_brace;
   }

   while (  (pc = pc->GetNextNc())->IsNotNullChunk()
         && pc->GetLevel() > open_brace->GetLevel())
   {
      if (  pc->GetLevel() != (open_brace->GetLevel() + 1)
         || pc->IsNot(CT_NEWLINE))
      {
         continue;
      }
      /*
       * If the newline is NOT after a comment or a brace open and
       * it is before a comment, then make sure that the newline is
       * at least doubled
       */
      Chunk *prev = pc->GetPrev();

      if (  !prev->IsComment()
         && prev->IsNot(CT_BRACE_OPEN)
         && pc->GetNext()->IsComment())
      {
         if (pc->GetNlCount() < 2)
         {
            double_newline(pc);
         }
      }
   }
} // newlines_double_space_struct_enum_union


void annotations_newlines()
{
   LOG_FUNC_ENTRY();

   Chunk *next;
   Chunk *prev;
   Chunk *ae;   // last token of the annotation
   Chunk *pc = Chunk::GetHead();

   while (  (pc = pc->GetNextType(CT_ANNOTATION))->IsNotNullChunk()
         && (next = pc->GetNextNnl())->IsNotNullChunk())
   {
      // find the end of this annotation
      if (next->IsParenOpen())
      {
         // TODO: control newline between annotation and '(' ?
         ae = next->GetClosingParen();
      }
      else
      {
         ae = pc;
      }

      if (ae->IsNullChunk())
      {
         break;
      }
      LOG_FMT(LANNOT, "%s(%d): orig line is %zu, orig col is %zu, annotation is '%s',  end @ orig line %zu, orig col %zu, is '%s'\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(),
              ae->GetOrigLine(), ae->GetOrigCol(), ae->Text());

      prev = ae->GetPrev();             // Issue #1845
      LOG_FMT(LANNOT, "%s(%d): prev orig line is %zu, orig col is %zu, Text() is '%s'\n",
              __func__, __LINE__, prev->GetOrigLine(), prev->GetOrigCol(), prev->Text());
      next = ae->GetNextNnl();

      if (next->Is(CT_ANNOTATION))
      {
         LOG_FMT(LANNOT, "%s(%d):  -- nl_between_annotation\n",
                 __func__, __LINE__);
         newline_iarf(ae, options::nl_between_annotation());
         log_rule_B("nl_between_annotation");
      }

      if (next->Is(CT_NEWLINE))
      {
         if (next->Is(CT_ANNOTATION))
         {
            LOG_FMT(LANNOT, "%s(%d):  -- nl_after_annotation\n",
                    __func__, __LINE__);
            newline_iarf(ae, options::nl_after_annotation());
            log_rule_B("nl_after_annotation");
         }
      }
   }
} // annotations_newlines


bool newlines_between(Chunk *pc_start, Chunk *pc_end, size_t &newlines, E_Scope scope)
{
   if (  pc_start->IsNullChunk()
      || pc_end->IsNullChunk())
   {
      return(false);
   }
   newlines = 0;

   Chunk *it = pc_start;

   for ( ; it->IsNotNullChunk() && it != pc_end; it = it->GetNext(scope))
   {
      newlines += it->GetNlCount();
   }

   // newline count is valid if search stopped on expected chunk
   return(it == pc_end);
} // newlines_between
