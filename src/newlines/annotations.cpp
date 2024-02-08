/**
 * @file annotations.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/annotations.h"

#include "chunk.h"
#include "log_rules.h"
#include "newlines/iarf.h"
#include "options.h"


using namespace uncrustify;


constexpr static auto LCURRENT = LNEWLINE;


void annotations_newlines()
{
   LOG_FUNC_ENTRY();

   Chunk *next;
   Chunk *prev;
   Chunk *ae;   // last token of the annotation
   Chunk *pc = Chunk::GetHead();

   while (  (pc = pc->GetNextType(CT_ANNOTATION))->IsNotNullChunk()
         && (next = pc->GetNextNnl())->IsNotNullChunk())
   {
      // find the end of this annotation
      if (next->IsParenOpen())
      {
         // TODO: control newline between annotation and '(' ?
         ae = next->GetClosingParen();
      }
      else
      {
         ae = pc;
      }

      if (ae->IsNullChunk())
      {
         break;
      }
      LOG_FMT(LANNOT, "%s(%d): orig line is %zu, orig col is %zu, annotation is '%s',  end @ orig line %zu, orig col %zu, is '%s'\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(),
              ae->GetOrigLine(), ae->GetOrigCol(), ae->Text());

      prev = ae->GetPrev();             // Issue #1845
      LOG_FMT(LANNOT, "%s(%d): prev orig line is %zu, orig col is %zu, Text() is '%s'\n",
              __func__, __LINE__, prev->GetOrigLine(), prev->GetOrigCol(), prev->Text());
      next = ae->GetNextNnl();

      if (next->Is(CT_ANNOTATION))
      {
         LOG_FMT(LANNOT, "%s(%d):  -- nl_between_annotation\n",
                 __func__, __LINE__);
         newline_iarf(ae, uncrustify::options::nl_between_annotation());
         log_rule_B("nl_between_annotation");
      }

      if (next->Is(CT_NEWLINE))
      {
         if (next->Is(CT_ANNOTATION))
         {
            LOG_FMT(LANNOT, "%s(%d):  -- nl_after_annotation\n",
                    __func__, __LINE__);
            newline_iarf(ae, options::nl_after_annotation());
            log_rule_B("nl_after_annotation");
         }
      }
   }
} // annotations_newlines
