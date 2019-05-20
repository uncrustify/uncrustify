/**
 * @file braces.cpp
 * Adds or removes braces.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "braces.h"

#include "chunk_list.h"
#include "combine.h"
#include "language_tools.h"
#include "newlines.h"
#include "prototypes.h"
#include "unc_ctype.h"
#include "uncrustify.h"
#include "uncrustify_types.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>


using namespace uncrustify;

using std::vector;


//! Converts a single brace into a virtual brace
static void convert_brace(chunk_t *br);


//! Converts a single virtual brace into a real brace
static void convert_vbrace(chunk_t *br);


static void convert_vbrace_to_brace(void);


//! Go backwards to honor brace newline removal limits
static void examine_braces(void);


/**
 * Step forward and count the number of semi colons at the current level.
 * Abort if more than 1 or if we enter a preprocessor
 */
static void examine_brace(chunk_t *bopen);


static void move_case_break(void);


static void mod_case_brace(void);


static void mod_full_brace_if_chain(void);


/**
 * Checks to see if the braces can be removed.
 *  - less than a certain length
 *  - doesn't mess up if/else stuff
 */
static bool can_remove_braces(chunk_t *bopen);


/**
 * Checks to see if the virtual braces should be converted to real braces.
 *  - over a certain length
 *
 * @param vbopen  Virtual Brace Open chunk
 *
 * @return true (convert to real braces) or false (leave alone)
 */
static bool should_add_braces(chunk_t *vbopen);


/**
 * Collect the text into txt that contains the full tag name.
 * Mainly for collecting namespace 'a.b.c' or function 'foo::bar()' names.
 */
static void append_tag_name(unc_text &txt, chunk_t *pc);


//! Remove the case brace, if allowable.
static chunk_t *mod_case_brace_remove(chunk_t *br_open);


//! Add the case brace, if allowable.
static chunk_t *mod_case_brace_add(chunk_t *cl_colon);


/**
 * Traverse the if chain and see if all can be removed
 *
 * @param br_start  chunk pointing to opening brace of if clause
 */
static void process_if_chain(chunk_t *br_start);


/**
 * Check if parenthesis pair that comes before a brace spans multiple lines
 *
 *
 * @param brace  the brace chunk whose predecessing parenthesis will be checked
 *
 * @pre   the brace chunk cannot be a nullptr,
 *        it needs to be of type CT_BRACE_OPEN or CT_BRACE_CLOSE,
 *        its parent type needs to be one of this types:
 *            CT_IF, CT_ELSEIF, CT_FOR, CT_USING_STMT, CT_WHILE,
 *            CT_FUNC_CLASS_DEF, CT_FUNC_DEF
 *
 * @return  false: if preconditions are not met,
 *                 if an error occurs while  counting the newline between the
 *                 parenthesis or
 *                 when no newlines are found between the parenthesis
 */
static bool paren_multiline_before_brace(chunk_t *brace)
{
   if (  brace == nullptr
      || (brace->type != CT_BRACE_OPEN && brace->type != CT_BRACE_CLOSE)
      || (  brace->parent_type != CT_IF
         && brace->parent_type != CT_ELSEIF
         && brace->parent_type != CT_FOR
         && brace->parent_type != CT_USING_STMT
         && brace->parent_type != CT_WHILE
         && brace->parent_type != CT_FUNC_CLASS_DEF
         && brace->parent_type != CT_FUNC_DEF))
   {
      return(false);
   }
   const auto paren_t = CT_SPAREN_CLOSE;

   // find parenthesis pair of the if/for/while/...
   auto paren_close = chunk_get_prev_type(brace, paren_t, brace->level, scope_e::ALL);
   auto paren_open  = chunk_skip_to_match_rev(paren_close, scope_e::ALL);

   if (  paren_close == nullptr
      || paren_open == nullptr
      || paren_close == brace
      || paren_open == paren_close)
   {
      return(false);
   }

   // determine number of lines in the parenthesis pair spans
   auto       nl_count = size_t {};
   const auto ret_flag = newlines_between(paren_open, paren_close, nl_count);
   if (!ret_flag)
   {
      LOG_FMT(LERR, "%s(%d): newlines_between error\n", __func__, __LINE__);
      return(false);
   }

   // nl_count = 0 -> 1 line
   return(nl_count > 0);
}


void do_braces(void)
{
   LOG_FUNC_ENTRY();
   if (  options::mod_full_brace_if_chain()
      || options::mod_full_brace_if_chain_only())
   {
      mod_full_brace_if_chain();
   }

   if ((options::mod_full_brace_if() |
        options::mod_full_brace_do() |
        options::mod_full_brace_for() |
        options::mod_full_brace_using() |
        options::mod_full_brace_while()) & IARF_REMOVE)
   {
      examine_braces();
   }

   // convert vbraces if needed
   if ((options::mod_full_brace_if() |
        options::mod_full_brace_do() |
        options::mod_full_brace_for() |
        options::mod_full_brace_function() |
        options::mod_full_brace_using() |
        options::mod_full_brace_while()) & IARF_ADD)
   {
      convert_vbrace_to_brace();
   }

   // Mark one-liners
   chunk_t *pc = chunk_get_head();
   while ((pc = chunk_get_next_ncnl(pc)) != nullptr)
   {
      if (pc->type != CT_BRACE_OPEN && pc->type != CT_VBRACE_OPEN)
      {
         continue;
      }
      chunk_t         *br_open = pc;
      const c_token_t brc_type = c_token_t(pc->type + 1); // corresponds to closing type
      // Detect empty bodies
      chunk_t         *tmp = chunk_get_next_ncnl(pc);
      if (chunk_is_token(tmp, brc_type))
      {
         chunk_flags_set(br_open, PCF_EMPTY_BODY);
         chunk_flags_set(tmp, PCF_EMPTY_BODY);
      }

      // Scan for the brace close or a newline
      tmp = br_open;
      while ((tmp = chunk_get_next_nc(tmp)) != nullptr)
      {
         if (chunk_is_newline(tmp))
         {
            break;
         }
         if (tmp->type == brc_type && br_open->level == tmp->level)
         {
            flag_series(br_open, tmp, PCF_ONE_LINER);
            break;
         }
      }
   }

   if (options::mod_case_brace() != IARF_IGNORE)
   {
      mod_case_brace();
   }
   if (options::mod_move_case_break())
   {
      move_case_break();
   }
} // do_braces


