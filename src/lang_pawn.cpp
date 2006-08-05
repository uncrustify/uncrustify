/**
 * @file lang_pawn.cpp
 * Special functions for pawn stuff
 *
 * $Id: combine.cpp 391 2006-08-04 02:02:37Z bengardner $
 */
#include "uncrustify_types.h"
#include "chunk_list.h"
#include "ChunkStack.h"
#include "prototypes.h"

static chunk_t *pawn_process_line(chunk_t *start);
static chunk_t *pawn_mark_function0(chunk_t *start, chunk_t *fcn);
static chunk_t *pawn_process_variable(chunk_t *start);
static chunk_t *pawn_process_func_def(chunk_t *pc);

static chunk_t *pawn_add_vsemi_after(chunk_t *pc)
{
   chunk_t chunk;

   chunk = *pc;
   chunk.type  = CT_VSEMICOLON;
   chunk.len   = cpd.settings[UO_mod_pawn_semicolon].b ? 1 : 0;
   chunk.str   = ";";
   chunk.column += pc->len;
   chunk.parent_type = CT_NONE;

   return(chunk_add_after(&chunk, pc));
}


/**
 * Checks to see if a token continues a statement to the next line
 */
bool pawn_continued(chunk_t *pc, int br_level)
{
   if (pc == NULL)
   {
      return false;
   }
   if ((pc->level > br_level) ||
       (pc->type == CT_ARITH) ||
       (pc->type == CT_QUESTION) ||
       (pc->type == CT_BOOL) ||
       (pc->type == CT_ASSIGN) ||
       (pc->type == CT_COMMA) ||
       (pc->type == CT_COMPARE) ||
       (pc->type == CT_IF) ||
       (pc->type == CT_ELSE) ||
       (pc->type == CT_DO) ||
       (pc->type == CT_SWITCH) ||
       (pc->type == CT_WHILE) ||
       (pc->type == CT_BRACE_OPEN) ||
       (pc->type == CT_VBRACE_OPEN) ||
       (pc->type == CT_FPAREN_OPEN) ||
       (pc->parent_type == CT_IF) ||
       (pc->parent_type == CT_ELSE) ||
       (pc->parent_type == CT_DO) ||
       (pc->parent_type == CT_SWITCH) ||
       (pc->parent_type == CT_WHILE) ||
       (pc->parent_type == CT_FUNC_DEF) ||
       (pc->parent_type == CT_ENUM) ||
       ((pc->flags & PCF_IN_ENUM) != 0) ||
       chunk_is_str(pc, ":", 1) ||
       chunk_is_str(pc, "+", 1) ||
       chunk_is_str(pc, "-", 1))
   {
      return true;
   }
   return false;
}


/**
 * Does a scan of level 0 BEFORE stuff in combine.cpp is called.
 * At this point, VSemis have been added only in VBraces.
 * Otherwise, all level info is correct, except for unbraced functions.
 *
 * We are looking for unbraced functions.
 */
void pawn_prescan(void)
{
   /* Start at the beginning and step through the entire file, and clean up
    * any questionable stuff
    */

   chunk_t *pc;
   //chunk_t *prev;
   bool    did_nl = true;

   pc = chunk_get_head();
   while (pc != NULL)
   {
      if (did_nl && (pc->type != CT_PREPROC) &&
          !chunk_is_newline(pc) && (pc->level == 0))
      {
         /* pc now points to the start of a line */
         pc = pawn_process_line(pc);
      }
      /* note that continued lines are ignored */
      if (pc != NULL)
      {
         did_nl = (pc->type == CT_NEWLINE);
      }

      pc = chunk_get_next_nc(pc);
   }
}


/**
 * Functions prototypes and definitions can only appear in level 0.
 *
 * Function prototypes start with "native", "forward", or are just a function
 * with a trailing semicolon instead of a open brace (or something else)
 *
 * somefunc(params)              <-- def
 * stock somefunc(params)        <-- def
 * somefunc(params);             <-- proto
 * forward somefunc(params)      <-- proto
 * native somefunc[rect](params) <-- proto
 *
 * Functions start with 'stock', 'static', 'public', or '@' (on level 0)
 *
 * Variable definitions start with 'stock', 'static', 'new', or 'public'.
 */
