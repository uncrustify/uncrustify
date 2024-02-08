/**
 * @file setup_newline_add.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#include "newlines/setup_newline_add.h"

#include "chunk.h"
#include "logger.h"
#include "newlines/one_liner.h"


/**
 * Basic approach:
 * 1. Find next open brace
 * 2. Find next close brace
 * 3. Determine why the braces are there
 * a. struct/union/enum "enum [name] {"
 * c. assignment "= {"
 * b. if/while/switch/for/etc ") {"
 * d. else "} else {"
 */
void setup_newline_add(Chunk *prev, Chunk *nl, Chunk *next)
{
   LOG_FUNC_ENTRY();

   if (  prev->IsNullChunk()
      || nl->IsNullChunk()
      || next->IsNullChunk())
   {
      return;
   }
   undo_one_liner(prev);

   nl->SetOrigLine(prev->GetOrigLine());
   nl->SetLevel(prev->GetLevel());
   nl->SetPpLevel(prev->GetPpLevel());
   nl->SetBraceLevel(prev->GetBraceLevel());
   nl->SetPpLevel(prev->GetPpLevel());
   nl->SetNlCount(1);
   nl->SetFlags((prev->GetFlags() & PCF_COPY_FLAGS) & ~PCF_IN_PREPROC);
   nl->SetOrigCol(prev->GetOrigColEnd());
   nl->SetColumn(prev->GetOrigCol());

   if (  prev->TestFlags(PCF_IN_PREPROC)
      && next->TestFlags(PCF_IN_PREPROC))
   {
      nl->SetFlagBits(PCF_IN_PREPROC);
   }

   if (nl->TestFlags(PCF_IN_PREPROC))
   {
      nl->SetType(CT_NL_CONT);
      nl->Str() = "\\\n";
   }
   else
   {
      nl->SetType(CT_NEWLINE);
      nl->Str() = "\n";
   }
} // setup_newline_add
