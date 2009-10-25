/**
 * @file indent.cpp
 * Does all the indenting stuff.
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


/**
 * General indenting approach:
 * Indenting levels are put into a stack.
 *
 * The stack entries contain:
 *  - opening type
 *  - brace column
 *  - continuation column
 *
 * Items that start a new stack item:
 *  - preprocessor (new parse frame)
 *  - Brace Open (Virtual brace also)
 *  - Paren, Square, Angle open
 *  - Assignments
 *  - C++ '<<' operator (ie, cout << "blah")
 *  - case
 *  - class colon
 *  - return
 *  - types
 *  - any other continued statement
 *
 * Note that the column of items marked 'PCF_WAS_ALIGNED' is not changed.
 *
 * For an open brace:
 *  - indent increases by indent_columns
 *  - if part of if/else/do/while/switch/etc, an extra indent may be applied
 *  - if in a paren, then cont-col is set to column + 1, ie "({ some code })"
 *
 * Open paren/square/angle:
 * cont-col is set to the column of the item after the open paren, unless
 * followed by a newline, then it is set to (brace-col + indent_columns).
 * Examples:
 *    a_really_long_funcion_name(
 *       param1, param2);
 *    a_really_long_funcion_name(param1,
 *                               param2);
 *
 * Assignments:
 * Assignments are continued aligned with the first item after the assignment,
 * unless the assign is followed by a newline.
 * Examples:
 *    some.variable = asdf + asdf +
 *                    asdf;
 *    some.variable =
 *       asdf + asdf + asdf;
 *
 * C++ << operator:
 * Handled the same as assignment.
 * Examples:
 *    cout << "this is test number: "
 *         << test_number;
 *
 * case:
 * Started with case or default.
 * Terminated with close brace at level or another case or default.
 * Special indenting according to various rules.
 *  - indent of case label
 *  - indent of case body
 *  - how to handle optional braces
 * Examples:
 * {
 * case x: {
 *    a++;
 *    break;
 *    }
 * case y:
 *    b--;
 *    break;
 * default:
 *    c++;
 *    break;
 * }
 *
 * Class colon:
 * Indent continuation by indent_columns:
 * class my_class :
 *    baseclass1,
 *    baseclass2
 * {
 *
 * Return: same as assignemts
 * If the return statement is not fully paren'd, then the indent continues at
 * the column of the item after the return. If it is paren'd, then the paren
 * rules apply.
 * return somevalue +
 *        othervalue;
 *
 * Type: pretty much the same as assignments
 * Examples:
 * int foo,
 *     bar,
 *     baz;
 *
 * Any other continued item:
 * There shouldn't be anything not covered by the above cases, but any other
 * continued item is indented by indent_columns:
 * Example:
 * somereallycrazylongname.with[lotsoflongstuff].
 *    thatreallyannoysme.whenIhavetomaintain[thecode] = 3;
 */

static void indent_comment(chunk_t *pc, int col);


void indent_to_column(chunk_t *pc, int column)
{
   if (column < pc->column)
   {
      column = pc->column;
   }
   reindent_line(pc, column);
}


enum align_mode
{
   ALMODE_SHIFT,     /* shift relative to the current column */
   ALMODE_KEEP_ABS,  /* try to keep the original absolute column */
   ALMODE_KEEP_REL,  /* try to keep the original gap */
};

/* Same as indent_to_column, except we can move both ways */
void align_to_column(chunk_t *pc, int column)
{
   if (column == pc->column)
   {
      return;
   }

   LOG_FMT(LINDLINE, "%s: %d] col %d on %.*s [%s] => %d\n",
           __func__, pc->orig_line, pc->column, pc->len, pc->str,
           get_token_name(pc->type), column);

   int col_delta = column - pc->column;
   int min_col   = column;
   int min_delta;

   pc->column = column;
   do
   {
      chunk_t    *next = chunk_get_next(pc);
      chunk_t    *prev;
      align_mode almod = ALMODE_SHIFT;

      if (next == NULL)
      {
         break;
      }
      min_delta = space_col_align(pc, next);
      min_col  += min_delta;
      prev      = pc;
      pc        = next;

      if (chunk_is_comment(pc) && (pc->parent_type != CT_COMMENT_EMBED))
      {
         almod = (chunk_is_single_line_comment(pc) &&
                  cpd.settings[UO_indent_relative_single_line_comments].b) ?
                 ALMODE_KEEP_REL : ALMODE_KEEP_ABS;
      }

      if (almod == ALMODE_KEEP_ABS)
      {
         /* Keep same absolute column */
         pc->column = pc->orig_col;
         if (pc->column < min_col)
         {
            pc->column = min_col;
         }
      }
      else if (almod == ALMODE_KEEP_REL)
      {
         /* Keep same relative column */
         int orig_delta = pc->orig_col - prev->orig_col;
         if (orig_delta < min_delta)
         {
            orig_delta = min_delta;
         }
         pc->column = prev->column + orig_delta;
      }
      else /* ALMODE_SHIFT */
      {
         /* Shift by the same amount */
         pc->column += col_delta;
         if (pc->column < min_col)
         {
            pc->column = min_col;
         }
      }
      LOG_FMT(LINDLINED, "   %s set column of %s on line %d to col %d (orig %d)\n",
              (almod == ALMODE_KEEP_ABS) ? "abs" :
              (almod == ALMODE_KEEP_REL) ? "rel" : "sft",
              get_token_name(pc->type), pc->orig_line, pc->column, pc->orig_col);
   } while ((pc != NULL) && (pc->nl_count == 0));
}


/**
 * Changes the initial indent for a line to the given column
 *
 * @param pc      The chunk at the start of the line
 * @param column  The desired column
 */
void reindent_line2(chunk_t *pc, int column, const char *fcn_name, int lineno)
{
   LOG_FMT(LINDLINE, "%s: %d] col %d on %.*s [%s] => %d <called from '%s' line %d\n",
           __func__, pc->orig_line, pc->column, pc->len, pc->str,
           get_token_name(pc->type), column, fcn_name, lineno);

   if (column == pc->column)
   {
      return;
   }

   int col_delta = column - pc->column;
   int min_col   = column;

   pc->column = column;
   do
   {
      chunk_t *next = chunk_get_next(pc);

      if (next == NULL)
      {
         break;
      }
      min_col += space_col_align(pc, next);
      pc       = next;

      bool is_comment = chunk_is_comment(pc);
      bool keep       = is_comment && chunk_is_single_line_comment(pc) &&
                        cpd.settings[UO_indent_relative_single_line_comments].b;

      if (is_comment && (pc->parent_type != CT_COMMENT_EMBED) && !keep)
      {
         pc->column = pc->orig_col;
         if (pc->column < min_col)
         {
            pc->column = min_col; // + 1;
         }
         LOG_FMT(LINDLINE, "%s: set comment on line %d to col %d (orig %d)\n",
                 __func__, pc->orig_line, pc->column, pc->orig_col);
      }
      else
      {
         pc->column += col_delta;
         if (pc->column < min_col)
         {
            pc->column = min_col;
         }
         LOG_FMT(LINDLINED, "   set column of '%.*s' to %d (orig %d)\n",
                 pc->len, pc->str, pc->column, pc->orig_col);
      }
   } while ((pc != NULL) && (pc->nl_count == 0));
}