static chunk_t *pawn_process_line(chunk_t *start)
{
   chunk_t *pc;
   chunk_t *fcn = NULL;

   //LOG_FMT(LSYS, "%s: %d - %.*s\n", __func__,
   //        start->orig_line, start->len, start->str);

   if ((start->type == CT_NEW) ||
       chunk_is_str(start, "const", 5))
   {
      return(pawn_process_variable(start));
   }

   /* if a open paren is found before an assign, then this is a function */
   if (start->type == CT_WORD)
   {
      fcn = start;
   }
   pc = start;
   while (((pc = chunk_get_next_nc(pc)) != NULL) &&
          !chunk_is_str(pc, "(", 1) &&
          (pc->type != CT_ASSIGN) &&
          (pc->type != CT_NEWLINE))
   {
      if ((pc->level == 0) &&
          ((pc->type == CT_FUNCTION) ||
           (pc->type == CT_WORD)))
      {
         fcn = pc;
      }
   }

   if (pc != NULL)
   {
      if (pc->type == CT_ASSIGN)
      {
         return(pawn_process_variable(pc));
      }
   }

   if (fcn != NULL)
   {
      //LOG_FMT(LSYS, "FUNCTION: %.*s\n", fcn->len, fcn->str);
      return pawn_mark_function0(start, fcn);
   }

   if (start->type == CT_ENUM)
   {
      pc = chunk_get_next_type(start, CT_BRACE_CLOSE, start->level);
      return pc;
   }

   //LOG_FMT(LSYS, "%s: Don't understand line %d, starting with '%.*s' [%s]\n",
   //        __func__, start->orig_line, start->len, start->str, get_token_name(start->type));
   return(start);
}

static chunk_t *pawn_process_variable(chunk_t *start)
{
   //LOG_FMT(LSYS, "%s: %d]", __func__, start->orig_line);

   chunk_t *prev = NULL;
   chunk_t *pc = start;
   while ((pc = chunk_get_next_nc(pc)) != NULL)
   {
      if ((pc->type == CT_NEWLINE) && !pawn_continued(prev, start->level))
      {
         pawn_add_vsemi_after(prev);
         break;
      }

      //if (!chunk_is_newline(pc) && !chunk_is_comment(pc))
      //{
      //   LOG_FMT(LSYS, " [%.*s]", pc->len, pc->str);
      //}

      prev = pc;
   }
   //LOG_FMT(LSYS, "\n");
   return pc;
}

void pawn_add_virtual_semicolons(void)
{
   chunk_t *prev;
   chunk_t *pc;

   /** Add Pawn virtual semicolons */
   prev = NULL;
   if ((cpd.lang_flags & LANG_PAWN) != 0)
   {
      pc = chunk_get_head();
      while ((pc = chunk_get_next(pc)) != NULL)
      {
         if (!chunk_is_comment(pc) &&
             !chunk_is_newline(pc) &&
             (pc->type != CT_VBRACE_CLOSE) &&
             (pc->type != CT_VBRACE_OPEN))
         {
            prev = pc;
         }
         if ((prev == NULL) ||
             ((pc->type != CT_NEWLINE)  &&
              (pc->type != CT_BRACE_CLOSE) &&
              (pc->type != CT_VBRACE_CLOSE)))
         {
            continue;
         }

         /* we just hit a newline and we have a previous token */
         if (((prev->flags & PCF_IN_PREPROC) == 0) &&
             (prev->parent_type != CT_FUNC_DEF) &&
             (prev->parent_type != CT_ENUM) &&
             ((prev->flags & PCF_IN_ENUM) == 0) &&
             (prev->type != CT_VSEMICOLON) &&
             (prev->type != CT_SEMICOLON) &&
             (prev->type != CT_BRACE_CLOSE) &&
             (prev->type != CT_VBRACE_CLOSE) &&
             (prev->type != CT_BRACE_OPEN) &&
             (prev->type != CT_ELSE) &&
             (prev->type != CT_DO) &&
             (prev->type != CT_VBRACE_OPEN) &&
             (prev->type != CT_SPAREN_OPEN) &&
             (prev->type != CT_SPAREN_CLOSE) &&
             (prev->type != CT_FPAREN_OPEN) &&
             (prev->brace_level == prev->level) &&
             (prev->type != CT_ARITH) &&
             (prev->type != CT_ASSIGN) &&
             (prev->type != CT_BOOL) &&
             (prev->type != CT_QUESTION) &&
             !chunk_is_str(prev, "+", 1) &&
             !chunk_is_str(prev, "-", 1) &&
             (prev->type != CT_COMMA) &&
             (prev->type != CT_COLON) &&
             (prev->type != CT_COMPARE))
         {
            pawn_add_vsemi_after(prev);

            LOG_FMT(LPVSEMI, "%s: Added VSEMI on line %d, prev='%.*s' [%s]\n",
                    __func__,
                    prev->orig_line, prev->len, prev->str, get_token_name(prev->type));
            prev = NULL;
         }
      }
   }
}

