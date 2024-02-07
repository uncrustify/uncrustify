/**
 * @file eigen_comma_init.cpp
 *
 * @author  Matthew Woehlke
 * @author  Guy Maurel
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align/eigen_comma_init.h"

#include "align/stack.h"
#include "indent.h"
#include "log_rules.h"


constexpr static auto LCURRENT = LALIGN;


using namespace uncrustify;


void align_eigen_comma_init()
{
   LOG_FUNC_ENTRY();

   Chunk      *start = Chunk::NullChunkPtr;
   AlignStack as;

   as.Start(255);

   Chunk *pc = Chunk::GetHead();

   while (pc->IsNotNullChunk())
   {
      LOG_CHUNK(LTOK, pc);

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
         else
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
            // Restart alignment
            as.Flush();
            as.Add(pc->GetNext());
            start = pc;
         }
      }
      else if (!as.m_aligned.Empty())
      {
         Chunk *prev = pc->GetPrev();

         if (  prev->IsNewline()
            && pc->GetPrevNcNnl()->Is(CT_COMMA))
         {
            log_rule_B("align_eigen_comma_init");
            as.Add(pc);
         }
      }
      pc = pc->GetNext();
   }
   as.End();
} // align_left_shift
