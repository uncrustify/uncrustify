/**
 * @file newline_case.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "newline_case.h"

#include "double_newline.h"
#include "newline_add.h"


/**
 * Put a empty line between the 'case' statement and the previous case colon
 * or semicolon.
 * Does not work with PAWN (?)
 */
void newline_case(Chunk *start)
{
   LOG_FUNC_ENTRY();

   //   printf("%s case (%s) on line %d col %d\n",
   //          __func__, c_chunk_names[start->GetType()],
   //          start->GetOrigLine(), start->GetOrigCol());

   // Scan backwards until a '{' or ';' or ':'. Abort if a multi-newline is found
   Chunk *prev = start;

   do
   {
      prev = prev->GetPrevNc();

      if (  prev->IsNotNullChunk()
         && prev->IsNewline()
         && prev->GetNlCount() > 1)
      {
         return;
      }
   } while (  prev->IsNot(CT_BRACE_OPEN)
           && prev->IsNot(CT_BRACE_CLOSE)
           && prev->IsNot(CT_SEMICOLON)
           && prev->IsNot(CT_CASE_COLON));

   if (prev->IsNullChunk())
   {
      return;
   }
   Chunk *pc = newline_add_between(prev, start);

   if (pc == nullptr)
   {
      return;
   }

   // Only add an extra line after a semicolon or brace close
   if (  prev->Is(CT_SEMICOLON)
      || prev->Is(CT_BRACE_CLOSE))
   {
      if (  pc->IsNewline()
         && pc->GetNlCount() < 2)
      {
         double_newline(pc);
      }
   }
} // newline_case


void newline_case_colon(Chunk *start)
{
   LOG_FUNC_ENTRY();

   // Scan forwards until a non-comment is found
   Chunk *pc = start;

   do
   {
      pc = pc->GetNext();
   } while (pc->IsComment());

   if (  pc->IsNotNullChunk()
      && !pc->IsNewline())
   {
      newline_add_before(pc);
   }
} // newline_case_colon
