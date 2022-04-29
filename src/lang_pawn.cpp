/**
 * @file lang_pawn.cpp
 * Special functions for pawn stuff
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "lang_pawn.h"

#include "prototypes.h"

using namespace uncrustify;


/**
 * Checks to see if a token continues a statement to the next line.
 * We need to check for 'open' braces/paren/etc because the level doesn't
 * change until the token after the open.
 */
static bool pawn_continued(Chunk *pc, size_t br_level);


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
static Chunk *pawn_process_line(Chunk *start);


//! We are on a level 0 function proto of def
static Chunk *pawn_mark_function0(Chunk *start, Chunk *fcn);


/**
 * follows a variable definition at level 0 until the end.
 * Adds a semicolon at the end, if needed.
 */
static Chunk *pawn_process_variable(Chunk *start);


static Chunk *pawn_process_func_def(Chunk *pc);


Chunk *pawn_add_vsemi_after(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   if (  chunk_is_token(pc, CT_VSEMICOLON)
      || chunk_is_token(pc, CT_SEMICOLON))
   {
      return(pc);
   }

   if (pc == nullptr)
   {
      pc = Chunk::NullChunkPtr;
   }
   Chunk *next = pc->GetNextNc();

   if (  next->IsNotNullChunk()
      && (  chunk_is_token(next, CT_VSEMICOLON)
         || chunk_is_token(next, CT_SEMICOLON)))
   {
      return(pc);
   }
   Chunk chunk = *pc;

   set_chunk_type(&chunk, CT_VSEMICOLON);
   set_chunk_parent(&chunk, CT_NONE);
   chunk.str     = options::mod_pawn_semicolon() ? ";" : "";
   chunk.column += pc->Len();

   LOG_FMT(LPVSEMI, "%s: Added VSEMI on line %zu, prev='%s' [%s]\n",
           __func__, pc->orig_line, pc->Text(),
           get_token_name(pc->type));

   return(chunk_add_after(&chunk, pc));
}


void pawn_scrub_vsemi(void)
{
   constexpr static auto LCURRENT = LPVSEMI;

   LOG_FUNC_ENTRY();

   log_rule_B("mod_pawn_semicolon");

   if (!options::mod_pawn_semicolon())
   {
      return;
   }

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (pc->type != CT_VSEMICOLON)
      {
         continue;
      }
      Chunk *prev = pc->GetPrevNcNnl();

      if (chunk_is_token(prev, CT_BRACE_CLOSE))
      {
         if (  get_chunk_parent_type(prev) == CT_IF
            || get_chunk_parent_type(prev) == CT_ELSE
            || get_chunk_parent_type(prev) == CT_SWITCH
            || get_chunk_parent_type(prev) == CT_CASE
            || get_chunk_parent_type(prev) == CT_WHILE_OF_DO)
         {
            pc->str.clear();
         }
      }
   }
}


static bool pawn_continued(Chunk *pc, size_t br_level)
{
   LOG_FUNC_ENTRY();

   if (pc == nullptr)
   {
      return(false);
   }

   if (  pc->level > br_level
      || chunk_is_token(pc, CT_ARITH)
      || chunk_is_token(pc, CT_SHIFT)
      || chunk_is_token(pc, CT_CARET)
      || chunk_is_token(pc, CT_QUESTION)
      || chunk_is_token(pc, CT_BOOL)
      || chunk_is_token(pc, CT_ASSIGN)
      || chunk_is_token(pc, CT_COMMA)
      || chunk_is_token(pc, CT_COMPARE)
      || chunk_is_token(pc, CT_IF)
      || chunk_is_token(pc, CT_ELSE)
      || chunk_is_token(pc, CT_DO)
      || chunk_is_token(pc, CT_SWITCH)
      || chunk_is_token(pc, CT_WHILE)
      || chunk_is_token(pc, CT_BRACE_OPEN)
      || chunk_is_token(pc, CT_VBRACE_OPEN)
      || chunk_is_token(pc, CT_FPAREN_OPEN)
      || get_chunk_parent_type(pc) == CT_IF
      || get_chunk_parent_type(pc) == CT_ELSE
      || get_chunk_parent_type(pc) == CT_ELSEIF
      || get_chunk_parent_type(pc) == CT_DO
      || get_chunk_parent_type(pc) == CT_FOR
      || get_chunk_parent_type(pc) == CT_SWITCH
      || get_chunk_parent_type(pc) == CT_WHILE
      || get_chunk_parent_type(pc) == CT_FUNC_DEF
      || get_chunk_parent_type(pc) == CT_ENUM
      || pc->flags.test_any(PCF_IN_ENUM | PCF_IN_STRUCT)
      || chunk_is_str(pc, ":")
      || chunk_is_str(pc, "+")
      || chunk_is_str(pc, "-"))
   {
      return(true);
   }
   return(false);
} // pawn_continued


