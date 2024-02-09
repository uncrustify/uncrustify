/**
 * @file parameter_pack_cleanup.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "tokenizer/parameter_pack_cleanup.h"

#include "chunk.h"
#include "logger.h"
#include "uncrustify.h"


void parameter_pack_cleanup()
{
   LOG_FUNC_ENTRY();

   Chunk *pc = Chunk::GetHead();

   while (pc->IsNotNullChunk())
   {
      LOG_CHUNK(LTOK, pc);

      // look for template
      if (pc->Is(CT_TEMPLATE))                 // Issue #3309
      {
         Chunk *template_end = pc->GetNextType(CT_SEMICOLON, pc->GetLevel());

         // look for a parameter pack
         while (pc->IsNotNullChunk())
         {
            LOG_CHUNK(LTOK, pc);

            if (pc->Is(CT_PARAMETER_PACK))
            {
               Chunk *parameter_pack = pc;

               // look for a token with the same text
               while (pc->IsNotNullChunk())
               {
                  LOG_CHUNK(LTOK, pc);

                  if (pc == template_end)
                  {
                     break;
                  }

                  if (strcmp(pc->Text(), parameter_pack->Text()) == 0)
                  {
                     pc->SetType(CT_PARAMETER_PACK);
                  }
                  pc = pc->GetNext();
               }
            }
            pc = pc->GetNext();

            if (pc == template_end)
            {
               break;
            }
         }
      }
      pc = pc->GetNext();
   }
} // parameter_pack_cleanup
