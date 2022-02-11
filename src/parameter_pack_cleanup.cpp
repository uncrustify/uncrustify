/**
 * @file parameter_pack_cleanup.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "parameter_pack_cleanup.h"

#include "chunk.h"


void parameter_pack_cleanup(void)
{
   LOG_FUNC_ENTRY();

   Chunk *pc = Chunk::get_head();

   while (  pc != nullptr
         && pc->IsNotNullChunk())
   {
      LOG_FMT(LTOK, "%s(%d): orig_line is %zu, orig_col is %zu, text() is '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());

      // look for template
      if (chunk_is_token(pc, CT_TEMPLATE))                 // Issue #3309
      {
         Chunk *template_end = chunk_get_next_type(pc, CT_SEMICOLON, pc->level);

         // look for a parameter pack
         while (  pc != nullptr
               && pc->IsNotNullChunk())
         {
            LOG_FMT(LTOK, "%s(%d): orig_line is %zu, orig_col is %zu, text() is '%s'\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());

            if (chunk_is_token(pc, CT_PARAMETER_PACK))
            {
               Chunk *parameter_pack = pc;

               // look for a token with the same text
               while (  pc != nullptr
                     && pc->IsNotNullChunk())
               {
                  LOG_FMT(LTOK, "%s(%d): orig_line is %zu, orig_col is %zu, text() is '%s'\n",
                          __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
                  //pc = pc->get_next();

                  if (pc == template_end)
                  {
                     break;
                  }

                  if (strcmp(pc->text(), parameter_pack->text()) == 0)
                  {
                     set_chunk_type(pc, CT_PARAMETER_PACK);
                  }
                  pc = pc->get_next();
               }
            }
            pc = pc->get_next();

            if (pc == template_end)
            {
               break;
            }
         }
      }
      pc = pc->get_next();
   }
} // parameter_pack_cleanup
