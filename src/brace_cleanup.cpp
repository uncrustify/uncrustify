/**
 * @file brace_cleanup.cpp
 * Determines the brace level and paren level.
 * Inserts virtual braces as needed.
 * Handles all that preprocessor stuff.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "brace_cleanup.h"

#include "chunk_list.h"
#include "flag_parens.h"
#include "frame_list.h"
#include "indent.h"
#include "keywords.h"
#include "lang_pawn.h"
#include "language_tools.h"
#include "logger.h"
#include "prototypes.h"
#include "unc_ctype.h"
#include "uncrustify.h"
#include "uncrustify_types.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>

using namespace uncrustify;

using std::invalid_argument;
using std::string;
using std::to_string;
using std::stringstream;


/*
 * abbreviations used:
 * - sparen = tbd
 * - PS     = Parenthesis Stack
 * - pse    = Parenthesis Stack
 */


/**
 * Called when a statement was just closed and the pse_tos was just
 * decremented.
 *
 * - if the TOS is now VBRACE, insert a CT_VBRACE_CLOSE and recurse.
 * - if the TOS is a complex statement, call handle_complex_close()
 *
 * @retval true   done with this chunk
 * @retval false  keep processing
 */
static bool close_statement(ParseFrame &frm, chunk_t *pc);


static size_t preproc_start(ParseFrame &frm, chunk_t *pc);


static void print_stack(log_sev_t logsev, const char *str, const ParseFrame &frm);


/**
 * pc is a CT_WHILE.
 * Scan backwards to see if we find a brace/vbrace with the parent set to CT_DO
 */
static bool maybe_while_of_do(chunk_t *pc);


/**
 * @param after  determines: true  - insert_vbrace_close_after(pc, frm)
 *                           false - insert_vbrace_open_before(pc, frm)
 */
static chunk_t *insert_vbrace(chunk_t *pc, bool after, const ParseFrame &frm);

#define insert_vbrace_close_after(pc, frm)    insert_vbrace(pc, true, frm)
#define insert_vbrace_open_before(pc, frm)    insert_vbrace(pc, false, frm)

static void parse_cleanup(ParseFrame &frm, chunk_t *pc);


/**
 * Checks the progression of complex statements.
 * - checks for else after if
 * - checks for if after else
 * - checks for while after do
 * - checks for open brace in BRACE2 and BRACE_DO stages, inserts open VBRACE
 * - checks for open paren in PAREN1 and BRACE2 stages, complains
 *
 * @param frm  The parse frame
 * @param pc   The current chunk
 *
 * @return true - done with this chunk, false - keep processing
 */
static bool check_complex_statements(ParseFrame &frm, chunk_t *pc);


/**
 * Handles a close paren or brace - just progress the stage, if the end
 * of the statement is hit, call close_statement()
 *
 * @param frm  The parse frame
 * @param pc   The current chunk
 *
 * @return true - done with this chunk, false - keep processing
 */
static bool handle_complex_close(ParseFrame &frm, chunk_t *pc);


//! We're on a 'namespace' skip the word and then set the parent of the braces.
static void mark_namespace(chunk_t *pns);


static size_t preproc_start(ParseFrame &frm, chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   const size_t pp_level = cpd.pp_level;

   chunk_t      *next = chunk_get_next_ncnl(pc);
   if (next == nullptr)
   {
      return(pp_level);
   }

   // Get the type of preprocessor and handle it
   cpd.in_preproc = next->type;
   // If we are not in a define, check for #if, #else, #endif, etc
   if (cpd.in_preproc != CT_PP_DEFINE)
   {
      return(fl_check(frm, pc));
   }

   // else push the frame stack
   fl_push(frm);

   // a preproc body starts a new, blank frame
   frm             = {};
   frm.level       = 1;
   frm.brace_level = 1;

   // TODO: not sure about the next 3 lines
   chunk_t tmp{};
   frm.push(tmp);
   frm.top().type = CT_PP_DEFINE;

   return(pp_level);
}


static void print_stack(log_sev_t logsev, const char *str,
                        const ParseFrame &frm)
{
   LOG_FUNC_ENTRY();
   if (!log_sev_on(logsev))
   {
      return;
   }

   log_fmt(logsev, "%s(%d): str is '%s'", __func__, __LINE__, str);

   for (size_t idx = 1; idx < frm.size(); idx++)
   {
      if (frm.at(idx).stage != brace_stage_e::NONE)
      {
         LOG_FMT(logsev, " [%s - %u]", get_token_name(frm.at(idx).type),
                 (unsigned int)frm.at(idx).stage);
      }
      else
      {
         LOG_FMT(logsev, " [%s]", get_token_name(frm.at(idx).type));
      }
   }

   log_fmt(logsev, "\n");
}