/**
 * Starts a new entry
 *
 * @param frm  The parse frame
 * @param pc   The chunk causing the push
 */
static void indent_pse_push(struct parse_frame& frm, chunk_t *pc)
{
   static int ref = 0;

   /* check the stack depth */
   if (frm.pse_tos < ((int)ARRAY_SIZE(frm.pse) - 1))
   {
      /* Bump up the index and initialize it */
      frm.pse_tos++;
      memset(&frm.pse[frm.pse_tos], 0, sizeof(frm.pse[frm.pse_tos]));

      LOG_FMT(LINDPSE, "%4d] (pp=%d) OPEN  [%d,%s] level=%d\n",
              pc->orig_line, cpd.pp_level, frm.pse_tos, get_token_name(pc->type), pc->level);

      frm.pse[frm.pse_tos].pc         = pc;
      frm.pse[frm.pse_tos].type       = pc->type;
      frm.pse[frm.pse_tos].level      = pc->level;
      frm.pse[frm.pse_tos].open_line  = pc->orig_line;
      frm.pse[frm.pse_tos].ref        = ++ref;
      frm.pse[frm.pse_tos].in_preproc = (pc->flags & PCF_IN_PREPROC) != 0;
      frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos - 1].indent_tab;
      frm.pse[frm.pse_tos].non_vardef = false;
   }
}


/**
 * Removes the top entry
 *
 * @param frm  The parse frame
 * @param pc   The chunk causing the push
 */
static void indent_pse_pop(struct parse_frame& frm, chunk_t *pc)
{
   /* Bump up the index and initialize it */
   if (frm.pse_tos > 0)
   {
      if (pc != NULL)
      {
         LOG_FMT(LINDPSE, "%4d] (pp=%d) CLOSE [%d,%s] on %s, started on line %d, level=%d/%d\n",
                 pc->orig_line, cpd.pp_level, frm.pse_tos,
                 get_token_name(frm.pse[frm.pse_tos].type),
                 get_token_name(pc->type),
                 frm.pse[frm.pse_tos].open_line,
                 frm.pse[frm.pse_tos].level,
                 pc->level);
      }
      else
      {
         LOG_FMT(LINDPSE, " EOF] CLOSE [%d,%s], started on line %d\n",
                 frm.pse_tos, get_token_name(frm.pse[frm.pse_tos].type),
                 frm.pse[frm.pse_tos].open_line);
      }

      /* Don't clear the stack entry because some code 'cheats' and uses the
       * just-popped indent values
       */
      frm.pse_tos--;
   }
}


static int token_indent(c_token_t type)
{
   switch (type)
   {
   case CT_IF:
   case CT_DO:
      return(3);

   case CT_FOR:
   case CT_ELSE:  // wacky, but that's what is wanted
      return(4);

   case CT_WHILE:
      return(6);

   case CT_SWITCH:
      return(7);

   case CT_ELSEIF:
      return(8);

   default:
      return(0);
   }
}


#define indent_column_set(X)                              \
   do {                                                   \
      indent_column = (X);                                \
      LOG_FMT(LINDENT2, "[line %d] indent_column = %d\n", \
              __LINE__, indent_column);                   \
   } while (0)

/**
 * Change the top-level indentation only by changing the column member in
 * the chunk structures.
 * The level indicator must already be set.
 */
