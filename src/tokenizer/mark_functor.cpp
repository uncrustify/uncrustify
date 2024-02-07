/**
 * @file mark_functor.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "tokenizer/mark_functor.h"

#include "chunk.h"


/*
 * tokenize the functor such as:
 * desc->add_options() ( a ) (b)(c);
 */
void mark_functor()
{
   LOG_FUNC_ENTRY();
   bool  found_functor  = false;
   Chunk *is_it_closing = Chunk::NullChunkPtr;

   // Issue #3914
   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() '%s'\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel(), pc->Text());

      if (pc->Is(CT_SEMICOLON))
      {
         found_functor = false;
         continue;
      }

      if (found_functor)
      {
         if (  pc->Is(CT_FPAREN_CLOSE)
            || pc->Is(CT_RPAREN_CLOSE))
         {
            LOG_FMT(LCOMBINE, "%s(%d): FOUND a Closing: orig line is %zu, orig col is %zu, level is %zu, Text() '%s'\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel(), pc->Text());
            // desc->add_options() ( a ) (
            //                         ^
            pc->SetType(CT_RPAREN_CLOSE);
         }
         else if (  pc->Is(CT_FPAREN_OPEN)
                 || pc->Is(CT_RPAREN_OPEN))
         {
            LOG_FMT(LCOMBINE, "%s(%d): FOUND a Opening: orig line is %zu, orig col is %zu, level is %zu, Text() '%s'\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel(), pc->Text());
            // desc->add_options() ( a ) (
            //                           ^
            pc->SetType(CT_RPAREN_OPEN);
         }
         else // pc->Is(CT_FPAREN_CLOSE) || pc->Is(CT_RPAREN_CLOSE))
         {
            continue;
         }
      }
      else // (found_functor)
      {
         if (pc->Is(CT_FPAREN_OPEN))
         {
            LOG_FMT(LCOMBINE, "%s(%d): FOUND 1 Opening: orig line is %zu, orig col is %zu, level is %zu, Text() '%s'\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel(), pc->Text());
            is_it_closing = pc->GetPrevNcNnl();
            LOG_FMT(LCOMBINE, "%s(%d): FOUND 2 Closing: orig line is %zu, orig col is %zu, level is %zu, Text() '%s'\n",
                    __func__, __LINE__, is_it_closing->GetOrigLine(), is_it_closing->GetOrigCol(), is_it_closing->GetLevel(), is_it_closing->Text());

            if (is_it_closing->Is(CT_FPAREN_CLOSE))
            {
               Chunk *opening = is_it_closing->GetOpeningParen();
               LOG_FMT(LCOMBINE, "%s(%d): FOUND 3 Opening: orig line is %zu, orig col is %zu, level is %zu, Text() '%s'\n",
                       __func__, __LINE__, opening->GetOrigLine(), opening->GetOrigCol(), opening->GetLevel(), opening->Text());
               // look for member function
               Chunk *is_it_func = opening->GetPrevNcNnl();
               LOG_FMT(LCOMBINE, "%s(%d): FOUND 4 func: orig line is %zu, orig col is %zu, level is %zu, Text() '%s'\n",
                       __func__, __LINE__, is_it_func->GetOrigLine(), is_it_func->GetOrigCol(), is_it_func->GetLevel(), is_it_func->Text());
               Chunk *is_it_member = is_it_func->GetPrevNcNnl();     // CT_MEMBER
               LOG_FMT(LCOMBINE, "%s(%d): FOUND 5 func: orig line is %zu, orig col is %zu, level is %zu, Text() '%s'\n",
                       __func__, __LINE__, is_it_member->GetOrigLine(), is_it_member->GetOrigCol(), is_it_member->GetLevel(), is_it_member->Text());

               if (is_it_member->Is(CT_MEMBER))
               {
                  // set parenthesis at the function
                  // desc->add_options() ( a ) (
                  //                   ^
                  is_it_closing->SetType(CT_RPAREN_CLOSE);
                  // desc->add_options() ( a ) (
                  //                  ^
                  opening->SetType(CT_RPAREN_OPEN);
                  // desc->add_options() ( a ) (
                  //                     ^
                  pc->SetType(CT_RPAREN_OPEN);
                  found_functor = true;
               }
               else
               {
                  continue;
               }
            }
            else
            {
               LOG_FMT(LCOMBINE, "%s(%d): NOT useable\n", __func__, __LINE__);
               continue;
            }
         }
         else // (pc->Is(CT_FPAREN_OPEN))
         {
            continue;
         } // (pc->Is(CT_FPAREN_OPEN))
      } // (found_functor)
   }
} // mark_functor