//TODO: This can be cleaned up and simplified - we can look both forward and backward!
void brace_cleanup(void)
{
   LOG_FUNC_ENTRY();

   cpd.unc_stage = unc_stage_e::BRACE_CLEANUP;
   cpd.frames.clear();
   cpd.in_preproc = CT_NONE;
   cpd.pp_level   = 0;

   ParseFrame frm{};
   chunk_t    *pc = chunk_get_head();
   while (pc != nullptr)
   {
      // Check for leaving a #define body
      if (cpd.in_preproc != CT_NONE && (pc->flags & PCF_IN_PREPROC) == 0)
      {
         if (cpd.in_preproc == CT_PP_DEFINE)
         {
            // out of the #define body, restore the frame
            fl_pop(frm);
         }

         cpd.in_preproc = CT_NONE;
      }

      // Check for a preprocessor start
      const size_t pp_level = (chunk_is_token(pc, CT_PREPROC))
                              ? preproc_start(frm, pc)
                              : cpd.pp_level;

      // Do before assigning stuff from the frame
      if (  language_is_set(LANG_PAWN)
         && frm.top().type == CT_VBRACE_OPEN
         && chunk_is_token(pc, CT_NEWLINE))
      {
         pc = pawn_check_vsemicolon(pc);
         if (pc == nullptr)
         {
            return;
         }
      }

      // Issue #1813
      if (chunk_is_token(pc, CT_NAMESPACE))
      {
         mark_namespace(pc);
      }
      // Assume the level won't change
      pc->level       = frm.level;
      pc->brace_level = frm.brace_level;
      pc->pp_level    = pp_level;

      /*
       * #define bodies get the full formatting treatment
       * Also need to pass in the initial '#' to close out any virtual braces.
       */
      if (  !chunk_is_comment(pc)
         && !chunk_is_newline(pc)
         && !chunk_is_token(pc, CT_ATTRIBUTE)
         && !chunk_is_token(pc, CT_IGNORED)            // Issue #2279
         && (cpd.in_preproc == CT_PP_DEFINE || cpd.in_preproc == CT_NONE))
      {
         cpd.consumed = false;
         parse_cleanup(frm, pc);
         print_stack(LBCSAFTER, (chunk_is_token(pc, CT_VBRACE_CLOSE)) ? "Virt-}" : pc->str.c_str(), frm);
      }
      pc = chunk_get_next(pc);
   }
} // brace_cleanup


static bool maybe_while_of_do(chunk_t *pc)
{
   LOG_FUNC_ENTRY();

   chunk_t *prev = chunk_get_prev_ncnl(pc);
   if (prev == nullptr || !(prev->flags & PCF_IN_PREPROC))
   {
      return(false);
   }

   // Find the chunk before the preprocessor
   while (prev != nullptr && (prev->flags & PCF_IN_PREPROC))
   {
      prev = chunk_get_prev_ncnl(prev);
   }

   if (  (chunk_is_token(prev, CT_VBRACE_CLOSE) || chunk_is_token(prev, CT_BRACE_CLOSE))
      && prev->parent_type == CT_DO)
   {
      return(true);
   }
   return(false);
}


/**
 * At the heart of this algorithm are two stacks.
 * There is the Paren Stack (PS) and the Frame stack.
 *
 * The PS (pse in the code) keeps track of braces, parens,
 * if/else/switch/do/while/etc items -- anything that is nestable.
 * Complex statements go through stages.
 * Take this simple if statement as an example:
 *   if ( x ) { x--; }
 *
 * The stack would change like so: 'token' stack afterwards
 * 'if' [IF - 1]
 * '('  [IF - 1] [PAREN OPEN]
 * 'x'  [IF - 1] [PAREN OPEN]
 * ')'  [IF - 2]       <- note that the state was incremented
 * '{'  [IF - 2] [BRACE OPEN]
 * 'x'  [IF - 2] [BRACE OPEN]
 * '--' [IF - 2] [BRACE OPEN]
 * ';'  [IF - 2] [BRACE OPEN]
 * '}'  [IF - 3]
 *                             <- lack of else kills the IF, closes statement
 *
 * Virtual braces example:
 *   if ( x ) x--; else x++;
 *
 * 'if'   [IF - 1]
 * '('    [IF - 1] [PAREN OPEN]
 * 'x'    [IF - 1] [PAREN OPEN]
 * ')'    [IF - 2]
 * 'x'    [IF - 2] [VBRACE OPEN]   <- VBrace open inserted before because '{' was not next
 * '--'   [IF - 2] [VBRACE OPEN]
 * ';'    [IF - 3]                 <- VBrace close inserted after semicolon
 * 'else' [ELSE - 0]               <- IF changed into ELSE
 * 'x'    [ELSE - 0] [VBRACE OPEN] <- lack of '{' -> VBrace
 * '++'   [ELSE - 0] [VBRACE OPEN]
 * ';'    [ELSE - 0]               <- VBrace close inserted after semicolon
 *                                 <- ELSE removed after statement close
 *
 * The pse stack is kept on a frame stack.
 * The frame stack is need for languages that support preprocessors (C, C++, C#)
 * that can arbitrarily change code flow. It also isolates #define macros so
 * that they are indented independently and do not affect the rest of the program.
 *
 * When an #if is hit, a copy of the current frame is push on the frame stack.
 * When an #else/#elif is hit, a copy of the current stack is pushed under the
 * #if frame and the original (pre-#if) frame is copied to the current frame.
 * When #endif is hit, the top frame is popped.
 * This has the following effects:
 *  - a simple #if / #endif does not affect program flow
 *  - #if / #else /#endif - continues from the #if clause
 *
 * When a #define is entered, the current frame is pushed and cleared.
 * When a #define is exited, the frame is popped.
 */
