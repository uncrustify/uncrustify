/**
 * @file lang_pawn.cpp
 * Special functions for pawn stuff
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "lang_pawn.h"

#include "prototypes.h"


constexpr static auto LCURRENT = LPVSEMI;


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

   if (pc->IsSemicolon())
   {
      return(pc);
   }
   Chunk *next = pc->GetNextNc();

   if (  next->IsNotNullChunk()
      && next->IsSemicolon())
   {
      return(pc);
   }
   Chunk chunk = *pc;

   chunk.SetType(CT_VSEMICOLON);
   chunk.SetParentType(CT_NONE);
   chunk.Str() = options::mod_pawn_semicolon() ? ";" : "";
   chunk.SetColumn(pc->GetColumn() + pc->Len());

   LOG_FMT(LPVSEMI, "%s: Added VSEMI on line %zu, prev='%s' [%s]\n",
           __func__, pc->GetOrigLine(), pc->Text(),
           get_token_name(pc->GetType()));

   return(chunk.CopyAndAddAfter(pc));
}


void pawn_scrub_vsemi()
{
   LOG_FUNC_ENTRY();

   log_rule_B("mod_pawn_semicolon");

   if (!options::mod_pawn_semicolon())
   {
      return;
   }

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (pc->IsNot(CT_VSEMICOLON))
      {
         continue;
      }
      Chunk *prev = pc->GetPrevNcNnl();

      if (prev->Is(CT_BRACE_CLOSE))
      {
         if (  prev->GetParentType() == CT_IF
            || prev->GetParentType() == CT_ELSE
            || prev->GetParentType() == CT_SWITCH
            || prev->GetParentType() == CT_CASE
            || prev->GetParentType() == CT_WHILE_OF_DO)
         {
            pc->Str().clear();
         }
      }
   }
}


static bool pawn_continued(Chunk *pc, size_t br_level)
{
   LOG_FUNC_ENTRY();

   if (pc->IsNullChunk())
   {
      return(false);
   }

   if (  pc->GetLevel() > br_level
      || pc->Is(CT_ARITH)
      || pc->Is(CT_SHIFT)
      || pc->Is(CT_CARET)
      || pc->Is(CT_QUESTION)
      || pc->Is(CT_BOOL)
      || pc->Is(CT_ASSIGN)
      || pc->Is(CT_COMMA)
      || pc->Is(CT_COMPARE)
      || pc->Is(CT_IF)
      || pc->Is(CT_ELSE)
      || pc->Is(CT_DO)
      || pc->Is(CT_SWITCH)
      || pc->Is(CT_WHILE)
      || pc->Is(CT_BRACE_OPEN)
      || pc->Is(CT_VBRACE_OPEN)
      || pc->Is(CT_FPAREN_OPEN)
      || pc->GetParentType() == CT_IF
      || pc->GetParentType() == CT_ELSE
      || pc->GetParentType() == CT_ELSEIF
      || pc->GetParentType() == CT_DO
      || pc->GetParentType() == CT_FOR
      || pc->GetParentType() == CT_SWITCH
      || pc->GetParentType() == CT_WHILE
      || pc->GetParentType() == CT_FUNC_DEF
      || pc->GetParentType() == CT_ENUM
      || pc->GetFlags().test_any(PCF_IN_ENUM | PCF_IN_STRUCT)
      || pc->IsString(":")
      || pc->IsString("+")
      || pc->IsString("-"))
   {
      return(true);
   }
   return(false);
} // pawn_continued


void pawn_prescan()
{
   LOG_FUNC_ENTRY();

   /*
    * Start at the beginning and step through the entire file, and clean up
    * any questionable stuff
    */
   bool  did_nl = true;
   Chunk *pc    = Chunk::GetHead();

   while (pc->IsNotNullChunk())
   {
      if (  did_nl
         && pc->IsNot(CT_PREPROC)
         && !pc->IsNewline()
         && pc->GetLevel() == 0)
      {
         // pc now points to the start of a line
         pc = pawn_process_line(pc);
      }

      // note that continued lines are ignored
      if (pc->IsNotNullChunk())
      {
         did_nl = (pc->Is(CT_NEWLINE));
      }
      pc = pc->GetNextNc();
   }
}


