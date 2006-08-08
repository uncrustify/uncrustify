/**
 * @file width.cpp
 * Limits line width.
 *
 * $Id: align.cpp 365 2006-07-27 02:40:27Z bengardner $
 */

#include "uncrustify_types.h"
#include "chunk_list.h"
#include "prototypes.h"

static void split_line(chunk_t *pc);
static chunk_t *split_fcn_params(chunk_t *start);

void do_code_width(void)
{
   chunk_t *pc;

   LOG_FMT(LSPLIT, "%s\n", __func__);

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next(pc))
   {
      if (!chunk_is_newline(pc) && !chunk_is_comment(pc) &&
         ((pc->column + pc->len) > cpd.settings[UO_code_width].n))
      {
         split_line(pc);
      }
   }
}

struct cw_entry
{
   chunk_t *pc;
};


/**
 * Checks to see if pc is a better spot to split.
 * This should only be called going BACKWARDS (ie prev)
 * Only checks CT_BOOL, CT_COMPARE, CT_ARITH
 */
static void try_split_here(cw_entry& ent, chunk_t *pc)
{
   chunk_t *prev;

   if ((pc->type != CT_BOOL) &&
       (pc->type != CT_COMPARE) &&
       (pc->type != CT_ARITH))
   {
      return;
   }

   prev = chunk_get_prev(pc);
   if ((prev == NULL) || chunk_is_newline(prev))
   {
      return;
   }

   bool change = false;
   if ((ent.pc == NULL) || (pc->level < ent.pc->level))
   {
      change = true;
   }
   else if (pc->level > ent.pc->level)
   {
      /* no way */
   }
   else
   {
      /* same level */
      if (pc->type == ent.pc->type)
      {
         change = true;
      }
      else if ((pc->type == CT_BOOL) ||
               ((pc->type == CT_COMPARE) &&
                (ent.pc->type != CT_BOOL)))
      {
         change = true;
      }
   }
   if (change)
   {
      ent.pc = pc;
   }
}


/**
 * Scan backwards to find the most appropriate spot to split the line
 * and insert a newline.
 *
 * See if this needs special function handling.
 * Scan backwards and find the best token for the split.
 *
 * @param start The first chunk that exceeded the limit
 */
static void split_line(chunk_t *start)
{
   chunk_t *pc = start;

   /* Don't break before a close, comma, or colon */
   if ((pc->type == CT_PAREN_CLOSE) ||
       (pc->type == CT_PAREN_OPEN) ||
       (pc->type == CT_FPAREN_CLOSE) ||
       (pc->type == CT_FPAREN_OPEN) ||
       (pc->type == CT_SPAREN_CLOSE) ||
       (pc->type == CT_SPAREN_OPEN) ||
       (pc->type == CT_ANGLE_CLOSE) ||
       (pc->type == CT_BRACE_CLOSE) ||
       (pc->type == CT_COMMA) ||
       (pc->type == CT_SEMICOLON) ||
       (pc->type == CT_VSEMICOLON) ||
       (pc->len == 0))
   {
      return;
   }

   LOG_FMT(LSPLIT, "%s: line %d, col %d ",
           __func__, start->orig_line, start->column);

   chunk_t *prev;
   if (((start->flags & PCF_IN_FCN_DEF) != 0) ||
       ((start->level == start->brace_level) &&
        ((start->flags & PCF_IN_FCN_CALL) != 0)))
   {
      pc = split_fcn_params(start);
   }
   else
   {
      cw_entry ent;

      memset(&ent, 0, sizeof(ent));
      pc = start;
      while (((pc = chunk_get_prev(pc)) != NULL) &&
             !chunk_is_newline(pc))
      {
         try_split_here(ent, pc);
      }
      pc = chunk_get_next(ent.pc);
      if (pc == NULL)
      {
         pc = start;
      }
   }

   /* add a newline before pc */
   prev = chunk_get_prev(pc);
   if ((prev != NULL) && !chunk_is_newline(pc) && !chunk_is_newline(prev))
   {
      int plen = (pc->len < 5) ? pc->len : 5;
      int slen = (start->len < 5) ? start->len : 5;
      LOG_FMT(LSPLIT, " '%.*s' [%s], started on token '%.*s' [%s]\n",
              plen, pc->str, get_token_name(pc->type),
              slen, start->str, get_token_name(start->type));

      newline_add_before(pc);
      reindent_line(pc, pc->brace_level * cpd.settings[UO_indent_columns].n);
      cpd.changes++;
   }
}


/**
 * Figures out where to split a function def/proto/call
 *
 * For fcn protos and defs. Also fcn calls where level == brace_level:
 *   - find the open fparen
 *     + if it doesn't have a newline right after it
 *       * see if all parameters will fit individually after the paren
 *       * if not, throw a newline after the open paren & return*
 *   - scan backwards to the open fparen or comma
 *     + if there isn't a newline after that item, add one & return
 *     + otherwise, add a newline before the start token
 *
 * @param start   the offending token
 * @return        the token that should have a newline
 *                inserted before it
 */
static chunk_t *split_fcn_params(chunk_t *start)
{
   LOG_FMT(LSPLIT, "  %s: ", __func__);

   chunk_t *prev;
   chunk_t *fpo;
   chunk_t *pc;

   /* Find the opening fparen */
   fpo = start;
   while (((fpo = chunk_get_prev(fpo)) != NULL) &&
          (fpo->type != CT_FPAREN_OPEN))
   {
      /* do nothing */
   }

   pc = chunk_get_next(fpo);
   if (!chunk_is_newline(pc))
   {
      int min_col = pc->column;
      int max_width = 0;
      int cur_width = 0;
      int last_col = -1;

      LOG_FMT(LSPLIT, " mincol=%d, max_width=%d ",
              min_col,
              cpd.settings[UO_code_width].n - min_col);

      while (pc != NULL)
      {
         if (chunk_is_newline(pc))
         {
            last_col = -1;
         }
         else
         {
            if (last_col < 0)
            {
               last_col = pc->column;
            }
            cur_width += (pc->column - last_col) + pc->len;
            last_col = pc->column + pc->len;

            if ((pc->type == CT_COMMA) ||
                (pc->type == CT_FPAREN_CLOSE))
            {
               cur_width--;
               LOG_FMT(LSPLIT, " width=%d ", cur_width);
               if (cur_width > max_width)
               {
                  max_width = cur_width;
                  if ((max_width + min_col) > cpd.settings[UO_code_width].n)
                  {
                     break;
                  }
               }
               cur_width = 0;
               last_col = -1;
               if (pc->type == CT_FPAREN_CLOSE)
               {
                  break;
               }
            }
         }
         pc = chunk_get_next(pc);
      }

      if ((max_width + min_col) > cpd.settings[UO_code_width].n)
      {
         LOG_FMT(LSPLIT, " - A param won't fit, nl after open paren.");
         return(chunk_get_next(fpo));
      }
   }

   /* back up until the prev is a comma */
   prev = pc;
   while ((prev = chunk_get_prev(prev)) != NULL)
   {
      if (chunk_is_newline(prev) ||
          (prev->type == CT_COMMA) ||
          (prev->type == CT_FPAREN_OPEN))
      {
         break;
      }
   }
   if (prev != NULL)
   {
      LOG_FMT(LSPLIT, " -- ended on [%s] -- ", get_token_name(prev->type));
   }
   if (prev != NULL)
   {
      pc = chunk_get_next(prev);
   }
   return(pc);
}

