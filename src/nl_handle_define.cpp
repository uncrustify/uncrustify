/**
 * @file nl_handle_define.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "nl_handle_define.h"

#include "newline_add_after.h"


//! Find the next newline or nl_cont
void nl_handle_define(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   Chunk *nl  = pc;
   Chunk *ref = Chunk::NullChunkPtr;

   while ((nl = nl->GetNext())->IsNotNullChunk())
   {
      if (nl->Is(CT_NEWLINE))
      {
         return;
      }

      if (  nl->Is(CT_MACRO)
         || (  nl->Is(CT_FPAREN_CLOSE)
            && nl->GetParentType() == CT_MACRO_FUNC))
      {
         ref = nl;
      }

      if (nl->Is(CT_NL_CONT))
      {
         if (ref->IsNotNullChunk())
         {
            newline_add_after(ref);
         }
         return;
      }
   }
} // nl_handle_define
