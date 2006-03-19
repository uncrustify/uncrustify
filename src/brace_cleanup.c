/**
 * @file brace_cleanup.c
 * Determines the brace level and paren level.
 * Inserts virtual braces as needed.
 * Handles all that preprocessor crap.
 *
 * $Id: tokenize.c 74 2006-03-18 05:26:34Z bengardner $
 */

#include "cparse_types.h"
#include "char_table.h"
#include "prototypes.h"
#include "chunk_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>


static chunk_t *insert_vbrace(chunk_t *pc, BOOL after,
                              struct parse_frame *frm);
#define insert_vbrace_after(pc, frm)      insert_vbrace(pc, TRUE, frm)
#define insert_vbrace_before(pc, frm)     insert_vbrace(pc, FALSE, frm)

void parse_cleanup(struct parse_frame *frm, chunk_t *pc);

void close_statement(struct parse_frame *frm, chunk_t *pc);
void handle_close_stage(struct parse_frame *frm, chunk_t *pc);


/**
 * Scans through the whole list and does stuff.
 * It has to do some tricks to parse preprocessors.
 */
void brace_cleanup(void)
{
   chunk_t            *pc;
   chunk_t            *next;
   chunk_t            *prev  = NULL;
   struct parse_frame frm;


   memset(&frm, 0, sizeof(frm));

   cpd.in_preproc = CT_NONE;

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next(pc))
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
      if (pc->type == CT_PREPROC)
      {
         /* Close any virtual braces - they can't cross preprocessors */
         prev = chunk_get_prev_ncnl(pc);
         if (prev != NULL)
         {
            if ((frm.pse[frm.pse_tos].type == CT_VBRACE_OPEN) ||
                (frm.pse[frm.pse_tos].type == CT_IF) ||
                (frm.pse[frm.pse_tos].type == CT_FOR) ||
                (frm.pse[frm.pse_tos].type == CT_SWITCH) ||
                (frm.pse[frm.pse_tos].type == CT_DO) ||
                (frm.pse[frm.pse_tos].type == CT_WHILE) ||
                (frm.pse[frm.pse_tos].type == CT_VOLATILE) ||
                (frm.pse[frm.pse_tos].type == CT_BRACED))
            {
               //fprintf(stderr, "%s: closing on token %s\n",
               //        __func__, get_token_name(pc->type));
               close_statement(&frm, prev);
            }
         }

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
               pf_push(&frm);

               /* a preproc body starts a new, blank frame */
               memset(&frm, 0, sizeof(frm));
               frm.level       = 1;
               frm.brace_level = 1;

               /*TODO: not sure about the next 3 lines */
               frm.pse_tos = 1;
               frm.pse[frm.pse_tos].type  = CT_PP_DEFINE;
               frm.pse[frm.pse_tos].stage = BS_NONE;
            }
            else
            {
               /* Check for #if, #else, #endif, etc */
               pf_check(&frm, next);
            }
         }
      }

      /* Assume the level won't change */
      pc->level       = frm.level;
      pc->brace_level = frm.brace_level;

      /* Special handling for preprocessor stuff */
      if (cpd.in_preproc != CT_NONE)
      {
         /* #define bodies get the full formatting treatment */
         if ((cpd.in_preproc == CT_PP_DEFINE) &&
             !chunk_is_comment(pc) && !chunk_is_newline(pc))
         {
            parse_cleanup(&frm, pc);
         }
      }
      else
      {
         if (!chunk_is_newline(pc) && !chunk_is_comment(pc))
         {
            parse_cleanup(&frm, pc);
         }
      }
   }
}


static void print_stack(struct parse_frame *frm, chunk_t *pc)
{
   if (log_sev_on(LFRMSTK))
   {
      int idx;

      log_fmt(LFRMSTK, "%2d> %2d", pc->orig_line, frm->pse_tos);

      for (idx = 1; idx <= frm->pse_tos; idx++)
      {
         log_fmt(LFRMSTK, " [%s/%d]",
                 get_token_name(frm->pse[idx].type), frm->pse[idx].stage);
      }
      log_fmt(LFRMSTK, "\n");
   }
}

