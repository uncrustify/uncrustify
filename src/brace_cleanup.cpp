/**
 * @file brace_cleanup.cpp
 * Determines the brace level and paren level.
 * Inserts virtual braces as needed.
 * Handles all that preprocessor crap.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "uncrustify_types.h"
#include "char_table.h"
#include "prototypes.h"
#include "chunk_list.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include "unc_ctype.h"


static chunk_t *insert_vbrace(chunk_t *pc, bool after,
                              struct parse_frame *frm);

#define insert_vbrace_close_after(pc, frm)    insert_vbrace(pc, true, frm)
#define insert_vbrace_open_before(pc, frm)    insert_vbrace(pc, false, frm)

static void parse_cleanup(struct parse_frame *frm, chunk_t *pc);

static bool close_statement(struct parse_frame *frm, chunk_t *pc);

static bool check_complex_statements(struct parse_frame *frm, chunk_t *pc);
static bool handle_complex_close(struct parse_frame *frm, chunk_t *pc);


static int preproc_start(struct parse_frame *frm, chunk_t *pc)
{
   chunk_t *next;
   int     pp_level = cpd.pp_level;

   /* Get the type of preprocessor and handle it */
   next = chunk_get_next_ncnl(pc);
   if (next != NULL)
   {
      cpd.in_preproc = next->type;

      /**
       * If we are in a define, push the frame stack.
       */
      if (cpd.in_preproc == CT_PP_DEFINE)
      {
         pf_push(frm);

         /* a preproc body starts a new, blank frame */
         memset(frm, 0, sizeof(*frm));
         frm->level       = 1;
         frm->brace_level = 1;

         /*TODO: not sure about the next 3 lines */
         frm->pse_tos = 1;
         frm->pse[frm->pse_tos].type  = CT_PP_DEFINE;
         frm->pse[frm->pse_tos].stage = BS_NONE;
      }
      else
      {
         /* Check for #if, #else, #endif, etc */
         pp_level = pf_check(frm, pc);
      }
   }
   return(pp_level);
}


static void print_stack(log_sev_t logsev, const char *str,
                        struct parse_frame *frm, chunk_t *pc)
{
   if (log_sev_on(logsev))
   {
      int idx;

      log_fmt(logsev, "%8.8s", str);

      for (idx = 1; idx <= frm->pse_tos; idx++)
      {
         if (frm->pse[idx].stage != BS_NONE)
         {
            LOG_FMT(logsev, " [%s - %d]", get_token_name(frm->pse[idx].type),
                    frm->pse[idx].stage);
         }
         else
         {
            LOG_FMT(logsev, " [%s]", get_token_name(frm->pse[idx].type));
         }
      }
      log_fmt(logsev, "\n");
   }
}


/**
 * Scans through the whole list and does stuff.
 * It has to do some tricks to parse preprocessors.
 *
 * TODO: This can be cleaned up and simplified - we can look both forward and backward!
 */
void brace_cleanup(void)
{
   chunk_t            *pc;
   struct parse_frame frm;
   int                pp_level;

   memset(&frm, 0, sizeof(frm));

   cpd.frame_count = 0;
   cpd.in_preproc  = CT_NONE;
   cpd.pp_level    = 0;

   pc = chunk_get_head();
   while (pc != NULL)
   {
      /* Check for leaving a #define body */
      if ((cpd.in_preproc != CT_NONE) && ((pc->flags & PCF_IN_PREPROC) == 0))
      {
         if (cpd.in_preproc == CT_PP_DEFINE)
         {
            /* out of the #define body, restore the frame */
            pf_pop(&frm);
         }

         cpd.in_preproc = CT_NONE;
      }

      /* Check for a preprocessor start */
      pp_level = cpd.pp_level;
      if (pc->type == CT_PREPROC)
      {
         pp_level = preproc_start(&frm, pc);
      }

      /* Do before assigning stuff from the frame */
      if ((cpd.lang_flags & LANG_PAWN) != 0)
      {
         if ((frm.pse[frm.pse_tos].type == CT_VBRACE_OPEN) &&
             (pc->type == CT_NEWLINE))
         {
            pc = pawn_check_vsemicolon(pc);
         }
      }

      /* Assume the level won't change */
      pc->level       = frm.level;
      pc->brace_level = frm.brace_level;
      pc->pp_level    = pp_level;


      /**
       * #define bodies get the full formatting treatment
       * Also need to pass in the initial '#' to close out any virtual braces.
       */
      if (!chunk_is_comment(pc) && !chunk_is_newline(pc) &&
          ((cpd.in_preproc == CT_PP_DEFINE) ||
           (cpd.in_preproc == CT_NONE)))
      {
         cpd.consumed = false;
         parse_cleanup(&frm, pc);
         print_stack(LBCSAFTER, (pc->type == CT_VBRACE_CLOSE) ? "Virt-}" : pc->str, &frm, pc);
      }
      pc = chunk_get_next(pc);
   }
}


