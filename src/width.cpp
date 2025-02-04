/**
 * @file width.cpp
 * Limits line width.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "width.h"

#include "indent.h"
#include "newlines/add.h"
#include "newlines/cleanup.h"
#include "newlines/one_liner.h"
#include "prototypes.h"
#include "reindent_line.h"


constexpr static auto LCURRENT = LSPLIT;

using namespace uncrustify;


/**
 * abbreviations used:
 * - fparen = function parenthesis
 */

struct SplitEntry
{
   Chunk  *pc;
   size_t pri;

   SplitEntry()
      : pc(Chunk::NullChunkPtr)
      , pri(0) {}
};


struct TokenPriority
{
   E_Token tok;
   size_t  pri;
};


static inline bool is_past_width(Chunk *pc);


//! Split right after the chunk
static void split_before_chunk(Chunk *pc);


static size_t get_split_pri(E_Token tok);


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
static void try_split_here(SplitEntry &ent, Chunk *pc);


/**
 * Scan backwards to find the most appropriate spot to split the line
 * and insert a newline.
 *
 * See if this needs special function handling.
 * Scan backwards and find the best token for the split.
 *
 * @param start The first chunk that exceeded the limit
 */
static bool split_line(Chunk *pc);


/**
 * Figures out where to split a template
 *
 *
 * @param start   the offending token
 */
static void split_template(Chunk *start);


/**
 * Splits the parameters at every comma that is at the fparen level.
 *
 * @param start   the offending token
 */
static void split_fcn_params_full(Chunk *start);


/**
 * Splits line on every boolean expression at the top level and then
 * recursively on nested levels in case splitted lines are still too long.
 * @param start
 */
static void split_bool_expr(Chunk *start);


/**
 * A for statement is too long.
 * Step backwards and forwards to find the semicolons
 * Try splitting at the semicolons first.
 * If that doesn't work, then look for a comma at paren level.
 * If that doesn't work, then look for an assignment at paren level.
 * If that doesn't work, then give up.
 */
static void split_for_stmt(Chunk *start);


static inline bool is_past_width(Chunk *pc)
{
   // allow char to sit at last column by subtracting 1
   size_t currCol    = pc->GetColumn() + pc->Len() - 1;
   bool   past_width = currCol > options::code_width();

   LOG_FMT(LSPLIT, "%s(%d): orig line %zu, orig col %zu, curr col %zu, text '%s', past width %s\n",
           __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), currCol, pc->Text(),
           past_width ? "YES" : "NO");
   return(past_width);
}


static void split_before_chunk(Chunk *pc)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LSPLIT, "%s(%d): Text() '%s'\n", __func__, __LINE__, pc->Text());

   Chunk *prev = pc->GetPrev();

   if (  !pc->IsNewline()
      && !prev->IsNewline())
   {
      newline_add_before(pc);
      // Mark chunk as continuation line, so indentation can be
      // correctly set over multiple passes
      pc->SetFlagBits(PCF_CONT_LINE);

      // Mark open and close parens as continuation line chunks.
      // This will prevent an additional level and frame to be
      // added to the current frame stack (issue 3105).
      if (  prev->Is(CT_PAREN_OPEN)
         || prev->Is(CT_LPAREN_OPEN)
         || prev->Is(CT_SPAREN_OPEN)
         || prev->Is(CT_FPAREN_OPEN)
         || prev->Is(CT_SQUARE_OPEN)
         || prev->Is(CT_ANGLE_OPEN))
      {
         LOG_FMT(LSPLIT, "%s(%d): set PCF_LINE_CONT for prev text '%s', orig line is %zu, orig col is %zu\n",
                 __func__, __LINE__, prev->Text(), prev->GetOrigLine(), prev->GetOrigCol());

         prev->SetFlagBits(PCF_CONT_LINE);
         Chunk *closing_paren = prev->GetClosingParen();

         if (closing_paren->IsNotNullChunk())
         {
            closing_paren->SetFlagBits(PCF_CONT_LINE);
         }
      }
      // reindent needs to include the indent_continue value and was off by one
      log_rule_B("indent_columns");
      log_rule_B("indent_continue");
      reindent_line(pc, pc->GetBraceLevel() * options::indent_columns() +
                    abs(options::indent_continue()) + 1);
      cpd.changes++;
   }
} // split_before_chunk


