/**
 * @file braces.cpp
 * Adds or removes braces.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "braces.h"
#include "uncrustify_types.h"
#include "chunk_list.h"
#include "prototypes.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "unc_ctype.h"
#include "uncrustify.h"
#include "combine.h"
#include "newlines.h"
#include "chunk_list.h"


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
      LOG_FMT(LERR, "newlines_between error\n");
      return(false);
   }

   // nl_count = 0 -> 1 line
   return(nl_count > 0);
}


void do_braces(void)
{
   LOG_FUNC_ENTRY();
   if (  cpd.settings[UO_mod_full_brace_if_chain].b
      || cpd.settings[UO_mod_full_brace_if_chain_only].b)
   {
      mod_full_brace_if_chain();
   }

   if ((cpd.settings[UO_mod_full_brace_if].a |
        cpd.settings[UO_mod_full_brace_do].a |
        cpd.settings[UO_mod_full_brace_for].a |
        cpd.settings[UO_mod_full_brace_using].a |
        cpd.settings[UO_mod_full_brace_while].a) & AV_REMOVE)
   {
      examine_braces();
   }

   // convert vbraces if needed
   if ((cpd.settings[UO_mod_full_brace_if].a |
        cpd.settings[UO_mod_full_brace_do].a |
        cpd.settings[UO_mod_full_brace_for].a |
        cpd.settings[UO_mod_full_brace_function].a |
        cpd.settings[UO_mod_full_brace_using].a |
        cpd.settings[UO_mod_full_brace_while].a) & AV_ADD)
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
      chunk_t   *br_open = pc;
      c_token_t brc_type = c_token_t(pc->type + 1); // corresponds to closing type
      // Detect empty bodies
      chunk_t   *tmp = chunk_get_next_ncnl(pc);
      if (tmp != nullptr && tmp->type == brc_type)
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

   if (cpd.settings[UO_mod_case_brace].a != AV_IGNORE)
   {
      mod_case_brace();
   }
   if (cpd.settings[UO_mod_move_case_break].b)
   {
      move_case_break();
   }
} // do_braces


static void examine_braces(void)
{
   LOG_FUNC_ENTRY();

   const auto multiline_block = cpd.settings[UO_mod_full_brace_nl_block_rem_mlcond].b;

   for (auto pc = chunk_get_tail(); pc != nullptr;)
   {
      auto prev = chunk_get_prev_type(pc, CT_BRACE_OPEN, -1);

      if (  pc->type == CT_BRACE_OPEN
         && ((pc->flags & PCF_IN_PREPROC) == 0)
         && (  (  (  pc->parent_type == CT_IF
                  || pc->parent_type == CT_ELSE
                  || pc->parent_type == CT_ELSEIF)
               && cpd.settings[UO_mod_full_brace_if].a == AV_REMOVE)
            || (  pc->parent_type == CT_DO
               && cpd.settings[UO_mod_full_brace_do].a == AV_REMOVE)
            || (  pc->parent_type == CT_FOR
               && cpd.settings[UO_mod_full_brace_for].a == AV_REMOVE)
            || (  pc->parent_type == CT_USING_STMT
               && cpd.settings[UO_mod_full_brace_using].a == AV_REMOVE)
            || (  pc->parent_type == CT_WHILE
               && cpd.settings[UO_mod_full_brace_while].a == AV_REMOVE)))
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
   size_t nl_max = cpd.settings[UO_mod_full_brace_nl].u;
   if (nl_max == 0)
   {
      return(false);
   }

   LOG_FMT(LBRDEL, "%s: start on %zu : ", __func__, vbopen->orig_line);
   chunk_t *pc;
   size_t  nl_count = 0;

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
      LOG_FMT(LBRDEL, " exceeded %zu newlines\n", nl_max);
      return(true);
   }
   return(false);
}


static bool can_remove_braces(chunk_t *bopen)
{
   LOG_FUNC_ENTRY();

   chunk_t *prev      = nullptr;
   size_t  semi_count = 0;
   size_t  level      = bopen->level + 1;
   bool    hit_semi   = false;
   bool    was_fcn    = false;
   size_t  nl_max     = cpd.settings[UO_mod_full_brace_nl].u;
   size_t  nl_count   = 0;
   size_t  if_count   = 0;
   int     br_count   = 0;

   // Cannot remove braces inside a preprocessor
   if (bopen->flags & PCF_IN_PREPROC)
   {
      return(false);
   }
   chunk_t *pc = chunk_get_next_ncnl(bopen, scope_e::PREPROC);
   if (pc != nullptr && pc->type == CT_BRACE_CLOSE)
   {
      // Can't remove empty statement
      return(false);
   }

   LOG_FMT(LBRDEL, "%s: start on %zu : ", __func__, bopen->orig_line);

   pc = chunk_get_next_nc(bopen, scope_e::ALL);
   while (pc != nullptr && pc->level >= level)
   {
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
            LOG_FMT(LBRDEL, " exceeded %zu newlines\n", nl_max);
            return(false);
         }
      }
      else
      {
         if (pc->type == CT_BRACE_OPEN)
         {
            br_count++;
         }
         else if (pc->type == CT_BRACE_CLOSE)
         {
            br_count--;
         }
         else if (pc->type == CT_IF || pc->type == CT_ELSEIF)
         {
            if (br_count == 0)
            {
               if_count++;
            }
         }

         if (pc->level == level)
         {
            if (semi_count > 0 && hit_semi)
            {
               // should have bailed due to close brace level drop
               LOG_FMT(LBRDEL, " no close brace\n");
               return(false);
            }

            LOG_FMT(LBRDEL, " [%s %zu-%zu]", pc->text(), pc->orig_line, semi_count);

            if (pc->type == CT_ELSE)
            {
               LOG_FMT(LBRDEL, " bailed on %s on line %zu\n",
                       pc->text(), pc->orig_line);
               return(false);
            }

            was_fcn = (prev != nullptr) && prev->type == CT_FPAREN_CLOSE;

            if (  chunk_is_semicolon(pc)
               || pc->type == CT_IF
               || pc->type == CT_ELSEIF
               || pc->type == CT_FOR
               || pc->type == CT_DO
               || pc->type == CT_WHILE
               || pc->type == CT_USING_STMT
               || (pc->type == CT_BRACE_OPEN && was_fcn))
            {
               hit_semi |= chunk_is_semicolon(pc);
               if (++semi_count > 1)
               {
                  LOG_FMT(LBRDEL, " bailed on %zu because of %s on line %zu\n",
                          bopen->orig_line, pc->text(), pc->orig_line);
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
      LOG_FMT(LBRDEL, " NULL\n");
      return(false);
   }

   if (pc->type == CT_BRACE_CLOSE && pc->parent_type == CT_IF)
   {
      chunk_t *next = chunk_get_next_ncnl(pc, scope_e::PREPROC);

      prev = chunk_get_prev_ncnl(pc, scope_e::PREPROC);

      if (  next != nullptr
         && next->type == CT_ELSE
         && (prev->type == CT_BRACE_CLOSE || prev->type == CT_VBRACE_CLOSE)
         && prev->parent_type == CT_IF)
      {
         LOG_FMT(LBRDEL, " - bailed on '%s'[%s] on line %zu due to 'if' and 'else' sequence\n",
                 get_token_name(pc->type), get_token_name(pc->parent_type),
                 pc->orig_line);
         return(false);
      }
   }

   LOG_FMT(LBRDEL, " - end on '%s' on line %zu. if_count=%zu semi_count=%zu\n",
           get_token_name(pc->type), pc->orig_line, if_count, semi_count);

   return(pc->type == CT_BRACE_CLOSE && pc->pp_level == bopen->pp_level);
} // can_remove_braces


static void examine_brace(chunk_t *bopen)
{
   LOG_FUNC_ENTRY();
   chunk_t *next;
   chunk_t *prev      = nullptr;
   size_t  semi_count = 0;
   size_t  level      = bopen->level + 1;
   bool    hit_semi   = false;
   bool    was_fcn    = false;
   size_t  nl_max     = cpd.settings[UO_mod_full_brace_nl].u;
   size_t  nl_count   = 0;
   size_t  if_count   = 0;
   int     br_count   = 0;

   LOG_FMT(LBRDEL, "%s: start on %zu : ", __func__, bopen->orig_line);

   chunk_t *pc = chunk_get_next_nc(bopen);
   while (pc != nullptr && pc->level >= level)
   {
      if (pc->flags & PCF_IN_PREPROC)
      {
         // Cannot remove braces that contain a preprocessor
         LOG_FMT(LBRDEL, " PREPROC\n");
         return;
      }

      if (chunk_is_newline(pc))
      {
         nl_count += pc->nl_count;
         if (nl_max > 0 && nl_count > nl_max)
         {
            LOG_FMT(LBRDEL, " exceeded %zu newlines\n", nl_max);
            return;
         }
      }
      else
      {
         if (pc->type == CT_BRACE_OPEN)
         {
            br_count++;
         }
         else if (pc->type == CT_BRACE_CLOSE)
         {
            br_count--;
            if (br_count == 0)
            {
               next = chunk_get_next_ncnl(pc, scope_e::PREPROC);
               if (next == nullptr || next->type != CT_BRACE_CLOSE)
               {
                  LOG_FMT(LBRDEL, " junk after close brace\n");
                  return;
               }
            }
         }
         else if (pc->type == CT_IF || pc->type == CT_ELSEIF)
         {
            if (br_count == 0)
            {
               if_count++;
            }
         }

         if (pc->level == level)
         {
            if (semi_count > 0 && hit_semi)
            {
               // should have bailed due to close brace level drop
               LOG_FMT(LBRDEL, " no close brace\n");
               return;
            }

            LOG_FMT(LBRDEL, " [%s %zu-%zu]", pc->text(), pc->orig_line, semi_count);

            if (pc->type == CT_ELSE)
            {
               LOG_FMT(LBRDEL, " bailed on %s on line %zu\n",
                       pc->text(), pc->orig_line);
               return;
            }

            was_fcn = (prev != nullptr) && prev->type == CT_FPAREN_CLOSE;

            if (  chunk_is_semicolon(pc)
               || pc->type == CT_IF
               || pc->type == CT_ELSEIF
               || pc->type == CT_FOR
               || pc->type == CT_DO
               || pc->type == CT_WHILE
               || pc->type == CT_SWITCH
               || pc->type == CT_USING_STMT
               || (pc->type == CT_BRACE_OPEN && was_fcn))
            {
               hit_semi |= chunk_is_semicolon(pc);
               if (++semi_count > 1)
               {
                  LOG_FMT(LBRDEL, " bailed on %zu because of %s on line %zu\n",
                          bopen->orig_line, pc->text(), pc->orig_line);
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
      LOG_FMT(LBRDEL, " NULL\n");
      return;
   }

   LOG_FMT(LBRDEL, " - end on '%s' on line %zu. if_count=%zu semi_count=%zu\n",
           get_token_name(pc->type), pc->orig_line, if_count, semi_count);

   if (pc->type == CT_BRACE_CLOSE)
   {
      next = chunk_get_next_ncnl(pc);
      if (next != nullptr)
      {
         while (next != nullptr && next->type == CT_VBRACE_CLOSE)
         {
            next = chunk_get_next_ncnl(next);
         }

         if (next != nullptr)
         {
            LOG_FMT(LBRDEL, " next is '%s'\n", get_token_name(next->type));
         }
         if (  if_count > 0
            && next != nullptr
            && (next->type == CT_ELSE || next->type == CT_ELSEIF))
         {
            LOG_FMT(LBRDEL, " bailed on because 'else' is next and %zu ifs\n", if_count);
            return;
         }
      }

      if (semi_count > 0)
      {
         if (bopen->parent_type == CT_ELSE)
         {
            next = chunk_get_next_ncnl(bopen);
            if (next->type == CT_IF)
            {
               prev = chunk_get_prev_ncnl(bopen);
               LOG_FMT(LBRDEL, " else-if removing braces on line %zu and %zu\n",
                       bopen->orig_line, pc->orig_line);

               chunk_del(bopen);
               chunk_del(pc);
               newline_del_between(prev, next);
               if (cpd.settings[UO_nl_else_if].a & AV_ADD)
               {
                  newline_add_between(prev, next);
               }
               return;
            }
         }

         // we have a pair of braces with only 1 statement inside
         convert_brace(bopen);
         convert_brace(pc);

         LOG_FMT(LBRDEL, " removing braces on line %zu and %zu\n",
                 bopen->orig_line, pc->orig_line);
      }
      else
      {
         LOG_FMT(LBRDEL, " empty statement\n");
      }
   }
   else
   {
      LOG_FMT(LBRDEL, " not a close brace? - '%s'\n", pc->text());
   }
} // examine_brace


static void convert_brace(chunk_t *br)
{
   LOG_FUNC_ENTRY();
   chunk_t *tmp;

   if (!br || (br->flags & PCF_KEEP_BRACE))
   {
      return;
   }

   if (br->type == CT_BRACE_OPEN)
   {
      set_chunk_type(br, CT_VBRACE_OPEN);
      br->str.clear();
      tmp = chunk_get_prev(br);
   }
   else if (br->type == CT_BRACE_CLOSE)
   {
      set_chunk_type(br, CT_VBRACE_CLOSE);
      br->str.clear();
      tmp = chunk_get_next(br);
   }
   else
   {
      return;
   }

   if (chunk_is_newline(tmp))
   {
      if (tmp->nl_count > 1)
      {
         tmp->nl_count--;
      }
      else
      {
         if (chunk_safe_to_del_nl(tmp))
         {
            chunk_del(tmp);
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

   if (vbr->type == CT_VBRACE_OPEN)
   {
      set_chunk_type(vbr, CT_BRACE_OPEN);
      vbr->str = "{";

      /*
       * If the next chunk is a preprocessor, then move the open brace after the
       * preprocessor.
       */
      chunk_t *tmp = chunk_get_next(vbr);
      if (tmp != nullptr && tmp->type == CT_PREPROC)
      {
         tmp = chunk_get_next(vbr, scope_e::PREPROC);
         chunk_move_after(vbr, tmp);
         newline_add_after(vbr);
      }
   }
   else if (vbr->type == CT_VBRACE_CLOSE)
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
            && (cpd.settings[UO_mod_full_brace_if].a & AV_ADD)
            && !cpd.settings[UO_mod_full_brace_if_chain].b)
         || (  pc->parent_type == CT_FOR
            && (cpd.settings[UO_mod_full_brace_for].a & AV_ADD))
         || (  pc->parent_type == CT_DO
            && (cpd.settings[UO_mod_full_brace_do].a & AV_ADD))
         || (  pc->parent_type == CT_WHILE
            && (cpd.settings[UO_mod_full_brace_while].a & AV_ADD))
         || (  pc->parent_type == CT_USING_STMT
            && (cpd.settings[UO_mod_full_brace_using].a & AV_ADD))
         || (  pc->parent_type == CT_FUNC_DEF
            && (cpd.settings[UO_mod_full_brace_function].a & AV_ADD)))
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
               && tmp->type == CT_VBRACE_CLOSE
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
      if (ref->type == CT_PP_ELSE)
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
   chunk_t *tmp = pc;

   // step backwards over all a::b stuff
   while ((tmp = chunk_get_prev_ncnl(tmp)) != nullptr)
   {
      if (tmp->type != CT_DC_MEMBER && tmp->type != CT_MEMBER)
      {
         break;
      }
      tmp = chunk_get_prev_ncnl(tmp);
      pc  = tmp;
      if (!chunk_is_word(tmp))
      {
         break;
      }
   }

   txt += pc->str;
   while ((pc = chunk_get_next_ncnl(pc)) != nullptr)
   {
      if (pc->type != CT_DC_MEMBER && pc->type != CT_MEMBER)
      {
         break;
      }
      txt += pc->str;
      pc   = chunk_get_next_ncnl(pc);
      if (pc)
      {
         txt += pc->str;
      }
   }
}


