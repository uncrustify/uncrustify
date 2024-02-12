/**
 * @file remove.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#include "newlines/remove.h"

#include "chunk.h"
#include "mark_change.h"
#include "newlines/can_increase_nl.h"
#include "newlines/iarf.h"


using namespace uncrustify;


void newlines_remove_disallowed()
{
   LOG_FUNC_ENTRY();

   Chunk *pc = Chunk::GetHead();
   Chunk *next;

   while ((pc = pc->GetNextNl())->IsNotNullChunk())
   {
      LOG_FMT(LBLANKD, "%s(%d): orig line is %zu, orig col is %zu, <Newline>, nl is %zu\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetNlCount());

      next = pc->GetNext();

      if (  next->IsNotNullChunk()
         && !next->Is(CT_NEWLINE)
         && !can_increase_nl(pc))
      {
         LOG_FMT(LBLANKD, "%s(%d): force to 1 orig line is %zu, orig col is %zu\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol());

         if (pc->GetNlCount() != 1)
         {
            pc->SetNlCount(1);
            MARK_CHANGE();
         }
      }
   }
} // newlines_remove_disallowed


void newlines_remove_newlines()
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LBLANK, "%s(%d):\n", __func__, __LINE__);
   Chunk *pc = Chunk::GetHead();

   if (!pc->IsNewline())
   {
      pc = pc->GetNextNl();
   }
   Chunk *next;
   Chunk *prev;

   while (pc->IsNotNullChunk())
   {
      // Remove all newlines not in preproc
      if (!pc->TestFlags(PCF_IN_PREPROC))
      {
         next = pc->GetNext();
         prev = pc->GetPrev();
         newline_iarf(pc, IARF_REMOVE);

         if (next == Chunk::GetHead())
         {
            pc = next;
            continue;
         }
         else if (  prev->IsNotNullChunk()
                 && !prev->GetNext()->IsNewline())
         {
            pc = prev;
         }
      }
      pc = pc->GetNextNl();
   }
} // newlines_remove_newlines