static TokenPriority pri_table[] =
{
   { CT_SEMICOLON,    1 },
   { CT_COMMA,        2 },
   { CT_BOOL,         3 },
   { CT_COMPARE,      4 },
   { CT_SHIFT,        5 },
   { CT_ARITH,        6 },
   { CT_CARET,        7 },
   { CT_ASSIGN,       9 },
   { CT_STRING,      10 },
   { CT_FOR_COLON,   11 },
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


void do_code_width()
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LSPLIT, "%s(%d)\n", __func__, __LINE__);

   // If indent_continue is negative, we want to look for long lines splits,
   // so raise CT_FPAREN_OPEN priority to get better results.
   if (options::indent_continue() < 0)
   {
      for (TokenPriority &token : pri_table)
      {
         if (token.tok == CT_FPAREN_OPEN)
         {
            token.pri = 8; // Before assignment priority
            break;
         }
      }
   }

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (  !pc->IsCommentOrNewline()
         && pc->IsNot(CT_SPACE)
         && is_past_width(pc))
      {
         if (  pc->Is(CT_VBRACE_CLOSE)  // don't break if a vbrace close
            && pc->IsLastChunkOnLine()) // is the last chunk on its line
         {
            continue;
         }
         bool split_OK = split_line(pc);

         if (split_OK)
         {
            // After a line split it is necessary to reindent the text before considering
            // further splits. This avoid splits out of position which would produce an
            // awkward result in some cases when a line needs more that one split.
            // After the split, skip over the remainder of the line.
            Chunk *nextNl = pc->GetNextNl();
            LOG_FMT(LSPLIT, "%s(%d): orig line %zu, orig col %zu, text '%s' splitted\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text());
            pc = nextNl;
         }
         else
         {
            LOG_FMT(LSPLIT, "%s(%d): orig line %zu, orig col %zu, text '%s' not splitted\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text());
            break;
         }
      }
   }
} // do_code_width


static size_t get_split_pri(E_Token tok)
{
   for (TokenPriority token : pri_table)
   {
      if (token.tok == tok)
      {
         return(token.pri);
      }
   }

   return(100); // Bigger than any valid priority
}