static Chunk *pawn_process_line(Chunk *start)
{
   LOG_FUNC_ENTRY();

   //LOG_FMT(LSYS, "%s: %d - %s\n", __func__,
   //        start->GetOrigLine(), start->Text());

   if (  start->Is(CT_NEW)
      || start->IsString("const"))
   {
      return(pawn_process_variable(start));
   }
   // if a open paren is found before an assign, then this is a function
   Chunk *fcn = Chunk::NullChunkPtr;

   if (start->Is(CT_WORD))
   {
      fcn = start;
   }
   Chunk *pc = start;

   while (  ((pc = pc->GetNextNc())->IsNotNullChunk())
         && !pc->IsString("(")
         && pc->IsNot(CT_ASSIGN)
         && pc->IsNot(CT_NEWLINE))
   {
      if (  pc->GetLevel() == 0
         && (  pc->Is(CT_FUNCTION)
            || pc->Is(CT_WORD)
            || pc->Is(CT_OPERATOR_VAL)))
      {
         fcn = pc;
      }
   }

   if (pc->IsNotNullChunk())
   {
      if (pc->Is(CT_ASSIGN))
      {
         return(pawn_process_variable(pc));
      }
   }

   if (fcn->IsNotNullChunk())
   {
      //LOG_FMT(LSYS, "FUNCTION: %s\n", fcn->Text());
      return(pawn_mark_function0(start, fcn));
   }

   if (start->Is(CT_ENUM))
   {
      pc = start->GetNextType(CT_BRACE_CLOSE, start->GetLevel());
      return(pc);
   }
   //LOG_FMT(LSYS, "%s: Don't understand line %d, starting with '%s' [%s]\n",
   //        __func__, start->GetOrigLine(), start->Text(), get_token_name(start->GetType()));
   return(start);
} // pawn_process_line


static Chunk *pawn_process_variable(Chunk *start)
{
   LOG_FUNC_ENTRY();
   Chunk *pc = Chunk::NullChunkPtr;

   if (start->IsNotNullChunk())
   {
      pc = start;
   }
   Chunk *prev = Chunk::NullChunkPtr;

   while ((pc = pc->GetNextNc())->IsNotNullChunk())
   {
      if (  pc->Is(CT_NEWLINE)
         && prev->IsNotNullChunk()
         && !pawn_continued(prev, start->GetLevel()))
      {
         if (!prev->IsSemicolon())
         {
            pawn_add_vsemi_after(prev);
         }
         break;
      }
      prev = pc;
   }
   return(pc);
}


