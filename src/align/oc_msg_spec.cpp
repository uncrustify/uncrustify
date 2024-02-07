/**
 * @file oc_msg_spec.cpp
 *
 * @author  Guy Maurel
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align/oc_msg_spec.h"

#include "align/assign.h"
#include "align/stack.h"


void align_oc_msg_spec(size_t span)
{
   LOG_FUNC_ENTRY();

   AlignStack as;

   as.Start(span, 0);

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (pc->IsNewline())
      {
         as.NewLines(pc->GetNlCount());
      }
      else if (pc->Is(CT_OC_MSG_SPEC))
      {
         as.Add(pc);
      }
   }

   as.End();
} // void align_oc_msg_spec