static void examine_braces(void)
{
   LOG_FUNC_ENTRY();

   const auto multiline_block = options::mod_full_brace_nl_block_rem_mlcond();

   for (auto pc = chunk_get_tail(); pc != nullptr;)
   {
      auto prev = chunk_get_prev_type(pc, CT_BRACE_OPEN, -1);

      if (  chunk_is_token(pc, CT_BRACE_OPEN)
         && ((pc->flags & PCF_IN_PREPROC) == 0)
         && (  (  (  pc->parent_type == CT_IF
                  || pc->parent_type == CT_ELSE
                  || pc->parent_type == CT_ELSEIF)
               && options::mod_full_brace_if() == IARF_REMOVE)
            || (  pc->parent_type == CT_DO
               && options::mod_full_brace_do() == IARF_REMOVE)
            || (  pc->parent_type == CT_FOR
               && options::mod_full_brace_for() == IARF_REMOVE)
            || (  pc->parent_type == CT_USING_STMT
               && options::mod_full_brace_using() == IARF_REMOVE)
            || (  pc->parent_type == CT_WHILE
               && options::mod_full_brace_while() == IARF_REMOVE)))
      {
         if (multiline_block && paren_multiline_before_brace(pc))
         {
            pc = prev;
            continue;
         }

         examine_brace(pc);
      }

      pc = prev;
   }
}


static bool should_add_braces(chunk_t *vbopen)
{
   LOG_FUNC_ENTRY();
   const size_t nl_max = options::mod_full_brace_nl();
   if (nl_max == 0)
   {
      return(false);
   }
   LOG_FMT(LBRDEL, "%s(%d): start on %zu : ",
           __func__, __LINE__, vbopen->orig_line);

   size_t  nl_count = 0;

   chunk_t *pc = nullptr;
   for (pc = chunk_get_next_nc(vbopen, scope_e::PREPROC);
        pc != nullptr && pc->level > vbopen->level;
        pc = chunk_get_next_nc(pc, scope_e::PREPROC))
   {
      if (chunk_is_newline(pc))
      {
         nl_count += pc->nl_count;
      }
   }

   if (  pc != nullptr
      && nl_count > nl_max
      && vbopen->pp_level == pc->pp_level)
   {
      LOG_FMT(LBRDEL, "%s(%d): exceeded %zu newlines\n",
              __func__, __LINE__, nl_max);
      return(true);
   }
   return(false);
}


