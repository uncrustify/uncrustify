/**
 * @file del_between.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/del_between.h"

#include "chunk.h"
#include "indent.h"
#include "mark_change.h"
#include "space.h"


void newline_del_between(Chunk *start, Chunk *end)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNEWLINE, "%s(%d): start->Text() is '%s', orig line is %zu, orig col is %zu\n",
           __func__, __LINE__, start->Text(), start->GetOrigLine(), start->GetOrigCol());
   LOG_FMT(LNEWLINE, "%s(%d): and end->Text() is '%s', orig line is %zu, orig col is %zu: preproc=%c/%c\n",
           __func__, __LINE__, end->Text(), end->GetOrigLine(), end->GetOrigCol(),
           start->TestFlags(PCF_IN_PREPROC) ? 'y' : 'n',
           end->TestFlags(PCF_IN_PREPROC) ? 'y' : 'n');
   log_func_stack_inline(LNEWLINE);

   // Can't remove anything if the preproc status differs
   if (!start->IsSamePreproc(end))
   {
      return;
   }
   Chunk *pc           = start;
   bool  start_removed = false;

   do
   {
      Chunk *next = pc->GetNext();

      if (pc->IsNewline())
      {
         Chunk *prev = pc->GetPrev();

         if (  (  !prev->IsComment()
               && !next->IsComment())
            || prev->IsNewline()
            || next->IsNewline())
         {
            if (pc->SafeToDeleteNl())
            {
               if (pc == start)
               {
                  start_removed = true;
               }
               Chunk::Delete(pc);
               MARK_CHANGE();

               if (prev->IsNotNullChunk())
               {
                  size_t temp = space_col_align(prev, next);
                  align_to_column(next, prev->GetColumn() + temp);
               }
            }
         }
         else
         {
            if (pc->GetNlCount() > 1)
            {
               pc->SetNlCount(1);
               MARK_CHANGE();
            }
         }
      }
      pc = next;
   } while (pc != end);

   if (  !start_removed
      && end->IsString("{")
      && (  start->IsString(")")
         || start->Is(CT_DO)
         || start->Is(CT_ELSE)))
   {
      end->MoveAfter(start);
   }
} // newline_del_between
