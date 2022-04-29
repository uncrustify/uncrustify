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

#include "flag_parens.h"
#include "frame_list.h"
#include "keywords.h"
#include "lang_pawn.h"
#include "prototypes.h"

#include <stdexcept>            // to get std::invalid_argument

constexpr static auto LCURRENT = LBC;

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


struct BraceState
{
   std::vector<ParseFrame> frames     = {};
   E_Token                 in_preproc = CT_NONE;
   int                     pp_level   = 0;
   bool                    consumed   = false;
};

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
static bool close_statement(ParseFrame &frm, Chunk *pc, const BraceState &braceState);


static size_t preproc_start(BraceState &braceState, ParseFrame &frm, Chunk *pc);


static void print_stack(log_sev_t logsev, const char *str, const ParseFrame &frm);


/**
 * pc is a CT_WHILE.
 * Scan backwards to see if we find a brace/vbrace with the parent set to CT_DO
 */
static bool maybe_while_of_do(Chunk *pc);


/**
 * @param after  determines: true  - insert_vbrace_close_after(pc, frm)
 *                           false - insert_vbrace_open_before(pc, frm)
 */
static Chunk *insert_vbrace(Chunk *pc, bool after, const ParseFrame &frm);

#define insert_vbrace_close_after(pc, frm)    insert_vbrace(pc, true, frm)
#define insert_vbrace_open_before(pc, frm)    insert_vbrace(pc, false, frm)

static void parse_cleanup(BraceState &braceState, ParseFrame &frm, Chunk *pc);


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
static bool check_complex_statements(ParseFrame &frm, Chunk *pc, const BraceState &braceState);


/**
 * Handles a close paren or brace - just progress the stage, if the end
 * of the statement is hit, call close_statement()
 *
 * @param frm  The parse frame
 * @param pc   The current chunk
 *
 * @return true - done with this chunk, false - keep processing
 */
static bool handle_complex_close(ParseFrame &frm, Chunk *pc, const BraceState &braceState);


//! We're on a 'namespace' skip the word and then set the parent of the braces.
static void mark_namespace(Chunk *pns);