static void try_split_here(SplitEntry &ent, Chunk *pc)
{
   LOG_FUNC_ENTRY();

   size_t pc_pri = get_split_pri(pc->GetType());
   LOG_FMT(LSPLIT, "%s(%d): text '%s', orig col %zu pc_pri %zu\n",
           __func__, __LINE__, pc->Text(), pc->GetOrigCol(), pc_pri);

   if (pc_pri == 0)
   {
      LOG_FMT(LSPLIT, "%s(%d): pc_pri is 0, return\n", __func__, __LINE__);
      return;
   }
   // Can't split after a newline
   Chunk *prev = pc->GetPrev();

   if (  prev->IsNullChunk()
      || (  prev->IsNewline()
         && pc->IsNot(CT_STRING)))
   {
      if (prev->IsNotNullChunk())
      {
         LOG_FMT(LSPLIT, "%s(%d): can't split after a newline, orig line is %zu, return\n",
                 __func__, __LINE__, prev->GetOrigLine());
      }
      return;
   }

   // Can't split a function without arguments
   if (pc->Is(CT_FPAREN_OPEN))
   {
      Chunk *next = pc->GetNext();

      if (next->Is(CT_FPAREN_CLOSE))
      {
         LOG_FMT(LSPLIT, "%s(%d): can't split a function without arguments, return\n", __func__, __LINE__);
         return;
      }
   }

   // Only split concatenated strings
   if (pc->Is(CT_STRING))
   {
      Chunk *next = pc->GetNext();

      if (next->IsNot(CT_STRING))
      {
         LOG_FMT(LSPLIT, "%s(%d): only split concatenated strings, return\n", __func__, __LINE__);
         return;
      }
   }

   // keep common groupings unless ls_code_width
   if (  !options::ls_code_width()
      && pc_pri >= 22)
   {
      LOG_FMT(LSPLIT, "%s(%d): keep common groupings unless ls_code_width, return\n", __func__, __LINE__);
      return;
   }

   // don't break after last term of a qualified type
   if (pc_pri == 25)
   {
      Chunk *next = pc->GetNext();

      if (  next->IsNot(CT_WORD)
         && (get_split_pri(next->GetType()) != 25))
      {
         LOG_FMT(LSPLIT, "%s(%d): don't break after last term of a qualified type, return\n", __func__, __LINE__);
         return;
      }
   }

   if (  options::nl_bool_expression_multiline()
      && (  options::pos_bool() == TP_LEAD
         || options::pos_bool() == TP_TRAIL))
   {
      // comma has a higher priority than bool, but in case nl_bool_expression_multiline is turned on
      // we want to break on bool if it is less nested than comma, so here the priority inversion is handled
      if (  pc->Is(CT_COMMA)
         && ent.pc->Is(CT_BOOL)
         && ent.pc->GetLevel() < pc->GetLevel())
      {
         LOG_FMT(LSPLIT, "%s(%d): don't break after comma if there is a bool on less nested level\n", __func__, __LINE__);
         return;
      }

      if (  pc->Is(CT_BOOL)
         && ent.pc->Is(CT_COMMA)
         && ent.pc->GetLevel() > pc->GetLevel())
      {
         LOG_FMT(LSPLIT, "%s(%d): found possible split on bool, because comma is on more nested level\n",
                 __func__, __LINE__);
         return;
      }
   }

   if (  ent.pc->IsNullChunk()
      || pc_pri < ent.pri
      || (  pc_pri == ent.pri
         && pc->IsNot(CT_FPAREN_OPEN)
         && pc->GetLevel() < ent.pc->GetLevel()))
   {
      LOG_FMT(LSPLIT, "%s(%d): found possible split\n", __func__, __LINE__);
      ent.pc  = pc;
      ent.pri = pc_pri;
   }
} // try_split_here


