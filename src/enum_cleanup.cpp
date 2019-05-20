/**
 * @file enum_cleanup.cpp
 * works on the last comma withing enum
 *
 * @author  Guy Maurel Juli 2018
 * @license GPL v2+
 */

#include "enum_cleanup.h"

#include "chunk_list.h"
#include "logger.h"
#include "uncrustify.h"
#include "uncrustify_types.h"

using namespace uncrustify;


void enum_cleanup(void)
{
   LOG_FUNC_ENTRY();

   if (options::mod_enum_last_comma() == IARF_IGNORE)
   {
      // nothing to do
      return;
   }

   chunk_t *pc = chunk_get_head();  // Issue #858
   while (pc != nullptr)
   {
      if (  pc->parent_type == CT_ENUM
         && chunk_is_token(pc, CT_BRACE_CLOSE))
      {
         LOG_FMT(LTOK, "%s(%d): orig_line is %zu, type is %s\n",
                 __func__, __LINE__, pc->orig_line, get_token_name(pc->type));
         chunk_t *prev = chunk_get_prev_ncnlnp(pc);
         // test of (prev == nullptr) is not necessary
         if (chunk_is_token(prev, CT_COMMA))
         {
            if (options::mod_enum_last_comma() == IARF_REMOVE)
            {
               chunk_del(prev);
            }
         }
         else
         {
            if (  options::mod_enum_last_comma() == IARF_ADD
               || options::mod_enum_last_comma() == IARF_FORCE)
            {
               // create a comma
               chunk_t comma;
               comma.orig_line = prev->orig_line;
               comma.orig_col  = prev->orig_col + 1;
               comma.nl_count  = 0;
               comma.flags     = 0;
               comma.type      = CT_COMMA;
               comma.str       = ",";
               chunk_add_after(&comma, prev);
            }
         }
      }
      pc = chunk_get_next(pc);
   }
} // enum_cleanup
