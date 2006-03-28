/**
 * @file c_indent.c
 * Does all the indenting stuff.
 *
 * $Id$
 */
#include "cparse_types.h"
#include "chunk_list.h"
#include "prototypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

//#define DBG_BRACE

static void indent_comment(chunk_t *pc, int col);


void indent_column(chunk_t *pc, int column)
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

   LOG_FMT(LINDLINE, "%s: %d] col %d on %s [%s]\n",
           __func__, pc->orig_line, pc->column, pc->str,
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
   BOOL               did_newline = TRUE;
   int                idx;
   int                vardefcol    = 0;
   int                last_str_col = -1;
   int                last_cpp_col = -1;
   int                tabsize      = cpd.settings[UO_output_tab_size];
   int                ref          = 0;
   int                tmp;
   struct parse_frame frm;
   BOOL               in_preproc = FALSE, was_preproc = FALSE;

   memset(&frm, 0, sizeof(frm));

   /* dummy top-level entry */
   frm.pse[0].indent     = 1;
   frm.pse[0].indent_tmp = 1;
   frm.pse[0].type       = CT_EOF;

   pc = chunk_get_head();
   while (pc != NULL)
   {
      was_preproc = in_preproc;
      in_preproc  = (pc->flags & PCF_IN_PREPROC) != 0;

      LOG_FMT(LINDENT, "%s: %d] %s - tos %s\n", __func__, pc->orig_line, get_token_name(pc->type),
              get_token_name(frm.pse[frm.pse_tos].type));

      /* Clean up after a #define */
      if (!in_preproc)
      {
         while ((frm.pse_tos > 0) && frm.pse[frm.pse_tos].in_preproc)
         {
            frm.pse_tos--;
         }
      }

      /* Add a dummy indent level */
      if (in_preproc && !was_preproc)
      {
         frm.level++;
         frm.pse_tos++;
         frm.pse[frm.pse_tos].type       = CT_PP;
         frm.pse[frm.pse_tos].indent     = 1 + tabsize;
         frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
         frm.pse[frm.pse_tos].open_line  = pc->orig_line;
         frm.pse[frm.pse_tos].ref        = ++ref;
         frm.pse[frm.pse_tos].in_preproc = TRUE;
      }

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
              (pc->type == CT_SEMICOLON)))
      {
         frm.pse_tos--;
         //         fprintf(stderr, "%3d] CLOSE(2) on %s, tos=%d\n", pc->orig_line,
         //                 c_chunk_names[pc->type], pse_tos);
      }

      /* End any CPP class colon crap */
      while ((frm.pse[frm.pse_tos].type == CT_CLASS_COLON) &&
             ((pc->type == CT_BRACE_OPEN) ||
              (pc->type == CT_SEMICOLON)))
      {
         frm.pse_tos--;
      }

      pf_check(&frm, pc);

      /* a case is ended with another case or a close brace */
      if ((frm.pse[frm.pse_tos].type == CT_CASE) &&
          ((pc->type == CT_BRACE_CLOSE) ||
           (pc->type == CT_CASE)))
      {
         frm.pse_tos--;
         //fprintf(stderr, "%3d] CLOSE(3) on %s, tos=%d\n", pc->orig_line,
         //        c_chunk_names[pc->type], pse_tos);
      }

      /* a return is ended with a semicolon */
      if ((frm.pse[frm.pse_tos].type == CT_RETURN) &&
          (pc->type == CT_SEMICOLON))
      {
         frm.pse_tos--;
         //fprintf(stderr, "%3d] CLOSE(3) on %s, tos=%d\n", pc->orig_line,
         //        get_token_name(pc->type), frm.pse_tos);
      }

      /* Start a case - indent UO_indent_switch_case from the switch level */
      if (pc->type == CT_CASE)
      {
         tmp = frm.pse[frm.pse_tos].indent + cpd.settings[UO_indent_switch_case];

         frm.pse_tos++;
         frm.pse[frm.pse_tos].type       = pc->type;
         frm.pse[frm.pse_tos].indent     = tmp + cpd.settings[UO_indent_case_body];
         frm.pse[frm.pse_tos].indent_tmp = tmp - tabsize;
         frm.pse[frm.pse_tos].open_line  = pc->orig_line;
         frm.pse[frm.pse_tos].ref        = ++ref;
         frm.pse[frm.pse_tos].in_preproc = (pc->flags & PCF_IN_PREPROC) != 0;
         //         fprintf(stderr, "%3d] OPEN(4) on %s, tos=%d\n", pc->orig_line,
         //                 c_chunk_names[pc->type], pse_tos);
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
            //         fprintf(stderr, "%3d] OPEN(4) on %s, tos=%d\n", pc->orig_line,
            //                 c_chunk_names[pc->type], pse_tos);
         }
      }

      else if (pc->type == CT_BRACE_CLOSE)
      {
         if (frm.pse[frm.pse_tos].type == CT_BRACE_OPEN)
         {
            frm.level--;
            frm.pse_tos--;
            LOG_FMT(LINDENT, "%3d] CLOSE(5) on %s, tos=%d\n", pc->orig_line,
                    get_token_name(pc->type), frm.pse_tos);
         }
      }
      else if ((frm.pse[frm.pse_tos].type == (pc->type - 1)) &&
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
      else if (pc->type == CT_BRACE_OPEN)
      {
         if ((pc->parent_type == CT_IF) ||
             (pc->parent_type == CT_ELSE) ||
             (pc->parent_type == CT_DO) ||
             (pc->parent_type == CT_WHILE) ||
             (pc->parent_type == CT_SWITCH) ||
             (pc->parent_type == CT_FOR))
         {
            frm.pse[frm.pse_tos].indent_tmp += cpd.settings[UO_indent_brace];
         }
         else if (pc->parent_type == CT_CASE)
         {
            /* The indent_case_brace setting affects the parent CT_CASE */
            frm.pse[frm.pse_tos].indent_tmp += cpd.settings[UO_indent_case_brace];
         }
      }
      else
      {
         /* nada */
      }

      /* Labels get sent to the left or backed up */
      if (pc->type == CT_LABEL)
      {
         if (cpd.settings[UO_indent_label] >= 0)
         {
            frm.pse[frm.pse_tos].indent_tmp = 1 + cpd.settings[UO_indent_label];
         }
         else
         {
            frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent +
                                              cpd.settings[UO_indent_label];
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

         LOG_FMT(LINDENT, "%3d] OPEN(10) on %s, tos=%d\n", pc->orig_line,
                 get_token_name(pc->type), frm.pse_tos);
      }

      /* Are we right after a newline? */
      if (did_newline && !chunk_is_newline(pc) && (pc->len != 0))
      {
         if ((vardefcol > 0) &&
             (pc->type == CT_WORD) &&
             ((pc->flags & PCF_VAR_DEF) != 0) &&
             (prev != NULL) && (prev->type == CT_COMMA))
         {
            LOG_FMT(LINDENT, "%s: %d] Vardefcol = %d\n", __func__, pc->orig_line, vardefcol);
            reindent_line(pc, vardefcol);
         }
         else if ((pc->type == CT_STRING) && (last_str_col > 0))
         {
            LOG_FMT(LINDENT, "stringcol = %d\n", last_str_col);
            reindent_line(pc, last_str_col);
         }
         else if (chunk_is_comment(pc))
         {
            indent_comment(pc, frm.pse[frm.pse_tos].indent_tmp);
         }
         else if (pc->type == CT_PREPROC)
         {
            LOG_FMT(LINDENT, "%s: %d] preproc indent\n", __func__, pc->orig_line);
            reindent_line(pc, 1);
         }
         else if ((pc->column != (frm.pse[frm.pse_tos].indent_tmp)) &&
                  ((pc->column != 1) ||
                   !((pc->type == CT_COMMENT) ||
                     (pc->type == CT_COMMENT_CPP) ||
                     (pc->type == CT_COMMENT_MULTI))))
         {
            LOG_FMT(LINDENT, "%s: line %d, column %d\n",
                    __func__, pc->orig_line, frm.pse[frm.pse_tos].indent_tmp);
            reindent_line(pc, frm.pse[frm.pse_tos].indent_tmp);
         }
         did_newline = FALSE;

         if (((frm.pse[frm.pse_tos].type == CT_BRACE_OPEN) ||
              (frm.pse[frm.pse_tos].type == CT_VBRACE_OPEN)) &&
             (frm.pse[frm.pse_tos].min_col <= 0))
         {
            /* Set it up for detection */
            frm.pse[frm.pse_tos].min_col = -1;
         }
      }

      /* Handle C++ cout-style crap '<<' line continuation */
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

      if (pc->type == CT_SEMICOLON)
      {
         frm.pse[frm.pse_tos].min_col = 0;
      }

      if ((pc->type == CT_STRING) && cpd.settings[UO_indent_align_string])
      {
         last_str_col = pc->column;
         //fprintf(stderr, "%s: %d] indent_align_string, stringcol = %d\n",
         //        __func__, pc->orig_line, last_str_col);
      }
      else if (!chunk_is_newline(pc))
      {
         last_str_col = -1;
      }

      if ((pc->column != 1) &&
          ((pc->type == CT_COMMENT_CPP) || (pc->type == CT_COMMENT)))
      {
         last_cpp_col = pc->column;
      }
      else if (!chunk_is_newline(pc))
      {
         last_cpp_col = -1;
      }

      if ((pc->type == CT_WORD) &&
          ((pc->flags & PCF_IN_FCN_DEF) == 0) &&
          ((pc->flags & PCF_VAR_1ST_DEF) == PCF_VAR_1ST_DEF))
      {
         vardefcol = pc->column;
      }
      if ((pc->type == CT_SEMICOLON) ||
          ((pc->type == CT_BRACE_OPEN) && (pc->parent_type == CT_FUNCTION)))
      {
         vardefcol = 0;
      }

      /* if we hit a newline, reset indent_tmp */
      if (chunk_is_newline(pc) ||
          (pc->type == CT_COMMENT_MULTI) ||
          (pc->type == CT_COMMENT_CPP))
      {
         did_newline                     = TRUE;
         frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
         if (frm.pse[frm.pse_tos].min_col > 0)
         {
            frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].min_col;
         }

         /**
          * Handle the case of a multi-line #define w/o a reference on the
          * first line (indent_tmp will be 1 or 0)
          */
         if ((pc->type == CT_NL_CONT) &&
             (frm.pse[frm.pse_tos].indent_tmp <= tabsize))
         {
            frm.pse[frm.pse_tos].indent_tmp = tabsize + 1;
         }
      }

      /* Process all the brace/paren/square opens after the indent */
      if (pc->type == CT_VBRACE_OPEN)
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
         LOG_FMT(LINDENT, "%3d] OPEN(7) on %s, tos=%d\n", pc->orig_line,
                 get_token_name(pc->type), frm.pse_tos);
      }
      else if (pc->type == CT_BRACE_OPEN)
      {
         frm.level++;
         frm.pse_tos++;
         frm.pse[frm.pse_tos].type       = pc->type;
         frm.pse[frm.pse_tos].ref        = ++ref;
         frm.pse[frm.pse_tos].in_preproc = (pc->flags & PCF_IN_PREPROC) != 0;
         frm.pse[frm.pse_tos].min_col    = 0;
         LOG_FMT(LINDENT, "%3d] OPEN(8) on %s, tos=%d\n", pc->orig_line,
                 get_token_name(pc->type), frm.pse_tos);

         if (frm.paren_count == 0)
         {
            frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent;
            prev                        = chunk_get_prev_ncnl(pc);
            if ((prev == NULL) || (prev->type != CT_CASE_COLON))
            {
               frm.pse[frm.pse_tos].indent += tabsize;
            }

            if ((pc->parent_type == CT_IF) ||
                (pc->parent_type == CT_ELSE) ||
                (pc->parent_type == CT_DO) ||
                (pc->parent_type == CT_WHILE) ||
                (pc->parent_type == CT_SWITCH) ||
                (pc->parent_type == CT_FOR))
            {
               frm.pse[frm.pse_tos].indent += cpd.settings[UO_indent_brace];
            }
            else if (pc->parent_type == CT_CASE)
            {
               /* The indent_case_brace setting affects the parent CT_CASE */
               frm.pse[frm.pse_tos - 1].indent += cpd.settings[UO_indent_case_brace];
               frm.pse[frm.pse_tos].indent_tmp += tabsize + cpd.settings[UO_indent_case_brace];
               frm.pse[frm.pse_tos].indent     += tabsize + cpd.settings[UO_indent_case_brace];
            }
         }
         else
         {
            frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent_tmp + tabsize;
         }

         next = chunk_get_next_ncnl(pc);
         if (!chunk_is_newline_between(pc, next))
         {
            frm.pse[frm.pse_tos].indent = next->column;
            //            fprintf(stderr, "Same-line scariness on line %d\n", pc->orig_line);
         }
         frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
         frm.pse[frm.pse_tos].open_line  = pc->orig_line;
      }
      else if ((pc->type == CT_PAREN_OPEN) ||
               (pc->type == CT_SPAREN_OPEN) ||
               (pc->type == CT_FPAREN_OPEN) ||
               (pc->type == CT_SQUARE_OPEN))
      {
         frm.pse_tos++;
         frm.pse[frm.pse_tos].type       = pc->type;
         frm.pse[frm.pse_tos].indent     = pc->column + pc->len;
         frm.pse[frm.pse_tos].open_line  = pc->orig_line;
         frm.pse[frm.pse_tos].ref        = ++ref;
         frm.pse[frm.pse_tos].in_preproc = (pc->flags & PCF_IN_PREPROC) != 0;

         if (cpd.settings[UO_indent_func_call_param] &&
             (pc->type == CT_FPAREN_OPEN))
         {
            prev = chunk_get_prev_ncnl(pc);
            if (prev->type == CT_FUNC_CALL)
            {
               frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent + tabsize;
            }
         }

         if (!cpd.settings[UO_indent_paren_nl])
         {
            next = chunk_get_next_nc(pc);
            if (chunk_is_newline(next))
            {
               frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent + tabsize;
            }
         }
         frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
         frm.paren_count++;

         LOG_FMT(LINDENT, "%3d] OPEN(%d) on %s, tos=%d col=%d\n",
                 pc->orig_line, frm.pse[frm.pse_tos].ref,
                 get_token_name(pc->type), frm.pse_tos, frm.pse[frm.pse_tos].indent);
      }
      else if (pc->type == CT_ASSIGN)
      {
         /* if there is a newline after the '=', just indent one level,
          * otherwise align on the '=' */
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

      if (!chunk_is_comment(pc) && !chunk_is_newline(pc))
      {
         prev = pc;
      }
      pc = chunk_get_next(pc);
   }

   if (in_preproc)
   {
      while ((frm.pse_tos > 0) && frm.pse[frm.pse_tos].in_preproc)
      {
         frm.pse_tos--;
      }
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
   if ((pc->orig_col == 1) && !cpd.settings[UO_indent_col1_comment])
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