static bool split_line(Chunk *start)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LSPLIT, "%s(%d): start text '%s', orig line %zu, orig col %zu, type %s\n",
           __func__, __LINE__, start->Text(), start->GetOrigLine(), start->GetOrigCol(), get_token_name(start->GetType()));
   LOG_FMT(LSPLIT, "   flags ");
   log_pcf_flags(LSPLIT, start->GetFlags());
   LOG_FMT(LSPLIT, "   parent type %s, (PCF_IN_FCN_DEF %s), (PCF_IN_FCN_CALL %s)\n",
           get_token_name(start->GetParentType()),
           start->TestFlags((PCF_IN_FCN_DEF)) ? "TRUE" : "FALSE",
           start->TestFlags((PCF_IN_FCN_CALL)) ? "TRUE" : "FALSE");

   // break at maximum line length if ls_code_width is true
   // Issue #2432
   if (start->TestFlags(PCF_ONE_LINER))
   {
      LOG_FMT(LSPLIT, "%s(%d): ** ONE LINER SPLIT **\n", __func__, __LINE__);
      undo_one_liner(start);
      newlines_cleanup_braces(false);
      // Issue #1352
      cpd.changes++;
      return(false);
   }

   if (options::ls_code_width())
   {
      log_rule_B("ls_code_width");
   }
   else if (start->TestFlags(PCF_IN_FOR))
   {
      // Check to see if we are in a for statement
      LOG_FMT(LSPLIT, " ** FOR SPLIT **\n");
      split_for_stmt(start);

      if (!is_past_width(start))
      {
         return(true);
      }
      LOG_FMT(LSPLIT, "%s(%d): for split didn't work\n", __func__, __LINE__);
   }
   else if (  start->TestFlags(PCF_IN_FCN_DEF)
           || start->TestFlags(PCF_IN_FCN_CALL)
           || start->GetParentType() == CT_FUNC_PROTO)            // Issue #1169
   {
      /*
       * If this is in a function call or prototype, split on commas or right
       * after the open parenthesis
       */
      LOG_FMT(LSPLIT, " ** FUNC SPLIT **\n");

      if (options::ls_func_split_full())
      {
         log_rule_B("ls_func_split_full");

         split_fcn_params_full(start);

         if (!is_past_width(start))
         {
            return(true);
         }
      }
   }
   else if (start->TestFlags(PCF_IN_TEMPLATE))
   {
      /*
       * If this is in a template, split on commas, Issue #1170
       */
      LOG_FMT(LSPLIT, " ** TEMPLATE SPLIT **\n");
      split_template(start);
      return(true);
   }
   // Try to find the best spot to split the line
   LOG_FMT(LSPLIT, "%s(%d): try to find a split point\n", __func__, __LINE__);
   SplitEntry ent;
   ent.pc  = Chunk::NullChunkPtr;
   ent.pri = CT_UNKNOWN;

   Chunk *pc = start->GetPrev();
   Chunk *prev;

   while (  pc->IsNotNullChunk()
         && !pc->IsNewline())
   {
      LOG_FMT(LSPLIT, "%s(%d): text '%s', orig line is %zu, orig col is %zu\n",
              __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());

      if (pc->IsNot(CT_SPACE))
      {
         try_split_here(ent, pc);

         // break at maximum line length
         if (  ent.pc->IsNotNullChunk()
            && options::ls_code_width())
         {
            log_rule_B("ls_code_width");
            LOG_FMT(LSPLIT, "%s(%d): found split\n", __func__, __LINE__);
            break;
         }
      }
      pc = pc->GetPrev();
   }

   if (ent.pc->IsNullChunk())
   {
      LOG_FMT(LSPLIT, "%s(%d):    TRY_SPLIT yielded NO SOLUTION for orig line %zu at '%s' [%s]\n",
              __func__, __LINE__, start->GetOrigLine(), start->Text(), get_token_name(start->GetType()));
   }
   else
   {
      LOG_FMT(LSPLIT, "%s(%d):    TRY_SPLIT yielded '%s' [%s] on orig line %zu\n",
              __func__, __LINE__, ent.pc->Text(), get_token_name(ent.pc->GetType()), ent.pc->GetOrigLine());
      LOG_FMT(LSPLIT, "%s(%d): ent at '%s', orig col is %zu\n",
              __func__, __LINE__, ent.pc->Text(), ent.pc->GetOrigCol());
   }

   // Break before the token instead of after it according to the pos_xxx rules
   if (ent.pc->IsNullChunk())
   {
      pc = Chunk::NullChunkPtr;
   }
   else
   {
      log_rule_B("pos_arith");
      log_rule_B("pos_assign");
      log_rule_B("pos_compare");
      log_rule_B("pos_conditional");
      log_rule_B("pos_shift");
      log_rule_B("pos_bool");

      if (ent.pc->Is(CT_BOOL))
      {
         if (  options::nl_bool_expression_multiline()
            && (  options::pos_bool() == TP_LEAD
               || options::pos_bool() == TP_TRAIL))
         {
            split_bool_expr(ent.pc);
            return(true);
         }
      }

      if (  (  ent.pc->Is(CT_SHIFT)
            && (options::pos_shift() & TP_LEAD))
         || (  (  ent.pc->Is(CT_ARITH)
               || ent.pc->Is(CT_CARET))
            && (options::pos_arith() & TP_LEAD))
         || (  ent.pc->Is(CT_ASSIGN)
            && (options::pos_assign() & TP_LEAD))
         || (  ent.pc->Is(CT_COMPARE)
            && (options::pos_compare() & TP_LEAD))
         || (  (  ent.pc->Is(CT_COND_COLON)
               || ent.pc->Is(CT_QUESTION))
            && (options::pos_conditional() & TP_LEAD))
         || (  ent.pc->Is(CT_BOOL)
            && (options::pos_bool() & TP_LEAD)))
      {
         pc = ent.pc;
      }
      else
      {
         pc = ent.pc->GetNext();
      }
      LOG_FMT(LSPLIT, "%s(%d): at '%s', orig col is %zu\n",
              __func__, __LINE__, pc->Text(), pc->GetOrigCol());
   }
   bool move_forward = false;

   if (pc->IsNullChunk())
   {
      pc = start;
      Chunk *next = pc->GetNext();

      // On comma, close parenthesis/bracket/braces or semicolon, empty
      // parenthesis/brackets/braces pairs, skip after them since they
      // are small chunks and the split looks much better.
      if (  pc->Is(CT_COMMA)
         || pc->IsSemicolon()
         || pc->IsParenClose()
         || pc->IsBraceClose()
         || pc->Is(CT_ANGLE_CLOSE)
         || pc->Is(CT_SQUARE_CLOSE)
         || (  pc->IsParenOpen()
            && next->IsParenClose())
         || (  pc->IsBraceOpen()
            && next->IsBraceClose())
         || (  pc->Is(CT_SQUARE_OPEN)
            && next->Is(CT_SQUARE_CLOSE))
         || pc->Len() == 0)
      {
         LOG_FMT(LSPLIT, "Move forward past the chuck");
         move_forward = true;
      }
   }
   // Add a newline before 'pc', if possible.
   // If 'pc' is the first chunk in the line, split after it since it is
   // already past the allowed code width. If the next chunk is a comma, close
   // parenthesis/bracket/braces or semicolon, skip after them since they
   // are small chunks and the split looks much better. Also skip over
   // empty parenthesis/brackets/braces pairs.
   prev = pc->GetPrev();

   if (  prev->IsNullChunk()
      || pc->IsNewline()
      || prev->IsNewline()
      || move_forward)
   {
      pc = pc->GetNext();
      Chunk *next = pc->GetNext();

      while (  pc->Is(CT_COMMA)
            || pc->IsSemicolon()
            || pc->IsParenClose()
            || pc->IsBraceClose()
            || pc->Is(CT_ANGLE_CLOSE)
            || pc->Is(CT_SQUARE_CLOSE)
            || (  pc->IsParenOpen()
               && next->IsParenClose())
            || (  pc->IsBraceOpen()
               && next->IsBraceClose())
            || (  pc->Is(CT_SQUARE_OPEN)
               && next->Is(CT_SQUARE_CLOSE)))
      {
         pc   = next;
         next = pc->GetNext();
      }
   }
   prev = pc->GetPrev();

   if (  prev->IsNotNullChunk()
      && !pc->IsNewline()
      && !prev->IsNewline())
   {
      //int plen = (pc->Len() < 5) ? pc->Len() : 5;
      //int slen = (start->Len() < 5) ? start->Len() : 5;
      //LOG_FMT(LSPLIT, " '%.*s' [%s], started on token '%.*s' [%s]\n",
      //        plen, pc->Text(), get_token_name(pc->GetType()),
      //        slen, start->Text(), get_token_name(start->GetType()));
      LOG_FMT(LSPLIT, "%s(%d): Text() '%s', type %s, started on token '%s', type %s\n",
              __func__, __LINE__, pc->Text(), get_token_name(pc->GetType()),
              start->Text(), get_token_name(start->GetType()));

      split_before_chunk(pc);
   }
   return(true);
} // split_line


