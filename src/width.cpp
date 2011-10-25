/**
 * @file width.cpp
 * Limits line width.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "uncrustify_types.h"
#include "chunk_list.h"
#include "prototypes.h"
#include <cstdlib>

static void split_line(chunk_t *pc);
static void split_fcn_params(chunk_t *start);
static void split_fcn_params_full(chunk_t *start);
static void split_for_stmt(chunk_t *start);

static_inline bool is_past_width(chunk_t *pc)
{
   // allow char to sit at last column by subtracting 1
   return((pc->column + pc->len() - 1) > cpd.settings[UO_code_width].n);
}


/**
 * Split right after the chunk
 */
static void split_before_chunk(chunk_t *pc)
{
   LOG_FMT(LSPLIT, "%s: %s\n", __func__, pc->str.c_str());

   if (!chunk_is_newline(pc) &&
       !chunk_is_newline(chunk_get_prev(pc)))
   {
      newline_add_before(pc);
      // reindent needs to include the indent_continue value and was off by one
      reindent_line(pc, pc->brace_level * cpd.settings[UO_indent_columns].n +
                    abs(cpd.settings[UO_indent_continue].n) + 1);
      cpd.changes++;
   }
}


/**
 * Step forward until a token goes beyond the limit and then call split_line()
 * to split the line at or before that point.
 */
void do_code_width(void)
{
   chunk_t *pc;

   LOG_FMT(LSPLIT, "%s\n", __func__);

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next(pc))
   {
      if (!chunk_is_newline(pc) &&
          !chunk_is_comment(pc) &&
          (pc->type != CT_SPACE) &&
          is_past_width(pc))
      {
         split_line(pc);
      }
   }
}


struct cw_entry
{
   chunk_t *pc;
   int     pri;
};

struct token_pri
{
   c_token_t tok;
   int       pri;
};

static const token_pri pri_table[] =
{
   { CT_SEMICOLON, 1 },
   { CT_COMMA,     2 },
   { CT_BOOL,      3 },
   { CT_COMPARE,   4 },
   { CT_ARITH,     5 },
   { CT_ASSIGN,    6 },
   //{ CT_DC_MEMBER, 10 },
   //{ CT_MEMBER,    10 },
   { CT_QUESTION,    20 }, // allow break in ? : for indent_continue < 0
   { CT_COND_COLON,  20 },
   { CT_FPAREN_OPEN, 21 }, // break after function open paren not followed by close paren
   { CT_QUALIFIER,   25 },
   { CT_CLASS,       25 },
   { CT_STRUCT,      25 },
   { CT_TYPE,        25 },
   { CT_TYPENAME,    25 },
   { CT_VOLATILE,    25 },
};


static int get_split_pri(c_token_t tok)
{
   int idx;

   for (idx = 0; idx < (int)ARRAY_SIZE(pri_table); idx++)
   {
      if (pri_table[idx].tok == tok)
      {
         return(pri_table[idx].pri);
      }
   }
   return(0);
}


/**
 * Checks to see if pc is a better spot to split.
 * This should only be called going BACKWARDS (ie prev)
 * A lower level wins
 *
 * Splitting Preference:
 *  - semicolon
 *  - comma
 *  - boolean op
 *  - comparison
 *  - arithmetic op
 *  - assignment
 *  - ? :
 *  - function open paren not followed by close paren
 */