void indent_text(void)
{
   chunk_t            *pc;
   chunk_t            *next;
   chunk_t            *prev       = NULL;
   bool               did_newline = true;
   int                idx;
   int                vardefcol   = 0;
   int                indent_size = cpd.settings[UO_indent_columns].n;
   int                tmp;
   struct parse_frame frm;
   bool               in_preproc = false, was_preproc = false;
   int                indent_column;
   int                parent_token_indent = 0;
   int                xml_indent          = 0;
   bool               token_used;
   int                sql_col      = 0;
   int                sql_orig_col = 0;

   memset(&frm, 0, sizeof(frm));

   /* dummy top-level entry */
   frm.pse[0].indent     = 1;
   frm.pse[0].indent_tmp = 1;
   frm.pse[0].indent_tab = 1;
   frm.pse[0].type       = CT_EOF;

   pc = chunk_get_head();
   while (pc != NULL)
   {
      /* Handle proprocessor transitions */
      was_preproc = in_preproc;
      in_preproc  = (pc->flags & PCF_IN_PREPROC) != 0;

      if (cpd.settings[UO_indent_brace_parent].b)
      {
         parent_token_indent = token_indent(pc->parent_type);
      }

      /* Clean up after a #define, etc */
      if (!in_preproc)
      {
         while ((frm.pse_tos > 0) && frm.pse[frm.pse_tos].in_preproc)
         {
            c_token_t type = frm.pse[frm.pse_tos].type;
            indent_pse_pop(frm, pc);

            /* If we just removed an #endregion, then check to see if a
             * PP_REGION_INDENT entry is right below it
             */
            if ((type == CT_PP_ENDREGION) &&
                (frm.pse[frm.pse_tos].type == CT_PP_REGION_INDENT))
            {
               indent_pse_pop(frm, pc);
            }
         }
      }
      else if (pc->type == CT_PREPROC)
      {
         /* Close out PP_IF_INDENT before playing with the parse frames */
         if ((frm.pse[frm.pse_tos].type == CT_PP_IF_INDENT) &&
             ((pc->parent_type == CT_PP_ENDIF) ||
              (pc->parent_type == CT_PP_ELSE)))
         {
            indent_pse_pop(frm, pc);
         }

         pf_check(&frm, pc);

         /* Indent the body of a #region here */
         if (cpd.settings[UO_pp_region_indent_code].b &&
             (pc->parent_type == CT_PP_REGION))
         {
            next = chunk_get_next(pc);
            /* Hack to get the logs to look right */
            next->type = CT_PP_REGION_INDENT;
            indent_pse_push(frm, next);
            next->type = CT_PP_REGION;

            /* Indent one level */
            frm.pse[frm.pse_tos].indent     = frm.pse[frm.pse_tos - 1].indent + indent_size;
            frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos - 1].indent_tab + indent_size;
            frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
            frm.pse[frm.pse_tos].in_preproc = false;
         }

         /* Indent the body of a #if here */
         if (cpd.settings[UO_pp_if_indent_code].b &&
             ((pc->parent_type == CT_PP_IF) ||
              (pc->parent_type == CT_PP_ELSE)))
         {
            next = chunk_get_next(pc);
            /* Hack to get the logs to look right */
            next->type = CT_PP_IF_INDENT;
            indent_pse_push(frm, next);
            next->type = CT_PP_IF;

            /* Indent one level */
            frm.pse[frm.pse_tos].indent     = frm.pse[frm.pse_tos - 1].indent + indent_size;
            frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos - 1].indent_tab + indent_size;
            frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
            frm.pse[frm.pse_tos].in_preproc = false;
         }

         /* Transition into a preproc by creating a dummy indent */
         frm.level++;
         indent_pse_push(frm, chunk_get_next(pc));

         if (pc->parent_type == CT_PP_DEFINE)
         {
            frm.pse[frm.pse_tos].indent_tmp = cpd.settings[UO_pp_define_at_level].b ?
                                              frm.pse[frm.pse_tos - 1].indent_tmp : 1;
            frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos].indent_tmp + indent_size;
         }
         else
         {
            if ((frm.pse[frm.pse_tos - 1].type == CT_PP_REGION_INDENT) ||
                ((frm.pse[frm.pse_tos - 1].type == CT_PP_IF_INDENT) &&
                 (frm.pse[frm.pse_tos].type != CT_PP_ENDIF)))
            {
               frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 2].indent;
            }
            else
            {
               frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent;
            }
            if ((pc->parent_type == CT_PP_REGION) ||
                (pc->parent_type == CT_PP_ENDREGION))
            {
               int val = cpd.settings[UO_pp_indent_region].n;
               if (val > 0)
               {
                  frm.pse[frm.pse_tos].indent = val;
               }
               else
               {
                  frm.pse[frm.pse_tos].indent += val;
               }
            }
            else if ((pc->parent_type == CT_PP_IF) ||
                     (pc->parent_type == CT_PP_ELSE) ||
                     (pc->parent_type == CT_PP_ENDIF))
            {
               int val = cpd.settings[UO_pp_indent_if].n;
               if (val > 0)
               {
                  frm.pse[frm.pse_tos].indent = val;
               }
               else
               {
                  frm.pse[frm.pse_tos].indent += val;
               }
            }
            frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
         }
      }

      /* Check for close XML tags "</..." */
      if (cpd.settings[UO_indent_xml_string].n > 0)
      {
         if (pc->type == CT_STRING)
         {
            if ((pc->len > 4) &&
                (xml_indent > 0) &&
                (pc->str[1] == '<') &&
                (pc->str[2] == '/'))
            {
               xml_indent -= cpd.settings[UO_indent_xml_string].n;
            }
         }
         else
         {
            if (!chunk_is_comment(pc) && !chunk_is_newline(pc))
            {
               xml_indent = 0;
            }
         }
      }

      /**
       * Handle non-brace closures
       */

      token_used = false;
      int old_pse_tos;
      do
      {
         old_pse_tos = frm.pse_tos;

         /* End anything that drops a level
          * REVISIT: not sure about the preproc check
          */
         if (!chunk_is_newline(pc) &&
             !chunk_is_comment(pc) &&
             ((pc->flags & PCF_IN_PREPROC) == 0) &&
             (frm.pse[frm.pse_tos].level > pc->level))
         {
            indent_pse_pop(frm, pc);
         }

         if (frm.pse[frm.pse_tos].level >= pc->level)
         {
            /* process virtual braces closes (no text output) */
            if ((pc->type == CT_VBRACE_CLOSE) &&
                (frm.pse[frm.pse_tos].type == CT_VBRACE_OPEN))
            {
               indent_pse_pop(frm, pc);
               frm.level--;
               pc = chunk_get_next(pc);
            }

            /* End any assign operations with a semicolon on the same level */
            if (((frm.pse[frm.pse_tos].type == CT_ASSIGN_NL) ||
                 (frm.pse[frm.pse_tos].type == CT_ASSIGN)) &&
                (chunk_is_semicolon(pc) ||
                 (pc->type == CT_COMMA) ||
                 (pc->type == CT_BRACE_OPEN) ||
                 (pc->type == CT_SPAREN_CLOSE) ||
                 ((pc->type == CT_SQUARE_OPEN) && (pc->parent_type == CT_ASSIGN))))
            {
               indent_pse_pop(frm, pc);
            }

            /* End any assign operations with a semicolon on the same level */
            if (chunk_is_semicolon(pc) &&
                ((frm.pse[frm.pse_tos].type == CT_IMPORT) ||
                 (frm.pse[frm.pse_tos].type == CT_THROW) ||
                 (frm.pse[frm.pse_tos].type == CT_USING)))
            {
               indent_pse_pop(frm, pc);
            }

            /* End any custom macro-based open/closes */
            if (!token_used &&
                (frm.pse[frm.pse_tos].type == CT_MACRO_OPEN) &&
                (pc->type == CT_MACRO_CLOSE))
            {
               token_used = true;
               indent_pse_pop(frm, pc);
            }

            /* End any CPP/ObjC class colon stuff */
            if ((frm.pse[frm.pse_tos].type == CT_CLASS_COLON) &&
                ((pc->type == CT_BRACE_OPEN) ||
                 (pc->type == CT_OC_END) ||
                 (pc->type == CT_OC_SCOPE) ||
                 chunk_is_semicolon(pc)))
            {
               indent_pse_pop(frm, pc);
            }

            /* a case is ended with another case or a close brace */
            if ((frm.pse[frm.pse_tos].type == CT_CASE) &&
                ((pc->type == CT_BRACE_CLOSE) ||
                 (pc->type == CT_CASE)))
            {
               indent_pse_pop(frm, pc);
            }

            /* a class scope is ended with another class scope or a close brace */
            if (cpd.settings[UO_indent_access_spec_body].b &&
                (frm.pse[frm.pse_tos].type == CT_PRIVATE) &&
                ((pc->type == CT_BRACE_CLOSE) ||
                 (pc->type == CT_PRIVATE)))
            {
               indent_pse_pop(frm, pc);
            }

            /* a return is ended with a semicolon */
            if ((frm.pse[frm.pse_tos].type == CT_RETURN) &&
                chunk_is_semicolon(pc))
            {
               indent_pse_pop(frm, pc);
            }

            /* an SQL EXEC is ended with a semicolon */
            if ((frm.pse[frm.pse_tos].type == CT_SQL_EXEC) &&
                chunk_is_semicolon(pc))
            {
               indent_pse_pop(frm, pc);
            }

            /* Close out parens and squares */
            if ((frm.pse[frm.pse_tos].type == (pc->type - 1)) &&
                ((pc->type == CT_PAREN_CLOSE) ||
                 (pc->type == CT_SPAREN_CLOSE) ||
                 (pc->type == CT_FPAREN_CLOSE) ||
                 (pc->type == CT_SQUARE_CLOSE) ||
                 (pc->type == CT_ANGLE_CLOSE)))
            {
               indent_pse_pop(frm, pc);
               frm.paren_count--;
            }
         }
      } while (old_pse_tos > frm.pse_tos);

      /* Grab a copy of the current indent */
      indent_column_set(frm.pse[frm.pse_tos].indent_tmp);

      if (!chunk_is_newline(pc) && !chunk_is_comment(pc))
      {
         LOG_FMT(LINDPC, " -=[ %.*s ]=- top=%d %s %d/%d\n",
                 pc->len, pc->str,
                 frm.pse_tos,
                 get_token_name(frm.pse[frm.pse_tos].type),
                 frm.pse[frm.pse_tos].indent_tmp,
                 frm.pse[frm.pse_tos].indent);
      }

      /**
       * Handle stuff that can affect the current indent:
       *  - brace close
       *  - vbrace open
       *  - brace open
       *  - case         (immediate)
       *  - labels       (immediate)
       *  - class colons (immediate)
       *
       * And some stuff that can't
       *  - open paren
       *  - open square
       *  - assignment
       *  - return
       */

      bool brace_indent = false;
      if ((pc->type == CT_BRACE_CLOSE) || (pc->type == CT_BRACE_OPEN))
      {
         brace_indent = (cpd.settings[UO_indent_braces].b &&
                         (!cpd.settings[UO_indent_braces_no_func].b ||
                          (pc->parent_type != CT_FUNC_DEF)));
      }

      if (pc->type == CT_BRACE_CLOSE)
      {
         if (frm.pse[frm.pse_tos].type == CT_BRACE_OPEN)
         {
            /* Indent the brace to match the open brace */
            indent_column_set(frm.pse[frm.pse_tos].brace_indent);

            indent_pse_pop(frm, pc);
            frm.level--;
         }
      }
      else if (pc->type == CT_VBRACE_OPEN)
      {
         frm.level++;
         indent_pse_push(frm, pc);

         frm.pse[frm.pse_tos].indent     = frm.pse[frm.pse_tos - 1].indent + indent_size;
         frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
         frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos].indent;

         /* Always indent on virtual braces */
         indent_column_set(frm.pse[frm.pse_tos].indent_tmp);
      }
      else if (pc->type == CT_BRACE_OPEN)
      {
         frm.level++;
         indent_pse_push(frm, pc);

         if (frm.paren_count != 0)
         {
            /* We are inside ({ ... }) -- indent one tab from the paren */
            frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent_tmp + indent_size;
         }
         else
         {
            /* Use the prev indent level + indent_size. */
            frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent + indent_size;

            /* If this brace is part of a statement, bump it out by indent_brace */
            if ((pc->parent_type == CT_IF) ||
                (pc->parent_type == CT_ELSE) ||
                (pc->parent_type == CT_ELSEIF) ||
                (pc->parent_type == CT_TRY) ||
                (pc->parent_type == CT_CATCH) ||
                (pc->parent_type == CT_DO) ||
                (pc->parent_type == CT_WHILE) ||
                (pc->parent_type == CT_SWITCH) ||
                (pc->parent_type == CT_FOR))
            {
               if (parent_token_indent != 0)
               {
                  frm.pse[frm.pse_tos].indent += parent_token_indent - indent_size;
               }
               else
               {
                  frm.pse[frm.pse_tos].indent += cpd.settings[UO_indent_brace].n;
                  indent_column_set(indent_column + cpd.settings[UO_indent_brace].n);
               }
            }
            else if (pc->parent_type == CT_CASE)
            {
               /* An open brace with the parent of case does not indent by default
                * UO_indent_case_brace can be used to indent the brace.
                * So we need to take the CASE indent, subtract off the
                * indent_size that was added above and then add indent_case_brace.
                */
               indent_column_set(frm.pse[frm.pse_tos - 1].indent - indent_size +
                                 cpd.settings[UO_indent_case_brace].n);

               /* Stuff inside the brace still needs to be indented */
               frm.pse[frm.pse_tos].indent     = indent_column + indent_size;
               frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
            }
            else if ((pc->parent_type == CT_CLASS) && !cpd.settings[UO_indent_class].b)
            {
               frm.pse[frm.pse_tos].indent -= indent_size;
            }
            else if (pc->parent_type == CT_NAMESPACE)
            {
               if ((pc->flags & PCF_LONG_BLOCK) ||
                   !cpd.settings[UO_indent_namespace].b)
               {
                  /* don't indent long blocks */
                  frm.pse[frm.pse_tos].indent -= indent_size;
               }
               else /* indenting 'short' namespace */
               {
                  if (cpd.settings[UO_indent_namespace_level].n > 0)
                  {
                     frm.pse[frm.pse_tos].indent -= indent_size;
                     frm.pse[frm.pse_tos].indent +=
                        cpd.settings[UO_indent_namespace_level].n;
                  }
               }
            }
            else if ((pc->parent_type == CT_EXTERN) && !cpd.settings[UO_indent_extern].b)
            {
               frm.pse[frm.pse_tos].indent -= indent_size;
            }

            frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos].indent;
         }

         if ((pc->flags & PCF_DONT_INDENT) != 0)
         {
            frm.pse[frm.pse_tos].indent = pc->column;
            indent_column_set(pc->column);
         }
         else
         {
            /**
             * If there isn't a newline between the open brace and the next
             * item, just indent to wherever the next token is.
             * This covers this sort of stuff:
             * { a++;
             *   b--; };
             */
            next = chunk_get_next_ncnl(pc);
            if (!chunk_is_newline_between(pc, next))
            {
               frm.pse[frm.pse_tos].indent = next->column;
            }
            frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
            frm.pse[frm.pse_tos].open_line  = pc->orig_line;

            /* Update the indent_column if needed */
            if (brace_indent || (parent_token_indent != 0))
            {
               indent_column_set(frm.pse[frm.pse_tos].indent_tmp);
            }
         }

         /* Save the brace indent */
         frm.pse[frm.pse_tos].brace_indent = indent_column;
      }
      else if (pc->type == CT_SQL_END)
      {
         if (frm.pse[frm.pse_tos].type == CT_SQL_BEGIN)
         {
            indent_pse_pop(frm, pc);
            frm.level--;
            indent_column_set(frm.pse[frm.pse_tos].indent_tmp);
         }
      }
      else if (pc->type == CT_SQL_BEGIN)
      {
         frm.level++;
         indent_pse_push(frm, pc);
         frm.pse[frm.pse_tos].indent     = frm.pse[frm.pse_tos - 1].indent + indent_size;
         frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
         frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos].indent;
      }
      else if (pc->type == CT_SQL_EXEC)
      {
         frm.level++;
         indent_pse_push(frm, pc);
         frm.pse[frm.pse_tos].indent     = frm.pse[frm.pse_tos - 1].indent + indent_size;
         frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
      }
      else if (pc->type == CT_MACRO_OPEN)
      {
         frm.level++;
         indent_pse_push(frm, pc);
         frm.pse[frm.pse_tos].indent     = frm.pse[frm.pse_tos - 1].indent + indent_size;
         frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
         frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos].indent;
      }
      else if (pc->type == CT_MACRO_ELSE)
      {
         if (frm.pse[frm.pse_tos].type == CT_MACRO_OPEN)
         {
            indent_column_set(frm.pse[frm.pse_tos - 1].indent);
         }
      }
      else if (pc->type == CT_CASE)
      {
         /* Start a case - indent UO_indent_switch_case from the switch level */
         tmp = frm.pse[frm.pse_tos].indent + cpd.settings[UO_indent_switch_case].n;

         indent_pse_push(frm, pc);

         frm.pse[frm.pse_tos].indent     = tmp;
         frm.pse[frm.pse_tos].indent_tmp = tmp - indent_size + cpd.settings[UO_indent_case_shift].n;
         frm.pse[frm.pse_tos].indent_tab = tmp;

         /* Always set on case statements */
         indent_column_set(frm.pse[frm.pse_tos].indent_tmp);

         /* comments before 'case' need to be aligned with the 'case' */
         chunk_t *pct = pc;
         while (((pct = chunk_get_prev_nnl(pct)) != NULL) &&
                chunk_is_comment(pct))
         {
            chunk_t *t2 = chunk_get_prev(pct);
            if (chunk_is_newline(t2))
            {
               pct->column        = frm.pse[frm.pse_tos].indent_tmp;
               pct->column_indent = pct->column;
            }
         }
      }
      else if (pc->type == CT_BREAK)
      {
         prev = chunk_get_prev_ncnl(pc);
         if ((prev != NULL) &&
             (prev->type == CT_BRACE_CLOSE) &&
             (prev->parent_type == CT_CASE))
         {
            /* This only affects the 'break', so no need for a stack entry */
            indent_column_set(prev->column);
         }
      }
      else if (pc->type == CT_LABEL)
      {
         /* Labels get sent to the left or backed up */
         if (cpd.settings[UO_indent_label].n > 0)
         {
            indent_column_set(cpd.settings[UO_indent_label].n);
         }
         else
         {
            indent_column_set(frm.pse[frm.pse_tos].indent +
                              cpd.settings[UO_indent_label].n);
         }
      }
      else if (pc->type == CT_PRIVATE)
      {
         if (cpd.settings[UO_indent_access_spec_body].b)
         {
            tmp = frm.pse[frm.pse_tos].indent + indent_size;

            indent_pse_push(frm, pc);

            frm.pse[frm.pse_tos].indent     = tmp;
            frm.pse[frm.pse_tos].indent_tmp = tmp - indent_size;
            frm.pse[frm.pse_tos].indent_tab = tmp;

            /* If we are indenting the body, then we must leave the access spec
             * indented at brace level
             */
            indent_column_set(frm.pse[frm.pse_tos].indent_tmp);
         }
         else
         {
            /* Access spec labels get sent to the left or backed up */
            if (cpd.settings[UO_indent_access_spec].n > 0)
            {
               indent_column_set(cpd.settings[UO_indent_access_spec].n);
            }
            else
            {
               indent_column_set(frm.pse[frm.pse_tos].indent +
                                 cpd.settings[UO_indent_access_spec].n);
            }
         }
      }
      else if (pc->type == CT_CLASS_COLON)
      {
         /* just indent one level */
         indent_pse_push(frm, pc);
         frm.pse[frm.pse_tos].indent     = frm.pse[frm.pse_tos - 1].indent_tmp + indent_size;
         frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
         frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos].indent;

         indent_column_set(frm.pse[frm.pse_tos].indent_tmp);

         if (cpd.settings[UO_indent_class_colon].b)
         {
            prev = chunk_get_prev(pc);
            if (chunk_is_newline(prev))
            {
               frm.pse[frm.pse_tos].indent += 2;
               /* don't change indent of current line */
            }
            else
            {
               next = chunk_get_next(pc);
               if ((next != NULL) && !chunk_is_newline(next))
               {
                  frm.pse[frm.pse_tos].indent = next->column;
               }
            }
         }
      }
      else if ((pc->type == CT_PAREN_OPEN) ||
               (pc->type == CT_SPAREN_OPEN) ||
               (pc->type == CT_FPAREN_OPEN) ||
               (pc->type == CT_SQUARE_OPEN) ||
               (pc->type == CT_ANGLE_OPEN))
      {
         /* Open parens and squares - never update indent_column, unless right
          * after a newline.
          */
         indent_pse_push(frm, pc);
         frm.pse[frm.pse_tos].indent = pc->column + pc->len;

         if (((pc->type == CT_FPAREN_OPEN) || (pc->type == CT_ANGLE_OPEN)) &&
             ((cpd.settings[UO_indent_func_call_param].b &&
               ((pc->parent_type == CT_FUNC_CALL) ||
                (pc->parent_type == CT_FUNC_CALL_USER)))
              ||
              (cpd.settings[UO_indent_func_proto_param].b &&
               ((pc->parent_type == CT_FUNC_PROTO) ||
                (pc->parent_type == CT_FUNC_CLASS)))
              ||
              (cpd.settings[UO_indent_func_class_param].b &&
               (pc->parent_type == CT_FUNC_CLASS))
              ||
              (cpd.settings[UO_indent_template_param].b &&
               (pc->parent_type == CT_TEMPLATE))
              ||
              (cpd.settings[UO_indent_func_ctor_var_param].b &&
               (pc->parent_type == CT_FUNC_CTOR_VAR))
              ||
              (cpd.settings[UO_indent_func_def_param].b &&
               (pc->parent_type == CT_FUNC_DEF))))
         {
            /* Skip any continuation indents */
            idx = frm.pse_tos - 1;
            while ((idx > 0) &&
                   (frm.pse[idx].type != CT_BRACE_OPEN) &&
                   (frm.pse[idx].type != CT_VBRACE_OPEN) &&
                   (frm.pse[idx].type != CT_PAREN_OPEN) &&
                   (frm.pse[idx].type != CT_FPAREN_OPEN) &&
                   (frm.pse[idx].type != CT_SPAREN_OPEN) &&
                   (frm.pse[idx].type != CT_SQUARE_OPEN) &&
                   (frm.pse[idx].type != CT_ANGLE_OPEN) &&
                   (frm.pse[idx].type != CT_CLASS_COLON) &&
                   (frm.pse[idx].type != CT_ASSIGN_NL))
            {
               idx--;
            }
            frm.pse[frm.pse_tos].indent = frm.pse[idx].indent + indent_size;
            if (cpd.settings[UO_indent_func_param_double].b)
            {
               frm.pse[frm.pse_tos].indent += indent_size;
            }
            frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos].indent;
         }

         else if ((chunk_is_str(pc, "(", 1) && !cpd.settings[UO_indent_paren_nl].b) ||
                  (chunk_is_str(pc, "<", 1) && !cpd.settings[UO_indent_paren_nl].b) || /* TODO: add indent_angle_nl? */
                  (chunk_is_str(pc, "[", 1) && !cpd.settings[UO_indent_square_nl].b))
         {
            next = chunk_get_next_nc(pc);
            if (chunk_is_newline(next))
            {
               int sub = 1;
               if (frm.pse[frm.pse_tos - 1].type == CT_ASSIGN)
               {
                  sub = 2;
               }
               frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - sub].indent + indent_size;
            }
            else
            {
               if (!chunk_is_comment(next))
               {
                  frm.pse[frm.pse_tos].indent = next->column;
               }
            }
         }

         if ((pc->type == CT_FPAREN_OPEN) &&
             chunk_is_newline(chunk_get_prev(pc)) &&
             !chunk_is_newline(chunk_get_next(pc)))
         {
            frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent + indent_size;
            indent_column_set(frm.pse[frm.pse_tos].indent);
         }
         frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
         frm.paren_count++;
      }
      else if ((pc->type == CT_ASSIGN) ||
               (pc->type == CT_IMPORT) ||
               (pc->type == CT_USING))
      {
         /**
          * if there is a newline after the '=' or the line starts with a '=',
          * just indent one level,
          * otherwise align on the '='.
          */
         if ((pc->type == CT_ASSIGN) && chunk_is_newline(chunk_get_prev(pc)))
         {
            frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent + indent_size;
            indent_column_set(frm.pse[frm.pse_tos].indent_tmp);
            LOG_FMT(LINDENT, "%s: %d] assign => %d [%.*s]\n",
                    __func__, pc->orig_line, indent_column, pc->len, pc->str);
            reindent_line(pc, frm.pse[frm.pse_tos].indent_tmp);
         }

         next = chunk_get_next(pc);
         if (next != NULL)
         {
            indent_pse_push(frm, pc);
            if (chunk_is_newline(next) || !cpd.settings[UO_indent_align_assign].b)
            {
               frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent_tmp + indent_size;
               if (pc->type == CT_ASSIGN)
               {
                  frm.pse[frm.pse_tos].type = CT_ASSIGN_NL;
               }
            }
            else
            {
               frm.pse[frm.pse_tos].indent = pc->column + pc->len + 1;
            }
            frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
         }
      }
      else if (pc->type == CT_RETURN)
      {
         /* don't count returns inside a () or [] */
         if (pc->level == pc->brace_level)
         {
            indent_pse_push(frm, pc);
            if (chunk_is_newline(chunk_get_next(pc)))
            {
               frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent + indent_size;
            }
            else
            {
               frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent + pc->len + 1;
            }
            frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos - 1].indent;
         }
      }
      else if (pc->type == CT_THROW)
      {
         // Pick up what was just before this.
         prev = chunk_get_prev(pc);
         if (pc->parent_type == CT_FUNC_PROTO)
         {
            indent_pse_push(frm, pc);

            if ((cpd.settings[UO_indent_func_throw].n != 0) &&
                ((prev == NULL) || (prev->type == CT_NEWLINE)))
            {
               frm.pse[frm.pse_tos].indent = cpd.settings[UO_indent_func_throw].n;
            }
            else
            {
               frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent_tmp + indent_size;
            }
            frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
            frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos].indent;

            indent_column_set(frm.pse[frm.pse_tos].indent_tmp);
         }
         else if (pc->parent_type == CT_FUNC_DEF)
         {
            if ((cpd.settings[UO_indent_func_throw].n != 0) &&
                ((prev == NULL) || (prev->type == CT_NEWLINE)))
            {
               indent_column_set(cpd.settings[UO_indent_func_throw].n);
               LOG_FMT(LINDENT, "%s: %d] throw => %d [%.*s]\n",
                       __func__, pc->orig_line, indent_column, pc->len, pc->str);
               reindent_line(pc, indent_column);
            }
         }
      }
      else
      {
         /* anything else? */
      }

      /**
       * Indent the line if needed
       */
      if (did_newline && !chunk_is_newline(pc) && (pc->len != 0))
      {
         pc->column_indent = frm.pse[frm.pse_tos].indent_tab;

         LOG_FMT(LINDENT2, "%s: %d] %d for %.*s\n",
                 __func__, pc->orig_line, pc->column_indent, pc->len, pc->str);

         /**
          * Check for special continuations.
          * Note that some of these could be done as a stack item like
          * everything else
          */

         prev = chunk_get_prev_ncnl(pc);
         next = chunk_get_next_ncnl(pc);
         if ((pc->flags & PCF_DONT_INDENT) != 0)
         {
            /* no change */
         }
         else if ((pc->parent_type == CT_SQL_EXEC) &&
                  cpd.settings[UO_indent_preserve_sql].b)
         {
            reindent_line(pc, sql_col + (pc->orig_col - sql_orig_col));
            LOG_FMT(LINDENT, "Indent SQL: [%.*s] to %d (%d/%d)\n",
                    pc->len, pc->str, pc->column, sql_col, sql_orig_col);
         }
         else if (((pc->flags & PCF_STMT_START) == 0) &&
                  ((pc->type == CT_MEMBER) ||
                   (pc->type == CT_DC_MEMBER) ||
                   ((prev != NULL) &&
                    ((prev->type == CT_MEMBER) ||
                     (prev->type == CT_DC_MEMBER)))))
         {
            tmp = cpd.settings[UO_indent_member].n + indent_column;
            LOG_FMT(LINDENT, "%s: %d] member => %d\n",
                    __func__, pc->orig_line, tmp);
            reindent_line(pc, tmp);
         }
         else if ((vardefcol > 0) &&
                  (pc->level == pc->brace_level) &&
                  (pc->type == CT_WORD) &&
                  ((pc->flags & PCF_VAR_DEF) != 0) &&
                  (prev != NULL) && (prev->type == CT_COMMA))
         {
            LOG_FMT(LINDENT, "%s: %d] Vardefcol => %d\n",
                    __func__, pc->orig_line, vardefcol);
            reindent_line(pc, vardefcol);
         }
         else if ((pc->type == CT_STRING) && (prev->type == CT_STRING) &&
                  cpd.settings[UO_indent_align_string].b)
         {
            tmp = (xml_indent != 0) ? xml_indent : prev->column;

            LOG_FMT(LINDENT, "%s: %d] String => %d\n",
                    __func__, pc->orig_line, tmp);
            reindent_line(pc, tmp);
         }
         else if (chunk_is_comment(pc))
         {
            LOG_FMT(LINDENT, "%s: %d] comment => %d\n",
                    __func__, pc->orig_line, frm.pse[frm.pse_tos].indent_tmp);
            indent_comment(pc, frm.pse[frm.pse_tos].indent_tmp);
         }
         else if (pc->type == CT_PREPROC)
         {
            LOG_FMT(LINDENT, "%s: %d] pp-indent => %d [%.*s]\n",
                    __func__, pc->orig_line, indent_column, pc->len, pc->str);
            reindent_line(pc, indent_column);
         }
         else if (chunk_is_paren_close(pc) || (pc->type == CT_ANGLE_CLOSE))
         {
            /* This is a big hack. We assume that since we hit a paren close,
             * that we just removed a paren open */
            if (frm.pse[frm.pse_tos + 1].type == c_token_t(pc->type - 1))
            {
               chunk_t *ck1 = frm.pse[frm.pse_tos + 1].pc;
               chunk_t *ck2 = chunk_get_prev(ck1);

               /* If the open paren was the first thing on the line or we are
               * doing mode 1, then put the close paren in the same column */
               if (chunk_is_newline(ck2) ||
                   (cpd.settings[UO_indent_paren_close].n == 1))
               {
                  indent_column_set(ck1->column);
               }
               else
               {
                  if (cpd.settings[UO_indent_paren_close].n != 2)
                  {
                     indent_column_set(frm.pse[frm.pse_tos + 1].indent_tmp);
                     if (cpd.settings[UO_indent_paren_close].n == 1)
                     {
                        indent_column--;
                     }
                  }
               }
            }
            LOG_FMT(LINDENT, "%s: %d] cl paren => %d [%.*s]\n",
                    __func__, pc->orig_line, indent_column, pc->len, pc->str);
            reindent_line(pc, indent_column);
         }
         else if (pc->type == CT_COMMA)
         {
            if (cpd.settings[UO_indent_comma_paren].b &&
                chunk_is_paren_open(frm.pse[frm.pse_tos].pc))
            {
               indent_column_set(frm.pse[frm.pse_tos].pc->column);
            }
            LOG_FMT(LINDENT, "%s: %d] comma => %d [%.*s]\n",
                    __func__, pc->orig_line, indent_column, pc->len, pc->str);
            reindent_line(pc, indent_column);
         }
         else if (cpd.settings[UO_indent_func_const].n &&
                  (pc->type == CT_QUALIFIER) &&
                  (strncasecmp(pc->str, "const", pc->len) == 0) &&
                  ((next == NULL) ||
                   (next->type == CT_BRACED) ||
                   (next->type == CT_BRACE_OPEN) ||
                   (next->type == CT_NEWLINE) ||
                   (next->type == CT_SEMICOLON) ||
                   (next->type == CT_THROW) ||
                   (next->type == CT_VBRACE_OPEN)))
         {
            // indent const - void GetFoo(void)\n const\n { return (m_Foo); }
            indent_column_set(cpd.settings[UO_indent_func_const].n);
            LOG_FMT(LINDENT, "%s: %d] const => %d [%.*s]\n",
                    __func__, pc->orig_line, indent_column, pc->len, pc->str);
            reindent_line(pc, indent_column);
         }
         else if (pc->type == CT_BOOL)
         {
            if (cpd.settings[UO_indent_bool_paren].b &&
                chunk_is_paren_open(frm.pse[frm.pse_tos].pc))
            {
               indent_column_set(frm.pse[frm.pse_tos].pc->column);
            }
            LOG_FMT(LINDENT, "%s: %d] bool => %d [%.*s]\n",
                    __func__, pc->orig_line, indent_column, pc->len, pc->str);
            reindent_line(pc, indent_column);
         }
         else
         {
            if (pc->column != indent_column)
            {
               LOG_FMT(LINDENT, "%s: %d] indent => %d [%.*s]\n",
                       __func__, pc->orig_line, indent_column, pc->len, pc->str);
               reindent_line(pc, indent_column);
            }
         }
         did_newline = false;

         if ((pc->type == CT_SQL_EXEC) ||
             (pc->type == CT_SQL_BEGIN) ||
             (pc->type == CT_SQL_END))
         {
            sql_col      = pc->column;
            sql_orig_col = pc->orig_col;
         }

         /* Handle indent for variable defs at the top of a block of code */
         if (pc->flags & PCF_VAR_TYPE)
         {
            if (!frm.pse[frm.pse_tos].non_vardef &&
                (frm.pse[frm.pse_tos].type == CT_BRACE_OPEN))
            {
               int tmp = indent_column;
               if (cpd.settings[UO_indent_var_def_blk].n > 0)
               {
                  tmp = cpd.settings[UO_indent_var_def_blk].n;
               }
               else
               {
                  tmp += cpd.settings[UO_indent_var_def_blk].n;
               }
               reindent_line(pc, tmp);
               LOG_FMT(LINDENT, "%s: %d] var_type indent => %d [%.*s]\n",
                       __func__, pc->orig_line, tmp, pc->len, pc->str);
            }
         }
         else
         {
            if (pc != frm.pse[frm.pse_tos].pc)
            {
               frm.pse[frm.pse_tos].non_vardef = true;
            }
         }
      }

      /**
       * Handle variable definition continuation indenting
       */
      if ((pc->type == CT_WORD) &&
          ((pc->flags & PCF_IN_FCN_DEF) == 0) &&
          ((pc->flags & PCF_VAR_1ST_DEF) == PCF_VAR_1ST_DEF))
      {
         vardefcol = pc->column;
      }
      if (chunk_is_semicolon(pc) ||
          ((pc->type == CT_BRACE_OPEN) && (pc->parent_type == CT_FUNCTION)))
      {
         vardefcol = 0;
      }

      /* if we hit a newline, reset indent_tmp */
      if (chunk_is_newline(pc) ||
          (pc->type == CT_COMMENT_MULTI) ||
          (pc->type == CT_COMMENT_CPP))
      {
         frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;

         /**
          * Handle the case of a multi-line #define w/o anything on the
          * first line (indent_tmp will be 1 or 0)
          */
         if ((pc->type == CT_NL_CONT) &&
             (frm.pse[frm.pse_tos].indent_tmp <= indent_size))
         {
            frm.pse[frm.pse_tos].indent_tmp = indent_size + 1;
         }

         /* Get ready to indent the next item */
         did_newline = true;
      }

      /* Check for open XML tags "</..." */
      if (cpd.settings[UO_indent_xml_string].n > 0)
      {
         if (pc->type == CT_STRING)
         {
            if ((pc->len > 4) &&
                (pc->str[1] == '<') &&
                (pc->str[2] != '/') &&
                (pc->str[pc->len - 3] != '/'))
            {
               if (xml_indent <= 0)
               {
                  xml_indent = pc->column;
               }
               xml_indent += cpd.settings[UO_indent_xml_string].n;
            }
         }
      }

      if (!chunk_is_comment(pc) && !chunk_is_newline(pc))
      {
         prev = pc;
      }
      pc = chunk_get_next(pc);
   }

   /* Throw out any stuff inside a preprocessor - no need to warn */
   while ((frm.pse_tos > 0) && frm.pse[frm.pse_tos].in_preproc)
   {
      indent_pse_pop(frm, pc);
   }

   for (idx = 1; idx <= frm.pse_tos; idx++)
   {
      LOG_FMT(LWARN, "%s:%d Unmatched %s\n",
              cpd.filename, frm.pse[idx].open_line,
              get_token_name(frm.pse[idx].type));
      cpd.error_count++;
   }

   quick_align_again();
}


