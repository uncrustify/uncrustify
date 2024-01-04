/**
 * @file newlines_cleanup_angles.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "newlines_cleanup_angles.h"

#include "chunk.h"
#include "newline_template.h"


void newlines_cleanup_angles()
{
   // Issue #1167
   LOG_FUNC_ENTRY();

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      char copy[1000];
      LOG_FMT(LBLANK, "%s(%d): orig line is %zu, orig col is %zu, Text() is '%s'\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->ElidedText(copy));

      if (pc->Is(CT_ANGLE_OPEN))
      {
         newline_template(pc);
      }
   }
} // newlines_cleanup_angles
