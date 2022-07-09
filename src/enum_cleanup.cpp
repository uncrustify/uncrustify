/**
 * @file enum_cleanup.cpp
 * works on the last comma within enum
 *
 * @author  Guy Maurel Juli 2018
 * @license GPL v2+
 */

#include "enum_cleanup.h"

#include "log_rules.h"

constexpr static auto LCURRENT = LTOK;

using namespace uncrustify;


void enum_cleanup()
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
      if (  pc->GetParentType() == CT_ENUM
         && pc->Is(CT_BRACE_CLOSE))
      {
         LOG_FMT(LTOK, "%s(%d): orig_line is %zu, type is %s\n",
                 __func__, __LINE__, pc->orig_line, get_token_name(pc->type));
         Chunk *prev = pc->GetPrevNcNnl();                           // Issue #3604

         if (  prev != nullptr
            && prev->IsNotNullChunk())
         {
            if (prev->Is(CT_COMMA))
            {
               log_rule_B("mod_enum_last_comma");

               if (options::mod_enum_last_comma() == IARF_REMOVE)
               {
                  Chunk::Delete(prev);
               }
            }
            else
            {
               if (prev->Is(CT_BRACE_OPEN))                // Issue #2902
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
                     comma.SetType(CT_COMMA);
                     comma.orig_line = prev->orig_line;
                     comma.orig_col  = prev->orig_col + 1;
                     comma.nl_count  = 0;
                     comma.pp_level  = 0;
                     comma.flags     = PCF_NONE;
                     comma.str       = ",";

                     if (prev->Is(CT_PP_ENDIF))                // Issue #3604
                     {
                        prev = prev->GetPrevNcNnlNpp();
                     }

                     if (prev->Is(CT_COMMA))                   // Issue #3604
                     {
                        // nothing to do
                     }
                     else
                     {
                        comma.CopyAndAddAfter(prev);
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