void add_long_closebrace_comment(void)
{
   LOG_FUNC_ENTRY();
   chunk_t  *br_close;
   chunk_t  *fcn_pc     = nullptr;
   chunk_t  *sw_pc      = nullptr;
   chunk_t  *ns_pc      = nullptr;
   chunk_t  *cl_pc      = nullptr;
   chunk_t  *cl_semi_pc = nullptr;
   unc_text xstr;

   for (chunk_t *pc = chunk_get_head(); pc; pc = chunk_get_next_ncnl(pc))
   {
      if (pc->type == CT_FUNC_DEF || pc->type == CT_OC_MSG_DECL)
      {
         fcn_pc = pc;
      }
      else if (pc->type == CT_SWITCH)
      {
         // pointless, since it always has the text "switch"
         sw_pc = pc;
      }
      else if (pc->type == CT_NAMESPACE)
      {
         ns_pc = pc;
      }
      else if (pc->type == CT_CLASS)
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
      while ((tmp = chunk_get_next(tmp)) != nullptr)
      {
         if (chunk_is_newline(tmp))
         {
            nl_count += tmp->nl_count;
         }
         else if (tmp->level == br_open->level && tmp->type == CT_BRACE_CLOSE)
         {
            br_close = tmp;

            //LOG_FMT(LSYS, "found brace pair on lines %d and %d, nl_count=%d\n",
            //        br_open->orig_line, br_close->orig_line, nl_count);

            // Found the matching close brace - make sure a newline is next
            tmp = chunk_get_next(tmp);

            // Check for end of class
            if (  tmp != nullptr
               && tmp->parent_type == CT_CLASS
               && tmp->type == CT_SEMICOLON)
            {
               cl_semi_pc = tmp;
               tmp        = chunk_get_next(tmp);
               if (tmp != nullptr && !chunk_is_newline(tmp))
               {
                  tmp        = cl_semi_pc;
                  cl_semi_pc = nullptr;
               }
            }
            if (tmp == nullptr || chunk_is_newline(tmp))
            {
               size_t  nl_min  = 0;
               chunk_t *tag_pc = nullptr;

               if (br_open->parent_type == CT_SWITCH)
               {
                  nl_min = cpd.settings[UO_mod_add_long_switch_closebrace_comment].u;
                  tag_pc = sw_pc;
                  xstr   = sw_pc ? sw_pc->str : nullptr; // TODO NULL is no unc_text structure
               }
               else if (  br_open->parent_type == CT_FUNC_DEF
                       || br_open->parent_type == CT_OC_MSG_DECL)
               {
                  nl_min = cpd.settings[UO_mod_add_long_function_closebrace_comment].u;
                  // 76006 Explicit null dereferenced, 2016-03-17
                  tag_pc = fcn_pc;
                  xstr.clear();
                  append_tag_name(xstr, tag_pc);
               }
               else if (br_open->parent_type == CT_NAMESPACE)
               {
                  nl_min = cpd.settings[UO_mod_add_long_namespace_closebrace_comment].u;
                  // 76007 Explicit null dereferenced, 2016-03-17
                  tag_pc = ns_pc;

                  /*
                   * obtain the next chunk, normally this is the name of the namespace
                   * and append it to generate "namespace xyz"
                   */
                  xstr = ns_pc->str;
                  xstr.append(" ");
                  append_tag_name(xstr, chunk_get_next(ns_pc));
               }
               else if (  br_open->parent_type == CT_CLASS
                       && cl_semi_pc
                       && cl_pc)
               {
                  nl_min = cpd.settings[UO_mod_add_long_class_closebrace_comment].u;
                  tag_pc = cl_pc;
                  xstr   = tag_pc->str;
                  xstr.append(" ");
                  append_tag_name(xstr, chunk_get_next(cl_pc));
                  br_close   = cl_semi_pc;
                  cl_semi_pc = nullptr;
                  cl_pc      = nullptr;
               }

               if (  nl_min > 0
                  && nl_count >= nl_min
                  && tag_pc != nullptr)
               {
                  // use the comment style that fits to the selected language
                  c_token_t style = (cpd.lang_flags & (LANG_CPP | LANG_CS)) ?
                                    CT_COMMENT_CPP : CT_COMMENT;

                  // Add a comment after the close brace
                  insert_comment_after(br_close, style, xstr);
               }
            }
            break;
         }
      }
   }
} // add_long_closebrace_comment


