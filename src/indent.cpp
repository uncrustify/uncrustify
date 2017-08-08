/**
 * @file indent.cpp
 * Does all the indenting stuff.
 *
 * @author  Ben Gardner
 * @author  Guy Maurel since version 0.62 for uncrustify4Qt
 *          October 2015, 2016
 * @license GPL v2+
 */
#include "indent.h"
#include "uncrustify_types.h"
#include "chunk_list.h"
#include "prototypes.h"
#include "options_for_QT.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include "unc_ctype.h"
#include "uncrustify.h"
#include "align.h"
#include "space.h"
#include "parse_frame.h"
#include "helper_for_print.h"


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
 * Return: same as assignments
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
static void indent_comment(chunk_t *pc, size_t col);


/**
 * Starts a new entry
 *
 * @param frm  The parse frame
 * @param pc   The chunk causing the push
 */
static void indent_pse_push(parse_frame_t &frm, chunk_t *pc);


/**
 * Removes the top entry
 *
 * @param frm  The parse frame
 * @param pc   The chunk causing the push
 */
static void indent_pse_pop(parse_frame_t &frm, chunk_t *pc);


static size_t token_indent(c_token_t type);


static size_t calc_indent_continue(parse_frame_t &frm, size_t pse_tos);


/**
 * We are on a '{' that has parent = OC_BLOCK_EXPR
 * find the column of the param tag
 */
static chunk_t *oc_msg_block_indent(chunk_t *pc, bool from_brace, bool from_caret, bool from_colon, bool from_keyword);


//! We are on a '{' that has parent = OC_BLOCK_EXPR
static chunk_t *oc_msg_prev_colon(chunk_t *pc);


/**
 * returns true if forward scan reveals only single newlines or comments
 * stops when hits code
 * false if next thing hit is a closing brace, also if 2 newlines in a row
 */
static bool single_line_comment_indent_rule_applies(chunk_t *start);


void indent_to_column(chunk_t *pc, size_t column)
{
   LOG_FUNC_ENTRY();
   if (column < pc->column)
   {
      column = pc->column;
   }
   reindent_line(pc, column);
}


enum class align_mode_e : unsigned int
{
   SHIFT,     //! shift relative to the current column
   KEEP_ABS,  //! try to keep the original absolute column
   KEEP_REL,  //! try to keep the original gap
};


void align_to_column(chunk_t *pc, size_t column)
{
   LOG_FUNC_ENTRY();
   if (pc == nullptr || column == pc->column)
   {
      return;
   }

   LOG_FMT(LINDLINE, "%s(%d): %zu] col %zu on %s [%s] => %zu\n",
           __func__, __LINE__, pc->orig_line, pc->column, pc->text(),
           get_token_name(pc->type), column);

   const auto col_delta = static_cast<int>(column) - static_cast<int>(pc->column);
   size_t     min_col   = column;

   pc->column = column;
   do
   {
      auto *next = chunk_get_next(pc);
      if (next == nullptr)
      {
         break;
      }

      auto         almod = align_mode_e::SHIFT;

      const size_t min_delta = space_col_align(pc, next);
      min_col += min_delta;

      const auto *prev = pc;
      pc = next;

      if (chunk_is_comment(pc) && pc->parent_type != CT_COMMENT_EMBED)
      {
         almod = (  chunk_is_single_line_comment(pc)
                 && cpd.settings[UO_indent_relative_single_line_comments].b)
                 ? align_mode_e::KEEP_REL : align_mode_e::KEEP_ABS;
      }

      if (almod == align_mode_e::KEEP_ABS)
      {
         // Keep same absolute column
         pc->column = max(pc->orig_col, min_col);
      }
      else if (almod == align_mode_e::KEEP_REL)
      {
         // Keep same relative column
         auto orig_delta = static_cast<int>(pc->orig_col) - static_cast<int>(prev->orig_col);
         orig_delta = max<int>(orig_delta, min_delta);  // keeps orig_delta positive

         pc->column = prev->column + static_cast<size_t>(orig_delta);
      }
      else // SHIFT
      {
         // Shift by the same amount, keep above negative values
         pc->column = (  col_delta >= 0
                      || cast_abs(pc->column, col_delta) < pc->column)
                      ? pc->column + col_delta : 0;
         pc->column = max(pc->column, min_col);
      }
      LOG_FMT(LINDLINED, "   %s set column of %s on line %zu to col %zu (orig %zu)\n",
              (almod == align_mode_e::KEEP_ABS) ? "abs" :
              (almod == align_mode_e::KEEP_REL) ? "rel" : "sft",
              get_token_name(pc->type), pc->orig_line, pc->column, pc->orig_col);
   } while (pc != nullptr && pc->nl_count == 0);
} // align_to_column


void reindent_line(chunk_t *pc, size_t column)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LINDLINE, "%s(%d): orig_line is %zu, orig_col is %zu, on '%s' [%s/%s] => %zu",
           __func__, __LINE__, pc->orig_line, pc->column, pc->text(),
           get_token_name(pc->type), get_token_name(pc->parent_type),
           column);
#ifdef DEBUG
   LOG_FMT(LINDLINE, "\n");
#endif
   log_func_stack_inline(LINDLINE);

   if (column == pc->column)
   {
      return;
   }

   auto col_delta = static_cast<int>(column) - static_cast<int>(pc->column);
   auto min_col   = column;

   pc->column = column;
   do
   {
      if (QT_SIGNAL_SLOT_found)
      {
         // fix the bug #654
         // connect(&mapper, SIGNAL(mapped(QString &)), this, SLOT(onSomeEvent(QString &)));
         // look for end of SIGNAL/SLOT block
         if (!(pc->flags & PCF_IN_QT_MACRO))
         {
            LOG_FMT(LINDLINE, "FLAGS is NOT set: PCF_IN_QT_MACRO\n");
            restore_options_for_QT();
         }
      }
      else
      {
         // look for begin of SIGNAL/SLOT block
         if (pc->flags & PCF_IN_QT_MACRO)
         {
            LOG_FMT(LINDLINE, "FLAGS is set: PCF_IN_QT_MACRO\n");
            save_set_options_for_QT(pc->level);
         }
      }
      chunk_t *next = chunk_get_next(pc);

      if (next == nullptr)
      {
         break;
      }
      if (pc->nl_count)
      {
         min_col   = 0;
         col_delta = 0;
      }
      min_col += space_col_align(pc, next);
      pc       = next;

      bool is_comment = chunk_is_comment(pc);
      bool keep       = (  is_comment
                        && chunk_is_single_line_comment(pc)
                        && cpd.settings[UO_indent_relative_single_line_comments].b);

      if (  is_comment
         && pc->parent_type != CT_COMMENT_EMBED
         && !keep)
      {
         pc->column = max(pc->orig_col, min_col);
         LOG_FMT(LINDLINE, "%s(%d): set comment on line %zu to col %zu (orig %zu)\n",
                 __func__, __LINE__, pc->orig_line, pc->column, pc->orig_col);
      }
      else
      {
         auto tmp_col = static_cast<int>(pc->column) + col_delta;
         pc->column = max(tmp_col, static_cast<int>(min_col));

         LOG_FMT(LINDLINED, "   set column of ");
         if (pc->type == CT_NEWLINE)
         {
            LOG_FMT(LINDLINED, "NEWLINE");
         }
         else
         {
            LOG_FMT(LINDLINED, "'%s'", pc->text());
         }
         LOG_FMT(LINDLINED, " to %zu (orig %zu)\n", pc->column, pc->orig_col);
      }
   } while (pc != nullptr && pc->nl_count == 0);
} // reindent_line


static void indent_pse_push(parse_frame_t &frm, chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   static size_t ref = 0;

   // check the stack depth
   if (frm.pse_tos < (ARRAY_SIZE(frm.pse) - 1))
   {
      // Bump up the index and initialize it
      frm.pse_tos++;
      LOG_FMT(LINDLINE, "%s(%d): orig_line is %zu, pse_tos is %zu, type is %s, brace_level is %zu, pc->level is %zu\n",
              __func__, __LINE__, pc->orig_line, frm.pse_tos, get_token_name(pc->type), pc->brace_level, pc->level);
      memset(&frm.pse[frm.pse_tos], 0, sizeof(frm.pse[frm.pse_tos]));

      //LOG_FMT(LINDPSE, "%s(%d):%d] (pp=%d) OPEN  [%d,%s] level=%d\n",
      //        __func__, __LINE__, pc->orig_line, cpd.pp_level, frm.pse_tos, get_token_name(pc->type), pc->level);

      frm.pse[frm.pse_tos].pc          = pc;
      frm.pse[frm.pse_tos].type        = pc->type;
      frm.pse[frm.pse_tos].level       = pc->level;
      frm.pse[frm.pse_tos].open_line   = pc->orig_line;
      frm.pse[frm.pse_tos].ref         = ++ref;
      frm.pse[frm.pse_tos].in_preproc  = (pc->flags & PCF_IN_PREPROC);
      frm.pse[frm.pse_tos].indent_tab  = frm.pse[frm.pse_tos - 1].indent_tab;
      frm.pse[frm.pse_tos].indent_cont = frm.pse[frm.pse_tos - 1].indent_cont;
      frm.pse[frm.pse_tos].non_vardef  = false;
      memcpy(&frm.pse[frm.pse_tos].ip, &frm.pse[frm.pse_tos - 1].ip, sizeof(frm.pse[frm.pse_tos].ip));
   }
   else
   {
      // the stack depth is too small
      // fatal error
      fprintf(stderr, "the stack depth is too small\n");
      log_flush(true);
      exit(EXIT_FAILURE);
   }
}


static void indent_pse_pop(parse_frame_t &frm, chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   // Bump up the index and initialize it
   if (frm.pse_tos > 0)
   {
      if (pc != nullptr)
      {
         LOG_FMT(LINDPSE, "%s(%d):\n",
                 __func__, __LINE__);
         LOG_FMT(LINDPSE, "  %zu] (pp=%d) CLOSE [%zu,%s] on %s, started on line %zu, level=%zu/%zu\n",
                 pc->orig_line, cpd.pp_level, frm.pse_tos,
                 get_token_name(frm.pse[frm.pse_tos].type),
                 get_token_name(pc->type),
                 frm.pse[frm.pse_tos].open_line,
                 frm.pse[frm.pse_tos].level,
                 pc->level);
      }
      else
      {
         LOG_FMT(LINDPSE, "%s(%d):\n",
                 __func__, __LINE__);
         LOG_FMT(LINDPSE, "   EOF] CLOSE [%zu,%s], started on line %zu\n",
                 frm.pse_tos, get_token_name(frm.pse[frm.pse_tos].type),
                 frm.pse[frm.pse_tos].open_line);
      }

      /*
       * Don't clear the stack entry because some code 'cheats' and uses the
       * just-popped indent values
       */
      frm.pse_tos--;
      if (pc != nullptr)
      {
         LOG_FMT(LINDLINE, "%s(%d): orig_line is %zu, pse_tos is %zu, type is %s\n",
                 __func__, __LINE__, pc->orig_line, frm.pse_tos, get_token_name(pc->type));
      }
      else
      {
         LOG_FMT(LINDLINE, "%s(%d): ------------------- pse_tos is %zu\n",
                 __func__, __LINE__, frm.pse_tos);
      }
   }
   else
   {
      // fatal error
      fprintf(stderr, "the stack index is already zero\n");
      char *outputMessage;
      outputMessage = make_message("at line=%zu, type is %s\n",
                                   pc->orig_line, get_token_name(pc->type));
      fprintf(stderr, "%s", outputMessage);
      free(outputMessage);
      log_flush(true);
      exit(EXIT_FAILURE);
   }
} // indent_pse_pop


