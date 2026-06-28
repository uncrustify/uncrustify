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
#include <vector> // Added this line

using std::vector;


void remove_duplicate_include()
{
   LOG_FUNC_ENTRY();

   vector<Chunk *> includes;

   Chunk           *preproc = Chunk::NullChunkPtr;
   Chunk           *pc      = Chunk::GetHead();

   while (pc->IsNotNullChunk())
   {
      //LOG_FMT(LRMRETURN, "%s(%d): orig line is %zu, orig col is %zu, text is '%s', type is %s, parent type is %s\n",
      //        __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLogText(),
      //        get_token_name(pc->GetType()), get_token_name(pc->GetParentType()));

      if (pc->Is(E_Token::CT_PREPROC))
      {
         preproc = pc;
         pc      = pc->GetNext();
      } // if (pc->Is(E_Token::CT_PREPROC))
      else
      {
         if (pc->Is(E_Token::CT_PP_INCLUDE))
         {
            Chunk *next = pc->GetNext();

            //LOG_FMT(LRMRETURN, "%s(%d): orig line is %zu, orig col is %zu, text is '%s', type is %s, parent type is %s\n",
            //        __func__, __LINE__, next->GetOrigLine(), next->GetOrigCol(), next->GetLogText(),
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
               auto itc = includes.begin();

               for ( ; itc != ite; ++itc)
               {
                  Chunk const *current = *itc;

                  //LOG_FMT(LRMRETURN, "%s(%d): next->text    is '%s'\n",
                  //        __func__, __LINE__, next->pc->GetLogText());
                  //LOG_FMT(LRMRETURN, "%s(%d): current->text is '%s'\n",
                  //        __func__, __LINE__, current->pc->GetLogText());

                  if (std::strcmp(next->GetLogText(), current->GetLogText()) == 0)
                  {
                     break;
                  }
               } // for (auto itc = includes.begin();

               // found nothing
               if (ite == itc)
               {
                  includes.push_back(next);
                  pc = pc->GetNext();
               } // if (ite == itc)
               else
               {
                  // erase the statement, deletes all chunks on original line

                  std::size_t orig_line                  = preproc->GetOrigLine();
                  Chunk       *next_line_start_chunk_ptr = pc->GetNextNl()->GetNext();
                  Chunk       *inline_chunk_ptr          = preproc;

                  while (inline_chunk_ptr->IsNotNullChunk() && inline_chunk_ptr->GetOrigLine() == orig_line)
                  {
                     Chunk *inline_next = inline_chunk_ptr->GetNext();

                     Chunk::Delete(inline_chunk_ptr);

                     // next iter
                     inline_chunk_ptr = inline_next;
                  } // while

                  pc = next_line_start_chunk_ptr;
               } // else
            } // if (includes.empty())
         } // if (pc->Is(E_Token::CT_PP_INCLUDE))
         else
         {
            // get the next token
            pc = pc->GetNext();
         } // else
      } // else
   } // while
} // remove_duplicate_include
