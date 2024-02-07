/**
 * @file left_shift.cpp
 *
 * @author  Guy Maurel
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align/left_shift.h"

#include "align/stack.h"
#include "indent.h"
#include "log_rules.h"


constexpr static auto LCURRENT = LALIGN;


using namespace uncrustify;


void align_left_shift()
{
   LOG_FUNC_ENTRY();

   Chunk      *start = Chunk::NullChunkPtr;
   AlignStack as;

   as.Start(255);

   Chunk *pc = Chunk::GetHead();

   while (pc->IsNotNullChunk())
   {
      if (pc->IsNewline())
      {
         LOG_FMT(LALIGN, "%s(%d): orig line is %zu, <Newline>\n", __func__, __LINE__, pc->GetOrigLine());
      }
      else
      {
         char copy[1000];
         LOG_FMT(LALIGN, "%s(%d): orig line is %zu, orig col is %zu, pc->Text() '%s'\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->ElidedText(copy));
      }

      if (  start->IsNotNullChunk()
         && ((pc->GetFlags() & PCF_IN_PREPROC) != (start->GetFlags() & PCF_IN_PREPROC)))
      {
         // a change in preproc status restarts the aligning
         as.Flush();
         start = Chunk::NullChunkPtr;
      }
      else if (pc->IsNewline())
      {
         as.NewLines(pc->GetNlCount());
      }
      else if (  start->IsNotNullChunk()
              && pc->GetLevel() < start->GetLevel())
      {
         // A drop in level restarts the aligning
         as.Flush();
         start = Chunk::NullChunkPtr;
      }
      else if (  start->IsNotNullChunk()
              && pc->GetLevel() > start->GetLevel())
      {
         // Ignore any deeper levels when aligning
      }
      else if (pc->Is(CT_SEMICOLON))
      {
         // A semicolon at the same level flushes
         as.Flush();
         start = Chunk::NullChunkPtr;
      }
      else if (  !pc->TestFlags(PCF_IN_ENUM)
              && !pc->TestFlags(PCF_IN_TYPEDEF)
              && pc->IsString("<<"))
      {
         if (pc->GetParentType() == CT_OPERATOR)
         {
            // Ignore operator<<
         }
         else if (as.m_aligned.Empty())
         {
            /*
             * check if the first one is actually on a blank line and then
             * indent it. Eg:
             *
             *      cout
             *          << "something";
             */
            Chunk *prev = pc->GetPrev();

            if (  prev->IsNotNullChunk()
               && prev->IsNewline())
            {
               log_rule_B("indent_columns");
               indent_to_column(pc, pc->GetColumnIndent() + options::indent_columns());
               pc->SetColumnIndent(pc->GetColumn());
               pc->SetFlagBits(PCF_DONT_INDENT);
            }
            // first one can be anywhere
            as.Add(pc);
            start = pc;
         }
         else if (pc->GetPrev()->IsNewline())
         {
            // subsequent ones must be after a newline
            as.Add(pc);
         }
      }
      else if (!as.m_aligned.Empty())
      {
         /*
          * check if the given statement is on a line of its own, immediately following <<
          * and then it. Eg:
          *
          *      cout <<
          *          "something";
          */
         Chunk *prev = pc->GetPrev();

         if (  prev->IsNotNullChunk()
            && prev->IsNewline())
         {
            log_rule_B("indent_columns");
            indent_to_column(pc, pc->GetColumnIndent() + options::indent_columns());
            pc->SetColumnIndent(pc->GetColumn());
            pc->SetFlagBits(PCF_DONT_INDENT);
         }
      }
      pc = pc->GetNext();
   }
   as.End();
} // align_left_shift
