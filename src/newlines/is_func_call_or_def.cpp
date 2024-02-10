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
   if (  pc->GetParentType() == CT_FUNC_DEF
      || pc->GetParentType() == CT_FUNC_CALL
      || pc->GetParentType() == CT_FUNC_CALL_USER
      || pc->GetParentType() == CT_FUNC_CLASS_DEF
      || pc->GetParentType() == CT_OC_CLASS
      || pc->GetParentType() == CT_OC_MSG_DECL
      || pc->GetParentType() == CT_CS_PROPERTY
      || pc->GetParentType() == CT_CPP_LAMBDA)
   {
      return(true);
   }
   return(false);
} // is_func_call_or_def