/**
 * We are on a level 0 function proto of def
 */
static chunk_t *pawn_mark_function0(chunk_t *start, chunk_t *fcn)
{
   chunk_t *last;

   /* handle prototypes */
   if (start == fcn)
   {
      last = chunk_get_next_type(fcn, CT_PAREN_CLOSE, fcn->level);
      last = chunk_get_next(last);
      if ((last != NULL) && (last->type == CT_SEMICOLON))
      {
         LOG_FMT(LPFUNC, "%s: %d] '%.*s' proto due to semicolon\n", __func__,
                 fcn->orig_line, fcn->len, fcn->str);
         fcn->type = CT_FUNC_PROTO;
         return(last);
      }
   }
   else
   {
      if ((start->type == CT_FORWARD) ||
          (start->type == CT_NATIVE))
      {
         LOG_FMT(LPFUNC, "%s: %d] '%.*s' [%s] proto due to %s\n", __func__,
                 fcn->orig_line, fcn->len, fcn->str,
                 get_token_name(fcn->type),
                 get_token_name(start->type));
         fcn->type = CT_FUNC_PROTO;
         return(chunk_get_next_nc(fcn));
      }
   }

   /* Not a prototype, so it must be a function def */
   return pawn_process_func_def(fcn);
}


/**
 * We are on a function word. we need to:
 *  - find out if this is a call or prototype or implementation
 *  - mark return type
 *  - mark parameter types
 *  - mark brace pair
 */
void pawn_mark_function(chunk_t *pc)
{
   //chunk_t *prev;
   //chunk_t *next;
   //chunk_t *last = pc;
   //
   //prev = pc;
   //
   ///* If followed by a colon, this can't be a function (native exception) */
   //next = chunk_get_next_ncnl(pc);
   //if (chunk_is_str(next, ":", 1))
   //{
   //   return;
   //}
   //
   //// find the first token on this line
   //while (((prev = chunk_get_prev(prev)) != NULL) &&
   //       (prev->type != CT_NEWLINE))
   //{
   //   last = prev;
   //}
   //
   //if (last != NULL)
   //{
   //   LOG_FMT(LPFUNC, "%s: %d] first item on the line is '%.*s' [%s]\n", __func__,
   //           last->orig_line, last->len, last->str, get_token_name(last->type));
   //
   //   /* Make sure the previous token is a */
   //}
   //
   ///* If the function name is the first thing on the line, then
   // * we need to check for a semicolon after the close paren
   // */
   //if (last == pc)
   //{
   //   last = chunk_get_next_type(pc, CT_PAREN_CLOSE, pc->level);
   //   last = chunk_get_next(last);
   //   if ((last != NULL) && (last->type == CT_SEMICOLON))
   //   {
   //      LOG_FMT(LPFUNC, "%s: %d] '%.*s' proto due to semicolon\n", __func__,
   //              pc->orig_line, pc->len, pc->str);
   //      pc->type = CT_FUNC_PROTO;
   //      return;
   //   }
   //}
   //else
   //{
   //   if ((last->type == CT_FORWARD) ||
   //       (last->type == CT_NATIVE))
   //   {
   //      LOG_FMT(LPFUNC, "%s: %d] '%.*s' [%s] proto due to %s\n", __func__,
   //              pc->orig_line, pc->len, pc->str, get_token_name(pc->type),
   //              get_token_name(last->type));
   //      pc->type = CT_FUNC_PROTO;
   //      return;
   //   }
   //}
   //
   ///* At this point its either a function definition or a function call
   // * If the brace level is 0, then it is a definition, otherwise its a call.
   // */
   if (pc->brace_level != 0)
   {
      pc->type = CT_FUNC_CALL;
      return;
   }
   //
   //pawn_process_func_def(pc);
}

