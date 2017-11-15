/**
 * @file indent.cpp
 * Does all the indenting stuff.
 *
 * $Id: indent.cpp 548 2006-10-21 02:31:55Z bengardner $
 */
#include "uncrustify_types.h"
#include "chunk_list.h"
#include "prototypes.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <cctype>


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
    column = pc->column;

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

  LOG_FMT(LINDLINE, "%s: %d] col %d on %.*s [%s] => %d\n",
          __func__, pc->orig_line, pc->column, pc->len, pc->str,
          get_token_name(pc->type), column);

  if (column == pc->column)
    return;

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
          pc->column = min_col + 1;

        LOG_FMT(LINDLINE, "%s: set comment on line %d to col %d (orig %d)\n",
                __func__, pc->orig_line, pc->column, pc->orig_col);
        }
      else
        {
        pc->column += col_delta;

        if (pc->column < min_col)
          pc->column = min_col;
        }
      }
    }
  while ((pc != NULL) && (pc->nl_count == 0));
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
  if (frm.pse_tos < (int)ARRAY_SIZE(frm.pse))
    {
      /* Bump up the index and initialize it */
    frm.pse_tos++;
    memset(&frm.pse[frm.pse_tos], 0, sizeof(frm.pse[frm.pse_tos]));

    LOG_FMT(LINDPSE, "%4d] OPEN  [%d,%s] level=%d\n",
            pc->orig_line, frm.pse_tos, get_token_name(pc->type), pc->level);

    frm.pse[frm.pse_tos].type       = pc->type;
    frm.pse[frm.pse_tos].level      = pc->level;
    frm.pse[frm.pse_tos].open_line  = pc->orig_line;
    frm.pse[frm.pse_tos].ref        = ++ref;
    frm.pse[frm.pse_tos].in_preproc = (pc->flags & PCF_IN_PREPROC) != 0;
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
      LOG_FMT(LINDPSE, "%4d] CLOSE [%d,%s] on %s, started on line %d, level=%d/%d\n",
              pc->orig_line, frm.pse_tos,
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

    frm.pse_tos--;
    }
  }

