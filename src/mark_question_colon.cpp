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
 * will be called if a chunk ? (CT_QUESTION) is entcountered
 * return the chunk :
 * return nullptr is a chunk ; (CT_SEMI_COLON) is found
 */
Chunk *search_for_colon(Chunk *pc_local)
{
   Chunk *pc2;

   LOG_FMT(LCOMBINE, "%s(%d): orig_line is %zu, orig_col is %zu, level is %zu, Text() is '%s'\n",
           __func__, __LINE__, pc_local->orig_line, pc_local->orig_col, pc_local->level, pc_local->Text());
   Chunk *colon = pc_local->GetNextType(CT_COLON, pc_local->level);

   if (colon != nullptr)
   {
      LOG_FMT(LCOMBINE, "%s(%d): orig_line is %zu, orig_col is %zu, level is %zu, Text() is '%s'\n",
              __func__, __LINE__, colon->orig_line, colon->orig_col, colon->level, colon->Text());
      set_chunk_type(colon, CT_COND_COLON);
      flag_series(pc_local, colon, PCF_IN_CONDITIONAL);

      // examine the next tokens, search for a next CT_QUESTION
      for (pc2 = colon->GetNext(); pc2->IsNotNullChunk(); pc2 = pc2->GetNextNcNnl())
      {
         LOG_FMT(LCOMBINE, "%s(%d): THE NEXT: orig_line is %zu, orig_col is %zu, level is %zu, Text() is '%s'\n",
                 __func__, __LINE__, pc2->orig_line, pc2->orig_col, pc2->level, pc2->Text());
         chunk_flags_set(pc2, PCF_IN_CONDITIONAL);
         log_pcf_flags(LCOMBINE, pc2->flags);

         if (chunk_is_token(pc2, CT_SEMICOLON))
         {
            LOG_FMT(LCOMBINE, "%s(%d): orig_line is %zu, orig_col is %zu, level is %zu, Text() is '%s'\n",
                    __func__, __LINE__, pc2->orig_line, pc2->orig_col, pc2->level, pc2->Text());
            return(pc2);
         }
         else if (chunk_is_token(pc2, CT_QUESTION))
         {
            LOG_FMT(LCOMBINE, "%s(%d): orig_line is %zu, orig_col is %zu, level is %zu, Text() is '%s'\n",
                    __func__, __LINE__, pc2->orig_line, pc2->orig_col, pc2->level, pc2->Text());
            pc2 = search_for_colon(pc2);
            LOG_FMT(LCOMBINE, "%s(%d): orig_line is %zu, orig_col is %zu, level is %zu, Text() is '%s'\n",
                    __func__, __LINE__, pc2->orig_line, pc2->orig_col, pc2->level, pc2->Text());
            return(pc2);
         }
      }
   }
   else
   {
      LOG_FMT(LWARN, "%s(%d): %zu: Error: Expected a colon\n",
              __func__, __LINE__, pc_local->orig_line);
      cpd.error_count++;
      return(nullptr);
   }
   LOG_FMT(LCOMBINE, "%s(%d): orig_line is %zu, orig_col is %zu, level is %zu, Text() is '?'\n",
           __func__, __LINE__, pc2->orig_line, pc2->orig_col, pc2->level);
   return(pc2);
} // search_for_colon


void mark_question_colon(void)
{
   LOG_FUNC_ENTRY();
   Chunk *pc;

   // Issue #3558
   for (pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      LOG_FMT(LCOMBINE, "%s(%d): orig_line is %zu, orig_col is %zu, level is %zu, Text() '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->level, pc->Text());
      log_pcf_flags(LCOMBINE, pc->flags);

      if (chunk_is_token(pc, CT_QUESTION))
      {
         Chunk *colon = search_for_colon(pc);

         if (colon != nullptr)
         {
            LOG_FMT(LCOMBINE, "%s(%d): orig_line is %zu, orig_col is %zu, level is %zu, Text() is '%s'\n",
                    __func__, __LINE__, colon->orig_line, colon->orig_col, colon->level, colon->Text());

            if (chunk_is_token(colon, CT_SEMICOLON))
            {
               // set at the end of the question statement ...
               pc = colon;
               LOG_FMT(LCOMBINE, "%s(%d): orig_line is %zu, orig_col is %zu, level is %zu, Text() is '%s'\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->level, pc->Text());
               // ... and go on
            }
         }
         else
         {
            LOG_FMT(LWARN, "%s(%d): %zu: Error: Expected a colon\n",
                    __func__, __LINE__, pc->orig_line);
            cpd.error_count++;
            return;
         }
      }
   }
} // mark_question_colon
