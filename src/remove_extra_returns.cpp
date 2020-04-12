/**
 * @file remove_extra_returns.cpp
 *
 * @author  Guy Maurel
 *          October 2015, 2016
 * @license GPL v2+
 * extract from combine.cpp
 */

#include "remove_extra_returns.h"

#include "uncrustify.h"


void remove_extra_returns(void)
{
   LOG_FUNC_ENTRY();

   chunk_t *pc = chunk_get_head();

   while (pc != nullptr)
   {
      LOG_FMT(LRMRETURN, "%s(%d): orig_line is %zu, orig_col is %zu, text() is '%s', type is %s, parent_type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(),
              get_token_name(pc->type), get_token_name(pc->parent_type));

      if (  chunk_is_token(pc, CT_RETURN)
         && !pc->flags.test(PCF_IN_PREPROC))
      {
         // we might be in a class, check it                                     Issue #2705
         // look for a closing brace
         bool    remove_it      = false;
         chunk_t *closing_brace = chunk_get_next_type(pc, CT_BRACE_CLOSE, 1);

         if (closing_brace != nullptr)
         {
            if (get_chunk_parent_type(closing_brace) == CT_FUNC_CLASS_DEF)
            {
               // we have a class. Do nothing
            }
            else if (get_chunk_parent_type(closing_brace) == CT_FUNC_DEF)
            {
               remove_it = true;
            }
         }
         else
         {
            // it is not a class
            // look for a closing brace
            closing_brace = chunk_get_next_type(pc, CT_BRACE_CLOSE, 0);

            if (closing_brace != nullptr)
            {
               if (get_chunk_parent_type(closing_brace) == CT_FUNC_DEF)
               {
                  remove_it = true;
               }
            }
         }

         if (remove_it)
         {
            chunk_t *semicolon = chunk_get_next_ncnl(pc);

            if (  semicolon != nullptr
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
      pc = chunk_get_next(pc);
   }
} // remove_extra_returns
