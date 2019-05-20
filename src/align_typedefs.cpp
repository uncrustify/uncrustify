/**
 * @file align_typedefs.cpp
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align_typedefs.h"

#include "align_stack.h"
#include "chunk_list.h"

using namespace uncrustify;


void align_typedefs(size_t span)
{
   LOG_FUNC_ENTRY();

   AlignStack as;
   as.Start(span);
   as.m_gap        = options::align_typedef_gap();
   as.m_star_style = static_cast<AlignStack::StarStyle>(options::align_typedef_star_style());
   as.m_amp_style  = static_cast<AlignStack::StarStyle>(options::align_typedef_amp_style());

   chunk_t *c_typedef = nullptr;
   chunk_t *pc        = chunk_get_head();
   while (pc != nullptr)
   {
      if (chunk_is_newline(pc))
      {
         as.NewLines(pc->nl_count);
         c_typedef = nullptr;
      }
      else if (c_typedef != nullptr)
      {
         if (pc->flags & PCF_ANCHOR)
         {
            as.Add(pc);
            LOG_FMT(LALTD, "%s(%d): typedef @ %zu:%zu, tag '%s' @ %zu:%zu\n",
                    __func__, __LINE__, c_typedef->orig_line, c_typedef->orig_col,
                    pc->text(), pc->orig_line, pc->orig_col);
            c_typedef = nullptr;
         }
      }
      else
      {
         if (chunk_is_token(pc, CT_TYPEDEF))
         {
            c_typedef = pc;
         }
      }

      pc = chunk_get_next(pc);
   }

   as.End();
} // align_typedefs
