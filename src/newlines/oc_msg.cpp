/**
 * @file oc_msg.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#include "newlines/oc_msg.h"

#include "log_rules.h"
#include "newlines/add.h"


using namespace uncrustify;


constexpr static auto LCURRENT = LNEWLINE;


/**
 * Formats a message, adding newlines before the item before the colons.
 *
 * Start points to the open '[' in:
 * [myObject doFooWith:arg1 name:arg2  // some lines with >1 arg
 *            error:arg3];
 */
void newline_oc_msg(Chunk *start)
{
   LOG_FUNC_ENTRY();

   Chunk *sq_c = start->GetClosingParen();

   if (sq_c->IsNullChunk())
   {
      return;
   }
   log_rule_B("nl_oc_msg_leave_one_liner");

   if (options::nl_oc_msg_leave_one_liner())
   {
      return;
   }
   bool should_nl_msg = false;

   // Get count of parameters
   size_t parameter_count = 0;

   for (Chunk *pc = start->GetNextNcNnl(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      if (pc->GetLevel() <= start->GetLevel())
      {
         break;
      }

      if (pc->Is(CT_OC_COLON) && pc->GetLevel() - 1 == start->GetLevel())
      {
         parameter_count++;
      }
   }

   size_t min_params = options::nl_oc_msg_args_min_params();

   if (  parameter_count >= min_params
      && min_params != 0)
   {
      should_nl_msg = true;
   }
   // Get length of longest line
   size_t longest_line = 0;

   for (Chunk *pc = start->GetNextNcNnl(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      if (pc->GetLevel() <= start->GetLevel())
      {
         break;
      }

      if (pc->GetOrigColEnd() > longest_line)
      {
         longest_line = pc->GetOrigColEnd();
      }
   }

   size_t max_code_width = options::nl_oc_msg_args_max_code_width();

   if (  longest_line > max_code_width
      && max_code_width != 0)
   {
      should_nl_msg = true;
   }
   bool finish_multi_line = options::nl_oc_msg_args_finish_multi_line();

   if (finish_multi_line)
   {
      size_t prev_line = 0;

      for (Chunk *pc = start->GetNextNcNnl(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
      {
         if (pc->GetLevel() <= start->GetLevel())
         {
            break;
         }

         if (prev_line > 0 && pc->GetOrigLine() != prev_line)
         {
            should_nl_msg = true;
         }
         prev_line = pc->GetOrigLine();
      }
   }

   // If both nl_oc_msg_args_min_params and nl_oc_msg_args_max_code_width are disabled
   // we should newline all messages.
   if (  max_code_width == 0
      && min_params == 0)
   {
      should_nl_msg = true;
   }

   if (!should_nl_msg)
   {
      return;
   }

   for (Chunk *pc = start->GetNextNcNnl(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      if (pc->GetLevel() <= start->GetLevel())
      {
         break;
      }

      if (pc->Is(CT_OC_MSG_NAME) && pc->GetLevel() - 1 == start->GetLevel())
      {
         newline_add_before(pc);
      }
   }
} // newline_oc_msg
