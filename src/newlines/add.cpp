/**
 * @file add.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/add.h"

#include "chunk.h"
#include "mark_change.h"
#include "newlines/one_liner.h"
#include "newlines/setup_newline_add.h"
#include "uncrustify.h"


Chunk *newline_add_after(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   if (pc->IsNullChunk())
   {
      return(Chunk::NullChunkPtr);
   }
   Chunk *next = pc->GetNextNvb();

   if (next->IsNewline())
   {
      // Already has a newline after this chunk
      return(next);
   }
   LOG_FMT(LNEWLINE, "%s(%d): '%s' on line %zu",
           __func__, __LINE__, pc->Text(), pc->GetOrigLine());
   log_func_stack_inline(LNEWLINE);

   Chunk nl;

   nl.SetOrigLine(pc->GetOrigLine());
   nl.SetOrigCol(pc->GetOrigCol());
   setup_newline_add(pc, &nl, next);

   MARK_CHANGE();
   // TO DO: check why the next statement is necessary
   nl.SetOrigCol(pc->GetOrigCol());
   nl.SetPpLevel(pc->GetPpLevel());
   return(nl.CopyAndAddAfter(pc));
} // newline_add_after


Chunk *newline_add_before(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   Chunk nl;
   Chunk *prev = pc->GetPrevNvb();

   if (prev->IsNewline())
   {
      // Already has a newline before this chunk
      return(prev);
   }
   LOG_FMT(LNEWLINE, "%s(%d): Text() '%s', on orig line is %zu, orig col is %zu, pc column is %zu",
           __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol(), pc->GetColumn());
   log_func_stack_inline(LNEWLINE);

   setup_newline_add(prev, &nl, pc);
   nl.SetOrigCol(pc->GetOrigCol());
   nl.SetPpLevel(pc->GetPpLevel());
   LOG_FMT(LNEWLINE, "%s(%d): nl column is %zu\n",
           __func__, __LINE__, nl.GetColumn());

   MARK_CHANGE();
   return(nl.CopyAndAddBefore(pc));
} // newline_add_before


Chunk *newline_add_between(Chunk *start, Chunk *end)
{
   LOG_FUNC_ENTRY();

   if (  start->IsNullChunk()
      || end->IsNullChunk()
      || end->Is(CT_IGNORED))
   {
      return(Chunk::NullChunkPtr);
   }
   LOG_FMT(LNEWLINE, "%s(%d): start->Text() is '%s', type is %s, orig line is %zu, orig col is %zu\n",
           __func__, __LINE__, start->Text(), get_token_name(start->GetType()),
           start->GetOrigLine(), start->GetOrigCol());
   LOG_FMT(LNEWLINE, "%s(%d): and end->Text() is '%s', orig line is %zu, orig col is %zu\n  ",
           __func__, __LINE__, end->Text(), end->GetOrigLine(), end->GetOrigCol());
   log_func_stack_inline(LNEWLINE);

   // Back-up check for one-liners (should never be true!)
   if (!one_liner_nl_ok(start))
   {
      return(Chunk::NullChunkPtr);
   }

   /*
    * Scan for a line break, if there is a line break between start and end
    * we won't add another one
    */
   for (Chunk *pc = start; pc != end; pc = pc->GetNext())
   {
      if (pc->IsNewline())
      {
         return(pc);
      }
   }

   /*
    * If the second one is a brace open, then check to see
    * if a comment + newline follows
    */
   if (end->Is(CT_BRACE_OPEN))
   {
      Chunk *pc = end->GetNext();

      if (pc->IsComment())
      {
         pc = pc->GetNext();

         if (pc->IsNewline())
         {
            // are there some more (comment + newline)s ?
            Chunk *pc1 = end->GetNextNcNnl();

            if (!pc1->IsNewline())
            {
               // yes, go back
               Chunk *pc2 = pc1->GetPrev();
               pc = pc2;
            }

            if (end == pc)
            {
               LOG_FMT(LNEWLINE, "%s(%d): pc1 and pc are identical\n",
                       __func__, __LINE__);
            }
            else
            {
               // Move the open brace to after the newline
               end->MoveAfter(pc);
            }
            LOG_FMT(LNEWLINE, "%s(%d):\n", __func__, __LINE__);
            newline_add_after(end);
            return(pc);
         }
         else                  // Issue #3873
         {
            LOG_FMT(LNEWLINE, "%s(%d):\n", __func__, __LINE__);
         }
      }
      else
      {
         LOG_FMT(LNEWLINE, "%s(%d):\n", __func__, __LINE__);
      }
   }
   else
   {
      LOG_FMT(LNEWLINE, "%s(%d):\n", __func__, __LINE__);
   }
   Chunk *tmp = newline_add_before(end);

   return(tmp);
} // newline_add_between
