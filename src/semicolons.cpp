/**
 * @file semicolons.cpp
 * Removes extra semicolons
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "semicolons.h"

#include "prototypes.h"


static void remove_semicolon(Chunk *pc);


/**
 * We are on a semicolon that is after an unidentified brace close.
 * Check for what is before the brace open.
 * Do not remove if it is a square close, word, type, or paren close.
 */
static void check_unknown_brace_close(Chunk *semi, Chunk *brace_close);


static void remove_semicolon(Chunk *pc)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LDELSEMI, "%s(%d): Removed semicolon: orig line is %zu, orig col is %zu",
           __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol());
   log_func_stack_inline(LDELSEMI);
   // TODO: do we want to shift stuff back a column?
   Chunk::Delete(pc);
}


void remove_extra_semicolons()
{
   LOG_FUNC_ENTRY();

   Chunk *pc = Chunk::GetHead();

   while (pc->IsNotNullChunk())
   {
      Chunk *next = pc->GetNextNcNnl();
      Chunk *prev;

      if (  pc->Is(E_Token::SEMICOLON)
         && !pc->TestFlags(PCF_IN_PREPROC)
         && (prev = pc->GetPrevNcNnl())->IsNotNullChunk())
      {
         LOG_FMT(LSCANSEMI, "%s(%d): Semi orig line is %zu, orig col is %zu, parent is %s, prev = '%s' [%s/%s]\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetParentType()),
                 prev->GetLogText(),
                 get_token_name(prev->GetType()), get_token_name(prev->GetParentType()));

         if (pc->GetParentType() == E_Token::TYPEDEF)
         {
            // keep it
         }
         else if (  prev->Is(E_Token::BRACE_CLOSE)
                 && (  prev->GetParentType() == E_Token::ELSE
                    || prev->GetParentType() == E_Token::ELSEIF
                    || prev->GetParentType() == E_Token::FOR
                    || prev->GetParentType() == E_Token::FUNC_CLASS_DEF
                    || prev->GetParentType() == E_Token::FUNC_DEF
                    || prev->GetParentType() == E_Token::IF
                    || prev->GetParentType() == E_Token::NAMESPACE
                    || prev->GetParentType() == E_Token::OC_MSG_DECL
                    || prev->GetParentType() == E_Token::SWITCH
                    || prev->GetParentType() == E_Token::USING_STMT
                    || prev->GetParentType() == E_Token::WHILE))
         {
            // looking for code block vs. initialisation
            bool  code_block_found = true;
            Chunk *closing_brace   = pc->GetPrevNcNnl();                   // Issue #3506

            if (closing_brace->IsNotNullChunk())
            {
               Chunk *opening_brace = closing_brace->GetOpeningParen();

               if (opening_brace->IsNotNullChunk())
               {
                  Chunk *equal_sign = opening_brace->GetPrevNcNnl();

                  if (  equal_sign->IsNotNullChunk()
                     && equal_sign->Is(E_Token::ASSIGN))
                  {
                     // initialisation found
                     code_block_found = false;
                  }
               }
            }

            if (code_block_found)
            {
               // code block found
               remove_semicolon(pc);
            }
         }
         else if (  prev->Is(E_Token::BRACE_CLOSE)
                 && prev->GetParentType() == E_Token::NONE)
         {
            check_unknown_brace_close(pc, prev);
         }
         else if (  prev->Is(E_Token::SEMICOLON)
                 && prev->GetParentType() != E_Token::FOR)
         {
            remove_semicolon(pc);
         }
         else if (  language_is_set(lang_flag_e::LANG_D)
                 && (  prev->GetParentType() == E_Token::ENUM
                    || prev->GetParentType() == E_Token::STRUCT
                    || prev->GetParentType() == E_Token::UNION))
         {
            remove_semicolon(pc);
         }
         else if (  language_is_set(lang_flag_e::LANG_JAVA)
                 && prev->GetParentType() == E_Token::SYNCHRONIZED)
         {
            remove_semicolon(pc);
         }
         else if (prev->Is(E_Token::BRACE_OPEN))
         {
            remove_semicolon(pc);
         }
      }
      pc = next;
   }
} // remove_extra_semicolons


static void check_unknown_brace_close(Chunk *semi, Chunk *brace_close)
{
   LOG_FUNC_ENTRY();
   Chunk *pc = brace_close->GetPrevType(E_Token::BRACE_OPEN, brace_close->GetLevel());

   pc = pc->GetPrevNcNnl();

   if (  pc->IsNotNullChunk()
      && pc->IsNot(E_Token::ANGLE_CLOSE)
      && pc->IsNot(E_Token::COND_COLON)                      // Issue #3920
      && pc->IsNot(E_Token::RETURN)
      && pc->IsNot(E_Token::SQUARE_CLOSE)
      && pc->IsNot(E_Token::TSQUARE)
      && pc->IsNot(E_Token::TYPE)
      && pc->IsNot(E_Token::WORD)
      && !pc->IsParenClose())
   {
      remove_semicolon(semi);
   }
}
