/**
 * @file indent.cpp
 * Does all the indenting stuff.
 *
 * @author  Ben Gardner
 * @author  Guy Maurel October 2015- 2021
 * @license GPL v2+
 */

#include "indent.h"

#include "align.h"
#include "frame_list.h"
#include "options_for_QT.h"
#include "prototypes.h"
#include "quick_align_again.h"
#include "space.h"

#ifdef WIN32
#include <algorithm>                   // to get max
#endif // ifdef WIN32

#ifdef IGNORE // WinBase.h
#undef IGNORE
#endif


constexpr static auto LCURRENT = LINDENT;

using namespace std;
using namespace uncrustify;


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
 *    a_really_long_function_name(
 *       param1, param2);
 *    a_really_long_function_name(param1,
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
static void indent_comment(Chunk *pc, size_t col);


static size_t token_indent(E_Token type);


static size_t calc_indent_continue(const ParseFrame &frm, size_t pse_tos);

/**
 * Get candidate chunk first on line to which OC blocks can be indented against.
 */
static Chunk *candidate_chunk_first_on_line(Chunk *pc);

/**
 * We are on a '{' that has parent = OC_BLOCK_EXPR
 * find the column of the param tag
 */
static Chunk *oc_msg_block_indent(Chunk *pc, bool from_brace, bool from_caret, bool from_colon, bool from_keyword);


/**
 * returns true if forward or reverse scan reveals only single newlines or comments
 * stops when hits code
 * false if next thing hit is a closing brace, also if 2 newlines in a row
 */
static bool single_line_comment_indent_rule_applies(Chunk *start, bool forward);

/**
 * returns true if semicolon on the same level ends any assign operations
 * false if next thing hit is not the end of an assign operation
 */
static bool is_end_of_assignment(Chunk *pc, const ParseFrame &frm);


void indent_to_column(Chunk *pc, size_t column)
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


enum class indent_mode_e : int
{
   INDENT = 0,   //! indent by one level
   ALIGN  = 1,   //! align under the open brace/parenthesis
   IGNORE = -1,  //! preserve original indentation
};


void align_to_column(Chunk *pc, size_t column)
{
   LOG_FUNC_ENTRY();

   if (  pc == nullptr
      || column == pc->column)
   {
      return;
   }
   LOG_FMT(LINDLINE, "%s(%d): orig_line is %zu, orig_col is %zu, Text() '%s', type is %s => column is %zu\n",
           __func__, __LINE__, pc->orig_line, pc->column, pc->Text(),
           get_token_name(pc->type), column);

   const auto col_delta = static_cast<int>(column) - static_cast<int>(pc->column);
   size_t     min_col   = column;

   pc->column = column;

   do
   {
      auto *next = pc->GetNext();

      if (next->IsNullChunk())
      {
         break;
      }
      const size_t min_delta = space_col_align(pc, next);
      min_col += min_delta;

      const auto *prev = pc;
      pc = next;

      auto almod = align_mode_e::SHIFT;

      if (  pc->IsComment()
         && get_chunk_parent_type(pc) != CT_COMMENT_EMBED)
      {
         log_rule_B("indent_relative_single_line_comments");
         almod = (  pc->IsSingleLineComment()
                 && options::indent_relative_single_line_comments())
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
      LOG_FMT(LINDLINED, "%s(%d):   %s set column of '%s', type is %s, orig_line is %zu, to col %zu (orig_col was %zu)\n",
              __func__, __LINE__,
              (almod == align_mode_e::KEEP_ABS) ? "abs" :
              (almod == align_mode_e::KEEP_REL) ? "rel" : "sft",
              pc->Text(), get_token_name(pc->type), pc->orig_line, pc->column, pc->orig_col);
   } while (  pc->IsNotNullChunk()
           && pc->nl_count == 0);
} // align_to_column


void reindent_line(Chunk *pc, size_t column)
{
   LOG_FUNC_ENTRY();
   char copy[1000];

   LOG_FMT(LINDLINE, "%s(%d): orig_line is %zu, orig_col is %zu, on '%s' [%s/%s] => %zu\n",
           __func__, __LINE__, pc->orig_line, pc->orig_col, pc->ElidedText(copy),
           get_token_name(pc->type), get_token_name(get_chunk_parent_type(pc)),
           column);
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
         if (!pc->flags.test(PCF_IN_QT_MACRO))
         {
            LOG_FMT(LINDLINE, "FLAGS is NOT set: PCF_IN_QT_MACRO\n");
            restore_options_for_QT();
         }
      }
      else
      {
         // look for begin of SIGNAL/SLOT block
         if (pc->flags.test(PCF_IN_QT_MACRO))
         {
            LOG_FMT(LINDLINE, "FLAGS is set: PCF_IN_QT_MACRO\n");
            save_set_options_for_QT(pc->level);
         }
      }
      Chunk *next = pc->GetNext();

      if (next->IsNullChunk())
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

      const bool is_comment = pc->IsComment();
      log_rule_B("indent_relative_single_line_comments");
      const bool keep = (  is_comment
                        && pc->IsSingleLineComment()
                        && options::indent_relative_single_line_comments());

      if (  is_comment
         && get_chunk_parent_type(pc) != CT_COMMENT_EMBED
         && !keep)
      {
         pc->column = max(pc->orig_col, min_col);
         LOG_FMT(LINDLINE, "%s(%d): set comment on line %zu to col %zu (orig %zu)\n",
                 __func__, __LINE__, pc->orig_line, pc->column, pc->orig_col);
      }
      else
      {
         const auto tmp_col = static_cast<int>(pc->column) + col_delta;
         pc->column = max(tmp_col, static_cast<int>(min_col));

         LOG_FMT(LINDLINED, "%s(%d): set column of ", __func__, __LINE__);

         if (pc->Is(CT_NEWLINE))
         {
            LOG_FMT(LINDLINED, "<Newline>");
         }
         else
         {
            LOG_FMT(LINDLINED, "'%s'", pc->Text());
         }
         LOG_FMT(LINDLINED, " to %zu (orig %zu)\n", pc->column, pc->orig_col);
      }
   } while (  pc->IsNotNullChunk()
           && pc->nl_count == 0);
} // reindent_line


static size_t token_indent(E_Token type)
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


static size_t get_indent_first_continue(Chunk *pc)
{
   log_rule_B("indent_ignore_first_continue");
   Chunk *continuation = pc->GetNextType(CT_NEWLINE, pc->level);

   if (continuation->IsNotNullChunk())
   {
      continuation = continuation->GetNext();

      if (continuation->IsNotNullChunk())
      {
         return(continuation->orig_col);
      }
   }
   return(0);
}


static size_t calc_indent_continue(const ParseFrame &frm, size_t pse_tos)
{
   log_rule_B("indent_continue");
   const int ic = options::indent_continue();

   if (  ic < 0
      && frm.at(pse_tos).indent_cont)
   {
      return(frm.at(pse_tos).indent);
   }
   return(frm.at(pse_tos).indent + abs(ic));
}


static size_t calc_indent_continue(const ParseFrame &frm)
{
   return(calc_indent_continue(frm, frm.size() - 1));
}


static Chunk *candidate_chunk_first_on_line(Chunk *pc)
{
   Chunk *first = pc->GetFirstChunkOnLine();

   log_rule_B("indent_inside_ternary_operator");

   if (  options::indent_inside_ternary_operator()
      && (  first->Is(CT_QUESTION)
         || first->Is(CT_COND_COLON)))
   {
      return(first->GetNextNcNnl());
   }
   else
   {
      return(first);
   }
}


static Chunk *oc_msg_block_indent(Chunk *pc, bool from_brace,
                                  bool from_caret, bool from_colon,
                                  bool from_keyword)
{
   LOG_FUNC_ENTRY();
   Chunk *tmp = Chunk::NullChunkPtr;

   if (pc != nullptr)
   {
      tmp = pc->GetPrevNc();
   }

   if (from_brace)
   {
      return(pc);
   }

   // Skip to open paren in ':^TYPE *(ARGS) {'
   if (tmp->IsParenClose())
   {
      tmp = tmp->SkipToMatchRev()->GetPrevNc();
   }

   // // Check for star in ':^TYPE *(ARGS) {'. Issue 2477
   if (tmp->Is(CT_PTR_TYPE))
   {
      tmp = tmp->GetPrevNc();
   }

   // Check for type in ':^TYPE *(ARGS) {'. Issue 2482
   if (tmp->Is(CT_TYPE))
   {
      tmp = tmp->GetPrevNc();
   }
   // Check for caret in ':^TYPE *(ARGS) {'
   // Store the caret position
   Chunk *caret_tmp = Chunk::NullChunkPtr;

   if (  tmp->IsNotNullChunk()
      && tmp->type == CT_OC_BLOCK_CARET)
   {
      caret_tmp = tmp;
   }
   else
   {
      caret_tmp = tmp->GetPrevType(CT_OC_BLOCK_CARET);
      tmp       = caret_tmp;
   }

   // If we still cannot find caret then return first chunk on the line
   if (  tmp->IsNullChunk()
      || tmp->IsNot(CT_OC_BLOCK_CARET))
   {
      return(candidate_chunk_first_on_line(pc));
   }

   if (from_caret)
   {
      return(tmp);
   }
   tmp = tmp->GetPrevNc();

   // Check for colon in ':^TYPE *(ARGS) {'
   if (from_colon)
   {
      if (  tmp->IsNullChunk()
         || tmp->IsNot(CT_OC_COLON))
      {
         return(candidate_chunk_first_on_line(pc));
      }
      else
      {
         return(tmp);
      }
   }
   tmp = tmp->GetPrevNc();

   if (from_keyword)
   {
      if (  tmp->IsNullChunk()
         || (  tmp->IsNot(CT_OC_MSG_NAME)
            && tmp->IsNot(CT_OC_MSG_FUNC)))
      {
         return(candidate_chunk_first_on_line(pc));
      }
      else
      {
         return(tmp);
      }
   }
   // In almost all the cases, its better to return the first chunk on the line than not indenting at all.
   tmp = candidate_chunk_first_on_line(pc);
   return(tmp);
} // oc_msg_block_indent


#define log_indent()                          \
   do { _log_indent(__func__, __LINE__, frm); \
   } while (false)


static void _log_indent(const char *func, const uint32_t line, const ParseFrame &frm)
{
   LOG_FMT(LINDLINE, "%s(%d): frm.pse_tos is %zu, ...indent is %zu\n",
           func, line, frm.size() - 1, frm.top().indent);
}


#define log_prev_indent()                          \
   do { _log_prev_indent(__func__, __LINE__, frm); \
   } while (false)


static void _log_prev_indent(const char *func, const uint32_t line, const ParseFrame &frm)
{
   LOG_FMT(LINDLINE, "%s(%d): frm.pse_tos is %zu, prev....indent is %zu\n",
           func, line, frm.size() - 1, frm.prev().indent);
}


#define log_indent_tmp()                          \
   do { _log_indent_tmp(__func__, __LINE__, frm); \
   } while (false)


static void _log_indent_tmp(const char *func, const uint32_t line, const ParseFrame &frm)
{
   LOG_FMT(LINDLINE, "%s(%d): frm.pse_tos is %zu, ...indent_tmp is %zu\n",
           func, line, frm.size() - 1, frm.top().indent_tmp);
}


static void quick_indent_again()
{
   LOG_FUNC_ENTRY();

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (pc->indent.ref == nullptr)
      {
         continue;
      }
      Chunk *tmp = pc->GetPrev();

      if (!tmp->IsNewline())
      {
         continue;
      }
      const size_t col = pc->indent.ref->column + pc->indent.delta;
      indent_to_column(pc, col);

      LOG_FMT(LINDENTAG, "%s(%d): [%zu] indent [%s] to %zu based on [%s] @ %zu:%zu\n",
              __func__, __LINE__, pc->orig_line, pc->Text(), col,
              pc->indent.ref->Text(), pc->indent.ref->orig_line,
              pc->indent.ref->column);
   }
}


