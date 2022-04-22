/**
 * @file remove_extra_returns.cpp
 *
 * @author  Guy Maurel
 *          October 2015, 2016
 * @license GPL v2+
 * extract from combine.cpp
 */

#include "remove_extra_returns.h"

#include "chunk.h"
#include "uncrustify.h"


void remove_extra_returns(void)
{
   LOG_FUNC_ENTRY();

   Chunk *pc = Chunk::GetHead();

   while (pc->IsNotNullChunk())
   {
      LOG_FMT(LRMRETURN, "%s(%d): orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s, parent_type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(),
              get_token_name(pc->type), get_token_name(pc->parent_type));

      if (  chunk_is_token(pc, CT_RETURN)
         && !pc->flags.test(PCF_IN_PREPROC))
      {
         // we might be in a class, check it                                     Issue #2705
         // look for a closing brace
         bool  remove_it      = false;
         Chunk *closing_brace = pc->GetNextType(CT_BRACE_CLOSE, 1);
         LOG_FMT(LRMRETURN, "%s(%d): on orig_line %zu, level is %zu\n",
                 __func__, __LINE__, pc->orig_line, pc->level);

         if (closing_brace->IsNotNullChunk())
         {
            if (get_chunk_parent_type(closing_brace) == CT_FUNC_CLASS_DEF)
            {
               // we have a class. Do nothing
            }
            else if (  get_chunk_parent_type(closing_brace) == CT_FUNC_DEF
                    && pc->level < 2)
            {
               remove_it = true;
            }
         }
         else
         {
            // it is not a class
            // look for a closing brace
            closing_brace = pc->GetNextType(CT_BRACE_CLOSE, 0);
            LOG_FMT(LRMRETURN, "%s(%d): on orig_line %zu, level is %zu\n",
                    __func__, __LINE__, pc->orig_line, pc->level);

            if (closing_brace->IsNotNullChunk())
            {
               if (get_chunk_parent_type(closing_brace) == CT_FUNC_DEF)
               {
                  remove_it = true;
               }
            }
         }

         if (remove_it)
         {
            Chunk *semicolon = pc->GetNextNcNnl();

            if (  semicolon->IsNotNullChunk()
               && chunk_is_token(semicolon, CT_SEMICOLON))
            {
               LOG_FMT(LRMRETURN, "%s(%d): Removed 'return;' on orig_line %zu\n",
                       __func__, __LINE__, pc->orig_line);
               chunk_del(pc);
               chunk_del(semicolon);
               pc = closing_brace;
            }
         }
      }
      pc = pc->GetNext();
   }
} // remove_extra_returns
