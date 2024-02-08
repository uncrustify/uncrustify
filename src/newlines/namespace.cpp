/**
 * @file namespace.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/namespace.h"

#include "log_rules.h"
#include "newlines/iarf.h"
#include "options.h"


using namespace uncrustify;


constexpr static auto LCURRENT = LNEWLINE;


// namespace {
// namespace word {
// namespace type::word {
void newlines_namespace(Chunk *start)
{
   LOG_FUNC_ENTRY();

   log_rule_B("nl_namespace_brace");

   // Add or remove newline between 'namespace' and 'BRACE_OPEN'
   log_rule_B("nl_define_macro");
   iarf_e nl_opt = options::nl_namespace_brace();

   if (  nl_opt == IARF_IGNORE
      || (  start->TestFlags(PCF_IN_PREPROC)
         && !options::nl_define_macro()))
   {
      return;
   }
   Chunk *braceOpen = start->GetNextType(CT_BRACE_OPEN, start->GetLevel());

   LOG_FMT(LNEWLINE, "%s(%d): braceOpen orig line is %zu, orig col is %zu, Text() is '%s'\n",
           __func__, __LINE__, braceOpen->GetOrigLine(), braceOpen->GetOrigCol(), braceOpen->Text());
   // produces much more log output. Use it only debugging purpose
   //log_pcf_flags(LNEWLINE, braceOpen->GetFlags());

   if (braceOpen->TestFlags(PCF_ONE_LINER))
   {
      LOG_FMT(LNEWLINE, "%s(%d): is one_liner\n",
              __func__, __LINE__);
      return;
   }
   Chunk *beforeBrace = braceOpen->GetPrev();

   LOG_FMT(LNEWLINE, "%s(%d): beforeBrace orig line is %zu, orig col is %zu, Text() is '%s'\n",
           __func__, __LINE__, beforeBrace->GetOrigLine(), beforeBrace->GetOrigCol(), beforeBrace->Text());
   // 'namespace' 'BRACE_OPEN'
   newline_iarf_pair(beforeBrace, braceOpen, nl_opt);
} // newlines_namespace
