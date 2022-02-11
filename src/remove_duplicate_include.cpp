/**
 * @file remove_duplicate_include.cpp
 *
 * @author  Guy Maurel
 *          October 2015, 2016
 * @license GPL v2+
 */

#include "remove_duplicate_include.h"

#include "chunk.h"
#include "uncrustify.h"

using std::vector;


void remove_duplicate_include(void)
{
   LOG_FUNC_ENTRY();

   vector<Chunk *> includes;

   Chunk           *preproc = Chunk::NullChunkPtr;
   Chunk           *pc      = Chunk::GetHead();

   while (pc->IsNotNullChunk())
   {
      //LOG_FMT(LRMRETURN, "%s(%d): orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s, parent_type is %s\n",
      //        __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(),
      //        get_token_name(pc->type), get_token_name(pc->parent_type));

      if (chunk_is_token(pc, CT_PREPROC))
      {
         preproc = pc;
      }
      else if (chunk_is_token(pc, CT_PP_INCLUDE))
      {
         Chunk *next = pc->GetNext();

         //LOG_FMT(LRMRETURN, "%s(%d): orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s, parent_type is %s\n",
         //        __func__, __LINE__, next->orig_line, next->orig_col, next->Text(),
         //        get_token_name(next->type), get_token_name(next->parent_type));
         if (includes.empty())
         {
            includes.push_back(next);
            // goto next newline
            pc = next->GetNextNl();
         }
         else
         {
            //LOG_FMT(LRMRETURN, "%s(%d): size is %zu\n",
            //        __func__, __LINE__, includes.size());
            // look for duplicate
            auto ite = includes.end();

            for (auto itc = includes.begin(); itc != ite; ++itc)
            {
               Chunk *current = *itc;

               //LOG_FMT(LRMRETURN, "%s(%d): next->Text()    is '%s'\n",
               //        __func__, __LINE__, next->Text());
               //LOG_FMT(LRMRETURN, "%s(%d): current->Text() is '%s'\n",
               //        __func__, __LINE__, current->Text());
               if (std::strcmp(next->Text(), current->Text()) == 0)
               {
                  // erase the statement
                  Chunk *temp    = pc;
                  Chunk *comment = next->GetNext();
                  Chunk *eol     = next->GetNextNl();
                  pc = preproc->GetPrev();
                  chunk_del(preproc);
                  chunk_del(temp);
                  chunk_del(next);

                  if (comment != eol)
                  {
                     chunk_del(comment);
                  }
                  chunk_del(eol);
                  break;
               }
               else
               {
                  // goto next newline
                  pc = next->GetNextNl();
                  // and still look for duplicate
               }
            } // for (auto itc = includes.begin();
         } // if (includes.empty())
      } // else if (chunk_is_token(pc, CT_PP_INCLUDE))
      // get the next token
      pc = pc->GetNext();
   }
} // remove_duplicate_include
