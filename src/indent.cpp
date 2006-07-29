/**
 * @file c_indent.c
 * Does all the indenting stuff.
 *
 * $Id$
 */
#include "uncrustify_types.h"
#include "chunk_list.h"
#include "prototypes.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <cctype>

//#define DBG_BRACE

static void indent_comment(chunk_t *pc, int col);


void indent_to_column(chunk_t *pc, int column)
{
   if (column < pc->column)
   {
      column = pc->column;
   }
   reindent_line(pc, column);
}

/**
 * Changes the initial indent for a line to the given column
 *
 * @param pc      The chunk at the start of the line
 * @param column  The desired column
 */
void reindent_line(chunk_t *pc, int column)
{
   int col_delta;
   int min_col;

   LOG_FMT(LINDLINE, "%s: %d] col %d on %.*s [%s]\n",
           __func__, pc->orig_line, pc->column, pc->len, pc->str,
           get_token_name(pc->type));

   if (column == pc->column)
   {
      return;
   }
   col_delta  = column - pc->column;
   pc->column = column;
   min_col    = pc->column;

   do
   {
      min_col += pc->len;
      pc       = chunk_get_next(pc);
      if (pc != NULL)
      {
         if (chunk_is_comment(pc))
         {
            pc->column = pc->orig_col;
            if (pc->column < min_col)
            {
               pc->column = min_col + 1;
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
         }
      }
   } while ((pc != NULL) &&
            (pc->type != CT_NEWLINE) &&
            (pc->type != CT_NL_CONT) &&
            (pc->type != CT_COMMENT_MULTI));
}


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
   int                vardefcol = 0;
   int                tabsize   = cpd.settings[UO_output_tab_size].n;
   int                ref       = 0;
   int                tmp;
   struct parse_frame frm;
   bool               in_preproc = false, was_preproc = false;
   int                indent_column;

   memset(&frm, 0, sizeof(frm));

   /* dummy top-level entry */
   frm.pse[0].indent     = 1;
   frm.pse[0].indent_tmp = 1;
   frm.pse[0].type       = CT_EOF;

   pc = chunk_get_head();
   while (pc != NULL)
   {
      LOG_FMT(LINDENT, "%s: %d] %s - tos %s\n", __func__, pc->orig_line, get_token_name(pc->type),
              get_token_name(frm.pse[frm.pse_tos].type));

      /* Handle proprocessor transitions */
      was_preproc = in_preproc;
      in_preproc  = (pc->flags & PCF_IN_PREPROC) != 0;

      /* Clean up after a #define */
      if (!in_preproc)
      {
         while ((frm.pse_tos > 0) && frm.pse[frm.pse_tos].in_preproc)
         {
            frm.pse_tos--;
         }
      }
      else
      {
         pf_check(&frm, pc);

         if (!was_preproc)
         {
            /* Transition into a preproc by creating a dummy indent */
            frm.level++;
            frm.pse_tos++;
            frm.pse[frm.pse_tos].type       = CT_PP;
            frm.pse[frm.pse_tos].indent     = 1 + tabsize;
            frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
            frm.pse[frm.pse_tos].open_line  = pc->orig_line;
            frm.pse[frm.pse_tos].ref        = ++ref;
            frm.pse[frm.pse_tos].in_preproc = true;
         }
      }

      /**
       * Handle non-brace closures
       */

      /* process virtual braces closes first (no text output) */
      while ((pc->type == CT_VBRACE_CLOSE) &&
             (frm.pse[frm.pse_tos].type == CT_VBRACE_OPEN))
      {
         frm.level--;
         frm.pse_tos--;
         pc = chunk_get_next(pc);
      }

      /* End any assign operations */
      while ((frm.pse[frm.pse_tos].type == CT_ASSIGN) &&
             ((pc->type == CT_BRACE_CLOSE) ||
              (pc->type == CT_PAREN_CLOSE) ||
              (pc->type == CT_SPAREN_CLOSE) ||
              (pc->type == CT_FPAREN_CLOSE) ||
              (pc->type == CT_SQUARE_CLOSE) ||
              (pc->type == CT_BRACE_OPEN) ||
              (pc->type == CT_COMMA) ||
              chunk_is_semicolon(pc)))
      {
         frm.pse_tos--;
      }

      /* End any CPP class colon crap */
      while ((frm.pse[frm.pse_tos].type == CT_CLASS_COLON) &&
             ((pc->type == CT_BRACE_OPEN) ||
              chunk_is_semicolon(pc)))
      {
         frm.pse_tos--;
      }

      /* a case is ended with another case or a close brace */
      if ((frm.pse[frm.pse_tos].type == CT_CASE) &&
          ((pc->type == CT_BRACE_CLOSE) ||
           (pc->type == CT_CASE)))
      {
         frm.pse_tos--;
      }

      /* a return is ended with a semicolon */
      if ((frm.pse[frm.pse_tos].type == CT_RETURN) &&
          chunk_is_semicolon(pc))
      {
         frm.pse_tos--;
      }

      /* Close out parens and squares */
      if ((frm.pse[frm.pse_tos].type == (pc->type - 1)) &&
          ((pc->type == CT_PAREN_CLOSE) ||
           (pc->type == CT_SPAREN_CLOSE) ||
           (pc->type == CT_FPAREN_CLOSE) ||
           (pc->type == CT_SQUARE_CLOSE)))
      {
         LOG_FMT(LINDENT, "%3d] CLOSE(%d) on %s, ",
                 pc->orig_line, frm.pse[frm.pse_tos].ref, get_token_name(pc->type));

         frm.pse_tos--;
         frm.paren_count--;

         LOG_FMT(LINDENT, "now at tos=%d col=%d top=%s\n",
                 frm.pse_tos, frm.pse[frm.pse_tos].indent,
                 get_token_name(frm.pse[frm.pse_tos].type));
      }

      /* Grab a copy of the current indent */
      indent_column = frm.pse[frm.pse_tos].indent_tmp;

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

      if (pc->type == CT_BRACE_CLOSE)
      {
         if (frm.pse[frm.pse_tos].type == CT_BRACE_OPEN)
         {
            frm.level--;
            frm.pse_tos--;

            /* Update the indent_column if needed */
            if (!cpd.settings[UO_indent_braces].b)
            {
               indent_column = frm.pse[frm.pse_tos].indent_tmp;
            }

            if ((pc->parent_type == CT_IF) ||
                (pc->parent_type == CT_ELSE) ||
                (pc->parent_type == CT_ELSEIF) ||
                (pc->parent_type == CT_DO) ||
                (pc->parent_type == CT_WHILE) ||
                (pc->parent_type == CT_SWITCH) ||
                (pc->parent_type == CT_FOR))
            {
               indent_column += cpd.settings[UO_indent_brace].n;
            }

            LOG_FMT(LINDENT, "%3d] %s col=%d, tos=%d\n", pc->orig_line,
                    get_token_name(pc->type), indent_column, frm.pse_tos);
         }
      }
      else if (pc->type == CT_VBRACE_OPEN)
      {
         frm.level++;
         frm.pse_tos++;
         frm.pse[frm.pse_tos].type       = pc->type;
         frm.pse[frm.pse_tos].indent     = frm.pse[frm.pse_tos - 1].indent + tabsize;
         frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
         frm.pse[frm.pse_tos].open_line  = pc->orig_line;
         frm.pse[frm.pse_tos].ref        = ++ref;
         frm.pse[frm.pse_tos].in_preproc = (pc->flags & PCF_IN_PREPROC) != 0;
         frm.pse[frm.pse_tos].min_col    = 0;

         /* Always indent on virtual braces */
         indent_column = frm.pse[frm.pse_tos].indent_tmp;

         LOG_FMT(LINDENT, "%3d] %s col=%d, tos=%d\n", pc->orig_line,
                 get_token_name(pc->type), indent_column, frm.pse_tos);
      }
      else if (pc->type == CT_BRACE_OPEN)
      {
         frm.level++;
         frm.pse_tos++;
         frm.pse[frm.pse_tos].type       = pc->type;
         frm.pse[frm.pse_tos].ref        = ++ref;
         frm.pse[frm.pse_tos].in_preproc = (pc->flags & PCF_IN_PREPROC) != 0;
         frm.pse[frm.pse_tos].min_col    = 0;

         if (frm.paren_count != 0)
         {
            /* We are inside ({ ... }) -- indent one tab from the paren */
            frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent_tmp + tabsize;
         }
         else
         {
            /* Use the prev indent level + tabsize. */
            frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent + tabsize;

            /* If this brace is part of a statement, bump it out by indent_brace */
            if ((pc->parent_type == CT_IF) ||
                (pc->parent_type == CT_ELSE) ||
                (pc->parent_type == CT_ELSEIF) ||
                (pc->parent_type == CT_DO) ||
                (pc->parent_type == CT_WHILE) ||
                (pc->parent_type == CT_SWITCH) ||
                (pc->parent_type == CT_FOR))
            {
               frm.pse[frm.pse_tos].indent += cpd.settings[UO_indent_brace].n;
               indent_column += cpd.settings[UO_indent_brace].n;
            }
            else if (pc->parent_type == CT_CASE)
            {
               /* The indent_case_brace setting affects the parent CT_CASE */
               //frm.pse[frm.pse_tos - 1].indent += cpd.settings[UO_indent_case_brace];
               frm.pse[frm.pse_tos].indent_tmp += cpd.settings[UO_indent_case_brace].n;
               frm.pse[frm.pse_tos].indent     += cpd.settings[UO_indent_case_brace].n;
            }
            else if ((pc->parent_type == CT_CLASS) && !cpd.settings[UO_indent_class].b)
            {
               frm.pse[frm.pse_tos].indent -= tabsize;
               //LOG_FMT(LSYS, "!indent_class: %.*s on line %d, indent is %d\n",
               //        pc->len, pc->str, pc->orig_line, frm.pse[frm.pse_tos].indent);
            }
            else if ((pc->parent_type == CT_NAMESPACE) && !cpd.settings[UO_indent_namespace].b)
            {
               frm.pse[frm.pse_tos].indent -= tabsize;
               //LOG_FMT(LSYS, "!indent_namespace: %.*s on line %d, indent is %d\n",
               //        pc->len, pc->str, pc->orig_line, frm.pse[frm.pse_tos].indent);
            }
         }

         if ((pc->flags & PCF_DONT_INDENT) != 0)
         {
            frm.pse[frm.pse_tos].indent = pc->column;
            indent_column = pc->column;
            //LOG_FMT(LSYS, "indent aligned: %.*s on line %d, col is %d\n",
            //        pc->len, pc->str, pc->orig_line, pc->column);
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
            if (cpd.settings[UO_indent_braces].n)
            {
               indent_column = frm.pse[frm.pse_tos].indent_tmp;
            }

            LOG_FMT(LINDENT, "%3d] %s col=%d, tos=%d\n", pc->orig_line,
                    get_token_name(pc->type), indent_column, frm.pse_tos);
         }
      }
      else if (pc->type == CT_CASE)
      {
         /* Start a case - indent UO_indent_switch_case from the switch level */
         tmp = frm.pse[frm.pse_tos].indent + cpd.settings[UO_indent_switch_case].n;

         frm.pse_tos++;
         frm.pse[frm.pse_tos].type       = pc->type;
         frm.pse[frm.pse_tos].indent     = tmp;
         frm.pse[frm.pse_tos].indent_tmp = tmp - tabsize;
         frm.pse[frm.pse_tos].open_line  = pc->orig_line;
         frm.pse[frm.pse_tos].ref        = ++ref;
         frm.pse[frm.pse_tos].in_preproc = (pc->flags & PCF_IN_PREPROC) != 0;

         /* Always set on case statements */
         indent_column = frm.pse[frm.pse_tos].indent_tmp;

         LOG_FMT(LINDENT, "%3d] %s col=%d, tos=%d\n", pc->orig_line,
                 get_token_name(pc->type), indent_column, frm.pse_tos);
      }
      else if (pc->type == CT_LABEL)
      {
         /* Labels get sent to the left or backed up */
         if (cpd.settings[UO_indent_label].n > 0)
         {
            indent_column = cpd.settings[UO_indent_label].n;
         }
         else
         {
            indent_column = frm.pse[frm.pse_tos].indent +
                            cpd.settings[UO_indent_label].n;
         }
      }
      else if (pc->type == CT_CLASS_COLON)
      {
         /* just indent one level */
         frm.pse_tos++;
         frm.pse[frm.pse_tos].type       = pc->type;
         frm.pse[frm.pse_tos].indent     = frm.pse[frm.pse_tos - 1].indent_tmp + tabsize;
         frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
         frm.pse[frm.pse_tos].open_line  = pc->orig_line;
         frm.pse[frm.pse_tos].ref        = ++ref;
         frm.pse[frm.pse_tos].in_preproc = (pc->flags & PCF_IN_PREPROC) != 0;

         indent_column = frm.pse[frm.pse_tos].indent_tmp;

         LOG_FMT(LINDENT, "%3d] %s col=%d, tos=%d\n", pc->orig_line,
                 get_token_name(pc->type), indent_column, frm.pse_tos);
      }
      else if ((pc->type == CT_PAREN_OPEN) ||
               (pc->type == CT_SPAREN_OPEN) ||
               (pc->type == CT_FPAREN_OPEN) ||
               (pc->type == CT_SQUARE_OPEN))
      {
         /* Open parens and squares - never update indent_column */
         frm.pse_tos++;
         frm.pse[frm.pse_tos].type       = pc->type;
         frm.pse[frm.pse_tos].indent     = pc->column + pc->len;
         frm.pse[frm.pse_tos].open_line  = pc->orig_line;
         frm.pse[frm.pse_tos].ref        = ++ref;
         frm.pse[frm.pse_tos].in_preproc = (pc->flags & PCF_IN_PREPROC) != 0;

         if (cpd.settings[UO_indent_func_call_param].b &&
             (pc->type == CT_FPAREN_OPEN))
         {
            prev = chunk_get_prev_ncnl(pc);
            if (prev->type == CT_FUNC_CALL)
            {
               frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent + tabsize;
            }
         }

         if (!cpd.settings[UO_indent_paren_nl].b)
         {
            next = chunk_get_next_nc(pc);
            if (chunk_is_newline(next))
            {
               frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent + tabsize;
            }
         }
         frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
         frm.paren_count++;

         LOG_FMT(LINDENT, "%3d] %s col=%d, tos=%d\n", pc->orig_line,
                 get_token_name(pc->type), indent_column, frm.pse_tos);
      }
      else if (pc->type == CT_ASSIGN)
      {
         /**
          * if there is a newline after the '=', just indent one level,
          * otherwise align on the '='.
          * Never update indent_column.
          */
         next = chunk_get_next(pc);
         if (next != NULL)
         {
            frm.pse_tos++;
            frm.pse[frm.pse_tos].type = pc->type;
            if (chunk_is_newline(next))
            {
               frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent_tmp + tabsize;
            }
            else
            {
               frm.pse[frm.pse_tos].indent = pc->column + pc->len + 1;
            }
            frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
            frm.pse[frm.pse_tos].open_line  = pc->orig_line;
            frm.pse[frm.pse_tos].ref        = ++ref;
            frm.pse[frm.pse_tos].in_preproc = (pc->flags & PCF_IN_PREPROC) != 0;

            LOG_FMT(LINDENT, "%3d] OPEN(10) on %s, tos=%d\n", pc->orig_line,
                    get_token_name(pc->type), frm.pse_tos);
         }
      }
      else if (pc->type == CT_RETURN)
      {
         /* don't count returns inside a () or [] */
         if (pc->level == pc->brace_level)
         {
            frm.pse_tos++;
            frm.pse[frm.pse_tos].type       = pc->type;
            frm.pse[frm.pse_tos].indent     = frm.pse[frm.pse_tos - 1].indent + pc->len + 1;
            frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos - 1].indent;
            frm.pse[frm.pse_tos].open_line  = pc->orig_line;
            frm.pse[frm.pse_tos].ref        = ++ref;
            frm.pse[frm.pse_tos].in_preproc = (pc->flags & PCF_IN_PREPROC) != 0;

            LOG_FMT(LINDENT, "%3d] %s, tos=%d\n", pc->orig_line,
                    get_token_name(pc->type), frm.pse_tos);
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
         /**
          * Check for special continuations.
          * Note that some of these could be done as a stack item like
          * everything else
          */

         prev = chunk_get_prev_ncnl(pc);
         if ((vardefcol > 0) &&
             (pc->type == CT_WORD) &&
             ((pc->flags & PCF_VAR_DEF) != 0) &&
             (prev != NULL) && (prev->type == CT_COMMA))
         {
            LOG_FMT(LINDENT, "%s: %d] Vardefcol = %d\n", __func__, pc->orig_line, vardefcol);
            reindent_line(pc, vardefcol);
         }
         else if ((pc->type == CT_STRING) && (prev->type == CT_STRING) &&
                  cpd.settings[UO_indent_align_string].b)
         {
            LOG_FMT(LINDENT, "%s: %d] String Col = %d\n",
                    __func__, pc->orig_line, prev->column);
            reindent_line(pc, prev->column);
         }
         else if (chunk_is_comment(pc))
         {
            indent_comment(pc, frm.pse[frm.pse_tos].indent_tmp);
         }
         else if (pc->type == CT_PREPROC)
         {
            LOG_FMT(LINDENT, "%s: %d] preproc indent\n", __func__, pc->orig_line);

            /*TODO: indent based on #if depth? */
            reindent_line(pc, 1);
         }
         else if (pc->column != indent_column)
         {
            LOG_FMT(LINDENT, "%s: indent line %d to column %d\n",
                    __func__, pc->orig_line, indent_column);
            reindent_line(pc, indent_column);
         }
         did_newline = false;

         /* Set up for detection of C++ cout << "crap"; */
         if (((frm.pse[frm.pse_tos].type == CT_BRACE_OPEN) ||
              (frm.pse[frm.pse_tos].type == CT_VBRACE_OPEN)) &&
             (frm.pse[frm.pse_tos].min_col <= 0))
         {
            /* Set it up for detection */
            frm.pse[frm.pse_tos].min_col = -1;
         }
      }

      /**
       * Handle C++ cout-style crap '<<' line continuation
       * Example:
       * std::cout << "first line"
       *           << "second line";
       */
      if ((frm.pse[frm.pse_tos].min_col == -1) && (pc->type != CT_WORD))
      {
         //LOG_FMT(LSYS, "%s: BAILED: pc=%s col=%d\n", __func__, pc->str, pc->column);
         frm.pse[frm.pse_tos].min_col = 0;
      }
      if (frm.pse[frm.pse_tos].min_col <= -1)
      {
         if ((pc->type != CT_WORD) &&
             (pc->type != CT_MEMBER) &&
             (pc->type != CT_DC_MEMBER) &&
             (pc->type != CT_ARITH))
         {
            /* Done with the tmp_indent */
            frm.pse[frm.pse_tos].min_col = -frm.pse[frm.pse_tos].min_col;
         }
         else
         {
            /* include this in the class name */
            frm.pse[frm.pse_tos].min_col = -pc->column;
         }
      }
      if (chunk_is_semicolon(pc))
      {
         frm.pse[frm.pse_tos].min_col = 0;
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
         if (frm.pse[frm.pse_tos].min_col > 0)
         {
            frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].min_col;
         }

         /**
          * Handle the case of a multi-line #define w/o anything on the
          * first line (indent_tmp will be 1 or 0)
          */
         if ((pc->type == CT_NL_CONT) &&
             (frm.pse[frm.pse_tos].indent_tmp <= tabsize))
         {
            frm.pse[frm.pse_tos].indent_tmp = tabsize + 1;
         }

         /* Get ready to indent the next item */
         did_newline = true;
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
      frm.pse_tos--;
   }

   for (idx = 1; idx <= frm.pse_tos; idx++)
   {
      LOG_FMT(LWARN, "Unmatched %s near line %d\n",
              get_token_name(frm.pse[idx].type), frm.pse[idx].open_line);
   }
}



