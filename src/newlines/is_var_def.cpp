/**
 * @file is_var_def.cpp
 * Adds or removes newlines.
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/is_var_def.h"

#include "chunk.h"

#include <string>

using namespace std;

using namespace uncrustify;


//! Check if token starts a variable declaration
bool is_var_def(Chunk *pc, Chunk *next)
{
   if (  pc->Is(E_Token::CT_DECLTYPE)
      && next->Is(E_Token::CT_PAREN_OPEN))
   {
      // If current token starts a decltype expression, skip it
      next = next->GetClosingParen();
      next = next->GetNextNcNnl();
   }
   else if (!pc->IsTypeDefinition())
   {
      // Otherwise, if the current token is not a type --> not a declaration
      return(false);
   }
   else if (next->Is(E_Token::CT_DC_MEMBER))
   {
      // If next token is E_Token::CT_DC_MEMBER, skip it
      next = next->SkipDcMember();
   }
   else if (next->Is(E_Token::CT_ANGLE_OPEN))
   {
      // If we have a template type, skip it
      next = next->GetClosingParen();
      next = next->GetNextNcNnl();
   }
   bool is = (  (  next->IsTypeDefinition()
                && next->GetParentType() != E_Token::CT_FUNC_DEF)           // Issue #2639
             || next->Is(E_Token::CT_WORD)
             || next->Is(E_Token::CT_FUNC_CTOR_VAR));

   return(is);
} // is_var_def