void parse_cleanup(struct parse_frame *frm, chunk_t *pc)
{
   c_token_t parent = CT_NONE;
   chunk_t *prev;

   prev = chunk_get_prev_ncnl(pc);

   if ((prev != NULL) && (prev->type == CT_VERSION) &&
       (frm->pse[frm->pse_tos].type == CT_VERSION))
   {
      if (pc->type == CT_PAREN_OPEN)
      {
         frm->pse[frm->pse_tos].type = CT_IF;
      }
      else
      {
         frm->pse_tos--;
      }
   }

   /* Mark statement starts */
   if (((frm->stmt_count == 0) || (frm->expr_count == 0)) &&
       (pc->type != CT_SEMICOLON) &&
       (pc->type != CT_BRACE_CLOSE))
   {
      pc->flags |= PCF_EXPR_START;
      pc->flags |= (frm->stmt_count == 0) ? PCF_STMT_START : 0;
      LOG_FMT(LPCU, "%d] 1.marked %s as stmt start st:%d ex:%d\n",
              pc->orig_line, pc->str, frm->stmt_count, frm->expr_count);
   }
   frm->stmt_count++;
   frm->expr_count++;

   if (frm->sparen_count > 0)
   {
      pc->flags |= PCF_IN_SPAREN;
   }

   LOG_FMT(LTOK, "%s:%d] %16s - tos:%d/%16s stg:%d\n",
           __func__, pc->orig_line, get_token_name(pc->type),
           frm->pse_tos, get_token_name(frm->pse[frm->pse_tos].type),
           frm->pse[frm->pse_tos].stage);

   /* Check for an else after after the close of an if */
   while (frm->pse[frm->pse_tos].stage == BS_ELSE)
   {
      //fprintf(stderr, "Checking for else\n");
      if (pc->type == CT_ELSE)
      {
         //fprintf(stderr, "%2d> found else for if\n", pc->orig_line);
         frm->pse[frm->pse_tos].type  = CT_ELSE;
         frm->pse[frm->pse_tos].stage = BS_ELSEIF;
         print_stack(frm, pc);
         return;
      }
      else
      {
         //fprintf(stderr, "%2d> did not find else for if\n", pc->orig_line);
         /* the previous chunk ended the statment */
         close_statement(frm, prev);
      }
   }

   /* change CT_ELSE to CT_IF when we hit an "else if" */
   if ((frm->pse[frm->pse_tos].type == CT_ELSE) &&
       (frm->pse[frm->pse_tos].stage == BS_ELSEIF))
   {
      if (pc->type == CT_IF)
      {
         //fprintf(stderr, " -- Changed else to if --\n");
         frm->pse[frm->pse_tos].type  = CT_IF;
         frm->pse[frm->pse_tos].stage = BS_PAREN1;
         return;
      }
      frm->pse[frm->pse_tos].stage = BS_BRACE2;
   }

   if (frm->pse[frm->pse_tos].stage == BS_WHILE)
   {
      if (pc->type == CT_WHILE)
      {
         pc->type                     = CT_WHILE_OF_DO;
         frm->pse[frm->pse_tos].stage = BS_PAREN2;
         return;
      }
      else
      {
         LOG_FMT(LWARN, "%s: Error: Expected 'while', got '%s'\n",
                 __func__, pc->str);
         frm->pse_tos--;
      }
   }

   /* Insert an opening virtual brace */
   if (((frm->pse[frm->pse_tos].stage == BS_BRACE_DO) ||
        (frm->pse[frm->pse_tos].stage == BS_BRACE2)) &&
       (pc->type != CT_BRACE_OPEN))
   {
      parent = frm->pse[frm->pse_tos].type;
      insert_vbrace_before(pc, frm);
      frm->level++;
      frm->brace_level++;
      frm->pse_tos++;
      frm->pse[frm->pse_tos].type   = CT_VBRACE_OPEN;
      frm->pse[frm->pse_tos].stage  = BS_NONE;
      frm->pse[frm->pse_tos].parent = parent;

      print_stack(frm, pc);

      /* update the level of pc */
      pc->level       = frm->level;
      pc->brace_level = frm->brace_level;

      /* Mark as a start of a statment */
      frm->stmt_count = 0;
      frm->expr_count = 0;
      pc->flags      |= PCF_STMT_START | PCF_EXPR_START;
      frm->stmt_count = 1;
      frm->expr_count = 1;
      //fprintf(stderr, "%d] 2.marked %s as stmt start\n", pc->orig_line, pc->str);
   }

   /* Handle an end-of-statement */
   if (pc->type == CT_SEMICOLON)
   {
      close_statement(frm, pc);
   }

   if (prev != NULL)
   {
      /* Detect simple cases of CT_STAR -> CT_PTR_TYPE
       * Change "TYPE *", "QUAL *" and "TYPE **" into
       */
      if ((pc->type == CT_STAR) &&
          ((prev->type == CT_TYPE) ||
           (prev->type == CT_QUALIFIER) ||
           (prev->type == CT_PTR_TYPE)))
      {
         pc->type = CT_PTR_TYPE;
      }

      /* Set the parent of a brace when preceeded by a '=' */
      if ((prev->type == CT_ASSIGN) && (prev->str[0] == '=') &&
          (pc->type == CT_BRACE_OPEN))
      {
         parent = CT_ASSIGN;
      }

      /* Set parent type for parens and change paren type */
      if (pc->type == CT_PAREN_OPEN)
      {
         if (prev->type == CT_WORD)
         {
            prev->type = CT_FUNCTION;
            pc->type   = CT_FPAREN_OPEN;
            parent     = CT_FUNCTION;
         }
         else if ((prev->type == CT_IF) ||
                  (prev->type == CT_FOR) ||
                  (prev->type == CT_WHILE) ||
                  (prev->type == CT_WHILE_OF_DO) ||
                  (prev->type == CT_SWITCH))
         {
            pc->type = CT_SPAREN_OPEN;
            parent   = prev->type;
            frm->sparen_count++;
         }
         else
         {
            /* no need to change the parens */
         }
      }

      /* Set the parent for open braces */
      if (pc->type == CT_BRACE_OPEN)
      {
         if (prev->type == CT_FPAREN_CLOSE)
         {
            parent = CT_FUNCTION;
         }
         else if (prev->type == CT_SPAREN_CLOSE)
         {
            parent = prev->parent_type;
         }
         else if ((prev->type == CT_ELSE) ||
                  (prev->type == CT_DO) ||
                  (prev->type == CT_VOLATILE) ||
                  (prev->type == CT_BRACED))
         {
            parent = prev->type;
         }
      }

      /* Change a WORD after ENUM/UNION/STRUCT to TYPE
       * Also change the first word in 'WORD WORD' to a type.
       */
      if (pc->type == CT_WORD)
      {
         if ((prev->type == CT_ENUM) ||
             (prev->type == CT_UNION) ||
             (prev->type == CT_STRUCT))
         {
            pc->type = CT_TYPE;
         }
         if (prev->type == CT_WORD)
         {
            prev->type = CT_TYPE;
         }
      }

      /* restart the current IF sequence if we hit an "else if" */
      if ((pc->type == CT_IF) && (prev->type == CT_ELSE))
      {
         frm->pse[frm->pse_tos].type  = CT_IF;
         frm->pse[frm->pse_tos].stage = 0;
      }
   }

   /* If we close a paren, change the type to match the open */
   if (pc->type == CT_PAREN_CLOSE)
   {
      if ((frm->pse[frm->pse_tos].type == CT_PAREN_OPEN) ||
          (frm->pse[frm->pse_tos].type == CT_FPAREN_OPEN) ||
          (frm->pse[frm->pse_tos].type == CT_SPAREN_OPEN))
      {
         pc->type = frm->pse[frm->pse_tos].type + 1;
         if (pc->type == CT_SPAREN_CLOSE)
         {
            frm->sparen_count--;
            pc->flags &= ~PCF_IN_SPAREN;
         }
      }
   }

   /* For closing braces/parens/squares, set the parent and handle the close.
    * Adjust the level.
    */
   if ((pc->type == CT_PAREN_CLOSE) ||
       (pc->type == CT_FPAREN_CLOSE) ||
       (pc->type == CT_SPAREN_CLOSE) ||
       (pc->type == CT_SQUARE_CLOSE) ||
       (pc->type == CT_BRACE_CLOSE))
   {
      if (pc->type == (frm->pse[frm->pse_tos].type + 1))
      {
         //fprintf(stderr, "%2d> closed on %s\n", pc->orig_line, get_token_name(pc->type));

         pc->parent_type = frm->pse[frm->pse_tos].parent;
         frm->level--;
         frm->pse_tos--;
         if (pc->type == CT_BRACE_CLOSE)
         {
            frm->brace_level--;
         }

         /* Update the close paren/brace level */
         pc->level       = frm->level;
         pc->brace_level = frm->brace_level;

         print_stack(frm, pc);

         handle_close_stage(frm, pc);
      }
      else
      {
         LOG_FMT(LWARN, "%s: Error: Unexpected '%s' on line %d- %s\n",
                 __func__, pc->str, pc->orig_line,
                 get_token_name(frm->pse[frm->pse_tos].type));
      }
   }

   /* Adjust the level for opens & create a stack entry */
   if ((pc->type == CT_BRACE_OPEN) ||
       (pc->type == CT_PAREN_OPEN) ||
       (pc->type == CT_FPAREN_OPEN) ||
       (pc->type == CT_SPAREN_OPEN) ||
       (pc->type == CT_SQUARE_OPEN))
   {
      frm->level++;
      if (pc->type == CT_BRACE_OPEN)
      {
         frm->brace_level++;
      }
      frm->pse_tos++;
      frm->pse[frm->pse_tos].type   = pc->type;
      frm->pse[frm->pse_tos].stage  = 0;
      frm->pse[frm->pse_tos].parent = parent;
      pc->parent_type               = parent;

      print_stack(frm, pc);
   }

   /* Create a stack entry for complex statments IF/DO/FOR/WHILE/SWITCH */
   if ((pc->type == CT_IF) ||
       (pc->type == CT_DO) ||
       (pc->type == CT_FOR) ||
       (pc->type == CT_WHILE) ||
       (pc->type == CT_VOLATILE) ||
       (pc->type == CT_SWITCH) ||
       (pc->type == CT_VERSION) ||
       (pc->type == CT_BRACED))
   {
      frm->pse_tos++;
      frm->pse[frm->pse_tos].type  = pc->type;
      frm->pse[frm->pse_tos].stage = (pc->type == CT_DO) ? BS_BRACE_DO :
                                     ((pc->type == CT_VOLATILE) ||
                                      (pc->type == CT_BRACED)) ? BS_BRACE2 : BS_PAREN1;
      //fprintf(stderr, "opening %s\n", pc->str);

      print_stack(frm, pc);
   }

   /* Mark simple statement/expression starts
    *  - after { or }
    *  - after ';', but not if the paren stack top is a paren
    *  - after '(' that has a parent type of CT_FOR
    */
   if (((pc->type == CT_BRACE_OPEN) && (pc->parent_type != CT_ASSIGN)) ||
       (pc->type == CT_BRACE_CLOSE) ||
       ((pc->type == CT_SPAREN_OPEN) && (pc->parent_type == CT_FOR)) ||
       ((pc->type == CT_SEMICOLON) &&
        (frm->pse[frm->pse_tos].type != CT_PAREN_OPEN) &&
        (frm->pse[frm->pse_tos].type != CT_FPAREN_OPEN) &&
        (frm->pse[frm->pse_tos].type != CT_SPAREN_OPEN)))
   {
      //fprintf(stderr, "%s: %d> reset stmt on %s\n", __func__, pc->orig_line, pc->str);
      frm->stmt_count = 0;
      frm->expr_count = 0;
   }

   /* Mark expression starts */
   if ((pc->type == CT_ARITH) ||
       (pc->type == CT_ASSIGN) ||
       (pc->type == CT_COMPARE) ||
       (pc->type == CT_ANGLE_OPEN) ||
       (pc->type == CT_ANGLE_CLOSE) ||
       (pc->type == CT_RETURN) ||
       (pc->type == CT_GOTO) ||
       (pc->type == CT_CONTINUE) ||
       (pc->type == CT_PAREN_OPEN) ||
       (pc->type == CT_FPAREN_OPEN) ||
       (pc->type == CT_SPAREN_OPEN) ||
       (pc->type == CT_BRACE_OPEN) ||
       (pc->type == CT_SEMICOLON) ||
       (pc->type == CT_COMMA) ||
       (pc->type == CT_COLON) ||
       (pc->type == CT_QUESTION))
   {
      frm->expr_count = 0;
      //fprintf(stderr, "%s: %d> reset expr on %s\n", __func__, pc->orig_line, pc->str);
   }
}


