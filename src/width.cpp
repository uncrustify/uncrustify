/**
 * @file width.cpp
 * Limits line width.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "width.h"

#include "indent.h"
#include "newlines.h"
#include "prototypes.h"


constexpr static auto LCURRENT = LSPLIT;

using namespace uncrustify;


/**
 * abbreviations used:
 * - fparen = function parenthesis
 */

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


static inline bool is_past_width(chunk_t *pc);


//! Split right after the chunk
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
 * For function prototypes and definition. Also function calls where
 * level == brace_level:
 *   - find the open function parenthesis
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
 * Figures out where to split a template
 *
 *
 * @param start   the offending token
 */
static void split_template(chunk_t *start);


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


static inline bool is_past_width(chunk_t *pc)
{
   // allow char to sit at last column by subtracting 1
   LOG_FMT(LSPLIT, "%s(%d): orig_line is %zu, orig_col is %zu, for %s\n",
           __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
   log_rule_B("code_width");
   return((pc->column + pc->len() - 1) > options::code_width());
}


static void split_before_chunk(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LSPLIT, "%s(%d): text() '%s'\n", __func__, __LINE__, pc->text());

   if (  !chunk_is_newline(pc)
      && !chunk_is_newline(chunk_get_prev(pc)))
   {
      newline_add_before(pc);
      // reindent needs to include the indent_continue value and was off by one
      log_rule_B("indent_columns");
      log_rule_B("indent_continue");
      reindent_line(pc, pc->brace_level * options::indent_columns() +
                    abs(options::indent_continue()) + 1);
      cpd.changes++;
   }
}


