/**
 * @file indent.cpp
 * Does all the indenting stuff.
 *
 * $Id: indent.cpp 548 2006-10-21 02:31:55Z bengardner $
 */
#include "uncrustify_types.h"
#include "chunk.h"
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

static void indent_comment(Chunk *pc, int col);


void indent_to_column(Chunk *pc, int column)
  {
  if (column < pc->GetColumn())
    column = pc->GetColumn();

  reindent_line(pc, column);
  }

/**
 * Changes the initial indent for a line to the given column
 *
 * @param pc      The chunk at the start of the line
 * @param column  The desired column
 */
void reindent_line(Chunk *pc, int column)
  {
  int col_delta;
  int min_col;

  LOG_FMT(LINDLINE, "%s: %d] col %d on %.*s [%s] => %d\n",
          __func__, pc->GetOrigLine(), pc->GetColumn(), pc->len, pc->GetStr(),
          get_token_name(pc->GetType()), column);

  if (column == pc->GetColumn())
    return;

  col_delta  = column - pc->GetColumn();
  pc->SetColumn(column);
  min_col    = pc->GetColumn();

  do
    {
    min_col += pc->len;
    pc       = pc->GetNext();

    if (pc != NULL)
      {
      if (pc->IsComment())
        {
        pc->SetColumn(pc->GetOrigCol());

        if (pc->GetColumn() < min_col)
          pc->SetColumn(min_col + 1);

        LOG_FMT(LINDLINE, "%s: set comment on line %d to col %d (orig %d)\n",
                __func__, pc->GetOrigLine(), pc->GetColumn(), pc->GetOrigCol());
        }
      else
        {
        pc->SetColumn(pc->GetColumn() + col_delta);

        if (pc->GetColumn() < min_col)
          pc->SetColumn(min_col);
        }
      }
    }
  while ((pc != NULL) && (pc->GetNlCount() == 0));
  }

/**
 * Starts a new entry
 *
 * @param frm  The parse frame
 * @param pc   The chunk causing the push
 */
static void indent_pse_push(struct parse_frame& frm, Chunk *pc)
  {
  static int ref = 0;

    /* check the stack depth */
  if (frm.m_parenStack_tos < (int)ARRAY_SIZE(frm.m_parenStack))
    {
      /* Bump up the index and initialize it */
    frm.pse_tos++;
    memset(&frm.m_parenStack[frm.m_parenStack_tos], 0, sizeof(frm.m_parenStack[frm.m_parenStack_tos]));

    LOG_FMT(LINDPSE, "%4d] OPEN  [%d,%s] level=%d\n",
            pc->GetOrigLine(), frm.pse_tos, get_token_name(pc->GetType()), pc->GetLevel());

    frm.m_parenStack[frm.m_parenStack_tos].type       = pc->GetType();
    frm.m_parenStack[frm.m_parenStack_tos].level      = pc->GetLevel();
    frm.m_parenStack[frm.m_parenStack_tos].open_line  = pc->GetOrigLine();
    frm.m_parenStack[frm.m_parenStack_tos].ref        = ++ref;
    frm.m_parenStack[frm.m_parenStack_tos].in_preproc = (pc->GetFlags() & PCF_IN_PREPROC) != 0;
    }
  }

/**
 * Removes the top entry
 *
 * @param frm  The parse frame
 * @param pc   The chunk causing the push
 */
static void indent_pse_pop(struct parse_frame& frm, Chunk *pc)
  {
    /* Bump up the index and initialize it */
  if (frm.pse_tos > 0)
    {
    if (pc != NULL)
      {
      LOG_FMT(LINDPSE, "%4d] CLOSE [%d,%s] on %s, started on line %d, level=%d/%d\n",
              pc->GetOrigLine(), frm.pse_tos,
              get_token_name(frm.m_parenStack[frm.m_parenStack_tos].type),
              get_token_name(pc->GetType()),
              frm.m_parenStack[frm.m_parenStack_tos].open_line,
              frm.m_parenStack[frm.m_parenStack_tos].level,
              pc->GetLevel());
      }
    else
      {
      LOG_FMT(LINDPSE, " EOF] CLOSE [%d,%s], started on line %d\n",
              frm.m_parenStack_tos, get_token_name(frm.m_parenStack[frm.m_parenStack_tos].type),
              frm.m_parenStack[frm.m_parenStack_tos].open_line);
      }

    frm.pse_tos--;
    }
  }

