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
   // end of block is reached
   // look back over comment, newline, preprocessor BUT NOT #endif
   chunk_t *last = chunk_get_prev_ncnnl(pc);

   LOG_FMT(LMCB, "%s(%d): text() is '%s', orig_line %zu, orig_col is %zu\n",
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