static size_t token_indent(c_token_t type)
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
   case CT_USING_STMT:
      return(6);

   case CT_SWITCH:
      return(7);

   case CT_ELSEIF:
      return(8);

   case CT_SYNCHRONIZED:
      return(13);

   default:
      return(0);
   }
}


#define indent_column_set(X)                                                                 \
   do {                                                                                      \
      LOG_FMT(LINDENT2, "%s(%d): orig_line is %zu, indent_column changed from %zu to %zu\n", \
              __func__, __LINE__, pc->orig_line, indent_column, (size_t)X);                  \
      indent_column = (X);                                                                   \
   } while (false)


static size_t calc_indent_continue(parse_frame_t &frm, size_t pse_tos)
{
   int ic = cpd.settings[UO_indent_continue].n;

   if (ic < 0 && frm.pse[pse_tos].indent_cont)
   {
      return(frm.pse[pse_tos].indent);
   }
   return(frm.pse[pse_tos].indent + abs(ic));
}


static chunk_t *oc_msg_block_indent(chunk_t *pc, bool from_brace,
                                    bool from_caret, bool from_colon,
                                    bool from_keyword)
{
   LOG_FUNC_ENTRY();
   chunk_t *tmp = chunk_get_prev_nc(pc);

   if (from_brace)
   {
      return(pc);
   }

   if (chunk_is_paren_close(tmp))
   {
      tmp = chunk_get_prev_nc(chunk_skip_to_match_rev(tmp));
   }
   if (!tmp || tmp->type != CT_OC_BLOCK_CARET)
   {
      return(nullptr);
   }
   if (from_caret)
   {
      return(tmp);
   }
   tmp = chunk_get_prev_nc(tmp);
   if (!tmp || tmp->type != CT_OC_COLON)
   {
      return(nullptr);
   }
   if (from_colon)
   {
      return(tmp);
   }
   tmp = chunk_get_prev_nc(tmp);
   if (  !tmp
      || (tmp->type != CT_OC_MSG_NAME && tmp->type != CT_OC_MSG_FUNC))
   {
      return(nullptr);
   }
   if (from_keyword)
   {
      return(tmp);
   }
   return(nullptr);
} // oc_msg_block_indent


static chunk_t *oc_msg_prev_colon(chunk_t *pc)
{
   return(chunk_get_prev_type(pc, CT_OC_COLON, pc->level, scope_e::ALL));
}


#define log_indent()                           \
   do { _log_indent(__func__, __LINE__, &frm); \
   } while (false)


static void _log_indent(const char *func, const uint32_t line, parse_frame_t *frm)
{
   LOG_FMT(LINDLINE, "%s(%d): frm.pse_tos is %zu, ...indent is %zu\n",
           func, line, frm->pse_tos, frm->pse[frm->pse_tos].indent);
}


#define log_indent_tmp()                           \
   do { _log_indent_tmp(__func__, __LINE__, &frm); \
   } while (false)


static void _log_indent_tmp(const char *func, const uint32_t line, parse_frame_t *frm)
{
   LOG_FMT(LINDLINE, "%s(%d): frm.pse_tos is %zu, ...indent_tmp is %zu\n",
           func, line, frm->pse_tos, frm->pse[frm->pse_tos].indent_tmp);
}


