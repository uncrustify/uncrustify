/**
 * @file enum_cleanup.cpp
 * works on the last comma withing enum
 *
 * @author  Guy Maurel Juli 2018
 * @license GPL v2+
 */

#include "enum_cleanup.h"

#include "log_rules.h"

constexpr static auto LCURRENT = LTOK;

using namespace uncrustify;


void enum_cleanup(void)
{
   LOG_FUNC_ENTRY();

   log_rule_B("mod_enum_last_comma");

   if (options::mod_enum_last_comma() == IARF_IGNORE)
   {
      // nothing to do
      return;
   }
   Chunk *pc = Chunk::GetHead();  // Issue #858

   while (pc->IsNotNullChunk())
   {
      if (  get_chunk_parent_type(pc) == CT_ENUM
         && chunk_is_token(pc, CT_BRACE_CLOSE))
      {
         LOG_FMT(LTOK, "%s(%d): orig_line is %zu, type is %s\n",
                 __func__, __LINE__, pc->orig_line, get_token_name(pc->type));
         Chunk *prev = pc->GetPrevNcNnl();                           // Issue #3604

         if (  prev != nullptr
            && prev->IsNotNullChunk())
         {
            if (chunk_is_token(prev, CT_COMMA))
            {
               log_rule_B("mod_enum_last_comma");

               if (options::mod_enum_last_comma() == IARF_REMOVE)
               {
                  chunk_del(prev);
               }
            }
            else
            {
               if (chunk_is_token(prev, CT_BRACE_OPEN))                // Issue #2902
               {
                  // nothing between CT_BRACE_OPEN and CT_BRACE_CLOSE
               }
               else
               {
                  log_rule_B("mod_enum_last_comma");

                  if (  options::mod_enum_last_comma() == IARF_ADD
                     || options::mod_enum_last_comma() == IARF_FORCE)
                  {
                     // create a comma
                     Chunk comma;
                     set_chunk_type(&comma, CT_COMMA);
                     comma.orig_line = prev->orig_line;
                     comma.orig_col  = prev->orig_col + 1;
                     comma.nl_count  = 0;
                     comma.pp_level  = 0;
                     comma.flags     = PCF_NONE;
                     comma.str       = ",";

                     if (chunk_is_token(prev, CT_PP_ENDIF))                // Issue #3604
                     {
                        prev = prev->GetPrevNcNnlNpp();
                     }

                     if (chunk_is_token(prev, CT_COMMA))                   // Issue #3604
                     {
                        // nothing to do
                     }
                     else
                     {
                        chunk_add_after(&comma, prev);
                     }
                     pc = pc->GetNext();
                  }
               }
            }
         }
      }
      pc = pc->GetNext();
   }
} // enum_cleanup
