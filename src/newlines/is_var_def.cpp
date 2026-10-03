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


//! Is parent the parent type given to the return type of a function
//! declaration or definition?
static bool is_func_decl_parent(E_Token parent)
{
   return(  parent == E_Token::CT_FUNC_DEF
         || parent == E_Token::CT_FUNC_PROTO
         || parent == E_Token::CT_FUNC_CLASS_DEF
         || parent == E_Token::CT_FUNC_CLASS_PROTO);
}


//! Check if token starts a variable declaration
bool is_var_def(Chunk const *pc, Chunk const *next)
{
   // Issue #4794: a word of a function's return type isn't the start of a
   // variable definition, however many words it has ('unsigned long f();',
   // '__forceinline int f();'). Without this, the second word of such a return
   // type was taken for the name of a variable that the first one declares.
   if (is_func_decl_parent(pc->GetParentType()))
   {
      return(false);
   }

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
                && !is_func_decl_parent(next->GetParentType()))             // Issue #2639, #4794
             || next->Is(E_Token::CT_WORD)
             || next->Is(E_Token::CT_FUNC_CTOR_VAR));

   return(is);
} // is_var_def