static int token_indent(E_Token type)
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
      return 0; //cpd.settings[UO_indent_braces].n;
    }
  }

/**
 * Change the top-level indentation only by changing the column member in
 * the chunk structures.
 * The level indicator must already be set.
 */
void indent_text(void)
  {
  Chunk            *pc;
  Chunk            *next;
  Chunk            *prev       = NULL;
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
  frm.m_parenStack[0].indent     = 1;
  frm.m_parenStack[0].indent_tmp = 1;
  frm.m_parenStack[0].type       = CT_EOF;

  pc = Chunk::GetHead();

  while (pc != NULL)
    {
      /* Handle proprocessor transitions */
    was_preproc = in_preproc;
    in_preproc  = (pc->GetFlags() & PCF_IN_PREPROC) != 0;

    if (cpd.settings[UO_indent_brace_parent].b)
      parent_token_indent = token_indent(pc->GetParentType());

      /* Clean up after a #define */
    if (!in_preproc)
      while ((frm.m_parenStack_tos > 0) && frm.m_parenStack[frm.m_parenStack_tos].in_preproc)
        indent_pse_pop(frm, pc);

    else
      {
      pf_check(&frm, pc);

      if (!was_preproc)
        {
          /* Transition into a preproc by creating a dummy indent */
        frm.level++;
        indent_pse_push(frm, pc);

        frm.m_parenStack[frm.m_parenStack_tos].indent     = 1 + indent_size;
        frm.m_parenStack[frm.m_parenStack_tos].indent_tmp = frm.m_parenStack[frm.m_parenStack_tos].indent;
        }
      }

    if ((cout_col > 0) &&
        (pc->IsSemicolon() ||
         (pc->GetLevel() < cout_level)))
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
      if (!pc->IsNewline() &&
          !pc->IsComment() &&
          ((pc->GetFlags() & PCF_IN_PREPROC) == 0) &&
          (frm.m_parenStack[frm.m_parenStack_tos].level > pc->GetLevel()))
        indent_pse_pop(frm, pc);

      if (frm.m_parenStack[frm.m_parenStack_tos].level == pc->GetLevel())
        {
          /* process virtual braces closes (no text output) */
        if ((pc->GetType() == CT_VBRACE_CLOSE) &&
            (frm.m_parenStack[frm.m_parenStack_tos].type == CT_VBRACE_OPEN))
          {
          indent_pse_pop(frm, pc);
          frm.level--;
          pc = pc->GetNext();
          }

          /* End any assign operations with a semicolon on the same level */
        if ((frm.m_parenStack[frm.m_parenStack_tos].type == CT_ASSIGN) &&
            (pc->IsSemicolon() ||
             (pc->GetType() == CT_COMMA) ||
             (pc->GetType() == CT_BRACE_OPEN)))
          indent_pse_pop(frm, pc);

          /* End any CPP class colon crap */
        if ((frm.m_parenStack[frm.m_parenStack_tos].type == CT_CLASS_COLON) &&
            ((pc->GetType() == CT_BRACE_OPEN) ||
             pc->IsSemicolon()))
          indent_pse_pop(frm, pc);

          /* a case is ended with another case or a close brace */
        if ((frm.m_parenStack[frm.m_parenStack_tos].type == CT_CASE) &&
            ((pc->GetType() == CT_BRACE_CLOSE) ||
             (pc->GetType() == CT_CASE)))
          indent_pse_pop(frm, pc);

          /* a return is ended with a semicolon */
        if ((frm.m_parenStack[frm.m_parenStack_tos].type == CT_RETURN) &&
            pc->IsSemicolon())
          indent_pse_pop(frm, pc);

          /* Close out parens and squares */
        if ((frm.m_parenStack[frm.m_parenStack_tos].type == (pc->GetType() - 1)) &&
            ((pc->GetType() == CT_PAREN_CLOSE) ||
             (pc->GetType() == CT_SPAREN_CLOSE) ||
             (pc->GetType() == CT_FPAREN_CLOSE) ||
             (pc->GetType() == CT_SQUARE_CLOSE) ||
             (pc->GetType() == CT_ANGLE_CLOSE)))
          {
          indent_pse_pop(frm, pc);
          frm.paren_count--;
          }
        }
      }
    while (old_pse_tos > frm.pse_tos);

      /* Grab a copy of the current indent */
    indent_column = frm.m_parenStack[frm.m_parenStack_tos].indent_tmp;

    if (!pc->IsNewline() && !pc->IsComment())
      {
      LOG_FMT(LINDPC, " -=[ %.*s ]=- top=%d %s %d/%d\n",
              pc->len, pc->GetStr(),
              frm.pse_tos,
              get_token_name(frm.m_parenStack[frm.m_parenStack_tos].type),
              frm.m_parenStack[frm.m_parenStack_tos].indent_tmp,
              frm.m_parenStack[frm.m_parenStack_tos].indent);
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

    if (pc->GetType() == CT_BRACE_CLOSE)
      {
      if (frm.m_parenStack[frm.m_parenStack_tos].type == CT_BRACE_OPEN)
        {
        indent_pse_pop(frm, pc);
        frm.level--;

          /* Update the indent_column if needed */
        if (!cpd.settings[UO_indent_braces].b &&
            (parent_token_indent == 0))
          indent_column = frm.m_parenStack[frm.m_parenStack_tos].indent_tmp;

        if ((pc->GetParentType() == CT_IF) ||
            (pc->GetParentType() == CT_ELSE) ||
            (pc->GetParentType() == CT_ELSEIF) ||
            (pc->GetParentType() == CT_DO) ||
            (pc->GetParentType() == CT_WHILE) ||
            (pc->GetParentType() == CT_SWITCH) ||
            (pc->GetParentType() == CT_FOR))
          indent_column += cpd.settings[UO_indent_brace].n;
        }
      }
    else if (pc->GetType() == CT_VBRACE_OPEN)
      {
      frm.level++;
      indent_pse_push(frm, pc);

      frm.m_parenStack[frm.m_parenStack_tos].indent     = frm.m_parenStack[frm.m_parenStack_tos - 1].indent + indent_size;
      frm.m_parenStack[frm.m_parenStack_tos].indent_tmp = frm.m_parenStack[frm.m_parenStack_tos].indent;

        /* Always indent on virtual braces */
      indent_column = frm.m_parenStack[frm.m_parenStack_tos].indent_tmp;
      }
    else if (pc->GetType() == CT_BRACE_OPEN)
      {
      frm.level++;
      indent_pse_push(frm, pc);

      if (frm.paren_count != 0)
          /* We are inside ({ ... }) -- indent one tab from the paren */
        frm.m_parenStack[frm.m_parenStack_tos].indent = frm.m_parenStack[frm.m_parenStack_tos - 1].indent_tmp + indent_size;
      else
        {
          /* Use the prev indent level + indent_size. */
        frm.m_parenStack[frm.m_parenStack_tos].indent = frm.m_parenStack[frm.m_parenStack_tos - 1].indent + indent_size;

          /* If this brace is part of a statement, bump it out by indent_brace */
        if ((pc->GetParentType() == CT_IF) ||
            (pc->GetParentType() == CT_ELSE) ||
            (pc->GetParentType() == CT_ELSEIF) ||
            (pc->GetParentType() == CT_DO) ||
            (pc->GetParentType() == CT_WHILE) ||
            (pc->GetParentType() == CT_SWITCH) ||
            (pc->GetParentType() == CT_FOR))
          {
          if (parent_token_indent != 0)
            frm.m_parenStack[frm.m_parenStack_tos].indent += parent_token_indent - indent_size;
          else
            {
            frm.m_parenStack[frm.m_parenStack_tos].indent += cpd.settings[UO_indent_brace].n;
            indent_column += cpd.settings[UO_indent_brace].n;
            }
          }
        else if (pc->GetParentType() == CT_CASE)
          {
            /* The indent_case_brace setting affects the parent CT_CASE */
          frm.m_parenStack[frm.m_parenStack_tos].indent_tmp += cpd.settings[UO_indent_case_brace].n;
          frm.m_parenStack[frm.m_parenStack_tos].indent     += cpd.settings[UO_indent_case_brace].n;
          }
        else if ((pc->GetParentType() == CT_CLASS) && !cpd.settings[UO_indent_class].b)
          frm.m_parenStack[frm.m_parenStack_tos].indent -= indent_size;
        else if ((pc->GetParentType() == CT_NAMESPACE) && !cpd.settings[UO_indent_namespace].b)
          frm.m_parenStack[frm.m_parenStack_tos].indent -= indent_size;
        }

      if ((pc->GetFlags() & PCF_DONT_INDENT) != 0)
        {
        frm.m_parenStack[frm.m_parenStack_tos].indent = pc->GetColumn();
        indent_column = pc->GetColumn();
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
        next = pc->GetNextNcNnl();

        if (!pc->IsNewlineBetween(next))
          frm.m_parenStack[frm.m_parenStack_tos].indent = next->GetColumn();

        frm.m_parenStack[frm.m_parenStack_tos].indent_tmp = frm.m_parenStack[frm.m_parenStack_tos].indent;
        frm.m_parenStack[frm.m_parenStack_tos].open_line  = pc->GetOrigLine();

          /* Update the indent_column if needed */
        if (cpd.settings[UO_indent_braces].n ||
            (parent_token_indent != 0))
          indent_column = frm.m_parenStack[frm.m_parenStack_tos].indent_tmp;
        }
      }
    else if (pc->GetType() == CT_CASE)
      {
        /* Start a case - indent UO_indent_switch_case from the switch level */
      tmp = frm.m_parenStack[frm.m_parenStack_tos].indent + cpd.settings[UO_indent_switch_case].n;

      indent_pse_push(frm, pc);

      frm.m_parenStack[frm.m_parenStack_tos].indent     = tmp;
      frm.m_parenStack[frm.m_parenStack_tos].indent_tmp = tmp - indent_size;

        /* Always set on case statements */
      indent_column = frm.m_parenStack[frm.m_parenStack_tos].indent_tmp;
      }
    else if (pc->GetType() == CT_LABEL)
      {
        /* Labels get sent to the left or backed up */
      if (cpd.settings[UO_indent_label].n > 0)
        indent_column = cpd.settings[UO_indent_label].n;
      else
        indent_column = frm.m_parenStack[frm.m_parenStack_tos].indent +
                        cpd.settings[UO_indent_label].n;
      }
    else if (pc->GetType() == CT_CLASS_COLON)
      {
        /* just indent one level */
      indent_pse_push(frm, pc);
      frm.m_parenStack[frm.m_parenStack_tos].indent     = frm.m_parenStack[frm.m_parenStack_tos - 1].indent_tmp + indent_size;
      frm.m_parenStack[frm.m_parenStack_tos].indent_tmp = frm.m_parenStack[frm.m_parenStack_tos].indent;

      indent_column = frm.m_parenStack[frm.m_parenStack_tos].indent_tmp;

      if (cpd.settings[UO_indent_class_colon].b)
        {
        prev = pc->GetPrev();

        if (prev->IsNewline())
          {
          frm.m_parenStack[frm.m_parenStack_tos].indent += 2;
          /* don't change indent of current line */
          }
        }
      }
    else if ((pc->GetType() == CT_PAREN_OPEN) ||
             (pc->GetType() == CT_SPAREN_OPEN) ||
             (pc->GetType() == CT_FPAREN_OPEN) ||
             (pc->GetType() == CT_SQUARE_OPEN) ||
             (pc->GetType() == CT_ANGLE_OPEN))
      {
        /* Open parens and squares - never update indent_column */
      indent_pse_push(frm, pc);
      frm.m_parenStack[frm.m_parenStack_tos].indent = pc->GetColumn() + pc->len;

      if (cpd.settings[UO_indent_func_call_param].b &&
          (pc->GetType() == CT_FPAREN_OPEN) &&
          (pc->GetParentType() == CT_FUNC_CALL))
        frm.m_parenStack[frm.m_parenStack_tos].indent = frm.m_parenStack[frm.m_parenStack_tos - 1].indent + indent_size;

      if ((chunk_is_str(pc, "(", 1) && !cpd.settings[UO_indent_paren_nl].b) ||
          (chunk_is_str(pc, "[", 1) && !cpd.settings[UO_indent_square_nl].b))
        {
        next = pc->GetNextNc();

        if (next->IsNewline())
          {
          int sub = 1;

          if (frm.m_parenStack[frm.m_parenStack_tos - 1].type == CT_ASSIGN)
            sub = 2;

          frm.m_parenStack[frm.m_parenStack_tos].indent = frm.m_parenStack[frm.m_parenStack_tos - sub].indent + indent_size;
          }
        }

      frm.m_parenStack[frm.m_parenStack_tos].indent_tmp = frm.m_parenStack[frm.m_parenStack_tos].indent;
      frm.paren_count++;
      }
    else if (pc->GetType() == CT_ASSIGN)
      {
      /**
       * if there is a newline after the '=', just indent one level,
       * otherwise align on the '='.
       * Never update indent_column.
       */
      next = pc->GetNext();

      if (next != NULL)
        {
        indent_pse_push(frm, pc);

        if (next->IsNewline())
          frm.m_parenStack[frm.m_parenStack_tos].indent = frm.m_parenStack[frm.m_parenStack_tos - 1].indent_tmp + indent_size;
        else
          frm.m_parenStack[frm.m_parenStack_tos].indent = pc->GetColumn() + pc->len + 1;

        frm.m_parenStack[frm.m_parenStack_tos].indent_tmp = frm.m_parenStack[frm.m_parenStack_tos].indent;
        }
      }
    else if (pc->GetType() == CT_RETURN)
      {
        /* don't count returns inside a () or [] */
      if (pc->GetLevel() == pc->GetBraceLevel())
        {
        indent_pse_push(frm, pc);
        frm.m_parenStack[frm.m_parenStack_tos].indent     = frm.m_parenStack[frm.m_parenStack_tos - 1].indent + pc->len + 1;
        frm.m_parenStack[frm.m_parenStack_tos].indent_tmp = frm.m_parenStack[frm.m_parenStack_tos - 1].indent;
        }
      }
    else if (chunk_is_str(pc, "<<", 2))
      {
      if (cout_col == 0)
        {
        cout_col   = pc->GetColumn();
        cout_level = pc->GetLevel();
        }
      }
    else
      {
      /* anything else? */
      }

    /**
     * Indent the line if needed
     */
    if (did_newline && !pc->IsNewline() && (pc->len != 0))
      {
      /**
       * Check for special continuations.
       * Note that some of these could be done as a stack item like
       * everything else
       */

      prev = pc->GetPrevNcNnl();

      if ((pc->GetType() == CT_MEMBER) ||
          (pc->GetType() == CT_DC_MEMBER) ||
          ((prev != NULL) &&
           ((prev->GetType() == CT_MEMBER) ||
            (prev->GetType() == CT_DC_MEMBER))))
        {
        tmp = cpd.settings[UO_indent_member].n + indent_column;
        LOG_FMT(LINDENT, "%s: %d] member => %d\n",
                __func__, pc->GetOrigLine(), tmp);
        reindent_line(pc, tmp);
        }
      else if (chunk_is_str(pc, "<<", 2) && (cout_col > 0))
        {
        LOG_FMT(LINDENT, "%s: %d] cout_col => %d\n",
                __func__, pc->GetOrigLine(), cout_col);
        reindent_line(pc, cout_col);
        }
      else if ((vardefcol > 0) &&
               (pc->GetType() == CT_WORD) &&
               ((pc->GetFlags() & PCF_VAR_DEF) != 0) &&
               (prev != NULL) && (prev->GetType() == CT_COMMA))
        {
        LOG_FMT(LINDENT, "%s: %d] Vardefcol => %d\n",
                __func__, pc->GetOrigLine(), vardefcol);
        reindent_line(pc, vardefcol);
        }
      else if ((pc->GetType() == CT_STRING) && (prev->GetType() == CT_STRING) &&
               cpd.settings[UO_indent_align_string].b)
        {
        LOG_FMT(LINDENT, "%s: %d] String => %d\n",
                __func__, pc->GetOrigLine(), prev->GetColumn());
        reindent_line(pc, prev->GetColumn());
        }
      else if (pc->IsComment())
        {
        LOG_FMT(LINDENT, "%s: %d] comment => %d\n",
                __func__, pc->GetOrigLine(), frm.m_parenStack[frm.m_parenStack_tos].indent_tmp);
        indent_comment(pc, frm.m_parenStack[frm.m_parenStack_tos].indent_tmp);
        }
      else if (pc->GetType() == CT_PREPROC)
        {
          /* Preprocs are always in column 1. See indent_preproc() */
        if (pc->GetColumn() != 1)
          reindent_line(pc, 1);
        }
      else
        {
        if (pc->GetColumn() != indent_column)
          {
          LOG_FMT(LINDENT, "%s: %d] indent => %d [%.*s]\n",
                  __func__, pc->GetOrigLine(), indent_column, pc->len, pc->GetStr());
          reindent_line(pc, indent_column);
          }
        }

      did_newline = false;
      }

    /**
     * Handle variable definition continuation indenting
     */
    if ((pc->GetType() == CT_WORD) &&
        ((pc->GetFlags() & PCF_IN_FCN_DEF) == 0) &&
        ((pc->GetFlags() & PCF_VAR_1ST_DEF) == PCF_VAR_1ST_DEF))
      vardefcol = pc->GetColumn();

    if (pc->IsSemicolon() ||
        ((pc->GetType() == CT_BRACE_OPEN) && (pc->GetParentType() == CT_FUNCTION)))
      vardefcol = 0;

      /* if we hit a newline, reset indent_tmp */
    if (pc->IsNewline() ||
        (pc->GetType() == CT_COMMENT_MULTI) ||
        (pc->GetType() == CT_COMMENT_CPP))
      {
      frm.m_parenStack[frm.m_parenStack_tos].indent_tmp = frm.m_parenStack[frm.m_parenStack_tos].indent;

      /**
       * Handle the case of a multi-line #define w/o anything on the
       * first line (indent_tmp will be 1 or 0)
       */
      if ((pc->GetType() == CT_NL_CONT) &&
          (frm.m_parenStack[frm.m_parenStack_tos].indent_tmp <= indent_size))
        frm.m_parenStack[frm.m_parenStack_tos].indent_tmp = indent_size + 1;

        /* Get ready to indent the next item */
      did_newline = true;
      }

    if (!pc->IsComment() && !pc->IsNewline())
      prev = pc;

    pc = pc->GetNext();
    }

    /* Throw out any stuff inside a preprocessor - no need to warn */
  while ((frm.m_parenStack_tos > 0) && frm.m_parenStack[frm.m_parenStack_tos].in_preproc)
    indent_pse_pop(frm, pc);

  for (idx = 1; idx <= frm.pse_tos; idx++)
    {
    LOG_FMT(LWARN, "%s:%d Unmatched %s\n",
            cpd.filename, frm.m_parenStack[idx].open_line,
            get_token_name(frm.m_parenStack[idx].type));
    cpd.error_count++;
    }
  }