/**
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
   if ((pc->orig_col == 1) && !cpd.settings[UO_indent_col1_comment].b)
   {
      LOG_FMT(LCMTIND, "rule 1 - keep in col 1\n");
      pc->column = 1;
      return;
   }

   nl = chunk_get_prev(pc);

   /* outside of any expression or statement? */
   if (pc->level == 0)
   {
      if ((nl != NULL) && (nl->nl_count > 1))
      {
         LOG_FMT(LCMTIND, "rule 2 - level 0, nl before\n");
         pc->column = 1;
         return;
      }
   }

   prev = chunk_get_prev(nl);
   if (chunk_is_comment(prev) && (nl->nl_count == 1))
   {
      int coldiff = prev->orig_col - pc->orig_col;

      if ((coldiff <= 3) && (coldiff >= -3))
      {
         pc->column = prev->column;
         LOG_FMT(LCMTIND, "rule 3 - prev comment, coldiff = %d, now in %d\n",
                 coldiff, pc->column);
         return;
      }
   }

   LOG_FMT(LCMTIND, "rule 4 - fall-through, stay in %d\n", col);

   pc->column = col;
}


/**
 * Put spaces on either side of the preproc (#) symbol.
 * This is done by pointing pc->str into pp_str and adjusting the
 * length.
 */
