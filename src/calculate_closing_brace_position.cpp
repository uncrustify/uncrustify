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
   LOG_FMT(LMCB, "%s(%d): cl_colon->Text() is '%s', orig_line %zu, orig_col is %zu, level is %zu\n",
           __func__, __LINE__, cl_colon->Text(), cl_colon->orig_line, cl_colon->orig_col, cl_colon->level);
   LOG_FMT(LMCB, "%s(%d): pc->Text()       is '%s', orig_line %zu, orig_col is %zu, level is %zu\n",
           __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col, pc->level);
   // end of block is reached
   // look back over newline, preprocessor BUT NOT #endif

   // Issue #3058

   // examine going back the tokens: look for a "brace closing" or a "semi colon" until the colon
   // look back over comment, newline, preprocessor BUT NOT #endif

   size_t check_level = 0;

   if (chunk_is_token(pc, CT_BRACE_CLOSE))
   {
      check_level = pc->level + 1;
   }
   else
   {
      check_level = pc->level;
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

      if (back->level == check_level)
      {
         if (  chunk_is_token(back, CT_BRACE_CLOSE)
            || chunk_is_token(back, CT_VBRACE_CLOSE))
         {
            // brace_close found
            is_brace_close = back;
            LOG_FMT(LMCB, "%s(%d): BRACE_CLOSE: line is %zu, col is %zu, level is %zu\n",
                    __func__, __LINE__, is_brace_close->orig_line, is_brace_close->orig_col, is_brace_close->level);
            erst_found = 3;
         }

         if (chunk_is_token(back, CT_SEMICOLON))
         {
            // semicolon found
            is_semicolon = back;
            LOG_FMT(LMCB, "%s(%d): SEMICOLON:   line is %zu, col is %zu, level is %zu\n",
                    __func__, __LINE__, is_semicolon->orig_line, is_semicolon->orig_col, is_semicolon->level);
            erst_found = 4;
         }

         if (back->IsComment())
         {
            // comment found
            is_comment = back;
            LOG_FMT(LMCB, "%s(%d): COMMENT:     line is %zu, col is %zu, level is %zu\n",
                    __func__, __LINE__, back->orig_line, back->orig_col, back->level);
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
            if (is_comment->orig_line == second->orig_line)
            {
               last = is_comment;

               if (cl_colon->orig_line == is_comment->orig_line)
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
            LOG_FMT(LMCB, "\n\n%s(%d):\n", __func__, __LINE__);
            fprintf(stderr, "FATAL: second is nullptr\n");
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
      LOG_FMT(LMCB, "\n\n%s(%d):\n", __func__, __LINE__);
      fprintf(stderr, "FATAL: erst_found is not 3 or 4\n");
      fprintf(stderr, "Please make a report.\n");
      exit(EX_SOFTWARE);
   }

   if (chunk_is_token(last, CT_COMMENT_CPP))         // Issue #3058
   {
      last = last->GetNext();
   }
   LOG_FMT(LMCB, "%s(%d): last->Text()     is '%s', orig_line %zu, orig_col is %zu\n",
           __func__, __LINE__, last->Text(), last->orig_line, last->orig_col);

   if (last->IsPreproc())
   {
      // we have a preprocessor token
      while (last->IsNotNullChunk())
      {
         LOG_FMT(LMCB, "%s(%d): Text() is '%s', orig_line %zu, orig_col is %zu\n",
                 __func__, __LINE__, last->Text(), last->orig_line, last->orig_col);

         if (chunk_is_token(last, CT_PP_ENDIF))
         {
            // look for the parent
            Chunk *parent_last = last->parent;
            // compare the positions
            int   comp = chunk_compare_position(parent_last, cl_colon);
            LOG_FMT(LMCB, "%s(%d): comp is %d\n",
                    __func__, __LINE__, comp);

            if (comp == -1)
            {
               // cl_colon is after parent_last ==>
               // the closing brace will be set before #endif
               Chunk *pp_start = chunk_get_pp_start(last);
               last = pp_start->GetPrevNnl();
               LOG_FMT(LMCB, "%s(%d): Text() is '%s', orig_line %zu, orig_col is %zu\n",
                       __func__, __LINE__, last->Text(), last->orig_line, last->orig_col);
            }
            else if (comp == 1)
            {
               // cl_colon is before parent_last ==>
               // the closing brace will be set after #endif
               LOG_FMT(LMCB, "%s(%d): Text() is '%s', orig_line %zu, orig_col is %zu\n",
                       __func__, __LINE__, last->Text(), last->orig_line, last->orig_col);
            }
            break;
         }
         last = last->GetPrevNcNnl();
         LOG_FMT(LMCB, "%s(%d): Text() is '%s', orig_line %zu, orig_col is %zu\n",
                 __func__, __LINE__, last->Text(), last->orig_line, last->orig_col);

         if (!last->IsPreproc())
         {
            break;
         }
      }
   }
   return(last);
} // calculate_closing_brace_position
