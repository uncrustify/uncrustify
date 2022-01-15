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
   Chunk *pc = chunk_get_head();  // Issue #858

   while (pc->isNotNullChunk())
   {
      if (  get_chunk_parent_type(pc) == CT_ENUM
         && chunk_is_token(pc, CT_BRACE_CLOSE))
      {
         LOG_FMT(LTOK, "%s(%d): orig_line is %zu, type is %s\n",
                 __func__, __LINE__, pc->orig_line, get_token_name(pc->type));
         Chunk *prev = chunk_get_prev_nc_nnl_np(pc);

         // test of (prev == nullptr) is not necessary
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
               // nothing betwen CT_BRACE_OPEN and CT_BRACE_CLOSE
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
                  chunk_add_after(&comma, prev);
               }
            }
         }
      }
      pc = pc->get_next();
   }
} // enum_cleanup