static int token_indent(c_token_t type)
  {
  switch (type)
    {
    case CT_IF:
    case CT_DO:
      return 3;

    case CT_FOR:
    case CT_ELSE: // wacky, but that's what is wanted
      return 4;

    case CT_WHILE:
      return 6;

    case CT_SWITCH:
      return 7;

    case CT_ELSEIF:
      return 8;

    default:
      return 0;  //cpd.settings[UO_indent_braces].n;
    }
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
  bool did_newline = true;
  int idx;
  int vardefcol   = 0;
  int indent_size = cpd.settings[UO_indent_columns].n;
  int tmp;
  struct parse_frame frm;
  bool in_preproc = false, was_preproc = false;
  int indent_column;
  int cout_col            = 0;                 // for aligning << stuff
  int cout_level          = 0;                 // for aligning << stuff
  int parent_token_indent = 0;

  memset(&frm, 0, sizeof(frm));

    /* dummy top-level entry */
  frm.pse[0].indent     = 1;
  frm.pse[0].indent_tmp = 1;
  frm.pse[0].type       = CT_EOF;

  pc = chunk_get_head();

  while (pc != NULL)
    {
      /* Handle proprocessor transitions */
    was_preproc = in_preproc;
    in_preproc  = (pc->flags & PCF_IN_PREPROC) != 0;

    if (cpd.settings[UO_indent_brace_parent].b)
      parent_token_indent = token_indent(pc->parent_type);

      /* Clean up after a #define */
    if (!in_preproc)
      while ((frm.pse_tos > 0) && frm.pse[frm.pse_tos].in_preproc)
        indent_pse_pop(frm, pc);

    else
      {
      pf_check(&frm, pc);

      if (!was_preproc)
        {
          /* Transition into a preproc by creating a dummy indent */
        frm.level++;
        indent_pse_push(frm, pc);

        frm.pse[frm.pse_tos].indent     = 1 + indent_size;
        frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
        }
      }

    if ((cout_col > 0) &&
        (chunk_is_semicolon(pc) ||
         (pc->level < cout_level)))
      {
      cout_col   = 0;
      cout_level = 0;
      }

    /**
     * Handle non-brace closures
     */

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
        indent_pse_pop(frm, pc);

      if (frm.pse[frm.pse_tos].level == pc->level)
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
        if ((frm.pse[frm.pse_tos].type == CT_ASSIGN) &&
            (chunk_is_semicolon(pc) ||
             (pc->type == CT_COMMA) ||
             (pc->type == CT_BRACE_OPEN)))
          indent_pse_pop(frm, pc);

          /* End any CPP class colon crap */
        if ((frm.pse[frm.pse_tos].type == CT_CLASS_COLON) &&
            ((pc->type == CT_BRACE_OPEN) ||
             chunk_is_semicolon(pc)))
          indent_pse_pop(frm, pc);

          /* a case is ended with another case or a close brace */
        if ((frm.pse[frm.pse_tos].type == CT_CASE) &&
            ((pc->type == CT_BRACE_CLOSE) ||
             (pc->type == CT_CASE)))
          indent_pse_pop(frm, pc);

          /* a return is ended with a semicolon */
        if ((frm.pse[frm.pse_tos].type == CT_RETURN) &&
            chunk_is_semicolon(pc))
          indent_pse_pop(frm, pc);

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
      }
    while (old_pse_tos > frm.pse_tos);

      /* Grab a copy of the current indent */
    indent_column = frm.pse[frm.pse_tos].indent_tmp;

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

    if (pc->type == CT_BRACE_CLOSE)
      {
      if (frm.pse[frm.pse_tos].type == CT_BRACE_OPEN)
        {
        indent_pse_pop(frm, pc);
        frm.level--;

          /* Update the indent_column if needed */
        if (!cpd.settings[UO_indent_braces].b &&
            (parent_token_indent == 0))
          indent_column = frm.pse[frm.pse_tos].indent_tmp;

        if ((pc->parent_type == CT_IF) ||
            (pc->parent_type == CT_ELSE) ||
            (pc->parent_type == CT_ELSEIF) ||
            (pc->parent_type == CT_DO) ||
            (pc->parent_type == CT_WHILE) ||
            (pc->parent_type == CT_SWITCH) ||
            (pc->parent_type == CT_FOR))
          indent_column += cpd.settings[UO_indent_brace].n;
        }
      }
    else if (pc->type == CT_VBRACE_OPEN)
      {
      frm.level++;
      indent_pse_push(frm, pc);

      frm.pse[frm.pse_tos].indent     = frm.pse[frm.pse_tos - 1].indent + indent_size;
      frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;

        /* Always indent on virtual braces */
      indent_column = frm.pse[frm.pse_tos].indent_tmp;
      }
    else if (pc->type == CT_BRACE_OPEN)
      {
      frm.level++;
      indent_pse_push(frm, pc);

      if (frm.paren_count != 0)
          /* We are inside ({ ... }) -- indent one tab from the paren */
        frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent_tmp + indent_size;
      else
        {
          /* Use the prev indent level + indent_size. */
        frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent + indent_size;

          /* If this brace is part of a statement, bump it out by indent_brace */
        if ((pc->parent_type == CT_IF) ||
            (pc->parent_type == CT_ELSE) ||
            (pc->parent_type == CT_ELSEIF) ||
            (pc->parent_type == CT_DO) ||
            (pc->parent_type == CT_WHILE) ||
            (pc->parent_type == CT_SWITCH) ||
            (pc->parent_type == CT_FOR))
          {
          if (parent_token_indent != 0)
            frm.pse[frm.pse_tos].indent += parent_token_indent - indent_size;
          else
            {
            frm.pse[frm.pse_tos].indent += cpd.settings[UO_indent_brace].n;
            indent_column += cpd.settings[UO_indent_brace].n;
            }
          }
        else if (pc->parent_type == CT_CASE)
          {
            /* The indent_case_brace setting affects the parent CT_CASE */
          frm.pse[frm.pse_tos].indent_tmp += cpd.settings[UO_indent_case_brace].n;
          frm.pse[frm.pse_tos].indent     += cpd.settings[UO_indent_case_brace].n;
          }
        else if ((pc->parent_type == CT_CLASS) && !cpd.settings[UO_indent_class].b)
          frm.pse[frm.pse_tos].indent -= indent_size;
        else if ((pc->parent_type == CT_NAMESPACE) && !cpd.settings[UO_indent_namespace].b)
          frm.pse[frm.pse_tos].indent -= indent_size;
        }

      if ((pc->flags & PCF_DONT_INDENT) != 0)
        {
        frm.pse[frm.pse_tos].indent = pc->column;
        indent_column = pc->column;
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
          frm.pse[frm.pse_tos].indent = next->column;

        frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
        frm.pse[frm.pse_tos].open_line  = pc->orig_line;

          /* Update the indent_column if needed */
        if (cpd.settings[UO_indent_braces].n ||
            (parent_token_indent != 0))
          indent_column = frm.pse[frm.pse_tos].indent_tmp;
        }
      }
    else if (pc->type == CT_CASE)
      {
        /* Start a case - indent UO_indent_switch_case from the switch level */
      tmp = frm.pse[frm.pse_tos].indent + cpd.settings[UO_indent_switch_case].n;

      indent_pse_push(frm, pc);

      frm.pse[frm.pse_tos].indent     = tmp;
      frm.pse[frm.pse_tos].indent_tmp = tmp - indent_size;

        /* Always set on case statements */
      indent_column = frm.pse[frm.pse_tos].indent_tmp;
      }
    else if (pc->type == CT_LABEL)
      {
        /* Labels get sent to the left or backed up */
      if (cpd.settings[UO_indent_label].n > 0)
        indent_column = cpd.settings[UO_indent_label].n;
      else
        indent_column = frm.pse[frm.pse_tos].indent +
                        cpd.settings[UO_indent_label].n;
      }
    else if (pc->type == CT_CLASS_COLON)
      {
        /* just indent one level */
      indent_pse_push(frm, pc);
      frm.pse[frm.pse_tos].indent     = frm.pse[frm.pse_tos - 1].indent_tmp + indent_size;
      frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;

      indent_column = frm.pse[frm.pse_tos].indent_tmp;

      if (cpd.settings[UO_indent_class_colon].b)
        {
        prev = chunk_get_prev(pc);

        if (chunk_is_newline(prev))
          frm.pse[frm.pse_tos].indent += 2;

        /* don't change indent of current line */
        }
      }
    else if ((pc->type == CT_PAREN_OPEN) ||
             (pc->type == CT_SPAREN_OPEN) ||
             (pc->type == CT_FPAREN_OPEN) ||
             (pc->type == CT_SQUARE_OPEN) ||
             (pc->type == CT_ANGLE_OPEN))
      {
        /* Open parens and squares - never update indent_column */
      indent_pse_push(frm, pc);
      frm.pse[frm.pse_tos].indent = pc->column + pc->len;

      if (cpd.settings[UO_indent_func_call_param].b &&
          (pc->type == CT_FPAREN_OPEN) &&
          (pc->parent_type == CT_FUNC_CALL))
        frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent + indent_size;

      if ((chunk_is_str(pc, "(", 1) && !cpd.settings[UO_indent_paren_nl].b) ||
          (chunk_is_str(pc, "[", 1) && !cpd.settings[UO_indent_square_nl].b))
        {
        next = chunk_get_next_nc(pc);

        if (chunk_is_newline(next))
          {
          int sub = 1;

          if (frm.pse[frm.pse_tos - 1].type == CT_ASSIGN)
            sub = 2;

          frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - sub].indent + indent_size;
          }
        }

      frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
      frm.paren_count++;
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
        indent_pse_push(frm, pc);

        if (chunk_is_newline(next))
          frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent_tmp + indent_size;
        else
          frm.pse[frm.pse_tos].indent = pc->column + pc->len + 1;

        frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
        }
      }
    else if (pc->type == CT_RETURN)
      {
        /* don't count returns inside a () or [] */
      if (pc->level == pc->brace_level)
        {
        indent_pse_push(frm, pc);
        frm.pse[frm.pse_tos].indent     = frm.pse[frm.pse_tos - 1].indent + pc->len + 1;
        frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos - 1].indent;
        }
      }
    else if (chunk_is_str(pc, "<<", 2))
      {
      if (cout_col == 0)
        {
        cout_col   = pc->column;
        cout_level = pc->level;
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

      if ((pc->type == CT_MEMBER) ||
          (pc->type == CT_DC_MEMBER) ||
          ((prev != NULL) &&
           ((prev->type == CT_MEMBER) ||
            (prev->type == CT_DC_MEMBER))))
        {
        tmp = cpd.settings[UO_indent_member].n + indent_column;
        LOG_FMT(LINDENT, "%s: %d] member => %d\n",
                __func__, pc->orig_line, tmp);
        reindent_line(pc, tmp);
        }
      else if (chunk_is_str(pc, "<<", 2) && (cout_col > 0))
        {
        LOG_FMT(LINDENT, "%s: %d] cout_col => %d\n",
                __func__, pc->orig_line, cout_col);
        reindent_line(pc, cout_col);
        }
      else if ((vardefcol > 0) &&
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
        LOG_FMT(LINDENT, "%s: %d] String => %d\n",
                __func__, pc->orig_line, prev->column);
        reindent_line(pc, prev->column);
        }
      else if (chunk_is_comment(pc))
        {
        LOG_FMT(LINDENT, "%s: %d] comment => %d\n",
                __func__, pc->orig_line, frm.pse[frm.pse_tos].indent_tmp);
        indent_comment(pc, frm.pse[frm.pse_tos].indent_tmp);
        }
      else if (pc->type == CT_PREPROC)
        {
          /* Preprocs are always in column 1. See indent_preproc() */
        if (pc->column != 1)
          reindent_line(pc, 1);
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
      }

    /**
     * Handle variable definition continuation indenting
     */
    if ((pc->type == CT_WORD) &&
        ((pc->flags & PCF_IN_FCN_DEF) == 0) &&
        ((pc->flags & PCF_VAR_1ST_DEF) == PCF_VAR_1ST_DEF))
      vardefcol = pc->column;

    if (chunk_is_semicolon(pc) ||
        ((pc->type == CT_BRACE_OPEN) && (pc->parent_type == CT_FUNCTION)))
      vardefcol = 0;

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
        frm.pse[frm.pse_tos].indent_tmp = indent_size + 1;

        /* Get ready to indent the next item */
      did_newline = true;
      }

    if (!chunk_is_comment(pc) && !chunk_is_newline(pc))
      prev = pc;

    pc = chunk_get_next(pc);
    }

    /* Throw out any stuff inside a preprocessor - no need to warn */
  while ((frm.pse_tos > 0) && frm.pse[frm.pse_tos].in_preproc)
    indent_pse_pop(frm, pc);

  for (idx = 1; idx <= frm.pse_tos; idx++)
    {
    LOG_FMT(LWARN, "%s:%d Unmatched %s\n",
            cpd.filename, frm.pse[idx].open_line,
            get_token_name(frm.pse[idx].type));
    cpd.error_count++;
    }
  }

