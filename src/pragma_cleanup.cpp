/**
 * @file pragma_cleanup.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "pragma_cleanup.h"

#include "chunk.h"
#include "unc_tools.h"


void pragma_cleanup(void)
{
   LOG_FUNC_ENTRY();

   bool  preproc_found   = false;
   bool  pragma_found    = false;
   bool  parameter_found = false;

   Chunk *pc = Chunk::GetHead();

   while (pc->IsNotNullChunk())
   {
      LOG_FMT(LMCB, "%s(%d): orig_line is %zu, orig_col is %zu, Text is '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());

      if (!preproc_found)
      {
         if (chunk_is_token(pc, CT_PREPROC))
         {
            LOG_FMT(LMCB, "%s(%d): PREPROC found: orig_line %zu, orig_col is %zu\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col);
            log_pcf_flags(LMCB, pc->flags);
            preproc_found = true;
         }
      }
      else
      {
         if (!pragma_found)
         {
            if (chunk_is_token(pc, CT_PP_PRAGMA))
            {
               LOG_FMT(LMCB, "%s(%d): PP_PRAGMA found: orig_line %zu, orig_col is %zu\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col);
               log_pcf_flags(LMCB, pc->flags);
               pragma_found = true;
            }
         }
         else
         {
            if (!parameter_found)
            {
               LOG_FMT(LMCB, "%s(%d): PARAMETER found: orig_line %zu, orig_col is %zu, Text is '%s'\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
               log_pcf_flags(LMCB, pc->flags);

               if (strcmp(pc->Text(), "endasm") == 0)
               {
                  set_chunk_type(pc, CT_PP_ENDASM);
               }
               else if (strcmp(pc->Text(), "region") == 0)
               {
                  set_chunk_type(pc, CT_PP_REGION);
               }
               //else if (strcmp(pc->Text(), "comment") == 0)
               //{
               //   set_chunk_type(pc, CT_PP_COMMENT);
               //}
               else
               {
                  set_chunk_type(pc, CT_PP_PRAGMA);
               }
               parameter_found = true;
            }
            else
            {
               LOG_FMT(LMCB, "%s(%d): orig_line is %zu, orig_col is %zu, Text is '%s'\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());

               if (pc->IsNewline())
               {
                  // reset
                  preproc_found   = false;
                  pragma_found    = false;
                  parameter_found = false;
               }
               else
               {
                  set_chunk_type(pc, CT_PP_IGNORE);
               }
            }
         }
      }
      pc = pc->GetNext();
   }
   //prot_the_line(__func__, __LINE__, 3, 0);
} // pragma_cleanup