/**
 * returns true if forward scan reveals only single newlines or comments
 * stops when hits code
 * false if next thing hit is a closing brace, also if 2 newlines in a row
 */


static bool single_line_comment_indent_rule_applies(Chunk *start)
  {
  Chunk *pc      = start;
  int nl_count = 0;

  if (!pc->IsSingleLineComment())
    return false;

    /* scan forward, if only single newlines and comments before next line of code, we want to apply */
  while ((pc = pc->GetNext()) != NULL)
    {
    if (pc->IsNewline())
      {
      if (nl_count > 0 || pc->GetNlCount() > 1)
        return false;

      nl_count++;
      }
    else
      {
      nl_count = 0;

      if (!pc->IsSingleLineComment())
        {
          /* here we check for things to run into that we wouldn't want to indent the comment for */
          /* for example, non-single line comment, closing brace */
        if (pc->IsComment() || pc->IsBraceClose())
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
static void indent_comment(Chunk *pc, int col)
  {
  Chunk *nl;
  Chunk *prev;

  LOG_FMT(LCMTIND, "%s: line %d, col %d, level %d: ", __func__,
          pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel());

    /* force column 1 comment to column 1 if not changing them */
  if ((pc->GetOrigCol() == 1) && !cpd.settings[UO_indent_col1_comment].b)
    {
    LOG_FMT(LCMTIND, "rule 1 - keep in col 1\n");
    pc->SetColumn(1);
    return;
    }

  nl = pc->GetPrev();

    /* outside of any expression or statement? */
  if (pc->GetLevel() == 0)
    {
    if ((nl != NULL) && (nl->GetNlCount() > 1))
      {
      LOG_FMT(LCMTIND, "rule 2 - level 0, nl before\n");
      pc->SetColumn(1);
      return;
      }
    }

  prev = nl->GetPrev();

  if (prev->IsComment() && (nl->GetNlCount() == 1))
    {
    int coldiff = prev->GetOrigCol() - pc->GetOrigCol();

    if ((coldiff <= 3) && (coldiff >= -3))
      {
      pc->SetColumn(prev->GetColumn());
      LOG_FMT(LCMTIND, "rule 3 - prev comment, coldiff = %d, now in %d\n",
              coldiff, pc->GetColumn());
      return;
      }
    }

    /* check if special single line comment rule applies */
  if (cpd.settings[UO_indent_sing_line_comments].n > 0 && single_line_comment_indent_rule_applies(pc))
    {
    pc->SetColumn(col + cpd.settings[UO_indent_sing_line_comments].n);
    LOG_FMT(LCMTIND, "rule 4 - single line comment indent, now in %d\n", pc->GetColumn());
    return;
    }

  LOG_FMT(LCMTIND, "rule 5 - fall-through, stay in %d\n", col);

  pc->SetColumn(col);
  }

/**
 * Put spaces on either side of the preproc (#) symbol.
 * This is done by pointing pc->GetStr() into pp_str and adjusting the
 * length.
 */
void indent_preproc(void)
  {
  Chunk *pc;
  Chunk *next;
  int pp_level;
  int pp_level_sub = 0;
  int tmp;

    /* Define a string of 16 spaces + # + 16 spaces */
  static const char *pp_str  = "                #                ";
  static const char *alt_str = "                %:                ";

    /* Scan to see if the whole file is covered by one #ifdef */
  int stage = 0;

  for (pc = Chunk::GetHead(); pc != NULL; pc = pc->GetNext())
    {
    if (pc->IsComment() || pc->IsNewline())
      continue;

    if (stage == 0)
      {
        /* Check the first PP, make sure it is an #if type */
      if (pc->GetType() != CT_PREPROC)
        break;

      next = pc->GetNext();

      if ((next == NULL) || (next->GetType() != CT_PP_IF))
        break;

      stage = 1;
      }
    else if (stage == 1)
      {
        /* Scan until a PP at level 0 is found - the close to the #if */
      if ((pc->GetType() == CT_PREPROC) &&
          (pc->GetPpLevel() == 0))
        stage = 2;

      continue;
      }
    else if (stage == 2)
      {
        /* We should only see the rest of the preprocessor */
      if ((pc->GetType() == CT_PREPROC) ||
          ((pc->GetFlags() & PCF_IN_PREPROC) == 0))
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

  for (pc = Chunk::GetHead(); pc != NULL; pc = pc->GetNext())
    {
    if (pc->GetType() != CT_PREPROC)
      continue;

    if (pc->GetColumn() != 1)
      {
        /* Don't handle preprocessors that aren't in column 1 */
      LOG_FMT(LINFO, "%s: Line %d doesn't start in column 1 (%d)\n",
              __func__, pc->GetOrigLine(), pc->GetColumn());
      continue;
      }

      /* point into pp_str */
    if (pc->len == 2)
        /* alternate token crap */
      pc->Str() = &alt_str[16];
    else
      pc->Str() = &pp_str[16];

    pp_level = pc->GetPpLevel() - pp_level_sub;

    if (pp_level < 0)
      pp_level = 0;
    else if (pp_level > 16)
      pp_level = 16;

      /* Note that the indent is removed by default */
    if ((cpd.settings[UO_pp_indent].a & AV_ADD) != 0)
      {
        /* Need to add some spaces */
      pc->Str() -= pp_level;
      pc->len += pp_level;
      }
    else if (cpd.settings[UO_pp_indent].a == AV_IGNORE)
      {
      tmp      = (pc->GetOrigCol() <= 16) ? pc->GetOrigCol() - 1 : 16;
      pc->Str() -= tmp;
      pc->len += tmp;
      }

      /* Add spacing by adjusting the length */
    if ((cpd.settings[UO_pp_space].a & AV_ADD) != 0)
      pc->len += pp_level;

    next = pc->GetNext();

    if (next != NULL)
      reindent_line(next, pc->len + 1);

    LOG_FMT(LPPIS, "%s: Indent line %d to %d (len %d, next->col %d)\n",
            __func__, pc->GetOrigLine(), GetPpLevel(), pc->len, next->GetColumn());
    }
  }
