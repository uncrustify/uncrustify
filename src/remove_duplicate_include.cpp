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

   vector<chunk_t *> includes;

   chunk_t           *preproc = nullptr;
   chunk_t           *pc      = chunk_get_head();

   while (pc != nullptr)
   {
      //LOG_FMT(LRMRETURN, "%s(%d): orig_line is %zu, orig_col is %zu, text() is '%s', type is %s, parent_type is %s\n",
      //        __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(),
      //        get_token_name(pc->type), get_token_name(pc->parent_type));

      if (chunk_is_token(pc, CT_PREPROC))
      {
         preproc = pc;
      }
      else if (chunk_is_token(pc, CT_PP_INCLUDE))
      {
         chunk_t *next = chunk_get_next(pc);

         //LOG_FMT(LRMRETURN, "%s(%d): orig_line is %zu, orig_col is %zu, text() is '%s', type is %s, parent_type is %s\n",
         //        __func__, __LINE__, next->orig_line, next->orig_col, next->text(),
         //        get_token_name(next->type), get_token_name(next->parent_type));
         if (includes.empty())
         {
            includes.push_back(next);
            // goto next newline
            pc = chunk_get_next_nl(next);
         }
         else
         {
            //LOG_FMT(LRMRETURN, "%s(%d): size is %zu\n",
            //        __func__, __LINE__, includes.size());
            // look for duplicate
            auto ite = includes.end();

            for (auto itc = includes.begin(); itc != ite; ++itc)
            {
               chunk_t *current = *itc;

               //LOG_FMT(LRMRETURN, "%s(%d): next->text()    is '%s'\n",
               //        __func__, __LINE__, next->text());
               //LOG_FMT(LRMRETURN, "%s(%d): current->text() is '%s'\n",
               //        __func__, __LINE__, current->text());
               if (std::strcmp(next->text(), current->text()) == 0)
               {
                  // erase the statement
                  chunk_t *temp    = pc;
                  chunk_t *comment = chunk_get_next(next);
                  chunk_t *eol     = chunk_get_next_nl(next);
                  pc = chunk_get_prev(preproc);
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
                  pc = chunk_get_next_nl(next);
                  // and still look for duplicate
               }
            } // for (auto itc = includes.begin();
         } // if (includes.empty())
      } // else if (chunk_is_token(pc, CT_PP_INCLUDE))
      // get the next token
      pc = chunk_get_next(pc);
   }
} // remove_duplicate_include
