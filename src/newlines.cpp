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
#include <cerrno>
#include "unc_ctype.h"


static void newlines_double_space_struct_enum_union(chunk_t *open_brace);
static bool one_liner_nl_ok(chunk_t *pc);
static void nl_handle_define(chunk_t *pc);
static void newline_iarf_pair(chunk_t *before, chunk_t *after, argval_t av);

/**
 * Check to see if we are allowed to increase the newline count.
 * The only case where we can't is if nl_squeeze_ifdef=true and a preproc is
 * after the newline.
 */
static bool can_increase_nl(chunk_t *nl)
{
   if (cpd.settings[UO_nl_squeeze_ifdef].b)
   {
      chunk_t *pc = chunk_get_next(nl);

      if ((pc != NULL) && (pc->type == CT_PREPROC) &&
          (pc->parent_type == CT_PP_ENDIF))
      {
         return(false);
      }
   }
   return(true);
}


/**
 * Double the newline, if allowed.
 */
static void double_newline(chunk_t *nl)
{
   chunk_t *prev = chunk_get_prev(nl);

   LOG_FMT(LNEWLINE, "%s: add newline after %.*s on line %d",
           __func__, prev->len, prev->str, prev->orig_line);

   if (!can_increase_nl(nl))
   {
      LOG_FMT(LNEWLINE, " - denied\n");
      return;
   }
   LOG_FMT(LNEWLINE, " - done\n");
   nl->nl_count = 2;
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
static void setup_newline_add(const chunk_t *prev, chunk_t *nl, const chunk_t *next)
{
   memset(nl, 0, sizeof(*nl));
   nl->nl_count = 1;
   nl->flags    = (prev->flags & PCF_COPY_FLAGS) & ~PCF_IN_PREPROC;
   if ((prev->flags & PCF_IN_PREPROC) && (next->flags & PCF_IN_PREPROC))
   {
      nl->flags |= PCF_IN_PREPROC;
   }
   if ((nl->flags & PCF_IN_PREPROC) != 0)
   {
      nl->type = CT_NL_CONT;
      nl->str  = "\\\n";
      nl->len  = 2;
   }
   else
   {
      nl->type = CT_NEWLINE;
      nl->str  = "\n";
      nl->len  = 1;
   }
}


/**
 * 2 parts:
 *  - if/switch/while/for after braces
 *  - else before/after braces
 *  - do after braces
 *  - do/while before braces
 */
chunk_t *newline_add_before2(chunk_t *pc, const char *fcn, int line)
{
   chunk_t nl;
   chunk_t *prev;

   prev = chunk_get_prev(pc);
   if (chunk_is_newline(prev))
   {
      /* Already has a newline before this chunk */
      return(prev);
   }

   LOG_FMT(LNEWLINE, "%s: '%.*s' on line %d (called from %s line %d)\n",
           __func__, pc->len, pc->str, pc->orig_line, fcn, line);

   setup_newline_add(prev, &nl, pc);

   return(chunk_add_before(&nl, pc));
}


chunk_t *newline_add_after2(chunk_t *pc, const char *fcn, int line)
{
   chunk_t nl;
   chunk_t *next;

   next = chunk_get_next(pc);
   if (chunk_is_newline(next))
   {
      /* Already has a newline after this chunk */
      return(next);
   }

   LOG_FMT(LNEWLINE, "%s: '%.*s' on line %d (called from %s line %d)\n",
           __func__, pc->len, pc->str, pc->orig_line, fcn, line);

   setup_newline_add(pc, &nl, next);

   return(chunk_add_after(&nl, pc));
}


#define newline_min_after(ref, cnt)    newline_min_after2(ref, cnt, __func__, __LINE__)

static void newline_min_after2(chunk_t *ref, INT32 count,
                               const char *func, int line)
{
   chunk_t *pc = ref;
   chunk_t *next;

   LOG_FMT(LNEWLINE, "%s: '%.*s' line %d - count %d : caller=%s:%d\n",
           __func__, ref->len, ref->str, ref->orig_line, count, func, line);

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
      newline_min_after(next, count);
      return;
   }
   else
   {
      if (chunk_is_newline(pc) && can_increase_nl(pc))
      {
         if (pc->nl_count < count)
         {
            pc->nl_count = count;
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
chunk_t *newline_add_between2(chunk_t *start, chunk_t *end,
                              const char *func, int line)
{
   chunk_t *pc;

   if ((start == NULL) || (end == NULL))
   {
      return(NULL);
   }

   LOG_FMT(LNEWLINE, "%s: '%.*s'[%s] line %d:%d and '%.*s' line %d:%d : caller=%s:%d\n",
           __func__, start->len, start->str, get_token_name(start->type),
           start->orig_line, start->orig_col,
           end->len, end->str, end->orig_line, end->orig_col, func, line);

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
void newline_del_between2(chunk_t *start, chunk_t *end,
                          const char *func, int line)
{
   chunk_t *next;
   chunk_t *prev;
   chunk_t *pc = start;

   LOG_FMT(LNEWLINE, "%s: '%.*s' line %d:%d and '%.*s' line %d:%d : caller=%s:%d preproc=%d/%d\n",
           __func__, start->len, start->str, start->orig_line, start->orig_col,
           end->len, end->str, end->orig_line, end->orig_col, func, line,
           ((start->flags & PCF_IN_PREPROC) != 0),
           ((end->flags & PCF_IN_PREPROC) != 0));

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
               align_to_column(next, prev->column + space_col_align(prev, next));
            }
         }
         else
         {
            if (pc->nl_count > 1)
            {
               pc->nl_count = 1;
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
 */
static void newlines_if_for_while_switch(chunk_t *start, argval_t nl_opt)
{
   chunk_t *pc;
   chunk_t *close_paren;
   chunk_t *brace_open;

   if ((nl_opt == AV_IGNORE) ||
       (((start->flags & PCF_IN_PREPROC) != 0) &&
        !cpd.settings[UO_nl_define_macro].b))
   {
      return;
   }

   pc = chunk_get_next_ncnl(start);
   if ((pc != NULL) && (pc->type == CT_SPAREN_OPEN))
   {
      close_paren = chunk_get_next_type(pc, CT_SPAREN_CLOSE, pc->level);
      brace_open  = chunk_get_next_ncnl(close_paren);

      if ((brace_open != NULL) && (brace_open->type == CT_BRACE_OPEN) &&
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

         newline_iarf_pair(close_paren, brace_open, nl_opt);

         newline_add_between(brace_open, chunk_get_next_ncnl(brace_open));

         /* Make sure nothing is cuddled with the closing brace */
         pc = chunk_get_next_type(brace_open, CT_BRACE_CLOSE, brace_open->level);
         newline_add_between(pc, chunk_get_next_nblank(pc));
      }
   }
}


/**
 * Add or remove extra newline before the chunk.
 * Adds before comments
 * Doesn't do anything if open brace before it
 * "code\n\ncomment\nif (...)" or "code\ncomment\nif (...)"
 */
static void newlines_if_for_while_switch_pre_blank_lines(chunk_t *start, argval_t nl_opt)
{
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
            if (nl_opt & AV_REMOVE)
            {
               /* if we're also adding, take care of that here */
               pc->nl_count = do_add ? 2 : 1;
               /* can keep using pc because anything other than newline stops loop, and we delete if newline */
               while (chunk_is_newline(prev = chunk_get_prev_nvb(pc)))
               {
                  /* Make sure we don't combine a preproc and non-preproc */
                  if (!chunk_safe_to_del_nl(prev))
                  {
                     break;
                  }
                  chunk_del(prev);
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
   chunk_t *next;

   while ((next = chunk_get_next(start)) != NULL)
   {
      if (chunk_is_newline(next) && chunk_safe_to_del_nl(next))
      {
         chunk_del(next);
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
         prev->nl_count = 1;
         remove_next_newlines(pc);
      }
      else if (chunk_is_newline(next = chunk_get_next_nvb(pc))) /* otherwise just deal with nls after brace */
      {
         next->nl_count = 1;
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
               //LOG_FMT(LSYS, "  -- pc1=%.*s [%s]\n", pc->len, pc->str, get_token_name(pc->type));

               pc = chunk_get_next(pc);
               //LOG_FMT(LSYS, "  -- pc2=%.*s [%s]\n", pc->len, pc->str, get_token_name(pc->type));
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
   chunk_t *next;

   if ((nl_opt == AV_IGNORE) ||
       (((start->flags & PCF_IN_PREPROC) != 0) &&
        !cpd.settings[UO_nl_define_macro].b))
   {
      return;
   }

   next = chunk_get_next_ncnl(start);
   if ((next != NULL) && (next->type == CT_BRACE_OPEN))
   {
      if (!one_liner_nl_ok(next))
      {
         return;
      }
      newline_iarf_pair(start, next, nl_opt);
      if ((nl_opt & AV_ADD) != 0)
      {
         chunk_t *tmp = chunk_get_next_nc(next);
         if ((tmp != NULL) && !chunk_is_newline(tmp))
         {
            newline_add_between(next, tmp);
         }
      }
   }
}


/**
 * We are at the open brace for a function body.
 * put a newline after the block of variable definitions
 */
static void newline_fnc_var_def(chunk_t *br_open, int nl_count)
{
   chunk_t *prev = NULL;
   chunk_t *next;
   chunk_t *pc;

   pc = chunk_get_next_ncnl(br_open);
   while (pc != NULL)
   {
      if (chunk_is_type(pc) || (pc->type == CT_QUALIFIER) || (pc->type == CT_DC_MEMBER))
      {
         pc = chunk_get_next_ncnl(pc);
         continue;
      }

      if (((pc->type == CT_WORD) || (pc->type == CT_FUNC_CTOR_VAR)) &&
          ((pc->flags & PCF_VAR_1ST) != 0))
      {
         pc   = chunk_get_next_type(pc, CT_SEMICOLON, pc->level);
         prev = pc;
         pc   = chunk_get_next_ncnl(pc);
         continue;
      }

      break;
   }

   /* prev is either NULL or points to a semicolon */
   if (prev != NULL)
   {
      next = chunk_get_next_ncnl(prev);
      if ((next != NULL) && (next->type != CT_BRACE_CLOSE))
      {
         newline_min_after(prev, 1 + cpd.settings[UO_nl_func_var_def_blk].n);
      }
   }
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
         }
      }
   }

   /* Handle the cases where the brace is part of a function call or definition */
   if ((br_open->parent_type == CT_FUNC_DEF) ||
       (br_open->parent_type == CT_FUNC_CALL) ||
       (br_open->parent_type == CT_FUNC_CALL_USER) ||
       (br_open->parent_type == CT_FUNC_CLASS) ||
       (br_open->parent_type == CT_OC_MSG_DECL))
   {
      /* Need to force a newline before the close brace, if not in a class body */
      if ((br_open->flags & PCF_IN_CLASS) == 0)
      {
         nl_close_brace = true;
      }

      /* handle newlines after the open brace */
      pc = chunk_get_next_ncnl(br_open);
      newline_add_between(br_open, pc);

      val = ((br_open->parent_type == CT_FUNC_DEF) ||
             (br_open->parent_type == CT_FUNC_CLASS) ||
             (br_open->parent_type == CT_OC_MSG_DECL)) ?
            cpd.settings[UO_nl_fdef_brace].a :
            cpd.settings[UO_nl_fcall_brace].a;

      if (val != AV_IGNORE)
      {
         /* Grab the chunk before the open brace */
         prev = chunk_get_prev_ncnl(br_open);

         newline_iarf_pair(prev, br_open, val);
      }

      if (cpd.settings[UO_nl_func_var_def_blk].n > 0)
      {
         newline_fnc_var_def(br_open, cpd.settings[UO_nl_func_var_def_blk].n);
      }
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
 * Put a empty line after a return statement, unless it is followed by a
 * close brace.
 *
 * May not work with PAWN
 */
static void newline_return(chunk_t *start)
{
   chunk_t *pc;
   chunk_t *semi;
   chunk_t *after;

   semi  = chunk_get_next_type(start, CT_SEMICOLON, start->level);
   after = chunk_get_next_nblank(semi);

   /* If we hit a brace or an 'else', then a newline isn't needed */
   if ((after == NULL) ||
       (after->type == CT_BRACE_CLOSE) ||
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
   if ((before != NULL) && (after != NULL))
   {
      if ((av & AV_REMOVE) != 0)
      {
         newline_del_between(before, after);
      }

      if ((av & AV_ADD) != 0)
      {
         newline_add_between(before, after);
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
   newline_iarf_pair(pc, chunk_get_next_nnl(pc), av);
}


/**
 * Formats a function declaration
 * Start points to the open paren
 */
static void newline_func_def(chunk_t *start)
{
   chunk_t *pc;
   chunk_t *prev = NULL;
   chunk_t *tmp;

   if (cpd.settings[UO_nl_func_paren].a != AV_IGNORE)
   {
      prev = chunk_get_prev_ncnl(start);
      if (prev != NULL)
      {
         newline_iarf(prev, cpd.settings[UO_nl_func_paren].a);
      }
   }

   /* Handle break newlines type and function */
   prev = chunk_get_prev_ncnl(start);
   /* Don't split up a function variable */
   prev = (prev->type == CT_PAREN_CLOSE) ? NULL : chunk_get_prev_ncnl(prev);

   if (prev != NULL)
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
             (prev->type != CT_BRACE_OPEN) &&
             (prev->type != CT_SEMICOLON))
         {
            newline_iarf(prev, a);
         }
      }
   }

   pc = chunk_get_next_ncnl(start);
   if (chunk_is_str(pc, ")", 1))
   {
      if (cpd.settings[UO_nl_func_decl_empty].a != AV_IGNORE)
      {
         newline_iarf(start, cpd.settings[UO_nl_func_decl_empty].a);
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
         newline_iarf(pc, cpd.settings[UO_nl_func_decl_args].a);
      }
   }

   argval_t as = cpd.settings[UO_nl_func_decl_start].a;
   argval_t ae = cpd.settings[UO_nl_func_decl_end].a;
   if (comma_count == 0)
   {
      if (cpd.settings[UO_nl_func_decl_start_single].a != AV_IGNORE)
      {
         as = cpd.settings[UO_nl_func_decl_start_single].a;
      }
      if (cpd.settings[UO_nl_func_decl_end_single].a != AV_IGNORE)
      {
         ae = cpd.settings[UO_nl_func_decl_end_single].a;
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
 * Checks to see if it is OK to add a newline around the chunk.
 * Don't want to break one-liners...
 */
static bool one_liner_nl_ok(chunk_t *pc)
{
   LOG_FMT(LNL1LINE, "%s: check [%s] parent=[%s] flg=%x, on line %d, col %d - ",
           __func__, get_token_name(pc->type), get_token_name(pc->parent_type),
           pc->flags, pc->orig_line, pc->orig_col);

   if ((pc->flags & PCF_ONE_LINER) &&
       ((pc->type == CT_BRACE_OPEN) ||
        (pc->type == CT_BRACE_CLOSE)))
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
          (pc->parent_type == CT_FUNC_DEF))
      {
         LOG_FMT(LNL1LINE, "false (func def)\n");
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


static void nl_create_one_liner(chunk_t *vbrace_open)
{
   chunk_t *tmp;
   chunk_t *first;

   /* See if we get a newline between the next text and the vbrace_close */
   tmp   = chunk_get_next_ncnl(vbrace_open);
   first = tmp;
   while ((tmp != NULL) && (tmp->type != CT_VBRACE_CLOSE))
   {
      if (chunk_is_newline(tmp))
      {
         return;
      }
      tmp = chunk_get_next(tmp);
   }

   if (tmp != NULL)
   {
      newline_del_between(vbrace_open, first);
   }
}


/**
 * Step through all chunks.
 */
void newlines_cleanup_braces(void)
{
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
            if ((pc->level == pc->brace_level) &&
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

         if (cpd.settings[UO_eat_blanks_before_close_brace].b)
         {
            /* Limit the newlines before the close brace to 1 */
            prev = chunk_get_prev(pc);
            if (chunk_is_newline(prev))
            {
               prev->nl_count = 1;
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

         /* Force a newline after a function def */
         if (cpd.settings[UO_nl_after_brace_close].b ||
             (pc->parent_type == CT_FUNC_DEF) ||
             (pc->parent_type == CT_OC_MSG_DECL))
         {
            next = chunk_get_next(pc);
            if ((next != NULL) &&
                (next->type != CT_SEMICOLON) &&
                (next->type != CT_COMMA) &&
                ((pc->flags & (PCF_IN_ARRAY_ASSIGN | PCF_IN_TYPEDEF)) == 0) &&
                !chunk_is_newline(next) &&
                !chunk_is_comment(next))
            {
               newline_iarf(pc, AV_ADD);
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
         newline_iarf(chunk_get_prev_ncnl(pc), cpd.settings[UO_nl_before_throw].a);
      }
      else if (pc->type == CT_CASE_COLON)
      {
         if (cpd.settings[UO_nl_after_case].b)
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
         if (cpd.settings[UO_nl_after_return].b)
         {
            newline_return(pc);
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
      }
      else if (pc->type == CT_FPAREN_OPEN)
      {
         if (((pc->parent_type == CT_FUNC_DEF) ||
              (pc->parent_type == CT_FUNC_PROTO) ||
              (pc->parent_type == CT_FUNC_CLASS) ||
              (pc->parent_type == CT_OPERATOR))
             &&
             ((cpd.settings[UO_nl_func_decl_start].a != AV_IGNORE) ||
              (cpd.settings[UO_nl_func_decl_args].a != AV_IGNORE) ||
              (cpd.settings[UO_nl_func_decl_end].a != AV_IGNORE) ||
              (cpd.settings[UO_nl_func_decl_empty].a != AV_IGNORE) ||
              (cpd.settings[UO_nl_func_type_name].a != AV_IGNORE) ||
              (cpd.settings[UO_nl_func_type_name_class].a != AV_IGNORE) ||
              (cpd.settings[UO_nl_func_scope_name].a != AV_IGNORE) ||
              (cpd.settings[UO_nl_func_proto_type_name].a != AV_IGNORE) ||
              (cpd.settings[UO_nl_func_paren].a != AV_IGNORE)))
         {
            newline_func_def(pc);
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
      else
      {
         /* ignore it */
      }
   }
}


/**
 * Find the next newline or nl_cont
 */
static void nl_handle_define(chunk_t *pc)
{
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
                     nnl->nl_count += pnl->nl_count - 1;
                     pnl->nl_count  = 1;

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
                  }
               }
            }
         }
      }
   }
}


void newlines_eat_start_end(void)
{
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
            }
            else if ((cpd.settings[UO_nl_start_of_file].a == AV_FORCE) ||
                     (pc->nl_count < cpd.settings[UO_nl_start_of_file_min].n))
            {
               pc->nl_count = cpd.settings[UO_nl_start_of_file_min].n;
            }
         }
         else if (((cpd.settings[UO_nl_start_of_file].a & AV_ADD) != 0) &&
                  (cpd.settings[UO_nl_start_of_file_min].n > 0))
         {
            chunk_t chunk;
            memset(&chunk, 0, sizeof(chunk));
            chunk.orig_line = pc->orig_line;
            chunk.type      = CT_NEWLINE;
            chunk.nl_count  = cpd.settings[UO_nl_start_of_file_min].n;
            chunk_add_before(&chunk, pc);
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
            }
            else if ((cpd.settings[UO_nl_end_of_file].a == AV_FORCE) ||
                     (pc->nl_count < cpd.settings[UO_nl_end_of_file_min].n))
            {
               pc->nl_count = cpd.settings[UO_nl_end_of_file_min].n;
            }
         }
         else if (((cpd.settings[UO_nl_end_of_file].a & AV_ADD) != 0) &&
                  (cpd.settings[UO_nl_end_of_file_min].n > 0))
         {
            chunk_t chunk;
            memset(&chunk, 0, sizeof(chunk));
            chunk.orig_line = pc->orig_line;
            chunk.type      = CT_NEWLINE;
            chunk.nl_count  = cpd.settings[UO_nl_end_of_file_min].n;
            chunk_add(&chunk);
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
   chunk_t *pc;
   chunk_t *next;
   chunk_t *prev;

   if (mode == TP_IGNORE)
   {
      return;
   }

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next_ncnl(pc))
   {
      if (pc->type == chunk_type)
      {
         prev = chunk_get_prev_nc(pc);
         next = chunk_get_next_nc(pc);

         /* if both are newlines or neither are newlines, skip this chunk */
         if (chunk_is_newline(prev) == chunk_is_newline(next))
         {
            continue;
         }

         if (chunk_is_newline(next))
         {
            chunk_t *next2 = chunk_get_next(next);
            if ((next2 != NULL) &&
                ((next2->type == CT_PREPROC) ||
                 ((chunk_type == CT_ASSIGN) &&
                  (next2->type == CT_BRACE_OPEN))))
            {
               continue;
            }
         }

         /*NOTE: may end up processing a chunk twice if changed */
         if (mode == TP_TRAIL)
         {
            if (chunk_is_newline(prev) && (prev->nl_count == 1))
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
         else  /* (mode == TP_LEAD) */
         {
            if (chunk_is_newline(next) && (next->nl_count == 1))
            {
               /* move the CT_BOOL to after the newline */
               chunk_move_after(pc, next);
            }
         }
      }
   }
}


/**
 * Searches for CT_CLASS_COLON and moves them, if needed.
 * Also breaks up the args
 */
void newlines_class_colon_pos(void)
{
   chunk_t    *pc;
   chunk_t    *next;
   chunk_t    *prev;
   tokenpos_e mode    = cpd.settings[UO_pos_class_colon].tp;
   chunk_t    *ccolon = NULL;

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next_ncnl(pc))
   {
      if ((ccolon == NULL) && (pc->type != CT_CLASS_COLON))
      {
         continue;
      }

      if (pc->type == CT_CLASS_COLON)
      {
         ccolon = pc;
         prev   = chunk_get_prev_nc(pc);
         next   = chunk_get_next_nc(pc);

         if (!chunk_is_newline(prev) && !chunk_is_newline(next) &&
             ((cpd.settings[UO_nl_class_colon].a & AV_ADD) != 0))
         {
            newline_add_after(pc);
            prev = chunk_get_prev_nc(pc);
            next = chunk_get_next_nc(pc);
         }

         if (cpd.settings[UO_nl_class_colon].a == AV_REMOVE)
         {
            if (chunk_is_newline(prev) &&
                chunk_safe_to_del_nl(prev))
            {
               chunk_del(prev);
               prev = chunk_get_prev_nc(pc);
            }
            if (chunk_is_newline(next) &&
                chunk_safe_to_del_nl(next))
            {
               chunk_del(next);
               next = chunk_get_next_nc(pc);
            }
         }

         if (mode == TP_TRAIL)
         {
            if (chunk_is_newline(prev) && (prev->nl_count == 1) &&
                chunk_safe_to_del_nl(prev))
            {
               chunk_swap(pc, prev);
            }
         }
         else if (mode == TP_LEAD)
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
            if ((cpd.settings[UO_nl_class_init_args].a & AV_ADD) != 0)
            {
               if (cpd.settings[UO_pos_class_comma].tp == TP_TRAIL)
               {
                  newline_add_after(pc);
               }
               else if (cpd.settings[UO_pos_class_comma].tp == TP_LEAD)
               {
                  newline_add_before(pc);
                  next = chunk_get_next_nc(pc);
                  if (chunk_is_newline(next) && chunk_safe_to_del_nl(next))
                  {
                     chunk_del(next);
                  }
               }
            }
            else if (cpd.settings[UO_nl_class_init_args].a == AV_REMOVE)
            {
               next = chunk_get_next(pc);
               if (chunk_is_newline(next) && chunk_safe_to_del_nl(next))
               {
                  chunk_del(next);
               }
            }
         }
      }
   }
}


/**
 * Scans for newline tokens and limits the nl_count.
 * A newline token has a minimum nl_count of 1.
 * Note that a blank line is actually 2 newlines, unless the newline is the
 * first chunk.  But we don't handle the first chunk.
 * So, most comparisons have +1 below.
 */
void do_blank_lines(void)
{
   chunk_t *pc;
   chunk_t *next;
   chunk_t *prev;
   chunk_t *pcmt;

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

      /* Limit consecutive newlines */
      if ((cpd.settings[UO_nl_max].n > 0) &&
          (pc->nl_count > (cpd.settings[UO_nl_max].n)))
      {
         pc->nl_count = cpd.settings[UO_nl_max].n;
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
            LOG_FMT(LCMTNL, "%s: NL-MLCommentC: line %d\n", __func__, next->orig_line);
            pc->nl_count = cpd.settings[UO_nl_before_block_comment].n;
         }
      }

      /** Control blanks before single line C comments */
      if ((cpd.settings[UO_nl_before_c_comment].n > pc->nl_count) &&
          (next != NULL) &&
          (next->type == CT_COMMENT))
      {
         /* Don't add blanks after a open brace */
         if ((prev == NULL) ||
             ((prev->type != CT_BRACE_OPEN) &&
              (prev->type != CT_VBRACE_OPEN) &&
              (pcmt->type != CT_COMMENT)))
         {
            LOG_FMT(LCMTNL, "%s: NL-CommentC: line %d\n", __func__, next->orig_line);
            pc->nl_count = cpd.settings[UO_nl_before_c_comment].n;
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
            LOG_FMT(LCMTNL, "%s: NL-CommentCPP: line %d\n", __func__, next->orig_line);
            pc->nl_count = cpd.settings[UO_nl_before_cpp_comment].n;
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
            LOG_FMT(LCMTNL, "%s: NL-BeforePrivate: line %d\n", __func__, next->orig_line);
            pc->nl_count = cpd.settings[UO_nl_before_access_spec].n;
         }
      }

      /** Control blanks after an access spec */
      if ((cpd.settings[UO_nl_after_access_spec].n > 0) &&
          (cpd.settings[UO_nl_after_access_spec].n != pc->nl_count) &&
          (prev != NULL) &&
          (prev->type == CT_PRIVATE_COLON))
      {
         LOG_FMT(LCMTNL, "%s: NL-AfterPrivate: line %d\n", __func__, prev->orig_line);
         pc->nl_count = cpd.settings[UO_nl_after_access_spec].n;
      }

      /* Add blanks after function bodies */
      if ((prev != NULL) && (prev->type == CT_BRACE_CLOSE) &&
          ((prev->parent_type == CT_FUNC_DEF) ||
           (prev->parent_type == CT_FUNC_CLASS) ||
           (prev->parent_type == CT_OC_MSG_DECL) ||
           (prev->parent_type == CT_ASSIGN)))
      {
         if (prev->flags & PCF_ONE_LINER)
         {
            if (cpd.settings[UO_nl_after_func_body_one_liner].n > pc->nl_count)
            {
               pc->nl_count = cpd.settings[UO_nl_after_func_body_one_liner].n;
            }
         }
         else
         {
            if ((cpd.settings[UO_nl_after_func_body].n > 0) &&
                (cpd.settings[UO_nl_after_func_body].n != pc->nl_count))
            {
               pc->nl_count = cpd.settings[UO_nl_after_func_body].n;
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
         }
         if ((cpd.settings[UO_nl_after_func_proto_group].n > pc->nl_count) &&
             (next != NULL) &&
             (next->parent_type != CT_FUNC_PROTO) &&
             can_increase_nl(pc))
         {
            pc->nl_count = cpd.settings[UO_nl_after_func_proto_group].n;
         }
      }

      /* Change blanks between a function comment and body */
      if ((cpd.settings[UO_nl_comment_func_def].n != 0) &&
          (pcmt->type == CT_COMMENT_MULTI) &&
          (pcmt->parent_type == CT_COMMENT_WHOLE) &&
          (next != NULL) && (next->parent_type == CT_FUNC_DEF))
      {
         if (cpd.settings[UO_nl_comment_func_def].n != pc->nl_count)
         {
            LOG_FMT(LCMTNL, "%s: nl_comment_func_def affected line %d\n", __func__, pc->orig_line);
            pc->nl_count = cpd.settings[UO_nl_comment_func_def].n;
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
               pc->nl_count = cpd.settings[UO_nl_after_try_catch_finally].n;
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
            pc->nl_count = cpd.settings[UO_nl_between_get_set].n;
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
            pc->nl_count = cpd.settings[UO_nl_around_cs_property].n;
         }
         else if ((next->parent_type == CT_CS_PROPERTY) &&
                  (next->flags & PCF_STMT_START))
         {
            pc->nl_count = cpd.settings[UO_nl_around_cs_property].n;
         }
      }
   }
}


void newlines_cleanup_dup(void)
{
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
         chunk_del(pc);
      }
      pc = next;
   }
}


/**
 * Make sure there is a blank line after a commented group of values
 */
static void newlines_double_space_struct_enum_union(chunk_t *open_brace)
{
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
