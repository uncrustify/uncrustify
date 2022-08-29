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


void remove_duplicate_include()
{
   LOG_FUNC_ENTRY();

   vector<Chunk *> includes;

   Chunk           *preproc = Chunk::NullChunkPtr;
   Chunk           *pc      = Chunk::GetHead();

   while (pc->IsNotNullChunk())
   {
      //LOG_FMT(LRMRETURN, "%s(%d): orig line is %zu, orig col is %zu, Text() is '%s', type is %s, parent type is %s\n",
      //        __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(),
      //        get_token_name(pc->GetType()), get_token_name(pc->GetParentType()));

      if (pc->Is(CT_PREPROC))
      {
         preproc = pc;
      }
      else if (pc->Is(CT_PP_INCLUDE))
      {
         Chunk *next = pc->GetNext();

         //LOG_FMT(LRMRETURN, "%s(%d): orig line is %zu, orig col is %zu, Text() is '%s', type is %s, parent type is %s\n",
         //        __func__, __LINE__, next->GetOrigLine(), next->GetOrigCol(), next->Text(),
         //        get_token_name(next->GetType()), get_token_name(next->GetParentType()));
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
                  Chunk::Delete(preproc);
                  Chunk::Delete(temp);
                  Chunk::Delete(next);

                  if (comment != eol)
                  {
                     Chunk::Delete(comment);
                  }
                  Chunk::Delete(eol);
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
      } // else if (pc->Is(CT_PP_INCLUDE))
      // get the next token
      pc = pc->GetNext();
   }
} // remove_duplicate_include
