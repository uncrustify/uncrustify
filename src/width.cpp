/**
 * @file width.cpp
 * Limits line width.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "width.h"
#include "uncrustify_types.h"
#include "chunk_list.h"
#include "prototypes.h"
#include "uncrustify.h"
#include "indent.h"
#include "newlines.h"
#include <cstdlib>


struct cw_entry
{
   chunk_t *pc;
   size_t  pri;
};

struct token_pri
{
   c_token_t tok;
   size_t    pri;
};


static_inline bool is_past_width(chunk_t *pc);


/**
 * Split right after the chunk
 */
static void split_before_chunk(chunk_t *pc);


static size_t get_split_pri(c_token_t tok);


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
 *  - concatenated strings
 *  - ? :
 *  - function open paren not followed by close paren
 */
static void try_split_here(cw_entry &ent, chunk_t *pc);


/**
 * Scan backwards to find the most appropriate spot to split the line
 * and insert a newline.
 *
 * See if this needs special function handling.
 * Scan backwards and find the best token for the split.
 *
 * @param start The first chunk that exceeded the limit
 */

static bool split_line(chunk_t *pc);


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
static void split_fcn_params(chunk_t *start);


/**
 * Splits the parameters at every comma that is at the fparen level.
 *
 * @param start   the offending token
 */
static void split_fcn_params_full(chunk_t *start);


/**
 * A for statement is too long.
 * Step backwards and forwards to find the semicolons
 * Try splitting at the semicolons first.
 * If that doesn't work, then look for a comma at paren level.
 * If that doesn't work, then look for an assignment at paren level.
 * If that doesn't work, then give up.
 */
static void split_for_stmt(chunk_t *start);


static_inline bool is_past_width(chunk_t *pc)
{
   // allow char to sit at last column by subtracting 1
   return((pc->column + pc->len() - 1) > cpd.settings[UO_code_width].u);
}


static void split_before_chunk(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LSPLIT, "  %s: %s\n", __func__, pc->text());

   if (!chunk_is_newline(pc) &&
       !chunk_is_newline(chunk_get_prev(pc)))
   {
      newline_add_before(pc);
      // reindent needs to include the indent_continue value and was off by one
      reindent_line(pc, pc->brace_level * cpd.settings[UO_indent_columns].u +
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
   LOG_FUNC_ENTRY();
   LOG_FMT(LSPLIT, "%s\n", __func__);

   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next(pc))
   {
      if (!chunk_is_newline(pc) &&
          !chunk_is_comment(pc) &&
          (pc->type != CT_SPACE) &&
          is_past_width(pc))
      {
         if (!split_line(pc))
         {
            LOG_FMT(LSPLIT, "%s: Bailed on %zu:%zu %s\n",
                    __func__, pc->orig_line, pc->orig_col, pc->text());
            break;
         }
      }
   }
}


static const token_pri pri_table[] =
{
   { CT_SEMICOLON,    1 },
   { CT_COMMA,        2 },
   { CT_BOOL,         3 },
   { CT_COMPARE,      4 },
   { CT_ARITH,        5 },
   { CT_CARET,        6 },
   { CT_ASSIGN,       7 },
   { CT_STRING,       8 },
   { CT_FOR_COLON,    9 },
   //{ CT_DC_MEMBER, 10 },
   //{ CT_MEMBER,    10 },
   { CT_QUESTION,    20 }, // allow break in ? : for ls_code_width
   { CT_COND_COLON,  20 },
   { CT_FPAREN_OPEN, 21 }, // break after function open paren not followed by close paren
   { CT_QUALIFIER,   25 },
   { CT_CLASS,       25 },
   { CT_STRUCT,      25 },
   { CT_TYPE,        25 },
   { CT_TYPENAME,    25 },
   { CT_VOLATILE,    25 },
};


static size_t get_split_pri(c_token_t tok)
{
   for (auto token : pri_table)
   {
      if (token.tok == tok)
      {
         return(token.pri);
      }
   }
   return(0);
}


static void try_split_here(cw_entry &ent, chunk_t *pc)
{
   LOG_FUNC_ENTRY();

   size_t pc_pri = get_split_pri(pc->type);
   if (pc_pri == 0)
   {
      return;
   }

   /* Can't split after a newline */
   chunk_t *prev = chunk_get_prev(pc);
   if ((prev == nullptr) || (chunk_is_newline(prev) && (pc->type != CT_STRING)))
   {
      return;
   }

   /* Can't split a function without arguments */
   if (pc->type == CT_FPAREN_OPEN)
   {
      chunk_t *next = chunk_get_next(pc);
      if (next->type == CT_FPAREN_CLOSE)
      {
         return;
      }
   }

   /* Only split concatenated strings */
   if (pc->type == CT_STRING)
   {
      chunk_t *next = chunk_get_next(pc);
      if (next->type != CT_STRING)
      {
         return;
      }
   }

   /* keep common groupings unless ls_code_width */
   if (!cpd.settings[UO_ls_code_width].b && (pc_pri >= 20))
   {
      return;
   }

   /* don't break after last term of a qualified type */
   if (pc_pri == 25)
   {
      chunk_t *next = chunk_get_next(pc);
      if ((next->type != CT_WORD) && (get_split_pri(next->type) != 25))
      {
         return;
      }
   }

   /* Check levels first */
   bool change = false;
   if ((ent.pc == nullptr) || (pc->level < ent.pc->level))
   {
      change = true;
   }
   else
   {
      if ((pc->level >= ent.pc->level) &&
          (pc_pri < ent.pri))
      {
         change = true;
      }
   }

   if (change)
   {
      ent.pc  = pc;
      ent.pri = pc_pri;
   }
} // try_split_here


