/**
 * @file newline_iarf.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "newline_iarf.h"

#include "newline_iarf_pair.h"

using namespace uncrustify;


void newline_iarf(Chunk *pc, iarf_e av)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNFD, "%s(%d): ", __func__, __LINE__);
   log_func_stack(LNFD, "CallStack:");
   Chunk *after = Chunk::NullChunkPtr;

   if (pc != nullptr)
   {
      after = pc->GetNextNnl();
   }

   if (  (pc != nullptr && pc->Is(CT_FPAREN_OPEN))                         // Issue #2914
      && pc->GetParentType() == CT_FUNC_CALL
      && after->Is(CT_COMMENT_CPP)
      && options::donot_add_nl_before_cpp_comment())
   {
      return;
   }
   newline_iarf_pair(pc, after, av);
} // newline_iarf
