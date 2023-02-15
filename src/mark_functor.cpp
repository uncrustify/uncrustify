/**
 * @file mark_functor.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "mark_functor.h"

#include "chunk.h"


void mark_functor()
{
   LOG_FUNC_ENTRY();
   bool  found_fparen_close = false;
   bool  in_Lamda           = false;
   Chunk *closing           = Chunk::NullChunkPtr;
   Chunk *saveLambdaOpen    = Chunk::NullChunkPtr;

   // Issue #3914
   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      LOG_FMT(LCOMBINE, "%s(%d): R1: orig line is %zu, orig col is %zu, level is %zu, Text() '%s'\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel(), pc->Text());
      log_pcf_flags(LCOMBINE, pc->GetFlags());

      if (in_Lamda)
      {
         if (  pc->Is(CT_SEMICOLON)
            && (pc->GetLevel() == (saveLambdaOpen->GetLevel() - 1)))
         {
            in_Lamda = false;
         }
      }
      else
      {
         if (pc->TestFlags(PCF_IN_LAMBDA))
         {
            in_Lamda       = true;
            saveLambdaOpen = pc;
         }
      }

      if (  found_fparen_close
         && !in_Lamda)
      {
         if (pc->Is(CT_FPAREN_OPEN))
         {
            LOG_FMT(LCOMBINE, "%s(%d): RR: FOUND orig line is %zu, orig col is %zu, level is %zu, Text() '%s'\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel(), pc->Text());
            log_pcf_flags(LCOMBINE, pc->GetFlags());

            Chunk *opening = closing->GetOpeningParen();
            opening->SetType(CT_RPAREN_OPEN);
            closing->SetType(CT_RPAREN_CLOSE);
            closing = Chunk::NullChunkPtr;

            Chunk *closing2 = pc->GetClosingParen();
            closing2->SetType(CT_RPAREN_CLOSE);
            pc->SetType(CT_RPAREN_OPEN);
            found_fparen_close = false;
         }
         else
         {
            found_fparen_close = false;
         }
      }
      else
      {
         if (  pc->Is(CT_FPAREN_CLOSE)
            || pc->Is(CT_RPAREN_CLOSE))
         {
            closing            = pc;
            found_fparen_close = true;
         }
      }
   }
} // mark_functor