static bool split_line(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LSPLIT, "%s: line %zu, col %zu token: '%s' [%s] (IN_FUNC=%d) ",
           __func__, start->orig_line, start->column, start->text(),
           get_token_name(start->type),
           (start->flags & (PCF_IN_FCN_DEF | PCF_IN_FCN_CALL)) != 0);
#ifdef DEBUG
   LOG_FMT(LSPLIT, "\n");
#endif // DEBUG

   /**
    * break at maximum line length if ls_code_width is true
    */
   if (start->flags & PCF_ONE_LINER)
   {
      LOG_FMT(LSPLIT, " ** ONCE LINER SPLIT **\n");
      undo_one_liner(start);
      newlines_cleanup_braces(false);
      return(false);
   }

   if (cpd.settings[UO_ls_code_width].b)
   {
   }
   /* Check to see if we are in a for statement */
   else if (start->flags & PCF_IN_FOR)
   {
      LOG_FMT(LSPLIT, " ** FOR SPLIT **\n");
      split_for_stmt(start);
      if (!is_past_width(start))
      {
         return(true);
      }
      LOG_FMT(LSPLIT, "%s: for split didn't work\n", __func__);
   }

   /* If this is in a function call or prototype, split on commas or right
    * after the open paren
    */
   else if ((start->flags & PCF_IN_FCN_DEF) ||
            ((start->level == (start->brace_level + 1)) &&
             (start->flags & PCF_IN_FCN_CALL)))
   {
      LOG_FMT(LSPLIT, " ** FUNC SPLIT **\n");

      if (cpd.settings[UO_ls_func_split_full].b)
      {
         split_fcn_params_full(start);
         if (!is_past_width(start))
         {
            return(true);
         }
      }
      split_fcn_params(start);
      return(true);
   }

   /**
    * Try to find the best spot to split the line
    */
   cw_entry ent;

   memset(&ent, 0, sizeof(ent));
   chunk_t *pc = start;
   chunk_t *prev;

   while (((pc = chunk_get_prev(pc)) != nullptr) && !chunk_is_newline(pc))
   {
      LOG_FMT(LSPLIT, "%s: at %s, col=%zu\n", __func__, pc->text(), pc->orig_col);
      if (pc->type != CT_SPACE)
      {
         try_split_here(ent, pc);
         /*  break at maximum line length */
         if ((ent.pc != nullptr) && (cpd.settings[UO_ls_code_width].b))
         {
            break;
         }
      }
   }

   if (ent.pc == nullptr)
   {
      LOG_FMT(LSPLIT, "\n%s:    TRY_SPLIT yielded NO SOLUTION for line %zu at %s [%s]\n",
              __func__, start->orig_line, start->text(), get_token_name(start->type));
   }
   else
   {
      LOG_FMT(LSPLIT, "\n%s:    TRY_SPLIT yielded '%s' [%s] on line %zu\n",
              __func__, ent.pc->text(), get_token_name(ent.pc->type), ent.pc->orig_line);
      LOG_FMT(LSPLIT, "%s: ent at %s, col=%zu\n",
              __func__, ent.pc->text(), ent.pc->orig_col);
   }

   /* Break before the token instead of after it according to the pos_xxx rules */
   if (ent.pc == nullptr)
   {
      pc = nullptr;
   }
   else
   {
      if (((chunk_is_token(ent.pc, CT_ARITH) || chunk_is_token(ent.pc, CT_CARET)) &&
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
      LOG_FMT(LSPLIT, "%s: at %s, col=%zu\n", __func__, pc->text(), pc->orig_col);
   }

   if (pc == nullptr)
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
         return(true);
      }
   }

   /* add a newline before pc */
   prev = chunk_get_prev(pc);
   if ((prev != nullptr) && !chunk_is_newline(pc) && !chunk_is_newline(prev))
   {
      //int plen = (pc->len() < 5) ? pc->len() : 5;
      //int slen = (start->len() < 5) ? start->len() : 5;
      //LOG_FMT(LSPLIT, " '%.*s' [%s], started on token '%.*s' [%s]\n",
      //        plen, pc->text(), get_token_name(pc->type),
      //        slen, start->text(), get_token_name(start->type));
      LOG_FMT(LSPLIT, "  %s [%s], started on token '%s' [%s]\n",
              pc->text(), get_token_name(pc->type),
              start->text(), get_token_name(start->type));

      split_before_chunk(pc);
   }
   return(true);
} // split_line


