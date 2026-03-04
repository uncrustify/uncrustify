/**
 * @file is_func_call_or_def.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/is_func_call_or_def.h"

#include "chunk.h"


/**
 * Test if an opening brace is part of a function call or definition.
 */
bool is_func_call_or_def(Chunk *pc)
{
   if (  pc->GetParentType() == E_Token::FUNC_DEF
      || pc->GetParentType() == E_Token::FUNC_CALL
      || pc->GetParentType() == E_Token::FUNC_CALL_USER
      || pc->GetParentType() == E_Token::FUNC_CLASS_DEF
      || pc->GetParentType() == E_Token::OC_CLASS
      || pc->GetParentType() == E_Token::OC_MSG_DECL
      || pc->GetParentType() == E_Token::CS_PROPERTY
      || pc->GetParentType() == E_Token::CPP_LAMBDA)
   {
      return(true);
   }
   return(false);
} // is_func_call_or_def