void pawn_prescan(void)
{
   LOG_FUNC_ENTRY();

   /*
    * Start at the beginning and step through the entire file, and clean up
    * any questionable stuff
    */
   bool  did_nl = true;
   Chunk *pc    = Chunk::GetHead();

   while (  pc != nullptr
         && pc->IsNotNullChunk())
   {
      if (  did_nl
         && pc->type != CT_PREPROC
         && !chunk_is_newline(pc)
         && pc->level == 0)
      {
         // pc now points to the start of a line
         pc = pawn_process_line(pc);
      }

      // note that continued lines are ignored
      if (  pc != nullptr
         && pc->IsNotNullChunk())
      {
         did_nl = (chunk_is_token(pc, CT_NEWLINE));
      }
      pc = pc->GetNextNc();
   }
}


static Chunk *pawn_process_line(Chunk *start)
{
   LOG_FUNC_ENTRY();

   //LOG_FMT(LSYS, "%s: %d - %s\n", __func__,
   //        start->orig_line, start->Text());

   if (  chunk_is_token(start, CT_NEW)
      || chunk_is_str(start, "const"))
   {
      return(pawn_process_variable(start));
   }
   // if a open paren is found before an assign, then this is a function
   Chunk *fcn = nullptr;

   if (chunk_is_token(start, CT_WORD))
   {
      fcn = start;
   }
   Chunk *pc = Chunk::NullChunkPtr;

   if (start != nullptr)
   {
      pc = start;
   }

   while (  ((pc = pc->GetNextNc())->IsNotNullChunk())
         && !chunk_is_str(pc, "(")
         && pc->type != CT_ASSIGN
         && pc->type != CT_NEWLINE)
   {
      if (  pc->level == 0
         && (  chunk_is_token(pc, CT_FUNCTION)
            || chunk_is_token(pc, CT_WORD)
            || chunk_is_token(pc, CT_OPERATOR_VAL)))
      {
         fcn = pc;
      }
   }

   if (pc->IsNotNullChunk())
   {
      if (chunk_is_token(pc, CT_ASSIGN))
      {
         return(pawn_process_variable(pc));
      }
   }

   if (fcn != nullptr)
   {
      //LOG_FMT(LSYS, "FUNCTION: %s\n", fcn->Text());
      return(pawn_mark_function0(start, fcn));
   }

   if (chunk_is_token(start, CT_ENUM))
   {
      pc = start->GetNextType(CT_BRACE_CLOSE, start->level);
      return(pc);
   }
   //LOG_FMT(LSYS, "%s: Don't understand line %d, starting with '%s' [%s]\n",
   //        __func__, start->orig_line, start->Text(), get_token_name(start->type));
   return(start);
} // pawn_process_line


static Chunk *pawn_process_variable(Chunk *start)
{
   LOG_FUNC_ENTRY();
   Chunk *pc = Chunk::NullChunkPtr;

   if (start != nullptr)
   {
      pc = start;
   }
   Chunk *prev = Chunk::NullChunkPtr;

   while ((pc = pc->GetNextNc())->IsNotNullChunk())
   {
      if (  chunk_is_token(pc, CT_NEWLINE)
         && prev->IsNotNullChunk()
         && !pawn_continued(prev, start->level))
      {
         if (  prev->type != CT_VSEMICOLON
            && prev->type != CT_SEMICOLON)
         {
            pawn_add_vsemi_after(prev);
         }
         break;
      }
      prev = pc;
   }
   return(pc);
}


