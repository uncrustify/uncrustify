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

   if (second->IsNot(CT_NEWLINE))
   {
      LOG_FMT(LSPACE, "%s(%zu): first orig line is %zu, orig col is %zu, Text() is '%s', [%s/%s] <===>\n",
              func, line, first->GetOrigLine(), first->GetOrigCol(), first->Text(),
              get_token_name(first->GetType()), get_token_name(first->GetParentType()));
      LOG_FMT(LSPACE, "           second orig line is %zu, orig col is %zu, Text() is '%s', [%s/%s] :",
              second->GetOrigLine(), second->GetOrigCol(), second->Text(),
              get_token_name(second->GetType()), get_token_name(second->GetParentType()));
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
   if (!(cpd.html_type == tracking_type_e::TT_SPACE))
   {
      return;
   }

   if (first->GetTrackingData() == nullptr)
   {
      first->TrackingData() = new TrackList;
   }
   // copy the rule
   size_t length = strlen(rule) + 1;
   char   *r     = (char *)malloc(length);

   strcpy(r, rule);
   size_t      a_number = get_A_Number();
   TrackNumber A        = std::make_pair(a_number, r);

   first->TrackingData()->push_back(A);
   size_t sizeOfTrack = first->GetTrackingData()->size();

   LOG_FMT(LSPACE, "log_rule4(%d): rule is '%s', after '%s', at line %zu, tracking number is %zu, size is %zu\n",
           __LINE__, rule, first->Text(), first->GetOrigLine(), a_number, sizeOfTrack);
} // void log_rule4


void log_ruleStart(const char *rule, Chunk *first)
{
   if (!(cpd.html_type == tracking_type_e::TT_START))
   {
      return;
   }

   if (first->GetTrackingData() == nullptr)
   {
      first->TrackingData() = new TrackList;
   }
   // copy the rule
   size_t length = strlen(rule) + 1;
   char   *r     = (char *)malloc(length);

   strcpy(r, rule);
   size_t      a_number = get_A_Number();
   TrackNumber A        = std::make_pair(a_number, r);

   first->TrackingData()->push_back(A);
   size_t sizeOfTrack = first->GetTrackingData()->size();

   LOG_FMT(LSPACE, "log_ruleStart(%d): rule is '%s', '%s', at line %zu, tracking number is %zu, size is %zu\n",
           __LINE__, rule, first->Text(), first->GetOrigLine(), a_number, sizeOfTrack);
} // log_ruleStart


void log_ruleNL(const char *rule, Chunk *pc)
{
   if (!(cpd.html_type == tracking_type_e::TT_NEWLINE))
   {
      return;
   }

   if (pc->GetTrackingData() == nullptr)
   {
      pc->TrackingData() = new TrackList;
   }
   // copy the rule
   size_t length = strlen(rule) + 1;
   char   *r     = (char *)malloc(length);

   strcpy(r, rule);
   size_t      a_number = get_A_Number();
   TrackNumber A        = std::make_pair(a_number, r);

   pc->TrackingData()->push_back(A);
   size_t sizeOfTrack = pc->GetTrackingData()->size();

   LOG_FMT(LSPACE, "log_ruleNL(%d): rule is '%s', after '%s', at line %zu, tracking number is %zu, size is %zu\n",
           __LINE__, rule, pc->Text(), pc->GetOrigLine(), a_number, sizeOfTrack);
} // void log_ruleNL