static chunk_t *insert_vbrace(chunk_t *pc, BOOL after,
                              struct parse_frame *frm)
{
   chunk_t chunk;
   chunk_t *rv;
   chunk_t *ref;

   memset(&chunk, 0, sizeof(chunk));

   //   fprintf(stderr, "%s_%s: %s on line %d\n",
   //           __func__, after ? "after" : "before",
   //           get_token_name(pc->type), pc->orig_line);

   chunk.len         = 0;
   chunk.orig_line   = pc->orig_line;
   chunk.parent_type = frm->pse[frm->pse_tos].type;
   chunk.level       = frm->level;
   chunk.brace_level = frm->brace_level;
   chunk.flags       = pc->flags & PCF_COPY_FLAGS;
   if (after)
   {
      chunk.type = CT_VBRACE_CLOSE;
      rv         = chunk_add_after(&chunk, pc);
   }
   else
   {
      ref = chunk_get_prev(pc);
      while (chunk_is_newline(ref) || chunk_is_comment(ref))
      {
         ref = chunk_get_prev(ref);
      }
      chunk.orig_line = ref->orig_line;
      chunk.column    = ref->column + ref->len + 1;
      chunk.type      = CT_VBRACE_OPEN;
      rv              = chunk_add_after(&chunk, ref);
   }
   return(rv);
}

