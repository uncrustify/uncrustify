/**
 * @file do_it_newlines_func_pre_blank_lines.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/do_it_newlines_func_pre_blank_lines.h"

#include "blank_line.h"
#include "chunk.h"
#include "log_rules.h"
#include "uncrustify.h"


constexpr static auto LCURRENT = LNEWLINE;


using namespace uncrustify;


bool do_it_newlines_func_pre_blank_lines(Chunk *last_nl, E_Token start_type)
{
   LOG_FUNC_ENTRY();

   if (last_nl->IsNullChunk())
   {
      return(false);
   }
   LOG_FMT(LNLFUNCT, "%s(%d): orig line is %zu, orig col is %zu, type is %s, Text() is '%s'\n",
           __func__, __LINE__,
           last_nl->GetOrigLine(), last_nl->GetOrigCol(), get_token_name(last_nl->GetType()), last_nl->Text());

   switch (start_type)
   {
   case CT_FUNC_CLASS_DEF:
   {
      log_rule_B("nl_before_func_class_def");
      bool diff = options::nl_before_func_class_def() <= last_nl->GetNlCount();
      LOG_FMT(LNLFUNCT, "%s(%d): is %s\n",
              __func__, __LINE__, diff ? "TRUE" : "FALSE");

      log_rule_B("nl_before_func_class_def");

      if (options::nl_before_func_class_def() != last_nl->GetNlCount())
      {
         LOG_FMT(LNLFUNCT, "%s(%d):   set blank line(s) to %u\n",
                 __func__, __LINE__, options::nl_before_func_class_def());
         blank_line_set(last_nl, options::nl_before_func_class_def);
      }
      return(diff);
   }

   case CT_FUNC_CLASS_PROTO:
   {
      log_rule_B("nl_before_func_class_proto");
      bool diff = options::nl_before_func_class_proto() <= last_nl->GetNlCount();
      LOG_FMT(LNLFUNCT, "%s(%d): is %s\n",
              __func__, __LINE__, diff ? "TRUE" : "FALSE");

      log_rule_B("nl_before_func_class_proto");

      if (options::nl_before_func_class_proto() != last_nl->GetNlCount())
      {
         LOG_FMT(LNLFUNCT, "%s(%d):   set blank line(s) to %u\n",
                 __func__, __LINE__, options::nl_before_func_class_proto());
         blank_line_set(last_nl, options::nl_before_func_class_proto);
      }
      return(diff);
   }

   case CT_FUNC_DEF:
   {
      LOG_FMT(LNLFUNCT, "%s(%d): nl_before_func_body_def() is %u, last_nl new line count is %zu\n",
              __func__, __LINE__, options::nl_before_func_body_def(), last_nl->GetNlCount());
      log_rule_B("nl_before_func_body_def");
      bool diff = options::nl_before_func_body_def() <= last_nl->GetNlCount();
      LOG_FMT(LNLFUNCT, "%s(%d): is %s\n",
              __func__, __LINE__, diff ? "TRUE" : "FALSE");

      log_rule_B("nl_before_func_body_def");

      if (options::nl_before_func_body_def() != last_nl->GetNlCount())
      {
         LOG_FMT(LNLFUNCT, "%s(%d):    set blank line(s) to %u\n",
                 __func__, __LINE__, options::nl_before_func_body_def());
         log_rule_B("nl_before_func_body_def");
         blank_line_set(last_nl, options::nl_before_func_body_def);
      }
      LOG_FMT(LNLFUNCT, "%s(%d): nl_before_func_body_def() is %u, last_nl new line count is %zu\n",
              __func__, __LINE__, options::nl_before_func_body_def(), last_nl->GetNlCount());
      return(diff);
   }

   case CT_FUNC_PROTO:
   {
      log_rule_B("nl_before_func_body_proto");
      bool diff = options::nl_before_func_body_proto() <= last_nl->GetNlCount();
      LOG_FMT(LNLFUNCT, "%s(%d): is %s\n",
              __func__, __LINE__, diff ? "TRUE" : "FALSE");

      log_rule_B("nl_before_func_body_proto");

      if (options::nl_before_func_body_proto() != last_nl->GetNlCount())
      {
         LOG_FMT(LNLFUNCT, "%s(%d):   set blank line(s) to %u\n",
                 __func__, __LINE__, options::nl_before_func_body_proto());
         log_rule_B("nl_before_func_body_proto");
         blank_line_set(last_nl, options::nl_before_func_body_proto);
      }
      return(diff);
   }

   default:
   {
      LOG_FMT(LERR, "%s(%d):   setting to blank line(s) at line %zu not possible\n",
              __func__, __LINE__, last_nl->GetOrigLine());
      return(false);
   }
   } // switch
} // do_it_newlines_func_pre_blank_lines
