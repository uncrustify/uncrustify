/**
 * @file log_rules.cpp
 * is an extract from space.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "log_rules.h"

#include "unc_tools.h"


void log_rule2(const char *func, size_t line, const char *rule, Chunk *first, Chunk *second)
{
   LOG_FUNC_ENTRY();

   if (second->type != CT_NEWLINE)
   {
      LOG_FMT(LSPACE, "%s(%zu): first->orig_line is %zu, first->orig_col is %zu, first->Text() is '%s', [%s/%s] <===>\n",
              func, line, first->orig_line, first->orig_col, first->Text(),
              get_token_name(first->type), get_token_name(get_chunk_parent_type(first)));
      LOG_FMT(LSPACE, "           second->orig_line is %zu, second->orig_col is %zu, second->Text() is '%s', [%s/%s] :",
              second->orig_line, second->orig_col, second->Text(),
              get_token_name(second->type), get_token_name(get_chunk_parent_type(second)));
      LOG_FMT(LSPACE, " rule %s[line %zu]\n",
              rule, line);
   }
}


#ifdef SUPER_LOG


void log_rule3(log_sev_t sev, const char *func, size_t line, const char *rule)
#else


void log_rule3(log_sev_t sev, const char *func, const char *rule)
#endif
{
   // some Windows platforms provide a qualified function name ("ABC::XYZ::function_Name")
   // as __func__; call get_unqualified_func_name() to return an unqualified function name

   func = get_unqualified_func_name(func);

#ifdef SUPER_LOG
   LOG_FMT(sev, "log_rule(%s:%zu): rule is '%s'\n", func, line, rule);
#else
   LOG_FMT(sev, "log_rule(%s): rule is '%s'\n", func, rule);
#endif
}


void log_rule4(const char *rule, Chunk *first)
{
   if (cpd.html_file == nullptr)
   {
      return;
   }

   if (first->tracking == nullptr)
   {
      first->tracking = new track_list;
      first->tracking->reserve(3);
   }
   // copy the rule
   size_t length = strlen(rule) + 1;
   char   *r     = (char *)malloc(length);

   strcpy(r, rule);
   size_t   a_number = get_A_Number();
   Track_nr A        = make_pair(a_number, r);

   first->tracking->push_back(A);
   size_t sizeOfTrack = first->tracking->size();

   LOG_FMT(LSPACE, "log_rule4(%d): rule is '%s', after '%s', at line %zu, tracking number is %zu, size is %zu\n",
           __LINE__, rule, first->Text(), first->orig_line, a_number, sizeOfTrack);
}