/*TODO */

/**
 * Called on the last chunk in a statement.
 *
 * This should be called on:
 *  - semicolons
 *  - CT_BRACE_CLOSE '}'
 *  - CT_VBRACE_CLOSE
 *
 * The action taken depends on top item on the stack.
 *  - if (tos.type == CT_IF) and (tos.stage == )
 */
void close_statement(struct parse_frame *frm, chunk_t *pc)
{
   chunk_t *vbc = pc;

   LOG_FMT(LTOK, "%s:%d] %s'%s' type %s stage %d\n", __func__,
           pc->orig_line,
           get_token_name(pc->type), pc->str,
           get_token_name(frm->pse[frm->pse_tos].type),
           frm->pse[frm->pse_tos].stage);

   if (pc->type != CT_VBRACE_CLOSE)
   {
      frm->expr_count = 1;
      if (frm->pse[frm->pse_tos].type != CT_SPAREN_OPEN)
      {
         frm->stmt_count = 1;
      }
   }

   /* See if we are done with a complex statement */
   if ((frm->pse[frm->pse_tos].stage == BS_PAREN2) ||
       (frm->pse[frm->pse_tos].stage == BS_BRACE2) ||
       (frm->pse[frm->pse_tos].stage == BS_ELSE))
   {
      frm->pse_tos--;

      //fprintf(stderr, "%2d> CS1: closed on %s\n", pc->orig_line, get_token_name(pc->type));

      print_stack(frm, pc);

      handle_close_stage(frm, vbc);
   }

   /* If we are in a virtual brace -- close it */
   if (frm->pse[frm->pse_tos].type == CT_VBRACE_OPEN)
   {
      frm->level--;
      frm->brace_level--;
      frm->pse_tos--;

      //fprintf(stderr, "%2d> CS1: closed on %s\n", pc->orig_line, get_token_name(pc->type));

      print_stack(frm, pc);

      vbc             = insert_vbrace_after(pc, frm);
      frm->stmt_count = 1;
      frm->expr_count = 1;
      handle_close_stage(frm, vbc);
   }
}