void do_code_width(void)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LSPLIT, "%s(%d)\n", __func__, __LINE__);

   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next(pc))
   {
      if (  !chunk_is_newline(pc)
         && !chunk_is_comment(pc)
         && chunk_is_not_token(pc, CT_SPACE)
         && is_past_width(pc))
      {
         if (  chunk_is_token(pc, CT_VBRACE_CLOSE) // don't break if a vbrace close
            && chunk_is_last_on_line(*pc))         // is the last chunk on its line
         {
            continue;
         }
         bool split_OK = split_line(pc);

         if (split_OK)
         {
            LOG_FMT(LSPLIT, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s'\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
         }
         else
         {
            LOG_FMT(LSPLIT, "%s(%d): Bailed! orig_line is %zu, orig_col is %zu, text() '%s'\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
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
   { CT_SHIFT,        5 },
   { CT_ARITH,        6 },
   { CT_CARET,        7 },
   { CT_ASSIGN,       8 },
   { CT_STRING,       9 },
   { CT_FOR_COLON,   10 },
   //{ CT_DC_MEMBER, 11 },
   //{ CT_MEMBER,    11 },
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

   LOG_FMT(LSPLIT, "%s(%d): at %s, orig_col=%zu\n", __func__, __LINE__, pc->text(), pc->orig_col);
   size_t pc_pri = get_split_pri(pc->type);

   LOG_FMT(LSPLIT, "%s(%d): pc_pri is %zu\n", __func__, __LINE__, pc_pri);

   if (pc_pri == 0)
   {
      LOG_FMT(LSPLIT, "%s(%d): pc_pri is 0, return\n", __func__, __LINE__);
      return;
   }
   LOG_FMT(LSPLIT, "%s(%d):\n", __func__, __LINE__);
   // Can't split after a newline
   chunk_t *prev = chunk_get_prev(pc);

   if (  prev == nullptr
      || (  chunk_is_newline(prev)
         && chunk_is_not_token(pc, CT_STRING)))
   {
      if (prev != nullptr)
      {
         LOG_FMT(LSPLIT, "%s(%d): Can't split after a newline, orig_line is %zu, return\n",
                 __func__, __LINE__, prev->orig_line);
      }
      return;
   }
   LOG_FMT(LSPLIT, "%s(%d):\n", __func__, __LINE__);

   // Can't split a function without arguments
   if (chunk_is_token(pc, CT_FPAREN_OPEN))
   {
      chunk_t *next = chunk_get_next(pc);

      if (chunk_is_token(next, CT_FPAREN_CLOSE))
      {
         LOG_FMT(LSPLIT, "%s(%d): Can't split a function without arguments, return\n", __func__, __LINE__);
         return;
      }
   }
   LOG_FMT(LSPLIT, "%s(%d):\n", __func__, __LINE__);

   // Only split concatenated strings
   if (chunk_is_token(pc, CT_STRING))
   {
      chunk_t *next = chunk_get_next(pc);

      if (chunk_is_not_token(next, CT_STRING))
      {
         LOG_FMT(LSPLIT, "%s(%d): Only split concatenated strings, return\n", __func__, __LINE__);
         return;
      }
   }
   LOG_FMT(LSPLIT, "%s(%d):\n", __func__, __LINE__);

   // keep common groupings unless ls_code_width
   log_rule_B("ls_code_width");

   if (  !options::ls_code_width()
      && pc_pri >= 20)
   {
      LOG_FMT(LSPLIT, "%s(%d): keep common groupings unless ls_code_width, return\n", __func__, __LINE__);
      return;
   }
   LOG_FMT(LSPLIT, "%s(%d):\n", __func__, __LINE__);

   // don't break after last term of a qualified type
   if (pc_pri == 25)
   {
      chunk_t *next = chunk_get_next(pc);

      if (  chunk_is_not_token(next, CT_WORD)
         && (get_split_pri(next->type) != 25))
      {
         LOG_FMT(LSPLIT, "%s(%d): don't break after last term of a qualified type, return\n", __func__, __LINE__);
         return;
      }
   }
   LOG_FMT(LSPLIT, "%s(%d):\n", __func__, __LINE__);
   // Check levels first
   bool change = false;

   if (  ent.pc == nullptr
      || pc->level < ent.pc->level)
   {
      LOG_FMT(LSPLIT, "%s(%d):\n", __func__, __LINE__);
      change = true;
   }
   else
   {
      if (  pc->level >= ent.pc->level
         && pc_pri < ent.pri)
      {
         LOG_FMT(LSPLIT, "%s(%d):\n", __func__, __LINE__);
         change = true;
      }
   }
   LOG_FMT(LSPLIT, "%s(%d): change is %s\n", __func__, __LINE__, change ? "TRUE" : "FALSE");

   if (change)
   {
      LOG_FMT(LSPLIT, "%s(%d): do the change\n", __func__, __LINE__);
      ent.pc  = pc;
      ent.pri = pc_pri;
   }
} // try_split_here


static bool split_line(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LSPLIT, "%s(%d): start->text() '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
           __func__, __LINE__, start->text(), start->orig_line, start->orig_col, get_token_name(start->type));
   LOG_FMT(LSPLIT, "   start->flags ");
   log_pcf_flags(LSPLIT, start->flags);
   LOG_FMT(LSPLIT, "   start->parent_type %s, (PCF_IN_FCN_DEF is %s), (PCF_IN_FCN_CALL is %s)\n",
           get_token_name(get_chunk_parent_type(start)),
           start->flags.test((PCF_IN_FCN_DEF)) ? "TRUE" : "FALSE",
           start->flags.test((PCF_IN_FCN_CALL)) ? "TRUE" : "FALSE");

   // break at maximum line length if ls_code_width is true
   // Issue #2432
   if (start->flags.test(PCF_ONE_LINER))
   {
      LOG_FMT(LSPLIT, "%s(%d): ** ONCE LINER SPLIT **\n", __func__, __LINE__);
      undo_one_liner(start);
      newlines_cleanup_braces(false);
      // Issue #1352
      cpd.changes++;
      return(false);
   }
   LOG_FMT(LSPLIT, "%s(%d): before ls_code_width\n", __func__, __LINE__);

   log_rule_B("ls_code_width");

   if (options::ls_code_width())
   {
   }
   // Check to see if we are in a for statement
   else if (start->flags.test(PCF_IN_FOR))
   {
      LOG_FMT(LSPLIT, " ** FOR SPLIT **\n");
      split_for_stmt(start);

      if (!is_past_width(start))
      {
         return(true);
      }
      LOG_FMT(LSPLIT, "%s(%d): for split didn't work\n", __func__, __LINE__);
   }

   /*
    * If this is in a function call or prototype, split on commas or right
    * after the open parenthesis
    */
   else if (  start->flags.test(PCF_IN_FCN_DEF)
           || get_chunk_parent_type(start) == CT_FUNC_PROTO            // Issue #1169
           || (  (start->level == (start->brace_level + 1))
              && start->flags.test(PCF_IN_FCN_CALL)))
   {
      LOG_FMT(LSPLIT, " ** FUNC SPLIT **\n");

      log_rule_B("ls_func_split_full");

      if (options::ls_func_split_full())
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

   /*
    * If this is in a template, split on commas, Issue #1170
    */
   else if (start->flags.test(PCF_IN_TEMPLATE))
   {
      LOG_FMT(LSPLIT, " ** TEMPLATE SPLIT **\n");
      split_template(start);
      return(true);
   }
   LOG_FMT(LSPLIT, "%s(%d):\n", __func__, __LINE__);
   // Try to find the best spot to split the line
   cw_entry ent;

   memset(&ent, 0, sizeof(ent));
   chunk_t *pc = start;
   chunk_t *prev;

   while (  ((pc = chunk_get_prev(pc)) != nullptr)
         && !chunk_is_newline(pc))
   {
      LOG_FMT(LSPLIT, "%s(%d): at %s, orig_line is %zu, orig_col is %zu\n",
              __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);

      if (chunk_is_not_token(pc, CT_SPACE))
      {
         try_split_here(ent, pc);

         // break at maximum line length
         log_rule_B("ls_code_width");

         if (  ent.pc != nullptr
            && (options::ls_code_width()))
         {
            break;
         }
      }
   }

   if (ent.pc == nullptr)
   {
      LOG_FMT(LSPLIT, "%s(%d):    TRY_SPLIT yielded NO SOLUTION for orig_line %zu at '%s' [%s]\n",
              __func__, __LINE__, start->orig_line, start->text(), get_token_name(start->type));
   }
   else
   {
      LOG_FMT(LSPLIT, "%s(%d):    TRY_SPLIT yielded '%s' [%s] on orig_line %zu\n",
              __func__, __LINE__, ent.pc->text(), get_token_name(ent.pc->type), ent.pc->orig_line);
      LOG_FMT(LSPLIT, "%s(%d): ent at '%s', orig_col is %zu\n",
              __func__, __LINE__, ent.pc->text(), ent.pc->orig_col);
   }

   // Break before the token instead of after it according to the pos_xxx rules
   if (ent.pc == nullptr)
   {
      pc = nullptr;
   }
   else
   {
      log_rule_B("pos_arith");
      log_rule_B("pos_assign");
      log_rule_B("pos_compare");
      log_rule_B("pos_conditional");
      log_rule_B("pos_shift");
      log_rule_B("pos_bool");

      if (  (  chunk_is_token(ent.pc, CT_SHIFT)
            && (options::pos_shift() & TP_LEAD))
         || (  (  chunk_is_token(ent.pc, CT_ARITH)
               || chunk_is_token(ent.pc, CT_CARET))
            && (options::pos_arith() & TP_LEAD))
         || (  chunk_is_token(ent.pc, CT_ASSIGN)
            && (options::pos_assign() & TP_LEAD))
         || (  chunk_is_token(ent.pc, CT_COMPARE)
            && (options::pos_compare() & TP_LEAD))
         || (  (  chunk_is_token(ent.pc, CT_COND_COLON)
               || chunk_is_token(ent.pc, CT_QUESTION))
            && (options::pos_conditional() & TP_LEAD))
         || (  chunk_is_token(ent.pc, CT_BOOL)
            && (options::pos_bool() & TP_LEAD)))
      {
         pc = ent.pc;
      }
      else
      {
         pc = chunk_get_next(ent.pc);
      }
      LOG_FMT(LSPLIT, "%s(%d): at '%s', orig_col is %zu\n",
              __func__, __LINE__, pc->text(), pc->orig_col);
   }

   if (pc == nullptr)
   {
      pc = start;

      // Don't break before a close, comma, or colon
      if (  chunk_is_token(start, CT_PAREN_CLOSE)
         || chunk_is_token(start, CT_PAREN_OPEN)
         || chunk_is_token(start, CT_FPAREN_CLOSE)
         || chunk_is_token(start, CT_FPAREN_OPEN)
         || chunk_is_token(start, CT_SPAREN_CLOSE)
         || chunk_is_token(start, CT_SPAREN_OPEN)
         || chunk_is_token(start, CT_ANGLE_CLOSE)
         || chunk_is_token(start, CT_BRACE_CLOSE)
         || chunk_is_token(start, CT_COMMA)
         || chunk_is_token(start, CT_SEMICOLON)
         || chunk_is_token(start, CT_VSEMICOLON)
         || start->len() == 0)
      {
         LOG_FMT(LSPLIT, " ** NO GO **\n");

         // TODO: Add in logic to handle 'hard' limits by backing up a token
         return(true);
      }
   }
   // add a newline before pc
   prev = chunk_get_prev(pc);

   if (  prev != nullptr
      && !chunk_is_newline(pc)
      && !chunk_is_newline(prev))
   {
      //int plen = (pc->len() < 5) ? pc->len() : 5;
      //int slen = (start->len() < 5) ? start->len() : 5;
      //LOG_FMT(LSPLIT, " '%.*s' [%s], started on token '%.*s' [%s]\n",
      //        plen, pc->text(), get_token_name(pc->type),
      //        slen, start->text(), get_token_name(start->type));
      LOG_FMT(LSPLIT, "%s(%d): text() '%s', type %s, started on token '%s', type %s\n",
              __func__, __LINE__, pc->text(), get_token_name(pc->type),
              start->text(), get_token_name(start->type));

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
static void split_for_stmt(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   // how many semicolons (1 or 2) do we need to find
   log_rule_B("ls_for_split_full");
   size_t  max_cnt     = options::ls_for_split_full() ? 2 : 1;
   chunk_t *open_paren = nullptr;
   size_t  nl_cnt      = 0;

   LOG_FMT(LSPLIT, "%s: starting on %s, line %zu\n",
           __func__, start->text(), start->orig_line);

   // Find the open paren so we know the level and count newlines
   chunk_t *pc = start;

   while ((pc = chunk_get_prev(pc)) != nullptr)
   {
      if (chunk_is_token(pc, CT_SPAREN_OPEN))
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
   // see if we started on the semicolon
   int     count = 0;
   chunk_t *st[2];

   pc = start;

   if (  chunk_is_token(pc, CT_SEMICOLON)
      && get_chunk_parent_type(pc) == CT_FOR)
   {
      st[count++] = pc;
   }

   // first scan backwards for the semicolons
   while (  (count < static_cast<int>(max_cnt))
         && ((pc = chunk_get_prev(pc)) != nullptr)
         && pc->flags.test(PCF_IN_SPAREN))
   {
      if (  chunk_is_token(pc, CT_SEMICOLON)
         && get_chunk_parent_type(pc) == CT_FOR)
      {
         st[count++] = pc;
      }
   }
   // And now scan forward
   pc = start;

   while (  (count < static_cast<int>(max_cnt))
         && ((pc = chunk_get_next(pc)) != nullptr)
         && pc->flags.test(PCF_IN_SPAREN))
   {
      if (  chunk_is_token(pc, CT_SEMICOLON)
         && get_chunk_parent_type(pc) == CT_FOR)
      {
         st[count++] = pc;
      }
   }

   while (--count >= 0)
   {
      // TODO: st[0] may be uninitialized here
      LOG_FMT(LSPLIT, "%s(%d): split before %s\n", __func__, __LINE__, st[count]->text());
      split_before_chunk(chunk_get_next(st[count]));
   }

   if (  !is_past_width(start)
      || nl_cnt > 0)
   {
      return;
   }
   // Still past width, check for commas at parentheses level
   pc = open_paren;

   while ((pc = chunk_get_next(pc)) != start)
   {
      if (  chunk_is_token(pc, CT_COMMA)
         && (pc->level == (open_paren->level + 1)))
      {
         split_before_chunk(chunk_get_next(pc));

         if (!is_past_width(pc))
         {
            return;
         }
      }
   }
   // Still past width, check for a assignments at parentheses level
   pc = open_paren;

   while ((pc = chunk_get_next(pc)) != start)
   {
      if (  chunk_is_token(pc, CT_ASSIGN)
         && (pc->level == (open_paren->level + 1)))
      {
         split_before_chunk(chunk_get_next(pc));

         if (!is_past_width(pc))
         {
            return;
         }
      }
   }
   // Oh, well. We tried.
} // split_for_stmt


static void split_fcn_params_full(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LSPLIT, "%s(%d): %s\n", __func__, __LINE__, start->text());

   // Find the opening function parenthesis
   chunk_t *fpo = start;

   LOG_FMT(LSPLIT, "  %s(%d): Find the opening function parenthesis\n", __func__, __LINE__);

   while ((fpo = chunk_get_prev(fpo)) != nullptr)
   {
      LOG_FMT(LSPLIT, "%s(%d): %s, orig_col is %zu, level is %zu\n",
              __func__, __LINE__, fpo->text(), fpo->orig_col, fpo->level);

      if (  chunk_is_token(fpo, CT_FPAREN_OPEN)
         && (fpo->level == start->level - 1))
      {
         break;  // opening parenthesis found. Issue #1020
      }
   }
   // Now break after every comma
   chunk_t *pc = fpo;

   while ((pc = chunk_get_next_ncnnl(pc)) != nullptr)
   {
      if (pc->level <= fpo->level)
      {
         break;
      }

      if (  (pc->level == (fpo->level + 1))
         && chunk_is_token(pc, CT_COMMA))
      {
         split_before_chunk(chunk_get_next(pc));
      }
   }
}


static void split_fcn_params(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LSPLIT, "%s(%d): start->text() is '%s', orig_line is %zu, orig_col is %zu\n",
           __func__, __LINE__, start->text(), start->orig_line, start->orig_col);
   chunk_t *fpo = start;

   if (!chunk_is_token(start, CT_FPAREN_OPEN))
   {
      // Find the opening function parenthesis
      LOG_FMT(LSPLIT, "%s(%d): Find the opening function parenthesis\n", __func__, __LINE__);

      while (  ((fpo = chunk_get_prev(fpo)) != nullptr)
            && chunk_is_not_token(fpo, CT_FPAREN_OPEN))
      {
         // do nothing
         LOG_FMT(LSPLIT, "%s(%d): '%s', orig_col is %zu, level is %zu\n",
                 __func__, __LINE__, fpo->text(), fpo->orig_col, fpo->level);
      }
   }
   chunk_t *pc     = chunk_get_next_ncnnl(fpo);
   size_t  min_col = pc->column;

   log_rule_B("code_width");
   LOG_FMT(LSPLIT, "    mincol is %zu, max_width is %zu\n",
           min_col, options::code_width() - min_col);

   int cur_width = 0;
   int last_col  = -1;

   LOG_FMT(LSPLIT, "%s(%d):look forward until CT_COMMA or CT_FPAREN_CLOSE\n", __func__, __LINE__);

   while (pc != nullptr)
   {
      LOG_FMT(LSPLIT, "%s(%d): pc->text() '%s', type is %s\n",
              __func__, __LINE__, pc->text(), get_token_name(pc->type));

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
            LOG_FMT(LSPLIT, "%s(%d): last_col is %d\n",
                    __func__, __LINE__, last_col);
         }
         cur_width += (pc->column - last_col) + pc->len();
         last_col   = pc->column + pc->len();

         LOG_FMT(LSPLIT, "%s(%d): last_col is %d\n",
                 __func__, __LINE__, last_col);

         if (  chunk_is_token(pc, CT_COMMA)
            || chunk_is_token(pc, CT_FPAREN_CLOSE))
         {
            if (cur_width == 0)
            {
               fprintf(stderr, "%s(%d): cur_width is ZERO, cannot be decremented, at line %zu, column %zu\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col);
               log_flush(true);
               exit(EX_SOFTWARE);
            }
            cur_width--;
            LOG_FMT(LSPLIT, "%s(%d): cur_width is %d\n",
                    __func__, __LINE__, cur_width);

            log_rule_B("code_width");

            if (  ((last_col - 1) > static_cast<int>(options::code_width()))
               || chunk_is_token(pc, CT_FPAREN_CLOSE))
            {
               break;
            }
         }
      }
      pc = chunk_get_next(pc);
   }
   // back up until the prev is a comma
   chunk_t *prev = pc;

   LOG_FMT(LSPLIT, "%s(%d): back up until the prev is a comma, begin is '%s', level is %zu\n",
           __func__, __LINE__, prev->text(), prev->level);

   while ((prev = chunk_get_prev(prev)) != nullptr)
   {
      LOG_FMT(LSPLIT, "%s(%d): prev->text() is '%s', prev->orig_line is %zu, prev->orig_col is %zu\n",
              __func__, __LINE__, prev->text(), prev->orig_line, prev->orig_col);
      LOG_FMT(LSPLIT, "%s(%d): prev->level is %zu, prev '%s', prev->type is %s\n",
              __func__, __LINE__, prev->level, prev->text(), get_token_name(prev->type));

      if (  chunk_is_newline(prev)
         || chunk_is_token(prev, CT_COMMA))
      {
         LOG_FMT(LSPLIT, "%s(%d): found at %zu\n",
                 __func__, __LINE__, prev->orig_col);
         break;
      }
      LOG_FMT(LSPLIT, "%s(%d): last_col is %d, prev->len() is %zu\n",
              __func__, __LINE__, last_col, prev->len());
      last_col -= prev->len();
      LOG_FMT(LSPLIT, "%s(%d): last_col is %d\n",
              __func__, __LINE__, last_col);

      if (chunk_is_token(prev, CT_FPAREN_OPEN))
      {
         pc = chunk_get_next(prev);

         log_rule_B("indent_paren_nl");

         if (!options::indent_paren_nl())
         {
            log_rule_B("indent_columns");
            min_col = pc->brace_level * options::indent_columns() + 1;
            LOG_FMT(LSPLIT, "%s(%d): min_col is %zu\n",
                    __func__, __LINE__, min_col);

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
            LOG_FMT(LSPLIT, "%s(%d): min_col is %zu\n",
                    __func__, __LINE__, min_col);
         }

         // Don't split "()"
         if (pc->type != c_token_t(prev->type + 1))
         {
            break;
         }
      }
   }

   if (  prev != nullptr
      && !chunk_is_newline(prev))
   {
      LOG_FMT(LSPLIT, "%s(%d): -- ended on %s --\n",
              __func__, __LINE__, get_token_name(prev->type));
      LOG_FMT(LSPLIT, "%s(%d): min_col is %zu\n",
              __func__, __LINE__, min_col);
      pc = chunk_get_next(prev);
      newline_add_before(pc);
      reindent_line(pc, min_col);
      cpd.changes++;
   }
} // split_fcn_params


static void split_template(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LSPLIT, "  %s(%d): start %s\n", __func__, __LINE__, start->text());
   LOG_FMT(LSPLIT, "  %s(%d): back up until the prev is a comma\n", __func__, __LINE__);

   // back up until the prev is a comma
   chunk_t *prev = start;

   while ((prev = chunk_get_prev(prev)) != nullptr)
   {
      LOG_FMT(LSPLIT, "  %s(%d): prev '%s'\n", __func__, __LINE__, prev->text());

      if (  chunk_is_newline(prev)
         || chunk_is_token(prev, CT_COMMA))
      {
         break;
      }
   }

   if (  prev != nullptr
      && !chunk_is_newline(prev))
   {
      LOG_FMT(LSPLIT, "  %s(%d):", __func__, __LINE__);
      LOG_FMT(LSPLIT, " -- ended on %s --\n", get_token_name(prev->type));
      chunk_t *pc = chunk_get_next(prev);
      newline_add_before(pc);
      size_t  min_col = 1;

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
