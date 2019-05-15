/**
 * @file align_tools.cpp
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align_tools.h"

#include "space.h"
#include "uncrustify.h"


chunk_t *skip_c99_array(chunk_t *sq_open)
{
   if (chunk_is_token(sq_open, CT_SQUARE_OPEN))
   {
      chunk_t *tmp = chunk_get_next_nc(chunk_skip_to_match(sq_open));

      if (chunk_is_token(tmp, CT_ASSIGN))
      {
         return(chunk_get_next_nc(tmp));
      }
   }
   return(nullptr);
} // skip_c99_array


chunk_t *scan_ib_line(chunk_t *start, bool first_pass)
{
   UNUSED(first_pass);
   LOG_FUNC_ENTRY();
   chunk_t *prev_match = nullptr;
   size_t  idx         = 0;

   // Skip past C99 "[xx] =" stuff
   chunk_t *tmp = skip_c99_array(start);
   if (tmp != nullptr)
   {
      set_chunk_parent(start, CT_TSQUARE);
      start            = tmp;
      cpd.al_c99_array = true;
   }
   chunk_t *pc = start;

   if (pc != nullptr)
   {
      LOG_FMT(LSIB, "%s(%d): start: orig_line is %zu, orig_col is %zu, column is %zu, type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->column, get_token_name(pc->type));
   }

   while (  pc != nullptr
         && !chunk_is_newline(pc)
         && pc->level >= start->level)
   {
      //LOG_FMT(LSIB, "%s:     '%s'   col %d/%d line %zu\n", __func__,
      //        pc->text(), pc->column, pc->orig_col, pc->orig_line);

      chunk_t *next = chunk_get_next(pc);
      if (next == nullptr || chunk_is_comment(next))
      {
         // do nothing
      }
      else if (  chunk_is_token(pc, CT_ASSIGN)
              || chunk_is_token(pc, CT_BRACE_OPEN)
              || chunk_is_token(pc, CT_BRACE_CLOSE)
              || chunk_is_token(pc, CT_COMMA))
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
            LOG_FMT(LSIB, "%s(%d):   New idx is %2.1zu, pc->column is %2.1zu, text() '%s', token_width is %zu, type is %s\n",
                    __func__, __LINE__, idx, pc->column, pc->text(), token_width, get_token_name(pc->type));
            cpd.al[cpd.al_cnt].type = pc->type;
            cpd.al[cpd.al_cnt].col  = pc->column;
            cpd.al[cpd.al_cnt].len  = token_width;
            cpd.al_cnt++;
            if (cpd.al_cnt == AL_SIZE)
            {
               fprintf(stderr, "Number of 'entry' to be aligned is too big for the current value %d,\n", AL_SIZE);
               fprintf(stderr, "at line %zu, column %zu.\n", pc->orig_line, pc->orig_col);
               fprintf(stderr, "Please make a report.\n");
               log_flush(true);
               exit(EX_SOFTWARE);
            }
            idx++;
         }
         else
         {
            // expect to match stuff
            if (cpd.al[idx].type == pc->type)
            {
               LOG_FMT(LSIB, "%s(%d):   Match? idx is %2.1zu, orig_line is %2.1zu, column is %2.1zu, token_width is %zu, type is %s\n",
                       __func__, __LINE__, idx, pc->orig_line, pc->column, token_width, get_token_name(pc->type));

               // Shift out based on column
               if (prev_match == nullptr)
               {
                  if (pc->column > cpd.al[idx].col)
                  {
                     LOG_FMT(LSIB, "%s(%d): [ pc->column (%zu) > cpd.al[%zu].col(%zu) ] \n",
                             __func__, __LINE__, pc->column, idx, cpd.al[idx].col);

                     ib_shift_out(idx, pc->column - cpd.al[idx].col);
                     cpd.al[idx].col = pc->column;
                  }
               }
               else if (idx > 0)
               {
                  LOG_FMT(LSIB, "%s(%d):   prev_match '%s', prev_match->orig_line is %zu, prev_match->orig_col is %zu\n",
                          __func__, __LINE__, prev_match->text(), prev_match->orig_line, prev_match->orig_col);
                  int min_col_diff = pc->column - prev_match->column;
                  int cur_col_diff = cpd.al[idx].col - cpd.al[idx - 1].col;
                  if (cur_col_diff < min_col_diff)
                  {
                     LOG_FMT(LSIB, "%s(%d):   pc->orig_line is %zu\n",
                             __func__, __LINE__, pc->orig_line);
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
      pc = chunk_get_next_nc(pc);
   }
   return(pc);
} // scan_ib_line


void ib_shift_out(size_t idx, size_t num)
{
   while (idx < cpd.al_cnt)
   {
      cpd.al[idx].col += num;
      idx++;
   }
} // ib_shift_out


chunk_t *step_back_over_member(chunk_t *pc)
{
   chunk_t *tmp;

   // Skip over any class stuff: bool CFoo::bar()
   while (  ((tmp = chunk_get_prev_ncnl(pc)) != nullptr)
         && chunk_is_token(tmp, CT_DC_MEMBER))
   {
      // TODO: verify that we are pointing at something sane?
      pc = chunk_get_prev_ncnl(tmp);
   }
   return(pc);
} // step_back_over_member
