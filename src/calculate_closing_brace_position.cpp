/**
 * @file calculate_closing_brace_position.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "calculate_closing_brace_position.h"

#include "chunk_list.h"

using namespace uncrustify;


chunk_t *calculate_closing_brace_position(const chunk_t *cl_colon, chunk_t *pc)
{
   LOG_FMT(LMCB, "%s(%d): cl_colon->text() is '%s', orig_line %zu, orig_col is %zu, level is %zu\n",
           __func__, __LINE__, cl_colon->text(), cl_colon->orig_line, cl_colon->orig_col, cl_colon->level);
   LOG_FMT(LMCB, "%s(%d): pc->text()       is '%s', orig_line %zu, orig_col is %zu, level is %zu\n",
           __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col, pc->level);
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
   size_t  erst_found      = 0;
   chunk_t *is_brace_close = nullptr;
   chunk_t *is_semicolon   = nullptr;
   chunk_t *is_comment     = nullptr;
   chunk_t *back           = chunk_get_prev_nnl(pc);

   while (back != nullptr)
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

         if (chunk_is_comment(back))
         {
            // comment found
            is_comment = back;
            LOG_FMT(LMCB, "%s(%d): COMMENT:     line is %zu, col is %zu, level is %zu\n",
                    __func__, __LINE__, back->orig_line, back->orig_col, back->level);
         }
      }
      back = chunk_get_prev(back);
   }
   LOG_FMT(LMCB, "%s(%d): erst_found is %zu\n",
           __func__, __LINE__, erst_found);
   chunk_t *last = nullptr;

   if (  erst_found == 3
      || erst_found == 4)
   {
      if (is_comment != nullptr)
      {
         chunk_t *second = nullptr;

         if (erst_found == 3)
         {
            second = is_brace_close;
         }
         else
         {
            // erst_found == 4
            second = is_semicolon;
         }

         if (second != nullptr)
         {
            if (is_comment->orig_line == second->orig_line)
            {
               last = is_comment;

               if (cl_colon->orig_line == is_comment->orig_line)
               {
                  last = chunk_get_next(is_comment);
               }
            }
            else
            {
               last = chunk_get_prev_ncnnl(pc);
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
         last = chunk_get_prev_ncnnl(pc);
      }
   }
   else
   {
      LOG_FMT(LMCB, "\n\n%s(%d):\n", __func__, __LINE__);
      fprintf(stderr, "FATAL: erst_found is not 3 or 4\n");
      fprintf(stderr, "Please make a report.\n");
      exit(EX_SOFTWARE);
   }
   LOG_FMT(LMCB, "%s(%d): last->text()     is '%s', orig_line %zu, orig_col is %zu\n",
           __func__, __LINE__, last->text(), last->orig_line, last->orig_col);

   if (chunk_is_preproc(last))
   {
      // we have a preprocessor token
      while (last != nullptr)
      {
         LOG_FMT(LMCB, "%s(%d): text() is '%s', orig_line %zu, orig_col is %zu\n",
                 __func__, __LINE__, last->text(), last->orig_line, last->orig_col);

         if (chunk_is_token(last, CT_PP_ENDIF))
         {
            // look for the parent
            chunk_t *parent_last = last->parent;
            // compare the positions
            int     comp = chunk_compare_position(parent_last, cl_colon);
            LOG_FMT(LMCB, "%s(%d): comp is %d\n",
                    __func__, __LINE__, comp);

            if (comp == -1)
            {
               // cl_colon is after parent_last ==>
               // the closing brace will be set before #endif
               chunk_t *pp_start = chunk_get_pp_start(last);
               last = chunk_get_prev_nnl(pp_start);
               LOG_FMT(LMCB, "%s(%d): text() is '%s', orig_line %zu, orig_col is %zu\n",
                       __func__, __LINE__, last->text(), last->orig_line, last->orig_col);
            }
            else if (comp == 1)
            {
               // cl_colon is before parent_last ==>
               // the closing brace will be set after #endif
               LOG_FMT(LMCB, "%s(%d): text() is '%s', orig_line %zu, orig_col is %zu\n",
                       __func__, __LINE__, last->text(), last->orig_line, last->orig_col);
            }
            break;
         }
         last = chunk_get_prev_ncnnl(last);
         LOG_FMT(LMCB, "%s(%d): text() is '%s', orig_line %zu, orig_col is %zu\n",
                 __func__, __LINE__, last->text(), last->orig_line, last->orig_col);

         if (!chunk_is_preproc(last))
         {
            break;
         }
      }
   }
   return(last);
} // calculate_closing_brace_position
