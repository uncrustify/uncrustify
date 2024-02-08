/**
 * @file typedefs.cpp
 *
 * @author  Guy Maurel
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align/typedefs.h"

#include "align/stack.h"
#include "log_rules.h"


constexpr static auto LCURRENT = LALTD;


using namespace uncrustify;


void align_typedefs(size_t span)
{
   LOG_FUNC_ENTRY();

   AlignStack as;

   as.Start(span);
   log_rule_B("align_typedef_gap");
   as.m_gap = options::align_typedef_gap();
   log_rule_B("align_typedef_star_style");
   as.m_star_style = static_cast<AlignStack::StarStyle>(options::align_typedef_star_style());
   log_rule_B("align_typedef_amp_style");
   as.m_amp_style = static_cast<AlignStack::StarStyle>(options::align_typedef_amp_style());

   Chunk *c_typedef = Chunk::NullChunkPtr;
   Chunk *pc        = Chunk::GetHead();

   while (pc->IsNotNullChunk())
   {
      if (pc->IsNewline())
      {
         as.NewLines(pc->GetNlCount());
         c_typedef = Chunk::NullChunkPtr;
      }
      else if (c_typedef->IsNotNullChunk())
      {
         if (pc->TestFlags(PCF_ANCHOR))
         {
            as.Add(pc);
            LOG_FMT(LALTD, "%s(%d): typedef @ %zu:%zu, tag '%s' @ %zu:%zu\n",
                    __func__, __LINE__, c_typedef->GetOrigLine(), c_typedef->GetOrigCol(),
                    pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());
            c_typedef = Chunk::NullChunkPtr;
         }
      }
      else
      {
         if (pc->Is(CT_TYPEDEF))
         {
            c_typedef = pc;
         }
      }
      pc = pc->GetNext();
   }
   as.End();
} // align_typedefs