void pawn_add_virtual_semicolons(void)
{
   LOG_FUNC_ENTRY();

   // Add Pawn virtual semicolons
   if (language_is_set(LANG_PAWN))
   {
      Chunk *prev = Chunk::NullChunkPtr;
      Chunk *pc   = Chunk::GetHead();

      while ((pc = pc->GetNext())->IsNotNullChunk())
      {
         if (  !pc->IsComment()
            && !chunk_is_newline(pc)
            && pc->type != CT_VBRACE_CLOSE
            && pc->type != CT_VBRACE_OPEN)
         {
            prev = pc;
         }

         if (  prev->IsNullChunk()
            || (  pc->type != CT_NEWLINE
               && pc->type != CT_BRACE_CLOSE
               && pc->type != CT_VBRACE_CLOSE))
         {
            continue;
         }

         // we just hit a newline and we have a previous token
         if (  !prev->flags.test(PCF_IN_PREPROC)
            && !prev->flags.test_any(PCF_IN_ENUM | PCF_IN_STRUCT)
            && prev->type != CT_VSEMICOLON
            && prev->type != CT_SEMICOLON
            && !pawn_continued(prev, prev->brace_level))
         {
            pawn_add_vsemi_after(prev);
            prev = Chunk::NullChunkPtr;
         }
      }
   }
} // pawn_add_virtual_semicolons


static Chunk *pawn_mark_function0(Chunk *start, Chunk *fcn)
{
   LOG_FUNC_ENTRY();

   // handle prototypes
   if (start == fcn)
   {
      Chunk *last = fcn->GetNextType(CT_PAREN_CLOSE, fcn->level)->GetNext();

      if (chunk_is_token(last, CT_SEMICOLON))
      {
         LOG_FMT(LPFUNC, "%s: %zu] '%s' proto due to semicolon\n",
                 __func__, fcn->orig_line, fcn->Text());
         set_chunk_type(fcn, CT_FUNC_PROTO);
         return(last);
      }
   }
   else
   {
      if (  chunk_is_token(start, CT_FORWARD)
         || chunk_is_token(start, CT_NATIVE))
      {
         LOG_FMT(LPFUNC, "%s: %zu] '%s' [%s] proto due to %s\n",
                 __func__, fcn->orig_line, fcn->Text(),
                 get_token_name(fcn->type),
                 get_token_name(start->type));
         set_chunk_type(fcn, CT_FUNC_PROTO);
         return(fcn->GetNextNc());
      }
   }
   // Not a prototype, so it must be a function def
   return(pawn_process_func_def(fcn));
}


