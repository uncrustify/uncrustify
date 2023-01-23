/**
 * @file mark_question_colon.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "mark_question_colon.h"

#include "chunk.h"
#include "combine_tools.h"
#include "log_levels.h"
#include "unc_tools.h"


/*
 * Issue #3558
 * will be called if a ? (CT_QUESTION) chunk is encountered
 * return the chunk colon if found or Chunk::NullChunkPtr
 * if a ; (CT_SEMI_COLON) chunk is found
 */
Chunk *search_for_colon(Chunk *pc_local)
{
   Chunk *pc2;

   LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '%s'\n",
           __func__, __LINE__, pc_local->GetOrigLine(), pc_local->GetOrigCol(), pc_local->GetLevel(), pc_local->Text());
   Chunk *colon = pc_local->GetNextType(CT_COLON, pc_local->GetLevel());

   if (colon != nullptr)
   {
      LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '%s'\n",
              __func__, __LINE__, colon->GetOrigLine(), colon->GetOrigCol(), colon->GetLevel(), colon->Text());
      colon->SetType(CT_COND_COLON);
      flag_series(pc_local, colon, PCF_IN_CONDITIONAL);

      // examine the next tokens, search for a next CT_QUESTION
      for (pc2 = colon->GetNext(); pc2->IsNotNullChunk(); pc2 = pc2->GetNextNcNnl())
      {
         LOG_FMT(LCOMBINE, "%s(%d): THE NEXT: orig line is %zu, orig col is %zu, level is %zu, Text() is '%s'\n",
                 __func__, __LINE__, pc2->GetOrigLine(), pc2->GetOrigCol(), pc2->GetLevel(), pc2->Text());
         pc2->SetFlagBits(PCF_IN_CONDITIONAL);
         log_pcf_flags(LCOMBINE, pc2->GetFlags());

         if (pc2->Is(CT_SEMICOLON))
         {
            LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '%s'\n",
                    __func__, __LINE__, pc2->GetOrigLine(), pc2->GetOrigCol(), pc2->GetLevel(), pc2->Text());
            return(pc2);
         }
         else if (pc2->Is(CT_QUESTION))
         {
            LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '%s'\n",
                    __func__, __LINE__, pc2->GetOrigLine(), pc2->GetOrigCol(), pc2->GetLevel(), pc2->Text());
            pc2 = search_for_colon(pc2);
            LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '%s'\n",
                    __func__, __LINE__, pc2->GetOrigLine(), pc2->GetOrigCol(), pc2->GetLevel(), pc2->Text());
            return(pc2);
         }
      }
   }
   else
   {
      LOG_FMT(LWARN, "%s(%d): %zu: Error: Expected a colon\n",
              __func__, __LINE__, pc_local->GetOrigLine());
      exit(EX_SOFTWARE);
   }
   LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '?'\n",
           __func__, __LINE__, pc2->GetOrigLine(), pc2->GetOrigCol(), pc2->GetLevel());
   return(pc2);
} // search_for_colon


void mark_question_colon()
{
   LOG_FUNC_ENTRY();
   Chunk *pc;

   // Issue #3558
   for (pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() '%s'\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel(), pc->Text());
      log_pcf_flags(LCOMBINE, pc->GetFlags());

      if (pc->Is(CT_QUESTION))
      {
         Chunk *colon = search_for_colon(pc);

         if (colon != nullptr)
         {
            LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '%s'\n",
                    __func__, __LINE__, colon->GetOrigLine(), colon->GetOrigCol(), colon->GetLevel(), colon->Text());

            if (colon->Is(CT_SEMICOLON))
            {
               // set at the end of the question statement ...
               pc = colon;
               LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '%s'\n",
                       __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel(), pc->Text());
               // ... and go on
            }
         }
         else
         {
            LOG_FMT(LWARN, "%s(%d): %zu: Error: Expected a colon\n",
                    __func__, __LINE__, pc->GetOrigLine());
            exit(EX_SOFTWARE);
         }
      }
   }
} // mark_question_colon
