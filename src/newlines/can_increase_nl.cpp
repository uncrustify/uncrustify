/**
 * @file can_increase_nl.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#include "newlines/can_increase_nl.h"

#include "chunk.h"
#include "ifdef_over_whole_file.h"
#include "log_rules.h"


constexpr static auto LCURRENT = LNEWLINE;


using namespace uncrustify;


bool can_increase_nl(Chunk *nl)
{
   LOG_FUNC_ENTRY();

   Chunk *prev = nl->GetPrevNc();

   Chunk *pcmt = nl->GetPrev();
   Chunk *next = nl->GetNext();

   if (options::nl_squeeze_ifdef())
   {
      log_rule_B("nl_squeeze_ifdef");

      Chunk *pp_start = prev->GetPpStart();

      if (  pp_start->IsNotNullChunk()
         && (  pp_start->GetParentType() == CT_PP_IF
            || pp_start->GetParentType() == CT_PP_ELSE)
         && (  pp_start->GetLevel() > 0
            || options::nl_squeeze_ifdef_top_level()))
      {
         log_rule_B("nl_squeeze_ifdef_top_level");
         bool rv = ifdef_over_whole_file() && pp_start->TestFlags(PCF_WF_IF);
         LOG_FMT(LBLANKD, "%s(%d): nl_squeeze_ifdef %zu (prev) pp_lvl=%zu rv=%d\n",
                 __func__, __LINE__, nl->GetOrigLine(), nl->GetPpLevel(), rv);
         return(rv);
      }

      if (  next->Is(CT_PREPROC)
         && (  next->GetParentType() == CT_PP_ELSE
            || next->GetParentType() == CT_PP_ENDIF)
         && (  next->GetLevel() > 0
            || options::nl_squeeze_ifdef_top_level()))
      {
         log_rule_B("nl_squeeze_ifdef_top_level");
         bool rv = ifdef_over_whole_file() && next->TestFlags(PCF_WF_ENDIF);
         LOG_FMT(LBLANKD, "%s(%d): nl_squeeze_ifdef %zu (next) pp_lvl=%zu rv=%d\n",
                 __func__, __LINE__, nl->GetOrigLine(), nl->GetPpLevel(), rv);
         return(rv);
      }
   }

   if (next->Is(CT_BRACE_CLOSE))
   {
      if (  options::nl_inside_namespace() > 0
         && next->GetParentType() == CT_NAMESPACE)
      {
         log_rule_B("nl_inside_namespace");
         LOG_FMT(LBLANKD, "%s(%d): nl_inside_namespace %zu\n",
                 __func__, __LINE__, nl->GetOrigLine());
         return(true);
      }

      if (  options::nl_inside_empty_func() > 0
         && prev->Is(CT_BRACE_OPEN)
         && (  next->GetParentType() == CT_FUNC_DEF
            || next->GetParentType() == CT_FUNC_CLASS_DEF))
      {
         log_rule_B("nl_inside_empty_func");
         LOG_FMT(LBLANKD, "%s(%d): nl_inside_empty_func %zu\n",
                 __func__, __LINE__, nl->GetOrigLine());
         return(true);
      }

      if (options::eat_blanks_before_close_brace())
      {
         log_rule_B("eat_blanks_before_close_brace");
         LOG_FMT(LBLANKD, "%s(%d): eat_blanks_before_close_brace %zu\n",
                 __func__, __LINE__, nl->GetOrigLine());
         return(false);
      }
   }

   if (prev->Is(CT_BRACE_CLOSE))
   {
      if (  options::nl_before_namespace()
         && prev->GetParentType() == CT_NAMESPACE)
      {
         log_rule_B("nl_before_namespace");
         LOG_FMT(LBLANKD, "%s(%d): nl_before_namespace %zu\n",
                 __func__, __LINE__, nl->GetOrigLine());
         return(true);
      }
   }

   if (prev->Is(CT_BRACE_OPEN))
   {
      if (  options::nl_inside_namespace() > 0
         && prev->GetParentType() == CT_NAMESPACE)
      {
         log_rule_B("nl_inside_namespace");
         LOG_FMT(LBLANKD, "%s(%d): nl_inside_namespace %zu\n",
                 __func__, __LINE__, nl->GetOrigLine());
         return(true);
      }

      if (  options::nl_inside_empty_func() > 0
         && next->Is(CT_BRACE_CLOSE)
         && (  prev->GetParentType() == CT_FUNC_DEF
            || prev->GetParentType() == CT_FUNC_CLASS_DEF))
      {
         log_rule_B("nl_inside_empty_func");
         LOG_FMT(LBLANKD, "%s(%d): nl_inside_empty_func %zu\n",
                 __func__, __LINE__, nl->GetOrigLine());
         return(true);
      }

      if (options::eat_blanks_after_open_brace())
      {
         log_rule_B("eat_blanks_after_open_brace");
         LOG_FMT(LBLANKD, "%s(%d): eat_blanks_after_open_brace %zu\n",
                 __func__, __LINE__, nl->GetOrigLine());
         return(false);
      }
   }
   log_rule_B("nl_start_of_file");

   if (  pcmt->IsNullChunk()
      && (options::nl_start_of_file() != IARF_IGNORE))
   {
      LOG_FMT(LBLANKD, "%s(%d): SOF no prev %zu\n", __func__, __LINE__, nl->GetOrigLine());
      return(false);
   }
   log_rule_B("nl_end_of_file");

   if (  next->IsNullChunk()
      && (options::nl_end_of_file() != IARF_IGNORE))
   {
      LOG_FMT(LBLANKD, "%s(%d): EOF no next %zu\n", __func__, __LINE__, nl->GetOrigLine());
      return(false);
   }
   return(true);
} // can_increase_nl
