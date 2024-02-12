/**
 * @file is_var_def.cpp
 * Adds or removes newlines.
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/is_var_def.h"

#include "chunk.h"

using namespace std;
using namespace uncrustify;


//! Check if token starts a variable declaration
bool is_var_def(Chunk *pc, Chunk *next)
{
   if (  pc->Is(CT_DECLTYPE)
      && next->Is(CT_PAREN_OPEN))
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
   else if (next->Is(CT_DC_MEMBER))
   {
      // If next token is CT_DC_MEMBER, skip it
      next = next->SkipDcMember();
   }
   else if (next->Is(CT_ANGLE_OPEN))
   {
      // If we have a template type, skip it
      next = next->GetClosingParen();
      next = next->GetNextNcNnl();
   }
   bool is = (  (  next->IsTypeDefinition()
                && next->GetParentType() != CT_FUNC_DEF)           // Issue #2639
             || next->Is(CT_WORD)
             || next->Is(CT_FUNC_CTOR_VAR));

   return(is);
} // is_var_def
