/**
 * @file preprocessor.cpp
 *
 * @author  Guy Maurel
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align/preprocessor.h"

#include "align/stack.h"
#include "log_rules.h"


constexpr static auto LCURRENT = LALPP;


using namespace uncrustify;


void align_preprocessor()
{
   LOG_FUNC_ENTRY();

   AlignStack as;    // value macros

   log_rule_B("align_pp_define_span");
   as.Start(options::align_pp_define_span());
   log_rule_B("align_pp_define_gap");
   as.m_gap = options::align_pp_define_gap();
   AlignStack *cur_as = &as;

   AlignStack asf;   // function macros

   log_rule_B("align_pp_define_span");
   asf.Start(options::align_pp_define_span());
   log_rule_B("align_pp_define_gap");
   asf.m_gap = options::align_pp_define_gap();

   Chunk *pc = Chunk::GetHead();

   while (pc->IsNotNullChunk())
   {
      // Note: not counting back-slash newline combos
      if (pc->Is(CT_NEWLINE))   // mind the gap: pc->IsNewline() is NOT the same!
      {
         as.NewLines(pc->GetNlCount());
         asf.NewLines(pc->GetNlCount());
      }

      // If we aren't on a 'define', then skip to the next non-comment
      if (pc->IsNot(CT_PP_DEFINE))
      {
         pc = pc->GetNextNc();
         continue;
      }
      // step past the 'define'
      pc = pc->GetNextNc();

      if (pc->IsNullChunk())
      {
         // coveralls will complain here. There are no example for that.
         // see https://en.wikipedia.org/wiki/Robustness_principle
         break;
      }
      LOG_FMT(LALPP, "%s(%d): define (%s) on line %zu col %zu\n",
              __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());

      cur_as = &as;

      if (pc->Is(CT_MACRO_FUNC))
      {
         log_rule_B("align_pp_define_together");

         if (!options::align_pp_define_together())
         {
            cur_as = &asf;
         }
         // Skip to the close parenthesis
         pc = pc->GetNextNc(); // point to open (
         pc = pc->GetNextType(CT_FPAREN_CLOSE, pc->GetLevel());

         LOG_FMT(LALPP, "%s(%d): jumped to (%s) on line %zu col %zu\n",
                 __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());
      }
      // step to the value past the close parenthesis or the macro name
      pc = pc->GetNext();

      if (pc->IsNullChunk())
      {
         // coveralls will complain here. There are no example for that.
         // see https://en.wikipedia.org/wiki/Robustness_principle
         break;
      }

      /*
       * don't align anything if the first line ends with a newline before
       * a value is given
       */
      if (!pc->IsNewline())
      {
         LOG_FMT(LALPP, "%s(%d): align on '%s', line %zu col %zu\n",
                 __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());

         cur_as->Add(pc);
      }
   }
   as.End();
   asf.End();
} // align_preprocessor