void pawn_add_virtual_semicolons()
{
   LOG_FUNC_ENTRY();

   // Add Pawn virtual semicolons
   if (language_is_set(lang_flag_e::LANG_PAWN))
   {
      Chunk *prev = Chunk::NullChunkPtr;
      Chunk *pc   = Chunk::GetHead();

      while ((pc = pc->GetNext())->IsNotNullChunk())
      {
         if (  !pc->IsCommentOrNewline()
            && !pc->IsVBrace())
         {
            prev = pc;
         }

         if (  prev->IsNullChunk()
            || (  pc->IsNot(CT_NEWLINE)
               && !pc->IsBraceClose()))
         {
            continue;
         }

         // we just hit a newline and we have a previous token
         if (  !prev->TestFlags(PCF_IN_PREPROC)
            && !prev->GetFlags().test_any(PCF_IN_ENUM | PCF_IN_STRUCT)
            && !prev->IsSemicolon()
            && !pawn_continued(prev, prev->GetBraceLevel()))
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
      Chunk *last = fcn->GetNextType(CT_PAREN_CLOSE, fcn->GetLevel())->GetNext();

      if (last->Is(CT_SEMICOLON))
      {
         LOG_FMT(LPFUNC, "%s: %zu] '%s' proto due to semicolon\n",
                 __func__, fcn->GetOrigLine(), fcn->Text());
         fcn->SetType(CT_FUNC_PROTO);
         return(last);
      }
   }
   else
   {
      if (  start->Is(CT_FORWARD)
         || start->Is(CT_NATIVE))
      {
         LOG_FMT(LPFUNC, "%s: %zu] '%s' [%s] proto due to %s\n",
                 __func__, fcn->GetOrigLine(), fcn->Text(),
                 get_token_name(fcn->GetType()),
                 get_token_name(start->GetType()));
         fcn->SetType(CT_FUNC_PROTO);
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
   pc->SetType(CT_FUNC_DEF);

   LOG_FMT(LPFUNC, "%s: %zu:%zu %s\n",
           __func__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text());

   /*
    * If we don't have a brace open right after the close fparen, then
    * we need to add virtual braces around the function body.
    */
   Chunk *clp  = pc->GetNextString(")", 1, 0);
   Chunk *last = clp->GetNextNcNnl();

   if (last->IsNotNullChunk())
   {
      LOG_FMT(LPFUNC, "%s: %zu] last is '%s' [%s]\n",
              __func__, last->GetOrigLine(), last->Text(), get_token_name(last->GetType()));
   }

   // See if there is a state clause after the function
   if (  last->IsNotNullChunk()
      && last->IsString("<"))
   {
      LOG_FMT(LPFUNC, "%s: %zu] '%s' has state angle open %s\n",
              __func__, pc->GetOrigLine(), pc->Text(), get_token_name(last->GetType()));

      last->SetType(CT_ANGLE_OPEN);
      last->SetParentType(CT_FUNC_DEF);

      while (  ((last = last->GetNext())->IsNotNullChunk())
            && !last->IsString(">"))
      {
         // do nothing just search, TODO: use search_chunk
      }

      if (last->IsNotNullChunk())
      {
         LOG_FMT(LPFUNC, "%s: %zu] '%s' has state angle close %s\n",
                 __func__, pc->GetOrigLine(), pc->Text(), get_token_name(last->GetType()));
         last->SetType(CT_ANGLE_CLOSE);
         last->SetParentType(CT_FUNC_DEF);
      }
      last = last->GetNextNcNnl();
   }

   if (last->IsNullChunk())
   {
      return(last);
   }

   if (last->Is(CT_BRACE_OPEN))
   {
      last->SetParentType(CT_FUNC_DEF);
      last = last->GetNextType(CT_BRACE_CLOSE, last->GetLevel());

      if (last->IsNotNullChunk())
      {
         last->SetParentType(CT_FUNC_DEF);
      }
   }
   else
   {
      LOG_FMT(LPFUNC, "%s: %zu] '%s' fdef: expected brace open: %s\n",
              __func__, pc->GetOrigLine(), pc->Text(), get_token_name(last->GetType()));

      // do not insert a vbrace before a preproc
      if (last->TestFlags(PCF_IN_PREPROC))
      {
         return(last);
      }
      Chunk chunk = *last;
      chunk.Str().clear();
      chunk.SetType(CT_VBRACE_OPEN);
      chunk.SetParentType(CT_FUNC_DEF);

      Chunk *prev = chunk.CopyAndAddBefore(last);
      last = prev;

      // find the next newline at level 0
      prev = prev->GetNextNcNnl();

      do
      {
         LOG_FMT(LPFUNC, "%s:%zu] check %s, level %zu\n",
                 __func__, prev->GetOrigLine(), get_token_name(prev->GetType()), prev->GetLevel());

         if (  prev->Is(CT_NEWLINE)
            && prev->GetLevel() == 0)
         {
            Chunk *next = prev->GetNextNcNnl();

            if (  next->IsNotNullChunk()
               && next->IsNot(CT_ELSE)
               && next->IsNot(CT_WHILE_OF_DO))
            {
               break;
            }
         }
         prev->SetLevel(prev->GetLevel() + 1);
         prev->SetBraceLevel(prev->GetBraceLevel() + 1);
         last = prev;
      } while ((prev = prev->GetNext())->IsNotNullChunk());

      if (last->IsNotNullChunk())
      {
         LOG_FMT(LPFUNC, "%s:%zu] ended on %s, level %zu\n",
                 __func__, last->GetOrigLine(), get_token_name(last->GetType()), last->GetLevel());
      }
      chunk = *last;
      chunk.Str().clear();
      chunk.SetType(CT_VBRACE_CLOSE);
      chunk.SetParentType(CT_FUNC_DEF);
      chunk.SetColumn(chunk.GetColumn() + last->Len());
      chunk.SetLevel(0);
      chunk.SetBraceLevel(0);
      last = chunk.CopyAndAddAfter(last);
   }
   return(last);
} // pawn_process_func_def


Chunk *pawn_check_vsemicolon(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   // Grab the open VBrace
   Chunk *vb_open = pc->GetPrevType(CT_VBRACE_OPEN);

   /*
    * Grab the item before the newline
    * Don't do anything if:
    *  - the only thing previous is the V-Brace open
    *  - in a preprocessor
    *  - level > (vb_open->GetLevel() + 1) -- ie, in () or []
    *  - it is something that needs a continuation
    *    + arith, assign, bool, comma, compare
    */
   Chunk *prev = pc->GetPrevNcNnl();

   if (  prev->IsNullChunk()
      || prev == vb_open
      || prev->TestFlags(PCF_IN_PREPROC)
      || pawn_continued(prev, vb_open->GetLevel() + 1))
   {
      if (prev->IsNotNullChunk())
      {
         LOG_FMT(LPVSEMI, "%s:  no  VSEMI on line %zu, prev='%s' [%s]\n",
                 __func__, prev->GetOrigLine(), prev->Text(), get_token_name(prev->GetType()));
      }
      return(pc);
   }
   return(pawn_add_vsemi_after(prev));
}