static bool can_remove_braces(chunk_t *bopen)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LBRDEL, "%s(%d): start on line %zu:\n",
           __func__, __LINE__, bopen->orig_line);

   // Cannot remove braces inside a preprocessor
   if (bopen->flags & PCF_IN_PREPROC)
   {
      return(false);
   }
   chunk_t *pc = chunk_get_next_ncnl(bopen, scope_e::PREPROC);
   if (chunk_is_token(pc, CT_BRACE_CLOSE))
   {
      // Can't remove empty statement
      return(false);
   }

   const size_t level  = bopen->level + 1;
   const size_t nl_max = options::mod_full_brace_nl();
   chunk_t      *prev  = nullptr;

   size_t       semi_count = 0;
   bool         hit_semi   = false;
   size_t       nl_count   = 0;
   size_t       if_count   = 0;
   int          br_count   = 0;

   pc = chunk_get_next_nc(bopen, scope_e::ALL);
   LOG_FMT(LBRDEL, "%s(%d):  - begin with token '%s', orig_line is %zu, orig_col is %zu\n",
           __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
   while (pc != nullptr && pc->level >= level)
   {
      LOG_FMT(LBRDEL, "%s(%d): test token '%s', orig_line is %zu, orig_col is %zu\n",
              __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
      if (pc->flags & PCF_IN_PREPROC)
      {
         // Cannot remove braces that contain a preprocessor
         return(false);
      }

      if (chunk_is_newline(pc))
      {
         nl_count += pc->nl_count;
         if (nl_max > 0 && nl_count > nl_max)
         {
            LOG_FMT(LBRDEL, "%s(%d):  exceeded %zu newlines\n",
                    __func__, __LINE__, nl_max);
            return(false);
         }
      }
      else
      {
         if (chunk_is_token(pc, CT_BRACE_OPEN))
         {
            br_count++;
         }
         else if (chunk_is_token(pc, CT_BRACE_CLOSE))
         {
            br_count--;
            if (pc->level == level)
            {
               // mean a statement in a braces { stmt; }
               // as a statement with a semicolon { stmt; };
               ++semi_count;
               hit_semi = true;
            }
         }
         else if (  (chunk_is_token(pc, CT_IF) || chunk_is_token(pc, CT_ELSEIF))
                 && br_count == 0)
         {
            if_count++;
         }

         if (pc->level == level)
         {
            if (semi_count > 0 && hit_semi)
            {
               // should have bailed due to close brace level drop
               LOG_FMT(LBRDEL, "%s(%d):  no close brace\n", __func__, __LINE__);
               return(false);
            }

            LOG_FMT(LBRDEL, "%s(%d): text() '%s', orig_line is %zu, semi_count is %zu\n",
                    __func__, __LINE__, pc->text(), pc->orig_line, semi_count);

            if (chunk_is_token(pc, CT_ELSE))
            {
               LOG_FMT(LBRDEL, "%s(%d):  bailed on '%s' on line %zu\n",
                       __func__, __LINE__, pc->text(), pc->orig_line);
               return(false);
            }

            if (  chunk_is_semicolon(pc)
               || chunk_is_token(pc, CT_IF)
               || chunk_is_token(pc, CT_ELSEIF)
               || chunk_is_token(pc, CT_FOR)
               || chunk_is_token(pc, CT_DO)
               || chunk_is_token(pc, CT_WHILE)
               || chunk_is_token(pc, CT_USING_STMT)
               || (  chunk_is_token(pc, CT_BRACE_OPEN)
                  && chunk_is_token(prev, CT_FPAREN_CLOSE)))
            {
               hit_semi |= chunk_is_semicolon(pc);
               if (++semi_count > 1)
               {
                  LOG_FMT(LBRDEL, "%s(%d):  bailed on %zu because of '%s' on line %zu\n",
                          __func__, __LINE__, bopen->orig_line, pc->text(), pc->orig_line);
                  return(false);
               }
            }
         }
      }
      prev = pc;
      pc   = chunk_get_next_nc(pc);
   }

   if (pc == nullptr)
   {
      LOG_FMT(LBRDEL, "%s(%d):  pc is nullptr\n", __func__, __LINE__);
      return(false);
   }

   if (chunk_is_token(pc, CT_BRACE_CLOSE) && pc->parent_type == CT_IF)
   {
      chunk_t *next     = chunk_get_next_ncnl(pc, scope_e::PREPROC);
      chunk_t *tmp_prev = chunk_get_prev_ncnl(pc, scope_e::PREPROC);

      if (  chunk_is_token(next, CT_ELSE)
         && (chunk_is_token(tmp_prev, CT_BRACE_CLOSE) || chunk_is_token(tmp_prev, CT_VBRACE_CLOSE))
         && tmp_prev->parent_type == CT_IF)
      {
         LOG_FMT(LBRDEL, "%s(%d):  - bailed on '%s'[%s] on line %zu due to 'if' and 'else' sequence\n",
                 __func__, __LINE__, get_token_name(pc->type), get_token_name(pc->parent_type),
                 pc->orig_line);
         return(false);
      }
   }

   LOG_FMT(LBRDEL, "%s(%d):  - end on '%s' on line %zu. if_count is %zu semi_count is %zu\n",
           __func__, __LINE__, get_token_name(pc->type), pc->orig_line, if_count, semi_count);

   return(chunk_is_token(pc, CT_BRACE_CLOSE) && pc->pp_level == bopen->pp_level);
} // can_remove_braces


static void examine_brace(chunk_t *bopen)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LBRDEL, "%s(%d): start on orig_line %zu, bopen->level is %zu\n",
           __func__, __LINE__, bopen->orig_line, bopen->level);

   const size_t level  = bopen->level + 1;
   const size_t nl_max = options::mod_full_brace_nl();

   chunk_t      *prev      = nullptr;
   size_t       semi_count = 0;
   bool         hit_semi   = false;
   size_t       nl_count   = 0;
   size_t       if_count   = 0;
   int          br_count   = 0;

   chunk_t      *pc = chunk_get_next_nc(bopen);
   while (pc != nullptr && pc->level >= level)
   {
      if (chunk_is_token(pc, CT_NEWLINE))
      {
         LOG_FMT(LBRDEL, "%s(%d): orig_line is %zu, orig_col is %zu, <Newline>\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col);
      }
      else
      {
         LOG_FMT(LBRDEL, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s'\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
      }
      if (pc->flags & PCF_IN_PREPROC)
      {
         // Cannot remove braces that contain a preprocessor
         LOG_FMT(LBRDEL, "%s(%d):  PREPROC\n", __func__, __LINE__);
         return;
      }

      if (chunk_is_newline(pc))
      {
         nl_count += pc->nl_count;
         if (nl_max > 0 && nl_count > nl_max)
         {
            LOG_FMT(LBRDEL, "%s(%d):  exceeded %zu newlines\n",
                    __func__, __LINE__, nl_max);
            return;
         }
      }
      else
      {
         LOG_FMT(LBRDEL, "%s(%d): for pc->text() '%s', pc->level is %zu,  bopen->level is %zu\n",
                 __func__, __LINE__, pc->text(), pc->level, bopen->level);
         if (  chunk_is_token(pc, CT_BRACE_OPEN)
            && pc->level == bopen->level)
         {
            br_count++;
            LOG_FMT(LBRDEL, "%s(%d): br_count is now %d, pc->level is %zu,  bopen->level is %zu\n",
                    __func__, __LINE__, br_count, pc->level, bopen->level);
         }
         else if (  chunk_is_token(pc, CT_BRACE_CLOSE)
                 && pc->level == bopen->level)
         {
            br_count--;
            LOG_FMT(LBRDEL, "%s(%d): br_count is now %d, pc->level is %zu,  bopen->level is %zu\n",
                    __func__, __LINE__, br_count, pc->level, bopen->level);
            if (br_count == 0)
            {
               chunk_t *next = chunk_get_next_ncnl(pc, scope_e::PREPROC);
               if (next == nullptr || next->type != CT_BRACE_CLOSE)
               {
                  LOG_FMT(LBRDEL, "%s(%d):  junk after close brace\n", __func__, __LINE__);
                  return;
               }
            }
         }
         else if (  (chunk_is_token(pc, CT_IF) || chunk_is_token(pc, CT_ELSEIF))
                 && br_count == 0)
         {
            if_count++;
         }

         LOG_FMT(LBRDEL, "%s(%d): pc->level is %zu, level is %zu\n",
                 __func__, __LINE__, pc->level, level);
         if (pc->level == level)
         {
            if (semi_count > 0 && hit_semi)
            {
               // should have bailed due to close brace level drop
               LOG_FMT(LBRDEL, "%s(%d):  no close brace\n", __func__, __LINE__);
               return;
            }

            LOG_FMT(LBRDEL, "%s(%d): text() '%s', orig_line is %zu, semi_count is %zu\n",
                    __func__, __LINE__, pc->text(), pc->orig_line, semi_count);

            if (chunk_is_token(pc, CT_ELSE))
            {
               LOG_FMT(LBRDEL, "%s(%d):  bailed on '%s' on line %zu\n",
                       __func__, __LINE__, pc->text(), pc->orig_line);
               return;
            }

            if (prev != nullptr)
            {
               LOG_FMT(LBRDEL, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s', prev->text '%s', prev->type %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), prev->text(), get_token_name(prev->type));
            }
            else
            {
               LOG_FMT(LBRDEL, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s', prev is nullptr\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
            }
            LOG_FMT(LBRDEL, "%s(%d): for pc->text() '%s', pc->level is %zu,  bopen->level is %zu\n",
                    __func__, __LINE__, pc->text(), pc->level, bopen->level);
            if (  chunk_is_semicolon(pc)
               || chunk_is_token(pc, CT_IF)
               || chunk_is_token(pc, CT_ELSEIF)
               || chunk_is_token(pc, CT_FOR)
               || chunk_is_token(pc, CT_DO)
               || chunk_is_token(pc, CT_WHILE)
               || chunk_is_token(pc, CT_SWITCH)
               || chunk_is_token(pc, CT_USING_STMT)
               || (  chunk_is_token(pc, CT_BRACE_OPEN)
                  && pc->level == bopen->level)) // Issue #1758
            {
               LOG_FMT(LBRDEL, "%s(%d): pc->text() '%s', orig_line is %zu, orig_col is %zu, level is %zu\n",
                       __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col, pc->level);
               hit_semi |= chunk_is_semicolon(pc);
               semi_count++;
               LOG_FMT(LBRDEL, "%s(%d): semi_count is %zu\n",
                       __func__, __LINE__, semi_count);
               if (semi_count > 1)
               {
                  LOG_FMT(LBRDEL, "%s(%d):  bailed on %zu because of '%s' on line %zu\n",
                          __func__, __LINE__, bopen->orig_line, pc->text(), pc->orig_line);
                  return;
               }
            }
         }
      }
      prev = pc;
      pc   = chunk_get_next_nc(pc);
   }

   if (pc == nullptr)
   {
      LOG_FMT(LBRDEL, "%s(%d): pc is nullptr\n", __func__, __LINE__);
      return;
   }

   LOG_FMT(LBRDEL, "%s(%d):  - end on '%s' on line %zu. if_count is %zu, semi_count is %zu\n",
           __func__, __LINE__, get_token_name(pc->type), pc->orig_line, if_count, semi_count);

   if (chunk_is_token(pc, CT_BRACE_CLOSE))
   {
      chunk_t *next = chunk_get_next_ncnl(pc);
      if (next != nullptr)
      {
         while (chunk_is_token(next, CT_VBRACE_CLOSE))
         {
            next = chunk_get_next_ncnl(next);
         }

         if (next != nullptr)
         {
            LOG_FMT(LBRDEL, "%s(%d): orig_line is %zu, orig_col is %zu, next is '%s'\n",
                    __func__, __LINE__, next->orig_line, next->orig_col, get_token_name(next->type));
         }
         if (  if_count > 0
            && (chunk_is_token(next, CT_ELSE) || chunk_is_token(next, CT_ELSEIF)))
         {
            LOG_FMT(LBRDEL, "%s(%d):  bailed on because 'else' is next and %zu ifs\n",
                    __func__, __LINE__, if_count);
            return;
         }
      }

      LOG_FMT(LBRDEL, "%s(%d): semi_count is %zu\n",
              __func__, __LINE__, semi_count);
      if (semi_count > 0)
      {
         LOG_FMT(LBRDEL, "%s(%d): bopen->parent_type is %s\n",
                 __func__, __LINE__, get_token_name(bopen->parent_type));
         if (bopen->parent_type == CT_ELSE)
         {
            chunk_t *tmp_next = chunk_get_next_ncnl(bopen);
            if (chunk_is_token(tmp_next, CT_IF))
            {
               chunk_t *tmp_prev = chunk_get_prev_ncnl(bopen);
               LOG_FMT(LBRDEL, "%s(%d):  else-if removing braces on line %zu and %zu\n",
                       __func__, __LINE__, bopen->orig_line, pc->orig_line);

               chunk_del(bopen);
               chunk_del(pc);
               newline_del_between(tmp_prev, tmp_next);
               if (options::nl_else_if() & IARF_ADD)
               {
                  newline_add_between(tmp_prev, tmp_next);
               }
               return;
            }
         }

         // we have a pair of braces with only 1 statement inside
         LOG_FMT(LBRDEL, "%s(%d): we have a pair of braces with only 1 statement inside\n",
                 __func__, __LINE__);
         LOG_FMT(LBRDEL, "%s(%d): removing braces on line %zu and %zu\n",
                 __func__, __LINE__, bopen->orig_line, pc->orig_line);
         convert_brace(bopen);
         convert_brace(pc);
      }
      else
      {
         LOG_FMT(LBRDEL, "%s(%d):  empty statement\n", __func__, __LINE__);
      }
   }
   else
   {
      LOG_FMT(LBRDEL, "%s(%d):  not a close brace? - '%s'\n",
              __func__, __LINE__, pc->text());
   }
} // examine_brace


static void convert_brace(chunk_t *br)
{
   LOG_FUNC_ENTRY();
   if (  br == nullptr
      || (br->flags & PCF_KEEP_BRACE))
   {
      return;
   }

   chunk_t *tmp;
   if (chunk_is_token(br, CT_BRACE_OPEN))
   {
      set_chunk_type(br, CT_VBRACE_OPEN);
      br->str.clear();
      tmp = chunk_get_prev(br);
   }
   else if (chunk_is_token(br, CT_BRACE_CLOSE))
   {
      set_chunk_type(br, CT_VBRACE_CLOSE);
      br->str.clear();
      tmp = chunk_get_next(br);
   }
   else
   {
      return;
   }
   LOG_FMT(LGUY, "%s(%d): br->type is %s, br->parent_type is %s\n",
           __func__, __LINE__, get_token_name(br->type), get_token_name(br->parent_type));
   LOG_FMT(LGUY, "%s(%d): br->orig_line is %zu, br->orig_col is %zu\n",
           __func__, __LINE__, br->orig_line, br->orig_col);

   if (chunk_is_newline(tmp))
   {
      if (tmp->nl_count > 1)
      {
         tmp->nl_count--;
      }
      else
      {
         // Issue #2219
         // look for opening brace
         chunk_t *brace;
         if (chunk_is_token(br, CT_VBRACE_OPEN))
         {
            brace = tmp;
         }
         else if (chunk_is_token(br, CT_VBRACE_CLOSE))
         {
            brace = chunk_skip_to_match_rev(br);
            if (brace == nullptr)
            {
               brace = chunk_get_prev_type(br, CT_BRACE_OPEN, br->level);
            }
         }
         if (  chunk_is_token(br, CT_VBRACE_OPEN)
            || (  chunk_is_token(br, CT_VBRACE_CLOSE)
               && brace->orig_line < tmp->orig_line))
         {
            if (chunk_safe_to_del_nl(tmp))
            {
               chunk_del(tmp);
            }
         }
      }
   }
} // convert_brace


static void convert_vbrace(chunk_t *vbr)
{
   LOG_FUNC_ENTRY();
   if (vbr == nullptr)
   {
      return;
   }

   if (chunk_is_token(vbr, CT_VBRACE_OPEN))
   {
      set_chunk_type(vbr, CT_BRACE_OPEN);
      vbr->str = "{";

      /*
       * If the next chunk is a preprocessor, then move the open brace after the
       * preprocessor.
       */
      chunk_t *tmp = chunk_get_next(vbr);
      if (chunk_is_token(tmp, CT_PREPROC))
      {
         tmp = chunk_get_next(vbr, scope_e::PREPROC);
         chunk_move_after(vbr, tmp);
         newline_add_after(vbr);
      }
   }
   else if (chunk_is_token(vbr, CT_VBRACE_CLOSE))
   {
      set_chunk_type(vbr, CT_BRACE_CLOSE);
      vbr->str = "}";

      /*
       * If the next chunk is a comment, followed by a newline, then
       * move the brace after the newline and add another newline after
       * the close brace.
       */
      chunk_t *tmp = chunk_get_next(vbr);
      if (chunk_is_comment(tmp))
      {
         tmp = chunk_get_next(tmp);
         if (chunk_is_newline(tmp))
         {
            chunk_move_after(vbr, tmp);
            newline_add_after(vbr);
         }
      }
   }
}


static void convert_vbrace_to_brace(void)
{
   LOG_FUNC_ENTRY();

   // Find every vbrace open
   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next_ncnl(pc))
   {
      if (pc->type != CT_VBRACE_OPEN)
      {
         continue;
      }

      bool in_preproc = (pc->flags & PCF_IN_PREPROC);

      if (  (  (  pc->parent_type == CT_IF
               || pc->parent_type == CT_ELSE
               || pc->parent_type == CT_ELSEIF)
            && (options::mod_full_brace_if() & IARF_ADD)
            && !options::mod_full_brace_if_chain())
         || (  pc->parent_type == CT_FOR
            && (options::mod_full_brace_for() & IARF_ADD))
         || (  pc->parent_type == CT_DO
            && (options::mod_full_brace_do() & IARF_ADD))
         || (  pc->parent_type == CT_WHILE
            && (options::mod_full_brace_while() & IARF_ADD))
         || (  pc->parent_type == CT_USING_STMT
            && (options::mod_full_brace_using() & IARF_ADD))
         || (  pc->parent_type == CT_FUNC_DEF
            && (options::mod_full_brace_function() & IARF_ADD)))
      {
         // Find the matching vbrace close
         chunk_t *vbc = nullptr;
         chunk_t *tmp = pc;
         while ((tmp = chunk_get_next(tmp)) != nullptr)
         {
            if (in_preproc && ((tmp->flags & PCF_IN_PREPROC) == 0))
            {
               // Can't leave a preprocessor
               break;
            }
            if (  pc->brace_level == tmp->brace_level
               && chunk_is_token(tmp, CT_VBRACE_CLOSE)
               && pc->parent_type == tmp->parent_type
               && ((tmp->flags & PCF_IN_PREPROC) == (pc->flags & PCF_IN_PREPROC)))
            {
               vbc = tmp;
               break;
            }
         }
         if (vbc == nullptr)
         {
            continue;
         }

         // if we found a corresponding virtual closing brace
         convert_vbrace(pc);   // convert both the opening
         convert_vbrace(vbc);  // and closing brace
      }
   }
} // convert_vbrace_to_brace


chunk_t *insert_comment_after(chunk_t *ref, c_token_t cmt_type,
                              const unc_text &cmt_text)
{
   LOG_FUNC_ENTRY();

   chunk_t new_cmt = *ref;
   new_cmt.prev  = nullptr;
   new_cmt.next  = nullptr;
   new_cmt.flags = (ref->flags & PCF_COPY_FLAGS);
   new_cmt.type  = cmt_type;
   new_cmt.str.clear();
   if (cmt_type == CT_COMMENT_CPP)
   {
      new_cmt.str.append("// ");
      new_cmt.str.append(cmt_text);
   }
   else
   {
      if (chunk_is_token(ref, CT_PP_ELSE))
      {  // make test c/ 02501 stable
         new_cmt.str.append(" ");
      }
      new_cmt.str.append("/* ");
      new_cmt.str.append(cmt_text);
      new_cmt.str.append(" */");
   }
   // TODO: expand comment type to cover other comment styles?

   new_cmt.column   = ref->column + ref->len() + 1;
   new_cmt.orig_col = new_cmt.column;

   return(chunk_add_after(&new_cmt, ref));
}


static void append_tag_name(unc_text &txt, chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *cur = pc;

   // step backwards over all a::b stuff
   for (chunk_t *tmp = chunk_get_prev_ncnl(pc)
        ; (chunk_is_token(tmp, CT_DC_MEMBER) || chunk_is_token(tmp, CT_MEMBER))
        ; tmp = chunk_get_prev_ncnl(tmp))
   {
      tmp = chunk_get_prev_ncnl(tmp);
      cur = tmp;

      if (!chunk_is_word(tmp))
      {
         break;
      }
   }

   if (cur == nullptr)
   {
      return;
   }

   txt += cur->str;
   cur  = chunk_get_next_ncnl(cur);

   for (chunk_t *tmp = cur, *tmp_next = chunk_get_next_ncnl(cur)
        ; (chunk_is_token(tmp, CT_DC_MEMBER) || chunk_is_token(tmp, CT_MEMBER))
        ; tmp = chunk_get_next_ncnl(tmp), tmp_next = chunk_get_next_ncnl(tmp))
   {
      txt += tmp->str;

      if (tmp_next == nullptr)
      {
         break;
      }
      txt += tmp_next->str;
   }
} // append_tag_name


void add_long_closebrace_comment(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *fcn_pc = nullptr;
   chunk_t *sw_pc  = nullptr;
   chunk_t *ns_pc  = nullptr;
   chunk_t *cl_pc  = nullptr;

   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next_ncnl(pc))
   {
      if (chunk_is_token(pc, CT_FUNC_DEF) || chunk_is_token(pc, CT_OC_MSG_DECL))
      {
         fcn_pc = pc;
      }
      else if (chunk_is_token(pc, CT_SWITCH))
      {
         // pointless, since it always has the text "switch"
         sw_pc = pc;
      }
      else if (chunk_is_token(pc, CT_NAMESPACE))
      {
         ns_pc = pc;
      }
      else if (chunk_is_token(pc, CT_CLASS))
      {
         cl_pc = pc;
      }

      if (pc->type != CT_BRACE_OPEN || (pc->flags & PCF_IN_PREPROC))
      {
         continue;
      }

      chunk_t *br_open = pc;
      size_t  nl_count = 0;

      chunk_t *tmp = pc;
      while ((tmp = chunk_get_next(tmp, scope_e::PREPROC)) != nullptr)
      {
         if (chunk_is_newline(tmp))
         {
            nl_count += tmp->nl_count;
            continue;
         }
         // handle only matching closing braces, skip other chunks
         if (tmp->level != br_open->level || tmp->type != CT_BRACE_CLOSE)
         {
            continue;
         }
         chunk_t *br_close = tmp;

         tmp = chunk_get_next(tmp);

         // check for a possible end semicolon
         if (chunk_is_token(tmp, CT_SEMICOLON))
         {
            // set br_close to the semi token,
            // as br_close is used to add the coment after it
            br_close = tmp;
            tmp      = chunk_get_next(tmp);
         }
         // make sure a newline follows in order to not overwrite an already
         // existring comment
         if (tmp != nullptr && !chunk_is_newline(tmp))
         {
            break;
         }

         size_t   nl_min  = 0;
         chunk_t  *tag_pc = nullptr;
         unc_text xstr;

         if (  br_open->parent_type == CT_FUNC_DEF
            || br_open->parent_type == CT_OC_MSG_DECL)
         {
            nl_min = options::mod_add_long_function_closebrace_comment();
            tag_pc = fcn_pc;

            if (tag_pc != nullptr)
            {
               append_tag_name(xstr, tag_pc);
            }
         }
         else if (br_open->parent_type == CT_SWITCH && sw_pc != nullptr)
         {
            nl_min = options::mod_add_long_switch_closebrace_comment();
            tag_pc = sw_pc;
            xstr   = sw_pc->str;
         }
         else if (br_open->parent_type == CT_NAMESPACE && ns_pc != nullptr)
         {
            nl_min = options::mod_add_long_namespace_closebrace_comment();
            tag_pc = ns_pc;
            xstr   = tag_pc->str;    // add 'namespace' to the string

            // next chunk, normally is going to be the namespace name
            // append it with a space to generate "namespace xyz"
            chunk_t *tmp_next = chunk_get_next_ncnl(tag_pc);
            if (  tmp_next != nullptr
               && tmp_next->type != CT_BRACE_OPEN) // anonymous namespace -> ignore
            {
               xstr.append(" ");
               append_tag_name(xstr, tmp_next);
            }
         }
         else if (  br_open->parent_type == CT_CLASS
                 && cl_pc != nullptr
                 && (  !language_is_set(LANG_CPP)                 // proceed if not C++
                    || (chunk_is_token(br_close, CT_SEMICOLON)))) // else a C++ class needs to end with a semicolon
         {
            nl_min = options::mod_add_long_class_closebrace_comment();
            tag_pc = cl_pc;
            xstr   = tag_pc->str;

            chunk_t *tmp_next = chunk_get_next(cl_pc);
            if (tag_pc != nullptr)
            {
               xstr.append(" ");
               append_tag_name(xstr, tmp_next);
            }
         }

         if (  nl_min > 0
            && nl_count >= nl_min
            && tag_pc != nullptr)
         {
            // use the comment style that fits to the selected language
            const c_token_t style = language_is_set(LANG_CPP | LANG_CS)
                                    ? CT_COMMENT_CPP : CT_COMMENT;

            // Add a comment after the close brace
            insert_comment_after(br_close, style, xstr);
         }

         break;
      }
   }
} // add_long_closebrace_comment


static void move_case_break(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *prev = nullptr;

   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next_ncnl(pc))
   {
      if (  chunk_is_token(pc, CT_BREAK)
         && chunk_is_token(prev, CT_BRACE_CLOSE)
         && prev->parent_type == CT_CASE
         && chunk_is_newline(chunk_get_prev(pc))
         && chunk_is_newline(chunk_get_prev(prev)))
      {
         chunk_swap_lines(prev, pc);
      }
      prev = pc;
   }
}