static Chunk *pawn_process_func_def(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   // We are on a function definition
   set_chunk_type(pc, CT_FUNC_DEF);

   LOG_FMT(LPFUNC, "%s: %zu:%zu %s\n",
           __func__, pc->orig_line, pc->orig_col, pc->Text());

   /*
    * If we don't have a brace open right after the close fparen, then
    * we need to add virtual braces around the function body.
    */
   Chunk *clp  = pc->GetNextString(")", 1, 0);
   Chunk *last = clp->GetNextNcNnl();

   if (last->IsNotNullChunk())
   {
      LOG_FMT(LPFUNC, "%s: %zu] last is '%s' [%s]\n",
              __func__, last->orig_line, last->Text(), get_token_name(last->type));
   }

   // See if there is a state clause after the function
   if (  last->IsNotNullChunk()
      && chunk_is_str(last, "<"))
   {
      LOG_FMT(LPFUNC, "%s: %zu] '%s' has state angle open %s\n",
              __func__, pc->orig_line, pc->Text(), get_token_name(last->type));

      set_chunk_type(last, CT_ANGLE_OPEN);
      set_chunk_parent(last, CT_FUNC_DEF);

      while (  ((last = last->GetNext())->IsNotNullChunk())
            && !chunk_is_str(last, ">"))
      {
         // do nothing just search, TODO: use search_chunk
      }

      if (last->IsNotNullChunk())
      {
         LOG_FMT(LPFUNC, "%s: %zu] '%s' has state angle close %s\n",
                 __func__, pc->orig_line, pc->Text(), get_token_name(last->type));
         set_chunk_type(last, CT_ANGLE_CLOSE);
         set_chunk_parent(last, CT_FUNC_DEF);
      }
      last = last->GetNextNcNnl();
   }

   if (last->IsNullChunk())
   {
      return(last);
   }

   if (chunk_is_token(last, CT_BRACE_OPEN))
   {
      set_chunk_parent(last, CT_FUNC_DEF);
      last = last->GetNextType(CT_BRACE_CLOSE, last->level);

      if (last->IsNotNullChunk())
      {
         set_chunk_parent(last, CT_FUNC_DEF);
      }
   }
   else
   {
      LOG_FMT(LPFUNC, "%s: %zu] '%s' fdef: expected brace open: %s\n",
              __func__, pc->orig_line, pc->Text(), get_token_name(last->type));

      // do not insert a vbrace before a preproc
      if (last->flags.test(PCF_IN_PREPROC))
      {
         return(last);
      }
      Chunk chunk = *last;
      chunk.str.clear();
      set_chunk_type(&chunk, CT_VBRACE_OPEN);
      set_chunk_parent(&chunk, CT_FUNC_DEF);

      Chunk *prev = chunk_add_before(&chunk, last);
      last = prev;

      // find the next newline at level 0
      prev = prev->GetNextNcNnl();

      do
      {
         LOG_FMT(LPFUNC, "%s:%zu] check %s, level %zu\n",
                 __func__, prev->orig_line, get_token_name(prev->type), prev->level);

         if (  chunk_is_token(prev, CT_NEWLINE)
            && prev->level == 0)
         {
            Chunk *next = prev->GetNextNcNnl();

            if (  next->IsNotNullChunk()
               && next->type != CT_ELSE
               && next->type != CT_WHILE_OF_DO)
            {
               break;
            }
         }
         prev->level++;
         prev->brace_level++;
         last = prev;
      } while ((prev = prev->GetNext())->IsNotNullChunk());

      if (  last != nullptr
         && last->IsNotNullChunk())
      {
         LOG_FMT(LPFUNC, "%s:%zu] ended on %s, level %zu\n",
                 __func__, last->orig_line, get_token_name(last->type), last->level);
      }
      chunk = *last;
      chunk.str.clear();
      set_chunk_type(&chunk, CT_VBRACE_CLOSE);
      set_chunk_parent(&chunk, CT_FUNC_DEF);
      chunk.column     += last->Len();
      chunk.level       = 0;
      chunk.brace_level = 0;
      last              = chunk_add_after(&chunk, last);
   }
   return(last);
} // pawn_process_func_def


Chunk *pawn_check_vsemicolon(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   // Grab the open VBrace
   Chunk *vb_open = pc->GetPrevType(CT_VBRACE_OPEN, -1);

   /*
    * Grab the item before the newline
    * Don't do anything if:
    *  - the only thing previous is the V-Brace open
    *  - in a preprocessor
    *  - level > (vb_open->level + 1) -- ie, in () or []
    *  - it is something that needs a continuation
    *    + arith, assign, bool, comma, compare
    */
   Chunk *prev = pc->GetPrevNcNnl();

   if (  prev->IsNullChunk()
      || prev == vb_open
      || prev->flags.test(PCF_IN_PREPROC)
      || pawn_continued(prev, vb_open->level + 1))
   {
      if (prev->IsNotNullChunk())
      {
         LOG_FMT(LPVSEMI, "%s:  no  VSEMI on line %zu, prev='%s' [%s]\n",
                 __func__, prev->orig_line, prev->Text(), get_token_name(prev->type));
      }
      return(pc);
   }
   return(pawn_add_vsemi_after(prev));
}
