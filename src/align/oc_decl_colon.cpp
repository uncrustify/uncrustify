/**
 * @file oc_decl_colon.cpp
 *
 * @author  Guy Maurel
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align/oc_decl_colon.h"

#include "align/stack.h"
#include "chunk.h"


using namespace uncrustify;


void align_oc_decl_colon()
{
   LOG_FUNC_ENTRY();

   bool       did_line;
   AlignStack cas;   // for the colons
   AlignStack nas;   // for the parameter label

   cas.Start(4);
   nas.Start(4);
   nas.m_right_align = !options::align_on_tabstop();

   Chunk *pc = Chunk::GetHead();

   while (pc->IsNotNullChunk())
   {
      if (pc->IsNot(CT_OC_SCOPE))
      {
         pc = pc->GetNext();
         continue;
      }
      nas.Reset();
      cas.Reset();

      size_t level = pc->GetLevel();
      pc = pc->GetNextNcNnl(E_Scope::PREPROC);

      did_line = false;

      while (  pc->IsNotNullChunk()
            && pc->GetLevel() >= level)
      {
         // The declaration ends with an open brace or semicolon
         if (  pc->Is(CT_BRACE_OPEN)
            || pc->IsSemicolon())
         {
            break;
         }

         if (pc->IsNewline())
         {
            nas.NewLines(pc->GetNlCount());
            cas.NewLines(pc->GetNlCount());
            did_line = false;
         }
         else if (  !did_line
                 && pc->Is(CT_OC_COLON))
         {
            cas.Add(pc);

            Chunk *tmp  = pc->GetPrev(E_Scope::PREPROC);
            Chunk *tmp2 = tmp->GetPrevNcNnl(E_Scope::PREPROC);

            // Check for an un-labeled parameter
            if (  (  tmp->Is(CT_WORD)
                  || tmp->Is(CT_TYPE)
                  || tmp->Is(CT_OC_MSG_DECL)
                  || tmp->Is(CT_OC_MSG_SPEC))
               && (  tmp2->Is(CT_WORD)
                  || tmp2->Is(CT_TYPE)
                  || tmp2->Is(CT_PAREN_CLOSE)))
            {
               nas.Add(tmp);
            }
            did_line = true;
         }
         pc = pc->GetNext(E_Scope::PREPROC);
      }
      nas.End();
      cas.End();
   }
} // align_oc_decl_colon