/*
 * The for statement split algorithm works as follows:
 *   1. Step backwards and forwards to find the semicolons
 *   2. Try splitting at the semicolons first.
 *   3. If that doesn't work, then look for a comma at paren level.
 *   4. If that doesn't work, then look for an assignment at paren level.
 *   5. If that doesn't work, then give up.
 */
static void split_for_stmt(Chunk *start)
{
   LOG_FUNC_ENTRY();
   // how many semicolons (1 or 2) do we need to find
   log_rule_B("ls_for_split_full");
   size_t max_cnt     = options::ls_for_split_full() ? 2 : 1;
   Chunk  *open_paren = Chunk::NullChunkPtr;
   size_t nl_cnt      = 0;

   LOG_FMT(LSPLIT, "%s: starting on %s, line %zu\n",
           __func__, start->Text(), start->GetOrigLine());

   // Find the open paren so we know the level and count newlines
   Chunk *pc = start;

   while ((pc = pc->GetPrev())->IsNotNullChunk())
   {
      if (pc->Is(CT_SPAREN_OPEN))
      {
         open_paren = pc;
         break;
      }

      if (pc->GetNlCount() > 0)
      {
         nl_cnt += pc->GetNlCount();
      }
   }

   if (open_paren->IsNullChunk())
   {
      LOG_FMT(LSPLIT, "No open paren\n");
      return;
   }
   // see if we started on the semicolon
   int   count = 0;
   Chunk *st[2];

   pc = start;

   if (  pc->Is(CT_SEMICOLON)
      && pc->GetParentType() == CT_FOR)
   {
      st[count++] = pc;
   }

   // first scan backwards for the semicolons
   while (  (count < static_cast<int>(max_cnt))
         && ((pc = pc->GetPrev())->IsNotNullChunk())
         && pc->IsNotNullChunk()
         && pc->TestFlags(PCF_IN_SPAREN))
   {
      if (  pc->Is(CT_SEMICOLON)
         && pc->GetParentType() == CT_FOR)
      {
         st[count++] = pc;
      }
   }
   // And now scan forward
   pc = start;

   while (  (count < static_cast<int>(max_cnt))
         && ((pc = pc->GetNext())->IsNotNullChunk())
         && pc->TestFlags(PCF_IN_SPAREN))
   {
      if (  pc->Is(CT_SEMICOLON)
         && pc->GetParentType() == CT_FOR)
      {
         st[count++] = pc;
      }
   }

   while (--count >= 0)
   {
      // TODO: st[0] may be uninitialized here
      LOG_FMT(LSPLIT, "%s(%d): split before %s\n", __func__, __LINE__, st[count]->Text());
      split_before_chunk(st[count]->GetNext());
   }

   if (  !is_past_width(start)
      || nl_cnt > 0)
   {
      return;
   }
   // Still past width, check for commas at parentheses level
   pc = open_paren;

   while ((pc = pc->GetNext()) != start)
   {
      if (  pc->Is(CT_COMMA)
         && (pc->GetLevel() == (open_paren->GetLevel() + 1)))
      {
         split_before_chunk(pc->GetNext());

         if (!is_past_width(pc))
         {
            return;
         }
      }
   }
   // Still past width, check for a assignments at parentheses level
   pc = open_paren;

   while ((pc = pc->GetNext()) != start)
   {
      if (  pc->Is(CT_ASSIGN)
         && (pc->GetLevel() == (open_paren->GetLevel() + 1)))
      {
         split_before_chunk(pc->GetNext());

         if (!is_past_width(pc))
         {
            return;
         }
      }
   }
   // Oh, well. We tried.
} // split_for_stmt


