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
 *
 * Test #51008: Added is_sibling_ternary parameter to indicate when we're
 * processing a sibling ternary in an OC message (not a nested ternary).
 * Sibling ternaries should not mark subsequent OC selector colons as CT_COND_COLON.
 */
Chunk *search_for_colon(Chunk *pc_question, int depth, bool is_sibling_ternary = false)
{
   Chunk *pc2                    = pc_question->GetNextNcNnl();
   bool  colon_found             = false;
   bool  colon_after_colon_found = false;   // Test #51007: Track if we see a colon after our ternary colon
   int   square_bracket_depth    = 0;
   int   brace_depth             = 0;       // Test #51012: Track OC dictionary @{...} depth

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

      // Issue: Don't treat comma inside OC message as terminator
      // Use <= 0 to handle ternary starting inside OC message brackets
      if (  (  pc2->Is(CT_SEMICOLON)
            || (  pc2->Is(CT_PAREN_CLOSE)
               && (pc_question->GetLevel() == pc2->GetLevel() + 1))
            || pc2->Is(CT_COMMA))
         && square_bracket_depth <= 0)
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
         // Test #51007: After finding our ternary's colon, we should only recurse
         // into a nested ternary if there was no OC selector colon in between.
         // In true nested ternary: "a ? b : c ? d : e" - the second ? comes right after E3
         // In OC sibling ternary: "[obj sel1:a ? b : c sel2:d ? e : f]" - there's "sel2:" between
         // If we've seen a colon after our ternary's colon, it's likely an OC selector
         // and the new ? is a sibling ternary, not nested.
         if (colon_found && colon_after_colon_found)
         {
            // Test #51008: This ? is a sibling ternary in an OC message, not nested.
            // We still need to process it recursively so it gets properly marked,
            // but we pass is_sibling_ternary = true so it doesn't mark subsequent
            // OC selector colons as CT_COND_COLON.
            LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '%s' (sibling ternary)\n",
                    __func__, __LINE__, pc2->GetOrigLine(), pc2->GetOrigCol(), pc2->GetLevel(), pc2->Text());
            pc2 = search_for_colon(pc2, depth + 1, true);  // is_sibling_ternary = true
            LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '%s'\n",
                    __func__, __LINE__, pc2->GetOrigLine(), pc2->GetOrigCol(), pc2->GetLevel(), pc2->Text());
            continue;
         }
         else
         {
            LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '%s'\n",
                    __func__, __LINE__, pc2->GetOrigLine(), pc2->GetOrigCol(), pc2->GetLevel(), pc2->Text());

            // Test #51011: Check if this is an elvis operator (?:) - the next token after ? is :
            // If so, we need to mark the elvis colon and continue searching for the outer ternary's colon.
            Chunk *inner_question = pc2;
            Chunk *next_after_q   = pc2->GetNextNcNnl();

            if (next_after_q->Is(CT_COLON))
            {
               // This is an elvis operator (?:). Mark the colon as CT_COND_COLON
               // but do NOT set colon_found for the outer ternary - the outer ternary
               // still needs its own colon.
               LOG_FMT(LCOMBINE, "%s(%d): Test #51011: Elvis operator found at line %zu col %zu, marking colon and continuing search\n",
                       __func__, __LINE__, inner_question->GetOrigLine(), inner_question->GetOrigCol());
               next_after_q->SetType(CT_COND_COLON);
               next_after_q->SetParent(inner_question);
               inner_question->SetParent(next_after_q);
               pc2 = next_after_q;
               // Continue to next token - we still need to find the outer ternary's colon
            }
            else
            {
               // Regular nested ternary - recurse
               pc2 = search_for_colon(pc2, depth + 1);
               LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '%s'\n",
                       __func__, __LINE__, pc2->GetOrigLine(), pc2->GetOrigCol(), pc2->GetLevel(), pc2->Text());
               continue;
            }
         }
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
            return(pc2);
         }
      }
      else if (  pc2->Is(CT_COLON)
              && square_bracket_depth <= 0
              && brace_depth <= 0)  // Test #51012: Don't mark dictionary colons as ternary colons
      {
         LOG_FMT(LCOMBINE, "%s(%d): orig line is %zu, orig col is %zu, level is %zu, Text() is '%s'\n",
                 __func__, __LINE__, pc2->GetOrigLine(), pc2->GetOrigCol(), pc2->GetLevel(), pc2->Text());

         // Test #51009: If square_bracket_depth is negative, we've exited more brackets
         // than we entered during this ternary search. This means we've left the ternary's
         // enclosing OC message, and this colon is an OC selector colon, not a ternary colon.
         // We should terminate here without marking the colon.
         if (square_bracket_depth < 0)
         {
            Chunk *prev_tok = pc2->GetPrevNcNnl();
            pc_question->SetParent(prev_tok);
            return(prev_tok);
         }

         // Test #51010: After finding our ternary colon, check if this new colon is an
         // OC message selector colon (pattern: WORD followed by COLON). If so, we've
         // reached the next OC message parameter and should terminate.
         if (colon_found)
         {
            Chunk *prev = pc2->GetPrevNcNnl();

            if (prev->Is(CT_WORD) || prev->Is(CT_TYPE) || prev->Is(CT_OC_MSG_NAME))
            {
               // This looks like an OC selector: "selectorName:" pattern
               // We should NOT mark this as a ternary colon; terminate the search.
               LOG_FMT(LCOMBINE, "%s(%d): Test #5100: Found OC selector colon after ternary, terminating at line %zu col %zu\n",
                       __func__, __LINE__, pc2->GetOrigLine(), pc2->GetOrigCol());
               Chunk *prev_tok = pc2->GetPrevNcNnl();
               pc_question->SetParent(prev_tok);
               return(prev_tok);
            }
         }

         if (colon_found && depth > 0 && !is_sibling_ternary)
         {
            // There can only be another CT_COND_COLON if there is more than 1 CT_QUESTION (ie. depth > 0)
            // BUT: If this is a sibling ternary (Test #51008), we should NOT mark subsequent colons
            // as CT_COND_COLON because they are OC selector colons, not nested ternary colons.
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
         else
         {
            // Test #51007: We've already found our ternary colon, and now we see another colon.
            // This is likely an OC selector colon (e.g., in "[obj sel1:val1 sel2:val2]")
            colon_after_colon_found = true;
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
      // Test #51012: Track OC dictionary @{...} brace depth
      // Check for CT_OC_AT preceding the brace to detect OC dictionary literals
      // Note: Parent type may not be set yet during search_for_colon, so check previous token
      else if (pc2->Is(CT_BRACE_OPEN))
      {
         Chunk *prev = pc2->GetPrevNcNnl();

         if (prev->Is(CT_OC_AT))
         {
            brace_depth++;
         }
      }
      else if (pc2->Is(CT_BRACE_CLOSE))
      {
         // If we're inside an OC dictionary, decrement brace depth
         if (brace_depth > 0)
         {
            brace_depth--;
         }
      }
      pc2 = pc2->GetNextNcNnl();
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