/**
 * pc is a CT_WHILE.
 * Scan backwards to see if we find a brace/vbrace with the parent set to CT_DO
 */
static bool maybe_while_of_do(chunk_t *pc)
{
   chunk_t *prev;

   prev = chunk_get_prev_ncnl(pc);
   if ((prev == NULL) || !(prev->flags & PCF_IN_PREPROC))
   {
      return(false);
   }

   /* Find the chunk before the preprocessor */
   while ((prev != NULL) && (prev->flags & PCF_IN_PREPROC))
   {
      prev = chunk_get_prev_ncnl(prev);
   }

   if ((prev != NULL) &&
       (prev->parent_type == CT_DO) &&
       ((prev->type == CT_VBRACE_CLOSE) ||
        (prev->type == CT_BRACE_CLOSE)))
   {
      return(true);
   }
   return(false);
}


static void push_fmr_pse(struct parse_frame *frm, chunk_t *pc,
                         brstage_e stage, const char *logtext)
{
   if (frm->pse_tos < ((int)ARRAY_SIZE(frm->pse) - 1))
   {
      frm->pse_tos++;
      frm->pse[frm->pse_tos].type  = pc->type;
      frm->pse[frm->pse_tos].stage = stage;
      frm->pse[frm->pse_tos].pc    = pc;

      print_stack(LBCSPUSH, logtext, frm, pc);
   }
   else
   {
      LOG_FMT(LWARN, "%s:%d Error: Frame stack overflow,  Unable to properly process this file.\n",
              cpd.filename, cpd.line_number);
      cpd.error_count++;
   }
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
static void parse_cleanup(struct parse_frame *frm, chunk_t *pc)
{
   c_token_t parent = CT_NONE;
   chunk_t   *prev;

   LOG_FMT(LTOK, "%s:%d] %16s - tos:%d/%16s stg:%d\n",
           __func__, pc->orig_line, get_token_name(pc->type),
           frm->pse_tos, get_token_name(frm->pse[frm->pse_tos].type),
           frm->pse[frm->pse_tos].stage);

   /* Mark statement starts */
   if (((frm->stmt_count == 0) || (frm->expr_count == 0)) &&
       !chunk_is_semicolon(pc) &&
       (pc->type != CT_BRACE_CLOSE) &&
       (pc->type != CT_VBRACE_CLOSE) &&
       !chunk_is_str(pc, ")", 1) &&
       !chunk_is_str(pc, "]", 1))
   {
      pc->flags |= PCF_EXPR_START;
      pc->flags |= (frm->stmt_count == 0) ? PCF_STMT_START : 0;
      LOG_FMT(LSTMT, "%d] 1.marked %s as %s start st:%d ex:%d\n",
              pc->orig_line, pc->str.c_str(), (pc->flags &PCF_STMT_START) ? "stmt" : "expr",
              frm->stmt_count, frm->expr_count);
   }
   frm->stmt_count++;
   frm->expr_count++;

   if (frm->sparen_count > 0)
   {
      int tmp;

      pc->flags |= PCF_IN_SPAREN;

      /* Mark everything in the a for statement */
      for (tmp = frm->pse_tos - 1; tmp >= 0; tmp--)
      {
         if (frm->pse[tmp].type == CT_FOR)
         {
            pc->flags |= PCF_IN_FOR;
            break;
         }
      }

      /* Mark the parent on semicolons in for() stmts */
      if ((pc->type == CT_SEMICOLON) &&
          (frm->pse_tos > 1) &&
          (frm->pse[frm->pse_tos - 1].type == CT_FOR))
      {
         pc->parent_type = CT_FOR;
      }
   }

   /* Check the progression of complex statements */
   if (frm->pse[frm->pse_tos].stage != BS_NONE)
   {
      if (check_complex_statements(frm, pc))
      {
         return;
      }
   }

   /**
    * Check for a virtual brace statement close due to a semicolon.
    * The virtual brace will get handled the next time through.
    * The semicolon isn't handled at all.
    * TODO: may need to float VBRACE past comments until newline?
    */
   if (frm->pse[frm->pse_tos].type == CT_VBRACE_OPEN)
   {
      if (chunk_is_semicolon(pc))
      {
         cpd.consumed = true;
         close_statement(frm, pc);
      }
      else if ((cpd.lang_flags & LANG_PAWN) != 0)
      {
         if (pc->type == CT_BRACE_CLOSE)
         {
            close_statement(frm, pc);
         }
      }
   }

   /* Handle close paren, vbrace, brace, and square */
   if ((pc->type == CT_PAREN_CLOSE) ||
       (pc->type == CT_BRACE_CLOSE) ||
       (pc->type == CT_VBRACE_CLOSE) ||
       (pc->type == CT_ANGLE_CLOSE) ||
       (pc->type == CT_MACRO_CLOSE) ||
       (pc->type == CT_SQUARE_CLOSE))
   {
      /* Change CT_PAREN_CLOSE into CT_SPAREN_CLOSE or CT_FPAREN_CLOSE */
      if ((pc->type == CT_PAREN_CLOSE) &&
          ((frm->pse[frm->pse_tos].type == CT_FPAREN_OPEN) ||
           (frm->pse[frm->pse_tos].type == CT_SPAREN_OPEN)))
      {
         pc->type = (c_token_t)(frm->pse[frm->pse_tos].type + 1);
         if (pc->type == CT_SPAREN_CLOSE)
         {
            frm->sparen_count--;
            pc->flags &= ~PCF_IN_SPAREN;
         }
      }

      /* Make sure the open / close match */
      if (pc->type != (frm->pse[frm->pse_tos].type + 1))
      {
         if ((frm->pse[frm->pse_tos].type != CT_NONE) &&
             (frm->pse[frm->pse_tos].type != CT_PP_DEFINE))
         {
            LOG_FMT(LWARN, "%s:%d Error: Unexpected '%s' for '%s', which was on line %d\n",
                    cpd.filename, pc->orig_line, pc->str.c_str(),
                    get_token_name(frm->pse[frm->pse_tos].pc->type),
                    frm->pse[frm->pse_tos].pc->orig_line);
            print_stack(LBCSPOP, "=Error  ", frm, pc);
            cpd.error_count++;
         }
      }
      else
      {
         cpd.consumed = true;

         /* Copy the parent, update the paren/brace levels */
         pc->parent_type = frm->pse[frm->pse_tos].parent;
         frm->level--;
         if ((pc->type == CT_BRACE_CLOSE) ||
             (pc->type == CT_VBRACE_CLOSE) ||
             (pc->type == CT_MACRO_CLOSE))
         {
            frm->brace_level--;
         }
         pc->level       = frm->level;
         pc->brace_level = frm->brace_level;

         /* Pop the entry */
         frm->pse_tos--;
         print_stack(LBCSPOP, "-Close  ", frm, pc);

         /* See if we are in a complex statement */
         if (frm->pse[frm->pse_tos].stage != BS_NONE)
         {
            handle_complex_close(frm, pc);
         }
      }
   }

   /* In this state, we expect a semicolon, but we'll also hit the closing
    * sparen, so we need to check cpd.consumed to see if the close sparen was
    * aleady handled.
    */
   if (frm->pse[frm->pse_tos].stage == BS_WOD_SEMI)
   {
      chunk_t *tmp = pc;

      if (cpd.consumed)
      {
         /* If consumed, then we are on the close sparen.
          * PAWN: Check the next chunk for a semicolon. If it isn't, then
          * add a virtual semicolon, which will get handled on the next pass.
          */
         if (cpd.lang_flags & LANG_PAWN)
         {
            tmp = chunk_get_next_ncnl(pc);

            if ((tmp->type != CT_SEMICOLON) && (tmp->type != CT_VSEMICOLON))
            {
               pawn_add_vsemi_after(pc);
            }
         }
      }
      else
      {
         /* Complain if this ISN'T a semicolon, but close out WHILE_OF_DO anyway */
         if ((pc->type == CT_SEMICOLON) || (pc->type == CT_VSEMICOLON))
         {
            cpd.consumed    = true;
            pc->parent_type = CT_WHILE_OF_DO;
         }
         else
         {
            LOG_FMT(LWARN, "%s:%d: Error: Expected a semicolon for WHILE_OF_DO, but got '%s'\n",
                    cpd.filename, pc->orig_line, get_token_name(pc->type));
            cpd.error_count++;
         }
         handle_complex_close(frm, pc);
      }
   }

   /* Get the parent type for brace and paren open */
   parent = pc->parent_type;
   if ((pc->type == CT_PAREN_OPEN) ||
       (pc->type == CT_FPAREN_OPEN) ||
       (pc->type == CT_SPAREN_OPEN) ||
       (pc->type == CT_BRACE_OPEN))
   {
      prev = chunk_get_prev_ncnl(pc);
      if (prev != NULL)
      {
         if ((pc->type == CT_PAREN_OPEN) ||
             (pc->type == CT_FPAREN_OPEN) ||
             (pc->type == CT_SPAREN_OPEN))
         {
            /* Set the parent for parens and change paren type */
            if (frm->pse[frm->pse_tos].stage != BS_NONE)
            {
               pc->type = CT_SPAREN_OPEN;
               parent   = frm->pse[frm->pse_tos].type;
               frm->sparen_count++;
            }
            else if (prev->type == CT_FUNCTION)
            {
               pc->type = CT_FPAREN_OPEN;
               parent   = CT_FUNCTION;
            }
            else
            {
               /* no need to set parent */
            }
         }
         else  /* must be CT_BRACE_OPEN */
         {
            /* Set the parent for open braces */
            if (frm->pse[frm->pse_tos].stage != BS_NONE)
            {
               parent = frm->pse[frm->pse_tos].type;
            }
            else if ((prev->type == CT_ASSIGN) && (prev->str[0] == '='))
            {
               parent = CT_ASSIGN;
            }
            else if (prev->type == CT_FPAREN_CLOSE)
            {
               parent = CT_FUNCTION;
            }
            else
            {
               /* no need to set parent */
            }
         }
      }
   }

   /**
    * Adjust the level for opens & create a stack entry
    * Note that CT_VBRACE_OPEN has already been handled.
    */
   if ((pc->type == CT_BRACE_OPEN) ||
       (pc->type == CT_PAREN_OPEN) ||
       (pc->type == CT_FPAREN_OPEN) ||
       (pc->type == CT_SPAREN_OPEN) ||
       (pc->type == CT_ANGLE_OPEN) ||
       (pc->type == CT_MACRO_OPEN) ||
       (pc->type == CT_SQUARE_OPEN))
   {
      frm->level++;
      if ((pc->type == CT_BRACE_OPEN) ||
          (pc->type == CT_MACRO_OPEN))
      {
         frm->brace_level++;
      }
      push_fmr_pse(frm, pc, BS_NONE, "+Open   ");
      frm->pse[frm->pse_tos].parent = parent;
      pc->parent_type = parent;
   }

   pattern_class patcls = get_token_pattern_class(pc->type);

   /** Create a stack entry for complex statements IF/DO/FOR/WHILE/SWITCH */
   if (patcls == PATCLS_BRACED)
   {
      push_fmr_pse(frm, pc,
                   (pc->type == CT_DO) ? BS_BRACE_DO : BS_BRACE2,
                   "+ComplexBraced");
   }
   else if (patcls == PATCLS_PBRACED)
   {
      brstage_e bs = BS_PAREN1;

      if ((pc->type == CT_WHILE) && maybe_while_of_do(pc))
      {
         pc->type = CT_WHILE_OF_DO;
         bs       = BS_WOD_PAREN;
      }
      push_fmr_pse(frm, pc, bs, "+ComplexParenBraced");
   }
   else if (patcls == PATCLS_OPBRACED)
   {
      push_fmr_pse(frm, pc, BS_OP_PAREN1, "+ComplexOpParenBraced");
   }
   else if (patcls == PATCLS_ELSE)
   {
      push_fmr_pse(frm, pc, BS_ELSEIF, "+ComplexElse");
   }

   /* Mark simple statement/expression starts
    *  - after { or }
    *  - after ';', but not if the paren stack top is a paren
    *  - after '(' that has a parent type of CT_FOR
    */
   if ((pc->type == CT_SQUARE_OPEN) ||
       ((pc->type == CT_BRACE_OPEN) && (pc->parent_type != CT_ASSIGN)) ||
       (pc->type == CT_BRACE_CLOSE) ||
       (pc->type == CT_VBRACE_CLOSE) ||
       ((pc->type == CT_SPAREN_OPEN) && (pc->parent_type == CT_FOR)) ||
       (chunk_is_semicolon(pc) &&
        (frm->pse[frm->pse_tos].type != CT_PAREN_OPEN) &&
        (frm->pse[frm->pse_tos].type != CT_FPAREN_OPEN) &&
        (frm->pse[frm->pse_tos].type != CT_SPAREN_OPEN)))
   {
      LOG_FMT(LSTMT, "%s: %d> reset1 stmt on %s\n",
              __func__, pc->orig_line, pc->str.c_str());
      frm->stmt_count = 0;
      frm->expr_count = 0;
   }

   /* Mark expression starts */
   chunk_t *tmp = chunk_get_next_ncnl(pc);
   if ((pc->type == CT_ARITH) ||
       (pc->type == CT_ASSIGN) ||
       (pc->type == CT_CASE) ||
       (pc->type == CT_COMPARE) ||
       ((pc->type == CT_STAR) && tmp && (tmp->type != CT_STAR)) ||
       (pc->type == CT_BOOL) ||
       (pc->type == CT_MINUS) ||
       (pc->type == CT_PLUS) ||
       (pc->type == CT_ANGLE_OPEN) ||
       (pc->type == CT_ANGLE_CLOSE) ||
       (pc->type == CT_RETURN) ||
       (pc->type == CT_GOTO) ||
       (pc->type == CT_CONTINUE) ||
       (pc->type == CT_PAREN_OPEN) ||
       (pc->type == CT_FPAREN_OPEN) ||
       (pc->type == CT_SPAREN_OPEN) ||
       (pc->type == CT_BRACE_OPEN) ||
       chunk_is_semicolon(pc) ||
       (pc->type == CT_COMMA) ||
       (pc->type == CT_NOT) ||
       (pc->type == CT_INV) ||
       (pc->type == CT_COLON) ||
       (pc->type == CT_QUESTION))
   {
      frm->expr_count = 0;
      LOG_FMT(LSTMT, "%s: %d> reset expr on %s\n",
              __func__, pc->orig_line, pc->str.c_str());
   }
}


/**
 * Checks the progression of complex statements.
 * - checks for else after if
 * - checks for if after else
 * - checks for while after do
 * - checks for open brace in BRACE2 and BRACE_DO stages, inserts open VBRACE
 * - checks for open paren in PAREN1 and PAREN2 stages, complains
 *
 * @param frm  The parse frame
 * @param pc   The current chunk
 * @return     true - done with this chunk, false - keep processing
 */
static bool check_complex_statements(struct parse_frame *frm, chunk_t *pc)
{
   c_token_t parent;
   chunk_t   *vbrace;

   /* Turn an optional paren into either a real paren or a brace */
   if (frm->pse[frm->pse_tos].stage == BS_OP_PAREN1)
   {
      frm->pse[frm->pse_tos].stage = (pc->type != CT_PAREN_OPEN) ? BS_BRACE2 : BS_PAREN1;
   }

   /* Check for CT_ELSE after CT_IF */
   while (frm->pse[frm->pse_tos].stage == BS_ELSE)
   {
      if (pc->type == CT_ELSE)
      {
         /* Replace CT_IF with CT_ELSE on the stack & we are done */
         frm->pse[frm->pse_tos].type  = CT_ELSE;
         frm->pse[frm->pse_tos].stage = BS_ELSEIF;
         print_stack(LBCSSWAP, "=Swap   ", frm, pc);
         return(true);
      }

      /* Remove the CT_IF and close the statement */
      frm->pse_tos--;
      print_stack(LBCSPOP, "-IF-CCS ", frm, pc);
      if (close_statement(frm, pc))
      {
         return(true);
      }
   }

   /* Check for CT_IF after CT_ELSE */
   if (frm->pse[frm->pse_tos].stage == BS_ELSEIF)
   {
      if (pc->type == CT_IF)
      {
         if (!cpd.settings[UO_indent_else_if].b ||
             !chunk_is_newline(chunk_get_prev_nc(pc)))
         {
            /* Replace CT_ELSE with CT_IF */
            pc->type = CT_ELSEIF;
            frm->pse[frm->pse_tos].type  = CT_ELSEIF;
            frm->pse[frm->pse_tos].stage = BS_PAREN1;
            return(true);
         }
      }

      /* Jump to the 'expecting brace' stage */
      frm->pse[frm->pse_tos].stage = BS_BRACE2;
   }

   /* Check for CT_CATCH or CT_FINALLY after CT_TRY or CT_CATCH */
   while (frm->pse[frm->pse_tos].stage == BS_CATCH)
   {
      if ((pc->type == CT_CATCH) || (pc->type == CT_FINALLY))
      {
         /* Replace CT_TRY with CT_CATCH on the stack & we are done */
         frm->pse[frm->pse_tos].type  = pc->type;
         frm->pse[frm->pse_tos].stage = (pc->type == CT_CATCH) ? BS_OP_PAREN1 : BS_BRACE2;
         print_stack(LBCSSWAP, "=Swap   ", frm, pc);
         return(true);
      }

      /* Remove the CT_TRY and close the statement */
      frm->pse_tos--;
      print_stack(LBCSPOP, "-TRY-CCS ", frm, pc);
      if (close_statement(frm, pc))
      {
         return(true);
      }
   }

   /* Check for CT_WHILE after the CT_DO */
   if (frm->pse[frm->pse_tos].stage == BS_WHILE)
   {
      if (pc->type == CT_WHILE)
      {
         pc->type = CT_WHILE_OF_DO;
         frm->pse[frm->pse_tos].type  = CT_WHILE_OF_DO; //CT_WHILE;
         frm->pse[frm->pse_tos].stage = BS_WOD_PAREN;
         return(true);
      }

      LOG_FMT(LWARN, "%s:%d Error: Expected 'while', got '%s'\n",
              cpd.filename, pc->orig_line, pc->str.c_str());
      frm->pse_tos--;
      print_stack(LBCSPOP, "-Error  ", frm, pc);
      cpd.error_count++;
   }

   /* Insert a CT_VBRACE_OPEN, if needed */
   if ((pc->type != CT_BRACE_OPEN) &&
       ((frm->pse[frm->pse_tos].stage == BS_BRACE2) ||
        (frm->pse[frm->pse_tos].stage == BS_BRACE_DO)))
   {
      parent = frm->pse[frm->pse_tos].type;

      vbrace = insert_vbrace_open_before(pc, frm);
      vbrace->parent_type = parent;

      frm->level++;
      frm->brace_level++;

      push_fmr_pse(frm, vbrace, BS_NONE, "+VBrace ");
      frm->pse[frm->pse_tos].parent = parent;

      /* update the level of pc */
      pc->level       = frm->level;
      pc->brace_level = frm->brace_level;

      /* Mark as a start of a statement */
      frm->stmt_count = 0;
      frm->expr_count = 0;
      pc->flags      |= PCF_STMT_START | PCF_EXPR_START;
      frm->stmt_count = 1;
      frm->expr_count = 1;
      LOG_FMT(LSTMT, "%d] 2.marked %s as stmt start\n", pc->orig_line, pc->str.c_str());
   }

   /* Verify open paren in complex statement */
   if ((pc->type != CT_PAREN_OPEN) &&
       ((frm->pse[frm->pse_tos].stage == BS_PAREN1) ||
        (frm->pse[frm->pse_tos].stage == BS_WOD_PAREN)))
   {
      LOG_FMT(LWARN, "%s:%d Error: Expected '(', got '%s' for '%s'\n",
              cpd.filename, pc->orig_line, pc->str.c_str(),
              get_token_name(frm->pse[frm->pse_tos].type));

      /* Throw out the complex statement */
      frm->pse_tos--;
      print_stack(LBCSPOP, "-Error  ", frm, pc);
      cpd.error_count++;
   }

   return(false);
}


/**
 * Handles a close paren or brace - just progress the stage, if the end
 * of the statement is hit, call close_statement()
 *
 * @param frm  The parse frame
 * @param pc   The current chunk
 * @return     true - done with this chunk, false - keep processing
 */
static bool handle_complex_close(struct parse_frame *frm, chunk_t *pc)
{
   chunk_t *next;

   if (frm->pse[frm->pse_tos].stage == BS_PAREN1)
   {
      /* PAREN1 always => BRACE2 */
      frm->pse[frm->pse_tos].stage = BS_BRACE2;
   }
   else if (frm->pse[frm->pse_tos].stage == BS_BRACE2)
   {
      /* BRACE2: IF => ELSE, anyting else => close */
      if ((frm->pse[frm->pse_tos].type == CT_IF) ||
          (frm->pse[frm->pse_tos].type == CT_ELSEIF))
      {
         frm->pse[frm->pse_tos].stage = BS_ELSE;

         /* If the next chunk isn't CT_ELSE, close the statement */
         next = chunk_get_next_ncnl(pc);
         if ((next != NULL) && (next->type != CT_ELSE))
         {
            frm->pse_tos--;
            print_stack(LBCSPOP, "-IF-HCS ", frm, pc);
            if (close_statement(frm, pc))
            {
               return(true);
            }
         }
      }
      else if ((frm->pse[frm->pse_tos].type == CT_TRY) ||
               (frm->pse[frm->pse_tos].type == CT_CATCH))
      {
         frm->pse[frm->pse_tos].stage = BS_CATCH;

         /* If the next chunk isn't CT_CATCH or CT_FINALLY, close the statement */
         next = chunk_get_next_ncnl(pc);
         if ((next != NULL) &&
             (next->type != CT_CATCH) &&
             (next->type != CT_FINALLY))
         {
            frm->pse_tos--;
            print_stack(LBCSPOP, "-TRY-HCS ", frm, pc);
            if (close_statement(frm, pc))
            {
               return(true);
            }
         }
      }
      else
      {
         LOG_FMT(LNOTE, "%s: close_statement on %s BS_BRACE2\n", __func__,
                 get_token_name(frm->pse[frm->pse_tos].type));
         frm->pse_tos--;
         print_stack(LBCSPOP, "-HCC B2 ", frm, pc);
         if (close_statement(frm, pc))
         {
            return(true);
         }
      }
   }
   else if (frm->pse[frm->pse_tos].stage == BS_BRACE_DO)
   {
      frm->pse[frm->pse_tos].stage = BS_WHILE;
   }
   else if (frm->pse[frm->pse_tos].stage == BS_WOD_PAREN)
   {
      LOG_FMT(LNOTE, "%s: close_statement on %s BS_WOD_PAREN\n", __func__,
              get_token_name(frm->pse[frm->pse_tos].type));
      frm->pse[frm->pse_tos].stage = BS_WOD_SEMI;
      print_stack(LBCSPOP, "-HCC WoDP ", frm, pc);
   }
   else if (frm->pse[frm->pse_tos].stage == BS_WOD_SEMI)
   {
      LOG_FMT(LNOTE, "%s: close_statement on %s BS_WOD_SEMI\n", __func__,
              get_token_name(frm->pse[frm->pse_tos].type));
      frm->pse_tos--;
      print_stack(LBCSPOP, "-HCC WoDS ", frm, pc);

      if (close_statement(frm, pc))
      {
         return(true);
      }
   }
   else
   {
      /* PROBLEM */
      LOG_FMT(LWARN, "%s:%d Error: TOS.type='%s' TOS.stage=%d\n",
              cpd.filename, pc->orig_line,
              get_token_name(frm->pse[frm->pse_tos].type),
              frm->pse[frm->pse_tos].stage);
      cpd.error_count++;
   }
   return(false);
}


static chunk_t *insert_vbrace(chunk_t *pc, bool after,
                              struct parse_frame *frm)
{
   chunk_t chunk;
   chunk_t *rv;
   chunk_t *ref;

   chunk.orig_line   = pc->orig_line;
   chunk.parent_type = frm->pse[frm->pse_tos].type;
   chunk.level       = frm->level;
   chunk.brace_level = frm->brace_level;
   chunk.flags       = pc->flags & PCF_COPY_FLAGS;
   chunk.str         = "";
   if (after)
   {
      chunk.type = CT_VBRACE_CLOSE;
      rv         = chunk_add_after(&chunk, pc);
   }
   else
   {
      ref = chunk_get_prev(pc);
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

      /* Don't back into a preprocessor */
      if (((pc->flags & PCF_IN_PREPROC) == 0) &&
          ((ref->flags & PCF_IN_PREPROC) != 0))
      {
         if (ref->type == CT_PREPROC_BODY)
         {
            do
            {
               ref = chunk_get_prev(ref);
            } while ((ref != NULL) && ((ref->flags & PCF_IN_PREPROC) != 0));
         }
         else
         {
            ref = chunk_get_next(ref);
         }
      }

      chunk.orig_line = ref->orig_line;
      chunk.column    = ref->column + ref->len() + 1;
      chunk.type      = CT_VBRACE_OPEN;
      rv = chunk_add_after(&chunk, ref);
   }
   return(rv);
}


/**
 * Called when a statement was just closed and the pse_tos was just
 * decremented.
 *
 * - if the TOS is now VBRACE, insert a CT_VBRACE_CLOSE and recurse.
 * - if the TOS is a complex statement, call handle_complex_close()
 *
 * @return     true - done with this chunk, false - keep processing
 */
bool close_statement(struct parse_frame *frm, chunk_t *pc)
{
   chunk_t *vbc = pc;

   LOG_FMT(LTOK, "%s:%d] %s '%s' type %s stage %d\n", __func__,
           pc->orig_line,
           get_token_name(pc->type), pc->str.c_str(),
           get_token_name(frm->pse[frm->pse_tos].type),
           frm->pse[frm->pse_tos].stage);

   if (cpd.consumed)
   {
      frm->stmt_count = 0;
      frm->expr_count = 0;
      LOG_FMT(LSTMT, "%s: %d> reset2 stmt on %s\n",
              __func__, pc->orig_line, pc->str.c_str());
   }

   /**
    * If we are in a virtual brace and we are not ON a CT_VBRACE_CLOSE add one
    */
   if (frm->pse[frm->pse_tos].type == CT_VBRACE_OPEN)
   {
      /* If the current token has already been consumed, then add after it */
      if (cpd.consumed)
      {
         insert_vbrace_close_after(pc, frm);
      }
      else
      {
         /* otherwise, add before it and consume the vbrace */
         vbc = chunk_get_prev_ncnl(pc);
         vbc = insert_vbrace_close_after(vbc, frm);
         vbc->parent_type = frm->pse[frm->pse_tos].parent;

         frm->level--;
         frm->brace_level--;
         frm->pse_tos--;

         /* Update the token level */
         pc->level       = frm->level;
         pc->brace_level = frm->brace_level;

         print_stack(LBCSPOP, "-CS VB  ", frm, pc);

         /* And repeat the close */
         close_statement(frm, pc);
         return(true);
      }
   }

   /* See if we are done with a complex statement */
   if (frm->pse[frm->pse_tos].stage != BS_NONE)
   {
      if (handle_complex_close(frm, vbc))
      {
         return(true);
      }
   }
   return(false);
}
