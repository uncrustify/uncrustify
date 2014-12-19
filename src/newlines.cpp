/**
 * @file newlines.cpp
 * Adds or removes newlines.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "uncrustify_types.h"
#include "chunk_list.h"
#include "prototypes.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <cerrno>
#include "unc_ctype.h"


static void newlines_double_space_struct_enum_union(chunk_t *open_brace);
static bool one_liner_nl_ok(chunk_t *pc);
static void undo_one_liner(chunk_t *pc);
static void nl_handle_define(chunk_t *pc);

static void newline_iarf_pair(chunk_t *before, chunk_t *after, argval_t av);

#define MARK_CHANGE()    mark_change(__func__, __LINE__)
static void mark_change(const char *func, int line)
{
   LOG_FUNC_ENTRY();
   cpd.changes++;
   if (cpd.pass_count == 0)
   {
      LOG_FMT(LCHANGE, "%s: change %d on %s:%d\n", __func__, cpd.changes, func, line);
   }
}


/**
 * Check to see if we are allowed to increase the newline count.
 * We can't increase the nl count:
 *  - if nl_squeeze_ifdef and a preproc is after the newline.
 *  - if eat_blanks_before_close_brace and the next is '}'
 *  - if eat_blanks_after_open_brace and the prev is '{'
 */
static bool can_increase_nl(chunk_t *nl)
{
   LOG_FUNC_ENTRY();
   chunk_t *prev = chunk_get_prev_nc(nl);
   chunk_t *pcmt = chunk_get_prev(nl);
   chunk_t *next = chunk_get_next(nl);

   if (cpd.settings[UO_nl_squeeze_ifdef].b)
   {
      if (prev && (prev->type == CT_PREPROC) &&
          (prev->parent_type == CT_PP_ENDIF))
      {
         LOG_FMT(LBLANKD, "%s: nl_squeeze_ifdef %d (prev)\n", __func__, nl->orig_line);
         return(false);
      }
      if (next && (next->type == CT_PREPROC) &&
          (next->parent_type == CT_PP_ENDIF))
      {
         LOG_FMT(LBLANKD, "%s: nl_squeeze_ifdef %d (next)\n", __func__, nl->orig_line);
         return(false);
      }
   }

   if (cpd.settings[UO_eat_blanks_before_close_brace].b)
   {
      if (next && (next->type == CT_BRACE_CLOSE))
      {
         LOG_FMT(LBLANKD, "%s: eat_blanks_before_close_brace %d\n", __func__, nl->orig_line);
         return(false);
      }
   }

   if (cpd.settings[UO_eat_blanks_after_open_brace].b)
   {
      if (prev && (prev->type == CT_BRACE_OPEN))
      {
         LOG_FMT(LBLANKD, "%s: eat_blanks_after_open_brace %d\n", __func__, nl->orig_line);
         return(false);
      }
   }

   if (!pcmt && (cpd.settings[UO_nl_start_of_file].a != AV_IGNORE))
   {
      LOG_FMT(LBLANKD, "%s: no prev %d\n", __func__, nl->orig_line);
      return(false);
   }

   if (!next && (cpd.settings[UO_nl_end_of_file].a != AV_IGNORE))
   {
      LOG_FMT(LBLANKD, "%s: no next %d\n", __func__, nl->orig_line);
      return(false);
   }

   return(true);
}


/**
 * Double the newline, if allowed.
 */
static void double_newline(chunk_t *nl)
{
   LOG_FUNC_ENTRY();
   chunk_t *prev = chunk_get_prev(nl);

   LOG_FMT(LNEWLINE, "%s: add newline after %s on line %d",
           __func__, prev->str.c_str(), prev->orig_line);

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


/*
 * Basic approach:
 * 1. Find next open brace
 * 2. Find next close brace
 * 3. Determine why the braces are there
 * a. struct/union/enum "enum [name] {"
 * c. assignment "= {"
 * b. if/while/switch/for/etc ") {"
 * d. else "} else {"
 */

//#define DEBUG_NEWLINES
static void setup_newline_add(chunk_t *prev, chunk_t *nl, chunk_t *next)
{
   LOG_FUNC_ENTRY();
   if (!prev || !nl || !next)
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
   if ((prev->flags & PCF_IN_PREPROC) && (next->flags & PCF_IN_PREPROC))
   {
      nl->flags |= PCF_IN_PREPROC;
   }
   if ((nl->flags & PCF_IN_PREPROC) != 0)
   {
      set_chunk_type(nl, CT_NL_CONT);
      nl->str  = "\\\n";
   }
   else
   {
      set_chunk_type(nl, CT_NEWLINE);
      nl->str  = "\n";
   }
}


/**
 * Add a newline before the chunk if there isn't already a newline present.
 * Virtual braces are skipped, as they do not contribute to the output.
 */
chunk_t *newline_add_before(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t nl;
   chunk_t *prev;

   prev = chunk_get_prev_nvb(pc);
   if (chunk_is_newline(prev))
   {
      /* Already has a newline before this chunk */
      return(prev);
   }

   LOG_FMT(LNEWLINE, "%s: '%s' on line %d",
           __func__, pc->str.c_str(), pc->orig_line);
   log_func_stack_inline(LSETTYP);

   setup_newline_add(prev, &nl, pc);

   MARK_CHANGE();
   return(chunk_add_before(&nl, pc));
}


chunk_t *newline_force_before(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *nl = newline_add_before(pc);
   if (nl && (nl->nl_count > 1))
   {
      nl->nl_count = 1;
      MARK_CHANGE();
   }
   return nl;
}


/**
 * Add a newline after the chunk if there isn't already a newline present.
 * Virtual braces are skipped, as they do not contribute to the output.
 */
chunk_t *newline_add_after(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t nl;
   chunk_t *next;

   if (!pc)
   {
      return(NULL);
   }

   next = chunk_get_next_nvb(pc);
   if (chunk_is_newline(next))
   {
      /* Already has a newline after this chunk */
      return(next);
   }

   LOG_FMT(LNEWLINE, "%s: '%s' on line %d",
           __func__, pc->str.c_str(), pc->orig_line);
   log_func_stack_inline(LNEWLINE);

   setup_newline_add(pc, &nl, next);

   MARK_CHANGE();
   return(chunk_add_after(&nl, pc));
}


chunk_t *newline_force_after(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *nl = newline_add_after(pc);
   if (nl && (nl->nl_count > 1))
   {
      nl->nl_count = 1;
      MARK_CHANGE();
   }
   return nl;
}


/**
 * Ensure that the next non-comment token after close brace is a nl
 */
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
      if ((br_close->flags & PCF_IN_PREPROC) &&
          (next != NULL) && (next->flags & PCF_IN_PREPROC))
      {
         nl.flags |= PCF_IN_PREPROC;
      }
      if ((nl.flags & PCF_IN_PREPROC) != 0)
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
      chunk_add_after(&nl, br_close);
   }
}


static void newline_min_after(chunk_t *ref, INT32 count, UINT64 flag)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc = ref;
   chunk_t *next;

   LOG_FMT(LNEWLINE, "%s: '%s' line %d - count=%d flg=0x%" PRIx64 ":",
           __func__, ref->str.c_str(), ref->orig_line, count, flag);
   log_func_stack_inline(LNEWLINE);

   do
   {
      pc = chunk_get_next(pc);
   } while ((pc != NULL) && !chunk_is_newline(pc));

   //LOG_FMT(LNEWLINE, "%s: on %s, line %d, col %d\n",
   //        __func__, get_token_name(pc->type), pc->orig_line, pc->orig_col);

   next = chunk_get_next(pc);
   if (chunk_is_comment(next) && (next->nl_count == 1) &&
       chunk_is_comment(chunk_get_prev(pc)))
   {
      newline_min_after(next, count, flag);
      return;
   }
   else
   {
      pc->flags |= flag;
      if (chunk_is_newline(pc) && can_increase_nl(pc))
      {
         if (pc->nl_count < count)
         {
            pc->nl_count = count;
            MARK_CHANGE();
         }
      }
   }
}


/**
 * Add a newline between two tokens.
 * If there is already a newline between then, nothing is done.
 * Otherwise a newline is inserted.
 *
 * If end is CT_BRACE_OPEN and a comment and newline follow, then
 * the brace open is moved instead of inserting a newline.
 *
 * In this situation:
 *    if (...) { //comment
 *
 * you get:
 *    if (...)   //comment
 *    {
 */
chunk_t *newline_add_between(chunk_t *start, chunk_t *end)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;

   if ((start == NULL) || (end == NULL))
   {
      return(NULL);
   }

   LOG_FMT(LNEWLINE, "%s: '%s'[%s] line %d:%d and '%s' line %d:%d :",
           __func__, start->str.c_str(), get_token_name(start->type),
           start->orig_line, start->orig_col,
           end->str.c_str(), end->orig_line, end->orig_col);
   log_func_stack_inline(LNEWLINE);

   /* Back-up check for one-liners (should never be true!) */
   if (!one_liner_nl_ok(start))
   {
      return(NULL);
   }

   /* Scan for a line break */
   for (pc = start; pc != end; pc = chunk_get_next(pc))
   {
      if (chunk_is_newline(pc))
      {
         return(pc);
      }
   }

   /* If the second one is a brace open, then check to see
    * if a comment + newline follows
    */
   if (end->type == CT_BRACE_OPEN)
   {
      pc = chunk_get_next(end);
      if (chunk_is_comment(pc))
      {
         pc = chunk_get_next(pc);
         if (chunk_is_newline(pc))
         {
            /* Move the open brace to after the newline */
            chunk_move_after(end, pc);
            return(pc);
         }
      }
   }

   return(newline_add_before(end));
}


/**
 * Removes any CT_NEWLINE or CT_NL_CONT between start and end.
 * Start must be before end on the chunk list.
 * If the 'PCF_IN_PREPROC' status differs between two tags, we can't remove
 * the newline.
 *
 * @param start   The starting chunk (cannot be a newline)
 * @param end     The ending chunk (cannot be a newline)
 * @return        true/false - removed something
 */