static chunk_t *mod_case_brace_remove(chunk_t *br_open)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LMCB, "%s(%d): line %zu",
           __func__, __LINE__, br_open->orig_line);

   // Find the matching brace close
   chunk_t *next     = chunk_get_next_ncnl(br_open, scope_e::PREPROC);
   chunk_t *br_close = chunk_get_next_type(br_open, CT_BRACE_CLOSE, br_open->level, scope_e::PREPROC);
   if (br_close == nullptr)
   {
      LOG_FMT(LMCB, "%s(%d):  - no close\n", __func__, __LINE__);
      return(next);
   }

   // Make sure 'break', 'return', 'goto', 'case' or '}' is after the close brace
   chunk_t *pc = chunk_get_next_ncnl(br_close, scope_e::PREPROC);
   if (  pc == nullptr
      || (  pc->type != CT_BREAK
         && pc->type != CT_RETURN
         && pc->type != CT_CASE
         && pc->type != CT_GOTO
         && pc->type != CT_BRACE_CLOSE))
   {
      LOG_FMT(LMCB, "%s(%d):  - after '%s'\n",
              __func__, __LINE__, (pc == nullptr) ? "<null>" : get_token_name(pc->type));
      return(next);
   }

   // scan to make sure there are no definitions at brace level between braces
   for (chunk_t *tmp_pc = br_open;
        tmp_pc != br_close;
        tmp_pc = chunk_get_next_ncnl(tmp_pc, scope_e::PREPROC))
   {
      if (tmp_pc->level == (br_open->level + 1) && (tmp_pc->flags & PCF_VAR_DEF))
      {
         LOG_FMT(LMCB, "%s(%d):  - vardef on line %zu: '%s'\n",
                 __func__, __LINE__, tmp_pc->orig_line, pc->text());
         return(next);
      }
   }
   LOG_FMT(LMCB, "%s(%d):  - removing braces on lines %zu and %zu\n",
           __func__, __LINE__, br_open->orig_line, br_close->orig_line);

   for (chunk_t *tmp_pc = br_open;
        tmp_pc != br_close;
        tmp_pc = chunk_get_next_ncnl(tmp_pc, scope_e::PREPROC))
   {
      tmp_pc->brace_level--;
      tmp_pc->level--;
   }
   next = chunk_get_prev(br_open, scope_e::PREPROC);

   chunk_del(br_open);
   chunk_del(br_close);

   return(chunk_get_next(next, scope_e::PREPROC));
} // mod_case_brace_remove