static void split_fcn_params_full(Chunk *start)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LSPLIT, "%s(%d): start at '%s'\n", __func__, __LINE__, start->Text());

   // Find the opening function parenthesis
   Chunk *fpo = start;

   LOG_FMT(LSPLIT, "  %s(%d): search for opening function parenthesis\n", __func__, __LINE__);

   while ((fpo = fpo->GetPrev())->IsNotNullChunk())
   {
      LOG_FMT(LSPLIT, "  %s(%d): %s, orig col is %zu, level is %zu\n",
              __func__, __LINE__, fpo->Text(), fpo->GetOrigCol(), fpo->GetLevel());

      if (  fpo->Is(CT_FPAREN_OPEN)
         && (fpo->GetLevel() == start->GetLevel() - 1))
      {
         LOG_FMT(LSPLIT, "  %s(%d): found open paren\n", __func__, __LINE__);
         break;  // opening parenthesis found. Issue #1020
      }
   }
   // Now break after every comma
   Chunk *pc = fpo->GetNextNcNnl();

   while (pc->IsNotNullChunk())
   {
      if (pc->GetLevel() <= fpo->GetLevel())
      {
         break;
      }

      if (  (pc->GetLevel() == (fpo->GetLevel() + 1))
         && pc->Is(CT_COMMA))
      {
         split_before_chunk(pc->GetNext());
      }
      pc = pc->GetNextNcNnl();
   }
}