void handle_close_stage(struct parse_frame *frm, chunk_t *pc)
{
   LOG_FMT(LTOK, "%s-top: line %d pse_tos=%12s stage=%d pc=%s\n",
           __func__, pc->orig_line,
           get_token_name(frm->pse[frm->pse_tos].type),
           frm->pse[frm->pse_tos].stage,
           get_token_name(pc->type));

   /* see if we just closed a do/if/else/for/switch/while section */
   switch (frm->pse[frm->pse_tos].stage)
   {
   case BS_PAREN1:   /* if/for/switch/while () ended */
      frm->pse[frm->pse_tos].stage = BS_BRACE2;
      break;

   case BS_PAREN2:   /* do/while () ended */
      close_statement(frm, pc);
      break;

   case BS_BRACE_DO:   /* do {} ended */
      frm->pse[frm->pse_tos].stage = BS_WHILE;
      break;

   case BS_BRACE2:   /* if/else/for/while/switch {} ended */
      if (frm->pse[frm->pse_tos].type == CT_IF)
      {
         frm->pse[frm->pse_tos].stage = BS_ELSE;
      }
      else
      {
         close_statement(frm, pc);
      }
      break;

   case BS_ELSE:     /* else {} ended */
   case BS_WHILE:    /* do/while () ended */
      LOG_FMT(LWARN, "Unexpect stage %d on line %d\n",
              frm->pse[frm->pse_tos].stage, pc->orig_line);
      break;

   case BS_NONE:
   default:
      /* nothing to do */
      break;
   }

   //   fprintf(stderr, "%s-end: line %d pse_tos=%12s stage=%d\n",
   //        __func__,  pc->orig_line,
   //        get_token_name(frm->pse[frm->pse_tos].type),
   //        frm->pse[frm->pse_tos].stage);
}

