/**
 * @file mark_question_colon.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "tokenizer/mark_question_colon.h"

#include "chunk.h"
#include "log_levels.h"
#include "tokenizer/combine_tools.h"


/*
 * Issue #3558
 * will be called if a ? (CT_QUESTION) chunk is encountered
 * return the chunk colon if found or Chunk::NullChunkPtr
 * if a ; (CT_SEMI_COLON) chunk is found
 */
Chunk *search_for_colon(Chunk *pc_question, int depth)
{
   Chunk *pc2                 = pc_question->GetNextNcNnl();
   bool  colon_found          = false;
   int   square_bracket_depth = 0;

   LOG_FMT(LCOMBINE, "%s(%d): pc_question.orig line is %zu, orig col is %zu, level is %zu, Text() is '%s'\n",
           __func__, __LINE__, pc_question->GetOrigLine(), pc_question->GetOrigCol(), pc_question->GetLevel(),
           pc_question->Text());

   if (pc2->Is(CT_COLON))
   {
      return(pc2);
   }

   // examine the next tokens, look for E2, E3, COLON, might be for a next CT_QUESTION
   while (pc2->IsNotNullChunk())
   {
      LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '%s'\n",
              __func__, __LINE__, pc2->GetOrigLine(), pc2->GetOrigCol(), pc2->GetLevel(), pc2->Text());

      if (  pc2->Is(CT_SEMICOLON)
         || (  pc2->Is(CT_PAREN_CLOSE)
            && (pc_question->GetLevel() == pc2->GetLevel() + 1))
         || pc2->Is(CT_COMMA))
      {
         LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '%s'\n",
                 __func__, __LINE__, pc2->GetOrigLine(), pc2->GetOrigCol(), pc2->GetLevel(), pc2->Text());
         pc2->SetFlagBits(PCF_IN_CONDITIONAL);
         log_pcf_flags(LCOMBINE, pc2->GetFlags());

         if (colon_found)
         {
            LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '%s'\n",
                    __func__, __LINE__, pc2->GetOrigLine(), pc2->GetOrigCol(), pc2->GetLevel(), pc2->Text());
            pc_question->SetParent(pc2);   // back again

            LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '%s'\n",
                    __func__, __LINE__, pc2->GetOrigLine(), pc2->GetOrigCol(), pc2->GetLevel(), pc2->Text());
            return(pc2);
         }
         else
         {
            pc2->SetParent(pc_question);   // save the question token
            pc_question->SetParent(pc2);   // back again
         }
      }
      else if (pc2->Is(CT_COMMA))
      {
         // TODO: is it necessary?
      }
      else if (pc2->Is(CT_QUESTION))
      {
         LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '%s'\n",
                 __func__, __LINE__, pc2->GetOrigLine(), pc2->GetOrigCol(), pc2->GetLevel(), pc2->Text());
         pc2 = search_for_colon(pc2, depth + 1);
         LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '%s'\n",
                 __func__, __LINE__, pc2->GetOrigLine(), pc2->GetOrigCol(), pc2->GetLevel(), pc2->Text());
         continue;
      }
      else if (pc2->Is(CT_COND_COLON))
      {
         LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '%s'\n",
                 __func__, __LINE__, pc2->GetOrigLine(), pc2->GetOrigCol(), pc2->GetLevel(), pc2->Text());

         if (colon_found)
         {
            LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '%s'\n",
                    __func__, __LINE__, pc2->GetOrigLine(), pc2->GetOrigCol(), pc2->GetLevel(), pc2->Text());
            Chunk *pr = pc2->GetPrevNcNnl();
            return(pr);
         }
         else
         {
            pc2->SetParent(pc_question);              // save the question token
            pc_question->SetParent(pc2);              // back again
            colon_found = true;
         }

         if (pc2->Is(CT_COLON))
         {
            if (colon_found)
            {
               return(pc2);
            }
            else
            {
            }
         }
      }
      else if (  pc2->Is(CT_COLON)
              && square_bracket_depth == 0)
      {
         LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '%s'\n",
                 __func__, __LINE__, pc2->GetOrigLine(), pc2->GetOrigCol(), pc2->GetLevel(), pc2->Text());

         if (colon_found && depth > 0)
         {
            // There can only be another CT_COND_COLON if there is more than 1 CT_QUESTION (ie. depth > 0)
            pc2->SetType(CT_COND_COLON);
            return(pc2);
         }
         else if (!colon_found)
         {
            // E2 found   orig line is 23, orig col is 3
            pc2->SetType(CT_COND_COLON);
            LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '%s'\n",
                    __func__, __LINE__, pc2->GetOrigLine(), pc2->GetOrigCol(), pc2->GetLevel(), pc2->Text());
            pc2->SetParent(pc_question);              // save the question token
            pc_question->SetParent(pc2);              // back again

            // look for E3
            colon_found = true;
         }
      }
      else if (pc2->Is(CT_SQUARE_OPEN))
      {
         square_bracket_depth++;
      }
      else if (pc2->Is(CT_SQUARE_CLOSE))
      {
         square_bracket_depth--;
      }
      pc2 = pc2->GetNextNcNnl();
   }

   if (pc2->IsNotNullChunk())
   {
      LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '?'\n",
              __func__, __LINE__, pc2->GetOrigLine(), pc2->GetOrigCol(), pc2->GetLevel());
   }
   LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '?'\n",
           __func__, __LINE__, pc2->GetOrigLine(), pc2->GetOrigCol(), pc2->GetLevel());
   return(pc2);
} // search_for_colon


void mark_question_colon()
{
   LOG_FUNC_ENTRY();
   Chunk *pc = Chunk::GetHead();
   Chunk *pc_question;

   // Issue #3558
   while (pc->IsNotNullChunk())
   {
      LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() '%s'\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel(), pc->Text());
      log_pcf_flags(LCOMBINE, pc->GetFlags());

      if (  pc->Is(CT_QUESTION)
         && !language_is_set(lang_flag_e::LANG_JAVA))
      {
         pc_question = pc;
         // look for E2, COLON, E3...
         pc = search_for_colon(pc, 0);

         LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '%s'\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel(), pc->Text());

         if (  pc->Is(CT_SEMICOLON)
            || (  pc->Is(CT_PAREN_CLOSE)
               && (pc_question->GetLevel() == pc->GetLevel() + 1))
            || pc->Is(CT_COMMA))
         {
            // set at the end of the question statement ...
            LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '%s'\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel(), pc->Text());
            // ... and go on
         }
      }
      pc = pc->GetNextNcNnl();
   }

   for (pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() '%s'\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel(), pc->Text());

      if (pc->Is(CT_QUESTION))
      {
         Chunk *from = pc;
         Chunk *to   = pc->GetParent();
         flag_series(from, to, PCF_IN_CONDITIONAL);
         pc = to;
      }
   }
} // mark_question_colon
