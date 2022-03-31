/**
 * @file mark_question_colon.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "mark_question_colon.h"

#include "combine_fix_mark.h"
#include "combine_skip.h"
#include "combine_tools.h"
#include "EnumStructUnionParser.h"
#include "flag_braced_init_list.h"
#include "flag_parens.h"
#include "lang_pawn.h"
#include "newlines.h"
#include "prototypes.h"
#include "tokenize_cleanup.h"

#include <limits>

constexpr static auto LCURRENT = LCOMBINE;

using namespace std;
using namespace uncrustify;


void mark_question_colon(void)
{
   LOG_FUNC_ENTRY();
   Chunk *pc = nullptr;
   Chunk *pc2 = nullptr;
   Chunk *firstQuestion = nullptr;

   // Issue #3558
   if (language_is_set(LANG_CPP))
   {
      for (pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
      {
         LOG_FMT(LFCNR, "%s(%d): orig_line is %zu, orig_col is %zu, level is %zu, Text() '%s'\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->level, pc->Text());
         log_pcf_flags(LFCNR, pc->flags);
         if (pc->flags.test(PCF_IN_PREPROC))
         {
            continue;
         }
         if (chunk_is_token(pc, CT_QUESTION))
         {
            if (firstQuestion == nullptr)
            {
               firstQuestion = pc;
            }
            // search the colon
            Chunk *cond_colon = chunk_get_next_type(pc, CT_COND_COLON, pc->level);
            Chunk *colon = chunk_get_next_type(pc, CT_COLON, pc->level);
            // choose
            if (cond_colon == nullptr)
            {
               // take colon
            }
            else
            {
               // take cond_colon
               colon = cond_colon;
            }
            if (colon != nullptr)
            {
               set_chunk_type(colon, CT_COND_COLON);
               // examine the next tokens, search for a next CT_QUESTION
               bool question_found = false;
               for (pc2 = colon; pc2->IsNotNullChunk(); pc2 = pc2->GetNextNcNnl())
               {
                  LOG_FMT(LFCNR, "%s(%d): THE NEXT: orig_line is %zu, orig_col is %zu, level is %zu, Text() '%s'\n",
                          __func__, __LINE__, pc2->orig_line, pc2->orig_col, pc2->level, pc2->Text());
                  log_pcf_flags(LFCNR, pc2->flags);
                  if (chunk_is_token(pc2, CT_SEMICOLON))
                  {
                     break;
                  }
                  else if (chunk_is_token(pc2, CT_QUESTION))
                  {
                     LOG_FMT(LFCNR, "%s(%d): orig_line is %zu, orig_col is %zu, level is %zu, Text() is ?\n",
                             __func__, __LINE__, pc2->orig_line, pc2->orig_col, pc2->level);
                     question_found = true;
                     break;
                  }
               }
               if (question_found)
               {
                  pc = pc2;
               }
               else
               {
                  // end of statement found
                  LOG_FMT(LFCNR, "%s(%d): End of statement found: orig_line is %zu, orig_col is %zu, level is %zu, Text() '%s'\n",
                          __func__, __LINE__, pc2->orig_line, pc2->orig_col, pc2->level, pc2->Text());
                  flag_series(firstQuestion, pc2, PCF_IN_CONDITIONAL);
               }
            }
            else
            {
               if (cond_colon != nullptr)
               {
                  LOG_FMT(LFCNR, "%s(%d): COND_COLON: orig_line is %zu, orig_col is %zu, level is %zu, Text() '%s'\n",
                          __func__, __LINE__, cond_colon->orig_line, cond_colon->orig_col, cond_colon->level, cond_colon->Text());
                  log_pcf_flags(LFCNR, cond_colon->flags);
               }
               LOG_FMT(LERR, "%s(%d): %zu: Error: Expected a colon\n",
                       __func__, __LINE__, pc->orig_line);
               cpd.error_count++;
               exit(EXIT_FAILURE);
            }
         }
      }
   }
} // mark_question_colon
