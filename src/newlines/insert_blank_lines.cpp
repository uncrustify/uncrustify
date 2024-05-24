/**
 * @file insert_blank_lines.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/if_for_while_switch.h"
#include "newlines/insert_blank_lines.h"
#include "newlines/one_liner.h"

#include "chunk.h"
#include "log_rules.h"
#include "newlines/func_pre_blank_lines.h"


using namespace uncrustify;


constexpr static auto LCURRENT = LNEWLINE;


void newlines_insert_blank_lines()
{
   LOG_FUNC_ENTRY();

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      //LOG_FMT(LNEWLINE, "%s(%d): orig line is %zu, orig col is %zu, Text() '%s', type is %s\n",
      //        __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), get_token_name(pc->GetType()));
      if (pc->Is(CT_IF))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_before_if());
         log_rule_B("nl_before_if");
         newlines_if_for_while_switch_post_blank_lines(pc, options::nl_after_if());
         log_rule_B("nl_after_if");
      }
      else if (pc->Is(CT_FOR))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_before_for());
         log_rule_B("nl_before_for");
         newlines_if_for_while_switch_post_blank_lines(pc, options::nl_after_for());
         log_rule_B("nl_after_for");
      }
      else if (pc->Is(CT_WHILE))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_before_while());
         log_rule_B("nl_before_while");
         newlines_if_for_while_switch_post_blank_lines(pc, options::nl_after_while());
         log_rule_B("nl_after_while");
      }
      else if (pc->Is(CT_SWITCH))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_before_switch());
         log_rule_B("nl_before_switch");
         newlines_if_for_while_switch_post_blank_lines(pc, options::nl_after_switch());
         log_rule_B("nl_after_switch");
      }
      else if (pc->Is(CT_SYNCHRONIZED))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_before_synchronized());
         log_rule_B("nl_before_synchronized");
         newlines_if_for_while_switch_post_blank_lines(pc, options::nl_after_synchronized());
         log_rule_B("nl_after_synchronized");
      }
      else if (pc->Is(CT_DO))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_before_do());
         log_rule_B("nl_before_do");
         newlines_if_for_while_switch_post_blank_lines(pc, options::nl_after_do());
         log_rule_B("nl_after_do");
      }
      else if (pc->Is(CT_OC_INTF))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_oc_before_interface());
         log_rule_B("nl_oc_before_interface");
      }
      else if (pc->Is(CT_OC_END))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_oc_before_end());
         log_rule_B("nl_oc_before_end");
      }
      else if (pc->Is(CT_OC_IMPL))
      {
         newlines_if_for_while_switch_pre_blank_lines(pc, options::nl_oc_before_implementation());
         log_rule_B("nl_oc_before_implementation");
      }
      else if (  pc->Is(CT_FUNC_CLASS_DEF)
              || pc->Is(CT_FUNC_DEF)
              || pc->Is(CT_FUNC_CLASS_PROTO)
              || pc->Is(CT_FUNC_PROTO))
      {
         if (  options::nl_class_leave_one_liner_groups()
            && is_class_one_liner(pc))
         {
            log_rule_B("nl_class_leave_one_liner_groups");
            newlines_func_pre_blank_lines(pc, CT_FUNC_PROTO);
         }
         else
         {
            newlines_func_pre_blank_lines(pc, pc->GetType());
         }
      }
      else
      {
         // ignore it
         //LOG_FMT(LNEWLINE, "%s(%d): ignore it\n", __func__, __LINE__);
      }
   }
} // newlines_insert_blank_lines