/**
 * returns true if forward scan reveals only single newlines or comments
 * stops when hits code
 * false if next thing hit is a closing brace, also if 2 newlines in a row
 */


static bool single_line_comment_indent_rule_applies(chunk_t *start)
  {
  chunk_t *pc      = start;
  int nl_count = 0;

  if (!chunk_is_single_line_comment(pc))
    return false;

    /* scan forward, if only single newlines and comments before next line of code, we want to apply */
  while ((pc = chunk_get_next(pc)) != NULL)
    {
    if (chunk_is_newline(pc))
      {
      if (nl_count > 0 || pc->nl_count > 1)
        return false;

      nl_count++;
      }
    else
      {
      nl_count = 0;

      if (!chunk_is_single_line_comment(pc))
        {
          /* here we check for things to run into that we wouldn't want to indent the comment for */
          /* for example, non-single line comment, closing brace */
        if (chunk_is_comment(pc) || chunk_is_closing_brace(pc))
          return false;

        return true;
        }
      }
    }

  return false;
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

    /* check if special single line comment rule applies */
  if (cpd.settings[UO_indent_sing_line_comments].n > 0 && single_line_comment_indent_rule_applies(pc))
    {
    pc->column = col + cpd.settings[UO_indent_sing_line_comments].n;
    LOG_FMT(LCMTIND, "rule 4 - single line comment indent, now in %d\n", pc->column);
    return;
    }

  LOG_FMT(LCMTIND, "rule 5 - fall-through, stay in %d\n", col);

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
  int pp_level;
  int pp_level_sub = 0;
  int tmp;

    /* Define a string of 16 spaces + # + 16 spaces */
  static const char *pp_str  = "                #                ";
  static const char *alt_str = "                %:                ";

    /* Scan to see if the whole file is covered by one #ifdef */
  int stage = 0;

  for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next(pc))
    {
    if (chunk_is_comment(pc) || chunk_is_newline(pc))
      continue;

    if (stage == 0)
      {
        /* Check the first PP, make sure it is an #if type */
      if (pc->type != CT_PREPROC)
        break;

      next = chunk_get_next(pc);

      if ((next == NULL) || (next->type != CT_PP_IF))
        break;

      stage = 1;
      }
    else if (stage == 1)
      {
        /* Scan until a PP at level 0 is found - the close to the #if */
      if ((pc->type == CT_PREPROC) &&
          (pc->pp_level == 0))
        stage = 2;

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
      continue;

    if (pc->column != 1)
      {
        /* Don't handle preprocessors that aren't in column 1 */
      LOG_FMT(LINFO, "%s: Line %d doesn't start in column 1 (%d)\n",
              __func__, pc->orig_line, pc->column);
      continue;
      }

      /* point into pp_str */
    if (pc->len == 2)
        /* alternate token crap */
      pc->str = &alt_str[16];
    else
      pc->str = &pp_str[16];

    pp_level = pc->pp_level - pp_level_sub;

    if (pp_level < 0)
      pp_level = 0;
    else if (pp_level > 16)
      pp_level = 16;

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
      pc->len += pp_level;

    next = chunk_get_next(pc);

    if (next != NULL)
      reindent_line(next, pc->len + 1);

    LOG_FMT(LPPIS, "%s: Indent line %d to %d (len %d, next->col %d)\n",
            __func__, pc->orig_line, pp_level, pc->len, next->column);
    }
  }
