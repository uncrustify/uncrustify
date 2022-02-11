/**
 * @file align_oc_msg_spec.cpp
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align_oc_msg_spec.h"

#include "align_assign.h"
#include "align_stack.h"


void align_oc_msg_spec(size_t span)
{
   LOG_FUNC_ENTRY();

   AlignStack as;

   as.Start(span, 0);

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (chunk_is_newline(pc))
      {
         as.NewLines(pc->nl_count);
      }
      else if (chunk_is_token(pc, CT_OC_MSG_SPEC))
      {
         as.Add(pc);
      }
   }

   as.End();
} // void align_oc_msg_spec