void indent_text(void)
{
   LOG_FUNC_ENTRY();
   chunk_t       *pc;
   chunk_t       *next;
   chunk_t       *prev       = nullptr;
   bool          did_newline = true;
   int           idx;
   size_t        vardefcol    = 0;
   size_t        shiftcontcol = 0;
   size_t        indent_size  = cpd.settings[UO_indent_columns].u;
   parse_frame_t frm;
   bool          in_preproc          = false;
   size_t        indent_column       = 0;
   size_t        parent_token_indent = 0;
   int           xml_indent          = 0;
   bool          token_used;
   size_t        sql_col      = 0;
   size_t        sql_orig_col = 0;
   bool          in_func_def  = false;
   c_token_t     memtype;

   memset(&frm, 0, sizeof(frm));
   cpd.frame_count = 0;

   // dummy top-level entry
   frm.pse[0].indent     = 1;
   frm.pse[0].indent_tmp = 1;
   frm.pse[0].indent_tab = 1;
   frm.pse[0].type       = CT_EOF;

   pc = chunk_get_head();
   while (pc != nullptr)
   {
      if (pc->type == CT_NEWLINE)
      {
         LOG_FMT(LINDLINE, "%s(%d): orig_line is %zu, NEWLINE\n",
                 __func__, __LINE__, pc->orig_line);
      }
      else if (pc->type == CT_NL_CONT)
      {
         LOG_FMT(LINDLINE, "%s(%d): orig_line is %zu, CT_NL_CONT\n",
                 __func__, __LINE__, pc->orig_line);
      }
      else
      {
         LOG_FMT(LINDLINE, "%s(%d): orig_line is %zu, orig_col is %zu, column is %zu, for '%s'\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->column, pc->text());
         log_pcf_flags(LINDLINE, pc->flags);
      }
      if (  (cpd.settings[UO_use_options_overriding_for_qt_macros].b)
         && (  (strcmp(pc->text(), "SIGNAL") == 0)
            || (strcmp(pc->text(), "SLOT") == 0)))
      {  // guy 2015-09-22
         LOG_FMT(LINDLINE, "%s(%d): orig_line=%zu: type %s SIGNAL/SLOT found\n",
                 __func__, __LINE__, pc->orig_line, get_token_name(pc->type));
      }
      // Handle preprocessor transitions
      in_preproc = (pc->flags & PCF_IN_PREPROC);

      if (cpd.settings[UO_indent_brace_parent].b)
      {
         parent_token_indent = token_indent(pc->parent_type);
      }

      // Handle "force indentation of function definition to start in column 1"
      if (cpd.settings[UO_indent_func_def_force_col1].b)
      {
         if (!in_func_def)
         {
            next = chunk_get_next_ncnl(pc);
            if (  pc->parent_type == CT_FUNC_DEF
               || (  pc->type == CT_COMMENT
                  && next != nullptr
                  && next->parent_type == CT_FUNC_DEF))
            {
               in_func_def = true;
               indent_pse_push(frm, pc);
               frm.pse[frm.pse_tos].indent_tmp = 1;
               frm.pse[frm.pse_tos].indent     = 1;
               frm.pse[frm.pse_tos].indent_tab = 1;
            }
         }
         else
         {
            prev = chunk_get_prev(pc);
            if (  prev->type == CT_BRACE_CLOSE
               && prev->parent_type == CT_FUNC_DEF)
            {
               in_func_def = false;
               indent_pse_pop(frm, pc);
            }
         }
      }

      // Clean up after a #define, etc
      if (!in_preproc)
      {
         while (frm.pse_tos > 0 && frm.pse[frm.pse_tos].in_preproc)
         {
            c_token_t type = frm.pse[frm.pse_tos].type;
            indent_pse_pop(frm, pc);

            /*
             * If we just removed an #endregion, then check to see if a
             * PP_REGION_INDENT entry is right below it
             */
            if (  type == CT_PP_ENDREGION
               && (frm.pse[frm.pse_tos].type == CT_PP_REGION_INDENT))
            {
               indent_pse_pop(frm, pc);
            }
         }
      }
      else if (pc->type == CT_PREPROC)
      {
         // Close out PP_IF_INDENT before playing with the parse frames
         if (  (frm.pse[frm.pse_tos].type == CT_PP_IF_INDENT)
            && (  pc->parent_type == CT_PP_ENDIF
               || pc->parent_type == CT_PP_ELSE))
         {
            indent_pse_pop(frm, pc);
         }

         pf_check(&frm, pc);

         // Indent the body of a #region here
         if (  cpd.settings[UO_pp_region_indent_code].b
            && pc->parent_type == CT_PP_REGION)
         {
            next = chunk_get_next(pc);
            if (next == nullptr)
            {
               break;
            }
            // Hack to get the logs to look right
            set_chunk_type(next, CT_PP_REGION_INDENT);
            indent_pse_push(frm, next);
            set_chunk_type(next, CT_PP_REGION);

            // Indent one level
            frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent + indent_size;
            log_indent();
            frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos - 1].indent_tab + indent_size;
            frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
            frm.pse[frm.pse_tos].in_preproc = false;
            log_indent_tmp();
         }

         // If option set, remove indent inside switch statement
         if (  frm.pse[frm.pse_tos].type == CT_CASE
            && !cpd.settings[UO_indent_switch_pp].b)
         {
            indent_pse_push(frm, pc);

            frm.pse[frm.pse_tos - 1].indent = frm.pse[frm.pse_tos].indent - indent_size;
            log_indent();
         }

         // Indent the body of a #if here
         if (  cpd.settings[UO_pp_if_indent_code].b
            && (  pc->parent_type == CT_PP_IF
               || pc->parent_type == CT_PP_ELSE))
         {
            next = chunk_get_next(pc);
            if (next == nullptr)
            {
               break;
            }

            int     should_indent_preproc = true;
            chunk_t *preproc_next         = chunk_get_next_nl(pc);
            preproc_next = chunk_get_next_nblank(preproc_next);

            /* Look ahead at what's on the line after the #if */
            while (  (preproc_next != NULL)
                  && (preproc_next->type != CT_NEWLINE))
            {
               if (  (  (  (preproc_next->type == CT_BRACE_OPEN)
                        || (preproc_next->type == CT_BRACE_CLOSE))
                     && !cpd.settings[UO_pp_indent_brace].b)
                  || (  preproc_next->type == CT_FUNC_DEF
                     && !cpd.settings[UO_pp_indent_func_def].b)
                  || (  preproc_next->type == CT_CASE
                     && !cpd.settings[UO_pp_indent_case].b)
                  || (  preproc_next->type == CT_EXTERN
                     && !cpd.settings[UO_pp_indent_extern].b))
               {
                  should_indent_preproc = false;
                  break;
               }
               preproc_next = chunk_get_next(preproc_next);
            }
            if (should_indent_preproc)
            {
               // Hack to get the logs to look right
               memtype = next->type;
               set_chunk_type(next, CT_PP_IF_INDENT);
               indent_pse_push(frm, next);
               set_chunk_type(next, memtype);

               // Indent one level except if the #if is a #include guard
               size_t extra = (  pc->pp_level == 0
                              && ifdef_over_whole_file()) ? 0 : indent_size;
               frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent + extra;
               log_indent();
               frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos - 1].indent_tab + extra;
               frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
               frm.pse[frm.pse_tos].in_preproc = false;
               log_indent_tmp();
            }
         }

         // Transition into a preproc by creating a dummy indent
         frm.level++;
         indent_pse_push(frm, chunk_get_next(pc));

         if (  pc->parent_type == CT_PP_DEFINE
            || pc->parent_type == CT_PP_UNDEF)
         {
            frm.pse[frm.pse_tos].indent_tmp = cpd.settings[UO_pp_define_at_level].b ?
                                              frm.pse[frm.pse_tos - 1].indent_tmp : 1;
            frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos].indent_tmp + indent_size;
            log_indent();
            frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos].indent;
            log_indent_tmp();
         }
         else if (  pc->parent_type == CT_PP_PRAGMA
                 && cpd.settings[UO_pp_define_at_level].b)
         {
            frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos - 1].indent_tmp;
            frm.pse[frm.pse_tos].indent     = frm.pse[frm.pse_tos].indent_tmp + indent_size;
            log_indent();
            frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos].indent;
            log_indent_tmp();
         }
         else
         {
            if (  (frm.pse[frm.pse_tos - 1].type == CT_PP_REGION_INDENT)
               || (  (frm.pse[frm.pse_tos - 1].type == CT_PP_IF_INDENT)
                  && (frm.pse[frm.pse_tos].type != CT_PP_ENDIF)))
            {
               frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 2].indent;
               log_indent();
            }
            else
            {
               frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent;
               log_indent();
            }
            log_indent();


            auto val = 0;
            if (  pc->parent_type == CT_PP_REGION
               || pc->parent_type == CT_PP_ENDREGION)
            {
               val = cpd.settings[UO_pp_indent_region].n;
               log_indent();
            }
            else if (  pc->parent_type == CT_PP_IF
                    || pc->parent_type == CT_PP_ELSE
                    || pc->parent_type == CT_PP_ENDIF)
            {
               val = cpd.settings[UO_pp_indent_if].n;
               log_indent();
            }
            if (val != 0)
            {
               auto &indent = frm.pse[frm.pse_tos].indent;

               indent = (val > 0) ? val                     // reassign if positive val,
                        : (cast_abs(indent, val) < indent)  // else if no underflow
                        ? (indent + val) : 0;               // reduce, else 0
            }

            frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
            log_indent_tmp();
         }
      }

      // Check for close XML tags "</..."
      if (cpd.settings[UO_indent_xml_string].u > 0)
      {
         if (pc->type == CT_STRING)
         {
            if (  (pc->len() > 4)
               && xml_indent > 0
               && (pc->str[1] == '<')
               && (pc->str[2] == '/'))
            {
               xml_indent -= cpd.settings[UO_indent_xml_string].u;
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

      // Handle non-brace closures
      log_indent_tmp();

      token_used = false;
      size_t old_pse_tos;
      do
      {
         old_pse_tos = frm.pse_tos;

         // End anything that drops a level
         if (  !chunk_is_newline(pc)
            && !chunk_is_comment(pc)
            && (frm.pse[frm.pse_tos].level > pc->level))
         {
            indent_pse_pop(frm, pc);
         }

         if (frm.pse[frm.pse_tos].level >= pc->level)
         {
            // process virtual braces closes (no text output)
            if (  pc->type == CT_VBRACE_CLOSE
               && (frm.pse[frm.pse_tos].type == CT_VBRACE_OPEN))
            {
               indent_pse_pop(frm, pc);
               frm.level--;
               pc = chunk_get_next(pc);
               if (!pc)
               {
                  // need to break out of both the do and while loops
                  goto null_pc;
               }
            }

            // End any assign operations with a semicolon on the same level
            if (  (  frm.pse[frm.pse_tos].type == CT_ASSIGN_NL
                  || frm.pse[frm.pse_tos].type == CT_ASSIGN)
               && (  chunk_is_semicolon(pc)
                  || pc->type == CT_COMMA
                  || pc->type == CT_BRACE_OPEN
                  || pc->type == CT_SPAREN_CLOSE
                  || (  pc->type == CT_SQUARE_OPEN
                     && pc->parent_type == CT_OC_AT)
                  || (  pc->type == CT_SQUARE_OPEN
                     && pc->parent_type == CT_ASSIGN))
               && pc->parent_type != CT_CPP_LAMBDA)
            {
               indent_pse_pop(frm, pc);
            }

            // End any assign operations with a semicolon on the same level
            if (  chunk_is_semicolon(pc)
               && (  (frm.pse[frm.pse_tos].type == CT_IMPORT)
                  || (frm.pse[frm.pse_tos].type == CT_USING)))
            {
               indent_pse_pop(frm, pc);
            }

            // End any custom macro-based open/closes
            if (  !token_used
               && (frm.pse[frm.pse_tos].type == CT_MACRO_OPEN)
               && pc->type == CT_MACRO_CLOSE)
            {
               token_used = true;
               indent_pse_pop(frm, pc);
            }

            // End any CPP/ObjC class colon stuff
            if (  (  (frm.pse[frm.pse_tos].type == CT_CLASS_COLON)
                  || (frm.pse[frm.pse_tos].type == CT_CONSTR_COLON))
               && (  pc->type == CT_BRACE_OPEN
                  || pc->type == CT_OC_END
                  || pc->type == CT_OC_SCOPE
                  || pc->type == CT_OC_PROPERTY
                  || chunk_is_semicolon(pc)))
            {
               indent_pse_pop(frm, pc);
            }
            // End ObjC class colon stuff inside of generic definition (like Test<T1: id<T3>>)
            if (  (frm.pse[frm.pse_tos].type == CT_CLASS_COLON)
               && pc->type == CT_ANGLE_CLOSE
               && pc->parent_type == CT_OC_GENERIC_SPEC)
            {
               indent_pse_pop(frm, pc);
            }

            // a case is ended with another case or a close brace
            if (  (frm.pse[frm.pse_tos].type == CT_CASE)
               && (pc->type == CT_BRACE_CLOSE || pc->type == CT_CASE))
            {
               indent_pse_pop(frm, pc);
            }

            // a class scope is ended with another class scope or a close brace
            if (  cpd.settings[UO_indent_access_spec_body].b
               && (frm.pse[frm.pse_tos].type == CT_PRIVATE)
               && (pc->type == CT_BRACE_CLOSE || pc->type == CT_PRIVATE))
            {
               indent_pse_pop(frm, pc);
            }

            // return & throw are ended with a semicolon
            if (  chunk_is_semicolon(pc)
               && (  (frm.pse[frm.pse_tos].type == CT_RETURN)
                  || (frm.pse[frm.pse_tos].type == CT_THROW)))
            {
               indent_pse_pop(frm, pc);
            }

            // an OC SCOPE ('-' or '+') ends with a semicolon or brace open
            if (  (frm.pse[frm.pse_tos].type == CT_OC_SCOPE)
               && (chunk_is_semicolon(pc) || pc->type == CT_BRACE_OPEN))
            {
               indent_pse_pop(frm, pc);
            }

            /*
             * a typedef and an OC SCOPE ('-' or '+') ends with a semicolon or
             * brace open
             */
            if (  (frm.pse[frm.pse_tos].type == CT_TYPEDEF)
               && (  chunk_is_semicolon(pc)
                  || chunk_is_paren_open(pc)
                  || pc->type == CT_BRACE_OPEN))
            {
               indent_pse_pop(frm, pc);
            }

            // an SQL EXEC is ended with a semicolon
            if (  (frm.pse[frm.pse_tos].type == CT_SQL_EXEC)
               && chunk_is_semicolon(pc))
            {
               indent_pse_pop(frm, pc);
            }

            // an CLASS is ended with a semicolon or brace open
            if (  (frm.pse[frm.pse_tos].type == CT_CLASS)
               && (  pc->type == CT_CLASS_COLON
                  || pc->type == CT_BRACE_OPEN
                  || chunk_is_semicolon(pc)))
            {
               indent_pse_pop(frm, pc);
            }

            // Close out parenthesis and squares
            if (  (frm.pse[frm.pse_tos].type == (pc->type - 1))
               && (  pc->type == CT_PAREN_CLOSE
                  || pc->type == CT_SPAREN_CLOSE
                  || pc->type == CT_FPAREN_CLOSE
                  || pc->type == CT_SQUARE_CLOSE
                  || pc->type == CT_ANGLE_CLOSE))
            {
               indent_pse_pop(frm, pc);
               frm.paren_count--;
            }
         }
      } while (old_pse_tos > frm.pse_tos);

      // Grab a copy of the current indent
      indent_column_set(frm.pse[frm.pse_tos].indent_tmp);
      log_indent_tmp();

      if (  !chunk_is_newline(pc)
         && !chunk_is_comment(pc)
         && log_sev_on(LINDPC))
      {
         LOG_FMT(LINDPC, " -=[ %zu:%zu %s ]=-\n",
                 pc->orig_line, pc->orig_col, pc->text());
         for (int ttidx = frm.pse_tos; ttidx > 0; ttidx--)
         {
            LOG_FMT(LINDPC, "     [%d %zu:%zu %s %s/%s tmp=%zu ind=%zu bri=%d tab=%zu cont=%d lvl=%zu blvl=%zu]\n",
                    ttidx,
                    frm.pse[ttidx].pc->orig_line,
                    frm.pse[ttidx].pc->orig_col,
                    frm.pse[ttidx].pc->text(),
                    get_token_name(frm.pse[ttidx].type),
                    get_token_name(frm.pse[ttidx].pc->parent_type),
                    frm.pse[ttidx].indent_tmp,
                    frm.pse[ttidx].indent,
                    frm.pse[ttidx].brace_indent,
                    frm.pse[ttidx].indent_tab,
                    frm.pse[ttidx].indent_cont,
                    frm.pse[ttidx].level,
                    frm.pse[ttidx].pc->brace_level);
         }
      }

      /*
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
      if (pc->type == CT_BRACE_CLOSE || pc->type == CT_BRACE_OPEN)
      {
         brace_indent = (  cpd.settings[UO_indent_braces].b
                        && (  !cpd.settings[UO_indent_braces_no_func].b
                           || pc->parent_type != CT_FUNC_DEF)
                        && (  !cpd.settings[UO_indent_braces_no_func].b
                           || pc->parent_type != CT_FUNC_CLASS_DEF)
                        && (  !cpd.settings[UO_indent_braces_no_class].b
                           || pc->parent_type != CT_CLASS)
                        && (  !cpd.settings[UO_indent_braces_no_struct].b
                           || pc->parent_type != CT_STRUCT));
      }

      if (pc->type == CT_BRACE_CLOSE)
      {
         if (frm.pse[frm.pse_tos].type == CT_BRACE_OPEN)
         {
            // Indent the brace to match the open brace
            indent_column_set(frm.pse[frm.pse_tos].brace_indent);

            if (frm.pse[frm.pse_tos].ip.ref)
            {
               pc->indent.ref   = frm.pse[frm.pse_tos].ip.ref;
               pc->indent.delta = 0;
            }

            indent_pse_pop(frm, pc);
            frm.level--;
         }
      }
      else if (pc->type == CT_VBRACE_OPEN)
      {
         frm.level++;
         indent_pse_push(frm, pc);

         size_t iMinIndent = cpd.settings[UO_indent_min_vbrace_open].u;
         if (indent_size > iMinIndent)
         {
            iMinIndent = indent_size;
         }
         size_t iNewIndent = frm.pse[frm.pse_tos - 1].indent + iMinIndent;
         if (cpd.settings[UO_indent_vbrace_open_on_tabstop].b)
         {
            iNewIndent = next_tab_column(iNewIndent);
         }
         frm.pse[frm.pse_tos].indent = iNewIndent;
         log_indent();
         frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
         frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos].indent;
         log_indent_tmp();

         // Always indent on virtual braces
         indent_column_set(frm.pse[frm.pse_tos].indent_tmp);
      }
      else if (  pc->type == CT_BRACE_OPEN
              && (pc->next != nullptr && pc->next->type != CT_NAMESPACE))
      {
         frm.level++;
         indent_pse_push(frm, pc);

         if (  cpd.settings[UO_indent_cpp_lambda_body].b
            && pc->parent_type == CT_CPP_LAMBDA)
         {
            frm.pse[frm.pse_tos].brace_indent = frm.pse[frm.pse_tos - 1].indent;
            indent_column_set(frm.pse[frm.pse_tos].brace_indent);
            frm.pse[frm.pse_tos].indent = indent_column + indent_size;
            log_indent();
            frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos].indent;
            frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
            log_indent_tmp();

            frm.pse[frm.pse_tos - 1].indent_tmp = frm.pse[frm.pse_tos].indent_tmp;
            log_indent_tmp();
         }
         else if (  (cpd.lang_flags & LANG_CS)
                 && cpd.settings[UO_indent_cs_delegate_brace].b
                 && (  pc->parent_type == CT_LAMBDA
                    || pc->parent_type == CT_DELEGATE))
         {
            frm.pse[frm.pse_tos].brace_indent = 1 + ((pc->brace_level + 1) * indent_size);
            indent_column_set(frm.pse[frm.pse_tos].brace_indent);
            frm.pse[frm.pse_tos].indent = indent_column + indent_size;
            log_indent();
            frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos].indent;
            frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
            log_indent_tmp();

            frm.pse[frm.pse_tos - 1].indent_tmp = frm.pse[frm.pse_tos].indent_tmp;
            log_indent_tmp();
         }
         else if (  !cpd.settings[UO_indent_paren_open_brace].b
                 && ((cpd.lang_flags & LANG_CS) == 0)
                 && pc->parent_type == CT_CPP_LAMBDA
                 && pc->flags & PCF_IN_FCN_DEF
                 && chunk_is_newline(chunk_get_next_nc(pc)))
         {
            // Issue #1165
            log_pcf_flags(LINDENT2, pc->flags);
            frm.pse[frm.pse_tos].brace_indent = 1 + ((pc->brace_level + 1) * indent_size);
            indent_column_set(frm.pse[frm.pse_tos].brace_indent);
            frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent_tmp;
            log_indent();
            frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
            log_indent_tmp();
         }
         // any '{' that is inside of a '(' overrides the '(' indent
         else if (  !cpd.settings[UO_indent_paren_open_brace].b
                 && chunk_is_paren_open(frm.pse[frm.pse_tos - 1].pc)
                 && chunk_is_newline(chunk_get_next_nc(pc)))
         {
            // FIXME: I don't know how much of this is necessary, but it seems to work
            LOG_FMT(LINDENT2, "%s(%d): orig_line is %zu, pc->brace_level is %zu, for '%s', pc->level is %zu, pc(-1)->level is %zu\n",
                    __func__, __LINE__, pc->orig_line, pc->brace_level, pc->text(), pc->level, frm.pse[frm.pse_tos - 1].pc->level);
            frm.pse[frm.pse_tos].brace_indent = 1 + (pc->brace_level * indent_size);
            indent_column_set(frm.pse[frm.pse_tos].brace_indent);
            frm.pse[frm.pse_tos].indent = indent_column + indent_size;
            log_indent();
            frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos].indent;
            frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
            log_indent_tmp();

            frm.pse[frm.pse_tos - 1].indent_tmp = frm.pse[frm.pse_tos].indent_tmp;
            log_indent_tmp();
         }
         else if (frm.paren_count != 0)
         {
            if (frm.pse[frm.pse_tos].pc->parent_type == CT_OC_BLOCK_EXPR)
            {
               if (  (pc->flags & PCF_IN_OC_MSG)
                  && cpd.settings[UO_indent_oc_block_msg].u)
               {
                  frm.pse[frm.pse_tos].ip.ref   = oc_msg_block_indent(pc, false, false, false, true);
                  frm.pse[frm.pse_tos].ip.delta = cpd.settings[UO_indent_oc_block_msg].u;
               }

               if (  cpd.settings[UO_indent_oc_block].b
                  || cpd.settings[UO_indent_oc_block_msg_xcode_style].b)
               {
                  bool in_oc_msg           = (pc->flags & PCF_IN_OC_MSG) != 0;     // forcing value to bool
                  bool indent_from_keyword = cpd.settings[UO_indent_oc_block_msg_from_keyword].b
                                             && in_oc_msg;
                  bool indent_from_colon = cpd.settings[UO_indent_oc_block_msg_from_colon].b
                                           && in_oc_msg;
                  bool indent_from_caret = cpd.settings[UO_indent_oc_block_msg_from_caret].b
                                           && in_oc_msg;
                  bool indent_from_brace = cpd.settings[UO_indent_oc_block_msg_from_brace].b
                                           && in_oc_msg;

                  /*
                   * In "Xcode indent mode", we want to indent:
                   *  - if the colon is aligned (namely, if a newline has been
                   *    added before it), indent_from_brace
                   *  - otherwise, indent from previous block (the "else" statement here)
                   */
                  if (cpd.settings[UO_indent_oc_block_msg_xcode_style].b)
                  {
                     chunk_t *colon        = oc_msg_prev_colon(pc);
                     chunk_t *param_name   = chunk_get_prev(colon);
                     chunk_t *before_param = chunk_get_prev(param_name);

                     if (before_param && before_param->type == CT_NEWLINE)
                     {
                        indent_from_keyword = true;
                        indent_from_colon   = false;
                        indent_from_caret   = false;
                        indent_from_brace   = false;
                     }
                     else
                     {
                        indent_from_brace   = false;
                        indent_from_colon   = false;
                        indent_from_caret   = false;
                        indent_from_keyword = false;
                     }
                  }

                  chunk_t *ref = oc_msg_block_indent(pc, indent_from_brace,
                                                     indent_from_caret,
                                                     indent_from_colon,
                                                     indent_from_keyword);
                  if (ref)
                  {
                     frm.pse[frm.pse_tos].indent = indent_size + ref->column;
                     log_indent();
                     indent_column_set(frm.pse[frm.pse_tos].indent - indent_size);
                  }
                  else
                  {
                     frm.pse[frm.pse_tos].indent = 1 + ((pc->brace_level + 1) * indent_size);
                     log_indent();
                     indent_column_set(frm.pse[frm.pse_tos].indent - indent_size);
                  }
               }
               else
               {
                  frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent_tmp + indent_size;
                  log_indent();
               }
            }
            else
            {
               // We are inside ({ ... }) -- indent one tab from the paren
               frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent_tmp + indent_size;
               log_indent();
            }
         }
         else
         {
            // Use the prev indent level + indent_size.
            frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent + indent_size;
            LOG_FMT(LINDLINE, "%s(%d): frm.pse_tos=%zu, ... indent=%zu\n",
                    __func__, __LINE__, frm.pse_tos, frm.pse[frm.pse_tos].indent);

            // If this brace is part of a statement, bump it out by indent_brace
            if (  pc->parent_type == CT_IF
               || pc->parent_type == CT_ELSE
               || pc->parent_type == CT_ELSEIF
               || pc->parent_type == CT_TRY
               || pc->parent_type == CT_CATCH
               || pc->parent_type == CT_DO
               || pc->parent_type == CT_WHILE
               || pc->parent_type == CT_USING_STMT
               || pc->parent_type == CT_SWITCH
               || pc->parent_type == CT_SYNCHRONIZED
               || pc->parent_type == CT_FOR)
            {
               if (parent_token_indent != 0)
               {
                  frm.pse[frm.pse_tos].indent += parent_token_indent - indent_size;
                  log_indent();
               }
               else
               {
                  frm.pse[frm.pse_tos].indent += cpd.settings[UO_indent_brace].u;
                  log_indent();
                  indent_column_set(indent_column + cpd.settings[UO_indent_brace].u);
               }
            }
            else if (pc->parent_type == CT_CASE)
            {
               const auto tmp_indent = static_cast<int>(frm.pse[frm.pse_tos - 1].indent)
                                       - static_cast<int>(indent_size)
                                       + cpd.settings[UO_indent_case_brace].n;

               /*
                * An open brace with the parent of case does not indent by default
                * UO_indent_case_brace can be used to indent the brace.
                * So we need to take the CASE indent, subtract off the
                * indent_size that was added above and then add indent_case_brace.
                * may take negative value
                */
               indent_column_set(max(tmp_indent, 0));

               // Stuff inside the brace still needs to be indented
               frm.pse[frm.pse_tos].indent = indent_column + indent_size;
               log_indent();
               frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
               log_indent_tmp();
            }
            else if (  pc->parent_type == CT_CLASS
                    && !cpd.settings[UO_indent_class].b)
            {
               LOG_FMT(LINDENT, "%s(%d):orig_line is %zu, orig_col is %zu, text is %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
               frm.pse[frm.pse_tos].indent -= indent_size;
               log_indent();
            }
            else if (pc->parent_type == CT_NAMESPACE)
            {
               frm.pse[frm.pse_tos].ns_cnt = frm.pse[frm.pse_tos - 1].ns_cnt + 1;
               if (  cpd.settings[UO_indent_namespace].b
                  && cpd.settings[UO_indent_namespace_single_indent].b)
               {
                  if (frm.pse[frm.pse_tos].ns_cnt >= 2)
                  {
                     // undo indent on all except the first namespace
                     frm.pse[frm.pse_tos].indent -= indent_size;
                     log_indent();
                  }
                  indent_column_set(frm.pse[frm.pse_tos - frm.pse[frm.pse_tos].ns_cnt].indent);
               }
               else if (  (pc->flags & PCF_LONG_BLOCK)
                       || !cpd.settings[UO_indent_namespace].b)
               {
                  // don't indent long blocks
                  frm.pse[frm.pse_tos].indent -= indent_size;
                  log_indent();
               }
               else // indenting 'short' namespace
               {
                  if (cpd.settings[UO_indent_namespace_level].u > 0)
                  {
                     frm.pse[frm.pse_tos].indent -= indent_size;
                     log_indent();
                     frm.pse[frm.pse_tos].indent +=
                        cpd.settings[UO_indent_namespace_level].u;
                     log_indent();
                  }
               }
            }
            else if (  pc->parent_type == CT_EXTERN
                    && !cpd.settings[UO_indent_extern].b)
            {
               frm.pse[frm.pse_tos].indent -= indent_size;
               log_indent();
            }

            frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos].indent;
         }

         if (pc->flags & PCF_DONT_INDENT)
         {
            frm.pse[frm.pse_tos].indent = pc->column;
            log_indent();
            indent_column_set(pc->column);
         }
         else
         {
            /*
             * If there isn't a newline between the open brace and the next
             * item, just indent to wherever the next token is.
             * This covers this sort of stuff:
             * { a++;
             *   b--; };
             */
            next = chunk_get_next_ncnl(pc);
            if (next == nullptr)
            {
               break;
            }
            if (!chunk_is_newline_between(pc, next))
            {
               if (cpd.settings[UO_indent_token_after_brace].b)
               {
                  // Issue #1108
                  if (!(pc->flags & PCF_ONE_LINER))
                  {
                     frm.pse[frm.pse_tos].indent = next->column;
                     log_indent();
                  }
               }
            }
            frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
            frm.pse[frm.pse_tos].open_line  = pc->orig_line;
            log_indent_tmp();

            // Update the indent_column if needed
            if (brace_indent || parent_token_indent != 0)
            {
               indent_column_set(frm.pse[frm.pse_tos].indent_tmp);
               log_indent_tmp();
            }
         }

         // Save the brace indent
         frm.pse[frm.pse_tos].brace_indent = indent_column;
      }
      else if (pc->type == CT_SQL_END)
      {
         if (frm.pse[frm.pse_tos].type == CT_SQL_BEGIN)
         {
            indent_pse_pop(frm, pc);
            frm.level--;
            indent_column_set(frm.pse[frm.pse_tos].indent_tmp);
            log_indent_tmp();
         }
      }
      else if (pc->type == CT_SQL_BEGIN)
      {
         frm.level++;
         indent_pse_push(frm, pc);
         frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent + indent_size;
         log_indent();
         frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
         frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos].indent;
         log_indent_tmp();
      }
      else if (pc->type == CT_SQL_EXEC)
      {
         frm.level++;
         indent_pse_push(frm, pc);
         frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent + indent_size;
         log_indent();
         frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
         log_indent_tmp();
      }
      else if (pc->type == CT_MACRO_OPEN)
      {
         frm.level++;
         indent_pse_push(frm, pc);
         frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent + indent_size;
         log_indent();
         frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
         frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos].indent;
         log_indent_tmp();
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
         // Start a case - indent UO_indent_switch_case from the switch level
         size_t tmp = frm.pse[frm.pse_tos].indent + cpd.settings[UO_indent_switch_case].u;

         indent_pse_push(frm, pc);

         frm.pse[frm.pse_tos].indent = tmp;
         log_indent();
         frm.pse[frm.pse_tos].indent_tmp = tmp - indent_size + cpd.settings[UO_indent_case_shift].u;
         frm.pse[frm.pse_tos].indent_tab = tmp;
         log_indent_tmp();

         // Always set on case statements
         indent_column_set(frm.pse[frm.pse_tos].indent_tmp);

         // comments before 'case' need to be aligned with the 'case'
         chunk_t *pct = pc;
         while (  ((pct = chunk_get_prev_nnl(pct)) != nullptr)
               && chunk_is_comment(pct))
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
         if (  prev != nullptr
            && prev->type == CT_BRACE_CLOSE
            && prev->parent_type == CT_CASE)
         {
            // issue #663
            chunk_t *temp = chunk_get_prev_type(pc, CT_BRACE_OPEN, pc->level);
            // This only affects the 'break', so no need for a stack entry
            indent_column_set(temp->column);
         }
      }
      else if (pc->type == CT_LABEL)
      {
         const auto val        = cpd.settings[UO_indent_label].n;
         const auto pse_indent = frm.pse[frm.pse_tos].indent;

         // Labels get sent to the left or backed up
         if (val > 0)
         {
            indent_column_set(val);

            next = chunk_get_next(pc);   // colon
            next = chunk_get_next(next); // possible statement

            if (  next != nullptr && !chunk_is_newline(next)
                  // label (+ 2, because there is colon and space after it) must fit into indent
               && (val + static_cast<int>(pc->len()) + 2 <= static_cast<int>(pse_indent)))
            {
               reindent_line(next, pse_indent);
            }
         }
         else
         {
            const auto no_underflow = cast_abs(pse_indent, val) < pse_indent;
            indent_column_set(((no_underflow) ? (pse_indent + val) : 0));
         }
      }
      else if (pc->type == CT_PRIVATE)
      {
         if (cpd.settings[UO_indent_access_spec_body].b)
         {
            size_t tmp = frm.pse[frm.pse_tos].indent + indent_size;

            indent_pse_push(frm, pc);

            frm.pse[frm.pse_tos].indent = tmp;
            log_indent();
            frm.pse[frm.pse_tos].indent_tmp = tmp - indent_size;
            frm.pse[frm.pse_tos].indent_tab = tmp;
            log_indent_tmp();

            /*
             * If we are indenting the body, then we must leave the access spec
             * indented at brace level
             */
            indent_column_set(frm.pse[frm.pse_tos].indent_tmp);
         }
         else
         {
            // Access spec labels get sent to the left or backed up
            const auto val = cpd.settings[UO_indent_access_spec].n;
            if (val > 0)
            {
               indent_column_set(val);
            }
            else
            {
               const auto pse_indent   = frm.pse[frm.pse_tos].indent;
               const auto no_underflow = cast_abs(pse_indent, val) < pse_indent;

               indent_column_set(no_underflow ? (pse_indent + val) : 0);
            }
         }
      }
      else if (pc->type == CT_CLASS)
      {
         frm.level++;
         indent_pse_push(frm, pc);
         frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent + indent_size;
         log_indent();
         frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
         frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos].indent;
         log_indent_tmp();
      }
      else if (pc->type == CT_CLASS_COLON || pc->type == CT_CONSTR_COLON)
      {
         // just indent one level
         indent_pse_push(frm, pc);
         frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent_tmp + indent_size;
         log_indent();
         frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
         frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos].indent;
         log_indent_tmp();
         indent_column_set(frm.pse[frm.pse_tos].indent_tmp);

         if (  cpd.settings[UO_indent_class_colon].b
            && pc->type == CT_CLASS_COLON)
         {
            if (cpd.settings[UO_indent_class_on_colon].b)
            {
               frm.pse[frm.pse_tos].indent = pc->column;
               log_indent();
            }
            else
            {
               next = chunk_get_next(pc);
               if (next != nullptr && !chunk_is_newline(next))
               {
                  frm.pse[frm.pse_tos].indent = next->column;
                  log_indent();
               }
            }
         }
         else if (  cpd.settings[UO_indent_constr_colon].b
                 && pc->type == CT_CONSTR_COLON)
         {
            prev = chunk_get_prev(pc);
            if (chunk_is_newline(prev))
            {
               frm.pse[frm.pse_tos].indent += cpd.settings[UO_indent_ctor_init_leading].u;
               log_indent();
            }

            // TODO: Create a dedicated indent_constr_on_colon?
            if (cpd.settings[UO_indent_class_on_colon].b)
            {
               frm.pse[frm.pse_tos].indent = pc->column;
               log_indent();
            }
            else if (cpd.settings[UO_indent_ctor_init].n != 0)
            {
               /*
                * If the std::max() calls were specialized with size_t (the type of the underlying variable),
                * they would never actually do their job, because size_t is unsigned and therefore even
                * a "negative" result would be always greater than zero.
                * Using ptrdiff_t (a standard signed type of the same size as size_t) in order to avoid that.
                */
               frm.pse[frm.pse_tos].indent = std::max<ptrdiff_t>(frm.pse[frm.pse_tos].indent + cpd.settings[UO_indent_ctor_init].n, 0);
               log_indent();
               frm.pse[frm.pse_tos].indent_tmp = std::max<ptrdiff_t>(frm.pse[frm.pse_tos].indent_tmp + cpd.settings[UO_indent_ctor_init].n, 0);
               frm.pse[frm.pse_tos].indent_tab = std::max<ptrdiff_t>(frm.pse[frm.pse_tos].indent_tab + cpd.settings[UO_indent_ctor_init].n, 0);
               log_indent_tmp();
               indent_column_set(frm.pse[frm.pse_tos].indent_tmp);
            }
            else
            {
               next = chunk_get_next(pc);
               if (next != nullptr && !chunk_is_newline(next))
               {
                  frm.pse[frm.pse_tos].indent = next->column;
                  log_indent();
               }
            }
         }
      }
      else if (  pc->type == CT_PAREN_OPEN
              || pc->type == CT_SPAREN_OPEN
              || pc->type == CT_FPAREN_OPEN
              || pc->type == CT_SQUARE_OPEN
              || pc->type == CT_ANGLE_OPEN)
      {
         /*
          * Open parenthesis and squares - never update indent_column,
          * unless right after a newline.
          */
         bool skipped = false;

         indent_pse_push(frm, pc);
         if (  chunk_is_newline(chunk_get_prev(pc))
            && pc->column != indent_column)
         {
            LOG_FMT(LINDENT, "%s[line %d]: %zu] indent => %zu [%s]\n",
                    __func__, __LINE__, pc->orig_line, indent_column, pc->text());
            reindent_line(pc, indent_column);
         }
         frm.pse[frm.pse_tos].indent = pc->column + pc->len();
         log_indent();

         if (pc->type == CT_SQUARE_OPEN && (cpd.lang_flags & LANG_D))
         {
            frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos].indent;
         }

         if (  (pc->type == CT_FPAREN_OPEN || pc->type == CT_ANGLE_OPEN)
            && (  (  cpd.settings[UO_indent_func_call_param].b
                  && (  pc->parent_type == CT_FUNC_CALL
                     || pc->parent_type == CT_FUNC_CALL_USER))
               || (  cpd.settings[UO_indent_func_proto_param].b
                  && (  pc->parent_type == CT_FUNC_PROTO
                     || pc->parent_type == CT_FUNC_CLASS_PROTO))
               || (  cpd.settings[UO_indent_func_class_param].b
                  && (  pc->parent_type == CT_FUNC_CLASS_DEF
                     || pc->parent_type == CT_FUNC_CLASS_PROTO))
               || (  cpd.settings[UO_indent_template_param].b
                  && pc->parent_type == CT_TEMPLATE)
               || (  cpd.settings[UO_indent_func_ctor_var_param].b
                  && pc->parent_type == CT_FUNC_CTOR_VAR)
               || (  cpd.settings[UO_indent_func_def_param].b
                  && pc->parent_type == CT_FUNC_DEF)))
         {
            // Skip any continuation indents
            idx = frm.pse_tos - 1;
            while (  idx > 0
                  && frm.pse[idx].type != CT_BRACE_OPEN
                  && frm.pse[idx].type != CT_VBRACE_OPEN
                  && frm.pse[idx].type != CT_PAREN_OPEN
                  && frm.pse[idx].type != CT_FPAREN_OPEN
                  && frm.pse[idx].type != CT_SPAREN_OPEN
                  && frm.pse[idx].type != CT_SQUARE_OPEN
                  && frm.pse[idx].type != CT_ANGLE_OPEN
                  && frm.pse[idx].type != CT_CLASS_COLON
                  && frm.pse[idx].type != CT_CONSTR_COLON
                  && frm.pse[idx].type != CT_ASSIGN_NL)
            {
               idx--;
               skipped = true;
            }
            // PR#381
            if (cpd.settings[UO_indent_param].u != 0)
            {
               frm.pse[frm.pse_tos].indent = frm.pse[idx].indent + cpd.settings[UO_indent_param].u;
               log_indent();
            }
            else
            {
               frm.pse[frm.pse_tos].indent = frm.pse[idx].indent + indent_size;
               log_indent();
            }
            if (cpd.settings[UO_indent_func_param_double].b)
            {
               // double is: Use both values of the options indent_columns and indent_param
               frm.pse[frm.pse_tos].indent += indent_size;
               log_indent();
            }
            frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos].indent;
         }

         else if (  (  chunk_is_str(pc, "(", 1)
                    && !cpd.settings[UO_indent_paren_nl].b)
                 || (  chunk_is_str(pc, "<", 1)
                    && !cpd.settings[UO_indent_paren_nl].b)    // TODO: add indent_angle_nl?
                 || (  chunk_is_str(pc, "[", 1)
                    && !cpd.settings[UO_indent_square_nl].b))
         {
            next = chunk_get_next_nc(pc);
            if (next == nullptr)
            {
               break;
            }
            if (  chunk_is_newline(next)
               && !cpd.settings[UO_indent_paren_after_func_def].b
               && !cpd.settings[UO_indent_paren_after_func_decl].b
               && !cpd.settings[UO_indent_paren_after_func_call].b)
            {
               size_t sub = 1;
               if (  (frm.pse[frm.pse_tos - 1].type == CT_ASSIGN)
                  || (frm.pse[frm.pse_tos - 1].type == CT_RETURN))
               {
                  sub = 2;
               }
               frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - sub].indent + indent_size;
               log_indent();
               frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos].indent;
               skipped                         = true;
            }
            else
            {
               if (next && !chunk_is_comment(next))
               {
                  if (next->type == CT_SPACE)
                  {
                     next = chunk_get_next_nc(next);
                     if (next == nullptr)
                     {
                        break;
                     }
                  }
                  frm.pse[frm.pse_tos].indent = next->column;
                  log_indent();
               }
            }
         }

         if (  (  pc->type == CT_FPAREN_OPEN
               && chunk_is_newline(chunk_get_prev(pc)))
            && (  (  (  pc->parent_type == CT_FUNC_PROTO
                     || pc->parent_type == CT_FUNC_CLASS_PROTO)
                  && cpd.settings[UO_indent_paren_after_func_decl].b)
               || (  pc->parent_type == CT_FUNC_DEF
                  && cpd.settings[UO_indent_paren_after_func_def].b)
               || (  (  pc->parent_type == CT_FUNC_CALL
                     || pc->parent_type == CT_FUNC_CALL_USER)
                  && cpd.settings[UO_indent_paren_after_func_call].b)
               || !chunk_is_newline(chunk_get_next(pc)))
            && (!cpd.settings[UO_use_indent_continue_only_once].b))     // Issue #1160
         {
            frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent + indent_size;
            log_indent();
            indent_column_set(frm.pse[frm.pse_tos].indent);
         }
         if (  pc->parent_type != CT_OC_AT
            && (cpd.settings[UO_indent_continue].n != 0)
            && (!skipped))
         {
            frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent;
            log_indent();
            if (  pc->level == pc->brace_level
               && (  pc->type == CT_FPAREN_OPEN
                  || pc->type == CT_SPAREN_OPEN
                  || pc->type == CT_ANGLE_OPEN))     // Issue #1170
            {
               //frm.pse[frm.pse_tos].indent += abs(cpd.settings[UO_indent_continue].n);
               //   frm.pse[frm.pse_tos].indent      = calc_indent_continue(frm, frm.pse_tos);
               //   frm.pse[frm.pse_tos].indent_cont = true;
               if (  (cpd.settings[UO_use_indent_continue_only_once].b)
                  && (frm.pse[frm.pse_tos].indent_cont)
                  && vardefcol != 0)
               {
                  /*
                   * The value of the indentation for a continuation line is calculate
                   * differently if the line is:
                   *   a declaration :your case with QString fileName ...
                   *   an assignment  :your case with pSettings = new QSettings( ...
                   * At the second case the option value might be used twice:
                   *   at the assignment
                   *   at the function call (if present)
                   * If you want to prevent the double use of the option value
                   * you may use the new option :
                   *   use_indent_continue_only_once
                   * with the value "true".
                   * use/don't use indent_continue once Guy 2016-05-16
                   */

                  // if vardefcol isn't zero, use it
                  frm.pse[frm.pse_tos].indent = vardefcol;
                  log_indent();
               }
               else
               {
                  frm.pse[frm.pse_tos].indent = calc_indent_continue(frm, frm.pse_tos);
                  log_indent();
                  frm.pse[frm.pse_tos].indent_cont = true;
               }
            }
         }
         frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
         log_indent_tmp();
         frm.paren_count++;
      }
      else if (  pc->type == CT_ASSIGN
              || pc->type == CT_IMPORT
              || pc->type == CT_USING)
      {
         /*
          * if there is a newline after the '=' or the line starts with a '=',
          * just indent one level,
          * otherwise align on the '='.
          */
         if (pc->type == CT_ASSIGN && chunk_is_newline(chunk_get_prev(pc)))
         {
            frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent + indent_size;
            log_indent_tmp();
            indent_column_set(frm.pse[frm.pse_tos].indent_tmp);
            LOG_FMT(LINDENT, "%s(%d): %zu] assign => %zu [%s]\n",
                    __func__, __LINE__, pc->orig_line, indent_column, pc->text());
            reindent_line(pc, frm.pse[frm.pse_tos].indent_tmp);
         }

         next = chunk_get_next(pc);
         if (next != nullptr)
         {
            indent_pse_push(frm, pc);
            if (cpd.settings[UO_indent_continue].n != 0)
            {
               frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent;
               log_indent();
               if (  pc->level == pc->brace_level
                  && (  pc->type != CT_ASSIGN
                     || (  pc->parent_type != CT_FUNC_PROTO
                        && pc->parent_type != CT_FUNC_DEF)))
               {
                  //frm.pse[frm.pse_tos].indent += abs(cpd.settings[UO_indent_continue].n);
                  //   frm.pse[frm.pse_tos].indent      = calc_indent_continue(frm, frm.pse_tos);
                  //   frm.pse[frm.pse_tos].indent_cont = true;
                  if (  (cpd.settings[UO_use_indent_continue_only_once].b)
                     && (frm.pse[frm.pse_tos].indent_cont)
                     && vardefcol != 0)
                  {
                     // if vardefcol isn't zero, use it
                     frm.pse[frm.pse_tos].indent = vardefcol;
                     log_indent();
                  }
                  else
                  {
                     frm.pse[frm.pse_tos].indent = calc_indent_continue(frm, frm.pse_tos);
                     log_indent();
                     vardefcol                        = frm.pse[frm.pse_tos].indent; // use the same variable for the next line
                     frm.pse[frm.pse_tos].indent_cont = true;
                  }
               }
            }
            else if (  chunk_is_newline(next)
                    || !cpd.settings[UO_indent_align_assign].b)
            {
               frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent_tmp + indent_size;
               log_indent();
               if (pc->type == CT_ASSIGN)
               {
                  frm.pse[frm.pse_tos].type       = CT_ASSIGN_NL;
                  frm.pse[frm.pse_tos].indent_tab = frm.pse[frm.pse_tos].indent;
               }
            }
            else
            {
               frm.pse[frm.pse_tos].indent = pc->column + pc->len() + 1;
               log_indent();
            }
            frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
            log_indent_tmp();
         }
      }
      else if (  pc->type == CT_RETURN
              || (pc->type == CT_THROW && pc->parent_type == CT_NONE))
      {
         // don't count returns inside a () or []
         if (pc->level == pc->brace_level)
         {
            indent_pse_push(frm, pc);
            if (chunk_is_newline(chunk_get_next(pc)))
            {
               frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent + indent_size;
               log_indent();
            }
            else
            {
               frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent + pc->len() + 1;
               log_indent();
            }
            frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos - 1].indent;
            log_indent_tmp();
         }
      }
      else if (pc->type == CT_OC_SCOPE || pc->type == CT_TYPEDEF)
      {
         indent_pse_push(frm, pc);
         // Issue # 405
         frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent;
         log_indent();
         frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
         LOG_FMT(LINDLINE, "%s(%d): .indent=%zu, .indent_tmp=%zu\n",
                 __func__, __LINE__, frm.pse[frm.pse_tos].indent, frm.pse[frm.pse_tos].indent_tmp);
         if (cpd.settings[UO_indent_continue].n != 0)
         {
            //frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent +
            //                              abs(cpd.settings[UO_indent_continue].n);
            frm.pse[frm.pse_tos].indent = calc_indent_continue(frm, frm.pse_tos - 1);
            log_indent();
            frm.pse[frm.pse_tos].indent_cont = true;
         }
         else
         {
            frm.pse[frm.pse_tos].indent = frm.pse[frm.pse_tos - 1].indent + indent_size;
            log_indent();
         }
      }
      else if (pc->type == CT_C99_MEMBER)
      {
         // nothing to do
      }
      else
      {
         // anything else?
      }

      // Handle shift expression continuation indenting
      shiftcontcol = 0;
      if (  cpd.settings[UO_indent_shift].b
         && !(pc->flags & PCF_IN_ENUM)
         && pc->parent_type != CT_OPERATOR
         && pc->type != CT_COMMENT
         && pc->type != CT_COMMENT_CPP
         && pc->type != CT_COMMENT_MULTI
         && pc->type != CT_BRACE_OPEN
         && pc->level > 0
         && !chunk_is_blank(pc))
      {
         bool in_shift    = false;
         bool is_operator = false;

         // Are we in such an expression? Go both forwards and backwards.
         chunk_t *tmp = pc;
         do
         {
            if (  tmp
               && (chunk_is_str(tmp, "<<", 2) || chunk_is_str(tmp, ">>", 2)))
            {
               in_shift = true;

               tmp = chunk_get_prev_ncnl(tmp);
               if (tmp && tmp->type == CT_OPERATOR)
               {
                  is_operator = true;
               }

               break;
            }
            tmp = chunk_get_prev_ncnl(tmp);
         } while (  !in_shift
                 && tmp
                 && tmp->type != CT_SEMICOLON
                 && tmp->type != CT_BRACE_OPEN
                 && tmp->type != CT_BRACE_CLOSE
                 && tmp->type != CT_COMMA
                 && tmp->type != CT_SPAREN_OPEN
                 && tmp->type != CT_SPAREN_CLOSE);

         tmp = pc;
         do
         {
            tmp = chunk_get_next_ncnl(tmp);
            if (  tmp
               && (chunk_is_str(tmp, "<<", 2) || chunk_is_str(tmp, ">>", 2)))
            {
               in_shift = true;

               tmp = chunk_get_prev_ncnl(tmp);
               if (tmp && tmp->type == CT_OPERATOR)
               {
                  is_operator = true;
               }

               break;
            }
         } while (  !in_shift
                 && tmp
                 && tmp->type != CT_SEMICOLON
                 && tmp->type != CT_BRACE_OPEN
                 && tmp->type != CT_BRACE_CLOSE
                 && tmp->type != CT_COMMA
                 && tmp->type != CT_SPAREN_OPEN
                 && tmp->type != CT_SPAREN_CLOSE);

         chunk_t *prev_nonl = chunk_get_prev_ncnl(pc);
         chunk_t *prev2     = chunk_get_prev_nc(pc);

         if (  prev_nonl
            && (  chunk_is_semicolon(prev_nonl)
               || prev_nonl->type == CT_BRACE_OPEN
               || prev_nonl->type == CT_BRACE_CLOSE
               || prev_nonl->type == CT_VBRACE_CLOSE
               || prev_nonl->type == CT_VBRACE_OPEN
               || prev_nonl->type == CT_CASE_COLON
               || (prev_nonl->flags & PCF_IN_PREPROC) != (pc->flags & PCF_IN_PREPROC)
               || prev_nonl->type == CT_COMMA
               || is_operator))
         {
            in_shift = false;
         }

         if (  prev2
            && prev2->type == CT_NEWLINE
            && in_shift)
         {
            shiftcontcol                     = calc_indent_continue(frm, frm.pse_tos);
            frm.pse[frm.pse_tos].indent_cont = true;

            // Work around the doubly increased indent in RETURNs and assignments
            bool   need_workaround = false;
            size_t sub             = 0;
            for (int i = frm.pse_tos; i >= 0; i--)
            {
               if (frm.pse[i].type == CT_RETURN || frm.pse[i].type == CT_ASSIGN)
               {
                  need_workaround = true;
                  sub             = frm.pse_tos - i + 1;
                  break;
               }
            }

            if (need_workaround)
            {
               shiftcontcol = calc_indent_continue(frm, frm.pse_tos - sub);
            }
         }
      }

      // Handle variable definition continuation indenting
      if (  vardefcol == 0
         && (pc->type == CT_WORD || pc->type == CT_FUNC_CTOR_VAR)
         && ((pc->flags & PCF_IN_FCN_DEF) == 0)
         && ((pc->flags & PCF_VAR_1ST_DEF) == PCF_VAR_1ST_DEF))
      {
         if (cpd.settings[UO_indent_continue].n != 0)
         {
            //vardefcol = frm.pse[frm.pse_tos].indent +
            //            abs(cpd.settings[UO_indent_continue].n);
            vardefcol                        = calc_indent_continue(frm, frm.pse_tos);
            frm.pse[frm.pse_tos].indent_cont = true;
         }
         else if (  cpd.settings[UO_indent_var_def_cont].b
                 || chunk_is_newline(chunk_get_prev(pc)))
         {
            vardefcol = frm.pse[frm.pse_tos].indent + indent_size;
         }
         else
         {
            vardefcol = pc->column;
            // need to skip backward over any '*'
            chunk_t *tmp = chunk_get_prev_nc(pc);
            while (chunk_is_token(tmp, CT_PTR_TYPE))
            {
               vardefcol = tmp->column;
               tmp       = chunk_get_prev_nc(tmp);
            }
         }
      }
      if (  chunk_is_semicolon(pc)
         || (pc->type == CT_BRACE_OPEN && pc->parent_type == CT_FUNCTION))
      {
         vardefcol = 0;
      }

      // Indent the line if needed
      if (  did_newline
         && !chunk_is_newline(pc)
         && (pc->len() != 0))
      {
         pc->column_indent = frm.pse[frm.pse_tos].indent_tab;

         if (frm.pse[frm.pse_tos].ip.ref)
         {
            pc->indent.ref   = frm.pse[frm.pse_tos].ip.ref;
            pc->indent.delta = frm.pse[frm.pse_tos].ip.delta;
         }

         LOG_FMT(LINDENT2, "%s(%d): orig_line is %zu, pc->column_indent is %zu, indent_column is %zu, for '%s'\n",
                 __func__, __LINE__, pc->orig_line, pc->column_indent, indent_column, pc->text());

         /*
          * Check for special continuations.
          * Note that some of these could be done as a stack item like
          * everything else
          */

         prev = chunk_get_prev_ncnl(pc);
         auto prevv = chunk_get_prev_ncnl(prev);
         next = chunk_get_next_ncnl(pc);

         bool do_vardefcol = false;
         if (  vardefcol > 0
            && pc->level == pc->brace_level
            && prev
            && (  prev->type == CT_COMMA
               || prev->type == CT_TYPE
               || prev->type == CT_PTR_TYPE
               || prev->type == CT_WORD))
         {
            chunk_t *tmp = pc;
            while (chunk_is_token(tmp, CT_PTR_TYPE))
            {
               tmp = chunk_get_next_ncnl(tmp);
            }
            if (  tmp
               && (tmp->flags & PCF_VAR_DEF)
               && (tmp->type == CT_WORD || tmp->type == CT_FUNC_CTOR_VAR))
            {
               do_vardefcol = true;
            }
         }

         if (pc->flags & PCF_DONT_INDENT)
         {
            // no change
         }
         else if (  pc->parent_type == CT_SQL_EXEC
                 && cpd.settings[UO_indent_preserve_sql].b)
         {
            reindent_line(pc, sql_col + (pc->orig_col - sql_orig_col));
            LOG_FMT(LINDENT, "Indent SQL: [%s] to %zu (%zu/%zu)\n",
                    pc->text(), pc->column, sql_col, sql_orig_col);
         }
         else if (  (pc->flags & PCF_STMT_START) == 0
                 && (  pc->type == CT_MEMBER
                    || (  pc->type == CT_DC_MEMBER
                       && prev != nullptr
                       && prev->type == CT_TYPE)
                    || (  prev != nullptr
                       && (  prev->type == CT_MEMBER
                          || (  prev->type == CT_DC_MEMBER
                             && prevv->type == CT_TYPE)))))
         {
            size_t tmp = cpd.settings[UO_indent_member].u + indent_column;
            LOG_FMT(LINDENT, "%s(%d): %zu] member => %zu\n",
                    __func__, __LINE__, pc->orig_line, tmp);
            reindent_line(pc, tmp);
         }

         else if (do_vardefcol)
         {
            LOG_FMT(LINDENT, "%s(%d): %zu] Vardefcol => %zu\n",
                    __func__, __LINE__, pc->orig_line, vardefcol);
            reindent_line(pc, vardefcol);
         }
         else if (shiftcontcol > 0)
         {
            LOG_FMT(LINDENT, "%s(%d): %zu] indent_shift => %zu\n",
                    __func__, __LINE__, pc->orig_line, shiftcontcol);
            reindent_line(pc, shiftcontcol);
         }
         else if (  pc->type == CT_NAMESPACE
                 && cpd.settings[UO_indent_namespace].b
                 && cpd.settings[UO_indent_namespace_single_indent].b
                 && frm.pse[frm.pse_tos].ns_cnt)
         {
            LOG_FMT(LINDENT, "%s(%d): %zu] Namespace => %d\n",
                    __func__, __LINE__, pc->orig_line, frm.pse[frm.pse_tos].brace_indent);
            reindent_line(pc, frm.pse[frm.pse_tos].brace_indent);
         }
         else if (  pc->type == CT_STRING
                 && prev != nullptr
                 && prev->type == CT_STRING
                 && cpd.settings[UO_indent_align_string].b)
         {
            int tmp = (xml_indent != 0) ? xml_indent : prev->column;

            LOG_FMT(LINDENT, "%s(%d): %zu] String => %d\n",
                    __func__, __LINE__, pc->orig_line, tmp);
            reindent_line(pc, tmp);
         }
         else if (chunk_is_comment(pc))
         {
            LOG_FMT(LINDENT, "%s(%d): %zu] comment => %zu\n",
                    __func__, __LINE__, pc->orig_line, frm.pse[frm.pse_tos].indent_tmp);
            indent_comment(pc, frm.pse[frm.pse_tos].indent_tmp);
         }
         else if (pc->type == CT_PREPROC)
         {
            LOG_FMT(LINDENT, "%s(%d): %zu] pp-indent => %zu [%s]\n",
                    __func__, __LINE__, pc->orig_line, indent_column, pc->text());
            reindent_line(pc, indent_column);
         }
         else if (chunk_is_paren_close(pc) || pc->type == CT_ANGLE_CLOSE)
         {
            /*
             * This is a big hack. We assume that since we hit a paren close,
             * that we just removed a paren open
             */
            LOG_FMT(LINDLINE, "%s(%d): indent_column is %zu\n",
                    __func__, __LINE__, indent_column);
            if (frm.pse[frm.pse_tos + 1].type == c_token_t(pc->type - 1))
            {
               // Issue # 405
               LOG_FMT(LINDLINE, "%s(%d): [%zu:%zu] [%s:%s]\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
               chunk_t *ck1 = frm.pse[frm.pse_tos + 1].pc;
               chunk_t *ck2 = chunk_get_prev(ck1);

               /*
                * If the open parenthesis was the first thing on the line or we
                * are doing mode 1, then put the close parenthesis in the same
                * column
                */
               if (  chunk_is_newline(ck2)
                  || (cpd.settings[UO_indent_paren_close].u == 1))
               {
                  LOG_FMT(LINDLINE, "%s(%d): [%zu:%zu] indent_paren_close is 1\n",
                          __func__, __LINE__, ck2->orig_line, ck2->orig_col);
                  indent_column_set(ck1->column);
                  LOG_FMT(LINDLINE, "%s(%d): [%zu:%zu] indent_column set to %zu\n",
                          __func__, __LINE__, ck2->orig_line, ck2->orig_col, indent_column);
               }
               else
               {
                  if (cpd.settings[UO_indent_paren_close].u != 2)
                  {
                     // 0 or 1
                     LOG_FMT(LINDLINE, "%s(%d): [%zu:%zu] indent_paren_close is 0 or 1\n",
                             __func__, __LINE__, ck2->orig_line, ck2->orig_col);
                     indent_column_set(frm.pse[frm.pse_tos + 1].indent_tmp);
                     LOG_FMT(LINDLINE, "%s(%d): [%zu:%zu] indent_column set to %zu\n",
                             __func__, __LINE__, ck2->orig_line, ck2->orig_col, indent_column);
                     pc->column_indent = frm.pse[frm.pse_tos + 1].indent_tab;
                     if (cpd.settings[UO_indent_paren_close].u == 1)
                     {
                        LOG_FMT(LINDLINE, "%s(%d): [%zu:%zu] indent_paren_close is 1\n",
                                __func__, __LINE__, ck2->orig_line, ck2->orig_col);
                        indent_column--;
                        LOG_FMT(LINDLINE, "%s(%d): [%zu:%zu] indent_column set to %zu\n",
                                __func__, __LINE__, ck2->orig_line, ck2->orig_col, indent_column);
                     }
                  }
                  else
                  {
                     // 2
                     LOG_FMT(LINDLINE, "%s(%d): [%zu:%zu] indent_paren_close is 2\n",
                             __func__, __LINE__, ck2->orig_line, ck2->orig_col);
                  }
               }
            }
            LOG_FMT(LINDENT, "%s(%d): %zu] cl paren => %zu [%s]\n",
                    __func__, __LINE__, pc->orig_line, indent_column, pc->text());
            reindent_line(pc, indent_column);
         }
         else if (pc->type == CT_COMMA)
         {
            if (  cpd.settings[UO_indent_comma_paren].b
               && chunk_is_paren_open(frm.pse[frm.pse_tos].pc))
            {
               indent_column_set(frm.pse[frm.pse_tos].pc->column);
            }
            LOG_FMT(LINDENT, "%s(%d): %zu] comma => %zu [%s]\n",
                    __func__, __LINE__, pc->orig_line, indent_column, pc->text());
            reindent_line(pc, indent_column);
         }
         else if (  cpd.settings[UO_indent_func_const].u
                 && pc->type == CT_QUALIFIER
                 && strncasecmp(pc->text(), "const", pc->len()) == 0
                 && (  next == nullptr
                    || next->type == CT_BRACED
                    || next->type == CT_BRACE_OPEN
                    || next->type == CT_NEWLINE
                    || next->type == CT_SEMICOLON
                    || next->type == CT_THROW
                    || next->type == CT_VBRACE_OPEN))
         {
            // indent const - void GetFoo(void)\n const\n { return (m_Foo); }
            indent_column_set(cpd.settings[UO_indent_func_const].u);
            LOG_FMT(LINDENT, "%s(%d): %zu] const => %zu [%s]\n",
                    __func__, __LINE__, pc->orig_line, indent_column, pc->text());
            reindent_line(pc, indent_column);
         }
         else if (  cpd.settings[UO_indent_func_throw].u
                 && pc->type == CT_THROW
                 && pc->parent_type != CT_NONE)
         {
            // indent throw - void GetFoo(void)\n throw()\n { return (m_Foo); }
            indent_column_set(cpd.settings[UO_indent_func_throw].u);
            LOG_FMT(LINDENT, "%s(%d): %zu] throw => %zu [%s]\n",
                    __func__, __LINE__, pc->orig_line, indent_column, pc->text());
            reindent_line(pc, indent_column);
         }
         else if (pc->type == CT_BOOL)
         {
            if (  cpd.settings[UO_indent_bool_paren].b
               && chunk_is_paren_open(frm.pse[frm.pse_tos].pc))
            {
               indent_column_set(frm.pse[frm.pse_tos].pc->column);
               if (cpd.settings[UO_indent_first_bool_expr].b)
               {
                  reindent_line(chunk_get_next(frm.pse[frm.pse_tos].pc),
                                indent_column + pc->len() + 1);
               }
            }
            LOG_FMT(LINDENT, "%s(%d): %zu] bool => %zu [%s]\n",
                    __func__, __LINE__, pc->orig_line, indent_column, pc->text());
            reindent_line(pc, indent_column);
         }
         else if (  cpd.settings[UO_indent_ternary_operator].u == 1
                 && (  (  pc->type == CT_ADDR
                       || pc->type == CT_WORD
                       || pc->type == CT_DEREF
                       || pc->type == CT_NUMBER
                       || pc->type == CT_STRING
                       || pc->type == CT_PAREN_OPEN)
                    && prev->type == CT_COND_COLON))
         {
            chunk_t *tmp = chunk_get_prev_type(prev, CT_QUESTION, -1);
            tmp = chunk_get_next_ncnl(tmp);
            LOG_FMT(LINDENT, "%s: %zu] ternarydefcol => %zu [%s]\n",
                    __func__, pc->orig_line, tmp->column, pc->text());
            reindent_line(pc, tmp->column);
         }
         else if (  cpd.settings[UO_indent_ternary_operator].u == 2
                 && pc->type == CT_COND_COLON)
         {
            chunk_t *tmp = chunk_get_prev_type(pc, CT_QUESTION, -1);
            LOG_FMT(LINDENT, "%s: %zu] ternarydefcol => %zu [%s]\n",
                    __func__, pc->orig_line, tmp->column, pc->text());
            reindent_line(pc, tmp->column);
         }
         else
         {
            bool   use_ident = true;
            size_t ttidx     = frm.pse_tos;
            if (ttidx > 0)
            {
               //if (strcasecmp(get_token_name(frm.pse[ttidx].pc->parent_type), "FUNC_CALL") == 0)
               if ((frm.pse[ttidx].pc)->parent_type == CT_FUNC_CALL)
               {
                  LOG_FMT(LINDPC, "FUNC_CALL OK [%d]\n", __LINE__);
                  if (cpd.settings[UO_use_indent_func_call_param].b)
                  {
                     LOG_FMT(LINDPC, "use is true [%d]\n", __LINE__);
                  }
                  else
                  {
                     LOG_FMT(LINDPC, "use is false [%d]\n", __LINE__);
                     use_ident = false;
                  }
               }
            }
            if (pc->column != indent_column)
            {
               if (use_ident && pc->type != CT_PP_IGNORE) // Leave indentation alone for PP_IGNORE tokens
               {
                  LOG_FMT(LINDENT, "%s(%d): orig_line is %zu, indent set to %zu, for '%s'\n",
                          __func__, __LINE__, pc->orig_line, indent_column, pc->text());
                  reindent_line(pc, indent_column);
               }
               else
               {
                  // do not indent this line
                  LOG_FMT(LINDENT, "%s(%d): %zu] don't indent this line [%d]\n",
                          __func__, __LINE__, pc->orig_line, __LINE__);
               }
            }
         }
         did_newline = false;

         if (  pc->type == CT_SQL_EXEC
            || pc->type == CT_SQL_BEGIN
            || pc->type == CT_SQL_END)
         {
            sql_col      = pc->column;
            sql_orig_col = pc->orig_col;
         }

         // Handle indent for variable defs at the top of a block of code
         if (pc->flags & PCF_VAR_TYPE)
         {
            if (  !frm.pse[frm.pse_tos].non_vardef
               && (frm.pse[frm.pse_tos].type == CT_BRACE_OPEN))
            {
               const auto val = cpd.settings[UO_indent_var_def_blk].n;
               if (val != 0)
               {
                  auto indent = indent_column;
                  indent = (val > 0) ? val                     // reassign if positive val,
                           : (cast_abs(indent, val) < indent)  // else if no underflow
                           ? (indent + val) : 0;               // reduce, else 0

                  reindent_line(pc, indent);
                  LOG_FMT(LINDENT, "%s(%d): %zu] var_type indent => %zu [%s]\n",
                          __func__, __LINE__, pc->orig_line, indent, pc->text());
               }
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

      // if we hit a newline, reset indent_tmp
      if (  chunk_is_newline(pc)
         || pc->type == CT_COMMENT_MULTI
         || pc->type == CT_COMMENT_CPP)
      {
         frm.pse[frm.pse_tos].indent_tmp = frm.pse[frm.pse_tos].indent;
         log_indent_tmp();

         /*
          * Handle the case of a multi-line #define w/o anything on the
          * first line (indent_tmp will be 1 or 0)
          */
         if (  pc->type == CT_NL_CONT
            && (frm.pse[frm.pse_tos].indent_tmp <= indent_size))
         {
            frm.pse[frm.pse_tos].indent_tmp = indent_size + 1;
            log_indent_tmp();
         }

         // Get ready to indent the next item
         did_newline = true;
      }

      // Check for open XML tags "</..."
      if (cpd.settings[UO_indent_xml_string].u > 0)
      {
         if (pc->type == CT_STRING)
         {
            if (  (pc->len() > 4)
               && (pc->str[1] == '<')
               && (pc->str[2] != '/')
               && (pc->str[pc->len() - 3] != '/'))
            {
               if (xml_indent <= 0)
               {
                  xml_indent = pc->column;
               }
               xml_indent += cpd.settings[UO_indent_xml_string].u;
            }
         }
      }

      if (!chunk_is_comment(pc) && !chunk_is_newline(pc))
      {
         prev = pc;
      }
      pc = chunk_get_next(pc);
   }
null_pc:

   // Throw out any stuff inside a preprocessor - no need to warn
   while (frm.pse_tos > 0 && frm.pse[frm.pse_tos].in_preproc)
   {
      indent_pse_pop(frm, pc);
   }

   // Throw out any VBRACE_OPEN at the end - implied with the end of file
   while (frm.pse_tos > 0 && (frm.pse[frm.pse_tos].type == CT_VBRACE_OPEN))
   {
      indent_pse_pop(frm, pc);
   }

   for (size_t idx_temp = 1; idx_temp <= frm.pse_tos; idx_temp++)
   {
      LOG_FMT(LWARN, "%s:%zu Unmatched %s\n",
              cpd.filename, frm.pse[idx_temp].open_line,
              get_token_name(frm.pse[idx_temp].type));
      cpd.error_count++;
   }

   quick_align_again();
   quick_indent_again();
} // indent_text


static bool single_line_comment_indent_rule_applies(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc      = start;
   size_t  nl_count = 0;

   if (!chunk_is_single_line_comment(pc))
   {
      return(false);
   }

   /*
    * scan forward, if only single newlines and comments before next line of
    * code, we want to apply
    */
   while ((pc = chunk_get_next(pc)) != nullptr)
   {
      if (chunk_is_newline(pc))
      {
         if (nl_count > 0 || pc->nl_count > 1)
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
            /*
             * here we check for things to run into that we wouldn't want to
             * indent the comment for. for example, non-single line comment,
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
} // single_line_comment_indent_rule_applies


static void indent_comment(chunk_t *pc, size_t col)
{
   LOG_FUNC_ENTRY();
   chunk_t *nl;
   chunk_t *prev;

   LOG_FMT(LCMTIND, "%s(%d): orig_line %zu, orig_col %zu, level %zu: ",
           __func__, __LINE__, pc->orig_line, pc->orig_col, pc->level);
#ifdef DEBUG
   LOG_FMT(LCMTIND, "\n");
#endif

   // force column 1 comment to column 1 if not changing them
   if (  pc->orig_col == 1
      && !cpd.settings[UO_indent_col1_comment].b
      && ((pc->flags & PCF_INSERTED) == 0))
   {
      LOG_FMT(LCMTIND, "rule 1 - keep in col 1\n");
      reindent_line(pc, 1);
      return;
   }

   nl = chunk_get_prev(pc);

   // outside of any expression or statement?
   if (pc->level == 0)
   {
      if (nl != nullptr && nl->nl_count > 1)
      {
         LOG_FMT(LCMTIND, "rule 2 - level 0, nl before\n");
         reindent_line(pc, 1);
         return;
      }
   }

   prev = chunk_get_prev(nl);
   if (chunk_is_comment(prev) && nl->nl_count == 1)
   {
      int coldiff = prev->orig_col - pc->orig_col;

      /*
       * Here we want to align comments that are relatively close one to another
       * but not when the comment is a Doxygen comment
       */
      // Issue #1134
      if (  coldiff <= 3
         && coldiff >= -3
         && !chunk_is_Doxygen_comment(pc))
      {
         reindent_line(pc, prev->column);
         LOG_FMT(LCMTIND, "rule 3 - prev comment, coldiff = %d, now in %zu\n",
                 coldiff, pc->column);
         return;
      }
   }

   // check if special single line comment rule applies
   if (  (cpd.settings[UO_indent_sing_line_comments].u > 0)
      && single_line_comment_indent_rule_applies(pc))
   {
      reindent_line(pc, col + cpd.settings[UO_indent_sing_line_comments].u);
      LOG_FMT(LCMTIND, "rule 4 - single line comment indent, now in %zu\n", pc->column);
      return;
   }
   LOG_FMT(LCMTIND, "rule 5 - fall-through, stay in %zu\n", col);

   reindent_line(pc, col);
} // indent_comment


bool ifdef_over_whole_file(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *next;
   chunk_t *end_pp = nullptr;
   size_t  stage   = 0;

   // the results for this file are cached
   if (cpd.ifdef_over_whole_file)
   {
      return(cpd.ifdef_over_whole_file > 0);
   }

   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next(pc))
   {
      if (chunk_is_comment(pc) || chunk_is_newline(pc))
      {
         continue;
      }

      if (stage == 0)
      {
         // Check the first preprocessor, make sure it is an #if type
         if (pc->type != CT_PREPROC)
         {
            break;
         }
         next = chunk_get_next(pc);
         if (next == nullptr || next->type != CT_PP_IF)
         {
            break;
         }
         stage = 1;
      }
      else if (stage == 1)
      {
         // Scan until a preprocessor at level 0 is found - the close to the #if
         if (pc->type == CT_PREPROC && pc->pp_level == 0)
         {
            stage  = 2;
            end_pp = pc;
         }
         continue;
      }
      else if (stage == 2)
      {
         // We should only see the rest of the preprocessor
         if (pc->type == CT_PREPROC || ((pc->flags & PCF_IN_PREPROC) == 0))
         {
            stage = 0;
            break;
         }
      }
   }

   cpd.ifdef_over_whole_file = (stage == 2) ? 1 : -1;
   if (cpd.ifdef_over_whole_file > 0)
   {
      chunk_flags_set(end_pp, PCF_WF_ENDIF);
   }
   LOG_FMT(LNOTE, "The whole file is%s covered by a #IF\n",
           (cpd.ifdef_over_whole_file > 0) ? "" : " NOT");
   return(cpd.ifdef_over_whole_file > 0);
} // ifdef_over_whole_file


void indent_preproc(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *next;
   int     pp_level;
   int     pp_level_sub = 0;

   // Scan to see if the whole file is covered by one #ifdef
   if (ifdef_over_whole_file())
   {
      pp_level_sub = 1;
   }

   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next(pc))
   {
      if (pc->type != CT_PREPROC)
      {
         continue;
      }

      next = chunk_get_next_ncnl(pc);
      if (next == nullptr)
      {
         break;
      }

      pp_level = pc->pp_level - pp_level_sub;
      if (pp_level < 0)
      {
         pp_level = 0;
      }

      // Adjust the indent of the '#'
      if (cpd.settings[UO_pp_indent].a & AV_ADD)
      {
         reindent_line(pc, 1 + pp_level * cpd.settings[UO_pp_indent_count].u);
      }
      else if (cpd.settings[UO_pp_indent].a & AV_REMOVE)
      {
         reindent_line(pc, 1);
      }

      // Add spacing by adjusting the length
      if ((cpd.settings[UO_pp_space].a != AV_IGNORE) && next != nullptr)
      {
         if (cpd.settings[UO_pp_space].a & AV_ADD)
         {
            size_t mult = cpd.settings[UO_pp_space_count].u;

            if (mult < 1)
            {
               mult = 1;
            }
            reindent_line(next, pc->column + pc->len() + (pp_level * mult));
         }
         else if (cpd.settings[UO_pp_space].a & AV_REMOVE)
         {
            reindent_line(next, pc->column + pc->len());
         }
      }

      // Mark as already handled if not region stuff or in column 1
      if (  (  !cpd.settings[UO_pp_indent_at_level].b
            || (pc->brace_level <= ((pc->parent_type == CT_PP_DEFINE) ? 1 : 0)))
         && pc->parent_type != CT_PP_REGION
         && pc->parent_type != CT_PP_ENDREGION)
      {
         if (  !cpd.settings[UO_pp_define_at_level].b
            || pc->parent_type != CT_PP_DEFINE)
         {
            chunk_flags_set(pc, PCF_DONT_INDENT);
         }
      }

      LOG_FMT(LPPIS, "%s(%d): orig_line %zu to %d (len %zu, next->col %zu)\n",
              __func__, __LINE__, pc->orig_line, 1 + pp_level, pc->len(),
              next ? next->column : -1);
   }
} // indent_preproc
