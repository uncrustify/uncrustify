/**
 * @file log_rules.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "log_rules.h"
#include <string.h>


// is an extract from space.cpp
void log_rule2(const char *func, size_t line, const char *rule, chunk_t *first, chunk_t *second)
{
   LOG_FUNC_ENTRY();

   if (second->type != CT_NEWLINE)
   {
      LOG_FMT(LSPACE, "%s(%zu): first->orig_line is %zu, first->orig_col is %zu, first->text() is '%s', [%s/%s] <===>\n",
              func, line, first->orig_line, first->orig_col, first->text(),
              get_token_name(first->type), get_token_name(get_chunk_parent_type(first)));
      LOG_FMT(LSPACE, "   second->orig_line is %zu, second->orig_col is %zu, second->text() '%s', [%s/%s] : rule %s[line %zu]\n",
              second->orig_line, second->orig_col, second->text(),
              get_token_name(second->type), get_token_name(get_chunk_parent_type(second)),
              rule, line);
   }
}


void log_rule3(const char *func, size_t line, const char *rule)
{
   // some Windows provide "ABC::function_Name" as __func__
   // we look for the last ':' character
   const char *where = rindex(func, ':');

   if (where == nullptr)
   {
      LOG_FMT(LSPACE, "log_rule(%s): rule is '%s' at line %zu\n",
              func, rule, line);
   }
   else
   {
      LOG_FMT(LSPACE, "log_rule(%s): rule is '%s' at line %zu\n",
              where + 1, rule, line);
   }
}