static chunk_t *mod_case_brace_add(chunk_t *cl_colon)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LMCB, "%s(%d): line %zu",
           __func__, __LINE__, cl_colon->orig_line);

   chunk_t *pc   = cl_colon;
   chunk_t *last = nullptr;
   chunk_t *next = chunk_get_next_ncnl(cl_colon, scope_e::PREPROC);

   while ((pc = chunk_get_next_ncnl(pc, scope_e::PREPROC)) != nullptr)
   {
      if (pc->level < cl_colon->level)
      {
         LOG_FMT(LMCB, "%s(%d):  - level drop\n", __func__, __LINE__);
         return(next);
      }

      if (  pc->level == cl_colon->level
         && (chunk_is_token(pc, CT_CASE) || chunk_is_token(pc, CT_BREAK)))
      {
         last = pc;
         break;
      }
   }

   if (last == nullptr)
   {
      LOG_FMT(LMCB, "%s(%d):  - last is nullptr\n", __func__, __LINE__);
      return(next);
   }

   LOG_FMT(LMCB, "%s(%d):  - adding before '%s' on line %zu\n",
           __func__, __LINE__, last->text(), last->orig_line);

   chunk_t chunk;
   chunk.type        = CT_BRACE_OPEN;
   chunk.orig_line   = cl_colon->orig_line;
   chunk.orig_col    = cl_colon->orig_col;
   chunk.parent_type = CT_CASE;
   chunk.level       = cl_colon->level;
   chunk.brace_level = cl_colon->brace_level;
   chunk.flags       = pc->flags & PCF_COPY_FLAGS;
   chunk.str         = "{";

   chunk_t *br_open = chunk_add_after(&chunk, cl_colon);

   chunk.type      = CT_BRACE_CLOSE;
   chunk.orig_line = last->orig_line;
   chunk.orig_col  = last->orig_col;
   chunk.str       = "}";

   chunk_t *br_close = chunk_add_before(&chunk, last);
   newline_add_before(last);

   for (pc = chunk_get_next(br_open, scope_e::PREPROC);
        pc != br_close;
        pc = chunk_get_next(pc, scope_e::PREPROC))
   {
      pc->level++;
      pc->brace_level++;
   }

   return(br_open);
} // mod_case_brace_add


