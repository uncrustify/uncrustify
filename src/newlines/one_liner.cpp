/**
 * @file one_liner.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#include "newlines/one_liner.h"

#include "keywords.h"
#include "log_rules.h"
#include "newlines/add.h"
#include "newlines/del_between.h"


using namespace uncrustify;


constexpr static auto LCURRENT = LNEWLINE;


bool is_class_one_liner(Chunk *pc)
{
   if (  (  pc->Is(CT_FUNC_CLASS_DEF)
         || pc->Is(CT_FUNC_DEF))
      && pc->TestFlags(PCF_IN_CLASS))
   {
      // Find opening brace
      pc = pc->GetNextType(CT_BRACE_OPEN, pc->GetLevel());
      return(  pc->IsNotNullChunk()
            && pc->TestFlags(PCF_ONE_LINER));
   }
   return(false);
} // is_class_one_liner


void nl_create_list_liner(Chunk *brace_open)
{
   LOG_FUNC_ENTRY();

   // See if we get a newline between the next text and the vbrace_close
   if (brace_open->IsNullChunk())
   {
      return;
   }
   Chunk *closing = brace_open->GetNextType(CT_BRACE_CLOSE, brace_open->GetLevel());
   Chunk *tmp     = brace_open;

   do
   {
      if (tmp->Is(CT_COMMA))
      {
         return;
      }
      tmp = tmp->GetNext();
   } while (tmp != closing);

   newline_del_between(brace_open, closing);
} // nl_create_list_liner


void nl_create_one_liner(Chunk *vbrace_open)
{
   LOG_FUNC_ENTRY();

   // See if we get a newline between the next text and the vbrace_close
   Chunk *tmp   = vbrace_open->GetNextNcNnl();
   Chunk *first = tmp;

   if (  first->IsNullChunk()
      || get_token_pattern_class(first->GetType()) != pattern_class_e::NONE)
   {
      return;
   }
   size_t nl_total = 0;

   while (tmp->IsNot(CT_VBRACE_CLOSE))
   {
      if (tmp->IsNewline())
      {
         nl_total += tmp->GetNlCount();

         if (nl_total > 1)
         {
            return;
         }
      }
      tmp = tmp->GetNext();
   }

   if (  tmp->IsNotNullChunk()
      && first->IsNotNullChunk())
   {
      newline_del_between(vbrace_open, first);
   }
} // nl_create_one_liner


//! Find the next newline or nl_cont
void nl_handle_define(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   Chunk *nl  = pc;
   Chunk *ref = Chunk::NullChunkPtr;

   while ((nl = nl->GetNext())->IsNotNullChunk())
   {
      if (nl->Is(CT_NEWLINE))
      {
         return;
      }

      if (  nl->Is(CT_MACRO)
         || (  nl->Is(CT_FPAREN_CLOSE)
            && nl->GetParentType() == CT_MACRO_FUNC))
      {
         ref = nl;
      }

      if (nl->Is(CT_NL_CONT))
      {
         if (ref->IsNotNullChunk())
         {
            newline_add_after(ref);
         }
         return;
      }
   }
} // nl_handle_define


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
      log_ruleNL("nl_func_leave_one_liners", pc);

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


void undo_one_liner(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   if (  pc->IsNotNullChunk()
      && pc->TestFlags(PCF_ONE_LINER))
   {
      LOG_FMT(LNL1LINE, "%s(%d): pc->Text() '%s', orig line is %zu, orig col is %zu",
              __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());
      pc->ResetFlagBits(PCF_ONE_LINER);

      // scan backward
      LOG_FMT(LNL1LINE, "%s(%d): scan backward\n", __func__, __LINE__);
      Chunk *tmp = pc;

      while ((tmp = tmp->GetPrev())->IsNotNullChunk())
      {
         if (!tmp->TestFlags(PCF_ONE_LINER))
         {
            LOG_FMT(LNL1LINE, "%s(%d): tmp->Text() '%s', orig line is %zu, orig col is %zu, --> break\n",
                    __func__, __LINE__, tmp->Text(), tmp->GetOrigLine(), tmp->GetOrigCol());
            break;
         }
         LOG_FMT(LNL1LINE, "%s(%d): clear for tmp->Text() '%s', orig line is %zu, orig col is %zu",
                 __func__, __LINE__, tmp->Text(), tmp->GetOrigLine(), tmp->GetOrigCol());
         tmp->ResetFlagBits(PCF_ONE_LINER);
      }
      // scan forward
      LOG_FMT(LNL1LINE, "%s(%d): scan forward\n", __func__, __LINE__);
      tmp = pc;
      LOG_FMT(LNL1LINE, "%s(%d): - \n", __func__, __LINE__);

      while ((tmp = tmp->GetNext())->IsNotNullChunk())
      {
         if (!tmp->TestFlags(PCF_ONE_LINER))
         {
            LOG_FMT(LNL1LINE, "%s(%d): tmp->Text() '%s', orig line is %zu, orig col is %zu, --> break\n",
                    __func__, __LINE__, tmp->Text(), tmp->GetOrigLine(), tmp->GetOrigCol());
            break;
         }
         LOG_FMT(LNL1LINE, "%s(%d): clear for tmp->Text() '%s', orig line is %zu, orig col is %zu",
                 __func__, __LINE__, tmp->Text(), tmp->GetOrigLine(), tmp->GetOrigCol());
         tmp->ResetFlagBits(PCF_ONE_LINER);
      }
      LOG_FMT(LNL1LINE, "\n");
   }
} // undo_one_liner
