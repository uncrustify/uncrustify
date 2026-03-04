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
      LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, text '%s'\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel(), pc->GetLogText());

      if (pc->Is(E_Token::SEMICOLON))
      {
         found_functor = false;
         continue;
      }

      if (found_functor)
      {
         if (  pc->Is(E_Token::FPAREN_CLOSE)
            || pc->Is(E_Token::RPAREN_CLOSE))
         {
            LOG_FMT(LCOMBINE, "%s(%d): FOUND a Closing: orig line is %zu, orig col is %zu, level is %zu, text '%s'\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel(), pc->GetLogText());
            // desc->add_options() ( a ) (
            //                         ^
            pc->SetType(E_Token::RPAREN_CLOSE);
         }
         else if (  pc->Is(E_Token::FPAREN_OPEN)
                 || pc->Is(E_Token::RPAREN_OPEN))
         {
            LOG_FMT(LCOMBINE, "%s(%d): FOUND a Opening: orig line is %zu, orig col is %zu, level is %zu, text '%s'\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel(), pc->GetLogText());
            // desc->add_options() ( a ) (
            //                           ^
            pc->SetType(E_Token::RPAREN_OPEN);
         }
         else // pc->Is(E_Token::FPAREN_CLOSE) || pc->Is(E_Token::RPAREN_CLOSE))
         {
            continue;
         }
      }
      else // (found_functor)
      {
         if (pc->Is(E_Token::FPAREN_OPEN))
         {
            LOG_FMT(LCOMBINE, "%s(%d): FOUND 1 Opening: orig line is %zu, orig col is %zu, level is %zu, text '%s'\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel(), pc->GetLogText());
            is_it_closing = pc->GetPrevNcNnl();
            LOG_FMT(LCOMBINE, "%s(%d): FOUND 2 Closing: orig line is %zu, orig col is %zu, level is %zu, text '%s'\n",
                    __func__, __LINE__, is_it_closing->GetOrigLine(), is_it_closing->GetOrigCol(), is_it_closing->GetLevel(), is_it_closing->GetLogText());

            if (is_it_closing->Is(E_Token::FPAREN_CLOSE))
            {
               Chunk *opening = is_it_closing->GetOpeningParen();
               LOG_FMT(LCOMBINE, "%s(%d): FOUND 3 Opening: orig line is %zu, orig col is %zu, level is %zu, text '%s'\n",
                       __func__, __LINE__, opening->GetOrigLine(), opening->GetOrigCol(), opening->GetLevel(), opening->GetLogText());
               // look for member function
               Chunk *is_it_func = opening->GetPrevNcNnl();
               LOG_FMT(LCOMBINE, "%s(%d): FOUND 4 func: orig line is %zu, orig col is %zu, level is %zu, text '%s'\n",
                       __func__, __LINE__, is_it_func->GetOrigLine(), is_it_func->GetOrigCol(), is_it_func->GetLevel(), is_it_func->GetLogText());
               Chunk *is_it_member = is_it_func->GetPrevNcNnl();     // E_Token::MEMBER
               LOG_FMT(LCOMBINE, "%s(%d): FOUND 5 func: orig line is %zu, orig col is %zu, level is %zu, text '%s'\n",
                       __func__, __LINE__, is_it_member->GetOrigLine(), is_it_member->GetOrigCol(), is_it_member->GetLevel(), is_it_member->GetLogText());

               if (is_it_member->Is(E_Token::MEMBER))
               {
                  // set parenthesis at the function
                  // desc->add_options() ( a ) (
                  //                   ^
                  is_it_closing->SetType(E_Token::RPAREN_CLOSE);
                  // desc->add_options() ( a ) (
                  //                  ^
                  opening->SetType(E_Token::RPAREN_OPEN);
                  // desc->add_options() ( a ) (
                  //                     ^
                  pc->SetType(E_Token::RPAREN_OPEN);
                  found_functor = true;
               }
               else
               {
                  continue;
               }
            }
            else
            {
               LOG_FMT(LCOMBINE, "%s(%d): NOT usable\n", __func__, __LINE__);
               continue;
            }
         }
         else // (pc->Is(E_Token::FPAREN_OPEN))
         {
            continue;
         } // (pc->Is(E_Token::FPAREN_OPEN))
      } // (found_functor)
   }
} // mark_functor
