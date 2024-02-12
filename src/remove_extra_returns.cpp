/**
 * @file remove_extra_returns.cpp
 *
 * @author  Guy Maurel October 2015, 2016
 * @license GPL v2+
 */

#include "remove_extra_returns.h"

#include "chunk.h"
#include "uncrustify.h"


void remove_extra_returns()
{
   LOG_FUNC_ENTRY();

   Chunk *pc = Chunk::GetHead();

   while (pc->IsNotNullChunk())
   {
      LOG_FMT(LRMRETURN, "%s(%d): orig line is %zu, orig col is %zu, Text() is '%s', type is %s, parent type is %s\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(),
              get_token_name(pc->GetType()), get_token_name(pc->GetParentType()));

      if (  pc->Is(CT_RETURN)
         && !pc->TestFlags(PCF_IN_PREPROC))
      {
         // we might be in a class, check it                                     Issue #2705
         // look for a closing brace
         bool  remove_it      = false;
         Chunk *closing_brace = pc->GetNextType(CT_BRACE_CLOSE, 1);
         LOG_FMT(LRMRETURN, "%s(%d): on orig line %zu, level is %zu\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetLevel());

         if (closing_brace->IsNotNullChunk())
         {
            if (closing_brace->GetParentType() == CT_FUNC_CLASS_DEF)
            {
               // we have a class. Do nothing
            }
            else if (  closing_brace->GetParentType() == CT_FUNC_DEF
                    && pc->GetLevel() < 2)
            {
               remove_it = true;
            }
         }
         else
         {
            // it is not a class
            // look for a closing brace
            // make sure we are at level 1 because 'return' could
            // be part of nested 'if' blocks
            closing_brace = pc->GetNextType(CT_BRACE_CLOSE, 0);
            LOG_FMT(LRMRETURN, "%s(%d): on orig line %zu, level is %zu\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetLevel());

            if (closing_brace->IsNotNullChunk())
            {
               if (  closing_brace->GetParentType() == CT_FUNC_DEF
                  && pc->GetLevel() < 2)
               {
                  remove_it = true;
               }
            }
         }

         if (remove_it)
         {
            Chunk *semicolon = pc->GetNextNcNnl();

            if (  semicolon->IsNotNullChunk()
               && semicolon->Is(CT_SEMICOLON))
            {
               LOG_FMT(LRMRETURN, "%s(%d): Removed 'return;' on orig line %zu\n",
                       __func__, __LINE__, pc->GetOrigLine());
               Chunk::Delete(pc);
               Chunk::Delete(semicolon);
               pc = closing_brace;
            }
         }
      }
      pc = pc->GetNext();
   }
} // remove_extra_returns