static void move_case_break(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *prev = nullptr;

   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next_ncnl(pc))
   {
      if (  pc->type == CT_BREAK
         && prev != nullptr
         && prev->type == CT_BRACE_CLOSE
         && prev->parent_type == CT_CASE)
      {
         if (  chunk_is_newline(chunk_get_prev(pc))
            && chunk_is_newline(chunk_get_prev(prev)))
         {
            chunk_swap_lines(prev, pc);
         }
      }
      prev = pc;
   }
}


static chunk_t *mod_case_brace_remove(chunk_t *br_open)
{
   LOG_FUNC_ENTRY();
   chunk_t *next = chunk_get_next_ncnl(br_open, scope_e::PREPROC);

   LOG_FMT(LMCB, "%s: line %zu", __func__, br_open->orig_line);

   // Find the matching brace close
   chunk_t *br_close = chunk_get_next_type(br_open, CT_BRACE_CLOSE, br_open->level, scope_e::PREPROC);
   if (br_close == nullptr)
   {
      LOG_FMT(LMCB, " - no close\n");
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
      LOG_FMT(LMCB, " - after '%s'\n",
              (pc == NULL) ? "<null>" : get_token_name(pc->type));
      return(next);
   }

   // scan to make sure there are no definitions at brace level between braces
   for (pc = br_open; pc != br_close; pc = chunk_get_next_ncnl(pc, scope_e::PREPROC))
   {
      if ((pc->level == (br_open->level + 1)) && (pc->flags & PCF_VAR_DEF))
      {
         LOG_FMT(LMCB, " - vardef on line %zu: '%s'\n", pc->orig_line, pc->text());
         return(next);
      }
   }
   LOG_FMT(LMCB, " - removing braces on lines %zu and %zu\n",
           br_open->orig_line, br_close->orig_line);

   for (pc = br_open; pc != br_close; pc = chunk_get_next_ncnl(pc, scope_e::PREPROC))
   {
      pc->brace_level--;
      pc->level--;
   }
   next = chunk_get_prev(br_open, scope_e::PREPROC);
   chunk_del(br_open);
   chunk_del(br_close);
   return(chunk_get_next(next, scope_e::PREPROC));
} // mod_case_brace_remove