/**
 * returns true if forward scan reveals only single newlines or comments
 * stops when hits code
 * false if next thing hit is a closing brace, also if 2 newlines in a row
 */
static bool single_line_comment_indent_rule_applies(chunk_t *start)
{
   chunk_t *pc      = start;
   int     nl_count = 0;

   if (!chunk_is_single_line_comment(pc))
   {
      return(false);
   }

   /* scan forward, if only single newlines and comments before next line of
    * code, we want to apply */
   while ((pc = chunk_get_next(pc)) != NULL)
   {
      if (chunk_is_newline(pc))
      {
         if ((nl_count > 0) || (pc->nl_count > 1))
         {
            return(false);
         }

         nl_count++;
      }
      else
      {
         nl_count = 0;
         if (!chunk_is_single_line_comment(pc))
         {
            /* here we check for things to run into that we wouldn't want to
             * indent the comment for.  for example, non-single line comment,
             * closing brace */
            if (chunk_is_comment(pc) || chunk_is_closing_brace(pc))
            {
               return(false);
            }

            return(true);
         }
      }
   }

   return(false);
}


/**
 * REVISIT: This needs to be re-checked, maybe cleaned up
 *
 * Indents comments in a (hopefully) smart manner.
 *
 * There are two type of comments that get indented:
 *  - stand alone (ie, no tokens on the line before the comment)
 *  - trailing comments (last token on the line apart from a linefeed)
 *    + note that a stand-alone comment is a special case of a trailing
 *
 * The stand alone comments will get indented in one of three ways:
 *  - column 1:
 *    + There is an empty line before the comment AND the indent level is 0
 *    + The comment was originally in column 1
 *
 *  - Same column as trailing comment on previous line (ie, aligned)
 *    + if originally within TBD (3) columns of the previous comment
 *
 *  - syntax indent level
 *    + doesn't fit in the previous categories
 *
 * Options modify this behavior:
 *  - keep original column (don't move the comment, if possible)
 *  - keep relative column (move out the same amount as first item on line)
 *  - fix trailing comment in column TBD
 *
 * @param pc   The comment, which is the first item on a line
 * @param col  The column if this is to be put at indent level
 */
