/**
 * @file tools.cpp
 *
 * @author  Guy Maurel
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align/tools.h"

#include "chunk.h"
#include "space.h"
#include "uncrustify.h"


Chunk *skip_c99_array(Chunk *sq_open)
{
   if (sq_open->Is(CT_SQUARE_OPEN))
   {
      Chunk *tmp = sq_open->GetClosingParen()->GetNextNc();

      if (tmp->Is(CT_ASSIGN))
      {
         return(tmp->GetNextNc());
      }
   }
   return(Chunk::NullChunkPtr);
} // skip_c99_array


Chunk *scan_ib_line(Chunk *start)
{
   LOG_FUNC_ENTRY();
   Chunk  *prev_match = Chunk::NullChunkPtr;
   size_t idx         = 0;

   // Skip past C99 "[xx] =" stuff
   Chunk *tmp = skip_c99_array(start);

   if (tmp->IsNotNullChunk())
   {
      start->SetParentType(CT_TSQUARE);
      start            = tmp;
      cpd.al_c99_array = true;
   }
   Chunk *pc = start;

   if (pc->IsNotNullChunk())
   {
      LOG_FMT(LSIB, "%s(%d): start: orig line is %zu, orig col is %zu, column is %zu, type is %s\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetColumn(), get_token_name(pc->GetType()));
   }
   else
   {
      pc = Chunk::NullChunkPtr;
   }

   while (  pc->IsNotNullChunk()
         && !pc->IsNewline()
         && pc->GetLevel() >= start->GetLevel())
   {
      //LOG_FMT(LSIB, "%s:     '%s'   col %d/%d line %zu\n", __func__,
      //        pc->Text(), pc->GetColumn(), pc->GetOrigCol(), pc->GetOrigLine());

      Chunk *next = pc->GetNext();

      if (  next->IsNullChunk()
         || next->IsComment())
      {
         // do nothing
      }
      else if (  pc->Is(CT_ASSIGN)
              || pc->Is(CT_BRACE_OPEN)
              || pc->Is(CT_BRACE_CLOSE)
              || pc->Is(CT_COMMA))
      {
         size_t token_width = space_col_align(pc, next);

         // TODO: need to handle missing structure defs? ie NULL vs { ... } ??

         // Is this a new entry?
         if (idx >= cpd.al_cnt)
         {
            if (idx == 0)
            {
               LOG_FMT(LSIB, "%s(%d): Prepare the 'idx's\n", __func__, __LINE__);
            }
            LOG_FMT(LSIB, "%s(%d):   New idx is %2.1zu, pc->GetColumn() is %2.1zu, Text() '%s', token_width is %zu, type is %s\n",
                    __func__, __LINE__, idx, pc->GetColumn(), pc->Text(), token_width, get_token_name(pc->GetType()));
            cpd.al[cpd.al_cnt].type = pc->GetType();
            cpd.al[cpd.al_cnt].col  = pc->GetColumn();
            cpd.al[cpd.al_cnt].len  = token_width;
            cpd.al[cpd.al_cnt].ref  = pc;                  // Issue #3786
            cpd.al_cnt++;

            if (cpd.al_cnt == uncrustify::limits::AL_SIZE)
            {
               fprintf(stderr, "Number of 'entry' to be aligned is too big for the current value %d,\n",
                       uncrustify::limits::AL_SIZE);
               fprintf(stderr, "at line %zu, column %zu.\n",
                       pc->GetOrigLine(), pc->GetOrigCol());
               fprintf(stderr, "Please make a report.\n");
               log_flush(true);
               exit(EX_SOFTWARE);
            }
            idx++;
         }
         else
         {
            // expect to match stuff
            if (cpd.al[idx].type == pc->GetType())
            {
               LOG_FMT(LSIB, "%s(%d):   Match? idx is %2.1zu, orig line is %2.1zu, column is %2.1zu, token_width is %zu, type is %s\n",
                       __func__, __LINE__, idx, pc->GetOrigLine(), pc->GetColumn(), token_width, get_token_name(pc->GetType()));

               // Shift out based on column
               if (prev_match->IsNullChunk())
               {
                  if (pc->GetColumn() > cpd.al[idx].col)
                  {
                     LOG_FMT(LSIB, "%s(%d): [ pc column (%zu) > cpd.al[%zu].col(%zu) ] \n",
                             __func__, __LINE__, pc->GetColumn(), idx, cpd.al[idx].col);

                     ib_shift_out(idx, pc->GetColumn() - cpd.al[idx].col);
                     cpd.al[idx].col = pc->GetColumn();
                  }
               }
               else if (idx > 0)
               {
                  LOG_FMT(LSIB, "%s(%d): prev_match '%s', orig line is %zu, orig col is %zu\n",
                          __func__, __LINE__, prev_match->Text(), prev_match->GetOrigLine(), prev_match->GetOrigCol());
                  int min_col_diff = pc->GetColumn() - prev_match->GetColumn();
                  int cur_col_diff = cpd.al[idx].col - cpd.al[idx - 1].col;

                  if (cur_col_diff < min_col_diff)
                  {
                     LOG_FMT(LSIB, "%s(%d):   pc orig line is %zu\n",
                             __func__, __LINE__, pc->GetOrigLine());
                     ib_shift_out(idx, min_col_diff - cur_col_diff);
                  }
               }
               LOG_FMT(LSIB, "%s(%d): at ende of the loop: now is col %zu, len is %zu\n",
                       __func__, __LINE__, cpd.al[idx].col, cpd.al[idx].len);
               idx++;
            }
         }
         prev_match = pc;
      }
      pc = pc->GetNextNc();
   }
   return(pc);
} // scan_ib_line


void ib_shift_out(size_t idx, size_t num)
{
   while (idx < cpd.al_cnt)
   {
      bool  is_empty = false;                  // Issue #3786
      Chunk *tmp     = cpd.al[idx].ref;

      if (tmp->Is(CT_BRACE_CLOSE))
      {
         Chunk *pre = tmp->GetPrev();

         if (pre->Is(CT_COMMA))
         {
            is_empty = true;
         }
      }

      if (!is_empty)
      {
         cpd.al[idx].col += num;
      }
      idx++;
   }
} // ib_shift_out


Chunk *step_back_over_member(Chunk *pc)
{
   Chunk *tmp = pc->GetPrevNcNnl();

   // Skip over any class stuff: bool CFoo::bar()
   while (tmp->Is(CT_DC_MEMBER))
   {
      pc  = tmp->GetPrevNcNnl();
      tmp = pc->GetPrevNcNnl();
   }
   return(pc);
} // step_back_over_member
