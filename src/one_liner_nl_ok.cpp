/**
 * @file one_liner_nl_ok.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "log_rules.h"
#include "mark_change.h"

constexpr static auto LCURRENT = LNEWLINE;

#define MARK_CHANGE()    mark_change(__func__, __LINE__)


/**
 * Checks to see if it is OK to add a newline around the chunk.
 * Don't want to break one-liners...
 * return value:
 *  true: a new line may be added
 * false: a new line may NOT be added
 */
bool one_liner_nl_ok(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNL1LINE, "%s(%d): check type is %s, parent is %s, flag is %s, orig line is %zu, orig col is %zu\n",
           __func__, __LINE__, get_token_name(pc->GetType()), get_token_name(pc->GetParentType()),
           pcf_flags_str(pc->GetFlags()).c_str(), pc->GetOrigLine(), pc->GetOrigCol());

   if (!pc->TestFlags(PCF_ONE_LINER))
   {
      LOG_FMT(LNL1LINE, "%s(%d): true (not 1-liner), a new line may be added\n", __func__, __LINE__);
      return(true);
   }
   // Step back to find the opening brace
   Chunk *br_open = pc;

   if (br_open->IsBraceClose())
   {
      br_open = br_open->GetPrevType(br_open->Is(CT_BRACE_CLOSE) ? CT_BRACE_OPEN : CT_VBRACE_OPEN,
                                     br_open->GetLevel(), E_Scope::ALL);
   }
   else
   {
      while (  br_open->IsNotNullChunk()
            && br_open->TestFlags(PCF_ONE_LINER)
            && !br_open->IsBraceOpen()
            && !br_open->IsBraceClose())
      {
         br_open = br_open->GetPrev();
      }
   }
   pc = br_open;

   if (  pc->IsNotNullChunk()
      && pc->TestFlags(PCF_ONE_LINER)
      && (  pc->IsBraceOpen()
         || pc->IsBraceClose()))
   {
      log_rule_B("nl_class_leave_one_liners");

      if (  options::nl_class_leave_one_liners()
         && pc->TestFlags(PCF_IN_CLASS))
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (class)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_assign_leave_one_liners");

      if (  options::nl_assign_leave_one_liners()
         && pc->GetParentType() == CT_ASSIGN)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (assign)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_enum_leave_one_liners");

      if (  options::nl_enum_leave_one_liners()
         && pc->GetParentType() == CT_ENUM)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (enum)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_getset_leave_one_liners");

      if (  options::nl_getset_leave_one_liners()
         && pc->GetParentType() == CT_GETSET)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (get/set), a new line may NOT be added\n", __func__, __LINE__);
         return(false);
      }
      // Issue #UT-98
      log_rule_B("nl_cs_property_leave_one_liners");

      if (  options::nl_cs_property_leave_one_liners()
         && pc->GetParentType() == CT_CS_PROPERTY)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (c# property), a new line may NOT be added\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_func_leave_one_liners");

      if (  options::nl_func_leave_one_liners()
         && (  pc->GetParentType() == CT_FUNC_DEF
            || pc->GetParentType() == CT_FUNC_CLASS_DEF))
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (func def)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_func_leave_one_liners");

      if (  options::nl_func_leave_one_liners()
         && pc->GetParentType() == CT_OC_MSG_DECL)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (method def)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_cpp_lambda_leave_one_liners");

      if (  options::nl_cpp_lambda_leave_one_liners()
         && ((pc->GetParentType() == CT_CPP_LAMBDA)))
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (lambda)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_oc_msg_leave_one_liner");

      if (  options::nl_oc_msg_leave_one_liner()
         && pc->TestFlags(PCF_IN_OC_MSG))
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (message)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_if_leave_one_liners");

      if (  options::nl_if_leave_one_liners()
         && (  pc->GetParentType() == CT_IF
            || pc->GetParentType() == CT_ELSEIF
            || pc->GetParentType() == CT_ELSE))
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (if/else)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_while_leave_one_liners");

      if (  options::nl_while_leave_one_liners()
         && pc->GetParentType() == CT_WHILE)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (while)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_do_leave_one_liners");

      if (  options::nl_do_leave_one_liners()
         && pc->GetParentType() == CT_DO)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (do)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_for_leave_one_liners");

      if (  options::nl_for_leave_one_liners()
         && pc->GetParentType() == CT_FOR)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (for)\n", __func__, __LINE__);
         return(false);
      }
      log_rule_B("nl_namespace_two_to_one_liner - 2");

      if (  options::nl_namespace_two_to_one_liner()
         && pc->GetParentType() == CT_NAMESPACE)
      {
         LOG_FMT(LNL1LINE, "%s(%d): false (namespace)\n", __func__, __LINE__);
         return(false);
      }
   }
   LOG_FMT(LNL1LINE, "%s(%d): true, a new line may be added\n", __func__, __LINE__);
   return(true);
} // one_liner_nl_ok