static void indent_comment(chunk_t *pc, int col)
{
   chunk_t *nl;
   chunk_t *prev;

   LOG_FMT(LCMTIND, "%s: line %d, col %d, level %d: ", __func__,
           pc->orig_line, pc->orig_col, pc->level);

   /* force column 1 comment to column 1 if not changing them */
   if ((pc->orig_col == 1) && !cpd.settings[UO_indent_col1_comment].b &&
       ((pc->flags & PCF_INSERTED) == 0))
   {
      LOG_FMT(LCMTIND, "rule 1 - keep in col 1\n");
      reindent_line(pc, 1);
      return;
   }

   nl = chunk_get_prev(pc);

   /* outside of any expression or statement? */
   if (pc->level == 0)
   {
      if ((nl != NULL) && (nl->nl_count > 1))
      {
         LOG_FMT(LCMTIND, "rule 2 - level 0, nl before\n");
         reindent_line(pc, 1);
         return;
      }
   }

   prev = chunk_get_prev(nl);
   if (chunk_is_comment(prev) && (nl->nl_count == 1))
   {
      int coldiff = prev->orig_col - pc->orig_col;

      if ((coldiff <= 3) && (coldiff >= -3))
      {
         reindent_line(pc, prev->column);
         LOG_FMT(LCMTIND, "rule 3 - prev comment, coldiff = %d, now in %d\n",
                 coldiff, pc->column);
         return;
      }
   }

   /* check if special single line comment rule applies */
   if ((cpd.settings[UO_indent_sing_line_comments].n > 0) &&
       single_line_comment_indent_rule_applies(pc))
   {
      reindent_line(pc, col + cpd.settings[UO_indent_sing_line_comments].n);
      LOG_FMT(LCMTIND, "rule 4 - single line comment indent, now in %d\n", pc->column);
      return;
   }
   LOG_FMT(LCMTIND, "rule 5 - fall-through, stay in %d\n", col);

   reindent_line(pc, col);
}