static void try_split_here(cw_entry& ent, chunk_t *pc)
{
   chunk_t *next;
   chunk_t *prev;
   int     pc_pri = get_split_pri(pc->type);

   if (pc_pri == 0)
   {
      return;
   }

   /* Can't split after a newline */
   prev = chunk_get_prev(pc);
   if ((prev == NULL) || chunk_is_newline(prev))
   {
      return;
   }

   /* Can't split a function without arguments */
   if (pc->type == CT_FPAREN_OPEN)
   {
      next = chunk_get_next(pc);
      if (next->type == CT_FPAREN_CLOSE)
      {
         return;
      }
   }

   /* keep common groupings unless indent_continue < 0 */
   if ((cpd.settings[UO_indent_continue].n >= 0) && (pc_pri >= 20))
   {
      return;
   }

   /* don't break after last term of a qualified type */
   if (pc_pri == 25)
   {
      next = chunk_get_next(pc);
      if ((next->type != CT_WORD) && (get_split_pri(next->type) != 25))
      {
         return;
      }
   }

   /* Check levels first */
   bool change = false;
   if ((ent.pc == NULL) || (pc->level < ent.pc->level))
   {
      change = true;
   }
   else
   {
      if ((pc->level > ent.pc->level) &&
          (pc_pri <= ent.pri))
      {
         change = true;
      }
   }

   if (change)
   {
      ent.pc  = pc;
      ent.pri = pc_pri;
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
   LOG_FMT(LSPLIT, "%s: line %d, col %d token:%s[%s] (IN_FUNC=%d) ",
           __func__, start->orig_line, start->column, start->str.c_str(),
           get_token_name(start->type),
           (start->flags & (PCF_IN_FCN_DEF | PCF_IN_FCN_CALL)) != 0);

   /**
    * break at maximum line length if indent_continue is absolute
    */
   if (cpd.settings[UO_indent_continue].n < 0)
   {
   }
   /* Check to see if we are in a for statement */
   else if ((start->flags & PCF_IN_FOR) != 0)
   {
      LOG_FMT(LSPLIT, " ** FOR SPLIT **\n");
      split_for_stmt(start);
      if (!is_past_width(start))
      {
         return;
      }
      LOG_FMT(LSPLIT, "%s: for split didn't work\n", __func__);
   }

   /* If this is in a function call or prototype, split on commas or right
    * after the open paren
    */
   else if (((start->flags & PCF_IN_FCN_DEF) != 0) ||
            ((start->level == (start->brace_level + 1)) &&
             ((start->flags & PCF_IN_FCN_CALL) != 0)))
   {
      LOG_FMT(LSPLIT, " ** FUNC SPLIT **\n");

      if (cpd.settings[UO_ls_func_split_full].b)
      {
         split_fcn_params_full(start);
         if (!is_past_width(start))
         {
            return;
         }
      }
      split_fcn_params(start);
      if (!is_past_width(start))
      {
         return;
      }
      LOG_FMT(LSPLIT, "%s: func split didn't work\n", __func__);
   }

   /**
    * Try to find the best spot to split the line
    */
   cw_entry ent;

   memset(&ent, 0, sizeof(ent));
   chunk_t *pc = start;
   chunk_t *prev;

   while (((pc = chunk_get_prev(pc)) != NULL) && !chunk_is_newline(pc))
   {
      if (pc->type != CT_SPACE)
      {
         try_split_here(ent, pc);
         // break at maximum line length if indent_continue is absolute
         if ((ent.pc != NULL) && (cpd.settings[UO_indent_continue].n < 0))
             break;
      }
   }

   if (ent.pc == NULL)
   {
      LOG_FMT(LSPLIT, "%s: TRY_SPLIT yielded NO SOLUTION for line %d at %s [%s]\n",
              __func__, start->orig_line, start->str.c_str(), get_token_name(start->type));
   }
   else
   {
      LOG_FMT(LSPLIT, "%s: TRY_SPLIT yielded '%s' [%s] on line %d\n", __func__,
              ent.pc->str.c_str(), get_token_name(ent.pc->type), ent.pc->orig_line);
   }

   /* Break before the token instead of after it according to the pos_xxx rules */
   if ((chunk_is_token(ent.pc, CT_ARITH) &&
        (cpd.settings[UO_pos_arith].tp & TP_LEAD)) ||
       (chunk_is_token(ent.pc, CT_ASSIGN) &&
        (cpd.settings[UO_pos_assign].tp & TP_LEAD)) ||
       (chunk_is_token(ent.pc, CT_COMPARE) &&
        (cpd.settings[UO_pos_compare].tp & TP_LEAD)) ||
       ((chunk_is_token(ent.pc, CT_COND_COLON) ||
         chunk_is_token(ent.pc, CT_QUESTION)) &&
        (cpd.settings[UO_pos_conditional].tp & TP_LEAD)) ||
       (chunk_is_token(ent.pc, CT_BOOL) &&
        (cpd.settings[UO_pos_bool].tp & TP_LEAD)))
   {
      pc = ent.pc;
   }
   else
   {
      pc = chunk_get_next(ent.pc);
   }
   if (pc == NULL)
   {
      pc = start;
      /* Don't break before a close, comma, or colon */
      if ((start->type == CT_PAREN_CLOSE) ||
          (start->type == CT_PAREN_OPEN) ||
          (start->type == CT_FPAREN_CLOSE) ||
          (start->type == CT_FPAREN_OPEN) ||
          (start->type == CT_SPAREN_CLOSE) ||
          (start->type == CT_SPAREN_OPEN) ||
          (start->type == CT_ANGLE_CLOSE) ||
          (start->type == CT_BRACE_CLOSE) ||
          (start->type == CT_COMMA) ||
          (start->type == CT_SEMICOLON) ||
          (start->type == CT_VSEMICOLON) ||
          (start->len() == 0))
      {
         LOG_FMT(LSPLIT, " ** NO GO **\n");

         /*TODO: Add in logic to handle 'hard' limits by backing up a token */
         return;
      }
   }

   /* add a newline before pc */
   prev = chunk_get_prev(pc);
   if ((prev != NULL) && !chunk_is_newline(pc) && !chunk_is_newline(prev))
   {
      int plen = (pc->len() < 5) ? pc->len() : 5;
      int slen = (start->len() < 5) ? start->len() : 5;
      LOG_FMT(LSPLIT, " '%.*s' [%s], started on token '%.*s' [%s]\n",
              plen, pc->str.c_str(), get_token_name(pc->type),
              slen, start->str.c_str(), get_token_name(start->type));

      split_before_chunk(pc);
   }
}


/**
 * A for statement is too long.
 * Step backwards and forwards to find the semicolons
 * Try splitting at the semicolons first.
 * If that doesn't work, then look for a comma at paren level.
 * If that doesn't work, then look for an assignment at paren level.
 * If that doesn't work, then give up.
 */
static void split_for_stmt(chunk_t *start)
{
   int     count   = 0;
   int     max_cnt = cpd.settings[UO_ls_for_split_full].b ? 2 : 1;
   chunk_t *st[2];
   chunk_t *pc;
   chunk_t *open_paren = NULL;
   int     nl_cnt      = 0;

   LOG_FMT(LSPLIT, "%s: starting on %s, line %d\n",
           __func__, start->str.c_str(), start->orig_line);

   /* Find the open paren so we know the level and count newlines */
   pc = start;
   while ((pc = chunk_get_prev(pc)) != NULL)
   {
      if (pc->type == CT_SPAREN_OPEN)
      {
         open_paren = pc;
         break;
      }
      if (pc->nl_count > 0)
      {
         nl_cnt += pc->nl_count;
      }
   }
   if (open_paren == NULL)
   {
      LOG_FMT(LSYS, "No open paren\n");
      return;
   }

   /* see if we started on the semicolon */
   pc = start;
   if ((pc->type == CT_SEMICOLON) && (pc->parent_type == CT_FOR))
   {
      st[count++] = pc;
   }

   /* first scan backwards for the semicolons */
   while ((count < max_cnt) && ((pc = chunk_get_prev(pc)) != NULL) &&
          (pc->flags & PCF_IN_SPAREN))
   {
      if ((pc->type == CT_SEMICOLON) && (pc->parent_type == CT_FOR))
      {
         st[count++] = pc;
      }
   }

   /* And now scan forward */
   pc = start;
   while ((count < max_cnt) && ((pc = chunk_get_next(pc)) != NULL) &&
          (pc->flags & PCF_IN_SPAREN))
   {
      if ((pc->type == CT_SEMICOLON) && (pc->parent_type == CT_FOR))
      {
         st[count++] = pc;
      }
   }

   while (--count >= 0)
   {
      LOG_FMT(LSPLIT, "%s: split before %s\n", __func__, st[count]->str.c_str());
      split_before_chunk(chunk_get_next(st[count]));
   }

   if (!is_past_width(start) || (nl_cnt > 0))
   {
      return;
   }

   /* Still past width, check for commas at paren level */
   pc = open_paren;
   while ((pc = chunk_get_next(pc)) != start)
   {
      if ((pc->type == CT_COMMA) && (pc->level == (open_paren->level + 1)))
      {
         split_before_chunk(chunk_get_next(pc));
         if (!is_past_width(pc))
         {
            return;
         }
      }
   }

   /* Still past width, check for a assignments at paren level */
   pc = open_paren;
   while ((pc = chunk_get_next(pc)) != start)
   {
      if ((pc->type == CT_ASSIGN) && (pc->level == (open_paren->level + 1)))
      {
         split_before_chunk(chunk_get_next(pc));
         if (!is_past_width(pc))
         {
            return;
         }
      }
   }
   /* Oh, well. We tried. */
}


/**
 * Splits the parameters at every comma that is at the fparen level.
 *
 * @param start   the offending token
 */
static void split_fcn_params_full(chunk_t *start)
{
   LOG_FMT(LSPLIT, "%s", __func__);

   chunk_t *fpo;
   chunk_t *pc;

   /* Find the opening fparen */
   fpo = start;
   while (((fpo = chunk_get_prev(fpo)) != NULL) &&
          (fpo->type != CT_FPAREN_OPEN))
   {
      /* do nothing */
   }

   /* Now break after every comma */
   pc = fpo;
   while ((pc = chunk_get_next_ncnl(pc)) != NULL)
   {
      if (pc->level <= fpo->level)
      {
         break;
      }
      if ((pc->level == (fpo->level + 1)) && (pc->type == CT_COMMA))
      {
         split_before_chunk(chunk_get_next(pc));
      }
   }
}


/**
 * Figures out where to split a function def/proto/call
 *
 * For fcn protos and defs. Also fcn calls where level == brace_level:
 *   - find the open fparen
 *     + if it doesn't have a newline right after it
 *       * see if all parameters will fit individually after the paren
 *       * if not, throw a newline after the open paren & return
 *   - scan backwards to the open fparen or comma
 *     + if there isn't a newline after that item, add one & return
 *     + otherwise, add a newline before the start token
 *
 * @param start   the offending token
 * @return        the token that should have a newline
 *                inserted before it
 */
static void split_fcn_params(chunk_t *start)
{
   LOG_FMT(LSPLIT, "  %s: ", __func__);

   chunk_t *next;
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
      int min_col   = pc->column;
      int max_width = 0;
      int cur_width = 0;
      int last_col  = -1;

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
            cur_width += (pc->column - last_col) + pc->len();
            last_col   = pc->column + pc->len();

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
               last_col  = -1;
               if (pc->type == CT_FPAREN_CLOSE)
               {
                  break;
               }
            }
         }
         pc = chunk_get_next(pc);
      }

      // don't split function w/o parameters
      next = chunk_get_next(fpo);
      if (((max_width + min_col) > cpd.settings[UO_code_width].n) &&
          next->type != CT_FPAREN_CLOSE)
      {
         LOG_FMT(LSPLIT, " - A param won't fit, nl after open paren.");
         split_before_chunk(chunk_get_next(fpo));
         return;
      }
   }

   /* back up until the prev is a comma */
   prev = pc;
   while ((prev = chunk_get_prev(prev)) != NULL)
   {
      if (chunk_is_newline(prev) ||
          (prev->type == CT_COMMA))
      {
         break;
      }
      if (prev->type == CT_FPAREN_OPEN)
      {
         /* Don't split "()" */
         pc = chunk_get_next(prev);
         if (pc->type != c_token_t(prev->type + 1))
         {
            break;
         }
      }
   }
   if (prev != NULL)
   {
      LOG_FMT(LSPLIT, " -- ended on [%s] -- ", get_token_name(prev->type));
   }
   if (prev != NULL)
   {
      split_before_chunk(chunk_get_next(prev));
   }
}