void split_bool_expr(Chunk *start)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LSPLIT, "%s(%d): start at '%s'\n", __func__, __LINE__, start->Text());
   bool   lead = options::pos_bool() & TP_LEAD ? true : false;

   Chunk  *pc                 = start;
   Chunk  *last_operator_bool = pc;
   Chunk  *return_statement   = Chunk::NullChunkPtr;

   size_t top_level = start->GetLevel();

   LOG_FMT(LSPLIT, "  %s(%d): search for boolean operators\n", __func__, __LINE__);

   // find last boolean operator on the line
   while (  (pc = pc->GetNext())->IsNotNullChunk()
         && !pc->IsNewline())
   {
      LOG_FMT(LSPLIT, "  %s(%d): %s, orig col is %zu, level is %zu\n",
              __func__, __LINE__, pc->Text(), pc->GetOrigCol(), pc->GetLevel());

      if (pc->Is(CT_BOOL))
      {
         last_operator_bool = pc;
      }
   }
   pc = last_operator_bool;

   // go through all bool operators on the same line to find out which level is top level
   do
   {
      // in case this operator is preceeded/succeeded by newline, it is already broken on it, skip it
      if (pc->Is(CT_BOOL))
      {
         bool already_broken = lead ? pc->GetPrev()->IsNewline() : pc->GetNext()->IsNewline();

         if (!already_broken && pc->GetLevel() < top_level)
         {
            top_level = pc->GetLevel();
         }
      }

      if (pc->Is(CT_RETURN))
      {
         return_statement = pc;
      }
   } while (  (pc = pc->GetPrev())->IsNotNullChunk()
           && !pc->IsNewline());

   // pass operators in reverse order and split on each of them if they are top level
   pc = last_operator_bool;

   do
   {
      if (  pc->Is(CT_BOOL)
         && pc->GetLevel() == top_level)
      {
         // in case of TP_TRAIL_BREAK and last operator is already at the end of line, skip it
         if (  !lead
            && pc->GetNext()->IsNewline())
         {
            continue;
         }
         Chunk *split_point = lead ? pc : pc->GetNext();
         split_before_chunk(split_point);
         pc = split_point->GetPrev(); // this yealds newline that we added that we need to skip

         if (  return_statement->IsNotNullChunk()
            && return_statement->GetLevel() == top_level)
         {
            split_point->ResetFlagBits(PCF_CONT_LINE);
         }

         // split previous line if too long
         if (is_past_width(pc->GetPrev()))
         {
            split_line(pc->GetPrev());
         }
         Chunk *nextNewLine = split_point->GetNextNl();

         // split following line if too long
         if (is_past_width(nextNewLine->GetPrev()))
         {
            split_line(nextNewLine->GetPrev());
         }
      }
   } while (  (pc = pc->GetPrev())->IsNotNullChunk()
           && !pc->IsNewline());
} // split_bool_expr


static void split_template(Chunk *start)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LSPLIT, "  %s(%d): start %s\n", __func__, __LINE__, start->Text());
   LOG_FMT(LSPLIT, "  %s(%d): back up until the prev is a comma\n", __func__, __LINE__);

   // back up until the prev is a comma
   Chunk *prev = start;

   while ((prev = prev->GetPrev())->IsNotNullChunk())
   {
      LOG_FMT(LSPLIT, "  %s(%d): prev '%s'\n", __func__, __LINE__, prev->Text());

      if (  prev->IsNewline()
         || prev->Is(CT_COMMA))
      {
         break;
      }
   }

   if (  prev->IsNotNullChunk()
      && !prev->IsNewline())
   {
      LOG_FMT(LSPLIT, "  %s(%d):", __func__, __LINE__);
      LOG_FMT(LSPLIT, " -- ended on %s --\n", get_token_name(prev->GetType()));
      Chunk  *pc = prev->GetNext();
      newline_add_before(pc);
      size_t min_col = 1;

      log_rule_B("indent_continue");

      if (options::indent_continue() == 0)
      {
         log_rule_B("indent_columns");
         min_col += options::indent_columns();
      }
      else
      {
         min_col += abs(options::indent_continue());
      }
      reindent_line(pc, min_col);
      cpd.changes++;
   }
} // split_templatefcn_params