/**
 * Scan to see if the whole file is covered by one #ifdef
 */
static bool ifdef_over_whole_file()
{
   chunk_t *pc;
   chunk_t *next;

   int stage = 0;

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next(pc))
   {
      if (chunk_is_comment(pc) || chunk_is_newline(pc))
      {
         continue;
      }

      if (stage == 0)
      {
         /* Check the first PP, make sure it is an #if type */
         if (pc->type != CT_PREPROC)
         {
            break;
         }
         next = chunk_get_next(pc);
         if ((next == NULL) || (next->type != CT_PP_IF))
         {
            break;
         }
         stage = 1;
      }
      else if (stage == 1)
      {
         /* Scan until a PP at level 0 is found - the close to the #if */
         if ((pc->type == CT_PREPROC) &&
             (pc->pp_level == 0))
         {
            stage = 2;
         }
         continue;
      }
      else if (stage == 2)
      {
         /* We should only see the rest of the preprocessor */
         if ((pc->type == CT_PREPROC) ||
             ((pc->flags & PCF_IN_PREPROC) == 0))
         {
            stage = 0;
            break;
         }
      }
   }

   if (stage == 2)
   {
      LOG_FMT(LINFO, "The whole file is covered by a #IF\n");
      return(true);
   }
   return(stage == 2);
}