static chunk_t *mod_case_brace_add(chunk_t *cl_colon)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc   = cl_colon;
   chunk_t *last = nullptr;
   chunk_t *next = chunk_get_next_ncnl(cl_colon, scope_e::PREPROC);

   LOG_FMT(LMCB, "%s: line %zu", __func__, pc->orig_line);

   while ((pc = chunk_get_next_ncnl(pc, scope_e::PREPROC)) != nullptr)
   {
      if (pc->level < cl_colon->level)
      {
         LOG_FMT(LMCB, " - level drop\n");
         return(next);
      }

      if (  pc->level == cl_colon->level
         && (pc->type == CT_CASE || pc->type == CT_BREAK))
      {
         last = pc;
         //if (pc->type == CT_BREAK)
         //{
         //   // Step past the semicolon
         //   last = chunk_get_next_ncnl(chunk_get_next_ncnl(last));
         //}
         break;
      }
   }

   if (last == nullptr)
   {
      LOG_FMT(LMCB, " - NULL last\n");
      return(next);
   }

   LOG_FMT(LMCB, " - adding before '%s' on line %zu\n", last->text(), last->orig_line);

   chunk_t chunk;
   chunk.type        = CT_BRACE_OPEN;
   chunk.orig_line   = cl_colon->orig_line;
   chunk.parent_type = CT_CASE;
   chunk.level       = cl_colon->level;
   chunk.brace_level = cl_colon->brace_level;
   chunk.flags       = pc->flags & PCF_COPY_FLAGS;
   chunk.str         = "{";

   chunk_t *br_open = chunk_add_after(&chunk, cl_colon);

   chunk.type      = CT_BRACE_CLOSE;
   chunk.orig_line = last->orig_line;
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

      if (  cpd.settings[UO_mod_case_brace].a == AV_REMOVE
         && pc->type == CT_BRACE_OPEN
         && pc->parent_type == CT_CASE)
      {
         pc = mod_case_brace_remove(pc);
      }
      else if (  (cpd.settings[UO_mod_case_brace].a & AV_ADD)
              && pc->type == CT_CASE_COLON
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
   chunk_t *braces[256];
   int     br_cnt           = 0;
   bool    must_have_braces = false;

   chunk_t *pc = br_start;

   LOG_FMT(LBRCH, "%s: if starts on line %zu\n", __func__, br_start->orig_line);

   while (pc != nullptr)
   {
      if (pc->type == CT_BRACE_OPEN)
      {
         bool tmp = can_remove_braces(pc);
         LOG_FMT(LBRCH, "  [%d] line %zu - can%s remove %s\n",
                 br_cnt, pc->orig_line, tmp ? "" : "not",
                 get_token_name(pc->type));
         if (!tmp)
         {
            must_have_braces = true;
         }
      }
      else
      {
         bool tmp = should_add_braces(pc);
         if (tmp)
         {
            must_have_braces = true;
         }
         LOG_FMT(LBRCH, "  [%d] line %zu - %s %s\n",
                 br_cnt, pc->orig_line, tmp ? "should add" : "ignore",
                 get_token_name(pc->type));
      }

      braces[br_cnt++] = pc;
      chunk_t *br_close = chunk_skip_to_match(pc, scope_e::PREPROC);
      if (br_close == nullptr)
      {
         break;
      }
      braces[br_cnt++] = br_close;

      pc = chunk_get_next_ncnl(br_close, scope_e::PREPROC);
      if (pc == nullptr || pc->type != CT_ELSE)
      {
         break;
      }

      if (cpd.settings[UO_mod_full_brace_if_chain_only].b)
      {
         // There is an 'else' - we want full braces.
         must_have_braces = true;
      }

      pc = chunk_get_next_ncnl(pc, scope_e::PREPROC);
      if (pc != nullptr && pc->type == CT_ELSEIF)
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
      LOG_FMT(LBRCH, "%s: add braces on lines[%d]:", __func__, br_cnt);
      while (--br_cnt >= 0)
      {
         chunk_flags_set(braces[br_cnt], PCF_KEEP_BRACE);
         if (  (braces[br_cnt]->type == CT_VBRACE_OPEN)
            || (braces[br_cnt]->type == CT_VBRACE_CLOSE))
         {
            LOG_FMT(LBRCH, " %zu", braces[br_cnt]->orig_line);
            convert_vbrace(braces[br_cnt]);
         }
         else
         {
            LOG_FMT(LBRCH, " {%zu}", braces[br_cnt]->orig_line);
         }
         braces[br_cnt] = nullptr;
      }
      LOG_FMT(LBRCH, "\n");
   }
   else if (cpd.settings[UO_mod_full_brace_if_chain].b)
   {
      /*
       * This might run because either
       * UO_mod_full_brace_if_chain or UO_mod_full_brace_if_chain_only
       * is used.
       * We only want to remove braces if the first one is active.
       */
      const auto multiline_block = cpd.settings[UO_mod_full_brace_nl_block_rem_mlcond].b;

      LOG_FMT(LBRCH, "%s: remove braces on lines[%d]:", __func__, br_cnt);
      while (--br_cnt >= 0)
      {
         if (  (  braces[br_cnt]->type == CT_BRACE_OPEN
               || braces[br_cnt]->type == CT_BRACE_CLOSE)
            && ((multiline_block) ? !paren_multiline_before_brace(braces[br_cnt]) : true))
         {
            LOG_FMT(LBRCH, " {%zu}", braces[br_cnt]->orig_line);
            convert_brace(braces[br_cnt]);
         }
         else
         {
            LOG_FMT(LBRCH, " %zu", braces[br_cnt]->orig_line);
         }
         braces[br_cnt] = nullptr;
      }
      LOG_FMT(LBRCH, "\n");
   }
} // process_if_chain


static void mod_full_brace_if_chain(void)
{
   LOG_FUNC_ENTRY();

   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next(pc))
   {
      if (  (pc->type == CT_BRACE_OPEN || pc->type == CT_VBRACE_OPEN)
         && pc->parent_type == CT_IF)
      {
         process_if_chain(pc);
      }
   }
}