void newline_del_between(chunk_t *start, chunk_t *end)
{
   LOG_FUNC_ENTRY();
   chunk_t *next;
   chunk_t *prev;
   chunk_t *pc = start;

   LOG_FMT(LNEWLINE, "%s: '%s' line %d:%d and '%s' line %d:%d : preproc=%d/%d ",
           __func__, start->str.c_str(), start->orig_line, start->orig_col,
           end->str.c_str(), end->orig_line, end->orig_col,
           ((start->flags & PCF_IN_PREPROC) != 0),
           ((end->flags & PCF_IN_PREPROC) != 0));
   log_func_stack_inline(LNEWLINE);

   /* Can't remove anything if the preproc status differs */
   if (!chunk_same_preproc(start, end))
   {
      return;
   }

   do
   {
      next = chunk_get_next(pc);
      if (chunk_is_newline(pc))
      {
         prev = chunk_get_prev(pc);
         if ((!chunk_is_comment(prev) && !chunk_is_comment(next)) ||
             chunk_is_newline(prev) ||
             chunk_is_newline(next))
         {
            if (chunk_safe_to_del_nl(pc))
            {
               chunk_del(pc);
               MARK_CHANGE();
               if (prev != NULL)
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

   if (chunk_is_str(end, "{", 1) &&
       (chunk_is_str(start, ")", 1) ||
        (start->type == CT_DO) ||
        (start->type == CT_ELSE)))
   {
      if (chunk_get_prev_nl(end) != start)
      {
         chunk_move_after(end, start);
      }
   }
}


/**
 * Add or remove a newline between the closing paren and opening brace.
 * Also uncuddles anything on the closing brace. (may get fixed later)
 *
 * "if (...) { \n" or "if (...) \n { \n"
 *
 * For virtual braces, we can only add a newline after the vbrace open.
 * If we do so, also add a newline after the vbrace close.
 */
static bool newlines_if_for_while_switch(chunk_t *start, argval_t nl_opt)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;
   chunk_t *close_paren;
   chunk_t *brace_open;
   bool    retval = false;

   if ((nl_opt == AV_IGNORE) ||
       (((start->flags & PCF_IN_PREPROC) != 0) &&
        !cpd.settings[UO_nl_define_macro].b))
   {
      return(false);
   }

   pc = chunk_get_next_ncnl(start);
   if ((pc != NULL) && (pc->type == CT_SPAREN_OPEN))
   {
      close_paren = chunk_get_next_type(pc, CT_SPAREN_CLOSE, pc->level);
      brace_open  = chunk_get_next_ncnl(close_paren);

      if ((brace_open != NULL) &&
          ((brace_open->type == CT_BRACE_OPEN) ||
           (brace_open->type == CT_VBRACE_OPEN)) &&
          one_liner_nl_ok(brace_open))
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
            /* Can only add - we don't want to create a one-line here */
            if (nl_opt & AV_ADD)
            {
               newline_iarf_pair(close_paren, chunk_get_next_ncnl(brace_open), nl_opt);
               pc = chunk_get_next_type(brace_open, CT_VBRACE_CLOSE, brace_open->level);
               if (!chunk_is_newline(chunk_get_prev_nc(pc)) &&
                   !chunk_is_newline(chunk_get_next_nc(pc)))
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

            /* Make sure nothing is cuddled with the closing brace */
            pc = chunk_get_next_type(brace_open, CT_BRACE_CLOSE, brace_open->level);
            newline_add_between(pc, chunk_get_next_nblank(pc));
            retval = true;
         }
      }
   }
   return(retval);
}


/**
 * Add or remove extra newline before the chunk.
 * Adds before comments
 * Doesn't do anything if open brace before it
 * "code\n\ncomment\nif (...)" or "code\ncomment\nif (...)"
 */
static void newlines_if_for_while_switch_pre_blank_lines(chunk_t *start, argval_t nl_opt)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;
   chunk_t *prev;
   chunk_t *next;
   chunk_t *last_nl = NULL;
   int     level    = start->level;
   bool    do_add   = nl_opt & AV_ADD;

   if ((nl_opt == AV_IGNORE) ||
       (((start->flags & PCF_IN_PREPROC) != 0) &&
        !cpd.settings[UO_nl_define_macro].b))
   {
      return;
   }

   /*
    * look backwards until we find
    *  open brace (don't add or remove)
    *  2 newlines in a row (don't add)
    *  something else (don't remove)
    */
   for (pc = chunk_get_prev(start); pc != NULL; pc = chunk_get_prev(pc))
   {
      if (chunk_is_newline(pc))
      {
         last_nl = pc;
         /* if we found 2 or more in a row */
         if ((pc->nl_count > 1) || chunk_is_newline(chunk_get_prev_nvb(pc)))
         {
            /* need to remove */
            if ((nl_opt & AV_REMOVE) && ((pc->flags & PCF_VAR_DEF) == 0))
            {
               /* if we're also adding, take care of that here */
               int nl_count = do_add ? 2 : 1;
               if (nl_count != pc->nl_count)
               {
                  pc->nl_count = nl_count;
                  MARK_CHANGE();
               }
               /* can keep using pc because anything other than newline stops loop, and we delete if newline */
               while (chunk_is_newline(prev = chunk_get_prev_nvb(pc)))
               {
                  /* Make sure we don't combine a preproc and non-preproc */
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
      else if (chunk_is_opening_brace(pc) || (pc->level < level))
      {
         return;
      }
      else if (chunk_is_comment(pc))
      {
         /* vbrace close is ok because it won't go into output, so we should skip it */
         last_nl = NULL;
         continue;
      }
      else
      {
         if (do_add) /* we found something previously besides a comment or a new line */
         {
            /* if we have run across a newline */
            if (last_nl != NULL)
            {
               if (last_nl->nl_count < 2)
               {
                  double_newline(last_nl);
               }
            }
            else
            {
               /* we didn't run into a nl, so we need to add one */
               if (((next = chunk_get_next(pc)) != NULL) &&
                   chunk_is_comment(next))
               {
                  pc = next;
               }
               if ((last_nl = newline_add_after(pc)) != NULL)
               {
                  double_newline(last_nl);
               }
            }
         }

         return;
      }
   }
}


static chunk_t *get_closing_brace(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;
   int     level = start->level;

   for (pc = start; (pc = chunk_get_next(pc)) != NULL; )
   {
      if (((pc->type == CT_BRACE_CLOSE) || (pc->type == CT_VBRACE_CLOSE)) && (pc->level == level))
      {
         return(pc);
      }
      /* for some reason, we can have newlines between if and opening brace that are lower level than either */
      if (!chunk_is_newline(pc) && (pc->level < level))
      {
         return(NULL);
      }
   }

   return(NULL);
}


/**
 * remove any consecutive newlines following this chunk
 * skip vbraces
 */
static void remove_next_newlines(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t *next;

   while ((next = chunk_get_next(start)) != NULL)
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


/**
 * Add or remove extra newline after end of the block started in chunk.
 * Doesn't do anything if close brace after it
 * Interesting issue is that at this point, nls can be before or after vbraces
 * VBraces will stay VBraces, conversion to real ones should have already happened
 * "if (...)\ncode\ncode" or "if (...)\ncode\n\ncode"
 */
static void newlines_if_for_while_switch_post_blank_lines(chunk_t *start, argval_t nl_opt)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;
   chunk_t *next;
   chunk_t *prev;
   bool    have_pre_vbrace_nl = false;
   int     nl_count;

   if ((nl_opt == AV_IGNORE) ||
       (((start->flags & PCF_IN_PREPROC) != 0) &&
        !cpd.settings[UO_nl_define_macro].b))
   {
      return;
   }

   /* first find ending brace */
   if ((pc = get_closing_brace(start)) == NULL)
   {
      return;
   }

   /* if we're dealing with an if, we actually want to add or remove blank lines after any elses */
   if (start->type == CT_IF)
   {
      while (true)
      {
         next = chunk_get_next_ncnl(pc);
         if ((next != NULL) && ((next->type == CT_ELSE) || (next->type == CT_ELSEIF)))
         {
            /* point to the closing brace of the else */
            if ((pc = get_closing_brace(next)) == NULL)
            {
               return;
            }
         }
         else
         {
            break;
         }
      }
   }

   /* if we're dealing with a do/while, we actually want to add or remove blank lines after while and its condition */
   if (start->type == CT_DO)
   {
      /* point to the next semicolon */
      if ((pc = chunk_get_next_type(pc, CT_SEMICOLON, start->level)) == NULL)
      {
         return;
      }
   }

   bool isVBrace = pc->type == CT_VBRACE_CLOSE;
   if ((prev = chunk_get_prev_nvb(pc)) == NULL)
   {
      return;
   }

   have_pre_vbrace_nl = isVBrace && chunk_is_newline(prev);
   if (nl_opt & AV_REMOVE)
   {
      /* if vbrace, have to check before and after */
      /* if chunk before vbrace, remove any nls after vbrace */
      if (have_pre_vbrace_nl)
      {
         if (prev->nl_count != 1)
         {
            prev->nl_count = 1;
            MARK_CHANGE();
         }
         remove_next_newlines(pc);
      }
      else if ((chunk_is_newline(next = chunk_get_next_nvb(pc))) &&
               !(next->flags & PCF_VAR_DEF))
      {
         /* otherwise just deal with nls after brace */
         if (next->nl_count != 1)
         {
            next->nl_count = 1;
            MARK_CHANGE();
         }
         remove_next_newlines(next);
      }
   }

   /* may have a nl before and after vbrace */
   /* don't do anything with it if the next non nl chunk is a closing brace */
   if (nl_opt & AV_ADD)
   {
      if ((next = chunk_get_next_nnl(pc)) == NULL)
      {
         return;
      }

      if (next->type != CT_BRACE_CLOSE)
      {
         /* if vbrace, have to check before and after */
         /* if chunk before vbrace, check its count */
         nl_count = have_pre_vbrace_nl ? prev->nl_count : 0;
         if (chunk_is_newline(next = chunk_get_next_nvb(pc)))
         {
            nl_count += next->nl_count;
         }

         /* if we have no newlines, add one and make it double */
         if (nl_count == 0)
         {
            if (((next = chunk_get_next(pc)) != NULL) &&
                chunk_is_comment(next))
            {
               pc = next;
            }

            if ((next = newline_add_after(pc)) == NULL)
            {
               return;
            }
            double_newline(next);
         }
         else if (nl_count == 1) /* if we don't have enough newlines */
         {
            /* if we have one before vbrace, need to add one after */
            if (have_pre_vbrace_nl)
            {
               next = newline_add_after(pc);
            }
            else
            {
               prev = chunk_get_prev_nnl(next);
               pc   = chunk_get_next_nl(next);
               //LOG_FMT(LSYS, "  -- pc1=%s [%s]\n", pc->str.c_str(), get_token_name(pc->type));

               pc = chunk_get_next(pc);
               //LOG_FMT(LSYS, "  -- pc2=%s [%s]\n", pc->str.c_str(), get_token_name(pc->type));
               if ((pc != NULL) && (pc->type == CT_PREPROC) &&
                   (pc->parent_type == CT_PP_ENDIF) &&
                   cpd.settings[UO_nl_squeeze_ifdef].b)
               {
                  LOG_FMT(LNEWLINE, "%s: cannot add newline after line %d due to nl_squeeze_ifdef\n",
                          __func__, prev->orig_line);
               }
               else
               {
                  /* make nl after double */
                  double_newline(next);
               }
            }
         }
      }
   }
}


/**
 * Adds or removes a newline between the keyword and the open brace.
 * If there is something after the '{' on the same line, then
 * the newline is removed unconditionally.
 * If there is a '=' between the keyword and '{', do nothing.
 *
 * "struct [name] {" or "struct [name] \n {"
 */
static void newlines_struct_enum_union(chunk_t *start, argval_t nl_opt, bool leave_trailing)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;
   chunk_t *next;

   if ((nl_opt == AV_IGNORE) ||
       (((start->flags & PCF_IN_PREPROC) != 0) &&
        !cpd.settings[UO_nl_define_macro].b))
   {
      return;
   }

   /* step past any junk between the keyword and the open brace
    * Quit if we hit a semicolon or '=', which are not expected.
    */
   int level = start->level;
   pc = start;
   while (((pc = chunk_get_next_ncnl(pc)) != NULL) && (pc->level >= level))
   {
      if ((pc->level == level) &&
          ((pc->type == CT_BRACE_OPEN) ||
           chunk_is_semicolon(pc) ||
           (pc->type == CT_ASSIGN)))
      {
         break;
      }
      start = pc;
   }

   /* If we hit a brace open, then we need to toy with the newlines */
   if ((pc != NULL) && (pc->type == CT_BRACE_OPEN))
   {
      /* Skip over embedded C comments */
      next = chunk_get_next(pc);
      while ((next != NULL) && (next->type == CT_COMMENT))
      {
         next = chunk_get_next(next);
      }
      if (leave_trailing &&
          !chunk_is_comment(next) &&
          !chunk_is_newline(next))
      {
         nl_opt = AV_IGNORE;
      }

      newline_iarf_pair(start, pc, nl_opt);
   }
}


/**
 * Cuddles or un-cuddles a chunk with a previous close brace
 *
 * "} while" vs "} \n while"
 * "} else" vs "} \n else"
 *
 * @param start   The chunk - should be CT_ELSE or CT_WHILE_OF_DO
 */
static void newlines_cuddle_uncuddle(chunk_t *start, argval_t nl_opt)
{
   LOG_FUNC_ENTRY();
   chunk_t *br_close;

   if (((start->flags & PCF_IN_PREPROC) != 0) &&
       !cpd.settings[UO_nl_define_macro].b)
   {
      return;
   }

   br_close = chunk_get_prev_ncnl(start);
   if ((br_close != NULL) && (br_close->type == CT_BRACE_CLOSE))
   {
      newline_iarf_pair(br_close, start, nl_opt);
   }
}


/**
 * Adds/removes a newline between else and '{'.
 * "else {" or "else \n {"
 */
static void newlines_do_else(chunk_t *start, argval_t nl_opt)
{
   LOG_FUNC_ENTRY();
   chunk_t *next;
   chunk_t *tmp;

   if ((nl_opt == AV_IGNORE) ||
       (((start->flags & PCF_IN_PREPROC) != 0) &&
        !cpd.settings[UO_nl_define_macro].b))
   {
      return;
   }

   next = chunk_get_next_ncnl(start);
   if ((next != NULL) &&
       ((next->type == CT_BRACE_OPEN) ||
        (next->type == CT_VBRACE_OPEN)))
   {
      if (!one_liner_nl_ok(next))
      {
         return;
      }
      if (next->type == CT_VBRACE_OPEN)
      {
         /* Can only add - we don't want to create a one-line here */
         if (nl_opt & AV_ADD)
         {
            newline_iarf_pair(start, chunk_get_next_ncnl(next), nl_opt);
            tmp = chunk_get_next_type(next, CT_VBRACE_CLOSE, next->level);
            if (!chunk_is_newline(chunk_get_next_nc(tmp)) &&
                !chunk_is_newline(chunk_get_prev_nc(tmp)))
            {
               newline_add_after(tmp);
            }
         }
      }
      else
      {
         newline_iarf_pair(start, next, nl_opt);
         if ((nl_opt & AV_ADD) != 0)
         {
            tmp = chunk_get_next_nc(next);
            if ((tmp != NULL) && !chunk_is_newline(tmp))
            {
               newline_add_between(next, tmp);
            }
         }
      }
   }
}


/**
 * Put a newline before and after a block of variable definitions
 */
static chunk_t *newline_def_blk(chunk_t *start, bool fn_top)
{
   LOG_FUNC_ENTRY();
   chunk_t *next;
   chunk_t *prev;
   chunk_t *pc;
   bool    did_this_line = false;
   bool    first_var_blk = true;
   bool    typedef_blk   = false;
   bool    var_blk       = false;

   prev = chunk_get_prev_ncnl(start);
   /* can't be any variable definitions in a "= {" block */
   if ((prev != NULL) && (prev->type == CT_ASSIGN))
   {
      pc = chunk_get_next_type(start, CT_BRACE_CLOSE, start->level);
      return(chunk_get_next_ncnl(pc));
   }
   pc = chunk_get_next(start);
   while ((pc != NULL) && ((pc->level >= start->level) || (pc->level == 0)))
   {
      if (chunk_is_comment(pc))
      {
         pc = chunk_get_next(pc);
         continue;
      }

      /* process nested braces */
      if (pc->type == CT_BRACE_OPEN)
      {
         pc = newline_def_blk(pc, false);
         continue;
      }

      /* Done with this brace set? */
      if (pc->type == CT_BRACE_CLOSE)
      {
         pc = chunk_get_next(pc);
         break;
      }

      /* skip vbraces */
      if (pc->type == CT_VBRACE_OPEN)
      {
         pc = chunk_get_next_type(pc, CT_VBRACE_CLOSE, pc->level);
         if (pc != NULL)
         {
            pc = chunk_get_next(pc);
         }
         continue;
      }

      /* Ignore stuff inside parens/squares/angles */
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

      /* Determine if this is a variable def or code */
      if (!did_this_line &&
          (pc->type != CT_FUNC_CLASS_DEF) &&
          (pc->type != CT_FUNC_CLASS_PROTO) &&
          ((pc->level == (start->level + 1)) ||
           (pc->level == 0)))
      {
         next = chunk_get_next_ncnl(pc);
         prev = chunk_get_prev_ncnl(pc);
         if (pc->type == CT_TYPEDEF)
         {
            /* set newlines before typedef block */
            if (!typedef_blk && (prev != NULL) &&
                (cpd.settings[UO_nl_typedef_blk_start].n > 0))
            {
               newline_min_after(prev, cpd.settings[UO_nl_typedef_blk_start].n, PCF_VAR_DEF);
            }
            /* set newlines within typedef block */
            else if (typedef_blk && (cpd.settings[UO_nl_typedef_blk_in].n > 0))
            {
               prev = chunk_get_prev(pc);
               if (chunk_is_newline(prev))
               {
                  if (prev->nl_count > cpd.settings[UO_nl_typedef_blk_in].n)
                  {
                     prev->nl_count = cpd.settings[UO_nl_typedef_blk_in].n;
                     MARK_CHANGE();
                  }
               }
            }
            /* set blank lines after first var def block */
            if (var_blk && first_var_blk && fn_top &&
                (cpd.settings[UO_nl_func_var_def_blk].n > 0))
            {
               newline_min_after(prev, 1 + cpd.settings[UO_nl_func_var_def_blk].n, PCF_VAR_DEF);
            }
            /* set newlines after var def block */
            else if (var_blk && (cpd.settings[UO_nl_var_def_blk_end].n > 0))
            {
               newline_min_after(prev, cpd.settings[UO_nl_var_def_blk_end].n, PCF_VAR_DEF);
            }
            pc            = chunk_get_next_type(pc, CT_SEMICOLON, pc->level);
            typedef_blk   = true;
            first_var_blk = false;
            var_blk       = false;
         }
         else if (chunk_is_type(pc) &&
                  ((chunk_is_type(next) || (next->type == CT_WORD) ||
                    (next->type == CT_FUNC_CTOR_VAR))))
         {
            /* set newlines before var def block */
            if (!var_blk && !first_var_blk &&
                (cpd.settings[UO_nl_var_def_blk_start].n > 0))
            {
               newline_min_after(prev, cpd.settings[UO_nl_var_def_blk_start].n, PCF_VAR_DEF);
            }
            /* set newlines within var def block */
            else if (var_blk && (cpd.settings[UO_nl_var_def_blk_in].n > 0))
            {
               prev = chunk_get_prev(pc);
               if (chunk_is_newline(prev))
               {
                  if (prev->nl_count > cpd.settings[UO_nl_var_def_blk_in].n)
                  {
                     prev->nl_count = cpd.settings[UO_nl_var_def_blk_in].n;
                     MARK_CHANGE();
                  }
               }
            }
            /* set newlines after typedef block */
            else if (typedef_blk && (cpd.settings[UO_nl_typedef_blk_end].n > 0))
            {
               newline_min_after(prev, cpd.settings[UO_nl_typedef_blk_end].n, PCF_VAR_DEF);
            }
            pc          = chunk_get_next_type(pc, CT_SEMICOLON, pc->level);
            typedef_blk = false;
            var_blk     = true;
         }
         else
         {
            /* set newlines after typedef block */
            if (typedef_blk && (cpd.settings[UO_nl_var_def_blk_end].n > 0))
            {
               newline_min_after(prev, cpd.settings[UO_nl_var_def_blk_end].n, PCF_VAR_DEF);
            }
            /* set blank lines after first var def block */
            if (var_blk && first_var_blk && fn_top &&
                (cpd.settings[UO_nl_func_var_def_blk].n > 0))
            {
               newline_min_after(prev, 1 + cpd.settings[UO_nl_func_var_def_blk].n, PCF_VAR_DEF);
            }
            /* set newlines after var def block */
            else if (var_blk && (cpd.settings[UO_nl_var_def_blk_end].n > 0))
            {
               newline_min_after(prev, cpd.settings[UO_nl_var_def_blk_end].n, PCF_VAR_DEF);
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
}


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
 *
 */
static void newlines_brace_pair(chunk_t *br_open)
{
   LOG_FUNC_ENTRY();
   chunk_t  *prev;
   chunk_t  *next;
   chunk_t  *pc;
   argval_t val            = AV_IGNORE;
   bool     nl_close_brace = false;

   if (((br_open->flags & PCF_IN_PREPROC) != 0) &&
       !cpd.settings[UO_nl_define_macro].b)
   {
      return;
   }

   if (cpd.settings[UO_nl_collapse_empty_body].b)
   {
      next = chunk_get_next_nnl(br_open);
      if ((next != NULL) && (next->type == CT_BRACE_CLOSE))
      {
         pc = chunk_get_next(br_open);

         while ((pc != NULL) && (pc->type != CT_BRACE_CLOSE))
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

   /* Make sure we don't break a one-liner */
   if (!one_liner_nl_ok(br_open))
   {
      return;
   }

   next = chunk_get_next_nc(br_open);

   /** Insert a newline between the '=' and open brace, if needed */
   if (br_open->parent_type == CT_ASSIGN)
   {
      /* Only mess with it if the open brace is followed by a newline */
      if (chunk_is_newline(next))
      {
         prev = chunk_get_prev_ncnl(br_open);

         newline_iarf_pair(prev, br_open, cpd.settings[UO_nl_assign_brace].a);
      }
   }

   /* Eat any extra newlines after the brace open */
   if (cpd.settings[UO_eat_blanks_after_open_brace].b)
   {
      if (chunk_is_newline(next))
      {
         if (next->nl_count > 1)
         {
            next->nl_count = 1;
            MARK_CHANGE();
         }
      }
   }

   /* Handle the cases where the brace is part of a function call or definition */
   if ((br_open->parent_type == CT_FUNC_DEF) ||
       (br_open->parent_type == CT_FUNC_CALL) ||
       (br_open->parent_type == CT_FUNC_CALL_USER) ||
       (br_open->parent_type == CT_FUNC_CLASS_DEF) ||
       (br_open->parent_type == CT_OC_MSG_DECL) ||
       (br_open->parent_type == CT_CS_PROPERTY) ||
       (br_open->parent_type == CT_CPP_LAMBDA))
   {
      /* Need to force a newline before the close brace, if not in a class body */
      if ((br_open->flags & PCF_IN_CLASS) == 0)
      {
         nl_close_brace = true;
      }

      /* handle newlines after the open brace */
      pc = chunk_get_next_ncnl(br_open);
      newline_add_between(br_open, pc);

      val = (((br_open->parent_type == CT_FUNC_DEF) ||
              (br_open->parent_type == CT_FUNC_CLASS_DEF) ||
              (br_open->parent_type == CT_OC_MSG_DECL)) ?
             cpd.settings[UO_nl_fdef_brace].a :
             ((br_open->parent_type == CT_CS_PROPERTY) ?
              cpd.settings[UO_nl_property_brace].a :
              ((br_open->parent_type == CT_CPP_LAMBDA) ?
               cpd.settings[UO_nl_cpp_ldef_brace].a :
               cpd.settings[UO_nl_fcall_brace].a)));

      if (val != AV_IGNORE)
      {
         /* Grab the chunk before the open brace */
         prev = chunk_get_prev_ncnl(br_open);

         newline_iarf_pair(prev, br_open, val);
      }

      newline_def_blk(br_open, true);
   }

   /* Handle the cases where the brace is part of a class or struct */
   if ((br_open->parent_type == CT_CLASS) ||
       (br_open->parent_type == CT_STRUCT))
   {
      newline_def_blk(br_open, false);
   }

   /* Grab the matching brace close */
   chunk_t *br_close;
   br_close = chunk_get_next_type(br_open, CT_BRACE_CLOSE, br_open->level);
   if (br_close == NULL)
   {
      return;
   }

   if (!nl_close_brace)
   {
      /**
       * If the open brace hits a CT_NEWLINE, CT_NL_CONT, CT_COMMENT_MULTI, or
       * CT_COMMENT_CPP without hitting anything other than CT_COMMENT, then
       * there should be a newline before the close brace.
       */
      pc = chunk_get_next(br_open);
      while ((pc != NULL) && (pc->type == CT_COMMENT))
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
}


/**
 * Put a empty line between the 'case' statement and the previous case colon
 * or semicolon.
 * Does not work with PAWN (?)
 */
static void newline_case(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;
   chunk_t *prev = start;

   //   printf("%s case (%s) on line %d col %d\n",
   //          __func__, c_chunk_names[start->type],
   //          start->orig_line, start->orig_col);

   /* Scan backwards until a '{' or ';' or ':'. Abort if a multi-newline is found */
   do
   {
      prev = chunk_get_prev_nc(prev);
      if ((prev != NULL) && chunk_is_newline(prev) && (prev->nl_count > 1))
      {
         return;
      }
   } while ((prev != NULL) &&
            (prev->type != CT_BRACE_OPEN) &&
            (prev->type != CT_BRACE_CLOSE) &&
            (prev->type != CT_SEMICOLON) &&
            (prev->type != CT_CASE_COLON));

   if (prev == NULL)
   {
      return;
   }

   pc = newline_add_between(prev, start);
   if (pc == NULL)
   {
      return;
   }

   /* Only add an extra line after a semicolon or brace close */
   if ((prev->type == CT_SEMICOLON) ||
       (prev->type == CT_BRACE_CLOSE))
   {
      if (chunk_is_newline(pc) && (pc->nl_count < 2))
      {
         double_newline(pc);
      }
   }
}


static void newline_case_colon(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc = start;

   /* Scan forwards until a non-comment is found */
   do
   {
      pc = chunk_get_next(pc);
   } while (chunk_is_comment(pc));

   if ((pc != NULL) && !chunk_is_newline(pc))
   {
      newline_add_before(pc);
   }
}


/**
 * Put a blank line before a return statement, unless it is after an open brace
 */
static void newline_before_return(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t *nl;
   chunk_t *pc;

   nl = chunk_get_prev(start);
   if (!chunk_is_newline(nl))
   {
      /* Don't mess with lines that don't start with 'return' */
      return;
   }

   /* Do we already have a blank line? */
   if (nl->nl_count > 1)
   {
      return;
   }

   pc = chunk_get_prev(nl);
   if (!pc || ((pc->type == CT_BRACE_OPEN) || (pc->type == CT_VBRACE_OPEN)))
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


/**
 * Put a empty line after a return statement, unless it is followed by a
 * close brace.
 *
 * May not work with PAWN
 */
static void newline_after_return(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;
   chunk_t *semi;
   chunk_t *after;

   semi  = chunk_get_next_type(start, CT_SEMICOLON, start->level);
   after = chunk_get_next_nblank(semi);

   /* If we hit a brace or an 'else', then a newline isn't needed */
   if ((after == NULL) ||
       (after->type == CT_BRACE_CLOSE) ||
       (after->type == CT_VBRACE_CLOSE) ||
       (after->type == CT_ELSE))
   {
      return;
   }

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


/**
 * Does the Ignore, Add, Remove, or Force thing between two chunks
 *
 * @param before The first chunk
 * @param after  The second chunk
 * @param av     The IARF value
 */
static void newline_iarf_pair(chunk_t *before, chunk_t *after, argval_t av)
{
   LOG_FUNC_ENTRY();
   log_func_stack(LNEWLINE, "Call Stack:");

   if ((before != NULL) && (after != NULL))
   {
      if ((av & AV_ADD) != 0)
      {
         chunk_t *nl = newline_add_between(before, after);
         if (nl && (av == AV_FORCE) && (nl->nl_count > 1))
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


/**
 * Does a simple Ignore, Add, Remove, or Force after the given chunk
 *
 * @param pc   The chunk
 * @param av   The IARF value
 */
void newline_iarf(chunk_t *pc, argval_t av)
{
   LOG_FUNC_ENTRY();
   log_func_stack(LNEWLINE, "CallStack:");

   newline_iarf_pair(pc, chunk_get_next_nnl(pc), av);
}


/**
 * Formats a function declaration
 * Start points to the open paren
 */
static void newline_func_def(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t  *pc;
   chunk_t  *prev = NULL;
   chunk_t  *tmp;
   argval_t atmp;
   bool     is_def = (start->parent_type == CT_FUNC_DEF) || (start->parent_type == CT_FUNC_CLASS_DEF);

   LOG_FMT(LNFD, "%s: called on %d:%d '%s' [%s/%s]\n",
           __func__, start->orig_line, start->orig_col,
           start->text(), get_token_name(start->type), get_token_name(start->parent_type));

   atmp = cpd.settings[is_def ? UO_nl_func_def_paren : UO_nl_func_paren].a;
   if (atmp != AV_IGNORE)
   {
      prev = chunk_get_prev_ncnl(start);
      if (prev != NULL)
      {
         newline_iarf(prev, atmp);
      }
   }

   /* Handle break newlines type and function */
   prev = chunk_get_prev_ncnl(start);
   prev = skip_template_prev(prev);
   /* Don't split up a function variable */
   prev = chunk_is_paren_close(prev) ? NULL : chunk_get_prev_ncnl(prev);

   if ((prev != NULL) && (prev->type != CT_PRIVATE_COLON))
   {
      if (prev->type == CT_OPERATOR)
      {
         tmp  = prev;
         prev = chunk_get_prev_ncnl(prev);
      }
      else
      {
         tmp = start;
      }
      if ((prev != NULL) && (prev->type == CT_DC_MEMBER))
      {
         if (cpd.settings[UO_nl_func_scope_name].a != AV_IGNORE)
         {
            newline_iarf(prev, cpd.settings[UO_nl_func_scope_name].a);
         }
      }

      if (chunk_get_next_ncnl(prev)->type != CT_FUNC_CLASS_DEF)
      {
         argval_t a = (tmp->parent_type == CT_FUNC_PROTO) ?
                      cpd.settings[UO_nl_func_proto_type_name].a :
                      cpd.settings[UO_nl_func_type_name].a;
         if ((tmp->flags & PCF_IN_CLASS) &&
             (cpd.settings[UO_nl_func_type_name_class].a != AV_IGNORE))
         {
            a = cpd.settings[UO_nl_func_type_name_class].a;
         }

         if (a != AV_IGNORE)
         {
            LOG_FMT(LNFD, "%s: prev %d:%d '%s' [%s/%s]\n",
                    __func__, prev->orig_line, prev->orig_col,
                    prev->text(), get_token_name(prev->type), get_token_name(prev->parent_type));

            if ((prev != NULL) && (prev->type == CT_DESTRUCTOR))
            {
               prev = chunk_get_prev_ncnl(prev);
            }

            /* If we are on a '::', step back two tokens
             * TODO: do we also need to check for '.' ?
             */
            while ((prev != NULL) && (prev->type == CT_DC_MEMBER))
            {
               prev = chunk_get_prev_ncnl(prev);
               prev = skip_template_prev(prev);
               prev = chunk_get_prev_ncnl(prev);
            }

            if ((prev != NULL) &&
                (prev->type != CT_BRACE_CLOSE) &&
                (prev->type != CT_VBRACE_CLOSE) &&
                (prev->type != CT_BRACE_OPEN) &&
                (prev->type != CT_SEMICOLON) &&
                (prev->parent_type != CT_TEMPLATE))
            {
               newline_iarf(prev, a);
            }
         }
      }
   }

   pc = chunk_get_next_ncnl(start);
   if (chunk_is_str(pc, ")", 1))
   {
      atmp = cpd.settings[is_def ? UO_nl_func_def_empty : UO_nl_func_decl_empty].a;
      if (atmp != AV_IGNORE)
      {
         newline_iarf(start, atmp);
      }
      return;
   }

   /* Now scan for commas */
   int comma_count = 0;
   for (pc = chunk_get_next_ncnl(start);
        (pc != NULL) && (pc->level > start->level);
        pc = chunk_get_next_ncnl(pc))
   {
      prev = pc;
      if ((pc->type == CT_COMMA) && (pc->level == (start->level + 1)))
      {
         comma_count++;
         tmp = chunk_get_next(pc);
         if (chunk_is_comment(tmp))
         {
            pc = tmp;
         }
         newline_iarf(pc, cpd.settings[((start->parent_type == CT_FUNC_DEF) ||
                                        (start->parent_type == CT_FUNC_CLASS_DEF)) ?
                                       UO_nl_func_def_args :
                                       UO_nl_func_decl_args].a);
      }
   }

   argval_t as = cpd.settings[is_def ? UO_nl_func_def_start : UO_nl_func_decl_start].a;
   argval_t ae = cpd.settings[is_def ? UO_nl_func_def_end : UO_nl_func_decl_end].a;
   if (comma_count == 0)
   {
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

   /* and fix up the close paren */
   if ((prev != NULL) && (pc != NULL) && (pc->type == CT_FPAREN_CLOSE))
   {
      prev = chunk_get_prev_nnl(pc);
      if (prev->type != CT_FPAREN_OPEN)
      {
         newline_iarf(prev, ae);
      }
   }
}


/**
 * Formats a message, adding newlines before the item before the colons.
 *
 * Start points to the open '[' in:
 * [myObject doFooWith:arg1 name:arg2  // some lines with >1 arg
 *            error:arg3];
 */
static void newline_oc_msg(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;
   chunk_t *sq_c;
   bool    one_liner = true;

   sq_c = chunk_skip_to_match(start);
   if (!sq_c)
   {
      return;
   }

   /* mark one-liner */
   for (pc = chunk_get_next(start); pc && (pc != sq_c); pc = chunk_get_next(pc))
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

   /* we don't use the 1-liner flag, but set it anyway */
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
}


/**
 * Checks to see if it is OK to add a newline around the chunk.
 * Don't want to break one-liners...
 */
static bool one_liner_nl_ok(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LNL1LINE, "%s: check [%s] parent=[%s] flg=%" PRIx64 ", on line %d, col %d - ",
           __func__, get_token_name(pc->type), get_token_name(pc->parent_type),
           pc->flags, pc->orig_line, pc->orig_col);

   if (!(pc->flags & PCF_ONE_LINER))
   {
      LOG_FMT(LNL1LINE, "true (not 1-liner)\n");
      return(true);
   }

   /* Step back to find the opening brace */
   chunk_t *br_open = pc;
   if (chunk_is_closing_brace(br_open))
   {
      br_open = chunk_get_prev_type(br_open,
                                    br_open->type == CT_BRACE_CLOSE ? CT_BRACE_OPEN : CT_VBRACE_OPEN,
                                    br_open->level, CNAV_ALL);
   }
   else
   {
      while (br_open &&
             (br_open->flags & PCF_ONE_LINER) &&
             !chunk_is_opening_brace(br_open) &&
             !chunk_is_closing_brace(br_open))
      {
         br_open = chunk_get_prev(br_open);
      }
   }
   pc = br_open;
   if (pc && (pc->flags & PCF_ONE_LINER) &&
       ((pc->type == CT_BRACE_OPEN) ||
        (pc->type == CT_BRACE_CLOSE) ||
        (pc->type == CT_VBRACE_OPEN) ||
        (pc->type == CT_VBRACE_CLOSE)))
   {
      if (cpd.settings[UO_nl_class_leave_one_liners].b &&
          (pc->flags & PCF_IN_CLASS))
      {
         LOG_FMT(LNL1LINE, "false (class)\n");
         return(false);
      }

      if (cpd.settings[UO_nl_assign_leave_one_liners].b &&
          (pc->parent_type == CT_ASSIGN))
      {
         LOG_FMT(LNL1LINE, "false (assign)\n");
         return(false);
      }

      if (cpd.settings[UO_nl_enum_leave_one_liners].b &&
          (pc->parent_type == CT_ENUM))
      {
         LOG_FMT(LNL1LINE, "false (enum)\n");
         return(false);
      }

      if (cpd.settings[UO_nl_getset_leave_one_liners].b &&
          (pc->parent_type == CT_GETSET))
      {
         LOG_FMT(LNL1LINE, "false (get/set)\n");
         return(false);
      }

      if (cpd.settings[UO_nl_func_leave_one_liners].b &&
          ((pc->parent_type == CT_FUNC_DEF) ||
           (pc->parent_type == CT_FUNC_CLASS_DEF)))
      {
         LOG_FMT(LNL1LINE, "false (func def)\n");
         return(false);
      }

      if (cpd.settings[UO_nl_func_leave_one_liners].b &&
          (pc->parent_type == CT_OC_MSG_DECL))
      {
         LOG_FMT(LNL1LINE, "false (method def)\n");
         return(false);
      }

      if (cpd.settings[UO_nl_cpp_lambda_leave_one_liners].b &&
          ((pc->parent_type == CT_CPP_LAMBDA)))
      {
         LOG_FMT(LNL1LINE, "false (lambda)\n");
         return(false);
      }

      if (cpd.settings[UO_nl_oc_msg_leave_one_liner].b &&
          (pc->flags & PCF_IN_OC_MSG))
      {
         LOG_FMT(LNL1LINE, "false (message)\n");
         return(false);
      }

      if (cpd.settings[UO_nl_if_leave_one_liners].b &&
          ((pc->parent_type == CT_IF) ||
           (pc->parent_type == CT_ELSE)))
      {
         LOG_FMT(LNL1LINE, "false (if/else)\n");
         return(false);
      }
   }
   LOG_FMT(LNL1LINE, "true\n");
   return(true);
}


/**
 * Clears the PCF_ONE_LINER flag on the current line.
 * Done right before inserting a newline.
 */
static void undo_one_liner(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *tmp;

   if (pc && (pc->flags & PCF_ONE_LINER))
   {
      LOG_FMT(LNL1LINE, "%s: [%s]", __func__, pc->text());
      pc->flags &= ~PCF_ONE_LINER;

      /* scan backward */
      tmp = pc;
      while ((tmp = chunk_get_prev(tmp)) != NULL)
      {
         if (!(tmp->flags & PCF_ONE_LINER))
         {
            break;
         }
         LOG_FMT(LNL1LINE, " %s", tmp->text());
         tmp->flags &= ~PCF_ONE_LINER;
      }

      /* scan forward */
      tmp = pc;
      LOG_FMT(LNL1LINE, " -");
      while ((tmp = chunk_get_next(tmp)) != NULL)
      {
         if (!(tmp->flags & PCF_ONE_LINER))
         {
            break;
         }
         LOG_FMT(LNL1LINE, " %s", tmp->text());
         tmp->flags &= ~PCF_ONE_LINER;
      }
      LOG_FMT(LNL1LINE, "\n");
   }
}


static void nl_create_one_liner(chunk_t *vbrace_open)
{
   LOG_FUNC_ENTRY();
   chunk_t *tmp;
   chunk_t *first;

   /* See if we get a newline between the next text and the vbrace_close */
   tmp   = chunk_get_next_ncnl(vbrace_open);
   first = tmp;
   if (!first || (get_token_pattern_class(first->type) != PATCLS_NONE))
   {
      return;
   }

   int nl_total = 0;
   while ((tmp != NULL) && (tmp->type != CT_VBRACE_CLOSE))
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

   if ((tmp != NULL) && (first != NULL))
   {
      newline_del_between(vbrace_open, first);
   }
}


/**
 * Remove all extra newlines.
 */
void newlines_remove_newlines(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc = chunk_get_head();
   chunk_t *next;
   chunk_t *prev;

   if (!chunk_is_newline(pc))
   {
      pc = chunk_get_next_nl(pc);
   }
   while (pc != NULL)
   {
      /* Remove all newlines not in preproc */
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


/**
 * Step through all chunks.
 */
void newlines_cleanup_braces(bool first)
{
   LOG_FUNC_ENTRY();
   chunk_t  *pc;
   chunk_t  *next;
   chunk_t  *prev;
   chunk_t  *tmp;
   argval_t arg;

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next_ncnl(pc))
   {
      if (pc->type == CT_IF)
      {
         newlines_if_for_while_switch(pc, cpd.settings[UO_nl_if_brace].a);
      }
      else if (pc->type == CT_ELSEIF)
      {
         arg = cpd.settings[UO_nl_elseif_brace].a;
         newlines_if_for_while_switch(
            pc, (arg != AV_IGNORE) ? arg : cpd.settings[UO_nl_if_brace].a);
      }
      else if (pc->type == CT_FOR)
      {
         newlines_if_for_while_switch(pc, cpd.settings[UO_nl_for_brace].a);
      }
      else if (pc->type == CT_CATCH)
      {
         newlines_cuddle_uncuddle(pc, cpd.settings[UO_nl_brace_catch].a);
         next = chunk_get_next_ncnl(pc);
         if ((next != NULL) && (next->type == CT_BRACE_OPEN))
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
      else if (pc->type == CT_DO)
      {
         newlines_do_else(pc, cpd.settings[UO_nl_do_brace].a);
      }
      else if (pc->type == CT_ELSE)
      {
         newlines_cuddle_uncuddle(pc, cpd.settings[UO_nl_brace_else].a);
         next = chunk_get_next_ncnl(pc);
         if ((next != NULL) && (next->type == CT_ELSEIF))
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
         if ((pc->parent_type == CT_DOUBLE_BRACE) &&
             (cpd.settings[UO_nl_paren_dbrace_open].a != AV_IGNORE))
         {
            prev = chunk_get_prev_ncnl(pc, CNAV_PREPROC);
            if (chunk_is_paren_close(prev))
            {
               newline_iarf_pair(prev, pc, cpd.settings[UO_nl_paren_dbrace_open].a);
            }
         }

         if (cpd.settings[UO_nl_brace_brace].a != AV_IGNORE)
         {
            next = chunk_get_next_nc(pc, CNAV_PREPROC);
            if ((next != NULL) && (next->type == CT_BRACE_OPEN))
            {
               newline_iarf_pair(pc, next, cpd.settings[UO_nl_brace_brace].a);
            }
         }

         if (cpd.settings[UO_nl_ds_struct_enum_cmt].b &&
             ((pc->parent_type == CT_ENUM) ||
              (pc->parent_type == CT_STRUCT) ||
              (pc->parent_type == CT_UNION)))
         {
            newlines_double_space_struct_enum_union(pc);
         }

         if ((pc->parent_type == CT_CLASS) && (pc->level == pc->brace_level))
         {
            newlines_do_else(chunk_get_prev_nnl(pc),
                             cpd.settings[UO_nl_class_brace].a);
         }

         next = chunk_get_next_nnl(pc);
         if (next == NULL)
         {
            // do nothing
         }
         else if (next->type == CT_BRACE_CLOSE)
         {
            //TODO: add an option to split open empty statements? { };
         }
         else if (next->type == CT_BRACE_OPEN)
         {
            // already handled
         }
         else
         {
            next = chunk_get_next_ncnl(pc);

            // Handle nl_after_brace_open
            if (((pc->parent_type == CT_CPP_LAMBDA) ||
                 (pc->level == pc->brace_level)) &&
                cpd.settings[UO_nl_after_brace_open].b)
            {
               if (!one_liner_nl_ok(pc))
               {
                  /* no change - preserve one liner body */
               }
               else if ((pc->flags & (PCF_IN_ARRAY_ASSIGN | PCF_IN_PREPROC)) != 0)
               {
                  /* no change - don't break up array assignments or preprocessors */
               }
               else
               {
                  /* Step back from next to the first non-newline item */
                  tmp = chunk_get_prev(next);
                  while (tmp != pc)
                  {
                     if (chunk_is_comment(tmp))
                     {
                        if (!cpd.settings[UO_nl_after_brace_open_cmt].b &&
                            (tmp->type != CT_COMMENT_MULTI))
                        {
                           break;
                        }
                     }
                     tmp = chunk_get_prev(tmp);
                  }
                  /* Add the newline */
                  newline_iarf(tmp, AV_ADD);
               }
            }
         }

         newlines_brace_pair(pc);
      }
      else if (pc->type == CT_BRACE_CLOSE)
      {
         if (cpd.settings[UO_nl_brace_brace].a != AV_IGNORE)
         {
            next = chunk_get_next_nc(pc, CNAV_PREPROC);
            if ((next != NULL) && (next->type == CT_BRACE_CLOSE))
            {
               newline_iarf_pair(pc, next, cpd.settings[UO_nl_brace_brace].a);
            }
         }

         if (cpd.settings[UO_nl_brace_square].a != AV_IGNORE)
         {
            next = chunk_get_next_nc(pc, CNAV_PREPROC);
            if ((next != NULL) && (next->type == CT_SQUARE_CLOSE))
            {
               newline_iarf_pair(pc, next, cpd.settings[UO_nl_brace_square].a);
            }
         }

         if (cpd.settings[UO_nl_brace_fparen].a != AV_IGNORE)
         {
            next = chunk_get_next_nc(pc, CNAV_PREPROC);
            if ((next != NULL) && (next->type == CT_FPAREN_CLOSE))
            {
               newline_iarf_pair(pc, next, cpd.settings[UO_nl_brace_fparen].a);
            }
         }

         if (cpd.settings[UO_eat_blanks_before_close_brace].b)
         {
            /* Limit the newlines before the close brace to 1 */
            prev = chunk_get_prev(pc);
            if (chunk_is_newline(prev))
            {
               if (prev->nl_count != 1)
               {
                  prev->nl_count = 1;
                  MARK_CHANGE();
               }
            }
         }
         else if (cpd.settings[UO_nl_ds_struct_enum_close_brace].b &&
                  ((pc->parent_type == CT_ENUM) ||
                   (pc->parent_type == CT_STRUCT) ||
                   (pc->parent_type == CT_UNION)))
         {
            if ((pc->flags & PCF_ONE_LINER) == 0)
            {
               /* Make sure the } is preceded by two newlines */
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

         /* Force a newline after a close brace */
         if ((cpd.settings[UO_nl_brace_struct_var].a != AV_IGNORE) &&
             ((pc->parent_type == CT_STRUCT) ||
              (pc->parent_type == CT_ENUM) ||
              (pc->parent_type == CT_UNION)))
         {
            next = chunk_get_next_ncnl(pc, CNAV_PREPROC);
            if (next &&
                (next->type != CT_SEMICOLON) &&
                (next->type != CT_COMMA))
            {
               newline_iarf(pc, cpd.settings[UO_nl_brace_struct_var].a);
            }
         }
         else if (cpd.settings[UO_nl_after_brace_close].b ||
                  (pc->parent_type == CT_FUNC_CLASS_DEF) ||
                  (pc->parent_type == CT_FUNC_DEF) ||
                  (pc->parent_type == CT_OC_MSG_DECL))
         {
            next = chunk_get_next(pc);
            if ((next != NULL) &&
                (next->type != CT_SEMICOLON) &&
                (next->type != CT_COMMA) &&
                (next->type != CT_SQUARE_CLOSE) &&
                (next->type != CT_FPAREN_CLOSE) &&
                ((pc->flags & (PCF_IN_ARRAY_ASSIGN | PCF_IN_TYPEDEF)) == 0) &&
                !chunk_is_newline(next) &&
                !chunk_is_comment(next))
            {
               newline_end_newline(pc);
            }
         }
      }
      else if (pc->type == CT_VBRACE_OPEN)
      {
         if (cpd.settings[UO_nl_after_vbrace_open].b ||
             cpd.settings[UO_nl_after_vbrace_open_empty].b)
         {
            next = chunk_get_next(pc, CNAV_PREPROC);
            bool add_it;
            if (chunk_is_semicolon(next))
            {
               add_it = cpd.settings[UO_nl_after_vbrace_open_empty].b;
            }
            else
            {
               add_it = (cpd.settings[UO_nl_after_vbrace_open].b &&
                         (next->type != CT_VBRACE_CLOSE) &&
                         !chunk_is_comment(next) &&
                         !chunk_is_newline(next));
            }
            if (add_it)
            {
               newline_iarf(pc, AV_ADD);
            }
         }

         if ((((pc->parent_type == CT_IF) ||
               (pc->parent_type == CT_ELSEIF) ||
               (pc->parent_type == CT_ELSE)) &&
              cpd.settings[UO_nl_create_if_one_liner].b) ||
             ((pc->parent_type == CT_FOR) &&
              cpd.settings[UO_nl_create_for_one_liner].b) ||
             ((pc->parent_type == CT_WHILE) &&
              cpd.settings[UO_nl_create_while_one_liner].b))
         {
            nl_create_one_liner(pc);
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
      else if ((pc->type == CT_SQUARE_OPEN) && (pc->parent_type == CT_OC_MSG))
      {
         if (cpd.settings[UO_nl_oc_msg_args].b)
         {
            newline_oc_msg(pc);
         }
      }
      else if (pc->type == CT_STRUCT)
      {
         newlines_struct_enum_union(pc, cpd.settings[UO_nl_struct_brace].a, true);
      }
      else if (pc->type == CT_UNION)
      {
         newlines_struct_enum_union(pc, cpd.settings[UO_nl_union_brace].a, true);
      }
      else if (pc->type == CT_ENUM)
      {
         newlines_struct_enum_union(pc, cpd.settings[UO_nl_enum_brace].a, true);
      }
      else if (pc->type == CT_CASE)
      {
         /* Note: 'default' also maps to CT_CASE */
         if (cpd.settings[UO_nl_before_case].b)
         {
            newline_case(pc);
         }
      }
      else if (pc->type == CT_THROW)
      {
         prev = chunk_get_prev(pc);
         if (prev && (prev->type == CT_PAREN_CLOSE))
         {
            newline_iarf(chunk_get_prev_ncnl(pc), cpd.settings[UO_nl_before_throw].a);
         }
      }
      else if (pc->type == CT_CASE_COLON)
      {
         next = chunk_get_next_nnl(pc);
         if (next && (next->type == CT_BRACE_OPEN) &&
             (cpd.settings[UO_nl_case_colon_brace].a != AV_IGNORE))
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
         if ((next != NULL) && (next->type == CT_BRACE_OPEN))
         {
            /* TODO:
             * this could be used to control newlines between the
             * the if/while/for/switch close paren and the open brace, but
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
         if (((pc->flags & (PCF_IN_SPAREN | PCF_IN_PREPROC)) == 0) &&
             cpd.settings[UO_nl_after_semicolon].b)
         {
            next = chunk_get_next(pc);
            while ((next != NULL) && (next->type == CT_VBRACE_CLOSE))
            {
               next = chunk_get_next(next);
            }
            if ((next != NULL) &&
                !chunk_is_comment(next) &&
                !chunk_is_newline(next))
            {
               if (one_liner_nl_ok(next))
               {
                  newline_iarf(pc, AV_ADD);
               }
            }
         }
         else if (pc->parent_type == CT_CLASS)
         {
            if (cpd.settings[UO_nl_after_class].n > 0)
            {
               newline_iarf(pc, AV_ADD);
            }
         }
      }
      else if (pc->type == CT_FPAREN_OPEN)
      {
         if (((pc->parent_type == CT_FUNC_DEF) ||
              (pc->parent_type == CT_FUNC_PROTO) ||
              (pc->parent_type == CT_FUNC_CLASS_DEF) ||
              (pc->parent_type == CT_FUNC_CLASS_PROTO) ||
              (pc->parent_type == CT_OPERATOR))
             &&
             ((cpd.settings[UO_nl_func_decl_start].a != AV_IGNORE) ||
              (cpd.settings[UO_nl_func_def_start].a != AV_IGNORE) ||
              (cpd.settings[UO_nl_func_decl_args].a != AV_IGNORE) ||
              (cpd.settings[UO_nl_func_def_args].a != AV_IGNORE) ||
              (cpd.settings[UO_nl_func_decl_end].a != AV_IGNORE) ||
              (cpd.settings[UO_nl_func_def_end].a != AV_IGNORE) ||
              (cpd.settings[UO_nl_func_decl_empty].a != AV_IGNORE) ||
              (cpd.settings[UO_nl_func_def_empty].a != AV_IGNORE) ||
              (cpd.settings[UO_nl_func_type_name].a != AV_IGNORE) ||
              (cpd.settings[UO_nl_func_type_name_class].a != AV_IGNORE) ||
              (cpd.settings[UO_nl_func_scope_name].a != AV_IGNORE) ||
              (cpd.settings[UO_nl_func_proto_type_name].a != AV_IGNORE) ||
              (cpd.settings[UO_nl_func_paren].a != AV_IGNORE) ||
              (cpd.settings[UO_nl_func_def_paren].a != AV_IGNORE)))
         {
            newline_func_def(pc);
         }
         else if (first && (cpd.settings[UO_nl_remove_extra_newlines].n == 1))
         {
            newline_iarf(pc, AV_REMOVE);
         }
      }
      else if (pc->type == CT_ANGLE_CLOSE)
      {
         if (pc->parent_type == CT_TEMPLATE)
         {
            next = chunk_get_next_ncnl(pc);
            if ((next != NULL) &&
                (next->level == next->brace_level))
            {
               tmp = chunk_get_prev_ncnl(chunk_get_prev_type(pc, CT_ANGLE_OPEN, pc->level));
               if ((tmp != NULL) && (tmp->type == CT_TEMPLATE))
               {
                  newline_iarf(pc, cpd.settings[UO_nl_template_class].a);
               }
            }
         }
      }
      else if (pc->type == CT_NAMESPACE)
      {
         newlines_struct_enum_union(pc, cpd.settings[UO_nl_namespace_brace].a, false);
      }
      else if (pc->type == CT_SQUARE_OPEN)
      {
         if ((pc->parent_type == CT_ASSIGN) &&
             ((pc->flags & PCF_ONE_LINER) == 0))
         {
            tmp = chunk_get_prev_ncnl(pc);
            newline_iarf(tmp, cpd.settings[UO_nl_assign_square].a);

            argval_t arg = cpd.settings[UO_nl_after_square_assign].a;

            if (cpd.settings[UO_nl_assign_square].a & AV_ADD)
            {
               arg = AV_ADD;
            }
            newline_iarf(pc, arg);

            /* if there is a newline after the open, then force a newline
             * before the close */
            tmp = chunk_get_next_nc(pc);
            if (chunk_is_newline(tmp))
            {
               tmp = chunk_get_next_type(pc, CT_SQUARE_CLOSE, pc->level);
               if (tmp != NULL)
               {
                  newline_add_before(tmp);
               }
            }
         }
      }
      else if (pc->type == CT_PRIVATE)
      {
         /** Make sure there is a newline before an access spec */
         if (cpd.settings[UO_nl_before_access_spec].n > 0)
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
         /** Make sure there is a newline after an access spec */
         if (cpd.settings[UO_nl_after_access_spec].n > 0)
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
      else if (first && (cpd.settings[UO_nl_remove_extra_newlines].n == 1) &&
               !(pc->flags & PCF_IN_PREPROC))
      {
         newline_iarf(pc, AV_REMOVE);
      }
      else
      {
         /* ignore it */
      }
   }
   newline_def_blk(chunk_get_head(), false);
}


/**
 * Find the next newline or nl_cont
 */
static void nl_handle_define(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *nl  = pc;
   chunk_t *ref = NULL;

   while ((nl = chunk_get_next(nl)) != NULL)
   {
      if (nl->type == CT_NEWLINE)
      {
         return;
      }
      if ((nl->type == CT_MACRO) ||
          ((nl->type == CT_FPAREN_CLOSE) && (nl->parent_type == CT_MACRO_FUNC)))
      {
         ref = nl;
      }
      if (nl->type == CT_NL_CONT)
      {
         if (ref != NULL)
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
   chunk_t *pc;
   chunk_t *tmp;

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next(pc))
   {
      if (pc->type != CT_COMMENT_MULTI)
      {
         continue;
      }

      tmp = pc;
      while (((tmp = chunk_get_next(tmp)) != NULL) && !chunk_is_newline(tmp))
      {
         if (!chunk_is_comment(tmp))
         {
            newline_add_before(tmp);
            break;
         }
      }
   }
}


/**
 * Handle insertion/removal of blank lines before if/for/while/do
 */
void newlines_insert_blank_lines(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next_ncnl(pc))
   {
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
      else if (pc->type == CT_DO)
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, cpd.settings[UO_nl_before_do].a);
         newlines_if_for_while_switch_post_blank_lines(pc, cpd.settings[UO_nl_after_do].a);
      }
      else
      {
         /* ignore it */
      }
   }
}


void newlines_squeeze_ifdef(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;
   chunk_t *ppr;
   chunk_t *pnl;
   chunk_t *nnl;
   chunk_t *tmp1;
   chunk_t *tmp2;

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next_ncnl(pc))
   {
      if ((pc->type == CT_PREPROC) && (pc->level > 0))
      {
         ppr = chunk_get_next(pc);

         if ((ppr->type == CT_PP_IF) ||
             (ppr->type == CT_PP_ELSE) ||
             (ppr->type == CT_PP_ENDIF))
         {
            pnl = NULL;
            nnl = chunk_get_next_nl(ppr);
            if (ppr->type == CT_PP_ENDIF)
            {
               pnl = chunk_get_prev_nl(pc);
            }

            if (nnl != NULL)
            {
               if (pnl != NULL)
               {
                  if (pnl->nl_count > 1)
                  {
                     //nnl->nl_count += pnl->nl_count - 1;
                     pnl->nl_count = 1;
                     MARK_CHANGE();

                     tmp1 = chunk_get_prev_nnl(pnl);
                     tmp2 = chunk_get_prev_nnl(nnl);

                     LOG_FMT(LNEWLINE, "%s: moved from after line %d to after %d\n",
                             __func__, tmp1->orig_line, tmp2->orig_line);
                  }
               }
               else
               {
                  if (nnl->nl_count > 1)
                  {
                     tmp1 = chunk_get_prev_nnl(nnl);
                     LOG_FMT(LNEWLINE, "%s: trimmed newlines after line %d from %d\n",
                             __func__, tmp1->orig_line, nnl->nl_count);
                     nnl->nl_count = 1;
                     MARK_CHANGE();
                  }
               }
            }
         }
      }
   }
}


void newlines_eat_start_end(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;

   /* Process newlines at the start of the file */
   if (((cpd.settings[UO_nl_start_of_file].a & AV_REMOVE) != 0) ||
       (((cpd.settings[UO_nl_start_of_file].a & AV_ADD) != 0) &&
        (cpd.settings[UO_nl_start_of_file_min].n > 0)))
   {
      pc = chunk_get_head();
      if (pc != NULL)
      {
         if (pc->type == CT_NEWLINE)
         {
            if (cpd.settings[UO_nl_start_of_file].a == AV_REMOVE)
            {
               chunk_del(pc);
               MARK_CHANGE();
            }
            else if ((cpd.settings[UO_nl_start_of_file].a == AV_FORCE) ||
                     (pc->nl_count < cpd.settings[UO_nl_start_of_file_min].n))
            {
               pc->nl_count = cpd.settings[UO_nl_start_of_file_min].n;
               MARK_CHANGE();
            }
         }
         else if (((cpd.settings[UO_nl_start_of_file].a & AV_ADD) != 0) &&
                  (cpd.settings[UO_nl_start_of_file_min].n > 0))
         {
            chunk_t chunk;
            chunk.orig_line = pc->orig_line;
            chunk.type      = CT_NEWLINE;
            chunk.nl_count  = cpd.settings[UO_nl_start_of_file_min].n;
            chunk_add_before(&chunk, pc);
            MARK_CHANGE();
         }
      }
   }

   /* Process newlines at the end of the file */
   if (((cpd.settings[UO_nl_end_of_file].a & AV_REMOVE) != 0) ||
       (((cpd.settings[UO_nl_end_of_file].a & AV_ADD) != 0) &&
        (cpd.settings[UO_nl_end_of_file_min].n > 0)))
   {
      pc = chunk_get_tail();
      if (pc != NULL)
      {
         if (pc->type == CT_NEWLINE)
         {
            if (cpd.settings[UO_nl_end_of_file].a == AV_REMOVE)
            {
               chunk_del(pc);
               MARK_CHANGE();
            }
            else if ((cpd.settings[UO_nl_end_of_file].a == AV_FORCE) ||
                     (pc->nl_count < cpd.settings[UO_nl_end_of_file_min].n))
            {
               if (pc->nl_count != cpd.settings[UO_nl_end_of_file_min].n)
               {
                  pc->nl_count = cpd.settings[UO_nl_end_of_file_min].n;
                  MARK_CHANGE();
               }
            }
         }
         else if (((cpd.settings[UO_nl_end_of_file].a & AV_ADD) != 0) &&
                  (cpd.settings[UO_nl_end_of_file_min].n > 0))
         {
            chunk_t chunk;
            chunk.orig_line = pc->orig_line;
            chunk.type      = CT_NEWLINE;
            chunk.nl_count  = cpd.settings[UO_nl_end_of_file_min].n;
            chunk_add(&chunk);
            MARK_CHANGE();
         }
      }
   }
}


/**
 * Searches for a chunk of type chunk_type and moves them, if needed.
 * Will not move tokens that are on their own line or have other than
 * exactly 1 newline before (UO_pos_comma == TRAIL) or after (UO_pos_comma == LEAD).
 * We can't remove a newline if it is right before a preprocessor.
 */
void newlines_chunk_pos(c_token_t chunk_type, tokenpos_e mode)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;
   chunk_t *next;
   chunk_t *prev;
   int     nl_flag;

   if ((mode & (TP_JOIN | TP_LEAD | TP_TRAIL)) == 0)
   {
      return;
   }

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next_ncnl(pc))
   {
      if (pc->type == chunk_type)
      {
         prev = chunk_get_prev_nc(pc);
         next = chunk_get_next_nc(pc);

         nl_flag = ((chunk_is_newline(prev) ? 1 : 0) |
                    (chunk_is_newline(next) ? 2 : 0));

         if (mode & TP_JOIN)
         {
            if (nl_flag & 1)
            {
               /* remove nl if not precededed by a comment */
               chunk_t *prev2 = chunk_get_prev(prev);

               if ((prev2 != NULL) && !(chunk_is_comment(prev2)))
               {
                  remove_next_newlines(prev2);
               }
            }
            if (nl_flag & 2)
            {
               /* remove nl if not followed by a comment */
               chunk_t *next2 = chunk_get_next(next);

               if ((next2 != NULL) && !(chunk_is_comment(next2)))
               {
                  remove_next_newlines(pc);
               }
            }
            continue;
         }

         if (((nl_flag == 0) && ((mode & (TP_FORCE | TP_BREAK)) == 0)) ||
             ((nl_flag == 3) && ((mode & TP_FORCE) == 0)))
         {
            /* No newlines and not adding any or both and not forcing */
            continue;
         }

         if (((mode & TP_LEAD) && (nl_flag == 1)) ||
             ((mode & TP_TRAIL) && (nl_flag == 2)))
         {
            /* Already a newline before (lead) or after (trail) */
            continue;
         }

         /* If there were no newlines, we need to add one */
         if (nl_flag == 0)
         {
            if (mode & TP_LEAD)
            {
               newline_add_before(pc);
            }
            else
            {
               newline_add_after(pc);
            }
            continue;
         }

         /* If there were both newlines, we need to remove one */
         if (nl_flag == 3)
         {
            if (mode & TP_LEAD)
            {
               remove_next_newlines(pc);
            }
            else
            {
               remove_next_newlines(chunk_get_prev_ncnl(pc));
            }
            continue;
         }

         /* we need to move the newline */
         if (mode & TP_LEAD)
         {
            chunk_t *next2 = chunk_get_next(next);
            if ((next2 != NULL) &&
                ((next2->type == CT_PREPROC) ||
                 ((chunk_type == CT_ASSIGN) &&
                  (next2->type == CT_BRACE_OPEN))))
            {
               continue;
            }
            if (next->nl_count == 1)
            {
               /* move the CT_BOOL to after the newline */
               chunk_move_after(pc, next);
            }
         }
         else
         {
            if (prev->nl_count == 1)
            {
               /* Back up to the next non-comment item */
               prev = chunk_get_prev_nc(prev);
               if ((prev != NULL) && !chunk_is_newline(prev) &&
                   !(prev->flags & PCF_IN_PREPROC))
               {
                  chunk_move_after(pc, prev);
               }
            }
         }
      }
   }
}


/**
 * Searches for CT_CLASS_COLON and moves them, if needed.
 * Also breaks up the args
 */
void newlines_class_colon_pos(c_token_t tok)
{
   LOG_FUNC_ENTRY();
   chunk_t    *pc;
   chunk_t    *next;
   chunk_t    *prev;
   chunk_t    *ccolon = NULL;
   tokenpos_e tpc, pcc;
   argval_t   anc, ncia;

   if (tok == CT_CLASS_COLON)
   {
      tpc = cpd.settings[UO_pos_class_colon].tp;
      anc = cpd.settings[UO_nl_class_colon].a;
      ncia = cpd.settings[UO_nl_class_init_args].a;
      pcc = cpd.settings[UO_pos_class_comma].tp;
   }
   else /* tok == CT_CONSTR_COLON */
   {
      tpc = cpd.settings[UO_pos_constr_colon].tp;
      anc = cpd.settings[UO_nl_constr_colon].a;
      ncia = cpd.settings[UO_nl_constr_init_args].a;
      pcc = cpd.settings[UO_pos_constr_comma].tp;
   }

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next_ncnl(pc))
   {
      if (!ccolon && (pc->type != tok))
      {
         continue;
      }

      if (pc->type == tok)
      {
         ccolon = pc;
         prev   = chunk_get_prev_nc(pc);
         next   = chunk_get_next_nc(pc);

         if (!chunk_is_newline(prev) && !chunk_is_newline(next) &&
             ((anc & AV_ADD) != 0))
         {
            newline_add_after(pc);
            prev = chunk_get_prev_nc(pc);
            next = chunk_get_next_nc(pc);
         }

         if (anc == AV_REMOVE)
         {
            if (chunk_is_newline(prev) &&
                chunk_safe_to_del_nl(prev))
            {
               chunk_del(prev);
               MARK_CHANGE();
               prev = chunk_get_prev_nc(pc);
            }
            if (chunk_is_newline(next) &&
                chunk_safe_to_del_nl(next))
            {
               chunk_del(next);
               MARK_CHANGE();
               next = chunk_get_next_nc(pc);
            }
         }

         if (tpc & TP_TRAIL)
         {
            if (chunk_is_newline(prev) && (prev->nl_count == 1) &&
                chunk_safe_to_del_nl(prev))
            {
               chunk_swap(pc, prev);
            }
         }
         else if (tpc & TP_LEAD)
         {
            if (chunk_is_newline(next) && (next->nl_count == 1) &&
                chunk_safe_to_del_nl(next))
            {
               chunk_swap(pc, next);
            }
         }
      }
      else
      {
         if ((pc->type == CT_BRACE_OPEN) || (pc->type == CT_SEMICOLON))
         {
            ccolon = NULL;
            continue;
         }

         if ((pc->type == CT_COMMA) && (pc->level == ccolon->level))
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
}


static void _blank_line_set(chunk_t *pc, const char *text, uncrustify_options uo)
{
   LOG_FUNC_ENTRY();
   if ((cpd.settings[uo].n > 0) && (pc->nl_count != cpd.settings[uo].n))
   {
      LOG_FMT(LBLANKD, "do_blank_lines: %s set line %d\n", text + 3, pc->orig_line);
      pc->nl_count = cpd.settings[uo].n;
      MARK_CHANGE();
   }
}


#define blank_line_set(pc, op)    _blank_line_set(pc, # op, op)


static void _blank_line_max(chunk_t *pc, const char *text, uncrustify_options uo)
{
   LOG_FUNC_ENTRY();
   if ((cpd.settings[uo].n > 0) && (pc->nl_count > cpd.settings[uo].n))
   {
      LOG_FMT(LBLANKD, "do_blank_lines: %s max line %d\n", text + 3, pc->orig_line);
      pc->nl_count = cpd.settings[uo].n;
      MARK_CHANGE();
   }
}


#define blank_line_max(pc, op)    _blank_line_max(pc, # op, op)


/**
 * Scans for newline tokens and changes the nl_count.
 * A newline token has a minimum nl_count of 1.
 * Note that a blank line is actually 2 newlines, unless the newline is the
 * first chunk.  But we don't handle the first chunk.
 * So, most comparisons have +1 below.
 */
void do_blank_lines(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;
   chunk_t *next;
   chunk_t *prev;
   chunk_t *pcmt;
   int     old_nl;

   /* Don't process the first token, as we don't care if it is a newline */
   pc = chunk_get_head();

   while ((pc = chunk_get_next(pc)) != NULL)
   {
      if (pc->type != CT_NEWLINE)
      {
         continue;
      }

      next = chunk_get_next(pc);
      prev = chunk_get_prev_nc(pc);
      pcmt = chunk_get_prev(pc);

      old_nl = pc->nl_count;
      if ((next != NULL) && (prev != NULL))
      {
         LOG_FMT(LBLANK, "%s: line %d [%s][%s] vs [%s][%s] nl=%d\n", __func__,
                 pc->orig_line,
                 prev->str.c_str(), get_token_name(prev->type),
                 next->str.c_str(), get_token_name(next->type),
                 pc->nl_count);
      }

      /* Limit consecutive newlines */
      if ((cpd.settings[UO_nl_max].n > 0) &&
          (pc->nl_count > cpd.settings[UO_nl_max].n))
      {
         blank_line_max(pc, UO_nl_max);
      }

      if (!can_increase_nl(pc))
      {
         LOG_FMT(LBLANKD, "do_blank_lines: force to 1 line %d\n", pc->orig_line);
         if (pc->nl_count != 1)
         {
            pc->nl_count = 1;
            MARK_CHANGE();
         }
         continue;
      }

      /** Control blanks before multi-line comments */
      if ((cpd.settings[UO_nl_before_block_comment].n > pc->nl_count) &&
          (next != NULL) &&
          (next->type == CT_COMMENT_MULTI))
      {
         /* Don't add blanks after a open brace */
         if ((prev == NULL) ||
             ((prev->type != CT_BRACE_OPEN) &&
              (prev->type != CT_VBRACE_OPEN)))
         {
            blank_line_set(pc, UO_nl_before_block_comment);
         }
      }

      /** Control blanks before single line C comments */
      if ((cpd.settings[UO_nl_before_c_comment].n > pc->nl_count) &&
          (next != NULL) &&
          (next->type == CT_COMMENT))
      {
         /* Don't add blanks after a open brace or a comment */
         if ((prev == NULL) ||
             ((prev->type != CT_BRACE_OPEN) &&
              (prev->type != CT_VBRACE_OPEN) &&
              (pcmt->type != CT_COMMENT)))
         {
            blank_line_set(pc, UO_nl_before_c_comment);
         }
      }

      /** Control blanks before CPP comments */
      if ((cpd.settings[UO_nl_before_cpp_comment].n > pc->nl_count) &&
          (next != NULL) &&
          (next->type == CT_COMMENT_CPP))
      {
         /* Don't add blanks after a open brace */
         if ((prev == NULL) ||
             ((prev->type != CT_BRACE_OPEN) &&
              (prev->type != CT_VBRACE_OPEN) &&
              (pcmt->type != CT_COMMENT_CPP)))
         {
            blank_line_set(pc, UO_nl_before_cpp_comment);
         }
      }

      /** Control blanks before an access spec */
      if ((cpd.settings[UO_nl_before_access_spec].n > 0) &&
          (cpd.settings[UO_nl_before_access_spec].n != pc->nl_count) &&
          (next != NULL) &&
          (next->type == CT_PRIVATE))
      {
         /* Don't add blanks after a open brace */
         if ((prev == NULL) ||
             ((prev->type != CT_BRACE_OPEN) &&
              (prev->type != CT_VBRACE_OPEN)))
         {
            blank_line_set(pc, UO_nl_before_access_spec);
         }
      }

      /** Control blanks after an access spec */
      if ((cpd.settings[UO_nl_after_access_spec].n > 0) &&
          (cpd.settings[UO_nl_after_access_spec].n != pc->nl_count) &&
          (prev != NULL) &&
          (prev->type == CT_PRIVATE_COLON))
      {
         blank_line_set(pc, UO_nl_after_access_spec);
      }

      /* Add blanks after function bodies */
      if ((prev != NULL) && (prev->type == CT_BRACE_CLOSE) &&
          ((prev->parent_type == CT_FUNC_DEF) ||
           (prev->parent_type == CT_FUNC_CLASS_DEF) ||
           (prev->parent_type == CT_OC_MSG_DECL) ||
           (prev->parent_type == CT_ASSIGN)))
      {
         if (prev->flags & PCF_ONE_LINER)
         {
            if (cpd.settings[UO_nl_after_func_body_one_liner].n > pc->nl_count)
            {
               blank_line_set(pc, UO_nl_after_func_body_one_liner);
            }
         }
         else
         {
            if ((prev->flags & PCF_IN_CLASS) &&
                (cpd.settings[UO_nl_after_func_body_class].n > 0))
            {
               if (cpd.settings[UO_nl_after_func_body_class].n != pc->nl_count)
               {
                  blank_line_set(pc, UO_nl_after_func_body_class);
               }
            }
            else if (cpd.settings[UO_nl_after_func_body].n > 0)
            {
               if (cpd.settings[UO_nl_after_func_body].n != pc->nl_count)
               {
                  blank_line_set(pc, UO_nl_after_func_body);
               }
            }
         }
      }

      /* Add blanks after function prototypes */
      if ((prev != NULL) &&
          (prev->type == CT_SEMICOLON) &&
          (prev->parent_type == CT_FUNC_PROTO))
      {
         if (cpd.settings[UO_nl_after_func_proto].n > pc->nl_count)
         {
            pc->nl_count = cpd.settings[UO_nl_after_func_proto].n;
            MARK_CHANGE();
         }
         if ((cpd.settings[UO_nl_after_func_proto_group].n > pc->nl_count) &&
             (next != NULL) &&
             (next->parent_type != CT_FUNC_PROTO))
         {
            blank_line_set(pc, UO_nl_after_func_proto_group);
         }
      }

      /* Add blanks after struct/enum/union/class */
      if ((prev != NULL) &&
          ((prev->type == CT_SEMICOLON) ||
           (prev->type == CT_BRACE_CLOSE)) &&
          ((prev->parent_type == CT_STRUCT) ||
           (prev->parent_type == CT_ENUM) ||
           (prev->parent_type == CT_UNION) ||
           (prev->parent_type == CT_CLASS)))
      {
         if (prev->parent_type == CT_CLASS)
         {
            if (cpd.settings[UO_nl_after_class].n > pc->nl_count)
            {
               blank_line_set(pc, UO_nl_after_class);
            }
         }
         else
         {
            if (cpd.settings[UO_nl_after_struct].n > pc->nl_count)
            {
               blank_line_set(pc, UO_nl_after_struct);
            }
         }
      }

      /* Change blanks between a function comment and body */
      if ((cpd.settings[UO_nl_comment_func_def].n != 0) &&
          (pcmt->type == CT_COMMENT_MULTI) &&
          (pcmt->parent_type == CT_COMMENT_WHOLE) &&
          (next != NULL) && ((next->parent_type == CT_FUNC_DEF) ||
                             (next->parent_type == CT_FUNC_CLASS_DEF)))
      {
         if (cpd.settings[UO_nl_comment_func_def].n != pc->nl_count)
         {
            blank_line_set(pc, UO_nl_comment_func_def);
         }
      }

      /* Change blanks after a try-catch-finally block */
      if ((cpd.settings[UO_nl_after_try_catch_finally].n != 0) &&
          (cpd.settings[UO_nl_after_try_catch_finally].n != pc->nl_count) &&
          (prev != NULL) && (next != NULL))
      {
         if ((prev->type == CT_BRACE_CLOSE) &&
             ((prev->parent_type == CT_CATCH) ||
              (prev->parent_type == CT_FINALLY)))
         {
            if ((next->type != CT_BRACE_CLOSE) &&
                (next->type != CT_CATCH) &&
                (next->type != CT_FINALLY))
            {
               blank_line_set(pc, UO_nl_after_try_catch_finally);
            }
         }
      }

      /* Change blanks after a try-catch-finally block */
      if ((cpd.settings[UO_nl_between_get_set].n != 0) &&
          (cpd.settings[UO_nl_between_get_set].n != pc->nl_count) &&
          (prev != NULL) && (next != NULL))
      {
         if ((prev->parent_type == CT_GETSET) &&
             (next->type != CT_BRACE_CLOSE) &&
             ((prev->type == CT_BRACE_CLOSE) ||
              (prev->type == CT_SEMICOLON)))
         {
            blank_line_set(pc, UO_nl_between_get_set);
         }
      }

      /* Change blanks after a try-catch-finally block */
      if ((cpd.settings[UO_nl_around_cs_property].n != 0) &&
          (cpd.settings[UO_nl_around_cs_property].n != pc->nl_count) &&
          (prev != NULL) && (next != NULL))
      {
         if ((prev->type == CT_BRACE_CLOSE) &&
             (prev->parent_type == CT_CS_PROPERTY) &&
             (next->type != CT_BRACE_CLOSE))
         {
            blank_line_set(pc, UO_nl_around_cs_property);
         }
         else if ((next->parent_type == CT_CS_PROPERTY) &&
                  (next->flags & PCF_STMT_START))
         {
            blank_line_set(pc, UO_nl_around_cs_property);
         }
      }

      if (old_nl != pc->nl_count)
      {
         LOG_FMT(LBLANK, "   -=> changed to %d\n", pc->nl_count);
      }
   }
}


void newlines_cleanup_dup(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;
   chunk_t *next;

   pc   = chunk_get_head();
   next = pc;

   while (pc != NULL)
   {
      next = chunk_get_next(next);
      if ((next != NULL) &&
          (pc->type == CT_NEWLINE) &&
          (next->type == CT_NEWLINE))
      {
         next->nl_count = max(pc->nl_count, next->nl_count);
         chunk_del(pc);
         MARK_CHANGE();
      }
      pc = next;
   }
}


/**
 * Make sure there is a blank line after a commented group of values
 */
static void newlines_double_space_struct_enum_union(chunk_t *open_brace)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc = open_brace;
   chunk_t *prev;

   while (((pc = chunk_get_next_nc(pc)) != NULL) &&
          (pc->level > open_brace->level))
   {
      if ((pc->level != (open_brace->level + 1)) ||
          (pc->type != CT_NEWLINE))
      {
         continue;
      }

      /* If the newline is NOT after a comment or a brace open and
       * it is before a comment, then make sure that the newline is
       * at least doubled
       */
      prev = chunk_get_prev(pc);
      if (!chunk_is_comment(prev) &&
          (prev->type != CT_BRACE_OPEN) &&
          chunk_is_comment(chunk_get_next(pc)))
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
   chunk_t *pc;
   chunk_t *next;
   chunk_t *ae;   /* last token of the annotation */

   pc = chunk_get_head();
   while (((pc = chunk_get_next_type(pc, CT_ANNOTATION, -1)) != NULL) &&
          ((next = chunk_get_next_nnl(pc)) != NULL))
   {
      /* find the end of this annotation */
      if (chunk_is_paren_open(next))
      {
         /* TODO: control newline between annotation and '(' ? */
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

      LOG_FMT(LANNOT, "%s: %d:%d annotation '%s' end@%d:%d '%s'",
              __func__, pc->orig_line, pc->orig_col, pc->text(),
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
}