void indent_preproc(void)
{
   chunk_t *pc;
   chunk_t *next;
   int     pp_level;
   int     pp_level_sub = 0;
   int     tmp;

   /* Define a string of 16 spaces + # + 16 spaces */
   static const char *pp_str  = "                #                ";
   static const char *alt_str = "                %:                ";

   /* Scan to see if the whole file is covered by one #ifdef */
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
      pp_level_sub = 1;
   }

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next(pc))
   {
      if (pc->type != CT_PREPROC)
      {
         continue;
      }

      if (pc->column != 1)
      {
         /* Don't handle preprocessors that aren't in column 1 */
         LOG_FMT(LINFO, "%s: Line %d doesn't start in column 1 (%d)\n",
                 __func__, pc->orig_line, pc->column);
         continue;
      }

      /* point into pp_str */
      if (pc->len == 2)
      {
         /* alternate token crap */
         pc->str = &alt_str[16];
      }
      else
      {
         pc->str = &pp_str[16];
      }

      pp_level = pc->pp_level - pp_level_sub;
      if (pp_level < 0)
      {
         pp_level = 0;
      }
      else if (pp_level > 16)
      {
         pp_level = 16;
      }

      /* Note that the indent is removed by default */
      if ((cpd.settings[UO_pp_indent].a & AV_ADD) != 0)
      {
         /* Need to add some spaces */
         pc->str -= pp_level;
         pc->len += pp_level;
      }
      else if (cpd.settings[UO_pp_indent].a == AV_IGNORE)
      {
         tmp      = (pc->orig_col <= 16) ? pc->orig_col - 1 : 16;
         pc->str -= tmp;
         pc->len += tmp;
      }

      /* Add spacing by adjusting the length */
      if ((cpd.settings[UO_pp_space].a & AV_ADD) != 0)
      {
         pc->len += pp_level;
      }

      next = chunk_get_next(pc);
      if (next != NULL)
      {
         reindent_line(next, pc->len + 1);
      }

      LOG_FMT(LPPIS, "%s: Indent line %d to %d (len %d, next->col %d)\n",
              __func__, pc->orig_line, pp_level, pc->len, next->column);
   }
}
