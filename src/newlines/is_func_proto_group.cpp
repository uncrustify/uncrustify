/**
 * @file is_func_proto_group.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "is_func_proto_group.h"

#include "chunk.h"
#include "log_rules.h"


constexpr static auto LCURRENT = LNEWLINE;


using namespace uncrustify;


/**
 * Test if a chunk may be combined with a function prototype group.
 *
 * If nl_class_leave_one_liner_groups is enabled, a chunk may be combined with
 * a function prototype group if it is a one-liner inside a class body, and is
 * a definition of the same sort as surrounding prototypes. This checks against
 * either the function name, or the function closing brace.
 */
bool is_func_proto_group(Chunk *pc, E_Token one_liner_type)
{
   if (  pc->IsNotNullChunk()
      && options::nl_class_leave_one_liner_groups()
      && (  pc->Is(one_liner_type)
         || pc->GetParentType() == one_liner_type)
      && pc->TestFlags(PCF_IN_CLASS))
   {
      log_rule_B("nl_class_leave_one_liner_groups");

      if (pc->Is(CT_BRACE_CLOSE))
      {
         return(pc->TestFlags(PCF_ONE_LINER));
      }
      else
      {
         // Find opening brace
         pc = pc->GetNextType(CT_BRACE_OPEN, pc->GetLevel());
         return(  pc->IsNotNullChunk()
               && pc->TestFlags(PCF_ONE_LINER));
      }
   }
   return(false);
} // is_func_proto_group