static chunk_t *pawn_process_func_def(chunk_t *pc)
{
   /* We are on a function definition */
   chunk_t *clp;
   chunk_t *last;
   chunk_t *next;

   pc->type = CT_FUNC_DEF;

   /* If we don't have a brace open right after the close fparen, then
    * we need to add virtual braces around the function body.
    */
   clp = chunk_get_next_str(pc, ")", 1, 0);
   last = chunk_get_next_ncnl(clp);

   if (last != NULL)
   {
      LOG_FMT(LPFUNC, "%s: %d] last is '%.*s' [%s]\n", __func__,
              last->orig_line, last->len, last->str, get_token_name(last->type));
   }

   /* See if there is a state clause after the function */
   if ((last != NULL) && chunk_is_str(last, "<", 1))
   {
      LOG_FMT(LPFUNC, "%s: %d] '%.*s' has state angle open %s\n", __func__,
              pc->orig_line, pc->len, pc->str, get_token_name(last->type));

      last->type        = CT_ANGLE_OPEN;
      last->parent_type = CT_FUNC_DEF;
      while (((last = chunk_get_next(last)) != NULL) &&
             !chunk_is_str(last, ">", 1))
      {
      }

      if (last != NULL)
      {
         LOG_FMT(LPFUNC, "%s: %d] '%.*s' has state angle close %s\n", __func__,
                 pc->orig_line, pc->len, pc->str, get_token_name(last->type));
         last->type        = CT_ANGLE_CLOSE;
         last->parent_type = CT_FUNC_DEF;
      }
      last = chunk_get_next_ncnl(last);
   }

   if (last == NULL)
   {
      return last;
   }
   if (last->type == CT_BRACE_OPEN)
   {
      last = chunk_get_next_type(last, CT_BRACE_CLOSE, last->level);
   }
   else
   {
      LOG_FMT(LPFUNC, "%s: %d] '%.*s' fdef: expected brace open: %s\n", __func__,
              pc->orig_line, pc->len, pc->str, get_token_name(last->type));

      chunk_t chunk;
      chunk         = *last;
      chunk.str     = "{";
      chunk.len     = 0;
      chunk.type    = CT_VBRACE_OPEN;
      chunk.parent_type = CT_FUNC_DEF;

      chunk_t *prev = chunk_add_before(&chunk, last);
      last = prev;

      /* find the next newline at level 0 */
      prev = chunk_get_next_ncnl(prev);
      do
      {
         LOG_FMT(LPFUNC, "%s:%d] check %s, level %d\n", __func__,
                 prev->orig_line, get_token_name(prev->type), prev->level);
         if ((prev->type == CT_NEWLINE) &&
             (prev->level == 0))
         {
            next = chunk_get_next_ncnl(prev);
            if ((next != NULL) &&
                (next->type != CT_ELSE) &&
                (next->type != CT_WHILE_OF_DO))
            {
               break;
            }
         }
         prev->level++;
         prev->brace_level++;
         last = prev;
      } while ((prev = chunk_get_next(prev)) != NULL);

      if (last != NULL)
      {
         LOG_FMT(LPFUNC, "%s:%d] ended on %s, level %d\n", __func__,
                 last->orig_line, get_token_name(last->type), last->level);
      }

      chunk         = *last;
      chunk.str     = "}";
      chunk.len     = 0;
      chunk.column += last->len;
      chunk.type    = CT_VBRACE_CLOSE;
      chunk.level   = 0;
      chunk.brace_level = 0;
      chunk.parent_type = CT_FUNC_DEF;
      last = chunk_add_after(&chunk, last);
   }
   return last;
}