static size_t preproc_start(BraceState &braceState, ParseFrame &frm, Chunk *pc)
{
   LOG_FUNC_ENTRY();
   const size_t pp_level = braceState.pp_level;

   Chunk        *next = pc->GetNextNcNnl();

   if (next->IsNullChunk())
   {
      return(pp_level);
   }
   // Get the type of preprocessor and handle it
   braceState.in_preproc = next->type;

   // If we are not in a define, check for #if, #else, #endif, etc
   if (braceState.in_preproc != CT_PP_DEFINE)
   {
      int pp_indent = fl_check(braceState.frames, frm, braceState.pp_level, pc);
      return(pp_indent);
   }
   // else push the frame stack
   fl_push(braceState.frames, frm);

   // a preproc body starts a new, blank frame
   frm             = {};
   frm.level       = 1;
   frm.brace_level = 1;

   // TODO: not sure about the next 3 lines
   frm.push(nullptr);
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

   BraceState braceState;
   ParseFrame frm{};
   Chunk      *pc = Chunk::GetHead();

   while (pc->IsNotNullChunk())
   {
      LOG_FMT(LTOK, "%s(%d): orig_line is %zu, orig_col is %zu, Text() is '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());

      // Check for leaving a #define body
      if (  braceState.in_preproc != CT_NONE
         && !pc->flags.test(PCF_IN_PREPROC))
      {
         if (braceState.in_preproc == CT_PP_DEFINE)
         {
            // out of the #define body, restore the frame
            size_t brace_level = frm.brace_level;

            if (  options::pp_warn_unbalanced_if()
               && brace_level != 1)
            {
               LOG_FMT(LWARN, "%s(%d): orig_line is %zu, unbalanced #define block braces, out-level is %zu\n",
                       __func__, __LINE__, pc->orig_line, brace_level);
            }
            fl_pop(braceState.frames, frm);
         }
         braceState.in_preproc = CT_NONE;
      }
      // Check for a preprocessor start
      size_t pp_level;

      if (chunk_is_token(pc, CT_PREPROC))
      {
         pp_level = preproc_start(braceState, frm, pc);
      }
      else
      {
         pp_level = braceState.pp_level;
      }
      LOG_FMT(LTOK, "%s(%d): pp_level is %zu\n",
              __func__, __LINE__, pp_level);

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
      if (  !pc->IsComment()
         && !chunk_is_newline(pc)
         && !chunk_is_token(pc, CT_ATTRIBUTE)
         && !chunk_is_token(pc, CT_IGNORED)            // Issue #2279
         && (  braceState.in_preproc == CT_PP_DEFINE
            || braceState.in_preproc == CT_NONE))
      {
         braceState.consumed = false;
         parse_cleanup(braceState, frm, pc);
         print_stack(LBCSAFTER, (chunk_is_token(pc, CT_VBRACE_CLOSE)) ? "Virt-}\n" : pc->str.c_str(), frm);
      }
      pc = pc->GetNext();
   }
//   pc = Chunk::GetHead();
//
//   while (pc->IsNotNullChunk())
//   {
//      LOG_FMT(LTOK, "%s(%d): orig_line is %zu, orig_col is %zu, Text() is '%s'\n",
//              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
//
//      // look for template
//      if (chunk_is_token(pc, CT_TEMPLATE))                 // Issue #3309
//      {
//         Chunk *template_end = pc->GetNextType(CT_SEMICOLON, pc->level);
//
//         // look for a parameter pack
//         while (pc->IsNotNullChunk())
//         {
//            LOG_FMT(LTOK, "%s(%d): orig_line is %zu, orig_col is %zu, Text() is '%s'\n",
//                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
//
//            if (chunk_is_token(pc, CT_PARAMETER_PACK))
//            {
//               Chunk *parameter_pack = pc;
//
//               // look for a token with the same text
//               while (pc->IsNotNullChunk())
//               {
//                  LOG_FMT(LTOK, "%s(%d): orig_line is %zu, orig_col is %zu, Text() is '%s'\n",
//                          __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
//                  pc = pc->GetNext();
//
//                  if (pc == template_end)
//                  {
//                     break;
//                  }
//
//                  if (strcmp(pc->Text(), parameter_pack->Text()) == 0)
//                  {
//                     set_chunk_type(pc, CT_PARAMETER_PACK);
//                  }
//               }
//            }
//            pc = pc->GetNext();
//
//            if (pc == template_end)
//            {
//               break;
//            }
//         }
//      }
//      pc = pc->GetNext();
//   }
} // brace_cleanup


static bool maybe_while_of_do(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   Chunk *prev = pc->GetPrevNcNnl();

   if (  prev->IsNullChunk()
      || !prev->flags.test(PCF_IN_PREPROC))
   {
      return(false);
   }

   // Find the chunk before the preprocessor
   while (  prev->IsNullChunk()
         && prev->flags.test(PCF_IN_PREPROC))
   {
      prev = prev->GetPrevNcNnl();
   }

   if (  (  chunk_is_token(prev, CT_VBRACE_CLOSE)
         || chunk_is_token(prev, CT_BRACE_CLOSE))
      && get_chunk_parent_type(prev) == CT_DO)
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
static void parse_cleanup(BraceState &braceState, ParseFrame &frm, Chunk *pc)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LTOK, "%s(%d): orig_line is %zu, orig_col is %zu, type is %s, tos is %zu, TOS.type is %s, TOS.stage is %s, ",
           __func__, __LINE__, pc->orig_line, pc->orig_col, get_token_name(pc->type),
           frm.size() - 1, get_token_name(frm.top().type),
           get_brace_stage_name(frm.top().stage));
   log_pcf_flags(LTOK, pc->flags);

   // Mark statement starts
   LOG_FMT(LTOK, "%s(%d): orig_line is %zu, type is %s, Text() is '%s'\n",
           __func__, __LINE__, pc->orig_line, get_token_name(pc->type), pc->Text());
   LOG_FMT(LTOK, "%s(%d): frm.stmt_count is %zu, frm.expr_count is %zu\n",
           __func__, __LINE__, frm.stmt_count, frm.expr_count);

   if (  (  frm.stmt_count == 0
         || frm.expr_count == 0)
      && !chunk_is_semicolon(pc)
      && chunk_is_not_token(pc, CT_BRACE_CLOSE)
      && chunk_is_not_token(pc, CT_VBRACE_CLOSE)
      && !chunk_is_str(pc, ")")
      && !chunk_is_str(pc, "]"))
   {
      chunk_flags_set(pc, PCF_EXPR_START | ((frm.stmt_count == 0) ? PCF_STMT_START : PCF_NONE));
      LOG_FMT(LSTMT, "%s(%d): orig_line is %zu, 1.marked '%s' as %s, start stmt_count is %zu, expr_count is %zu\n",
              __func__, __LINE__, pc->orig_line, pc->Text(),
              pc->flags.test(PCF_STMT_START) ? "stmt" : "expr", frm.stmt_count,
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
      && check_complex_statements(frm, pc, braceState))
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
         braceState.consumed = true;
         close_statement(frm, pc, braceState);
      }
      else if (  language_is_set(LANG_PAWN)
              && chunk_is_token(pc, CT_BRACE_CLOSE))
      {
         close_statement(frm, pc, braceState);
      }
      else if (  language_is_set(LANG_D)
              && chunk_is_token(pc, CT_BRACE_CLOSE))
      {
         close_statement(frm, pc, braceState);
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
         set_chunk_type(pc, static_cast<E_Token>(frm.top().type + 1));

         if (chunk_is_token(pc, CT_SPAREN_CLOSE))
         {
            frm.sparen_count--;
            chunk_flags_clr(pc, PCF_IN_SPAREN);
         }
      }

      // Make sure the open / close match
      if (chunk_is_not_token(pc, (E_Token)(frm.top().type + 1)))
      {
         if (pc->flags.test(PCF_IN_PREPROC))                // Issue #3113, #3283
         {
            // do nothing
         }
         else
         {
            LOG_FMT(LWARN, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
            paren_stack_entry_t AA = frm.top();                // Issue #3055

            if (AA.type != CT_EOF)
            {
               LOG_FMT(LWARN, "%s(%d): (frm.top().type + 1) is %s\n",
                       __func__, __LINE__, get_token_name((E_Token)(frm.top().type + 1)));
            }

            if (  frm.top().type != CT_EOF
               && frm.top().type != CT_PP_DEFINE)
            {
               LOG_FMT(LWARN, "%s(%d): File: %s, orig_line is %zu, orig_col is %zu, Error: Unexpected '%s' for '%s', which was on line %zu\n",
                       __func__, __LINE__, cpd.filename.c_str(), pc->orig_line, pc->orig_col,
                       pc->Text(), get_token_name(frm.top().pc->type),
                       frm.top().pc->orig_line);
               print_stack(LBCSPOP, "=Error  ", frm);
               cpd.error_count++;
               exit(EXIT_FAILURE);
            }
         }
      }
      else
      {
         braceState.consumed = true;

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
         LOG_FMT(LBCSPOP, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
         frm.pop(__func__, __LINE__, pc);
         print_stack(LBCSPOP, "-Close  ", frm);

         if (  frm.top().stage == brace_stage_e::NONE
            && (  chunk_is_token(pc, CT_VBRACE_CLOSE)
               || chunk_is_token(pc, CT_BRACE_CLOSE)
               || chunk_is_token(pc, CT_SEMICOLON))
            && chunk_is_token(frm.top().pc, CT_VBRACE_OPEN))
         {
            // frames for functions are not created as they are for an if
            // this here is a hackish solution to close a vbrace of a block that
            // contains the function
            frm.push(nullptr); // <- dummy frame for the function
            frm.top().stage = brace_stage_e::BRACE2;
         }

         // See if we are in a complex statement
         if (frm.top().stage != brace_stage_e::NONE)
         {
            handle_complex_close(frm, pc, braceState);
         }
      }
   }

   /*
    * In this state, we expect a semicolon, but we'll also hit the closing
    * sparen, so we need to check braceState.consumed to see if the close sparen
    * was aleady handled.
    */
   if (frm.top().stage == brace_stage_e::WOD_SEMI)
   {
      if (braceState.consumed)
      {
         /*
          * If consumed, then we are on the close sparen.
          * PAWN: Check the next chunk for a semicolon. If it isn't, then
          * add a virtual semicolon, which will get handled on the next pass.
          */
         if (language_is_set(LANG_PAWN))
         {
            Chunk *tmp = pc->GetNextNcNnl();

            if (  chunk_is_not_token(tmp, CT_SEMICOLON)
               && chunk_is_not_token(tmp, CT_VSEMICOLON))
            {
               pawn_add_vsemi_after(pc);
            }
         }
      }
      else
      {
         // Complain if this ISN'T a semicolon, but close out WHILE_OF_DO anyway
         if (  chunk_is_token(pc, CT_SEMICOLON)
            || chunk_is_token(pc, CT_VSEMICOLON))
         {
            braceState.consumed = true;
            set_chunk_parent(pc, CT_WHILE_OF_DO);
         }
         else
         {
            LOG_FMT(LWARN, "%s: %s(%d): %zu: Error: Expected a semicolon for WHILE_OF_DO, but got '%s'\n",
                    cpd.filename.c_str(), __func__, __LINE__, pc->orig_line,
                    get_token_name(pc->type));
            cpd.error_count++;
         }
         handle_complex_close(frm, pc, braceState);
      }
   }
   // Get the parent type for brace and parenthesis open
   E_Token parent = get_chunk_parent_type(pc);

   if (  chunk_is_token(pc, CT_PAREN_OPEN)
      || chunk_is_token(pc, CT_FPAREN_OPEN)
      || chunk_is_token(pc, CT_SPAREN_OPEN)
      || chunk_is_token(pc, CT_BRACE_OPEN))
   {
      Chunk *prev = pc->GetPrevNcNnl();

      if (prev->IsNotNullChunk())
      {
         if (  chunk_is_token(pc, CT_PAREN_OPEN)
            || chunk_is_token(pc, CT_FPAREN_OPEN)
            || chunk_is_token(pc, CT_SPAREN_OPEN))
         {
            // Set the parent for parenthesis and change parenthesis type
            if (  chunk_is_token(prev, CT_IF)
               || chunk_is_token(prev, CT_CONSTEXPR)
               || chunk_is_token(prev, CT_ELSEIF)
               || chunk_is_token(prev, CT_WHILE)
               || chunk_is_token(prev, CT_WHILE_OF_DO)
               || chunk_is_token(prev, CT_DO)
               || chunk_is_token(prev, CT_FOR)
               || chunk_is_token(prev, CT_SWITCH)
               || chunk_is_token(prev, CT_CATCH)
               || chunk_is_token(prev, CT_SYNCHRONIZED)
               || chunk_is_token(prev, CT_D_VERSION)
               || chunk_is_token(prev, CT_D_VERSION_IF)
               || chunk_is_token(prev, CT_D_SCOPE)
               || chunk_is_token(prev, CT_D_SCOPE_IF))
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
            else if (  chunk_is_token(prev, CT_ENUM)
                    && language_is_set(LANG_OC))
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
            else if (  chunk_is_token(prev, CT_ASSIGN)
                    && (prev->str[0] == '='))
            {
               parent = CT_ASSIGN;
            }
            else if (  chunk_is_token(prev, CT_RETURN)
                    && language_is_set(LANG_CPP))
            {
               parent = CT_RETURN;
            }
            // Carry through CT_ENUM parent in NS_ENUM (type, name) {
            // only to help the vim command }
            else if (  chunk_is_token(prev, CT_FPAREN_CLOSE)
                    && language_is_set(LANG_OC)
                    && get_chunk_parent_type(prev) == CT_ENUM)
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

      if (  chunk_is_token(pc, CT_BRACE_OPEN)
         || chunk_is_token(pc, CT_MACRO_OPEN))
      {
         // Issue #1813
         bool single = false;

         if (get_chunk_parent_type(pc) == CT_NAMESPACE)
         {
            LOG_FMT(LBCSPOP, "%s(%d): parent_type is NAMESPACE\n",
                    __func__, __LINE__);
            Chunk *tmp = frm.top().pc;

            if (  tmp != nullptr
               && get_chunk_parent_type(tmp) == CT_NAMESPACE)
            {
               LOG_FMT(LBCSPOP, "%s(%d): tmp->parent_type is NAMESPACE\n",
                       __func__, __LINE__);

               log_rule_B("indent_namespace");
               log_rule_B("indent_namespace_single_indent");

               if (  options::indent_namespace()
                  && options::indent_namespace_single_indent())
               {
                  LOG_FMT(LBCSPOP, "%s(%d): Options are SINGLE\n",
                          __func__, __LINE__);
                  single = true;
               }
            }
         }
         LOG_FMT(LBCSPOP, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s, parent_type is %s\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type), get_token_name(get_chunk_parent_type(pc)));

         if (!single)
         {
            frm.brace_level++;
            LOG_FMT(LBCSPOP, "%s(%d): frm.brace_level increased to %zu\n",
                    __func__, __LINE__, frm.brace_level);
         }
      }
      frm.push(pc, __func__, __LINE__);
      frm.top().parent = parent;
      // set parent type
      set_chunk_parent(pc, parent);
   }
   // Issue #2281

   if (  chunk_is_token(pc, CT_BRACE_OPEN)
      && pc->parent_type == CT_SWITCH)
   {
      size_t idx = frm.size();
      LOG_FMT(LBCSPOP, "%s(%d): idx is %zu\n",
              __func__, __LINE__, idx);
      Chunk *saved = frm.at(idx - 2).pc;

      if (saved != nullptr)
      {
         // set parent member
         chunk_set_parent(pc, saved);
      }
   }

   if (  chunk_is_token(pc, CT_CASE)
      || chunk_is_token(pc, CT_DEFAULT))
   {
      Chunk *prev = pc->GetPrevNcNnl();         // Issue #3176

      if (  chunk_is_token(pc, CT_CASE)
         || (  chunk_is_token(pc, CT_DEFAULT)
            && chunk_is_not_token(prev, CT_ASSIGN)))
      {
         // it is a CT_DEFAULT from a switch
         LOG_FMT(LBCSPOP, "%s(%d): pc->orig_line is %zu, pc->orig_col is %zu\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col);
         set_chunk_parent(pc, CT_SWITCH);
         size_t idx = frm.size();
         LOG_FMT(LBCSPOP, "%s(%d): idx is %zu\n",
                 __func__, __LINE__, idx);
         Chunk *saved = frm.at(idx - 2).pc;

         if (saved != nullptr)
         {
            // set parent member
            chunk_set_parent(pc, saved);
         }
      }
   }

   if (chunk_is_token(pc, CT_BREAK))
   {
      LOG_FMT(LBCSPOP, "%s(%d): pc->orig_line is %zu, pc->orig_col is %zu\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col);
      size_t idx = frm.size();
      LOG_FMT(LBCSPOP, "%s(%d): idx is %zu\n",
              __func__, __LINE__, idx);
      Chunk *saved = frm.at(idx - 2).pc;

      if (saved != nullptr)
      {
         // set parent member
         chunk_set_parent(pc, saved);
      }
   }
   const pattern_class_e patcls = get_token_pattern_class(pc->type);

   /*
    * Create a stack entry for complex statements:
    * if, elseif, switch, for, while, synchronized, using, lock, with,
    * version, CT_D_SCOPE_IF
    */
   if (patcls == pattern_class_e::BRACED)
   {
      frm.push(pc, __func__, __LINE__, (chunk_is_token(pc, CT_DO) ? brace_stage_e::BRACE_DO
                    : brace_stage_e::BRACE2));
      // "+ComplexBraced"
   }
   else if (patcls == pattern_class_e::PBRACED)
   {
      brace_stage_e bs = brace_stage_e::PAREN1;

      if (  chunk_is_token(pc, CT_WHILE)
         && maybe_while_of_do(pc))
      {
         set_chunk_type(pc, CT_WHILE_OF_DO);
         bs = brace_stage_e::WOD_PAREN;
      }
      frm.push(pc, __func__, __LINE__, bs);
      // "+ComplexParenBraced"
   }
   else if (patcls == pattern_class_e::OPBRACED)
   {
      frm.push(pc, __func__, __LINE__, brace_stage_e::OP_PAREN1);
      // "+ComplexOpParenBraced");
   }
   else if (patcls == pattern_class_e::ELSE)
   {
      frm.push(pc, __func__, __LINE__, brace_stage_e::ELSEIF);
      // "+ComplexElse");
   }

   /*
    * Mark simple statement/expression starts
    *  - after { or }
    *  - after ';', but not if the paren stack top is a paren
    *  - after '(' that has a parent type of CT_FOR
    */
   if (  chunk_is_token(pc, CT_SQUARE_OPEN)
      || (  chunk_is_token(pc, CT_BRACE_OPEN)
         && get_chunk_parent_type(pc) != CT_ASSIGN)
      || chunk_is_token(pc, CT_BRACE_CLOSE)
      || chunk_is_token(pc, CT_VBRACE_CLOSE)
      || (  chunk_is_token(pc, CT_SPAREN_OPEN)
         && get_chunk_parent_type(pc) == CT_FOR)
      || chunk_is_token(pc, CT_COLON)
      || chunk_is_token(pc, CT_OC_END)
      || (  chunk_is_semicolon(pc)
         && frm.top().type != CT_PAREN_OPEN
         && frm.top().type != CT_FPAREN_OPEN
         && frm.top().type != CT_SPAREN_OPEN)
      || chunk_is_token(pc, CT_MACRO))                         // Issue #2742
   {
      LOG_FMT(LSTMT, "%s(%d): orig_line is %zu, reset1 stmt on '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->Text());
      frm.stmt_count = 0;
      frm.expr_count = 0;
      LOG_FMT(LTOK, "%s(%d): frm.stmt_count is %zu, frm.expr_count is %zu\n",
              __func__, __LINE__, frm.stmt_count, frm.expr_count);
   }
   // Mark expression starts
   LOG_FMT(LSTMT, "%s(%d): Mark expression starts: orig_line is %zu, orig_col is %zu, Text() is '%s'\n",
           __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
   Chunk *tmp = pc->GetNextNcNnl();

   if (  chunk_is_token(pc, CT_ARITH)
      || chunk_is_token(pc, CT_SHIFT)
      || chunk_is_token(pc, CT_ASSIGN)
      || chunk_is_token(pc, CT_CASE)
      || chunk_is_token(pc, CT_COMPARE)
      || (  chunk_is_token(pc, CT_STAR)
         && chunk_is_not_token(tmp, CT_STAR))
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
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
   }
} // parse_cleanup


static bool check_complex_statements(ParseFrame &frm, Chunk *pc, const BraceState &braceState)
{
   LOG_FUNC_ENTRY();

   brace_stage_e atest = frm.top().stage;

   LOG_FMT(LBCSPOP, "%s(%d): atest is %s\n",
           __func__, __LINE__, get_brace_stage_name(atest));

   // Turn an optional parenthesis into either a real parenthesis or a brace
   if (frm.top().stage == brace_stage_e::OP_PAREN1)
   {
      frm.top().stage = (chunk_is_not_token(pc, CT_PAREN_OPEN))
                        ? brace_stage_e::BRACE2
                        : brace_stage_e::PAREN1;
      LOG_FMT(LBCSPOP, "%s(%d): frm.top().stage is now %s\n",
              __func__, __LINE__, get_brace_stage_name(frm.top().stage));
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
      LOG_FMT(LBCSPOP, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
      frm.pop(__func__, __LINE__, pc);
      print_stack(LBCSPOP, "-IF-CCS ", frm);

      if (close_statement(frm, pc, braceState))
      {
         return(true);
      }
   }

   // Check for CT_IF after CT_ELSE
   if (frm.top().stage == brace_stage_e::ELSEIF)
   {
      log_rule_B("indent_else_if");

      if (  chunk_is_token(pc, CT_IF)
         && (  !options::indent_else_if()
            || !chunk_is_newline(pc->GetPrevNc())))
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
      if (  chunk_is_token(pc, CT_CATCH)
         || chunk_is_token(pc, CT_FINALLY))
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
      LOG_FMT(LBCSPOP, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
      frm.pop(__func__, __LINE__, pc);
      print_stack(LBCSPOP, "-TRY-CCS ", frm);

      if (close_statement(frm, pc, braceState))
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
              pc->Text());
      LOG_FMT(LBCSPOP, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
      frm.pop(__func__, __LINE__, pc);
      print_stack(LBCSPOP, "-Error  ", frm);
      cpd.error_count++;
   }
   // Insert a CT_VBRACE_OPEN, if needed
   // but not in a preprocessor
   atest = frm.top().stage;

   if (  chunk_is_not_token(pc, CT_BRACE_OPEN)
      && !pc->flags.test(PCF_IN_PREPROC)
      && (  (frm.top().stage == brace_stage_e::BRACE2)
         || (frm.top().stage == brace_stage_e::BRACE_DO)))
   {
      log_rule_B("indent_using_block");

      if (  language_is_set(LANG_CS)
         && chunk_is_token(pc, CT_USING_STMT)
         && (!options::indent_using_block()))
      {
         // don't indent the using block
      }
      else
      {
         const E_Token parent = frm.top().type;

         Chunk         *vbrace = insert_vbrace_open_before(pc, frm);
         set_chunk_parent(vbrace, parent);

         frm.level++;
         frm.brace_level++;
         LOG_FMT(LBCSPOP, "%s(%d): frm.brace_level increased to %zu\n",
                 __func__, __LINE__, frm.brace_level);
         log_pcf_flags(LBCSPOP, pc->flags);

         frm.push(vbrace, __func__, __LINE__, brace_stage_e::NONE);
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
         chunk_flags_set(pc, PCF_STMT_START | PCF_EXPR_START);
         frm.stmt_count = 1;
         frm.expr_count = 1;
         LOG_FMT(LSTMT, "%s(%d): orig_line is %zu, 2.marked '%s' as stmt start\n",
                 __func__, __LINE__, pc->orig_line, pc->Text());
      }
   }

   // Check for "constexpr" after CT_IF or CT_ELSEIF
   if (  frm.top().stage == brace_stage_e::PAREN1
      && (  frm.top().type == CT_IF
         || frm.top().type == CT_ELSEIF)
      && chunk_is_token(pc, CT_CONSTEXPR))
   {
      return(false);
   }

   // Verify open parenthesis in complex statement
   if (  chunk_is_not_token(pc, CT_PAREN_OPEN)
      && (  (frm.top().stage == brace_stage_e::PAREN1)
         || (frm.top().stage == brace_stage_e::WOD_PAREN)))
   {
      LOG_FMT(LWARN, "%s(%d): %s, orig_line is %zu, Error: Expected '(', got '%s' for '%s'\n",
              __func__, __LINE__, cpd.filename.c_str(), pc->orig_line, pc->Text(),
              get_token_name(frm.top().type));

      // Throw out the complex statement
      LOG_FMT(LBCSPOP, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
      frm.pop(__func__, __LINE__, pc);
      print_stack(LBCSPOP, "-Error  ", frm);
      cpd.error_count++;
   }
   return(false);
} // check_complex_statements


static bool handle_complex_close(ParseFrame &frm, Chunk *pc, const BraceState &braceState)
{
   LOG_FUNC_ENTRY();

   if (frm.top().stage == brace_stage_e::PAREN1)
   {
      if (  pc->next != nullptr
         && pc->next->type == CT_WHEN)
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
         Chunk *next = pc->GetNextNcNnl();

         if (  next->IsNullChunk()
            || chunk_is_not_token(next, CT_ELSE))
         {
            LOG_FMT(LBCSPOP, "%s(%d): no CT_ELSE, pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
            frm.pop(__func__, __LINE__, pc);
            print_stack(LBCSPOP, "-IF-HCS ", frm);

            return(close_statement(frm, pc, braceState));
         }
      }
      else if (  (frm.top().type == CT_TRY)
              || (frm.top().type == CT_CATCH))
      {
         frm.top().stage = brace_stage_e::CATCH;

         // If the next chunk isn't CT_CATCH or CT_FINALLY, close the statement
         Chunk *next = pc->GetNextNcNnl();

         if (  chunk_is_not_token(next, CT_CATCH)
            && chunk_is_not_token(next, CT_FINALLY))
         {
            LOG_FMT(LBCSPOP, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
            frm.pop(__func__, __LINE__, pc);
            print_stack(LBCSPOP, "-TRY-HCS ", frm);

            return(close_statement(frm, pc, braceState));
         }
      }
      else
      {
         LOG_FMT(LNOTE, "%s(%d): close_statement on %s brace_stage_e::BRACE2\n",
                 __func__, __LINE__, get_token_name(frm.top().type));
         LOG_FMT(LBCSPOP, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
         frm.pop(__func__, __LINE__, pc);
         print_stack(LBCSPOP, "-HCC B2 ", frm);

         return(close_statement(frm, pc, braceState));
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
      LOG_FMT(LBCSPOP, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
      frm.pop(__func__, __LINE__, pc);
      print_stack(LBCSPOP, "-HCC WoDS ", frm);

      return(close_statement(frm, pc, braceState));
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


static void mark_namespace(Chunk *pns)
{
   LOG_FUNC_ENTRY();
   // Issue #1813
   Chunk *br_close;
   bool  is_using = false;

   Chunk *pc = pns->GetPrevNcNnl();

   if (chunk_is_token(pc, CT_USING))
   {
      is_using = true;
      set_chunk_parent(pns, CT_USING);
   }
   pc = pns->GetNextNcNnl();

   while (pc->IsNotNullChunk())
   {
      set_chunk_parent(pc, CT_NAMESPACE);

      if (chunk_is_not_token(pc, CT_BRACE_OPEN))
      {
         if (chunk_is_token(pc, CT_SEMICOLON))
         {
            if (is_using)
            {
               set_chunk_parent(pc, CT_USING);
            }
            return;
         }
         pc = pc->GetNextNcNnl();
         continue;
      }
      log_rule_B("indent_namespace_limit");

      if (  (options::indent_namespace_limit() > 0)
         && ((br_close = chunk_skip_to_match(pc)) != nullptr))
      {
         // br_close->orig_line is always >= pc->orig_line;
         size_t numberOfLines = br_close->orig_line - pc->orig_line - 1;                 // Issue #2345
         LOG_FMT(LTOK, "%s(%d): br_close->orig_line is %zu, pc->orig_line is %zu\n",
                 __func__, __LINE__, br_close->orig_line, pc->orig_line);
         LOG_FMT(LTOK, "%s(%d): numberOfLines is %zu, indent_namespace_limit() is %d\n",
                 __func__, __LINE__, numberOfLines, options::indent_namespace_limit());

         log_rule_B("indent_namespace_limit");

         if (numberOfLines > options::indent_namespace_limit())
         {
            LOG_FMT(LTOK, "%s(%d): PCF_LONG_BLOCK is set\n", __func__, __LINE__);
            chunk_flags_set(pc, PCF_LONG_BLOCK);
            chunk_flags_set(br_close, PCF_LONG_BLOCK);
         }
      }
      flag_parens(pc, PCF_IN_NAMESPACE, CT_NONE, CT_NAMESPACE, false);
      return;
   }
} // mark_namespace


static Chunk *insert_vbrace(Chunk *pc, bool after, const ParseFrame &frm)
{
   LOG_FUNC_ENTRY();

   Chunk chunk;

   set_chunk_parent(&chunk, frm.top().type);
   chunk.orig_line   = pc->orig_line;
   chunk.level       = frm.level;
   chunk.pp_level    = frm.pp_level;
   chunk.brace_level = frm.brace_level;
   chunk.flags       = pc->flags & PCF_COPY_FLAGS;
   chunk.str         = "";

   if (after)
   {
      chunk.orig_col = pc->orig_col;
      set_chunk_type(&chunk, CT_VBRACE_CLOSE);
      return(chunk_add_after(&chunk, pc));
   }
   Chunk *ref = pc->GetPrev();

   if (ref->IsNullChunk())
   {
      return(nullptr);
   }

   if (!ref->flags.test(PCF_IN_PREPROC))
   {
      chunk.flags &= ~PCF_IN_PREPROC;
   }
   bool ref_is_comment = ref->IsComment();      // Issue #3351

   while (  chunk_is_newline(ref)
         || ref->IsComment())
   {
      ref->level++;
      ref->brace_level++;
      ref = ref->GetPrev();
   }

   if (ref->IsNullChunk())
   {
      return(nullptr);
   }

   // Don't back into a preprocessor
   if (  !pc->flags.test(PCF_IN_PREPROC)
      && ref->flags.test(PCF_IN_PREPROC))
   {
      if (chunk_is_token(ref, CT_PREPROC_BODY))
      {
         while (  ref->IsNotNullChunk()
               && ref->flags.test(PCF_IN_PREPROC))
         {
            ref = ref->GetPrev();
         }
      }
      else
      {
         ref = ref->GetNext();

         if (chunk_is_token(ref, CT_COMMENT)) // Issue #3034
         {
            ref = ref->GetNextNc();
         }
      }
   }

   if (ref_is_comment)                                      // Issue #3351
   {
      ref = ref->GetNext();
   }

   if (ref->IsNullChunk())
   {
      return(nullptr);
   }
   chunk.orig_line = ref->orig_line;
   chunk.orig_col  = ref->orig_col;
   chunk.column    = ref->column + ref->Len() + 1;
   chunk.pp_level  = ref->pp_level;                         // Issue #3055
   set_chunk_type(&chunk, CT_VBRACE_OPEN);

   return(chunk_add_after(&chunk, ref));
} // insert_vbrace


bool close_statement(ParseFrame &frm, Chunk *pc, const BraceState &braceState)
{
   LOG_FUNC_ENTRY();

   if (pc == nullptr)
   {
      throw invalid_argument(string(__func__) + ":" + to_string(__LINE__)
                             + "args cannot be nullptr");
   }
   LOG_FMT(LTOK, "%s(%d): orig_line is %zu, type is %s, '%s' type is %s, stage is %u\n",
           __func__, __LINE__, pc->orig_line,
           get_token_name(pc->type), pc->Text(),
           get_token_name(frm.top().type),
           (unsigned int)frm.top().stage);

   if (braceState.consumed)
   {
      frm.stmt_count = 0;
      frm.expr_count = 0;
      LOG_FMT(LSTMT, "%s(%d): orig_line is %zu> reset2 stmt on '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->Text());
   }
   /*
    * Insert a CT_VBRACE_CLOSE, if needed:
    * If we are in a virtual brace and we are not ON a CT_VBRACE_CLOSE add one
    */
   Chunk *vbc = pc;

   if (frm.top().type == CT_VBRACE_OPEN)
   {
      // If the current token has already been consumed, then add after it
      if (braceState.consumed)
      {
         insert_vbrace_close_after(pc, frm);
      }
      else
      {
         // otherwise, add before it and consume the vbrace
         vbc = pc->GetPrevNcNnl();

         frm.level--;
         frm.brace_level--;
         vbc = insert_vbrace_close_after(vbc, frm);
         set_chunk_parent(vbc, frm.top().parent);

         LOG_FMT(LBCSPOP, "%s(%d): frm.brace_level decreased to %zu\n",
                 __func__, __LINE__, frm.brace_level);
         log_pcf_flags(LBCSPOP, pc->flags);
         LOG_FMT(LBCSPOP, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
         frm.pop(__func__, __LINE__, pc);

         // Update the token level
         pc->level       = frm.level;
         pc->brace_level = frm.brace_level;

         print_stack(LBCSPOP, "-CS VB  ", frm);

         // And repeat the close
         close_statement(frm, pc, braceState);
         return(true);
      }
   }

   // See if we are done with a complex statement
   if (frm.top().stage != brace_stage_e::NONE)
   {
      if (handle_complex_close(frm, vbc, braceState))
      {
         return(true);
      }
   }
   return(false);
} // close_statement
