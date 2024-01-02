/**
 * @file is_class_one_liner.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "is_class_one_liner.h"


bool is_class_one_liner(Chunk *pc)
{
   if (  (  pc->Is(CT_FUNC_CLASS_DEF)
         || pc->Is(CT_FUNC_DEF))
      && pc->TestFlags(PCF_IN_CLASS))
   {
      // Find opening brace
      pc = pc->GetNextType(CT_BRACE_OPEN, pc->GetLevel());
      return(  pc->IsNotNullChunk()
            && pc->TestFlags(PCF_ONE_LINER));
   }
   return(false);
} // is_class_one_liner