/**
 * Indent the preprocessor stuff from column 1.
 * FIXME: This is broken if there is a comment or escaped newline
 * between '#' and 'define'.
 */
void indent_preproc(void)
{
   chunk_t *pc;
   chunk_t *next;
   int     pp_level;
   int     pp_level_sub = 0;

   /* Scan to see if the whole file is covered by one #ifdef */
   if (ifdef_over_whole_file())
   {
      pp_level_sub = 1;
   }

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next(pc))
   {
      if (pc->type != CT_PREPROC)
      {
         continue;
      }

      next = chunk_get_next_ncnl(pc);

      pp_level = pc->pp_level - pp_level_sub;
      if (pp_level < 0)
      {
         pp_level = 0;
      }

      /* Adjust the indent of the '#' */
      if ((cpd.settings[UO_pp_indent].a & AV_ADD) != 0)
      {
         reindent_line(pc, 1 + pp_level * cpd.settings[UO_pp_indent_count].n);
      }
      else if ((cpd.settings[UO_pp_indent].a & AV_REMOVE) != 0)
      {
         reindent_line(pc, 1);
      }

      /* Add spacing by adjusting the length */
      if ((cpd.settings[UO_pp_space].a != AV_IGNORE) && (next != NULL))
      {
         if ((cpd.settings[UO_pp_space].a & AV_ADD) != 0)
         {
            int mult = cpd.settings[UO_pp_space_count].n;

            if (mult < 1)
            {
               mult = 1;
            }
            reindent_line(next, pc->column + pc->len + (pp_level * mult));
         }
         else if ((cpd.settings[UO_pp_space].a & AV_REMOVE) != 0)
         {
            reindent_line(next, pc->column + pc->len);
         }
      }

      /* Mark as already handled if not region stuff or in column 1 */
      if ((!cpd.settings[UO_pp_indent_at_level].b ||
           (pc->brace_level <= (pc->parent_type == CT_PP_DEFINE ? 1 : 0))) &&
          (pc->parent_type != CT_PP_REGION) &&
          (pc->parent_type != CT_PP_ENDREGION))
      {
         if (!cpd.settings[UO_pp_define_at_level].b ||
             (pc->parent_type != CT_PP_DEFINE))
         {
            pc->flags |= PCF_DONT_INDENT;
         }
      }

      LOG_FMT(LPPIS, "%s: Indent line %d to %d (len %d, next->col %d)\n",
              __func__, pc->orig_line, 1 + pp_level, pc->len, next->column);
   }
}