static void parse_cleanup(ParseFrame &frm, chunk_t *pc)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LTOK, "%s(%d): orig_line is %zu, type is %s, tos is %zu, TOS.type is %s, TOS.stage is %s, ",
           __func__, __LINE__, pc->orig_line, get_token_name(pc->type),
           frm.size() - 1, get_token_name(frm.top().type),
           get_brace_stage_name(frm.top().stage));
   log_pcf_flags(LTOK, pc->flags);

   // Mark statement starts
   LOG_FMT(LTOK, "%s(%d): orig_line is %zu, type is %s, text() is '%s'\n",
           __func__, __LINE__, pc->orig_line, get_token_name(pc->type), pc->text());
   LOG_FMT(LTOK, "%s(%d): frm.stmt_count is %zu, frm.expr_count is %zu\n",
           __func__, __LINE__, frm.stmt_count, frm.expr_count);
   if (  (frm.stmt_count == 0 || frm.expr_count == 0)
      && !chunk_is_semicolon(pc)
      && pc->type != CT_BRACE_CLOSE
      && pc->type != CT_VBRACE_CLOSE
      && !chunk_is_str(pc, ")", 1)
      && !chunk_is_str(pc, "]", 1))
   {
      chunk_flags_set(pc, PCF_EXPR_START | ((frm.stmt_count == 0) ? PCF_STMT_START : 0));
      LOG_FMT(LSTMT, "%s(%d): orig_line is %zu, 1.marked '%s' as %s, start stmt_count is %zu, expr_count is %zu\n",
              __func__, __LINE__, pc->orig_line, pc->text(),
              (pc->flags & PCF_STMT_START) ? "stmt" : "expr", frm.stmt_count,
              frm.expr_count);
   }
   frm.stmt_count++;
   frm.expr_count++;
   LOG_FMT(LTOK, "%s(%d): frm.stmt_count is %zu, frm.expr_count is %zu\n",
           __func__, __LINE__, frm.stmt_count, frm.expr_count);

   if (frm.sparen_count > 0)
   {
      chunk_flags_set(pc, PCF_IN_SPAREN);

      // Mark everything in the for statement
      for (int tmp = static_cast<int>(frm.size()) - 2; tmp >= 0; tmp--)
      {
         if (frm.at(tmp).type == CT_FOR)
         {
            chunk_flags_set(pc, PCF_IN_FOR);
            break;
         }
      }

      // Mark the parent on semicolons in for() statements
      if (  chunk_is_token(pc, CT_SEMICOLON)
         && frm.size() > 2
         && frm.prev().type == CT_FOR)
      {
         set_chunk_parent(pc, CT_FOR);
      }
   }

   // Check the progression of complex statements
   if (  frm.top().stage != brace_stage_e::NONE
      && !chunk_is_token(pc, CT_AUTORELEASEPOOL)
      && check_complex_statements(frm, pc))
   {
      return;
   }

   /*
    * Check for a virtual brace statement close due to a semicolon.
    * The virtual brace will get handled the next time through.
    * The semicolon isn't handled at all.
    * TODO: may need to float VBRACE past comments until newline?
    */
   if (frm.top().type == CT_VBRACE_OPEN)
   {
      if (chunk_is_semicolon(pc))
      {
         cpd.consumed = true;
         close_statement(frm, pc);
      }
      else if (language_is_set(LANG_PAWN) && chunk_is_token(pc, CT_BRACE_CLOSE))
      {
         close_statement(frm, pc);
      }
   }

   // Handle close parenthesis, vbrace, brace, and square
   if (  chunk_is_token(pc, CT_PAREN_CLOSE)
      || chunk_is_token(pc, CT_BRACE_CLOSE)
      || chunk_is_token(pc, CT_VBRACE_CLOSE)
      || chunk_is_token(pc, CT_ANGLE_CLOSE)
      || chunk_is_token(pc, CT_MACRO_CLOSE)
      || chunk_is_token(pc, CT_SQUARE_CLOSE))
   {
      // Change CT_PAREN_CLOSE into CT_SPAREN_CLOSE or CT_FPAREN_CLOSE
      if (  chunk_is_token(pc, CT_PAREN_CLOSE)
         && (  (frm.top().type == CT_FPAREN_OPEN)
            || (frm.top().type == CT_SPAREN_OPEN)))
      {
         // TODO: fix enum hack
         set_chunk_type(pc, static_cast<c_token_t>(frm.top().type + 1));
         if (chunk_is_token(pc, CT_SPAREN_CLOSE))
         {
            frm.sparen_count--;
            chunk_flags_clr(pc, PCF_IN_SPAREN);
         }
      }

      // Make sure the open / close match
      if (pc->type != (frm.top().type + 1))
      {
         LOG_FMT(LWARN, "%s(%d): pc->orig_line is %zu, orig_col is %zu, text() is '%s', type is %s\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
         LOG_FMT(LWARN, "%s(%d): (frm.top().type + 1) is %s\n",
                 __func__, __LINE__, get_token_name((c_token_t)(frm.top().type + 1)));
         if (  frm.top().type != CT_EOF
            && frm.top().type != CT_PP_DEFINE)
         {
            LOG_FMT(LWARN, "%s(%d): File: %s, orig_line is %zu, orig_col is %zu, Error: Unexpected '%s' for '%s', which was on line %zu\n",
                    __func__, __LINE__, cpd.filename.c_str(), pc->orig_line, pc->orig_col,
                    pc->text(), get_token_name(frm.top().pc->type),
                    frm.top().pc->orig_line);
            print_stack(LBCSPOP, "=Error  ", frm);
            cpd.error_count++;
         }
      }
      else
      {
         cpd.consumed = true;

         // Copy the parent, update the parenthesis/brace levels
         set_chunk_parent(pc, frm.top().parent);
         frm.level--;
         if (  chunk_is_token(pc, CT_BRACE_CLOSE)
            || chunk_is_token(pc, CT_VBRACE_CLOSE)
            || chunk_is_token(pc, CT_MACRO_CLOSE))
         {
            frm.brace_level--;
            LOG_FMT(LBCSPOP, "%s(%d): frm.brace_level decreased to %zu",
                    __func__, __LINE__, frm.brace_level);
            log_pcf_flags(LBCSPOP, pc->flags);
         }
         pc->level       = frm.level;
         pc->brace_level = frm.brace_level;

         // Pop the entry
         LOG_FMT(LBCSPOP, "%s(%d): pc->orig_line is %zu, orig_col is %zu, text() is '%s', type is %s\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
         frm.pop();
         print_stack(LBCSPOP, "-Close  ", frm);

         // See if we are in a complex statement
         if (frm.top().stage != brace_stage_e::NONE)
         {
            handle_complex_close(frm, pc);
         }
      }
   }

   /*
    * In this state, we expect a semicolon, but we'll also hit the closing
    * sparen, so we need to check cpd.consumed to see if the close sparen was
    * aleady handled.
    */
   if (frm.top().stage == brace_stage_e::WOD_SEMI)
   {
      if (cpd.consumed)
      {
         /*
          * If consumed, then we are on the close sparen.
          * PAWN: Check the next chunk for a semicolon. If it isn't, then
          * add a virtual semicolon, which will get handled on the next pass.
          */
         if (language_is_set(LANG_PAWN))
         {
            chunk_t *tmp = chunk_get_next_ncnl(pc);

            if (  tmp != nullptr
               && tmp->type != CT_SEMICOLON
               && tmp->type != CT_VSEMICOLON)
            {
               pawn_add_vsemi_after(pc);
            }
         }
      }
      else
      {
         // Complain if this ISN'T a semicolon, but close out WHILE_OF_DO anyway
         if (chunk_is_token(pc, CT_SEMICOLON) || chunk_is_token(pc, CT_VSEMICOLON))
         {
            cpd.consumed = true;
            set_chunk_parent(pc, CT_WHILE_OF_DO);
         }
         else
         {
            LOG_FMT(LWARN, "%s: %s(%d): %zu: Error: Expected a semicolon for WHILE_OF_DO, but got '%s'\n",
                    cpd.filename.c_str(), __func__, __LINE__, pc->orig_line,
                    get_token_name(pc->type));
            cpd.error_count++;
         }
         handle_complex_close(frm, pc);
      }
   }

   // Get the parent type for brace and parenthesis open
   c_token_t parent = pc->parent_type;
   if (  chunk_is_token(pc, CT_PAREN_OPEN)
      || chunk_is_token(pc, CT_FPAREN_OPEN)
      || chunk_is_token(pc, CT_SPAREN_OPEN)
      || chunk_is_token(pc, CT_BRACE_OPEN))
   {
      chunk_t *prev = chunk_get_prev_ncnl(pc);
      if (prev != nullptr)
      {
         if (  chunk_is_token(pc, CT_PAREN_OPEN)
            || chunk_is_token(pc, CT_FPAREN_OPEN)
            || chunk_is_token(pc, CT_SPAREN_OPEN))
         {
            // Set the parent for parenthesis and change parenthesis type
            if (frm.top().stage != brace_stage_e::NONE)
            {
               set_chunk_type(pc, CT_SPAREN_OPEN);
               parent = frm.top().type;
               frm.sparen_count++;
            }
            else if (chunk_is_token(prev, CT_FUNCTION))
            {
               set_chunk_type(pc, CT_FPAREN_OPEN);
               parent = CT_FUNCTION;
            }
            // NS_ENUM and NS_OPTIONS are followed by a (type, name) pair
            else if (chunk_is_token(prev, CT_ENUM) && language_is_set(LANG_OC))
            {
               // Treat both as CT_ENUM since the syntax is identical
               set_chunk_type(pc, CT_FPAREN_OPEN);
               parent = CT_ENUM;
            }
            else if (chunk_is_token(prev, CT_DECLSPEC))  // Issue 1289
            {
               parent = CT_DECLSPEC;
            }
            // else: no need to set parent
         }
         else  // must be CT_BRACE_OPEN
         {
            // Set the parent for open braces
            if (frm.top().stage != brace_stage_e::NONE)
            {
               parent = frm.top().type;
            }
            else if (chunk_is_token(prev, CT_ASSIGN) && (prev->str[0] == '='))
            {
               parent = CT_ASSIGN;
            }
            else if (chunk_is_token(prev, CT_RETURN) && language_is_set(LANG_CPP))
            {
               parent = CT_RETURN;
            }
            // Carry through CT_ENUM parent in NS_ENUM (type, name) {
            else if (  chunk_is_token(prev, CT_FPAREN_CLOSE)
                    && language_is_set(LANG_OC)
                    && prev->parent_type == CT_ENUM)
            {
               parent = CT_ENUM;
            }
            else if (chunk_is_token(prev, CT_FPAREN_CLOSE))
            {
               parent = CT_FUNCTION;
            }
            // else: no need to set parent
         }
      }
   }

   /*
    * Adjust the level for opens & create a stack entry
    * Note that CT_VBRACE_OPEN has already been handled.
    */
   if (  chunk_is_token(pc, CT_BRACE_OPEN)
      || chunk_is_token(pc, CT_PAREN_OPEN)
      || chunk_is_token(pc, CT_FPAREN_OPEN)
      || chunk_is_token(pc, CT_SPAREN_OPEN)
      || chunk_is_token(pc, CT_ANGLE_OPEN)
      || chunk_is_token(pc, CT_MACRO_OPEN)
      || chunk_is_token(pc, CT_SQUARE_OPEN))
   {
      frm.level++;
      if (chunk_is_token(pc, CT_BRACE_OPEN) || chunk_is_token(pc, CT_MACRO_OPEN))
      {
         // Issue #1813
         bool single = false;
         if (pc->parent_type == CT_NAMESPACE)
         {
            LOG_FMT(LBCSPOP, "%s(%d): parent_type is NAMESPACE\n",
                    __func__, __LINE__);
            chunk_t *tmp = frm.top().pc;
            if (tmp != nullptr && tmp->parent_type == CT_NAMESPACE)
            {
               LOG_FMT(LBCSPOP, "%s(%d): tmp->parent_type is NAMESPACE\n",
                       __func__, __LINE__);
               if (  options::indent_namespace()
                  && options::indent_namespace_single_indent())
               {
                  LOG_FMT(LBCSPOP, "%s(%d): Options are SINGLE\n",
                          __func__, __LINE__);
                  single = true;
               }
            }
         }
         LOG_FMT(LBCSPOP, "%s(%d): pc->orig_line is %zu, orig_col is %zu, text() is '%s', type is %s, parent_type is %s\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type), get_token_name(pc->parent_type));
         if (!single)
         {
            frm.brace_level++;
            LOG_FMT(LBCSPOP, "%s(%d): frm.brace_level increased to %zu\n",
                    __func__, __LINE__, frm.brace_level);
         }
      }
      frm.push(*pc);
      frm.top().parent = parent;
      set_chunk_parent(pc, parent);
   }

   const pattern_class_e patcls = get_token_pattern_class(pc->type);

   /*
    * Create a stack entry for complex statements:
    * if, elseif, switch, for, while, synchronized, using, lock, with,
    * version, CT_D_SCOPE_IF
    */
   if (patcls == pattern_class_e::BRACED)
   {
      frm.push(*pc, (chunk_is_token(pc, CT_DO) ? brace_stage_e::BRACE_DO
                     : brace_stage_e::BRACE2));
      // "+ComplexBraced"
   }
   else if (patcls == pattern_class_e::PBRACED)
   {
      brace_stage_e bs = brace_stage_e::PAREN1;

      if (chunk_is_token(pc, CT_WHILE) && maybe_while_of_do(pc))
      {
         set_chunk_type(pc, CT_WHILE_OF_DO);
         bs = brace_stage_e::WOD_PAREN;
      }
      frm.push(*pc, bs);
      // "+ComplexParenBraced"
   }
   else if (patcls == pattern_class_e::OPBRACED)
   {
      frm.push(*pc, brace_stage_e::OP_PAREN1);
      // "+ComplexOpParenBraced");
   }
   else if (patcls == pattern_class_e::ELSE)
   {
      frm.push(*pc, brace_stage_e::ELSEIF);
      // "+ComplexElse");
   }

   /*
    * Mark simple statement/expression starts
    *  - after { or }
    *  - after ';', but not if the paren stack top is a paren
    *  - after '(' that has a parent type of CT_FOR
    */
   if (  chunk_is_token(pc, CT_SQUARE_OPEN)
      || (chunk_is_token(pc, CT_BRACE_OPEN) && pc->parent_type != CT_ASSIGN)
      || chunk_is_token(pc, CT_BRACE_CLOSE)
      || chunk_is_token(pc, CT_VBRACE_CLOSE)
      || (chunk_is_token(pc, CT_SPAREN_OPEN) && pc->parent_type == CT_FOR)
      || chunk_is_token(pc, CT_COLON)
      || chunk_is_token(pc, CT_OC_END)
      || (  chunk_is_semicolon(pc)
         && frm.top().type != CT_PAREN_OPEN
         && frm.top().type != CT_FPAREN_OPEN
         && frm.top().type != CT_SPAREN_OPEN))
   {
      LOG_FMT(LSTMT, "%s(%d): orig_line is %zu, reset1 stmt on '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->text());
      frm.stmt_count = 0;
      frm.expr_count = 0;
      LOG_FMT(LTOK, "%s(%d): frm.stmt_count is %zu, frm.expr_count is %zu\n",
              __func__, __LINE__, frm.stmt_count, frm.expr_count);
   }

   // Mark expression starts
   LOG_FMT(LSTMT, "%s(%d): Mark expression starts: orig_line is %zu, orig_col is %zu, text() is '%s'\n",
           __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
   chunk_t *tmp = chunk_get_next_ncnl(pc);
   if (  chunk_is_token(pc, CT_ARITH)
      || chunk_is_token(pc, CT_ASSIGN)
      || chunk_is_token(pc, CT_CASE)
      || chunk_is_token(pc, CT_COMPARE)
      || (  chunk_is_token(pc, CT_STAR)
         && tmp != nullptr && tmp->type != CT_STAR)
      || chunk_is_token(pc, CT_BOOL)
      || chunk_is_token(pc, CT_MINUS)
      || chunk_is_token(pc, CT_PLUS)
      || chunk_is_token(pc, CT_CARET)
      || chunk_is_token(pc, CT_ANGLE_OPEN)
      || chunk_is_token(pc, CT_ANGLE_CLOSE)
      || chunk_is_token(pc, CT_RETURN)
      || chunk_is_token(pc, CT_THROW)
      || chunk_is_token(pc, CT_GOTO)
      || chunk_is_token(pc, CT_CONTINUE)
      || chunk_is_token(pc, CT_PAREN_OPEN)
      || chunk_is_token(pc, CT_FPAREN_OPEN)
      || chunk_is_token(pc, CT_SPAREN_OPEN)
      || chunk_is_token(pc, CT_BRACE_OPEN)
      || chunk_is_semicolon(pc)
      || chunk_is_token(pc, CT_COMMA)
      || chunk_is_token(pc, CT_NOT)
      || chunk_is_token(pc, CT_INV)
      || chunk_is_token(pc, CT_COLON)
      || chunk_is_token(pc, CT_QUESTION))
   {
      frm.expr_count = 0;
      LOG_FMT(LSTMT, "%s(%d): orig_line is %zu, orig_col is %zu, reset expr on '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
   }
   else if (  chunk_is_token(pc, CT_BRACE_CLOSE)
           && !cpd.consumed
           && cpd.in_preproc != CT_PP_DEFINE)
   {
      size_t file_pp_level = ifdef_over_whole_file() ? 1 : 0;
      if (!cpd.unc_off_used && pc->pp_level == file_pp_level)
      {
         // fatal error
         LOG_FMT(LERR, "%s(%d): Unmatched BRACE_CLOSE\n   orig_line is %zu, orig_col is %zu\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col);
         if (!options::tok_split_gte())
         {
            LOG_FMT(LERR, "%s(%d): Try the option 'tok_split_gte = true'\n",
                    __func__, __LINE__);
         }
         exit(EXIT_FAILURE);
      }
   }
} // parse_cleanup


static bool check_complex_statements(ParseFrame &frm, chunk_t *pc)
{
   LOG_FUNC_ENTRY();

   // Turn an optional parenthesis into either a real parenthesis or a brace
   if (frm.top().stage == brace_stage_e::OP_PAREN1)
   {
      frm.top().stage = (pc->type != CT_PAREN_OPEN)
                        ? brace_stage_e::BRACE2
                        : brace_stage_e::PAREN1;
   }

   // Check for CT_ELSE after CT_IF
   while (frm.top().stage == brace_stage_e::ELSE)
   {
      if (chunk_is_token(pc, CT_ELSE))
      {
         // Replace CT_IF with CT_ELSE on the stack & we are done
         frm.top().type  = CT_ELSE;
         frm.top().stage = brace_stage_e::ELSEIF;
         print_stack(LBCSSWAP, "=Swap   ", frm);

         return(true);
      }

      // Remove the CT_IF and close the statement
      LOG_FMT(LBCSPOP, "%s(%d): pc->orig_line is %zu, orig_col is %zu, text() is '%s', type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
      frm.pop();
      print_stack(LBCSPOP, "-IF-CCS ", frm);
      if (close_statement(frm, pc))
      {
         return(true);
      }
   }

   // Check for CT_IF after CT_ELSE
   if (frm.top().stage == brace_stage_e::ELSEIF)
   {
      if (  chunk_is_token(pc, CT_IF)
         && (  !options::indent_else_if()
            || !chunk_is_newline(chunk_get_prev_nc(pc))))
      {
         // Replace CT_ELSE with CT_IF
         set_chunk_type(pc, CT_ELSEIF);
         frm.top().type  = CT_ELSEIF;
         frm.top().stage = brace_stage_e::PAREN1;
         return(true);
      }

      // Jump to the 'expecting brace' stage
      frm.top().stage = brace_stage_e::BRACE2;
   }

   // Check for CT_CATCH or CT_FINALLY after CT_TRY or CT_CATCH
   while (frm.top().stage == brace_stage_e::CATCH)
   {
      if (chunk_is_token(pc, CT_CATCH) || chunk_is_token(pc, CT_FINALLY))
      {
         // Replace CT_TRY with CT_CATCH or CT_FINALLY on the stack & we are done
         frm.top().type = pc->type;
         if (language_is_set(LANG_CS))
         {
            frm.top().stage = (chunk_is_token(pc, CT_CATCH)) ? brace_stage_e::CATCH_WHEN : brace_stage_e::BRACE2;
         }
         else
         {
            // historically this used OP_PAREN1; however, to my knowledge the expression after a catch clause
            // is only optional for C# which has been handled above; therefore, this should now always expect
            // a parenthetical expression after the catch keyword and brace after the finally keyword
            frm.top().stage = (chunk_is_token(pc, CT_CATCH)) ? brace_stage_e::PAREN1 : brace_stage_e::BRACE2;
         }
         print_stack(LBCSSWAP, "=Swap   ", frm);

         return(true);
      }

      // Remove the CT_TRY and close the statement
      LOG_FMT(LBCSPOP, "%s(%d): pc->orig_line is %zu, orig_col is %zu, text() is '%s', type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
      frm.pop();
      print_stack(LBCSPOP, "-TRY-CCS ", frm);

      if (close_statement(frm, pc))
      {
         return(true);
      }
   }

   // Check for optional parenthesis and optional CT_WHEN after CT_CATCH
   if (frm.top().stage == brace_stage_e::CATCH_WHEN)
   {
      if (chunk_is_token(pc, CT_PAREN_OPEN)) // this is for the paren after "catch"
      {
         // Replace CT_PAREN_OPEN with CT_SPAREN_OPEN
         set_chunk_type(pc, CT_SPAREN_OPEN);
         frm.top().type  = pc->type;
         frm.top().stage = brace_stage_e::PAREN1;

         return(false);
      }

      if (chunk_is_token(pc, CT_WHEN))
      {
         frm.top().type  = pc->type;
         frm.top().stage = brace_stage_e::OP_PAREN1;

         return(true);
      }

      if (chunk_is_token(pc, CT_BRACE_OPEN))
      {
         frm.top().stage = brace_stage_e::BRACE2;

         return(false);
      }
   }

   // Check for CT_WHILE after the CT_DO
   if (frm.top().stage == brace_stage_e::WHILE)
   {
      if (chunk_is_token(pc, CT_WHILE))
      {
         set_chunk_type(pc, CT_WHILE_OF_DO);
         frm.top().type  = CT_WHILE_OF_DO; //CT_WHILE;
         frm.top().stage = brace_stage_e::WOD_PAREN;

         return(true);
      }

      LOG_FMT(LWARN, "%s(%d): %s, orig_line is %zu, Error: Expected 'while', got '%s'\n",
              __func__, __LINE__, cpd.filename.c_str(), pc->orig_line,
              pc->text());
      LOG_FMT(LBCSPOP, "%s(%d): pc->orig_line is %zu, orig_col is %zu, text() is '%s', type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
      frm.pop();
      print_stack(LBCSPOP, "-Error  ", frm);
      cpd.error_count++;
   }

   // Insert a CT_VBRACE_OPEN, if needed
   // but not in a preprocessor
   if (  pc->type != CT_BRACE_OPEN
      && !(pc->flags & PCF_IN_PREPROC)
      && (  (frm.top().stage == brace_stage_e::BRACE2)
         || (frm.top().stage == brace_stage_e::BRACE_DO)))
   {
      if (  language_is_set(LANG_CS)
         && chunk_is_token(pc, CT_USING_STMT)
         && (!options::indent_using_block()))
      {
         // don't indent the using block
      }
      else
      {
         const c_token_t parent = frm.top().type;

         chunk_t         *vbrace = insert_vbrace_open_before(pc, frm);
         set_chunk_parent(vbrace, parent);

         frm.level++;
         frm.brace_level++;
         LOG_FMT(LBCSPOP, "%s(%d): frm.brace_level increased to %zu\n",
                 __func__, __LINE__, frm.brace_level);
         log_pcf_flags(LBCSPOP, pc->flags);

         frm.push(*vbrace, brace_stage_e::NONE);
         // "+VBrace");

         frm.top().parent = parent;

         // update the level of pc
         pc->level       = frm.level;
         pc->brace_level = frm.brace_level;

         // Mark as a start of a statement
         frm.stmt_count = 0;
         frm.expr_count = 0;
         LOG_FMT(LTOK, "%s(%d): frm.stmt_count is %zu, frm.expr_count is %zu\n",
                 __func__, __LINE__, frm.stmt_count, frm.expr_count);
         pc->flags     |= PCF_STMT_START | PCF_EXPR_START;
         frm.stmt_count = 1;
         frm.expr_count = 1;
         LOG_FMT(LSTMT, "%s(%d): orig_line is %zu, 2.marked '%s' as stmt start\n",
                 __func__, __LINE__, pc->orig_line, pc->text());
      }
   }

   // Check for "constexpr" after CT_IF or CT_ELSEIF
   if (  frm.top().stage == brace_stage_e::PAREN1
      && (  frm.top().type == CT_IF
         || frm.top().type == CT_ELSEIF)
      && pc->str.equals("constexpr")) // FIXME: Take care of the "constexpr" const string.
   {
      return(false);
   }

   // Verify open parenthesis in complex statement
   if (  pc->type != CT_PAREN_OPEN
      && (  (frm.top().stage == brace_stage_e::PAREN1)
         || (frm.top().stage == brace_stage_e::WOD_PAREN)))
   {
      LOG_FMT(LWARN, "%s(%d): %s, orig_line is %zu, Error: Expected '(', got '%s' for '%s'\n",
              __func__, __LINE__, cpd.filename.c_str(), pc->orig_line, pc->text(),
              get_token_name(frm.top().type));

      // Throw out the complex statement
      LOG_FMT(LBCSPOP, "%s(%d): pc->orig_line is %zu, orig_col is %zu, text() is '%s', type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
      frm.pop();
      print_stack(LBCSPOP, "-Error  ", frm);
      cpd.error_count++;
   }

   return(false);
} // check_complex_statements


static bool handle_complex_close(ParseFrame &frm, chunk_t *pc)
{
   LOG_FUNC_ENTRY();

   if (frm.top().stage == brace_stage_e::PAREN1)
   {
      if (pc->next != nullptr && pc->next->type == CT_WHEN)
      {
         frm.top().type  = pc->type;
         frm.top().stage = brace_stage_e::CATCH_WHEN;

         return(true);
      }

      // PAREN1 always => BRACE2
      frm.top().stage = brace_stage_e::BRACE2;
   }
   else if (frm.top().stage == brace_stage_e::BRACE2)
   {
      // BRACE2: IF => ELSE, anything else => close
      if (  (frm.top().type == CT_IF)
         || (frm.top().type == CT_ELSEIF))
      {
         frm.top().stage = brace_stage_e::ELSE;

         // If the next chunk isn't CT_ELSE, close the statement
         chunk_t *next = chunk_get_next_ncnl(pc);
         if (next != nullptr && next->type != CT_ELSE)
         {
            LOG_FMT(LBCSPOP, "%s(%d): no CT_ELSE, pc->orig_line is %zu, orig_col is %zu, text() is '%s', type is %s\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
            frm.pop();
            print_stack(LBCSPOP, "-IF-HCS ", frm);

            return(close_statement(frm, pc));
         }
      }
      else if (  (frm.top().type == CT_TRY)
              || (frm.top().type == CT_CATCH))
      {
         frm.top().stage = brace_stage_e::CATCH;

         // If the next chunk isn't CT_CATCH or CT_FINALLY, close the statement
         chunk_t *next = chunk_get_next_ncnl(pc);
         if (  next != nullptr
            && next->type != CT_CATCH
            && next->type != CT_FINALLY)
         {
            LOG_FMT(LBCSPOP, "%s(%d): pc->orig_line is %zu, orig_col is %zu, text() is '%s', type is %s\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
            frm.pop();
            print_stack(LBCSPOP, "-TRY-HCS ", frm);

            return(close_statement(frm, pc));
         }
      }
      else
      {
         LOG_FMT(LNOTE, "%s(%d): close_statement on %s brace_stage_e::BRACE2\n",
                 __func__, __LINE__, get_token_name(frm.top().type));
         LOG_FMT(LBCSPOP, "%s(%d): pc->orig_line is %zu, orig_col is %zu, text() is '%s', type is %s\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
         frm.pop();
         print_stack(LBCSPOP, "-HCC B2 ", frm);

         return(close_statement(frm, pc));
      }
   }
   else if (frm.top().stage == brace_stage_e::BRACE_DO)
   {
      frm.top().stage = brace_stage_e::WHILE;
   }
   else if (frm.top().stage == brace_stage_e::WOD_PAREN)
   {
      LOG_FMT(LNOTE, "%s(%d): close_statement on %s brace_stage_e::WOD_PAREN\n",
              __func__, __LINE__, get_token_name(frm.top().type));
      frm.top().stage = brace_stage_e::WOD_SEMI;
      print_stack(LBCSPOP, "-HCC WoDP ", frm);
   }
   else if (frm.top().stage == brace_stage_e::WOD_SEMI)
   {
      LOG_FMT(LNOTE, "%s(%d): close_statement on %s brace_stage_e::WOD_SEMI\n",
              __func__, __LINE__, get_token_name(frm.top().type));
      LOG_FMT(LBCSPOP, "%s(%d): pc->orig_line is %zu, orig_col is %zu, text() is '%s', type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
      frm.pop();
      print_stack(LBCSPOP, "-HCC WoDS ", frm);

      return(close_statement(frm, pc));
   }
   else
   {
      // PROBLEM
      LOG_FMT(LWARN, "%s(%d): %s:%zu Error: TOS.type='%s' TOS.stage=%u\n",
              __func__, __LINE__, cpd.filename.c_str(), pc->orig_line,
              get_token_name(frm.top().type),
              (unsigned int)frm.top().stage);
      cpd.error_count++;
   }
   return(false);
} // handle_complex_close


static void mark_namespace(chunk_t *pns)
{
   LOG_FUNC_ENTRY();
   // Issue #1813
   chunk_t *br_close;
   bool    is_using = false;

   chunk_t *pc = chunk_get_prev_ncnl(pns);
   if (chunk_is_token(pc, CT_USING))
   {
      is_using = true;
      set_chunk_parent(pns, CT_USING);
   }

   pc = chunk_get_next_ncnl(pns);
   while (pc != nullptr)
   {
      set_chunk_parent(pc, CT_NAMESPACE);
      if (pc->type != CT_BRACE_OPEN)
      {
         if (chunk_is_token(pc, CT_SEMICOLON))
         {
            if (is_using)
            {
               set_chunk_parent(pc, CT_USING);
            }
            return;
         }
         pc = chunk_get_next_ncnl(pc);
         continue;
      }

      if (  (options::indent_namespace_limit() > 0)
         && ((br_close = chunk_skip_to_match(pc)) != nullptr))
      {
         // br_close->orig_line is always >= pc->orig_line;
         size_t diff = br_close->orig_line - pc->orig_line;

         if (diff > options::indent_namespace_limit())
         {
            chunk_flags_set(pc, PCF_LONG_BLOCK);
            chunk_flags_set(br_close, PCF_LONG_BLOCK);
         }
      }
      flag_parens(pc, PCF_IN_NAMESPACE, CT_NONE, CT_NAMESPACE, false);
      return;
   }
} // mark_namespace


static chunk_t *insert_vbrace(chunk_t *pc, bool after, const ParseFrame &frm)
{
   LOG_FUNC_ENTRY();

   chunk_t chunk;
   chunk.orig_line   = pc->orig_line;
   chunk.parent_type = frm.top().type;
   chunk.level       = frm.level;
   chunk.brace_level = frm.brace_level;
   chunk.flags       = pc->flags & PCF_COPY_FLAGS;
   chunk.str         = "";

   if (after)
   {
      chunk.orig_col = pc->orig_col;
      chunk.type     = CT_VBRACE_CLOSE;
      return(chunk_add_after(&chunk, pc));
   }


   chunk_t *ref = chunk_get_prev(pc);
   if (ref == nullptr)
   {
      return(nullptr);
   }

   if ((ref->flags & PCF_IN_PREPROC) == 0)
   {
      chunk.flags &= ~PCF_IN_PREPROC;
   }

   while (chunk_is_newline(ref) || chunk_is_comment(ref))
   {
      ref->level++;
      ref->brace_level++;
      ref = chunk_get_prev(ref);
   }
   if (ref == nullptr)
   {
      return(nullptr);
   }

   // Don't back into a preprocessor
   if (  (pc->flags & PCF_IN_PREPROC) == 0
      && (ref->flags & PCF_IN_PREPROC) != 0)
   {
      if (chunk_is_token(ref, CT_PREPROC_BODY))
      {
         while (ref != nullptr && (ref->flags & PCF_IN_PREPROC))
         {
            ref = chunk_get_prev(ref);
         }
      }
      else
      {
         ref = chunk_get_next(ref);
      }
   }
   if (ref == nullptr)
   {
      return(nullptr);
   }

   chunk.orig_line = ref->orig_line;
   chunk.orig_col  = ref->orig_col;
   chunk.column    = ref->column + ref->len() + 1;
   chunk.type      = CT_VBRACE_OPEN;

   return(chunk_add_after(&chunk, ref));
} // insert_vbrace


bool close_statement(ParseFrame &frm, chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   if (pc == nullptr)
   {
      throw invalid_argument(string(__func__) + ":" + to_string(__LINE__)
                             + "args cannot be nullptr");
   }
   LOG_FMT(LTOK, "%s(%d): orig_line is %zu, type is %s, '%s' type is %s, stage is %u\n",
           __func__, __LINE__, pc->orig_line,
           get_token_name(pc->type), pc->text(),
           get_token_name(frm.top().type),
           (unsigned int)frm.top().stage);

   if (cpd.consumed)
   {
      frm.stmt_count = 0;
      frm.expr_count = 0;
      LOG_FMT(LSTMT, "%s(%d): orig_line is %zu> reset2 stmt on '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->text());
   }

   /*
    * Insert a CT_VBRACE_CLOSE, if needed:
    * If we are in a virtual brace and we are not ON a CT_VBRACE_CLOSE add one
    */
   chunk_t *vbc = pc;
   if (frm.top().type == CT_VBRACE_OPEN)
   {
      // If the current token has already been consumed, then add after it
      if (cpd.consumed)
      {
         insert_vbrace_close_after(pc, frm);
      }
      else
      {
         // otherwise, add before it and consume the vbrace
         vbc = chunk_get_prev_ncnl(pc);
         vbc = insert_vbrace_close_after(vbc, frm);
         set_chunk_parent(vbc, frm.top().parent);

         frm.level--;
         frm.brace_level--;
         LOG_FMT(LBCSPOP, "%s(%d): frm.brace_level decreased to %zu\n",
                 __func__, __LINE__, frm.brace_level);
         log_pcf_flags(LBCSPOP, pc->flags);
         LOG_FMT(LBCSPOP, "%s(%d): pc->orig_line is %zu, orig_col is %zu, text() is '%s', type is %s\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
         frm.pop();

         // Update the token level
         pc->level       = frm.level;
         pc->brace_level = frm.brace_level;

         print_stack(LBCSPOP, "-CS VB  ", frm);

         // And repeat the close
         close_statement(frm, pc);
         return(true);
      }
   }

   // See if we are done with a complex statement
   if (frm.top().stage != brace_stage_e::NONE)
   {
      if (handle_complex_close(frm, vbc))
      {
         return(true);
      }
   }
   return(false);
} // close_statement