void indent_text()
{
   LOG_FUNC_ENTRY();
   bool   did_newline = true;
   size_t vardefcol   = 0;

   log_rule_B("indent_columns");
   const size_t indent_size   = options::indent_columns();
   size_t       indent_column = 0;
   int          xml_indent    = 0;
   size_t       sql_col       = 0;
   size_t       sql_orig_col  = 0;
   bool         in_func_def   = false;


   std::vector<ParseFrame> frames;
   ParseFrame              frm{};


   Chunk *pc        = Chunk::GetHead();
   bool  classFound = false;                                 // Issue #672

   while (  pc != nullptr
         && pc->IsNotNullChunk())
   {
      LOG_FMT(LINDLINE, "%s(%d): orig_line is %zu, orig_col is %zu, for '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
      //  forces string literal to column-1 [Fix for 1246]
      log_rule_B("indent_col1_multi_string_literal");

      if (  (pc->type == CT_STRING_MULTI)
         && !(cpd.lang_flags & LANG_OC)                      // Issue #1795
         && options::indent_col1_multi_string_literal())
      {
         string str = pc->Text();

         if (  (str[0] == '@')
            && (pc->GetPrev()->type == CT_NEWLINE))
         {
            indent_column_set(1);
            reindent_line(pc, indent_column);
            pc          = pc->GetNext();
            did_newline = false;
         }
      }

      if (pc->Is(CT_NEWLINE))
      {
         LOG_FMT(LINDLINE, "%s(%d): orig_line is %zu, <Newline>\n",
                 __func__, __LINE__, pc->orig_line);
      }
      else if (pc->Is(CT_NL_CONT))
      {
         LOG_FMT(LINDLINE, "%s(%d): orig_line is %zu, CT_NL_CONT\n",
                 __func__, __LINE__, pc->orig_line);
      }
      else
      {
         char copy[1000];
         LOG_FMT(LINDLINE, "%s(%d): orig_line is %zu, orig_col is %zu, column is %zu, for '%s'\n   ",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->column, pc->ElidedText(copy));
         log_pcf_flags(LINDLINE, pc->flags);
      }
      log_rule_B("use_options_overriding_for_qt_macros");

      if (  options::use_options_overriding_for_qt_macros()
         && (  strcmp(pc->Text(), "SIGNAL") == 0
            || strcmp(pc->Text(), "SLOT") == 0))
      {
         LOG_FMT(LINDLINE, "%s(%d): orig_line=%zu: type %s SIGNAL/SLOT found\n",
                 __func__, __LINE__, pc->orig_line, get_token_name(pc->type));
      }
      // Handle preprocessor transitions
      log_rule_B("indent_brace_parent");
      const size_t parent_token_indent = (options::indent_brace_parent())
                                         ? token_indent(get_chunk_parent_type(pc)) : 0;

      // Handle "force indentation of function definition to start in column 1"
      log_rule_B("indent_func_def_force_col1");

      if (options::indent_func_def_force_col1())
      {
         if (!in_func_def)
         {
            Chunk *next = pc->GetNextNcNnl();

            if (  get_chunk_parent_type(pc) == CT_FUNC_DEF
               || (  pc->Is(CT_COMMENT)
                  && next->IsNotNullChunk()
                  && get_chunk_parent_type(next) == CT_FUNC_DEF))
            {
               in_func_def = true;
               frm.push(pc, __func__, __LINE__);
               frm.top().indent_tmp = 1;
               frm.top().indent     = 1;
               frm.top().indent_tab = 1;
            }
         }
         else
         {
            Chunk *prev = pc->GetPrev();

            if (  prev->Is(CT_BRACE_CLOSE)
               && get_chunk_parent_type(prev) == CT_FUNC_DEF)
            {
               in_func_def = false;
               LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
               frm.pop(__func__, __LINE__, pc);
            }
         }
      }
      // Clean up after a #define, etc
      const bool in_preproc = pc->flags.test(PCF_IN_PREPROC);

      if (!in_preproc)
      {
         while (  !frm.empty()
               && frm.top().in_preproc)
         {
            const E_Token type = frm.top().type;
            LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
            frm.pop(__func__, __LINE__, pc);

            /*
             * If we just removed an #endregion, then check to see if a
             * PP_REGION_INDENT entry is right below it
             */
            if (  type == CT_PP_ENDREGION
               && frm.top().type == CT_PP_REGION_INDENT)
            {
               LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
               frm.pop(__func__, __LINE__, pc);
            }
         }
      }
      else if (pc->Is(CT_PREPROC)) // #
      {
         // Close out PP_IF_INDENT before playing with the parse frames
         if (  frm.top().type == CT_PP_IF_INDENT
            && (  get_chunk_parent_type(pc) == CT_PP_ENDIF
               || get_chunk_parent_type(pc) == CT_PP_ELSE))
         {
            LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
            frm.pop(__func__, __LINE__, pc);
         }
         ParseFrame frmbkup = frm;
         fl_check(frames, frm, cpd.pp_level, pc);

         // Indent the body of a #region here
         log_rule_B("pp_region_indent_code");

         if (  options::pp_region_indent_code()
            && get_chunk_parent_type(pc) == CT_PP_REGION)
         {
            Chunk *next = pc->GetNext();

            if (next->IsNullChunk())
            {
               break;
            }
            // Hack to get the logs to look right
            set_chunk_type(next, CT_PP_REGION_INDENT);
            frm.push(next, __func__, __LINE__);
            set_chunk_type(next, CT_PP_REGION);

            // Indent one level
            frm.top().indent = frm.prev().indent + indent_size;
            log_indent();

            frm.top().indent_tab = frm.prev().indent_tab + indent_size;
            frm.top().indent_tmp = frm.top().indent;
            frm.top().in_preproc = false;
            log_indent_tmp();
         }
         // If option set, remove indent inside switch statement
         log_rule_B("indent_switch_pp");

         if (  frm.top().type == CT_CASE
            && !options::indent_switch_pp())
         {
            frm.push(pc, __func__, __LINE__);
            LOG_FMT(LINDPC, "%s(%d): frm.top().indent is %zu, indent_size is %zu\n",
                    __func__, __LINE__, frm.top().indent, indent_size);

            if (frm.top().indent >= indent_size)
            {
               frm.prev().indent = frm.top().indent - indent_size;
            }
            log_prev_indent();
         }
         // Indent the body of a #if here
         log_rule_B("pp_if_indent_code");

         if (  options::pp_if_indent_code()
            && (  get_chunk_parent_type(pc) == CT_PP_IF
               || get_chunk_parent_type(pc) == CT_PP_ELSE))
         {
            Chunk *next = pc->GetNext();

            if (next->IsNullChunk())
            {
               break;
            }
            int   should_indent_preproc = true;
            int   should_ignore_preproc = false;
            Chunk *preproc_next         = pc->GetNextNl();
            preproc_next = preproc_next->GetNextNcNnlNet();

            /* Look ahead at what's on the line after the #if */
            log_rule_B("pp_indent_brace");
            log_rule_B("pp_indent_func_def");
            log_rule_B("pp_indent_case");
            log_rule_B("pp_indent_extern");

            while (  preproc_next->IsNotNullChunk()
                  && preproc_next->IsNot(CT_NEWLINE))
            {
               if (  (preproc_next->Is(CT_BRACE_OPEN))
                  || (preproc_next->Is(CT_BRACE_CLOSE)))
               {
                  if (options::pp_indent_brace() == 0)
                  {
                     should_indent_preproc = false;
                     break;
                  }
                  else if (options::pp_indent_brace() == -1)
                  {
                     should_ignore_preproc = true;
                     break;
                  }
               }
               else if (  (  preproc_next->Is(CT_FUNC_DEF)
                          && !options::pp_indent_func_def())
                       || (  preproc_next->Is(CT_CASE)
                          && !options::pp_indent_case())
                       || (  preproc_next->Is(CT_EXTERN)
                          && !options::pp_indent_extern()))
               {
                  should_indent_preproc = false;
                  break;
               }
               preproc_next = preproc_next->GetNext();
            }

            if (should_indent_preproc)
            {
               // Hack to get the logs to look right

               const E_Token memtype = next->type;
               set_chunk_type(next, CT_PP_IF_INDENT);
               frm.push(next, __func__, __LINE__);
               set_chunk_type(next, memtype);

               if (should_ignore_preproc)
               {
                  // Preserve original indentation
                  frm.top().indent = pc->GetNextNl()->GetNext()->orig_col;
                  log_indent();
               }
               else
               {
                  // Indent one level except if the #if is a #include guard
                  size_t extra = (  pc->pp_level == 0
                                 && ifdef_over_whole_file())
                                 ? 0 : indent_size;

                  frm.top().indent = frm.prev().indent + extra;
                  log_indent();

                  frm.top().indent_tab = frm.prev().indent_tab + extra;
               }
               frm.top().indent_tmp = frm.top().indent;
               frm.top().in_preproc = false;
               log_indent_tmp();
            }
         }
         log_rule_B("indent_member_single");

         if (options::indent_member_single())
         {
            if (get_chunk_parent_type(pc) == CT_PP_IF)
            {
               // do nothing
            }
            else if (get_chunk_parent_type(pc) == CT_PP_ELSE)
            {
               if (  frm.top().type == CT_MEMBER
                  && frm.top().pop_pc->IsNotNullChunk()
                  && frm.top().pc != frmbkup.top().pc)
               {
                  Chunk *tmp = pc->GetNextNcNnlNpp();

                  if (tmp->IsNotNullChunk())
                  {
                     if (  tmp->Is(CT_WORD)
                        || tmp->Is(CT_TYPE))
                     {
                        tmp = pc->GetNextNcNnlNpp();
                     }
                     else if (  tmp->Is(CT_FUNC_CALL)
                             || tmp->Is(CT_FPAREN_OPEN))
                     {
                        tmp = tmp->GetNextType(CT_FPAREN_CLOSE, tmp->level);

                        if (tmp->IsNotNullChunk())
                        {
                           tmp = pc->GetNextNcNnlNpp();
                        }
                     }

                     if (tmp->IsNotNullChunk())
                     {
                        frm.top().pop_pc = tmp;
                     }
                  }
               }
            }
            else if (get_chunk_parent_type(pc) == CT_PP_ENDIF)
            {
               if (  frmbkup.top().type == CT_MEMBER
                  && frm.top().type == CT_MEMBER)
               {
                  frm.top().pop_pc = frmbkup.top().pop_pc;
               }
            }
         }
         // Transition into a preproc by creating a dummy indent
         Chunk *pp_next = pc->GetNext();

         if (pp_next->IsNullChunk())
         {
            return;
         }
         frm.push(pp_next, __func__, __LINE__);

         if (  get_chunk_parent_type(pc) == CT_PP_DEFINE
            || get_chunk_parent_type(pc) == CT_PP_UNDEF)
         {
            log_rule_B("pp_define_at_level");
            frm.top().indent_tmp = options::pp_define_at_level()
                                   ? frm.prev().indent_tmp : 1;
            frm.top().indent = frm.top().indent_tmp + indent_size;
            log_indent();

            frm.top().indent_tab = frm.top().indent;
            log_indent_tmp();
         }
         else if (  (  get_chunk_parent_type(pc) == CT_PP_PRAGMA
                    || get_chunk_parent_type(pc) == CT_PP_OTHER)
                 && options::pp_define_at_level())
         {
            log_rule_B("pp_define_at_level");
            frm.top().indent_tmp = frm.prev().indent_tmp;
            frm.top().indent     = frm.top().indent_tmp + indent_size;
            log_indent();

            frm.top().indent_tab = frm.top().indent;
            log_indent_tmp();
         }
         else if (  get_chunk_parent_type(pc) == CT_PP_INCLUDE
                 && options::pp_include_at_level())
         {
            log_rule_B("pp_include_at_level");
            frm.top().indent_tmp = frm.prev().indent_tmp;
            frm.top().indent     = frm.top().indent_tmp + indent_size;
            log_indent();

            frm.top().indent_tab = frm.top().indent;
            log_indent_tmp();
         }
         else
         {
            if (  (frm.prev().type == CT_PP_REGION_INDENT)
               || (  (frm.prev().type == CT_PP_IF_INDENT)
                  && (frm.top().type != CT_PP_ENDIF)))
            {
               frm.top().indent = frm.prev(2).indent;
               log_indent();
            }
            else
            {
               frm.top().indent = frm.prev().indent;
               log_indent();
            }
            log_indent();


            auto val = 0;

            if (  get_chunk_parent_type(pc) == CT_PP_REGION
               || get_chunk_parent_type(pc) == CT_PP_ENDREGION)
            {
               log_rule_B("pp_indent_region");
               val = options::pp_indent_region();
               log_indent();
            }
            else if (  get_chunk_parent_type(pc) == CT_PP_IF
                    || get_chunk_parent_type(pc) == CT_PP_ELSE
                    || get_chunk_parent_type(pc) == CT_PP_ENDIF)
            {
               log_rule_B("pp_indent_if");
               val = options::pp_indent_if();
               log_indent();
            }

            if (val != 0)
            {
               auto &indent = frm.top().indent;

               indent = (val > 0) ? val                     // reassign if positive val,
                        : (cast_abs(indent, val) < indent)  // else if no underflow
                        ? (indent + val) : 0;               // reduce, else 0
            }
            frm.top().indent_tmp = frm.top().indent;
            log_indent_tmp();
         }
      }
      // Check for close XML tags "</..."
      log_rule_B("indent_xml_string");

      if (options::indent_xml_string() > 0)
      {
         if (pc->Is(CT_STRING))
         {
            if (  pc->Len() > 4
               && xml_indent > 0
               && pc->str[1] == '<'
               && pc->str[2] == '/')
            {
               log_rule_B("indent_xml_string");
               xml_indent -= options::indent_xml_string();
            }
         }
         else if (!pc->IsCommentOrNewline())
         {
            xml_indent = 0;
         }
      }
      // Handle non-brace closures
      log_indent_tmp();

      bool   token_used = false;
      size_t old_frm_size;

      do
      {
         old_frm_size = frm.size();

         // End anything that drops a level
         if (  !pc->IsCommentOrNewline()
            && frm.top().level > pc->level)
         {
            LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
            frm.pop(__func__, __LINE__, pc);
         }

         if (frm.top().level >= pc->level)
         {
            // process virtual braces closes (no text output)
            if (  pc->Is(CT_VBRACE_CLOSE)
               && frm.top().type == CT_VBRACE_OPEN)
            {
               LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
               frm.pop(__func__, __LINE__, pc);
               pc = pc->GetNext();
               LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));

               if (pc->IsNullChunk())
               {
                  // need to break out of both the do and while loops
                  goto null_pc;
               }
            }

            if (  pc->Is(CT_BRACE_CLOSE)
               && get_chunk_parent_type(pc) == CT_ENUM)
            {
               Chunk *prev_ncnl = pc->GetPrevNcNnl();
               LOG_FMT(LINDLINE, "%s(%d): prev_ncnl is '%s', prev_ncnl->orig_line is %zu, prev_ncnl->orig_col is %zu\n",
                       __func__, __LINE__, prev_ncnl->Text(), prev_ncnl->orig_line, prev_ncnl->orig_col);

               if (prev_ncnl->Is(CT_COMMA))
               {
                  LOG_FMT(LINDLINE, "%s(%d): prev_ncnl is comma\n", __func__, __LINE__);
               }
               else
               {
                  LOG_FMT(LINDLINE, "%s(%d): prev_ncnl is NOT comma\n", __func__, __LINE__);
               }
            }

            // End any assign operations with a semicolon on the same level
            if (is_end_of_assignment(pc, frm))
            {
               LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
               frm.pop(__func__, __LINE__, pc);
            }
            // Pop Colon from stack in ternary operator
            // a
            // ? b
            // : e/*top*/;/*pc*/
            log_rule_B("indent_inside_ternary_operator");

            if (  options::indent_inside_ternary_operator()
               && (frm.top().type == CT_COND_COLON)
               && (  pc->IsSemicolon()
                  || pc->Is(CT_COMMA)
                  || pc->Is(CT_OC_MSG_NAME)
                  || pc->Is(CT_SPAREN_CLOSE))) // Issue #1130, #1715
            {
               LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
               frm.pop(__func__, __LINE__, pc);
            }

            // End any assign operations with a semicolon on the same level
            if (  pc->IsSemicolon()
               && (  (frm.top().type == CT_IMPORT)
                  || (frm.top().type == CT_USING)))
            {
               LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
               frm.pop(__func__, __LINE__, pc);
            }

            // End any custom macro-based open/closes
            if (  !token_used
               && (frm.top().type == CT_MACRO_OPEN)
               && pc->Is(CT_MACRO_CLOSE))
            {
               token_used = true;
               LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
               frm.pop(__func__, __LINE__, pc);
            }

            // End any CPP/ObjC class colon stuff
            if (  (  (frm.top().type == CT_CLASS_COLON)
                  || (frm.top().type == CT_CONSTR_COLON))
               && (  pc->Is(CT_BRACE_OPEN)
                  || pc->Is(CT_OC_END)
                  || pc->Is(CT_OC_SCOPE)
                  || pc->Is(CT_OC_PROPERTY)
                  || pc->Is(CT_TYPEDEF) // Issue #2675
                  || pc->Is(CT_MACRO_OPEN)
                  || pc->Is(CT_MACRO_CLOSE)
                  || (  language_is_set(LANG_OC)
                     && pc->IsComment()
                     && get_chunk_parent_type(pc) == CT_COMMENT_WHOLE) // Issue #2675
                  || pc->IsSemicolon()))
            {
               LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
               frm.pop(__func__, __LINE__, pc);
            }

            // End ObjC class colon stuff inside of generic definition (like Test<T1: id<T3>>)
            if (  (frm.top().type == CT_CLASS_COLON)
               && pc->Is(CT_ANGLE_CLOSE)
               && get_chunk_parent_type(pc) == CT_OC_GENERIC_SPEC)
            {
               LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
               frm.pop(__func__, __LINE__, pc);
            }

            // End Objc nested message and boxed array
            // TODO: ideally formatting would know which opens occurred on a line and group closes in the same manor
            if (  language_is_set(LANG_OC)
               && pc->Is(CT_SQUARE_CLOSE)
               && get_chunk_parent_type(pc) == CT_OC_AT
               && frm.top().level >= pc->level)
            {
               size_t count = 1;
               Chunk  *next = pc->GetNextNc();

               while (  next->IsNotNullChunk()
                     && (  (  next->Is(CT_BRACE_CLOSE)
                           && get_chunk_parent_type(next) == CT_OC_AT)
                        || (  next->Is(CT_SQUARE_CLOSE)
                           && get_chunk_parent_type(next) == CT_OC_AT)
                        || (  next->Is(CT_SQUARE_CLOSE)
                           && get_chunk_parent_type(next) == CT_OC_MSG)))
               {
                  count++;
                  next = next->GetNextNc();
               }
               count = std::min(count, frm.size());

               if (count > 0)
               {
                  while (count-- > 0)
                  {
                     if (frm.top().type == CT_SQUARE_OPEN)
                     {
                        if (frm.paren_count == 0)
                        {
                           fprintf(stderr, "%s(%d): frm.paren_count is ZERO, cannot be decremented, at line %zu, column %zu\n",
                                   __func__, __LINE__, pc->orig_line, pc->orig_col);
                           log_flush(true);
                           exit(EX_SOFTWARE);
                        }
                        frm.paren_count--;
                     }
                     LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                             __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
                     frm.pop(__func__, __LINE__, pc);
                  }

                  if (next)
                  {
                     // End any assign operations with a semicolon on the same level
                     if (is_end_of_assignment(next, frm))
                     {
                        LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                                __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
                        frm.pop(__func__, __LINE__, pc);
                     }
                  }
                  // Indent the brace to match outer most brace/square
                  indent_column_set(frm.top().indent_tmp);
                  continue;
               }
            }

            // a case is ended with another case or a close brace
            if (  (frm.top().type == CT_CASE)
               && (  pc->Is(CT_BRACE_CLOSE)
                  || pc->Is(CT_CASE)))
            {
               LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
               frm.pop(__func__, __LINE__, pc);
            }

            if (frm.top().pop_pc->IsNotNullChunk())
            {
               LOG_FMT(LINDLINE, "%s(%d): pop_pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                       __func__, __LINE__, frm.top().pop_pc->orig_line, frm.top().pop_pc->orig_col,
                       frm.top().pop_pc->Text(), get_token_name(frm.top().pop_pc->type));
            }
            LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));

            if (  (frm.top().type == CT_MEMBER)
               && frm.top().pop_pc == pc)
            {
               LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
               frm.pop(__func__, __LINE__, pc);
            }

            if (  (frm.top().type == CT_LAMBDA)
               && (  pc->Is(CT_SEMICOLON)
                  || pc->Is(CT_COMMA)
                  || pc->Is(CT_BRACE_OPEN)))
            {
               LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
               frm.pop(__func__, __LINE__, pc);
            }
            // a class scope is ended with another class scope or a close brace
            log_rule_B("indent_access_spec_body");

            if (  options::indent_access_spec_body()
               && (frm.top().type == CT_ACCESS)
               && (  pc->Is(CT_BRACE_CLOSE)
                  || pc->Is(CT_ACCESS)))
            {
               LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
               frm.pop(__func__, __LINE__, pc);
            }

            // return & throw are ended with a semicolon
            if (  pc->IsSemicolon()
               && (  (frm.top().type == CT_RETURN)
                  || (frm.top().type == CT_THROW)))
            {
               LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
               frm.pop(__func__, __LINE__, pc);
            }

            // an OC SCOPE ('-' or '+') ends with a semicolon or brace open
            if (  (frm.top().type == CT_OC_SCOPE)
               && (  pc->IsSemicolon()
                  || pc->Is(CT_BRACE_OPEN)))
            {
               LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
               frm.pop(__func__, __LINE__, pc);
            }

            /*
             * a typedef and an OC SCOPE ('-' or '+') ends with a semicolon or
             * brace open
             */
            if (  (frm.top().type == CT_TYPEDEF)
               && (  pc->IsSemicolon()
                  || pc->IsParenOpen()
                  || pc->Is(CT_BRACE_OPEN)))
            {
               LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
               frm.pop(__func__, __LINE__, pc);
            }

            // an SQL EXEC is ended with a semicolon
            if (  (frm.top().type == CT_SQL_EXEC)
               && pc->IsSemicolon())
            {
               LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
               frm.pop(__func__, __LINE__, pc);
            }

            // an CLASS is ended with a semicolon or brace open
            if (  (frm.top().type == CT_CLASS)
               && (  pc->Is(CT_CLASS_COLON)
                  || pc->Is(CT_BRACE_OPEN)
                  || pc->IsSemicolon()))
            {
               LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
               frm.pop(__func__, __LINE__, pc);
            }
            log_rule_B("indent_oc_inside_msg_sel");

            // Pop OC msg selector stack
            if (  options::indent_oc_inside_msg_sel()
               && (frm.top().type != CT_SQUARE_OPEN)
               && frm.top().level >= pc->level
               && (  pc->Is(CT_OC_MSG_FUNC)
                  || pc->Is(CT_OC_MSG_NAME))) // Issue #2658
            {
               LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
               frm.pop(__func__, __LINE__, pc);
            }

            // Close out parenthesis and squares
            if (  (frm.top().type == (pc->type - 1))
               && (  pc->Is(CT_PAREN_CLOSE)
                  || pc->Is(CT_LPAREN_CLOSE)                     // Issue #3054
                  || pc->Is(CT_SPAREN_CLOSE)
                  || pc->Is(CT_FPAREN_CLOSE)
                  || pc->Is(CT_SQUARE_CLOSE)
                  || pc->Is(CT_ANGLE_CLOSE)))
            {
               LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
               frm.pop(__func__, __LINE__, pc);

               if (frm.paren_count == 0)
               {
                  fprintf(stderr, "%s(%d): frm.paren_count is ZERO, cannot be decremented, at line %zu, column %zu\n",
                          __func__, __LINE__, pc->orig_line, pc->orig_col);
                  log_flush(true);
                  exit(EX_SOFTWARE);
               }
               frm.paren_count--;
            }
         }
      } while (old_frm_size > frm.size());

      // Grab a copy of the current indent
      indent_column_set(frm.top().indent_tmp);                        // Issue #3294
      log_indent_tmp();

      log_rule_B("indent_single_newlines");

      if (  pc->Is(CT_NEWLINE)
         && options::indent_single_newlines())
      {
         pc->nl_column = indent_column;
      }

      if (  !pc->IsCommentOrNewline()
         && log_sev_on(LINDPC))
      {
         LOG_FMT(LINDPC, "%s(%d):\n", __func__, __LINE__);
         LOG_FMT(LINDPC, "   -=[ pc->orig_line is %zu, orig_col is %zu, Text() is '%s' ]=-, frm.size() is %zu\n",
                 pc->orig_line, pc->orig_col, pc->Text(), frm.size());

         for (size_t ttidx = frm.size() - 1; ttidx > 0; ttidx--)
         {
            LOG_FMT(LINDPC, "     [%zu %zu:%zu '%s' %s/%s tmp=%zu indent=%zu brace_indent=%zu indent_tab=%zu indent_cont=%d level=%zu pc->brace_level=%zu]\n",
                    ttidx,
                    frm.at(ttidx).pc->orig_line,
                    frm.at(ttidx).pc->orig_col,
                    frm.at(ttidx).pc->Text(),
                    get_token_name(frm.at(ttidx).type),
                    get_token_name(frm.at(ttidx).pc->parent_type),
                    frm.at(ttidx).indent_tmp,
                    frm.at(ttidx).indent,
                    frm.at(ttidx).brace_indent,
                    frm.at(ttidx).indent_tab,
                    frm.at(ttidx).indent_cont,
                    frm.at(ttidx).level,
                    frm.at(ttidx).pc->brace_level);
         }
      }
      char copy[1000];
      LOG_FMT(LINDENT2, "%s(%d): orig_line is %zu, orig_col is %zu, column is %zu, Text() is '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->column, pc->ElidedText(copy));

      // Issue #672
      if (  pc->Is(CT_BRACE_OPEN)
         && classFound)
      {
         LOG_FMT(LINDENT, "%s(%d): CT_BRACE_OPEN found, CLOSE IT\n",
                 __func__, __LINE__);
         LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
         classFound = false;
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
      log_rule_B("indent_braces");
      log_rule_B("indent_braces_no_func");
      log_rule_B("indent_braces_no_class");
      log_rule_B("indent_braces_no_struct");
      const bool brace_indent = (  (  pc->Is(CT_BRACE_CLOSE)
                                   || pc->Is(CT_BRACE_OPEN))
                                && options::indent_braces()
                                && (  !options::indent_braces_no_func()
                                   || get_chunk_parent_type(pc) != CT_FUNC_DEF)
                                && (  !options::indent_braces_no_func()
                                   || get_chunk_parent_type(pc) != CT_FUNC_CLASS_DEF)
                                && (  !options::indent_braces_no_class()
                                   || get_chunk_parent_type(pc) != CT_CLASS)
                                && (  !options::indent_braces_no_struct()
                                   || get_chunk_parent_type(pc) != CT_STRUCT));
      LOG_FMT(LINDENT, "%s(%d): brace_indent is %s\n",
              __func__, __LINE__, brace_indent ? "TRue" : "FAlse");

      if (pc->Is(CT_BRACE_CLOSE))
      {
         if (language_is_set(LANG_OC))
         {
            if (  frm.top().type == CT_BRACE_OPEN
               && frm.top().level >= pc->level)
            {
               size_t count = 1;
               Chunk  *next = pc->GetNextNc();

               while (  next->IsNotNullChunk()
                     && (  (  next->Is(CT_BRACE_CLOSE)
                           && get_chunk_parent_type(next) == CT_OC_AT)
                        || (  next->Is(CT_SQUARE_CLOSE)
                           && get_chunk_parent_type(next) == CT_OC_AT)
                        || (  next->Is(CT_SQUARE_CLOSE)
                           && get_chunk_parent_type(next) == CT_OC_MSG)))
               {
                  count++;
                  next = next->GetNextNc();
               }
               count = std::min(count, frm.size());

               // End Objc nested boxed dictionary
               // TODO: ideally formatting would know which opens occurred on a line and group closes in the same manor
               if (  count > 0
                  && pc->Is(CT_BRACE_CLOSE)
                  && get_chunk_parent_type(pc) == CT_OC_AT)
               {
                  if (frm.top().ip.ref)
                  {
                     pc->indent.ref   = frm.top().ip.ref;
                     pc->indent.delta = 0;
                  }

                  while (count-- > 0)
                  {
                     LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                             __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
                     frm.pop(__func__, __LINE__, pc);
                  }

                  if (next)
                  {
                     // End any assign operations with a semicolon on the same level
                     if (is_end_of_assignment(next, frm))
                     {
                        LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                                __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
                        frm.pop(__func__, __LINE__, pc);
                     }
                  }

                  // Indent the brace to match outer most brace/square
                  if (frm.top().indent_cont)
                  {
                     indent_column_set(frm.top().indent_tmp - indent_size);
                  }
                  else
                  {
                     indent_column_set(frm.top().indent_tmp);
                  }
               }
               else
               {
                  // Indent the brace to match the open brace
                  indent_column_set(frm.top().brace_indent);

                  if (frm.top().ip.ref)
                  {
                     pc->indent.ref   = frm.top().ip.ref;
                     pc->indent.delta = 0;
                  }
                  LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                          __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
                  frm.pop(__func__, __LINE__, pc);
               }
            }
         }
         else if (frm.top().brace_indent) // Issue #3421
         {
            // Indent the brace to match the open brace
            indent_column_set(frm.top().brace_indent);

            if (frm.top().ip.ref)
            {
               pc->indent.ref   = frm.top().ip.ref;
               pc->indent.delta = 0;
            }
            LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
            frm.pop(__func__, __LINE__, pc);
         }
      }
      else if (pc->Is(CT_VBRACE_OPEN))
      {
         frm.push(pc, __func__, __LINE__);

         log_rule_B("indent_min_vbrace_open");
         size_t iMinIndent = options::indent_min_vbrace_open();

         if (indent_size > iMinIndent)
         {
            iMinIndent = indent_size;
         }
         size_t iNewIndent = frm.prev().indent + iMinIndent;

         log_rule_B("indent_vbrace_open_on_tabstop");

         if (options::indent_vbrace_open_on_tabstop())
         {
            iNewIndent = next_tab_column(iNewIndent);
         }
         frm.top().indent = iNewIndent;
         log_indent();
         frm.top().indent_tmp = frm.top().indent;
         frm.top().indent_tab = frm.top().indent;
         log_indent_tmp();

         // Always indent on virtual braces
         indent_column_set(frm.top().indent_tmp);
      }
      else if (  pc->Is(CT_BRACE_OPEN)
              && (  pc->next != nullptr
                 && pc->GetNext()->IsNot(CT_NAMESPACE)))
      {
         LOG_FMT(LINDENT2, "%s(%d): orig_line is %zu, orig_col is %zu, Text() is '%s'\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
         frm.push(pc, __func__, __LINE__);

         log_rule_B("indent_macro_brace");

         if (  !options::indent_macro_brace()
            && frm.prev().type == CT_PP_DEFINE
            && frm.prev().open_line == frm.top().open_line)
         {
            LOG_FMT(LINDENT2, "%s(%d): indent_macro_brace\n", __func__, __LINE__);
         }
         else if (  options::indent_cpp_lambda_body()
                 && get_chunk_parent_type(pc) == CT_CPP_LAMBDA)
         {
            log_rule_B("indent_cpp_lambda_body");
            frm.top().brace_indent = frm.prev().indent;

            Chunk *head     = frm.top().pc->GetPrevNcNnlNpp();
            Chunk *tail     = Chunk::NullChunkPtr;
            Chunk *frm_prev = frm.prev().pc;
            bool  enclosure = (  frm_prev->parent_type != CT_FUNC_DEF           // Issue #3407
                              && frm_prev != frm_prev->SkipToMatch());
            bool  linematch = true;

            for (auto it = frm.rbegin(); it != frm.rend() && tail->IsNullChunk(); ++it)
            {
               if (it->pc && it->pc != frm.top().pc)
               {
                  linematch &= it->pc->IsOnSameLine(head);
               }
               Chunk *match = it->pc->SkipToMatch();

               if (match->IsNullChunk())
               {
                  continue;
               }
               Chunk *target = match->GetNextNcNnlNpp();

               while (  tail->IsNullChunk()
                     && target->IsNotNullChunk())
               {
                  if (  target->IsSemicolon()
                     && target->level == match->level)
                  {
                     tail = target;
                  }
                  else if (target->level < match->level)
                  {
                     break;
                  }
                  else
                  {
                     target = target->GetNextNcNnlNpp();
                  }
               }
            }

            bool toplevel = true;

            for (auto it = frm.rbegin(); it != frm.rend() && tail->IsNotNullChunk(); ++it)
            {
               if (!it->pc->Is(CT_FPAREN_OPEN))
               {
                  continue;
               }

               if (it->pc->level < tail->level)
               {
                  toplevel = false;
                  break;
               }
            }

            // Issues: #1813, #3409, #3428
            // if enclosure is set, the namespace is already accounted for
            // so don't apply the correction twice.
            // And for some reason, it's also accounted for when
            // indent_namespace_single_indent is set
            if (!enclosure && !options::indent_namespace_single_indent())
            {
               size_t namespace_indent_to_ignore = 0;
               log_rule_B("indent_namespace");

               for (auto i = frm.rbegin(); i != frm.rend(); ++i)
               {
                  if (i->ns_cnt)
                  {
                     const auto foo = i->ns_cnt;
                     namespace_indent_to_ignore = indent_size * foo;
                     break;
                  }
               }

               if (namespace_indent_to_ignore && options::indent_namespace())
               {
                  // I honestly don't know what's going on, so this is an
                  // empirical fix. For some reason lambdas don't have
                  // their indent calculated properly when indent_namespace
                  // is true. But only if they are not in enclosures.
                  namespace_indent_to_ignore = indent_size;
               }

               if (namespace_indent_to_ignore <= frm.top().brace_indent)
               {
                  frm.top().brace_indent -= namespace_indent_to_ignore;
               }
               else
               {
                  frm.top().brace_indent = 1;
               }
            }
            // A few things to check:
            // 1. The matching brace is on the same line as the ending semicolon
            // 2a. If it's an assignment, check that both sides of the assignment operator are on the same line
            // 2b. If it's inside some closure, check that all the frames are on the same line,
            //     and it is in the top level closure, and indent_continue is non-zero
            bool sameLine = frm.top().pc->SkipToMatch()->IsOnSameLine(tail);

            bool isAssignSameLine =
               !enclosure
               && options::align_assign_span() == 0
               && !options::indent_align_assign()
               && frm.prev().pc->GetPrevNcNnlNpp()->IsOnSameLine(frm.prev().pc)
               && frm.prev().pc->IsOnSameLine(frm.prev().pc->GetNextNcNnlNpp());

            bool closureSameLineTopLevel =
               (options::indent_continue() > 0)
               && enclosure
               && linematch
               && toplevel;

            if (sameLine && ((isAssignSameLine) || (closureSameLineTopLevel)))
            {
               if (indent_size > frm.top().brace_indent)       // if options::indent_indent_columns() is too big
               {
                  frm.top().brace_indent = 1;
               }
               else
               {
                  frm.top().brace_indent -= indent_size;
               }
            }
            indent_column_set(frm.top().brace_indent);
            frm.top().indent = indent_column + indent_size;
            log_indent();

            frm.top().indent_tab = frm.top().indent;
            frm.top().indent_tmp = frm.top().indent;
            log_indent_tmp();

            frm.prev().indent_tmp = frm.top().indent_tmp;
            log_indent_tmp();
         }
         else if (  language_is_set(LANG_CPP)
                 && options::indent_cpp_lambda_only_once()
                 && (get_chunk_parent_type(pc) == CT_CPP_LAMBDA))
         {
            // test example cpp:30756
            log_rule_B("indent_cpp_lambda_only_once");

            size_t namespace_indent_to_ignore = 0;                   // Issue #1813
            log_rule_B("indent_namespace");

            if (!options::indent_namespace())
            {
               for (auto i = frm.rbegin(); i != frm.rend(); ++i)
               {
                  if (i->ns_cnt)
                  {
                     namespace_indent_to_ignore = i->ns_cnt;
                     break;
                  }
               }
            }
            // Issue # 1296
            frm.top().brace_indent = 1 + ((pc->brace_level - namespace_indent_to_ignore) * indent_size);
            indent_column_set(frm.top().brace_indent);
            frm.top().indent = indent_column + indent_size;
            log_indent();
            frm.top().indent_tab = frm.top().indent;
            frm.top().indent_tmp = frm.top().indent;
            log_indent_tmp();

            frm.prev().indent_tmp = frm.top().indent_tmp;
            log_indent_tmp();
         }
         else if (  language_is_set(LANG_CS | LANG_JAVA)
                 && options::indent_cs_delegate_brace()
                 && (  get_chunk_parent_type(pc) == CT_LAMBDA
                    || get_chunk_parent_type(pc) == CT_DELEGATE))
         {
            log_rule_B("indent_cs_delegate_brace");
            frm.top().brace_indent = 1 + ((pc->brace_level + 1) * indent_size);
            indent_column_set(frm.top().brace_indent);
            frm.top().indent = indent_column + indent_size;
            log_indent();
            frm.top().indent_tab = frm.top().indent;
            frm.top().indent_tmp = frm.top().indent;
            log_indent_tmp();

            frm.prev().indent_tmp = frm.top().indent_tmp;
            log_indent_tmp();
         }
         else if (  language_is_set(LANG_CS | LANG_JAVA)
                 && !options::indent_cs_delegate_brace()
                 && !options::indent_align_paren()
                 && (  get_chunk_parent_type(pc) == CT_LAMBDA
                    || get_chunk_parent_type(pc) == CT_DELEGATE))
         {
            log_rule_B("indent_cs_delegate_brace");
            log_rule_B("indent_align_paren");
            frm.top().brace_indent = frm.prev().indent;

            // Issue # 1620, UNI-24090.cs
            if (frm.prev().pc->IsOnSameLine(frm.top().pc->GetPrevNcNnlNpp()))
            {
               frm.top().brace_indent -= indent_size;
            }
            indent_column_set(frm.top().brace_indent);
            frm.top().indent = indent_column + indent_size;
            log_indent();
            frm.top().indent_tab = frm.top().indent;
            frm.top().indent_tmp = frm.top().indent;
            log_indent_tmp();
            frm.prev().indent_tmp = frm.top().indent_tmp;
            log_indent_tmp();
         }
         else if (  !options::indent_paren_open_brace()
                 && !language_is_set(LANG_CS)
                 && get_chunk_parent_type(pc) == CT_CPP_LAMBDA
                 && (  pc->flags.test(PCF_IN_FCN_DEF)
                    || pc->flags.test(PCF_IN_FCN_CTOR)) // Issue #2152
                 && pc->GetNextNc()->IsNewline())
         {
            log_rule_B("indent_paren_open_brace");
            // Issue #1165
            LOG_FMT(LINDENT2, "%s(%d): orig_line is %zu, pc->brace_level is %zu, for '%s', pc->level is %zu, pc(-1)->level is %zu\n",
                    __func__, __LINE__, pc->orig_line, pc->brace_level, pc->Text(), pc->level, frm.prev().pc->level);
            frm.top().brace_indent = 1 + ((pc->brace_level + 1) * indent_size);
            indent_column_set(frm.top().brace_indent);
            frm.top().indent = frm.prev().indent_tmp;
            log_indent();

            frm.top().indent_tmp = frm.top().indent;
            log_indent_tmp();
         }
         // any '{' that is inside of a '(' overrides the '(' indent
         // only to help the vim command }
         else if (  !options::indent_paren_open_brace()
                 && frm.prev().pc->IsParenOpen()
                 && pc->GetNextNc()->IsNewline())
         {
            log_rule_B("indent_paren_open_brace");
            LOG_FMT(LINDENT2, "%s(%d): orig_line is %zu, pc->brace_level is %zu, for '%s', pc->level is %zu, pc(-1)->level is %zu\n",
                    __func__, __LINE__, pc->orig_line, pc->brace_level, pc->Text(), pc->level, frm.prev().pc->level);
            // FIXME: I don't know how much of this is necessary, but it seems to work
            frm.top().brace_indent = 1 + (pc->brace_level * indent_size);
            indent_column_set(frm.top().brace_indent);
            frm.top().indent = indent_column + indent_size;
            log_indent();

            if (  (get_chunk_parent_type(pc) == CT_OC_BLOCK_EXPR)
               && pc->flags.test(PCF_IN_OC_MSG))
            {
               frm.top().indent = frm.prev().indent_tmp + indent_size;
               log_indent();
               frm.top().brace_indent = frm.prev().indent_tmp;
               indent_column_set(frm.top().brace_indent);
            }
            frm.top().indent_tab = frm.top().indent;
            frm.top().indent_tmp = frm.top().indent;
            log_indent_tmp();

            frm.prev().indent_tmp = frm.top().indent_tmp;
         }
         else if (frm.paren_count != 0)
         {
            if (frm.top().pc->parent_type == CT_OC_BLOCK_EXPR)
            {
               log_rule_B("indent_oc_block_msg");

               if (  pc->flags.test(PCF_IN_OC_MSG)
                  && options::indent_oc_block_msg())
               {
                  frm.top().ip.ref = oc_msg_block_indent(pc, false, false, false, true);
                  log_rule_B("indent_oc_block_msg");
                  frm.top().ip.delta = options::indent_oc_block_msg();
               }
               log_rule_B("indent_oc_block");
               log_rule_B("indent_oc_block_msg_xcode_style");

               if (  options::indent_oc_block()
                  || options::indent_oc_block_msg_xcode_style())
               {
                  bool in_oc_msg = pc->flags.test(PCF_IN_OC_MSG);
                  log_rule_B("indent_oc_block_msg_from_keyword");
                  bool indent_from_keyword = options::indent_oc_block_msg_from_keyword()
                                             && in_oc_msg;
                  log_rule_B("indent_oc_block_msg_from_colon");
                  bool indent_from_colon = options::indent_oc_block_msg_from_colon()
                                           && in_oc_msg;
                  log_rule_B("indent_oc_block_msg_from_caret");
                  bool indent_from_caret = options::indent_oc_block_msg_from_caret()
                                           && in_oc_msg;
                  log_rule_B("indent_oc_block_msg_from_brace");
                  bool indent_from_brace = options::indent_oc_block_msg_from_brace()
                                           && in_oc_msg;

                  /*
                   * In "Xcode indent mode", we want to indent:
                   *  - if the colon is aligned (namely, if a newline has been
                   *    added before it), indent_from_brace
                   *  - otherwise, indent from previous block (the "else" statement here)
                   */
                  log_rule_B("indent_oc_block_msg_xcode_style");

                  if (options::indent_oc_block_msg_xcode_style())
                  {
                     Chunk *bbc           = pc->SkipToMatch(); // block brace close '}'
                     Chunk *bbc_next_ncnl = bbc->GetNextNcNnl();

                     if (  bbc_next_ncnl->type == CT_OC_MSG_NAME
                        || bbc_next_ncnl->type == CT_OC_MSG_FUNC)
                     {
                        indent_from_brace   = false;
                        indent_from_colon   = false;
                        indent_from_caret   = false;
                        indent_from_keyword = true;
                     }
                     else
                     {
                        indent_from_brace   = false;
                        indent_from_colon   = false;
                        indent_from_caret   = false;
                        indent_from_keyword = false;
                     }
                  }
                  Chunk *ref = oc_msg_block_indent(pc, indent_from_brace,
                                                   indent_from_caret,
                                                   indent_from_colon,
                                                   indent_from_keyword);

                  if (ref)
                  {
                     frm.top().indent = indent_size + ref->column;
                  }
                  else
                  {
                     frm.top().indent = 1 + ((pc->brace_level + 1) * indent_size);
                  }
                  log_indent();
                  indent_column_set(frm.top().indent - indent_size);
               }
               else
               {
                  frm.top().indent = frm.prev().indent_tmp + indent_size;
                  log_indent();
               }
            }
            else if (  frm.top().pc->type == CT_BRACE_OPEN
                    && frm.top().pc->parent_type == CT_OC_AT)
            {
               // We are inside @{ ... } -- indent one tab from the paren
               if (frm.prev().indent_cont)
               {
                  frm.top().indent = frm.prev().indent_tmp;
               }
               else
               {
                  frm.top().indent = frm.prev().indent_tmp + indent_size;
               }
               log_indent();
            }
            // Issue # 1620, UNI-24090.cs
            else if (  frm.prev().pc->IsOnSameLine(frm.top().pc)
                    && !options::indent_align_paren()
                    && frm.prev().pc->IsParenOpen()
                    && !pc->flags.test(PCF_ONE_LINER))
            {
               log_rule_B("indent_align_paren");
               // We are inside ({ ... }) -- where { and ( are on the same line, avoiding double indentations.
               // only to help the vim command }
               frm.top().brace_indent = frm.prev().indent - indent_size;
               indent_column_set(frm.top().brace_indent);
               frm.top().indent = frm.prev().indent_tmp;
               log_indent();
            }
            else if (  frm.prev().pc->IsOnSameLine(frm.top().pc->GetPrevNcNnlNpp())
                    && !options::indent_align_paren()
                    && frm.prev().pc->IsParenOpen()
                    && !pc->flags.test(PCF_ONE_LINER))
            {
               log_rule_B("indent_align_paren");
               // We are inside ({ ... }) -- where { and ( are on adjacent lines, avoiding indentation of brace.
               // only to help the vim command }
               frm.top().brace_indent = frm.prev().indent - indent_size;
               indent_column_set(frm.top().brace_indent);
               frm.top().indent = frm.prev().indent_tmp;
               log_indent();
            }
            else if (  options::indent_oc_inside_msg_sel()
                    && (  frm.prev().type == CT_OC_MSG_FUNC
                       || frm.prev().type == CT_OC_MSG_NAME)) // Issue #2658
            {
               log_rule_B("indent_oc_inside_msg_sel");
               // [Class Message:{<here>
               frm.top().indent = frm.prev().pc->column + indent_size;
               log_indent();
               indent_column_set(frm.prev().pc->column);
            }
            else
            {
               // We are inside ({ ... }) -- indent one tab from the paren
               frm.top().indent = frm.prev().indent_tmp + indent_size;

               if (!frm.prev().pc->IsParenOpen())
               {
                  frm.top().indent_tab = frm.top().indent;
               }
               log_indent();
            }
         }
         else if (  frm.top().pc->type == CT_BRACE_OPEN
                 && frm.top().pc->parent_type == CT_OC_AT)
         {
            // We are inside @{ ... } -- indent one tab from the paren
            if (frm.prev().indent_cont)
            {
               frm.top().indent = frm.prev().indent_tmp;
            }
            else
            {
               frm.top().indent     = frm.prev().indent_tmp + indent_size;
               frm.top().indent_tab = frm.top().indent;
            }
            log_indent();
         }
         else if (  (  get_chunk_parent_type(pc) == CT_BRACED_INIT_LIST
                    || (  !options::indent_compound_literal_return()
                       && get_chunk_parent_type(pc) == CT_C_CAST))
                 && frm.prev().type == CT_RETURN)
         {
            log_rule_B("indent_compound_literal_return");

            // we're returning either a c compound literal (CT_C_CAST) or a
            // C++11 initialization list (CT_BRACED_INIT_LIST), use indent from the return.
            if (frm.prev().indent_cont)
            {
               frm.top().indent = frm.prev().indent_tmp;
            }
            else
            {
               frm.top().indent = frm.prev().indent_tmp + indent_size;
            }
            log_indent();
         }
         else
         {
            // Use the prev indent level + indent_size.
            if (get_chunk_parent_type(pc) == CT_SWITCH)
            {
               frm.top().indent = frm.prev().indent + options::indent_switch_body();
            }
            else
            {
               frm.top().indent = frm.prev().indent + indent_size;
            }
            LOG_FMT(LINDLINE, "%s(%d): frm.pse_tos is %zu, ... indent is %zu\n",
                    __func__, __LINE__, frm.size() - 1, frm.top().indent);
            LOG_FMT(LINDLINE, "%s(%d): orig_line is %zu, orig_col is %zu, Text() is '%s', parent_type is %s\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(),
                    get_token_name(get_chunk_parent_type(pc)));

            // If this brace is part of a statement, bump it out by indent_brace
            if (  get_chunk_parent_type(pc) == CT_IF
               || get_chunk_parent_type(pc) == CT_ELSE
               || get_chunk_parent_type(pc) == CT_ELSEIF
               || get_chunk_parent_type(pc) == CT_TRY
               || get_chunk_parent_type(pc) == CT_CATCH
               || get_chunk_parent_type(pc) == CT_DO
               || get_chunk_parent_type(pc) == CT_WHILE
               || get_chunk_parent_type(pc) == CT_USING_STMT
               || get_chunk_parent_type(pc) == CT_SWITCH
               || get_chunk_parent_type(pc) == CT_SYNCHRONIZED
               || get_chunk_parent_type(pc) == CT_FOR)
            {
               if (parent_token_indent != 0)
               {
                  frm.top().indent += parent_token_indent - indent_size;
                  log_indent();
               }
               else
               {
                  log_rule_B("indent_brace");
                  frm.top().indent += options::indent_brace();
                  log_indent();
                  indent_column_set(indent_column + options::indent_brace());
               }
            }
            else if (get_chunk_parent_type(pc) == CT_CASE)
            {
               if (options::indent_ignore_case_brace())
               {
                  log_rule_B("indent_ignore_case_brace");
                  indent_column_set(pc->orig_col);
               }
               else
               {
                  log_rule_B("indent_case_brace");
                  const auto tmp_indent = static_cast<int>(frm.prev().indent)
                                          - static_cast<int>(indent_size)
                                          + options::indent_case_brace();
                  /*
                   * An open brace with the parent of case does not indent by default
                   * UO_indent_case_brace can be used to indent the brace.
                   * So we need to take the CASE indent, subtract off the
                   * indent_size that was added above and then add indent_case_brace.
                   * may take negative value
                   */
                  indent_column_set(max(tmp_indent, 0));
               }
               // Stuff inside the brace still needs to be indented
               frm.top().indent = indent_column + indent_size;
               log_indent();

               frm.top().indent_tmp = frm.top().indent;
               log_indent_tmp();
            }
            else if (  get_chunk_parent_type(pc) == CT_CLASS
                    && !options::indent_class())
            {
               log_rule_B("indent_class");
               LOG_FMT(LINDENT, "%s(%d): orig_line is %zu, orig_col is %zu, text is %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
               frm.top().indent -= indent_size;
               log_indent();
            }
            else if (get_chunk_parent_type(pc) == CT_NAMESPACE)
            {
               frm.top().ns_cnt = frm.prev().ns_cnt + 1;

               log_rule_B("indent_namespace");
               log_rule_B("indent_namespace_single_indent");

               if (  options::indent_namespace()
                  && options::indent_namespace_single_indent())
               {
                  if (frm.top().ns_cnt >= 2)
                  {
                     // undo indent on all except the first namespace
                     frm.top().indent -= indent_size;
                     log_indent();
                  }
                  indent_column_set(frm.prev(frm.top().ns_cnt).indent);
               }
               else if (  options::indent_namespace()
                       && options::indent_namespace_inner_only())
               {
                  if (frm.top().ns_cnt == 1)
                  {
                     // undo indent on first namespace only
                     frm.top().indent -= indent_size;
                     log_indent();
                  }
               }
               else if (  pc->flags.test(PCF_LONG_BLOCK)
                       || !options::indent_namespace())
               {
                  log_rule_B("indent_namespace");
                  // don't indent long blocks
                  frm.top().indent -= indent_size;
                  log_indent();
               }
               else // indenting 'short' namespace
               {
                  log_rule_B("indent_namespace_level");

                  if (options::indent_namespace_level() > 0)
                  {
                     frm.top().indent -= indent_size;
                     log_indent();

                     frm.top().indent +=
                        options::indent_namespace_level();
                     log_indent();
                  }
               }
            }
            else if (  get_chunk_parent_type(pc) == CT_EXTERN
                    && !options::indent_extern())
            {
               log_rule_B("indent_extern");
               frm.top().indent -= indent_size;
               log_indent();
            }
            frm.top().indent_tab = frm.top().indent;
         }

         if (pc->flags.test(PCF_DONT_INDENT))
         {
            frm.top().indent = pc->column;
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
            Chunk *next = pc->GetNextNcNnl();

            if (next->IsNullChunk())
            {
               break;
            }
            Chunk *prev = pc->GetPrev();

            if (  get_chunk_parent_type(pc) == CT_BRACED_INIT_LIST
               && prev->Is(CT_BRACE_OPEN)
               && get_chunk_parent_type(prev) == CT_BRACED_INIT_LIST)
            {
               indent_column = frm.prev().brace_indent;
               frm.top().indent = frm.prev().indent;
               log_indent();
            }
            else if (  !chunk_is_newline_between(pc, next)
                    && get_chunk_parent_type(next) != CT_BRACED_INIT_LIST
                    && options::indent_token_after_brace()
                    && !pc->flags.test(PCF_ONE_LINER)) // Issue #1108
            {
               log_rule_B("indent_token_after_brace");
               frm.top().indent = next->column;
               log_indent();
            }
            frm.top().indent_tmp = frm.top().indent;
            frm.top().open_line  = pc->orig_line;
            log_indent_tmp();

            log_rule_B("Update the indent_column");

            // Update the indent_column if needed
            if (  brace_indent
               || parent_token_indent != 0)
            {
               indent_column_set(frm.top().indent_tmp);
               log_indent_tmp();
            }
         }
         // Save the brace indent
         frm.top().brace_indent = indent_column;
      }
      else if (pc->Is(CT_SQL_END))
      {
         if (frm.top().type == CT_SQL_BEGIN)
         {
            LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
            frm.pop(__func__, __LINE__, pc);
            indent_column_set(frm.top().indent_tmp);
            log_indent_tmp();
         }
      }
      else if (  pc->Is(CT_SQL_BEGIN)
              || pc->Is(CT_MACRO_OPEN)
              || (  pc->Is(CT_CLASS)
                 && language_is_set(LANG_CS))) // Issue #3536
      {
         frm.push(pc, __func__, __LINE__);

         frm.top().indent = frm.prev().indent + indent_size;
         log_indent();

         frm.top().indent_tmp = frm.top().indent;
         frm.top().indent_tab = frm.top().indent;
         log_indent_tmp();
      }
      else if (pc->Is(CT_SQL_EXEC))
      {
         frm.push(pc, __func__, __LINE__);

         frm.top().indent = frm.prev().indent + indent_size;
         log_indent();

         frm.top().indent_tmp = frm.top().indent;
         log_indent_tmp();
      }
      else if (pc->Is(CT_MACRO_ELSE))
      {
         if (frm.top().type == CT_MACRO_OPEN)
         {
            indent_column_set(frm.prev().indent);
         }
      }
      else if (pc->Is(CT_CASE))
      {
         // Start a case - indent UO_indent_switch_case from the switch level
         log_rule_B("indent_switch_case");
         const size_t tmp = frm.top().indent + indent_size
                            - options::indent_switch_body()
                            + options::indent_switch_case();
         frm.push(pc, __func__, __LINE__);

         frm.top().indent = tmp;
         log_indent();

         log_rule_B("indent_case_shift");
         frm.top().indent_tmp = tmp - indent_size + options::indent_case_shift();
         frm.top().indent_tab = tmp;
         log_indent_tmp();

         // Always set on case statements
         indent_column_set(frm.top().indent_tmp);

         if (options::indent_case_comment())
         {
            // comments before 'case' need to be aligned with the 'case'
            Chunk *pct = pc;

            while (  ((pct = pct->GetPrevNnl())->IsNotNullChunk())
                  && pct->IsComment())
            {
               Chunk *t2 = pct->GetPrev();

               if (t2->IsNewline())
               {
                  pct->column        = frm.top().indent_tmp;
                  pct->column_indent = pct->column;
               }
            }
         }
      }
      else if (pc->Is(CT_BREAK))
      {
         Chunk *prev = pc->GetPrevNcNnl();

         if (  prev->Is(CT_BRACE_CLOSE)
            && get_chunk_parent_type(prev) == CT_CASE)
         {
            // issue #663 + issue #1366
            Chunk *prev_prev_newline = pc->GetPrevNl()->GetPrevNl();

            if (prev_prev_newline->IsNotNullChunk())
            {
               // This only affects the 'break', so no need for a stack entry
               indent_column_set(prev_prev_newline->GetNext()->column);
            }
         }
      }
      else if (pc->Is(CT_LABEL))
      {
         if (options::indent_ignore_label())
         {
            log_rule_B("indent_ignore_label");
            indent_column_set(pc->orig_col);
         }
         else
         {
            log_rule_B("indent_label");
            const auto val        = options::indent_label();
            const auto pse_indent = frm.top().indent;

            // Labels get sent to the left or backed up
            if (val > 0)
            {
               indent_column_set(val);

               Chunk *next = pc->GetNext()->GetNext();  // colon + possible statement

               if (  next->IsNotNullChunk()
                  && !next->IsNewline()
                     // label (+ 2, because there is colon and space after it) must fit into indent
                  && (val + static_cast<int>(pc->Len()) + 2 <= static_cast<int>(pse_indent)))
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
      }
      else if (pc->Is(CT_ACCESS))
      {
         log_rule_B("indent_access_spec_body");

         if (options::indent_access_spec_body())
         {
            const size_t tmp = frm.top().indent + indent_size;
            frm.push(pc, __func__, __LINE__);

            frm.top().indent = tmp;
            log_indent();

            frm.top().indent_tmp = tmp - indent_size;
            frm.top().indent_tab = tmp;
            log_indent_tmp();

            /*
             * If we are indenting the body, then we must leave the access spec
             * indented at brace level
             */
            indent_column_set(frm.top().indent_tmp);
            // Issues 1161 + 2704
            // comments before 'access specifier' need to be aligned with the 'access specifier'
            // unless it is a Doxygen comment
            Chunk *pct = pc;

            while (  ((pct = pct->GetPrevNnl())->IsNotNullChunk())
                  && pct->IsComment()
                  && !pct->IsDoxygenComment())
            {
               Chunk *t2 = pct->GetPrev();

               if (t2->IsNewline())
               {
                  pct->column        = frm.top().indent_tmp;
                  pct->column_indent = pct->column;
               }
            }
         }
         else
         {
            // Access spec labels get sent to the left or backed up
            log_rule_B("indent_access_spec");
            const auto val = options::indent_access_spec();

            if (val > 0)
            {
               indent_column_set(val);
            }
            else
            {
               const auto pse_indent   = frm.top().indent;
               const auto no_underflow = cast_abs(pse_indent, val) < pse_indent;

               indent_column_set(no_underflow ? (pse_indent + val) : 0);
            }
         }
      }
      else if (  pc->Is(CT_CLASS_COLON)
              || pc->Is(CT_CONSTR_COLON))
      {
         // just indent one level
         frm.push(pc, __func__, __LINE__);

         frm.top().indent = frm.prev().indent_tmp + indent_size;
         log_indent();

         frm.top().indent_tmp = frm.top().indent;
         frm.top().indent_tab = frm.top().indent;
         log_indent_tmp();

         if (pc->Is(CT_CLASS_COLON))
         {
            if (options::indent_ignore_before_class_colon())
            {
               log_rule_B("indent_ignore_before_class_colon");
               frm.top().indent_tmp = pc->orig_col;
               log_indent_tmp();
            }
            else if (options::indent_before_class_colon() != 0)
            {
               log_rule_B("indent_before_class_colon");
               frm.top().indent_tmp = std::max<ptrdiff_t>(frm.top().indent_tmp + options::indent_before_class_colon(), 0);
               log_indent_tmp();
            }
         }
         indent_column_set(frm.top().indent_tmp);

         log_rule_B("indent_class_colon");

         if (  options::indent_class_colon()
            && pc->Is(CT_CLASS_COLON))
         {
            log_rule_B("indent_class_on_colon");

            if (options::indent_class_on_colon())
            {
               frm.top().indent = pc->column;
               log_indent();
            }
            else
            {
               Chunk *next = pc->GetNext();

               if (  next->IsNotNullChunk()
                  && !next->IsNewline())
               {
                  frm.top().indent = next->column;
                  log_indent();
               }
            }
         }
         else if (pc->Is(CT_CONSTR_COLON))
         {
            if (options::indent_ignore_before_constr_colon())
            {
               log_rule_B("indent_ignore_before_constr_colon");
               frm.top().indent_tmp = pc->orig_col;
               indent_column_set(frm.top().indent_tmp);
            }

            if (options::indent_constr_colon())
            {
               log_rule_B("indent_constr_colon");
               Chunk *prev = pc->GetPrev();

               if (prev->IsNewline())
               {
                  log_rule_B("indent_ctor_init_following");
                  frm.top().indent += options::indent_ctor_init_following();
                  log_indent();
               }
               // TODO: Create a dedicated indent_constr_on_colon?
               log_rule_B("indent_class_on_colon");

               if (options::indent_ctor_init() != 0)
               {
                  log_rule_B("indent_ctor_init");
                  /*
                   * If the std::max() calls were specialized with size_t (the type of the underlying variable),
                   * they would never actually do their job, because size_t is unsigned and therefore even
                   * a "negative" result would be always greater than zero.
                   * Using ptrdiff_t (a standard signed type of the same size as size_t) in order to avoid that.
                   */
                  frm.top().indent = std::max<ptrdiff_t>(frm.top().indent + options::indent_ctor_init(), 0);
                  log_indent();
                  frm.top().indent_tmp = std::max<ptrdiff_t>(frm.top().indent_tmp + options::indent_ctor_init(), 0);
                  frm.top().indent_tab = std::max<ptrdiff_t>(frm.top().indent_tab + options::indent_ctor_init(), 0);
                  log_indent_tmp();
                  indent_column_set(frm.top().indent_tmp);
               }
               else if (options::indent_class_on_colon())
               {
                  frm.top().indent = pc->column;
                  log_indent();
               }
               else
               {
                  Chunk *next = pc->GetNext();

                  if (  next->IsNotNullChunk()
                     && !next->IsNewline())
                  {
                     frm.top().indent = next->column;
                     log_indent();
                  }
               }
            }
         }
      }
      else if (  pc->Is(CT_PAREN_OPEN)
              && (  get_chunk_parent_type(pc) == CT_ASM
                 || (  pc->GetPrevNcNnl()->IsNotNullChunk()
                    && pc->GetPrevNcNnl()->type == CT_ASM))
              && options::indent_ignore_asm_block())
      {
         log_rule_B("indent_ignore_asm_block");
         Chunk *tmp = pc->SkipToMatch();

         int   move = 0;

         if (  pc->GetPrev()->IsNewline()
            && pc->column != indent_column)
         {
            move = indent_column - pc->column;
         }
         else
         {
            move = pc->column - pc->orig_col;
         }

         do
         {
            pc->column = pc->orig_col + move;
            pc         = pc->GetNext();
         } while (pc != tmp);

         reindent_line(pc, indent_column);
      }
      else if (  pc->Is(CT_PAREN_OPEN)
              || pc->Is(CT_LPAREN_OPEN)                     // Issue #3054
              || pc->Is(CT_SPAREN_OPEN)
              || pc->Is(CT_FPAREN_OPEN)
              || pc->Is(CT_SQUARE_OPEN)
              || pc->Is(CT_ANGLE_OPEN))
      {
         /*
          * Open parenthesis and squares - never update indent_column,
          * unless right after a newline.
          */
         frm.push(pc, __func__, __LINE__);

         if (  pc->GetPrev()->IsNewline()
            && pc->column != indent_column
            && !pc->flags.test(PCF_DONT_INDENT))
         {
            LOG_FMT(LINDENT, "%s(%d): orig_line is %zu, indent => %zu, text is '%s'\n",
                    __func__, __LINE__, pc->orig_line, indent_column, pc->Text());
            reindent_line(pc, indent_column);
         }
         frm.top().indent = pc->column + pc->Len();
         log_indent();

         if (  pc->Is(CT_SQUARE_OPEN)
            && language_is_set(LANG_D))
         {
            frm.top().indent_tab = frm.top().indent;
         }
         bool skipped = false;
         log_rule_B("indent_inside_ternary_operator");
         log_rule_B("indent_align_paren");

         if (  options::indent_inside_ternary_operator()
            && (  pc->Is(CT_FPAREN_OPEN)
               || pc->Is(CT_PAREN_OPEN))
            && frm.size() > 2
            && (  frm.prev().type == CT_QUESTION
               || frm.prev().type == CT_COND_COLON)
            && !options::indent_align_paren())
         {
            frm.top().indent = frm.prev().indent_tmp + indent_size;
            log_indent();
            frm.top().indent_tab = frm.top().indent;
            frm.top().indent_tmp = frm.top().indent;
            log_indent_tmp();
         }
         else if (  (  pc->Is(CT_FPAREN_OPEN)
                    || pc->Is(CT_ANGLE_OPEN))
                 && (  (  options::indent_func_call_param()
                       && (  get_chunk_parent_type(pc) == CT_FUNC_CALL
                          || get_chunk_parent_type(pc) == CT_FUNC_CALL_USER))
                    || (  options::indent_func_proto_param()
                       && get_chunk_parent_type(pc) == CT_FUNC_PROTO)
                    || (  options::indent_func_class_param()
                       && (  get_chunk_parent_type(pc) == CT_FUNC_CLASS_DEF
                          || get_chunk_parent_type(pc) == CT_FUNC_CLASS_PROTO))
                    || (  options::indent_template_param()
                       && get_chunk_parent_type(pc) == CT_TEMPLATE)
                    || (  options::indent_func_ctor_var_param()
                       && get_chunk_parent_type(pc) == CT_FUNC_CTOR_VAR)
                    || (  options::indent_func_def_param()
                       && get_chunk_parent_type(pc) == CT_FUNC_DEF)
                    || (  !options::indent_func_def_param()          // Issue #931
                       && get_chunk_parent_type(pc) == CT_FUNC_DEF
                       && options::indent_func_def_param_paren_pos_threshold() > 0
                       && pc->orig_col > options::indent_func_def_param_paren_pos_threshold())))
         {
            log_rule_B("indent_func_call_param");
            log_rule_B("indent_func_proto_param");
            log_rule_B("indent_func_class_param");
            log_rule_B("indent_template_param");
            log_rule_B("indent_func_ctor_var_param");
            log_rule_B("indent_func_def_param");
            log_rule_B("indent_func_def_param_paren_pos_threshold");
            // Skip any continuation indents
            size_t idx = (!frm.empty()) ? frm.size() - 2 : 0;

            while (  (  (  idx > 0
                        && frm.at(idx).type != CT_BRACE_OPEN
                        && frm.at(idx).type != CT_VBRACE_OPEN
                        && frm.at(idx).type != CT_PAREN_OPEN
                        && frm.at(idx).type != CT_FPAREN_OPEN
                        && frm.at(idx).type != CT_SPAREN_OPEN
                        && frm.at(idx).type != CT_SQUARE_OPEN
                        && frm.at(idx).type != CT_ANGLE_OPEN
                        && frm.at(idx).type != CT_CASE
                        && frm.at(idx).type != CT_MEMBER
                        && frm.at(idx).type != CT_QUESTION
                        && frm.at(idx).type != CT_COND_COLON
                        && frm.at(idx).type != CT_LAMBDA
                        && frm.at(idx).type != CT_ASSIGN_NL)
                     || frm.at(idx).pc->IsOnSameLine(frm.top().pc))
                  && (  frm.at(idx).type != CT_CLASS_COLON
                     && frm.at(idx).type != CT_CONSTR_COLON
                     && !(  frm.at(idx).type == CT_LAMBDA
                         && frm.at(idx).pc->GetPrevNc()->type == CT_NEWLINE)))
            {
               if (idx == 0)
               {
                  fprintf(stderr, "%s(%d): idx is ZERO, cannot be decremented, at line %zu, column %zu\n",
                          __func__, __LINE__, pc->orig_line, pc->orig_col);
                  log_flush(true);
                  exit(EX_SOFTWARE);
               }
               idx--;
               skipped = true;
            }
            // PR#381
            log_rule_B("indent_param");

            if (options::indent_param() != 0)
            {
               frm.top().indent = frm.at(idx).indent + options::indent_param();
               log_indent();
            }
            else
            {
               frm.top().indent = frm.at(idx).indent + indent_size;
               log_indent();
            }
            log_rule_B("indent_func_param_double");

            if (options::indent_func_param_double())
            {
               // double is: Use both values of the options indent_columns and indent_param
               frm.top().indent += indent_size;
               log_indent();
            }
            frm.top().indent_tab = frm.top().indent;
         }
         else if (  options::indent_oc_inside_msg_sel()
                 && pc->Is(CT_PAREN_OPEN)
                 && frm.size() > 2
                 && (  frm.prev().type == CT_OC_MSG_FUNC
                    || frm.prev().type == CT_OC_MSG_NAME)
                 && !options::indent_align_paren()) // Issue #2658
         {
            log_rule_B("indent_oc_inside_msg_sel");
            log_rule_B("indent_align_paren");
            // When parens are inside OC messages, push on the parse frame stack
            // [Class Message:(<here>
            frm.top().indent = frm.prev().pc->column + indent_size;
            log_indent();
            frm.top().indent_tab = frm.top().indent;
            frm.top().indent_tmp = frm.top().indent;
            log_indent_tmp();
         }
         else if (  pc->Is(CT_PAREN_OPEN)
                 && !pc->GetNext()->IsNewline()
                 && !options::indent_align_paren()
                 && !pc->flags.test(PCF_IN_SPAREN))
         {
            log_rule_B("indent_align_paren");
            int idx = static_cast<int>(frm.size()) - 2;

            while (  idx > 0
                  && frm.at(idx).pc->IsOnSameLine(frm.top().pc))
            {
               idx--;
               skipped = true;
            }
            frm.top().indent = frm.at(idx).indent + indent_size;
            log_indent();

            frm.top().indent_tab = frm.top().indent;
            skipped = true;
         }
         else if (  (  pc->IsString("(")
                    && !options::indent_paren_nl())
                 || (  pc->IsString("<")
                    && !options::indent_paren_nl())    // TODO: add indent_angle_nl?
                 || (  pc->IsString("[")
                    && !options::indent_square_nl()))
         {
            log_rule_B("indent_paren_nl");
            log_rule_B("indent_square_nl");
            Chunk *next = pc->GetNextNc();

            if (next->IsNullChunk())
            {
               break;
            }
            log_rule_B("indent_paren_after_func_def");
            log_rule_B("indent_paren_after_func_decl");
            log_rule_B("indent_paren_after_func_call");

            if (  next->IsNewline()
               && !options::indent_paren_after_func_def()
               && !options::indent_paren_after_func_decl()
               && !options::indent_paren_after_func_call())
            {
               size_t sub = 2;

               if (  (frm.prev().type == CT_ASSIGN)
                  || (frm.prev().type == CT_RETURN))
               {
                  sub = 3;
               }
               sub = static_cast<int>(frm.size()) - sub;

               log_rule_B("indent_align_paren");

               if (!options::indent_align_paren())
               {
                  sub = static_cast<int>(frm.size()) - 2;

                  while (  sub > 0
                        && frm.at(sub).pc->IsOnSameLine(frm.top().pc))
                  {
                     sub--;
                     skipped = true;
                  }

                  if (  (  frm.at(sub + 1).type == CT_CLASS_COLON
                        || frm.at(sub + 1).type == CT_CONSTR_COLON)
                     && (frm.at(sub + 1).pc->GetPrev()->Is(CT_NEWLINE)))
                  {
                     sub = sub + 1;
                  }
               }
               frm.top().indent = frm.at(sub).indent + indent_size;
               log_indent();

               frm.top().indent_tab = frm.top().indent;
               skipped = true;
            }
            else
            {
               if (  next->IsNotNullChunk()
                  && !next->IsComment())
               {
                  if (next->Is(CT_SPACE))
                  {
                     next = next->GetNextNc();

                     if (next->IsNullChunk())
                     {
                        break;
                     }
                  }

                  if (next->prev->IsComment())
                  {
                     // Issue #2099
                     frm.top().indent = next->prev->column;
                  }
                  else
                  {
                     frm.top().indent = next->column;
                  }
                  log_indent();
               }
            }
         }
         log_rule_B("use_indent_continue_only_once");
         log_rule_B("indent_paren_after_func_decl");
         log_rule_B("indent_paren_after_func_def");
         log_rule_B("indent_paren_after_func_call");

         if (  (  (  !frm.top().indent_cont                     // Issue #3567
                  && vardefcol == 0)
               || (  !options::use_indent_continue_only_once()  // Issue #1160
                  && !options::indent_ignore_first_continue())) // Issue #3561
            && (  pc->Is(CT_FPAREN_OPEN)
               && pc->GetPrev()->IsNewline())
            && (  (  (  get_chunk_parent_type(pc) == CT_FUNC_PROTO
                     || get_chunk_parent_type(pc) == CT_FUNC_CLASS_PROTO)
                  && options::indent_paren_after_func_decl())
               || (  (  get_chunk_parent_type(pc) == CT_FUNC_DEF
                     || get_chunk_parent_type(pc) == CT_FUNC_CLASS_DEF)
                  && options::indent_paren_after_func_def())
               || (  (  get_chunk_parent_type(pc) == CT_FUNC_CALL
                     || get_chunk_parent_type(pc) == CT_FUNC_CALL_USER)
                  && options::indent_paren_after_func_call())
               || !pc->GetNext()->IsNewline()))
         {
            frm.top().indent = frm.prev().indent + indent_size;
            log_indent();

            indent_column_set(frm.top().indent);
         }
         log_rule_B("indent_continue");

         if (  get_chunk_parent_type(pc) != CT_OC_AT
            && (  options::indent_ignore_first_continue()
               || options::indent_continue() != 0)
            && !skipped)
         {
            if (options::indent_ignore_first_continue())
            {
               frm.top().indent = get_indent_first_continue(pc->GetNext());
            }
            else
            {
               frm.top().indent = frm.prev().indent;
            }
            log_indent();

            if (  pc->level == pc->brace_level
               && !options::indent_ignore_first_continue()
               && (  pc->Is(CT_FPAREN_OPEN)
                  || pc->Is(CT_SPAREN_OPEN)
                  || (  pc->Is(CT_SQUARE_OPEN)
                     && get_chunk_parent_type(pc) != CT_OC_MSG)
                  || pc->Is(CT_ANGLE_OPEN)))     // Issue #1170
            {
               //log_rule_B("indent_continue");
               //frm.top().indent += abs(options::indent_continue());
               //   frm.top().indent      = calc_indent_continue(frm);
               //   frm.top().indent_cont = true;
               log_rule_B("use_indent_continue_only_once");

               if (  (options::use_indent_continue_only_once())
                  && (frm.top().indent_cont)
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
                  frm.top().indent = vardefcol;
                  log_indent();
               }
               else
               {
                  frm.top().indent = calc_indent_continue(frm);
                  log_indent();
                  frm.top().indent_cont = true;

                  log_rule_B("indent_sparen_extra");

                  if (  pc->Is(CT_SPAREN_OPEN)
                     && options::indent_sparen_extra() != 0)
                  {
                     frm.top().indent += options::indent_sparen_extra();
                     log_indent();
                  }
               }
            }
         }
         frm.top().indent_tmp = frm.top().indent;
         log_indent_tmp();

         frm.paren_count++;
      }
      else if (  options::indent_member_single()
              && pc->Is(CT_MEMBER)
              && (strcmp(pc->Text(), ".") == 0)
              && language_is_set(LANG_CS | LANG_CPP))
      {
         log_rule_B("indent_member_single");

         if (frm.top().type != CT_MEMBER)
         {
            frm.push(pc, __func__, __LINE__);
            Chunk *tmp = frm.top().pc->GetPrevNcNnlNpp();

            if (frm.prev().pc->IsOnSameLine(tmp))
            {
               frm.top().indent = frm.prev().indent;
            }
            else
            {
               frm.top().indent = frm.prev().indent + indent_size;
            }
            log_indent();
            frm.top().indent_tmp = frm.top().indent;
            log_indent_tmp();
         }

         if (pc->GetPrev()->IsNewline())
         {
            if (  pc->Is(CT_MEMBER)                           // Issue #2890
               && language_is_set(LANG_CPP))
            {
               // will be done at another place
               // look at the comment: XXXXXXXXXXXXXXXXXXXXXXXXXX
            }
            else
            {
               indent_column_set(frm.top().indent);
               reindent_line(pc, indent_column);
               did_newline = false;
            }
         }
         //check for the series of CT_member chunks else pop it.
         Chunk *tmp = pc->GetNextNcNnlNpp();

         if (tmp->IsNotNullChunk())
         {
            if (tmp->Is(CT_FUNC_CALL))
            {
               tmp = tmp->GetNextType(CT_FPAREN_CLOSE, tmp->level);
               tmp = tmp->GetNextNcNnlNpp();
            }
            else if (  tmp->Is(CT_WORD)
                    || tmp->Is(CT_TYPE))
            {
               tmp = tmp->GetNextNcNnlNpp();
            }
         }

         if (  tmp->IsNotNullChunk()
            && (  (strcmp(tmp->Text(), ".") != 0)
               || tmp->IsNot(CT_MEMBER)))
         {
            if (tmp->IsParenClose())
            {
               tmp = tmp->GetPrevNcNnlNpp();
            }
            Chunk *local_prev = tmp->GetPrev();             // Issue #3294

            if (local_prev->IsComment())
            {
               tmp = tmp->GetPrev();                        // Issue #3294
            }

            if (  tmp->IsNotNullChunk()
               && tmp->GetPrev()->IsNewline())
            {
               tmp = tmp->GetPrevNcNnlNpp()->GetNextNl();
            }

            if (tmp->IsNotNullChunk())
            {
               frm.top().pop_pc = tmp;
            }
         }
      }
      else if (  pc->Is(CT_ASSIGN)
              || pc->Is(CT_IMPORT)
              || (  pc->Is(CT_USING)
                 && language_is_set(LANG_CS)))
      {
         /*
          * if there is a newline after the '=' or the line starts with a '=',
          * just indent one level,
          * otherwise align on the '='.
          */
         if (  pc->Is(CT_ASSIGN)
            && pc->GetPrev()->IsNewline())
         {
            if (frm.top().type == CT_ASSIGN_NL)
            {
               frm.top().indent_tmp = frm.top().indent;
            }
            else
            {
               frm.top().indent_tmp = frm.top().indent + indent_size;
            }
            log_indent_tmp();

            indent_column_set(frm.top().indent_tmp);
            LOG_FMT(LINDENT, "%s(%d): %zu] assign => %zu [%s]\n",
                    __func__, __LINE__, pc->orig_line, indent_column, pc->Text());
            reindent_line(pc, frm.top().indent_tmp);
         }
         Chunk *next = pc->GetNext();

         if (next->IsNotNullChunk())
         {
            /*
             * fixes  1260 , 1268 , 1277 (Extra indentation after line with multiple assignments)
             * For multiple consecutive assignments in single line , the indent of all these
             * assignments should be same and one more than this line's indent.
             * so popping the previous assign and pushing the new one
             */
            if (  frm.top().type == CT_ASSIGN
               && pc->Is(CT_ASSIGN))
            {
               LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
               frm.pop(__func__, __LINE__, pc);
            }
            frm.push(pc, __func__, __LINE__);

            if (  pc->Is(CT_ASSIGN)
               && pc->GetPrev()->IsNewline())
            {
               frm.top().type = CT_ASSIGN_NL;
            }
            log_rule_B("indent_continue");

            if (options::indent_ignore_first_continue())
            {
               frm.top().indent = get_indent_first_continue(pc);
               log_indent();
               frm.top().indent_cont = true; // Issue #3567
            }
            else if (options::indent_continue() != 0)
            {
               frm.top().indent = frm.prev().indent;
               log_indent();

               if (  pc->level == pc->brace_level
                  && (  pc->IsNot(CT_ASSIGN)
                     || (  get_chunk_parent_type(pc) != CT_FUNC_PROTO
                        && get_chunk_parent_type(pc) != CT_FUNC_DEF)))
               {
                  log_rule_B("use_indent_continue_only_once");

                  if (  (options::use_indent_continue_only_once())
                     && (frm.top().indent_cont)
                     && vardefcol != 0)
                  {
                     // if vardefcol isn't zero, use it
                     frm.top().indent = vardefcol;
                     log_indent();
                  }
                  else
                  {
                     frm.top().indent = calc_indent_continue(frm);
                     log_indent();

                     vardefcol = frm.top().indent;  // use the same variable for the next line
                     frm.top().indent_cont = true;
                  }
               }
            }
            else if (  next->IsNewline()
                    || !options::indent_align_assign())
            {
               log_rule_B("indent_align_assign");
               log_rule_B("indent_off_after_assign");

               if (options::indent_off_after_assign())             // Issue #2591
               {
                  frm.top().indent = frm.prev().indent_tmp;
               }
               else
               {
                  frm.top().indent = frm.prev().indent_tmp + indent_size;
               }
               log_indent();

               if (  pc->Is(CT_ASSIGN)
                  && next->IsNewline())
               {
                  frm.top().type       = CT_ASSIGN_NL;
                  frm.top().indent_tab = frm.top().indent;
               }
            }
            else
            {
               frm.top().indent = pc->column + pc->Len() + 1;
               log_indent();
            }
            frm.top().indent_tmp = frm.top().indent;
            log_indent_tmp();
         }
      }
      else if (  pc->Is(CT_RETURN)
              || (  pc->Is(CT_THROW)
                 && get_chunk_parent_type(pc) == CT_NONE))
      {
         // don't count returns inside a () or []
         if (  pc->level == pc->brace_level
            || pc->flags.test(PCF_IN_LAMBDA))
         {
            Chunk *next = pc->GetNext();

            // Avoid indentation on return token set by the option.
            log_rule_B("indent_off_after_return");

            // Avoid indentation on return token if the next token is a new token
            // to properly indent object initializers returned by functions.
            log_rule_B("indent_off_after_return_new");
            bool indent_after_return = (  next->IsNotNullChunk()
                                       && next->type == CT_NEW)
                                       ? !options::indent_off_after_return_new()
                                       : !options::indent_off_after_return();

            if (  indent_after_return
               || next->IsNullChunk())
            {
               frm.push(pc, __func__, __LINE__);

               log_rule_B("indent_single_after_return");

               if (  next->IsNewline()
                  || (  pc->Is(CT_RETURN)
                     && options::indent_single_after_return()))
               {
                  // apply normal single indentation
                  frm.top().indent = frm.prev().indent + indent_size;
               }
               else
               {
                  // indent after the return token
                  frm.top().indent = frm.prev().indent + pc->Len() + 1;
               }
               log_indent();
               frm.top().indent_tmp = frm.prev().indent;
               log_indent_tmp();
            }
            log_indent();
         }
      }
      else if (  pc->Is(CT_OC_SCOPE)
              || pc->Is(CT_TYPEDEF))
      {
         frm.push(pc, __func__, __LINE__);
         // Issue #405
         frm.top().indent = frm.prev().indent;
         log_indent();

         frm.top().indent_tmp = frm.top().indent;
         LOG_FMT(LINDLINE, "%s(%d): .indent is %zu, .indent_tmp is %zu\n",
                 __func__, __LINE__, frm.top().indent, frm.top().indent_tmp);

         log_rule_B("indent_continue");

         if (options::indent_ignore_first_continue())
         {
            frm.top().indent = get_indent_first_continue(pc);
            log_indent();
         }
         else if (options::indent_continue() != 0)
         {
            frm.top().indent = calc_indent_continue(frm, frm.size() - 2);
            log_indent();

            frm.top().indent_cont = true;
         }
         else
         {
            frm.top().indent = frm.prev().indent + indent_size;
            log_indent();
         }
      }
      else if (pc->Is(CT_C99_MEMBER))
      {
         // nothing to do
      }
      else if (pc->Is(CT_WHERE_SPEC))
      {
         /* class indentation is ok already, just need to adjust func */
         /* TODO: make this configurable, obviously.. */
         if (  get_chunk_parent_type(pc) == CT_FUNC_DEF
            || get_chunk_parent_type(pc) == CT_FUNC_PROTO
            || (  get_chunk_parent_type(pc) == CT_STRUCT
               && frm.top().type != CT_CLASS_COLON))
         {
            indent_column_set(frm.top().indent + 4);
         }
      }
      else if (  options::indent_inside_ternary_operator()
              && (  pc->Is(CT_QUESTION)
                 || pc->Is(CT_COND_COLON))) // Issue #1130, #1715
      {
         log_rule_B("indent_inside_ternary_operator");

         // Pop any colons before because they should already be processed
         while (  pc->Is(CT_COND_COLON)
               && frm.top().type == CT_COND_COLON)
         {
            frm.pop(__func__, __LINE__, pc);
         }
         log_rule_B("indent_inside_ternary_operator");

         // Pop Question from stack in ternary operator
         if (  options::indent_inside_ternary_operator()
            && pc->Is(CT_COND_COLON)
            && frm.top().type == CT_QUESTION)
         {
            LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
            frm.pop(__func__, __LINE__, pc);
            indent_column_set(frm.top().indent_tmp);
         }
         frm.push(pc, __func__, __LINE__);

         frm.top().indent     = frm.prev().indent + indent_size;
         frm.top().indent_tab = frm.top().indent;
         log_indent();
         frm.top().indent_tmp = frm.top().indent;
         log_indent_tmp();
      }
      else if (  pc->Is(CT_LAMBDA)
              && (language_is_set(LANG_CS | LANG_JAVA))
              && pc->GetNextNcNnlNpp()->IsNot(CT_BRACE_OPEN)
              && options::indent_cs_delegate_body())
      {
         log_rule_B("indent_cs_delegate_body");
         frm.push(pc, __func__, __LINE__);
         frm.top().indent = frm.prev().indent;
         log_indent();

         if (  pc->GetPrevNc()->IsNewline()
            && !frm.prev().pc->IsOnSameLine(pc->GetPrevNcNnl()))
         {
            frm.top().indent = frm.prev().indent + indent_size;
            log_indent();
            reindent_line(pc, (frm.prev().indent + indent_size));
            did_newline = false;
         }
         else if (  pc->GetNextNc()->IsNewline()
                 && !frm.prev().pc->IsOnSameLine(frm.top().pc))
         {
            frm.top().indent = frm.prev().indent + indent_size;
         }
         log_indent();
         frm.top().indent_tmp = frm.top().indent;
         log_indent_tmp();
      }
      else if (  options::indent_oc_inside_msg_sel()
              && (  pc->Is(CT_OC_MSG_FUNC)
                 || pc->Is(CT_OC_MSG_NAME))
              && pc->GetNextNcNnl()->Is(CT_OC_COLON)) // Issue #2658
      {
         log_rule_B("indent_oc_inside_msg_sel");
         // Pop the OC msg name that is on the top of the stack
         // [Class Message:<here>
         frm.push(pc, __func__, __LINE__);

         frm.top().indent     = frm.prev().indent;
         frm.top().indent_tab = frm.prev().indent_tab;
         log_indent();
         frm.top().indent_tmp = frm.prev().indent_tmp;
         log_indent_tmp();
      }
      else if (pc->IsComment())
      {
         // Issue #3294
         Chunk *next = pc->GetNext();

         if (next->Is(CT_COND_COLON))
         {
            LOG_FMT(LINDLINE, "%s(%d): Comment and COND_COLON: pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
            frm.pop(__func__, __LINE__, pc);
         }
// uncomment the line below to get debug info
// #define ANYTHING_ELSE
#ifdef ANYTHING_ELSE
         else
         {
            // anything else?
            // Issue #3294
            LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
            LOG_FMT(LSPACE, "\n\n%s(%d): WARNING: unrecognize indent_text:\n",
                    __func__, __LINE__);
         }
#endif /* ANYTHING_ELSE */
      }
      else
      {
         // anything else?
#ifdef ANYTHING_ELSE
         LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
         LOG_FMT(LSPACE, "\n\n%s(%d): WARNING: unrecognize indent_text:\n",
                 __func__, __LINE__);
#endif /* ANYTHING_ELSE */
      }
      // Handle shift expression continuation indenting
      size_t shiftcontcol = 0;

      log_rule_B("indent_shift");

      if (  options::indent_shift() == 1
         && !pc->flags.test(PCF_IN_ENUM)
         && get_chunk_parent_type(pc) != CT_OPERATOR
         && !pc->IsComment()
         && pc->IsNot(CT_BRACE_OPEN)
         && pc->level > 0
         && !pc->IsEmptyText())
      {
         bool in_shift    = false;
         bool is_operator = false;

         // Are we in such an expression? Go both forwards and backwards.
         Chunk *tmp = pc;

         do
         {
            if (tmp->Is(CT_SHIFT))
            {
               in_shift = true;
               LOG_FMT(LINDENT2, "%s(%d): in_shift set to TRUE\n",
                       __func__, __LINE__);

               tmp = tmp->GetPrevNcNnl();

               if (tmp->Is(CT_OPERATOR))
               {
                  is_operator = true;
               }
               break;
            }
            tmp = tmp->GetPrevNcNnl();
         } while (  !in_shift
                 && tmp->IsNotNullChunk()
                 && tmp->IsNot(CT_SEMICOLON)
                 && tmp->IsNot(CT_BRACE_OPEN)
                 && tmp->IsNot(CT_BRACE_CLOSE)
                 && tmp->IsNot(CT_COMMA)
                 && tmp->IsNot(CT_SPAREN_OPEN)
                 && tmp->IsNot(CT_SPAREN_CLOSE));

         tmp = pc;

         do
         {
            tmp = tmp->GetNextNcNnl();

            if (  tmp->IsNotNullChunk()
               && tmp->Is(CT_SHIFT))
            {
               in_shift = true;
               LOG_FMT(LINDENT2, "%s(%d): in_shift set to TRUE\n",
                       __func__, __LINE__);

               tmp = tmp->GetPrevNcNnl();

               if (tmp->Is(CT_OPERATOR))
               {
                  is_operator = true;
               }
               break;
            }
         } while (  !in_shift
                 && tmp->IsNotNullChunk()
                 && tmp->IsNot(CT_SEMICOLON)
                 && tmp->IsNot(CT_BRACE_OPEN)
                 && tmp->IsNot(CT_BRACE_CLOSE)
                 && tmp->IsNot(CT_COMMA)
                 && tmp->IsNot(CT_SPAREN_OPEN)
                 && tmp->IsNot(CT_SPAREN_CLOSE));

         LOG_FMT(LINDENT2, "%s(%d): in_shift is %s\n",
                 __func__, __LINE__, in_shift ? "TRUE" : "FALSE");
         Chunk *prev_nonl = pc->GetPrevNcNnl();
         Chunk *prev2     = pc->GetPrevNc();

         if ((  prev_nonl->IsSemicolon()
             || prev_nonl->IsBraceOpen()
             || prev_nonl->IsBraceClose()
             || prev_nonl->Is(CT_CASE_COLON)
             || (  prev_nonl->IsNotNullChunk()
                && prev_nonl->flags.test(PCF_IN_PREPROC)) != pc->flags.test(PCF_IN_PREPROC)
             || prev_nonl->Is(CT_COMMA)
             || is_operator))
         {
            in_shift = false;
         }
         LOG_FMT(LINDENT2, "%s(%d): in_shift is %s\n",
                 __func__, __LINE__, in_shift ? "TRUE" : "FALSE");

         if (  prev2->Is(CT_NEWLINE)
            && in_shift)
         {
            shiftcontcol = calc_indent_continue(frm);
            // Setting frm.top().indent_cont = true in the top context when the indent is not also set
            // just leads to complications when succeeding statements try to indent based on being
            // embedded in a continuation. In other words setting frm.top().indent_cont = true
            // should only be set if frm.top().indent is also set.

            // Work around the doubly increased indent in RETURNs and assignments
            bool   need_workaround = false;
            size_t sub             = 0;

            for (int i = frm.size() - 1; i >= 0; i--)
            {
               if (  frm.at(i).type == CT_RETURN
                  || frm.at(i).type == CT_ASSIGN)
               {
                  need_workaround = true;
                  sub             = frm.size() - i;
                  break;
               }
            }

            if (need_workaround)
            {
               shiftcontcol = calc_indent_continue(frm, frm.size() - 1 - sub);
            }
         }
      }

      // Handle variable definition continuation indenting
      if (  vardefcol == 0
         && (  pc->Is(CT_WORD)
            || pc->Is(CT_FUNC_CTOR_VAR))
         && !pc->flags.test(PCF_IN_FCN_DEF)
         && pc->flags.test(PCF_VAR_1ST_DEF))
      {
         log_rule_B("indent_continue");

         if (options::indent_ignore_first_continue())
         {
            vardefcol = get_indent_first_continue(pc);
         }
         else if (options::indent_continue() != 0)
         {
            vardefcol = calc_indent_continue(frm);
            // Setting frm.top().indent_cont = true in the top context when the indent is not also set
            // just leads to complications when succeeding statements try to indent based on being
            // embedded in a continuation. In other words setting frm.top().indent_cont = true
            // should only be set if frm.top().indent is also set.
         }
         else if (  options::indent_var_def_cont()
                 || pc->GetPrev()->IsNewline())
         {
            log_rule_B("indent_var_def_cont");
            vardefcol = frm.top().indent + indent_size;
         }
         else
         {
            // Issue #3010
            vardefcol = pc->column;
            // BUT, we need to skip backward over any '*'
            Chunk *tmp = pc->GetPrevNc();

            while (tmp->Is(CT_PTR_TYPE))
            {
               vardefcol = tmp->column;
               tmp       = tmp->GetPrevNc();
            }
            // BUT, we need to skip backward over any '::' or TYPE
            //tmp = pc->GetPrevNc();

            //if (tmp->Is(CT_DC_MEMBER))
            //{
            //   // look for a type
            //   Chunk *tmp2 = tmp->GetPrevNc();
            //   if (tmp2->Is(CT_TYPE))
            //   {
            //      // we have something like "SomeLongNamespaceName::Foo()"
            //      vardefcol = tmp2->column;
            //      LOG_FMT(LINDENT, "%s(%d): orig_line is %zu, vardefcol is %zu\n",
            //              __func__, __LINE__, pc->orig_line, vardefcol);
            //   }
            //}
         }
      }

      if (  pc->IsSemicolon()
         || (  pc->Is(CT_BRACE_OPEN)
            && (  get_chunk_parent_type(pc) == CT_FUNCTION
               || get_chunk_parent_type(pc) == CT_CLASS))) //Issue #3576
      {
         vardefcol = 0;
      }

      // Indent the line if needed
      if (  did_newline
         && !pc->IsNewline()
         && (pc->Len() != 0))
      {
         pc->column_indent = frm.top().indent_tab;

         if (frm.top().ip.ref)
         {
            pc->indent.ref   = frm.top().ip.ref;
            pc->indent.delta = frm.top().ip.delta;
         }
         LOG_FMT(LINDENT2, "%s(%d): orig_line is %zu, pc->column_indent is %zu, indent_column is %zu, for '%s'\n",
                 __func__, __LINE__, pc->orig_line, pc->column_indent, indent_column, pc->ElidedText(copy));

         /*
          * Check for special continuations.
          * Note that some of these could be done as a stack item like
          * everything else
          */

         auto prev  = pc->GetPrevNcNnl();
         auto prevv = prev->GetPrevNcNnl();
         auto next  = pc->GetNextNcNnl();

         bool do_vardefcol = false;

         if (  vardefcol > 0
            && pc->level == pc->brace_level
            && (  prev->Is(CT_COMMA)
               || prev->Is(CT_TYPE)
               || prev->Is(CT_PTR_TYPE)
               || prev->Is(CT_WORD)))
         {
            Chunk *tmp = pc;

            while (tmp->Is(CT_PTR_TYPE))
            {
               tmp = tmp->GetNextNcNnl();
            }
            LOG_FMT(LINDENT2, "%s(%d): orig_line is %zu, for '%s'",
                    __func__, __LINE__, tmp->orig_line, tmp->Text());
            LOG_FMT(LINDENT2, " tmp->flags: ");
            log_pcf_flags(LINDENT2, tmp->flags);                   // Issue #2332

            if (  tmp->flags.test(PCF_VAR_DEF)
               && (  tmp->Is(CT_WORD)
                  || tmp->Is(CT_FUNC_CTOR_VAR)))
            {
               do_vardefcol = true;
            }
         }
         //LOG_FMT(LINDENT2, "%s(%d): GUY 2:\n", __func__, __LINE__);

         if (pc->flags.test(PCF_DONT_INDENT))
         {
            // no change
         }
         else if (  get_chunk_parent_type(pc) == CT_SQL_EXEC
                 && options::indent_preserve_sql())
         {
            log_rule_B("indent_preserve_sql");
            reindent_line(pc, sql_col + (pc->orig_col - sql_orig_col));
            LOG_FMT(LINDENT, "Indent SQL: [%s] to %zu (%zu/%zu)\n",
                    pc->Text(), pc->column, sql_col, sql_orig_col);
         }
         else if (  !options::indent_member_single()
                 && !pc->flags.test(PCF_STMT_START)
                 && (  pc->Is(CT_MEMBER)
                    || (  pc->Is(CT_DC_MEMBER)
                       && prev->Is(CT_TYPE))
                    || (  prev->Is(CT_MEMBER)
                       || (  prev->Is(CT_DC_MEMBER)
                          && prevv->Is(CT_TYPE)))))
         {
            log_rule_B("indent_member_single");
            log_rule_B("indent_member");
            size_t tmp = options::indent_member() + indent_column;
            LOG_FMT(LINDENT, "%s(%d): orig_line is %zu, member => %zu\n",
                    __func__, __LINE__, pc->orig_line, tmp);
            reindent_line(pc, tmp);
         }
         else if (do_vardefcol)
         {
            LOG_FMT(LINDENT, "%s(%d): orig_line is %zu, vardefcol is %zu\n",
                    __func__, __LINE__, pc->orig_line, vardefcol);
            reindent_line(pc, vardefcol);
         }
         else if (shiftcontcol > 0)
         {
            LOG_FMT(LINDENT, "%s(%d): orig_line is %zu, shiftcontcol is %zu\n",
                    __func__, __LINE__, pc->orig_line, shiftcontcol);
            reindent_line(pc, shiftcontcol);
         }
         else if (  pc->Is(CT_NAMESPACE)
                 && options::indent_namespace()
                 && options::indent_namespace_single_indent()
                 && frm.top().ns_cnt)
         {
            log_rule_B("indent_namespace");
            log_rule_B("indent_namespace_single_indent");
            LOG_FMT(LINDENT, "%s(%d): orig_line is %zu, Namespace => %zu\n",
                    __func__, __LINE__, pc->orig_line, frm.top().brace_indent);
            reindent_line(pc, frm.top().brace_indent);
         }
         else if (  pc->Is(CT_STRING)
                 && prev->Is(CT_STRING)
                 && options::indent_align_string())
         {
            log_rule_B("indent_align_string");
            const int tmp = (xml_indent != 0) ? xml_indent : prev->column;

            LOG_FMT(LINDENT, "%s(%d): orig_line is %zu, String => %d\n",
                    __func__, __LINE__, pc->orig_line, tmp);
            reindent_line(pc, tmp);
         }
         else if (pc->IsComment())
         {
            LOG_FMT(LINDENT, "%s(%d): orig_line is %zu, comment => %zu\n",
                    __func__, __LINE__, pc->orig_line, frm.top().indent_tmp);
            indent_comment(pc, frm.top().indent_tmp);
         }
         else if (pc->Is(CT_PREPROC))
         {
            LOG_FMT(LINDENT, "%s(%d): orig_line is %zu, pp-indent => %zu [%s]\n",
                    __func__, __LINE__, pc->orig_line, indent_column, pc->Text());
            reindent_line(pc, indent_column);
         }
         else if (  pc->IsParenClose()
                 || pc->Is(CT_ANGLE_CLOSE))
         {
            /*
             * This is a big hack. We assume that since we hit a paren close,
             * that we just removed a paren open
             */
            LOG_FMT(LINDLINE, "%s(%d): indent_column is %zu\n",
                    __func__, __LINE__, indent_column);

            if (frm.poped().type == E_Token(pc->type - 1))
            {
               // Issue # 405
               LOG_FMT(LINDLINE, "%s(%d): orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
               Chunk *ck1 = frm.poped().pc;
               LOG_FMT(LINDLINE, "%s(%d): ck1->orig_line is %zu, ck1->orig_col is %zu, ck1->Text() is '%s', ck1->type is %s\n",
                       __func__, __LINE__, ck1->orig_line, ck1->orig_col, ck1->Text(), get_token_name(ck1->type));
               Chunk *ck2 = ck1->GetPrev();
               LOG_FMT(LINDLINE, "%s(%d): ck2->orig_line is %zu, ck2->orig_col is %zu, ck2->Text() is '%s', ck2->type is %s\n",
                       __func__, __LINE__, ck2->orig_line, ck2->orig_col, ck2->Text(), get_token_name(ck2->type));

               log_rule_B("indent_paren_close");

               if (options::indent_paren_close() == -1)
               {
                  LOG_FMT(LINDLINE, "%s(%d): [%zu:%zu] indent_paren_close is -1\n",
                          __func__, __LINE__, ck2->orig_line, ck2->orig_col);
                  indent_column_set(pc->orig_col);
                  LOG_FMT(LINDLINE, "%s(%d): [%zu:%zu] indent_column set to %zu\n",
                          __func__, __LINE__, ck2->orig_line, ck2->orig_col, indent_column);
               }
               else if (  ck2->IsNewline()
                       || (options::indent_paren_close() == 1))
               {
                  /*
                   * If the open parenthesis was the first thing on the line or we
                   * are doing mode 1, then put the close parenthesis in the same
                   * column
                   */
                  LOG_FMT(LINDLINE, "%s(%d): [%zu:%zu] indent_paren_close is 1\n",
                          __func__, __LINE__, ck2->orig_line, ck2->orig_col);
                  indent_column_set(ck1->column);
                  LOG_FMT(LINDLINE, "%s(%d): [%zu:%zu] indent_column set to %zu\n",
                          __func__, __LINE__, ck2->orig_line, ck2->orig_col, indent_column);
               }
               else
               {
                  if (options::indent_paren_close() != 2)
                  {
                     // indent_paren_close is 0 or 1
                     LOG_FMT(LINDLINE, "%s(%d): [%zu:%zu] indent_paren_close is 0 or 1\n",
                             __func__, __LINE__, ck2->orig_line, ck2->orig_col);
                     indent_column_set(frm.poped().indent_tmp);
                     LOG_FMT(LINDLINE, "%s(%d): [%zu:%zu] indent_column set to %zu\n",
                             __func__, __LINE__, ck2->orig_line, ck2->orig_col, indent_column);
                     pc->column_indent = frm.poped().indent_tab;
                     log_rule_B("indent_paren_close");

                     if (options::indent_paren_close() == 1)
                     {
                        LOG_FMT(LINDLINE, "%s(%d): [%zu:%zu] indent_paren_close is 1\n",
                                __func__, __LINE__, ck2->orig_line, ck2->orig_col);

                        if (indent_column == 0)
                        {
                           fprintf(stderr, "%s(%d): indent_column is ZERO, cannot be decremented, at line %zu, column %zu\n",
                                   __func__, __LINE__, pc->orig_line, pc->orig_col);
                           log_flush(true);
                           exit(EX_SOFTWARE);
                        }
                        indent_column--;
                        LOG_FMT(LINDLINE, "%s(%d): [%zu:%zu] indent_column set to %zu\n",
                                __func__, __LINE__, ck2->orig_line, ck2->orig_col, indent_column);
                     }
                  }
                  else
                  {
                     // indent_paren_close is 2: Indent to the brace level
                     LOG_FMT(LINDLINE, "%s(%d): indent_paren_close is 2\n",
                             __func__, __LINE__);
                     LOG_FMT(LINDLINE, "%s(%d): ck2->orig_line is %zu, ck2->orig_col is %zu, ck2->Text() is '%s'\n",
                             __func__, __LINE__, ck2->orig_line, ck2->orig_col, ck2->Text());

                     if (pc->GetPrev()->type == CT_NEWLINE)
                     {
                        LOG_FMT(LINDLINE, "%s(%d): orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                                __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
                        LOG_FMT(LINDLINE, "%s(%d): prev is <newline>\n",
                                __func__, __LINE__);
                        Chunk *search = pc;

                        while (search->GetNext()->IsParenClose())
                        {
                           search = search->GetNext();
                        }
                        Chunk *searchNext = search->GetNext();

                        // Issue #3407 - Skip over a possible 'noexcept' keyword before going forward.
                        if (searchNext->type == CT_NOEXCEPT)
                        {
                           searchNext = searchNext->GetNext();
                        }

                        if (  searchNext->type == CT_SEMICOLON
                           || searchNext->type == CT_MEMBER            // Issue #2582
                           || searchNext->type == CT_NEWLINE)
                        {
                           LOG_FMT(LINDLINE, "%s(%d):\n", __func__, __LINE__);
                           search = search->SkipToMatchRev();

                           if (  options::indent_oc_inside_msg_sel()
                              && search->GetPrevNcNnl()->Is(CT_OC_COLON)
                              && (  frm.top().type == CT_OC_MSG_FUNC
                                 || frm.top().type == CT_OC_MSG_NAME)) // Issue #2658
                           {
                              log_rule_B("indent_oc_inside_msg_sel");
                              // [Class Message:(...)<here>
                              indent_column_set(frm.top().pc->column);
                           }
                           else if (  options::indent_inside_ternary_operator()
                                   && (  frm.top().type == CT_QUESTION
                                      || frm.top().type == CT_COND_COLON)) // Issue #1130, #1715
                           {
                              log_rule_B("indent_inside_ternary_operator");
                              indent_column_set(frm.top().indent);
                           }
                           else
                           {
                              search = search->GetPrevNl()->GetNext();

                              if (search->IsNullChunk())
                              {
                                 search = Chunk::GetHead();
                              }
                              indent_column_set(search->column);
                           }
                        }
                     }
                  }
               }
            }
            size_t indent_value = 0;
            LOG_FMT(LINDENT, "%s(%d): orig_line is %zu, closing parenthesis => %zu, text is '%s'\n",
                    __func__, __LINE__, pc->orig_line, indent_column, pc->Text());
            LOG_FMT(LINDENT, "%s(%d): [%s/%s]\n",
                    __func__, __LINE__,
                    get_token_name(pc->type), get_token_name(get_chunk_parent_type(pc)));
            Chunk *prev2 = pc->GetPrev();                  // Issue #2930
            LOG_FMT(LINDENT, "%s(%d): prev2 is orig_line is %zu, text is '%s'\n",
                    __func__, __LINE__, prev2->orig_line, prev2->Text());
            Chunk *next2 = pc->GetNext();
            LOG_FMT(LINDENT, "%s(%d): next2 is orig_line is %zu, text is '%s'\n",
                    __func__, __LINE__, next2->orig_line, next2->Text());

            if (  get_chunk_parent_type(pc) == CT_FUNC_DEF
               && prev2->IsNewline()
               && next2->IsNewline())
            {
               if (options::donot_indent_func_def_close_paren())
               {
                  indent_value = 1;
               }
               else
               {
                  reindent_line(pc, indent_column);
                  indent_value = indent_column;
               }
            }
            else
            {
               indent_value = indent_column;
            }
            reindent_line(pc, indent_value);
         }
         else if (pc->Is(CT_COMMA))
         {
            bool align  = false;
            bool ignore = false;

            if (frm.top().pc->IsParenOpen())
            {
               log_rule_B("indent_comma_paren");
               align  = options::indent_comma_paren() == (int)indent_mode_e::ALIGN;
               ignore = options::indent_comma_paren() == (int)indent_mode_e::IGNORE;
            }
            else if (frm.top().pc->IsBraceOpen())
            {
               log_rule_B("indent_comma_brace");
               align  = options::indent_comma_brace() == (int)indent_mode_e::ALIGN;
               ignore = options::indent_comma_brace() == (int)indent_mode_e::IGNORE;
            }

            if (ignore)
            {
               indent_column_set(pc->orig_col);
            }
            else if (align)
            {
               indent_column_set(frm.top().pc->column);
            }
            LOG_FMT(LINDENT, "%s(%d): %zu] comma => %zu [%s]\n",
                    __func__, __LINE__, pc->orig_line, indent_column, pc->Text());
            reindent_line(pc, indent_column);
         }
         else if (  options::indent_func_const()
                 && pc->Is(CT_QUALIFIER)
                 && strncasecmp(pc->Text(), "const", pc->Len()) == 0
                 && (  next == nullptr
                    || next->Is(CT_BRACED)
                    || next->IsBraceOpen()
                    || next->Is(CT_NEWLINE)
                    || next->Is(CT_SEMICOLON)
                    || next->Is(CT_THROW)))
         {
            // indent const - void GetFoo(void)\n const\n { return (m_Foo); }
            log_rule_B("indent_func_const");
            indent_column_set(frm.top().indent + options::indent_func_const());
            LOG_FMT(LINDENT, "%s(%d): %zu] const => %zu [%s]\n",
                    __func__, __LINE__, pc->orig_line, indent_column, pc->Text());
            reindent_line(pc, indent_column);
         }
         else if (  options::indent_func_throw()
                 && pc->Is(CT_THROW)
                 && get_chunk_parent_type(pc) != CT_NONE)
         {
            // indent throw - void GetFoo(void)\n throw()\n { return (m_Foo); }
            log_rule_B("indent_func_throw");
            indent_column_set(options::indent_func_throw());
            LOG_FMT(LINDENT, "%s(%d): %zu] throw => %zu [%s]\n",
                    __func__, __LINE__, pc->orig_line, indent_column, pc->Text());
            reindent_line(pc, indent_column);
         }
         else if (pc->Is(CT_SEMICOLON))
         {
            if (  pc->flags.test(PCF_IN_FOR)
               && options::indent_semicolon_for_paren())
            {
               log_rule_B("indent_semicolon_for_paren");
               indent_column_set(frm.top().pc->column);

               log_rule_B("indent_first_for_expr");

               if (options::indent_first_for_expr())
               {
                  reindent_line(frm.top().pc->GetNext(),
                                indent_column + pc->Len() + 1);
               }
               LOG_FMT(LINDENT, "%s(%d): %zu] SEMICOLON => %zu [%s]\n",
                       __func__, __LINE__, pc->orig_line, indent_column, pc->Text());
               reindent_line(pc, indent_column);
            }
            else
            {
               log_rule_B("indent_ignore_semicolon");

               if (options::indent_ignore_semicolon())
               {
                  indent_column_set(pc->orig_col);
               }
               LOG_FMT(LINDENT, "%s(%d): %zu] semicolon => %zu [%s]\n",
                       __func__, __LINE__, pc->orig_line, indent_column, pc->Text());
               reindent_line(pc, indent_column);
            }
         }
         else if (pc->Is(CT_BOOL))
         {
            if (frm.top().pc->IsParenOpen())
            {
               log_rule_B("indent_bool_paren");

               if (options::indent_bool_paren() == (int)indent_mode_e::IGNORE)
               {
                  indent_column_set(pc->orig_col);
               }
               else if (options::indent_bool_paren() == (int)indent_mode_e::ALIGN)
               {
                  indent_column_set(frm.top().pc->column);

                  log_rule_B("indent_first_bool_expr");

                  if (options::indent_first_bool_expr())
                  {
                     reindent_line(frm.top().pc->GetNext(),
                                   indent_column + pc->Len() + 1);
                  }
               }
            }
            else
            {
               log_rule_B("indent_ignore_bool");

               if (options::indent_ignore_bool())
               {
                  indent_column_set(pc->orig_col);
               }
            }
            LOG_FMT(LINDENT, "%s(%d): %zu] bool => %zu [%s]\n",
                    __func__, __LINE__, pc->orig_line, indent_column, pc->Text());
            reindent_line(pc, indent_column);
         }
         else if (  pc->Is(CT_ARITH)
                 || pc->Is(CT_CARET))
         {
            log_rule_B("indent_ignore_arith");

            if (options::indent_ignore_arith())
            {
               indent_column_set(pc->orig_col);
            }
            LOG_FMT(LINDENT, "%s(%d): %zu] arith => %zu [%s]\n",
                    __func__, __LINE__, pc->orig_line, indent_column, pc->Text());
            reindent_line(pc, indent_column);
         }
         else if (pc->Is(CT_SHIFT))
         {
            log_rule_B("indent_shift");

            if (options::indent_shift() == -1)
            {
               indent_column_set(pc->orig_col);
            }
            LOG_FMT(LINDENT, "%s(%d): %zu] shift => %zu [%s]\n",
                    __func__, __LINE__, pc->orig_line, indent_column, pc->Text());
            reindent_line(pc, indent_column);
         }
         else if (pc->Is(CT_ASSIGN))
         {
            log_rule_B("indent_ignore_assign");

            if (options::indent_ignore_assign())
            {
               indent_column_set(pc->orig_col);
            }
            LOG_FMT(LINDENT, "%s(%d): %zu] assign => %zu [%s]\n",
                    __func__, __LINE__, pc->orig_line, indent_column, pc->Text());
            reindent_line(pc, indent_column);
         }
         else if (  options::indent_ternary_operator() == 1
                 && prev->Is(CT_COND_COLON)
                 && (  pc->Is(CT_ADDR)
                    || pc->Is(CT_WORD)
                    || pc->Is(CT_DEREF)
                    || pc->Is(CT_NUMBER)
                    || pc->Is(CT_STRING)
                    || pc->Is(CT_PAREN_OPEN)))
         {
            log_rule_B("indent_ternary_operator");
            Chunk *tmp = prev->GetPrevType(CT_QUESTION);

            if (tmp->IsNotNullChunk())
            {
               tmp = tmp->GetNextNcNnl();

               if (tmp->IsNotNullChunk())
               {
                  LOG_FMT(LINDENT, "%s: %zu] ternarydefcol => %zu [%s]\n",
                          __func__, pc->orig_line, tmp->column, pc->Text());
                  reindent_line(pc, tmp->column);
               }
            }
         }
         else if (  options::indent_ternary_operator() == 2
                 && pc->Is(CT_COND_COLON))
         {
            log_rule_B("indent_ternary_operator");
            Chunk *tmp = pc->GetPrevType(CT_QUESTION);

            if (tmp->IsNotNullChunk())
            {
               LOG_FMT(LINDENT, "%s: %zu] ternarydefcol => %zu [%s]\n",
                       __func__, pc->orig_line, tmp->column, pc->Text());
               reindent_line(pc, tmp->column);
            }
         }
         else if (  options::indent_oc_inside_msg_sel()
                 && (  pc->Is(CT_OC_MSG_FUNC)
                    || pc->Is(CT_OC_MSG_NAME))) // Issue #2658
         {
            log_rule_B("indent_oc_inside_msg_sel");
            reindent_line(pc, frm.top().indent);
         }
         else
         {
            bool         use_indent = true;
            const size_t ttidx      = frm.size() - 1;

            if (ttidx > 0)
            {
               LOG_FMT(LINDPC, "%s(%d): (frm.at(ttidx).pc)->parent_type is %s\n",
                       __func__, __LINE__, get_token_name((frm.at(ttidx).pc)->parent_type));

               if ((frm.at(ttidx).pc)->parent_type == CT_FUNC_CALL)
               {
                  LOG_FMT(LINDPC, "FUNC_CALL OK [%d]\n", __LINE__);

                  log_rule_B("use_indent_func_call_param");

                  if (options::use_indent_func_call_param())
                  {
                     LOG_FMT(LINDPC, "use is true [%d]\n", __LINE__);
                  }
                  else
                  {
                     LOG_FMT(LINDPC, "use is false [%d]\n", __LINE__);
                     use_indent = false;
                  }
               }
            }
            LOG_FMT(LINDENT, "%s(%d): pc->line is %zu, pc->column is %zu, pc->Text() is '%s, indent_column is %zu\n",
                    __func__, __LINE__, pc->orig_line, pc->column, pc->Text(), indent_column);

            if (  use_indent
               && pc->IsNot(CT_PP_IGNORE)) // Leave indentation alone for PP_IGNORE tokens
            {
               log_rule_B("pos_conditional");

               if (  (  pc->Is(CT_QUESTION)      // Issue #2101
                     || pc->Is(CT_COND_COLON))   // Issue #2101
                  && options::pos_conditional() == TP_IGNORE)
               {
                  // do not indent this line
                  LOG_FMT(LINDENT, "%s(%d): %zu] don't indent this line\n",
                          __func__, __LINE__, pc->orig_line);
               }
               else if (pc->Is(CT_BREAK))
               {
                  // Issue #1692
                  log_rule_B("indent_switch_break_with_case");

                  // Issue #2281
                  if (  options::indent_switch_break_with_case()
                     && get_type_of_the_parent(pc) == CT_SWITCH)
                  {
                     // look for a case before Issue #2735
                     Chunk *whereIsCase = pc->GetPrevType(CT_CASE, pc->level);

                     if (whereIsCase->IsNotNullChunk())
                     {
                        LOG_FMT(LINDENT, "%s(%d): orig_line is %zu, orig_col is %zu, Text() is '%s'\n",
                                __func__, __LINE__, whereIsCase->orig_line, whereIsCase->orig_col, whereIsCase->Text());
                        LOG_FMT(LINDENT, "%s(%d): column is %zu\n",
                                __func__, __LINE__, whereIsCase->column);
                        reindent_line(pc, whereIsCase->column);
                     }
                  }
                  else
                  {
                     LOG_FMT(LINDENT, "%s(%d): orig_line is %zu, indent_column set to %zu, for '%s'\n",
                             __func__, __LINE__, pc->orig_line, indent_column, pc->Text());
                     reindent_line(pc, indent_column);
                  }
               }
               else if (  pc->Is(CT_MEMBER)                      // Issue #2890
                       && language_is_set(LANG_CPP))
               {
                  // comment name: XXXXXXXXXXXXXXXXXXXXXXXXXX
                  LOG_FMT(LINDENT, "%s(%d): orig_line is %zu, indent_column set to %zu, for '%s'\n",
                          __func__, __LINE__, pc->orig_line, indent_column, pc->Text());
                  const size_t frm_size = frm.size();
                  LOG_FMT(LINDPC, "%s(%d): frm_size is %zu\n",
                          __func__, __LINE__, frm_size);
                  // get pc
                  LOG_FMT(LINDPC, "%s(%d): Text() is '%s', (frm.at(frm_size - 1).pc)->type is %s\n",
                          __func__, __LINE__, (frm.at(frm_size - 1).pc)->Text(), get_token_name((frm.at(frm_size - 1).pc)->type));
                  // get the token before
                  const size_t temp_ttidx = frm_size - 2;

                  if (temp_ttidx == 0)
                  {
                     indent_column = 1 + indent_size;
                     reindent_line(pc, indent_column);
                  }
                  else
                  {
                     Chunk *token_before = frm.at(temp_ttidx).pc;
                     LOG_FMT(LINDPC, "%s(%d): Text() is '%s', token_before->type is %s\n",
                             __func__, __LINE__, token_before->Text(), get_token_name(token_before->type));

                     size_t vor_col = 0;

                     if (token_before->Is(CT_ASSIGN))
                     {
                        Chunk *before_Assign = frm.at(temp_ttidx - 1).pc;

                        if (before_Assign->IsNullChunk())
                        {
                           indent_column = 1 + indent_size;
                        }
                        else
                        {
                           vor_col = before_Assign->column;
                           LOG_FMT(LINDPC, "%s(%d): Text() is '%s', before_Assign->type is %s, column is %zu\n",
                                   __func__, __LINE__, before_Assign->Text(), get_token_name(before_Assign->type), vor_col);
                           indent_column = vor_col + 2 * indent_size;
                        }
                     }
                     else if (token_before->Is(CT_BRACE_OPEN))
                     {
                        vor_col = token_before->column;
                        LOG_FMT(LINDPC, "%s(%d): Text() is '%s', token_before->type is %s, column is %zu\n",
                                __func__, __LINE__, token_before->Text(), get_token_name(token_before->type), vor_col);
                        indent_column = vor_col + 2 * indent_size;
                     }
                     else if (token_before->Is(CT_RETURN))
                     {
                        Chunk *before_Return = frm.at(temp_ttidx - 1).pc;
                        vor_col = before_Return->column;
                        LOG_FMT(LINDPC, "%s(%d): Text() is '%s', before_Return->type is %s, column is %zu\n",
                                __func__, __LINE__, before_Return->Text(), get_token_name(before_Return->type), vor_col);
                        indent_column = vor_col + 2 * indent_size;
                     }
                     else
                     {
                        // TO DO
                     }
                     reindent_line(pc, indent_column);
                  }
                  reindent_line(pc, indent_column);
               }
               else
               {
                  LOG_FMT(LINDENT, "%s(%d): orig_line is %zu, indent_column set to %zu, for '%s'\n",
                          __func__, __LINE__, pc->orig_line, indent_column, pc->Text());
                  reindent_line(pc, indent_column);
               }
            }
            else
            {
               // do not indent this line
               LOG_FMT(LINDENT, "%s(%d): %zu] don't indent this line\n",
                       __func__, __LINE__, pc->orig_line);
            }
         }
         did_newline = false;

         if (  pc->Is(CT_SQL_EXEC)
            || pc->Is(CT_SQL_BEGIN)
            || pc->Is(CT_SQL_END))
         {
            sql_col      = pc->column;
            sql_orig_col = pc->orig_col;
         }

         // Handle indent for variable defs at the top of a block of code
         if (pc->flags.test(PCF_VAR_TYPE))
         {
            if (  !frm.top().non_vardef
               && (frm.top().type == CT_BRACE_OPEN))
            {
               log_rule_B("indent_var_def_blk");
               const auto val = options::indent_var_def_blk();

               if (val != 0)
               {
                  auto indent = indent_column;
                  indent = (val > 0) ? val                     // reassign if positive val,
                           : (cast_abs(indent, val) < indent)  // else if no underflow
                           ? (indent + val) : 0;               // reduce, else 0

                  LOG_FMT(LINDENT, "%s(%d): %zu] var_type indent => %zu [%s]\n",
                          __func__, __LINE__, pc->orig_line, indent, pc->Text());
                  reindent_line(pc, indent);
               }
            }
         }
         else if (pc != frm.top().pc)
         {
            frm.top().non_vardef = true;
         }
      }

      // if we hit a newline, reset indent_tmp
      if (  pc->IsNewline()
         || pc->Is(CT_COMMENT_MULTI)
         || pc->Is(CT_COMMENT_CPP))
      {
         log_indent();
         frm.top().indent_tmp = frm.top().indent;
         log_indent_tmp();

         /*
          * Handle the case of a multi-line #define w/o anything on the
          * first line (indent_tmp will be 1 or 0)
          */
         if (  pc->Is(CT_NL_CONT)
            && (frm.top().indent_tmp <= indent_size))
         {
            frm.top().indent_tmp = indent_size + 1;
            log_indent_tmp();
         }
         // Get ready to indent the next item
         did_newline = true;
      }
      // Check for open XML tags "</..."
      log_rule_B("indent_xml_string");

      if (  options::indent_xml_string() > 0
         && pc->Is(CT_STRING)
         && pc->Len() > 4
         && pc->str[1] == '<'
         && pc->str[2] != '/'
         && pc->str[pc->Len() - 3] != '/')
      {
         if (xml_indent <= 0)
         {
            xml_indent = pc->column;
         }
         log_rule_B("indent_xml_string");
         xml_indent += options::indent_xml_string();
      }
      // Issue #672
      log_rule_B("indent_continue_class_head");

      if (  pc->Is(CT_CLASS)
         && language_is_set(LANG_CPP | LANG_JAVA)
         && (  options::indent_ignore_first_continue()
            || options::indent_continue_class_head() != 0)
         && !classFound)
      {
         LOG_FMT(LINDENT, "%s(%d): orig_line is %zu, CT_CLASS found, OPEN IT\n",
                 __func__, __LINE__, pc->orig_line);
         frm.push(pc, __func__, __LINE__);

         if (options::indent_ignore_first_continue())
         {
            frm.top().indent = get_indent_first_continue(pc);
         }
         else
         {
            frm.top().indent = frm.prev().indent + options::indent_continue_class_head();
         }
         log_indent();

         frm.top().indent_tmp = frm.top().indent;
         frm.top().indent_tab = frm.top().indent;
         log_indent_tmp();
         classFound = true;
      }
      pc = pc->GetNext();

      if (pc->Is(CT_SPACE))                       // Issue #3710
      {
         pc = pc->GetNext();
      }
      LOG_FMT(LINDLINE, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
   }
null_pc:

   // Throw out any stuff inside a preprocessor - no need to warn
   while (  !frm.empty()
         && frm.top().in_preproc)
   {
      frm.pop(__func__, __LINE__, pc);
   }

   // Throw out any VBRACE_OPEN at the end - implied with the end of file
   while (  !frm.empty()
         && frm.top().type == CT_VBRACE_OPEN)
   {
      frm.pop(__func__, __LINE__, pc);
   }

   for (size_t idx_temp = 1; idx_temp < frm.size(); idx_temp++)
   {
      LOG_FMT(LWARN, "%s(%d): size is %zu\n",
              __func__, __LINE__, frm.size());
      LOG_FMT(LWARN, "%s(%d): File: %s, open_line is %zu, parent is %s: Unmatched %s\n",
              __func__, __LINE__, cpd.filename.c_str(), frm.at(idx_temp).open_line,
              get_token_name(frm.at(idx_temp).parent),
              get_token_name(frm.at(idx_temp).type));
      cpd.error_count++;
   }

   LOG_FMT(LINDLINE, "%s(%d): before quick_align_again\n", __func__, __LINE__);
   quick_align_again();
   quick_indent_again();
   LOG_FMT(LINDLINE, "%s(%d): after quick_align_again\n", __func__, __LINE__);
} // indent_text


static bool single_line_comment_indent_rule_applies(Chunk *start, bool forward)
{
   LOG_FUNC_ENTRY();

   if (!start->IsSingleLineComment())
   {
      return(false);
   }
   Chunk  *pc      = start;
   size_t nl_count = 0;

   while ((pc = forward ? pc->GetNext() : pc->GetPrev())->IsNotNullChunk())
   {
      if (pc->IsNewline())
      {
         if (  nl_count > 0
            || pc->nl_count > 1)
         {
            return(false);
         }
         nl_count++;
      }
      else if (pc->IsSingleLineComment())
      {
         nl_count = 0;
      }
      else if (  pc->Is(CT_COMMENT_MULTI)
              || (forward && pc->IsBraceClose())
              || (!forward && pc->IsBraceOpen()))
      {
         /*
          * check for things we wouldn't want to indent the comment for
          * example: non-single line comment, closing brace
          */
         return(false);
      }
      else
      {
         return(true);
      }
   }
   return(false);
} // single_line_comment_indent_rule_applies


static bool is_end_of_assignment(Chunk *pc, const ParseFrame &frm)
{
   return(  (  frm.top().type == CT_ASSIGN_NL
            || frm.top().type == CT_MEMBER
            || frm.top().type == CT_ASSIGN)
         && (  pc->IsSemicolon()
            || pc->Is(CT_COMMA)
            || pc->Is(CT_BRACE_OPEN)
            || pc->Is(CT_SPAREN_CLOSE)
            || (  pc->Is(CT_SQUARE_OPEN)
               && get_chunk_parent_type(pc) == CT_ASSIGN))
         && get_chunk_parent_type(pc) != CT_CPP_LAMBDA);
}


static size_t calc_comment_next_col_diff(Chunk *pc)
{
   Chunk *next = pc; // assumes pc has a comment type

   LOG_FMT(LCMTIND, "%s(%d): next->Text() is '%s'\n",
           __func__, __LINE__, next->Text());

   // Note: every comment is squashed into a single token
   // (including newline chars for multiline comments) and is followed by
   // a newline token (unless there are no more tokens left)
   do
   {
      Chunk *newline_token = next->GetNext();
      LOG_FMT(LCMTIND, "%s(%d): newline_token->Text() is '%s', orig_line is %zu, orig_col is %zu\n",
              __func__, __LINE__, newline_token->Text(), newline_token->orig_line, newline_token->orig_col);

      if (  newline_token->IsNullChunk()
         || newline_token->nl_count > 1)
      {
         return(5000);  // FIXME: Max thresh magic number 5000
      }
      next = newline_token->GetNext();

      if (next->IsNotNullChunk())
      {
         LOG_FMT(LCMTIND, "%s(%d): next->Text() is '%s', orig_line is %zu, orig_col is %zu\n",
                 __func__, __LINE__, next->Text(), next->orig_line, next->orig_col);
      }
   } while (next->IsComment());

   if (next->IsNullChunk())
   {
      return(5000);     // FIXME: Max thresh magic number 5000
   }
   LOG_FMT(LCMTIND, "%s(%d): next->Text() is '%s'\n",
           __func__, __LINE__, next->Text());
   // here next is the first non comment, non newline token
   return(next->orig_col > pc->orig_col
          ? next->orig_col - pc->orig_col
          : pc->orig_col - next->orig_col);
}


static void indent_comment(Chunk *pc, size_t col)
{
   LOG_FUNC_ENTRY();
   char copy[1000];

   LOG_FMT(LCMTIND, "%s(%d): pc->Text() is '%s', orig_line %zu, orig_col %zu, level %zu\n",
           __func__, __LINE__, pc->ElidedText(copy), pc->orig_line, pc->orig_col, pc->level);

   // force column 1 comment to column 1 if not changing them
   log_rule_B("indent_col1_comment");

   if (  pc->orig_col == 1
      && !options::indent_col1_comment()
      && !pc->flags.test(PCF_INSERTED))
   {
      LOG_FMT(LCMTIND, "%s(%d): rule 1 - keep in col 1\n", __func__, __LINE__);
      reindent_line(pc, 1);
      return;
   }
   Chunk *nl = pc->GetPrev();

   if (nl->IsNotNullChunk())
   {
      LOG_FMT(LCMTIND, "%s(%d): nl->Text() is '%s', orig_line %zu, orig_col %zu, level %zu\n",
              __func__, __LINE__, nl->Text(), nl->orig_line, nl->orig_col, nl->level);
   }

   if (pc->orig_col > 1)
   {
      Chunk *prev = nl->GetPrev();

      if (prev->IsNotNullChunk())
      {
         LOG_FMT(LCMTIND, "%s(%d): prev->Text() is '%s', orig_line %zu, orig_col %zu, level %zu\n",
                 __func__, __LINE__, prev->Text(), prev->orig_line, prev->orig_col, prev->level);
         log_pcf_flags(LCMTIND, prev->flags);
      }

      if (  prev->IsComment()
         && nl->nl_count == 1)
      {
         const size_t prev_col_diff = (prev->orig_col > pc->orig_col)
                                      ? prev->orig_col - pc->orig_col
                                      : pc->orig_col - prev->orig_col;
         LOG_FMT(LCMTIND, "%s(%d): prev_col_diff is %zu\n",
                 __func__, __LINE__, prev_col_diff);

         /*
          * Here we want to align comments that are relatively close one to
          * another but not when the comment is a Doxygen comment (Issue #1134)
          */
         if (prev_col_diff <= options::indent_comment_align_thresh())
         {
            LOG_FMT(LCMTIND, "%s(%d): prev->Text() is '%s', Doxygen_comment(prev) is %s\n",
                    __func__, __LINE__, prev->Text(), prev->IsDoxygenComment() ? "TRUE" : "FALSE");
            LOG_FMT(LCMTIND, "%s(%d): pc->Text() is '%s', Doxygen_comment(pc) is %s\n",
                    __func__, __LINE__, pc->Text(), pc->IsDoxygenComment() ? "TRUE" : "FALSE");

            if (prev->IsDoxygenComment() == pc->IsDoxygenComment())
            {
               const size_t next_col_diff = calc_comment_next_col_diff(pc);
               LOG_FMT(LCMTIND, "%s(%d): next_col_diff is %zu\n",
                       __func__, __LINE__, next_col_diff);

               // Align to the previous comment or to the next token?
               if (  prev_col_diff <= next_col_diff
                  || next_col_diff == 5000) // FIXME: Max thresh magic number 5000
               {
                  LOG_FMT(LCMTIND, "%s(%d): rule 3 - prev comment, coldiff = %zu, now in %zu\n",
                          __func__, __LINE__, prev_col_diff, pc->column);
                  reindent_line(pc, prev->column);
                  return;
               }
            }
         }
      }
   }
   // check if special single-line-comment-before-code rule applies
   log_rule_B("indent_single_line_comments_before");

   if (  (options::indent_single_line_comments_before() > 0)
      && single_line_comment_indent_rule_applies(pc, true))
   {
      LOG_FMT(LCMTIND, "%s(%d): rule 4 - indent single line comments before code, now in %zu\n",
              __func__, __LINE__, pc->column);
      reindent_line(pc, col + options::indent_single_line_comments_before());
      return;
   }
   // check if special single-line-comment-after-code rule applies
   log_rule_B("indent_single_line_comments_after");

   if (  (options::indent_single_line_comments_after() > 0)
      && single_line_comment_indent_rule_applies(pc, false))
   {
      LOG_FMT(LCMTIND, "%s(%d): rule 4 - indent single line comments after code, now in %zu\n",
              __func__, __LINE__, pc->column);
      reindent_line(pc, col + options::indent_single_line_comments_after());
      return;
   }
   log_rule_B("indent_comment");

   if (  pc->orig_col > 1
      && !options::indent_comment())
   {
      LOG_FMT(LCMTIND, "%s(%d): rule 5 - keep in orig_col\n", __func__, __LINE__);
      reindent_line(pc, pc->orig_col);
      return;
   }
   LOG_FMT(LCMTIND, "%s(%d): rule 6 - fall-through, stay in %zu\n",
           __func__, __LINE__, col);
   reindent_line(pc, col);
} // indent_comment


bool ifdef_over_whole_file()
{
   LOG_FUNC_ENTRY();

   // if requested, treat an #if that guards the entire file the same as any other #if
   // if running as frag, assume #if is not a guard
   if (  options::pp_indent_in_guard()
      || cpd.frag)
   {
      return(false);
   }

   // the results for this file are cached
   if (cpd.ifdef_over_whole_file)
   {
      return(cpd.ifdef_over_whole_file > 0);
   }
   Chunk  *start_pp = Chunk::NullChunkPtr;
   Chunk  *end_pp   = Chunk::NullChunkPtr;
   size_t IFstage   = 0;

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      LOG_FMT(LNOTE, "%s(%d): pc->pp_level is %zu, pc->orig_line is %zu, pc->orig_col is %zu, pc->Text() is '%s'\n",
              __func__, __LINE__, pc->pp_level, pc->orig_line, pc->orig_col, pc->Text());

      if (pc->IsCommentOrNewline())
      {
         continue;
      }

      if (IFstage == 0)                   // 0 is BEGIN
      {
         // Check the first preprocessor, make sure it is an #if type
         if (pc->IsNot(CT_PREPROC))
         {
            break;
         }
         Chunk *next = pc->GetNext();

         if (  next->IsNullChunk()
            || next->IsNot(CT_PP_IF))
         {
            break;
         }
         IFstage  = 1;                      // 1 is CT_PP_IF found
         start_pp = pc;
      }
      else if (IFstage == 1)                // 1 is CT_PP_IF found
      {
         // Scan until a preprocessor at level 0 is found - the close to the #if
         if (pc->Is(CT_PREPROC))
         {
            if (pc->pp_level == 0)
            {
               IFstage = 2;
               end_pp  = pc;
            }
         }
         continue;
      }
      else if (IFstage == 2)
      {
         // We should only see the rest of the preprocessor
         if (  pc->Is(CT_PREPROC)
            || !pc->flags.test(PCF_IN_PREPROC))
         {
            IFstage = 0;
            break;
         }
      }
   }

   cpd.ifdef_over_whole_file = (IFstage == 2) ? 1 : -1;

   if (cpd.ifdef_over_whole_file > 0)
   {
      chunk_flags_set(start_pp, PCF_WF_IF);
      chunk_flags_set(end_pp, PCF_WF_ENDIF);
   }
   LOG_FMT(LNOTE, "The whole file is%s covered by a #IF\n",
           (cpd.ifdef_over_whole_file > 0) ? "" : " NOT");
   return(cpd.ifdef_over_whole_file > 0);
} // ifdef_over_whole_file


void indent_preproc()
{
   LOG_FUNC_ENTRY();

   // Scan to see if the whole file is covered by one #ifdef
   const size_t pp_level_sub = ifdef_over_whole_file() ? 1 : 0;

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      LOG_FMT(LPPIS, "%s(%d): orig_line is %zu, orig_col is %zu, pc->Text() is '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());

      if (pc->IsNot(CT_PREPROC))
      {
         continue;
      }
      Chunk *next = pc->GetNextNcNnl();

      if (next->IsNullChunk())
      {
         break;
      }
      const size_t pp_level = (pc->pp_level > pp_level_sub)
                              ? pc->pp_level - pp_level_sub : 0;

      // Adjust the indent of the '#'
      log_rule_B("pp_indent");

      if (options::pp_indent() & IARF_ADD)
      {
         log_rule_B("pp_indent_count");
         reindent_line(pc, 1 + pp_level * options::pp_indent_count());
      }
      else if (options::pp_indent() & IARF_REMOVE)
      {
         log_rule_B("pp_indent");
         reindent_line(pc, 1);
      }
      // Add spacing by adjusting the length
      log_rule_B("pp_space");

      if (  (options::pp_space() != IARF_IGNORE)
         && next != nullptr
         && next->IsNotNullChunk())
      {
         if (options::pp_space() & IARF_ADD)
         {
            log_rule_B("pp_space_count");
            // Issue #3055
            const size_t mult = max<size_t>(options::pp_space_count(), 1);
            reindent_line(next, pc->column + pc->Len() + (pp_level * mult));
         }
         else if (options::pp_space() & IARF_REMOVE)
         {
            log_rule_B("pp_space");
            reindent_line(next, pc->column + pc->Len());
         }
      }
      // Mark as already handled if not region stuff or in column 1
      log_rule_B("pp_indent_at_level");

      bool at_file_level = pc->brace_level <= ((get_chunk_parent_type(pc) == CT_PP_DEFINE) ? 1 : 0);

      if (  (  (  at_file_level
               && !options::pp_indent_at_level0())
            || (  !at_file_level
               && !options::pp_indent_at_level()))
         && get_chunk_parent_type(pc) != CT_PP_REGION
         && get_chunk_parent_type(pc) != CT_PP_ENDREGION)
      {
         log_rule_B("pp_define_at_level");

         if (  !options::pp_define_at_level()
            || get_chunk_parent_type(pc) != CT_PP_DEFINE)
         {
            chunk_flags_set(pc, PCF_DONT_INDENT);
         }
      }
      LOG_FMT(LPPIS, "%s(%d): orig_line %zu to %zu (len %zu, next->col %zu)\n",
              __func__, __LINE__, pc->orig_line, 1 + pp_level, pc->Len(),
              next ? next->column : -1);
   }
} // indent_preproc
