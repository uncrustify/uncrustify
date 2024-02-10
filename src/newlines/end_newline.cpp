/**
 * @file end_newline.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/end_newline.h"

#include "chunk.h"
#include "mark_change.h"


//! Ensure that the next non-comment token after close brace is a newline
void newline_end_newline(Chunk *br_close)
{
   LOG_FUNC_ENTRY();

   Chunk *next = br_close->GetNext();
   Chunk nl;

   if (!next->IsCommentOrNewline())
   {
      nl.SetOrigLine(br_close->GetOrigLine());
      nl.SetOrigCol(br_close->GetOrigCol());
      nl.SetNlCount(1);
      nl.SetPpLevel(0);
      nl.SetFlags((br_close->GetFlags() & PCF_COPY_FLAGS) & ~PCF_IN_PREPROC);

      if (  br_close->TestFlags(PCF_IN_PREPROC)
         && next->IsNotNullChunk()
         && next->TestFlags(PCF_IN_PREPROC))
      {
         nl.SetFlagBits(PCF_IN_PREPROC);
      }

      if (nl.TestFlags(PCF_IN_PREPROC))
      {
         nl.SetType(CT_NL_CONT);
         nl.Str() = "\\\n";
      }
      else
      {
         nl.SetType(CT_NEWLINE);
         nl.Str() = "\n";
      }
      MARK_CHANGE();
      LOG_FMT(LNEWLINE, "%s(%d): %zu:%zu add newline after '%s'\n",
              __func__, __LINE__, br_close->GetOrigLine(), br_close->GetOrigCol(), br_close->Text());
      nl.CopyAndAddAfter(br_close);
   }
} // newline_end_newline