static void split_for_stmt(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   size_t  max_cnt     = cpd.settings[UO_ls_for_split_full].b ? 2 : 1;
   chunk_t *open_paren = nullptr;
   size_t  nl_cnt      = 0;

   LOG_FMT(LSPLIT, "%s: starting on %s, line %zu\n",
           __func__, start->text(), start->orig_line);

   /* Find the open paren so we know the level and count newlines */
   chunk_t *pc = start;
   while ((pc = chunk_get_prev(pc)) != nullptr)
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
   if (open_paren == nullptr)
   {
      LOG_FMT(LSPLIT, "No open paren\n");
      return;
   }

   /* see if we started on the semicolon */
   int     count = 0;
   chunk_t *st[2];
   pc = start;
   if ((pc->type == CT_SEMICOLON) && (pc->parent_type == CT_FOR))
   {
      st[count++] = pc;
   }

   /* first scan backwards for the semicolons */
   while ((count < static_cast<int>(max_cnt)) && ((pc = chunk_get_prev(pc)) != nullptr) &&
          (pc->flags & PCF_IN_SPAREN))
   {
      if ((pc->type == CT_SEMICOLON) && (pc->parent_type == CT_FOR))
      {
         st[count++] = pc;
      }
   }

   /* And now scan forward */
   pc = start;
   while ((count < static_cast<int>(max_cnt)) && ((pc = chunk_get_next(pc)) != nullptr) &&
          (pc->flags & PCF_IN_SPAREN))
   {
      if ((pc->type == CT_SEMICOLON) && (pc->parent_type == CT_FOR))
      {
         st[count++] = pc;
      }
   }

   while (--count >= 0)
   {
      /* \todo st[0] may be uninitialized here */
      LOG_FMT(LSPLIT, "%s: split before %s\n", __func__, st[count]->text());
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
} // split_for_stmt


static void split_fcn_params_full(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LSPLIT, "%s", __func__);
#ifdef DEBUG
   LOG_FMT(LSPLIT, "\n");
#endif // DEBUG

   /* Find the opening fparen */
   chunk_t *fpo = start;
   while ((fpo = chunk_get_prev(fpo)) != nullptr)
   {
#ifdef DEBUG
      LOG_FMT(LSPLIT, "%s: %s, Col=%zu, Level=%zu\n",
              __func__, fpo->text(), fpo->orig_col, fpo->level);
#endif // DEBUG
      if ((fpo->type == CT_FPAREN_OPEN) &&
          (fpo->level == start->level - 1))
      {
         // opening parenthesis found. Issue #1020
         break;
      }
   }

   /* Now break after every comma */
   chunk_t *pc = fpo;
   while ((pc = chunk_get_next_ncnl(pc)) != nullptr)
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


static void split_fcn_params(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LSPLIT, "  %s: %s", __func__, start->text());
#ifdef DEBUG
   LOG_FMT(LSPLIT, "\n");
#endif // DEBUG

   /* Find the opening fparen */
   chunk_t *fpo = start;
   while (((fpo = chunk_get_prev(fpo)) != nullptr) &&
          (fpo->type != CT_FPAREN_OPEN))
   {
      /* do nothing */
   }

   chunk_t *pc     = chunk_get_next_ncnl(fpo);
   size_t  min_col = pc->column;

   LOG_FMT(LSPLIT, " mincol=%zu, max_width=%zu ",
           min_col, cpd.settings[UO_code_width].u - min_col);

   int cur_width = 0;
   int last_col  = -1;
   while (pc != nullptr)
   {
      if (chunk_is_newline(pc))
      {
         cur_width = 0;
         last_col  = -1;
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
            if (((last_col - 1) > static_cast<int>(cpd.settings[UO_code_width].u)) ||
                (pc->type == CT_FPAREN_CLOSE))
            {
               break;
            }
         }
      }
      pc = chunk_get_next(pc);
   }

   /* back up until the prev is a comma */
   chunk_t *prev = pc;
   while ((prev = chunk_get_prev(prev)) != nullptr)
   {
      if (chunk_is_newline(prev) || (prev->type == CT_COMMA))
      {
         break;
      }
      last_col -= pc->len();
      if (prev->type == CT_FPAREN_OPEN)
      {
         pc = chunk_get_next(prev);
         if (!cpd.settings[UO_indent_paren_nl].b)
         {
            min_col = pc->brace_level * cpd.settings[UO_indent_columns].u + 1;
            if (cpd.settings[UO_indent_continue].n == 0)
            {
               min_col += cpd.settings[UO_indent_columns].u;
            }
            else
            {
               min_col += abs(cpd.settings[UO_indent_continue].n);
            }
         }
         /* Don't split "()" */
         if (pc->type != c_token_t(prev->type + 1))
         {
            break;
         }
      }
   }
   if ((prev != nullptr) && !chunk_is_newline(prev))
   {
      LOG_FMT(LSPLIT, " -- ended on [%s] --\n", get_token_name(prev->type));
      pc = chunk_get_next(prev);
      newline_add_before(pc);
      reindent_line(pc, min_col);
      cpd.changes++;
   }
} // split_fcn_params
