/**
 * @file calculate_closing_brace_position.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "calculate_closing_brace_position.h"

#include "chunk.h"

using namespace uncrustify;


Chunk *calculate_closing_brace_position(const Chunk *cl_colon, Chunk *pc)
{
   LOG_FMT(LMCB, "%s(%d): cl_colon->Text() is '%s', orig line %zu, orig col is %zu, level is %zu\n",
           __func__, __LINE__, cl_colon->Text(), cl_colon->GetOrigLine(), cl_colon->GetOrigCol(), cl_colon->GetLevel());
   LOG_FMT(LMCB, "%s(%d): pc->Text()       is '%s', orig line %zu, orig col is %zu, level is %zu\n",
           __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel());
   // end of block is reached
   // look back over newline, preprocessor BUT NOT #endif

   // Issue #3058

   // examine going back the tokens: look for a "brace closing" or a "semi colon" until the colon
   // look back over comment, newline, preprocessor BUT NOT #endif

   size_t check_level = 0;

   if (pc->Is(CT_BRACE_CLOSE))
   {
      check_level = pc->GetLevel() + 1;
   }
   else
   {
      check_level = pc->GetLevel();
   }
   size_t erst_found      = 0;
   Chunk  *is_brace_close = Chunk::NullChunkPtr;
   Chunk  *is_semicolon   = Chunk::NullChunkPtr;
   Chunk  *is_comment     = Chunk::NullChunkPtr;
   Chunk  *back           = pc->GetPrevNnl();

   while (back->IsNotNullChunk())
   {
      if (back == cl_colon)
      {
         break;
      }

      if (erst_found != 0)
      {
         break;
      }

      if (back->GetLevel() == check_level)
      {
         if (back->IsBraceClose())
         {
            // brace_close found
            is_brace_close = back;
            LOG_FMT(LMCB, "%s(%d): BRACE_CLOSE: line is %zu, col is %zu, level is %zu\n",
                    __func__, __LINE__, is_brace_close->GetOrigLine(), is_brace_close->GetOrigCol(), is_brace_close->GetLevel());
            erst_found = 3;
         }

         if (back->Is(CT_SEMICOLON))
         {
            // semicolon found
            is_semicolon = back;
            LOG_FMT(LMCB, "%s(%d): SEMICOLON:   line is %zu, col is %zu, level is %zu\n",
                    __func__, __LINE__, is_semicolon->GetOrigLine(), is_semicolon->GetOrigCol(), is_semicolon->GetLevel());
            erst_found = 4;
         }

         if (back->IsComment())
         {
            // comment found
            is_comment = back;
            LOG_FMT(LMCB, "%s(%d): COMMENT:     line is %zu, col is %zu, level is %zu\n",
                    __func__, __LINE__, back->GetOrigLine(), back->GetOrigCol(), back->GetLevel());
         }
      }
      back = back->GetPrev();
   }
   LOG_FMT(LMCB, "%s(%d): erst_found is %zu\n",
           __func__, __LINE__, erst_found);
   Chunk *last = Chunk::NullChunkPtr;

   if (  erst_found == 3
      || erst_found == 4)
   {
      if (is_comment->IsNotNullChunk())
      {
         Chunk *second = Chunk::NullChunkPtr;

         if (erst_found == 3)
         {
            second = is_brace_close;
         }
         else
         {
            // erst_found == 4
            second = is_semicolon;
         }

         if (second->IsNotNullChunk())
         {
            if (is_comment->GetOrigLine() == second->GetOrigLine())
            {
               last = is_comment;

               if (cl_colon->GetOrigLine() == is_comment->GetOrigLine())
               {
                  last = is_comment->GetNext();
               }
            }
            else
            {
               last = pc->GetPrevNcNnl();
            }
         }
         else
         {
            LOG_FMT(LMCB, "\n\n%s(%d): FATAL: second is null chunk\n", __func__, __LINE__);
            fprintf(stderr, "FATAL: second is null chunk\n");
            fprintf(stderr, "Please make a report.\n");
            exit(EX_SOFTWARE);
         }
      }
      else
      {
         last = pc->GetPrevNcNnl();
      }
   }
   else
   {
      LOG_FMT(LMCB, "\n\n%s(%d): FATAL: erst_found is not 3 or 4\n", __func__, __LINE__);
      fprintf(stderr, "FATAL: erst_found is not 3 or 4\n");
      fprintf(stderr, "Please make a report.\n");
      exit(EX_SOFTWARE);
   }

   if (last->Is(CT_COMMENT_CPP))         // Issue #3058
   {
      last = last->GetNext();
   }
   LOG_FMT(LMCB, "%s(%d): last->Text()     is '%s', orig line %zu, orig col is %zu\n",
           __func__, __LINE__, last->Text(), last->GetOrigLine(), last->GetOrigCol());

   if (last->IsPreproc())
   {
      // we have a preprocessor token
      while (last->IsNotNullChunk())
      {
         LOG_FMT(LMCB, "%s(%d): Text() is '%s', orig line %zu, orig col is %zu\n",
                 __func__, __LINE__, last->Text(), last->GetOrigLine(), last->GetOrigCol());

         if (last->Is(CT_PP_ENDIF))
         {
            // look for the parent and compare the positions
            Chunk *parent_last = last->GetParent();
            int   comp         = parent_last->ComparePosition(cl_colon);
            LOG_FMT(LMCB, "%s(%d): comp is %d\n",
                    __func__, __LINE__, comp);

            if (comp == -1)
            {
               // cl_colon is after parent_last ==>
               // the closing brace will be set before #endif
               Chunk *pp_start = last->GetPpStart();
               last = pp_start->GetPrevNnl();
               LOG_FMT(LMCB, "%s(%d): Text() is '%s', orig line %zu, orig col is %zu\n",
                       __func__, __LINE__, last->Text(), last->GetOrigLine(), last->GetOrigCol());
            }
            else if (comp == 1)
            {
               // cl_colon is before parent_last ==>
               // the closing brace will be set after #endif
               LOG_FMT(LMCB, "%s(%d): Text() is '%s', orig line %zu, orig col is %zu\n",
                       __func__, __LINE__, last->Text(), last->GetOrigLine(), last->GetOrigCol());
            }
            break;
         }
         last = last->GetPrevNcNnl();
         LOG_FMT(LMCB, "%s(%d): Text() is '%s', orig line %zu, orig col is %zu\n",
                 __func__, __LINE__, last->Text(), last->GetOrigLine(), last->GetOrigCol());

         if (!last->IsPreproc())
         {
            break;
         }
      }
   }
   return(last);
} // calculate_closing_brace_position
