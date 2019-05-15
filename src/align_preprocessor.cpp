/**
 * @file align_preprocessor.cpp
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align_preprocessor.h"

#include "align_assign.h"
#include "align_stack.h"

using namespace uncrustify;


void align_preprocessor(void)
{
   LOG_FUNC_ENTRY();

   AlignStack as;    // value macros
   as.Start(options::align_pp_define_span());
   as.m_gap = options::align_pp_define_gap();
   AlignStack *cur_as = &as;

   AlignStack asf;   // function macros
   asf.Start(options::align_pp_define_span());
   asf.m_gap = options::align_pp_define_gap();

   chunk_t *pc = chunk_get_head();
   while (pc != nullptr)
   {
      // Note: not counting back-slash newline combos
      if (chunk_is_token(pc, CT_NEWLINE))   // mind the gap: chunk_is_newline(pc) is NOT the same!
      {
         as.NewLines(pc->nl_count);
         asf.NewLines(pc->nl_count);
      }

      // If we aren't on a 'define', then skip to the next non-comment
      if (pc->type != CT_PP_DEFINE)
      {
         pc = chunk_get_next_nc(pc);
         continue;
      }

      // step past the 'define'
      pc = chunk_get_next_nc(pc);
      if (pc == nullptr)
      {
         // coveralls will complain here. There are no example for that.
         // see https://en.wikipedia.org/wiki/Robustness_principle
         break;
      }

      LOG_FMT(LALPP, "%s(%d): define (%s) on line %zu col %zu\n",
              __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);

      cur_as = &as;
      if (chunk_is_token(pc, CT_MACRO_FUNC))
      {
         if (!options::align_pp_define_together())
         {
            cur_as = &asf;
         }

         // Skip to the close parenthesis
         pc = chunk_get_next_nc(pc); // point to open (
         pc = chunk_get_next_type(pc, CT_FPAREN_CLOSE, pc->level);

         LOG_FMT(LALPP, "%s(%d): jumped to (%s) on line %zu col %zu\n",
                 __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
      }

      // step to the value past the close parenthesis or the macro name
      pc = chunk_get_next(pc);
      if (pc == nullptr)
      {
         // coveralls will complain here. There are no example for that.
         // see https://en.wikipedia.org/wiki/Robustness_principle
         break;
      }

      /*
       * don't align anything if the first line ends with a newline before
       * a value is given
       */
      if (!chunk_is_newline(pc))
      {
         LOG_FMT(LALPP, "%s(%d): align on '%s', line %zu col %zu\n",
                 __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);

         cur_as->Add(pc);
      }
   }

   as.End();
   asf.End();
} // align_preprocessor