static void mod_case_brace(void)
{
   LOG_FUNC_ENTRY();

   chunk_t *pc = chunk_get_head();
   while (pc != nullptr)
   {
      chunk_t *next = chunk_get_next_ncnl(pc, scope_e::PREPROC);
      if (next == nullptr)
      {
         return;
      }

      if (  options::mod_case_brace() == IARF_REMOVE
         && chunk_is_token(pc, CT_BRACE_OPEN)
         && pc->parent_type == CT_CASE)
      {
         pc = mod_case_brace_remove(pc);
      }
      else if (  (options::mod_case_brace() & IARF_ADD)
              && chunk_is_token(pc, CT_CASE_COLON)
              && next->type != CT_BRACE_OPEN
              && next->type != CT_BRACE_CLOSE
              && next->type != CT_CASE)
      {
         pc = mod_case_brace_add(pc);
      }
      else
      {
         pc = chunk_get_next_ncnl(pc, scope_e::PREPROC);
      }
   }
}


static void process_if_chain(chunk_t *br_start)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LBRCH, "%s(%d): if starts on line %zu, orig_col is %zu.\n",
           __func__, __LINE__, br_start->orig_line, br_start->orig_col);

   vector<chunk_t *> braces;
   braces.reserve(16);

   bool    must_have_braces = false;

   chunk_t *pc = br_start;
   while (pc != nullptr)
   {
      LOG_FMT(LBRCH, "%s(%d): pc->text() is '%s', orig_line is %zu, orig_col is %zu.\n",
              __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
      if (chunk_is_token(pc, CT_BRACE_OPEN))
      {
         const bool tmp = can_remove_braces(pc);
         LOG_FMT(LBRCH, "%s(%d): braces.size() is %zu, line is %zu, - can%s remove %s\n",
                 __func__, __LINE__, braces.size(), pc->orig_line, tmp ? "" : "not",
                 get_token_name(pc->type));
         if (!tmp)
         {
            must_have_braces = true;
         }
      }
      else
      {
         const bool tmp = should_add_braces(pc);
         if (tmp)
         {
            must_have_braces = true;
         }
         LOG_FMT(LBRCH, "%s(%d): braces.size() is %zu, line is %zu, - %s %s\n",
                 __func__, __LINE__, braces.size(), pc->orig_line, tmp ? "should add" : "ignore",
                 get_token_name(pc->type));
      }

      braces.push_back(pc);
      chunk_t *br_close = chunk_skip_to_match(pc, scope_e::PREPROC);
      if (br_close == nullptr)
      {
         break;
      }
      braces.push_back(br_close);

      pc = chunk_get_next_ncnl(br_close, scope_e::PREPROC);
      if (pc == nullptr || pc->type != CT_ELSE)
      {
         break;
      }

      if (options::mod_full_brace_if_chain_only())
      {
         // There is an 'else' - we want full braces.
         must_have_braces = true;
      }

      pc = chunk_get_next_ncnl(pc, scope_e::PREPROC);
      if (chunk_is_token(pc, CT_ELSEIF))
      {
         while (  pc != nullptr
               && pc->type != CT_VBRACE_OPEN
               && pc->type != CT_BRACE_OPEN)
         {
            pc = chunk_get_next_ncnl(pc, scope_e::PREPROC);
         }
      }
      if (pc == nullptr)
      {
         break;
      }
      if (pc->type != CT_BRACE_OPEN && pc->type != CT_VBRACE_OPEN)
      {
         break;
      }
   }

   if (must_have_braces)
   {
      LOG_FMT(LBRCH, "%s(%d): add braces on lines[%zu]:",
              __func__, __LINE__, braces.size());

      const auto ite = braces.rend();
      for (auto itc = braces.rbegin(); itc != ite; ++itc)
      {
         const auto brace = *itc;

         chunk_flags_set(brace, PCF_KEEP_BRACE);
         if (chunk_is_token(brace, CT_VBRACE_OPEN) || chunk_is_token(brace, CT_VBRACE_CLOSE))
         {
            LOG_FMT(LBRCH, "%s(%d):  %zu",
                    __func__, __LINE__, brace->orig_line);
            convert_vbrace(brace);
         }
         else
         {
            LOG_FMT(LBRCH, "%s(%d):  {%zu}",
                    __func__, __LINE__, brace->orig_line);
         }
      }
      LOG_FMT(LBRCH, "\n");
   }
   else if (options::mod_full_brace_if_chain())
   {
      LOG_FMT(LBRCH, "%s(%d): remove braces on lines[%zu]:\n",
              __func__, __LINE__, braces.size());

      /*
       * This might run because either
       * mod_full_brace_if_chain or mod_full_brace_if_chain_only
       * is used.
       * We only want to remove braces if the first one is active.
       */
      const auto multiline_block = options::mod_full_brace_nl_block_rem_mlcond();

      LOG_FMT(LBRCH, "%s(%d): remove braces on lines:\n", __func__, __LINE__);

      // Issue #2229
      const auto ite = braces.end();
      for (auto itc = braces.begin(); itc != ite; ++itc)
      {
         const auto brace = *itc;

         if (  (chunk_is_token(brace, CT_BRACE_OPEN) || chunk_is_token(brace, CT_BRACE_CLOSE))
            && (multiline_block ? !paren_multiline_before_brace(brace) : true))
         {
            LOG_FMT(LBRCH, "%s(%d): brace->orig_line is %zu, brace->orig_col is %zu\n",
                    __func__, __LINE__, brace->orig_line, brace->orig_col);
            convert_brace(brace);
         }
         else
         {
            LOG_FMT(LBRCH, "%s(%d): brace->orig_line is %zu, brace->orig_col is %zu\n",
                    __func__, __LINE__, brace->orig_line, brace->orig_col);
         }
      }
   }
} // process_if_chain


static void mod_full_brace_if_chain(void)
{
   LOG_FUNC_ENTRY();

   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next(pc))
   {
      if (  (chunk_is_token(pc, CT_BRACE_OPEN) || chunk_is_token(pc, CT_VBRACE_OPEN))
         && pc->parent_type == CT_IF)
      {
         process_if_chain(pc);
      }
   }
}
