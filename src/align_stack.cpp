/**
 * @file align_stack.cpp
 * Manages a align stack, which is just a pair of chunk stacks.
 * There can be at most 1 item per line in the stack.
 * The seqnum is actually a line counter.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "align_stack.h"
#include "prototypes.h"
#include "chunk_list.h"

/**
 * Resets the two ChunkLists and zeroes local vars.
 *
 * @param span    The row span limit
 * @param thresh  The column threshold
 */
void AlignStack::Start(int span, int thresh)
{
   LOG_FMT(LAS, "Start(%d, %d)\n", span, thresh);

   m_aligned.Reset();
   m_skipped.Reset();
   m_span        = span;
   m_thresh      = thresh;
   m_min_col     = 9999;
   m_max_col     = 0;
   m_nl_seqnum   = 0;
   m_seqnum      = 0;
   m_gap         = 0;
   m_right_align = false;
   m_star_style  = SS_IGNORE;
   m_amp_style   = SS_IGNORE;
}


/**
 * Calls Add on all the skipped items
 */
void AlignStack::ReAddSkipped()
{
   if (!m_skipped.Empty())
   {
      /* Make a copy of the ChunkStack and clear m_skipped */
      m_scratch.Set(m_skipped);
      m_skipped.Reset();

      const ChunkStack::Entry *ce;

      /* Need to add them in order so that m_nl_seqnum is correct */
      for (int idx = 0; idx < m_scratch.Len(); idx++)
      {
         ce = m_scratch.Get(idx);
         LOG_FMT(LAS, "ReAddSkipped [%d] - ", ce->m_seqnum);
         Add(ce->m_pc, ce->m_seqnum);
      }

      /* Check to see if we need to flush right away */
      NewLines(0);
   }
}


/**
 * Adds an entry to the appropriate stack.
 *
 * @param pc      The chunk
 * @param seqnum  Optional seqnum (0=assign one)
 */
void AlignStack::Add(chunk_t *start, int seqnum)
{
   /* Assign a seqnum if needed */
   if (seqnum == 0)
   {
      seqnum = m_seqnum;
   }

   chunk_t *ali;
   chunk_t *ref;
   chunk_t *tmp;
   chunk_t *prev;
   chunk_t *next;

   int col_adj = 0;  /* Amount the column is shifted for 'dangle' mode */
   int tmp_col;
   int endcol;
   int gap;

   m_last_added = 0;

   /* Check threshold limits */
   if ((m_max_col == 0) || (m_thresh == 0) ||
       (((start->column + m_gap) <= (m_max_col + m_thresh)) &&
        (((start->column + m_gap) >= (m_max_col - m_thresh)) ||
         (start->column >= m_min_col))))
   {
      /* we are adding it, so update the newline seqnum */
      if (seqnum > m_nl_seqnum)
      {
         m_nl_seqnum = seqnum;
      }

      /**
       * SS_IGNORE: no special handling of '*' or '&', only 'foo' is aligned
       *     void     foo;  // gap=5, 'foo' is aligned
       *     char *   foo;  // gap=3, 'foo' is aligned
       *     foomatic foo;  // gap=1, 'foo' is aligned
       *  The gap is the columns between 'foo' and the previous token.
       *  [void - foo], ['*' - foo], etc
       *
       * SS_INCLUDE: - space between variable and '*' or '&' is eaten
       *     void     foo;  // gap=5, 'foo' is aligned
       *     char     *foo; // gap=5, '*' is aligned
       *     foomatic foo;  // gap=1, 'foo' is aligned
       *  The gap is the columns between the first '*' or '&' before foo
       *  and the previous token. [void - foo], [char - '*'], etc
       *
       * SS_DANGLE: - space between variable and '*' or '&' is eaten
       *     void     foo;  // gap=5
       *     char    *bar;  // gap=5, as the '*' doesn't count
       *     foomatic foo;  // gap=1
       *  The gap is the columns between 'foo' and the chunk before the first
       *  '*' or '&'. [void - foo], [char - bar], etc
       *
       * If the gap < m_gap, then the column is bumped out by the difference.
       * So, if m_gap is 2, then the above would be:
       * SS_IGNORE:
       *     void      foo;  // gap=6
       *     char *    foo;  // gap=4
       *     foomatic  foo;  // gap=2
       * SS_INCLUDE:
       *     void      foo;  // gap=6
       *     char      *foo; // gap=6
       *     foomatic  foo;  // gap=2
       * SS_DANGLE:
       *     void      foo;  // gap=6
       *     char     *bar;  // gap=6, as the '*' doesn't count
       *     foomatic  foo;  // gap=2
       * Right aligned numbers:
       *     #define A    -1
       *     #define B   631
       *     #define C     3
       * Left aligned numbers:
       *     #define A     -1
       *     #define B     631
       *     #define C     3
       *
       * In the code below, pc is set to the item that is aligned.
       * In the above examples, that is 'foo', '*', '-', or 63.
       *
       * Ref is set to the last part of the type.
       * In the above examples, that is 'void', 'char', 'foomatic', 'A', or 'B'.
       *
       * The '*' and '&' can float between the two.
       *
       * If align_on_tabstop=true, then SS_DANGLE is changed to SS_INCLUDE.
       */

      if (cpd.settings[UO_align_on_tabstop].b && (m_star_style == SS_DANGLE))
      {
         m_star_style = SS_INCLUDE;
      }

      /* Find ref. Back up to the real item that is aligned. */
      prev = start;
      while (((prev = chunk_get_prev(prev)) != NULL) &&
             (chunk_is_star(prev) ||
              chunk_is_addr(prev) ||
              (chunk_is_str(prev, "(", 1) && (prev->parent_type == CT_TYPEDEF))))
      {
         /* do nothing - we want prev when this exits */
      }
      ref = prev;
      if (chunk_is_newline(ref))
      {
         ref = chunk_get_next(ref);
      }

      /* Find the item that we are going to align. */
      ali = start;
      if (m_star_style != SS_IGNORE)
      {
         /* back up to the first '*' preceding the token */
         prev = chunk_get_prev(ali);
         while (chunk_is_star(prev))
         {
            ali  = prev;
            prev = chunk_get_prev(ali);
         }
         if (chunk_is_str(prev, "(", 1) && (prev->parent_type == CT_TYPEDEF))
         {
            ali  = prev;
            prev = chunk_get_prev(ali);
         }
      }
      if (m_amp_style != SS_IGNORE)
      {
         /* back up to the first '&' preceding the token */
         prev = chunk_get_prev(ali);
         while (chunk_is_addr(prev))
         {
            ali  = prev;
            prev = chunk_get_prev(ali);
         }
      }

      /* Tighten down the spacing between ref and start */
      tmp_col = ref->column;
      tmp     = ref;
      while (tmp != start)
      {
         next     = chunk_get_next(tmp);
         tmp_col += space_col_align(tmp, next);
         if (next->column != tmp_col)
         {
            align_to_column(next, tmp_col);
         }
         tmp = next;
      }

      /* Set the column adjust and gap */
      col_adj = 0;
      gap     = 0;
      if (ref != ali)
      {
         gap = ali->column - (ref->column + ref->len());
      }
      tmp = ali;
      if (chunk_is_str(tmp, "(", 1) && (tmp->parent_type == CT_TYPEDEF))
      {
         tmp = chunk_get_next(tmp);
      }
      if ((chunk_is_star(tmp) && (m_star_style == SS_DANGLE)) ||
          (chunk_is_addr(tmp) && (m_amp_style == SS_DANGLE)))
      {
         col_adj = start->column - ali->column;
         gap     = start->column - (ref->column + ref->len());
      }

      /* See if this pushes out the max_col */
      endcol = ali->column + col_adj;
      if (gap < m_gap)
      {
         endcol += m_gap - gap;
      }

      // LOG_FMT(LSYS, "[%p] line %d pc='%s' [%s] col:%d ali='%s' [%s] col:%d ref='%s' [%s] col:%d  col_adj=%d  endcol=%d, ss=%d as=%d, gap=%d\n",
      //         this,
      //         start->orig_line,
      //         start->str.c_str(), get_token_name(start->type), start->column,
      //         ali->str.c_str(), get_token_name(ali->type), ali->column,
      //         ref->str.c_str(), get_token_name(ref->type), ref->column,
      //         col_adj, endcol, m_star_style, m_amp_style, gap);

      ali->align.col_adj = col_adj;
      ali->align.ref     = ref;
      ali->align.start   = start;
      m_aligned.Push(ali, seqnum);
      m_last_added = 1;

      LOG_FMT(LAS, "Add-[%s]: line %d, col %d, adj %d : ref=[%s] endcol=%d\n",
              ali->str.c_str(), ali->orig_line, ali->column, ali->align.col_adj,
              ref->str.c_str(), endcol);

      if (m_min_col > endcol)
      {
         m_min_col = endcol;
      }

      if (endcol > m_max_col)
      {
         LOG_FMT(LAS, "Add-aligned [%d/%d/%d]: line %d, col %d : max_col old %d, new %d - min_col %d\n",
                 seqnum, m_nl_seqnum, m_seqnum,
                 ali->orig_line, ali->column, m_max_col, endcol, m_min_col);
         m_max_col = endcol;

         /**
          * If there were any entries that were skipped, re-add them as they
          * may now be within the threshold
          */
         if (!m_skipped.Empty())
         {
            ReAddSkipped();
         }
      }
      else
      {
         LOG_FMT(LAS, "Add-aligned [%d/%d/%d]: line %d, col %d : col %d <= %d - min_col %d\n",
                 seqnum, m_nl_seqnum, m_seqnum,
                 ali->orig_line, ali->column, endcol, m_max_col, m_min_col);
      }
   }
   else
   {
      /* The threshold check failed, so add it to the skipped list */
      m_skipped.Push(start, seqnum);
      m_last_added = 2;

      LOG_FMT(LAS, "Add-skipped [%d/%d/%d]: line %d, col %d <= %d + %d\n",
              seqnum, m_nl_seqnum, m_seqnum,
              start->orig_line, start->column, m_max_col, m_thresh);
   }
}


/**
 * Adds some newline and calls Flush() if needed
 */
void AlignStack::NewLines(int cnt)
{
   if (!m_aligned.Empty())
   {
      m_seqnum += cnt;
      if (m_seqnum > (m_nl_seqnum + m_span))
      {
         LOG_FMT(LAS, "Newlines<%d>-", cnt);
         Flush();
      }
      else
      {
         LOG_FMT(LAS, "Newlines<%d>\n", cnt);
      }
   }
}


/**
 * Aligns all the stuff in m_aligned.
 * Re-adds 'newer' items in m_skipped.
 */
void AlignStack::Flush()
{
   int last_seqnum = 0;
   int idx;
   int tmp_col;
   const ChunkStack::Entry *ce = NULL;
   chunk_t                 *pc;

   LOG_FMT(LAS, "Flush (min=%d, max=%d)\n", m_min_col, m_max_col);

   m_last_added = 0;
   m_max_col    = 0;

   /* Recalculate the max_col - it may have shifted since the last Add() */
   for (idx = 0; idx < m_aligned.Len(); idx++)
   {
      pc = m_aligned.Get(idx)->m_pc;

      /* Set the column adjust and gap */
      int col_adj = 0;
      int gap     = 0;
      if (pc != pc->align.ref)
      {
         gap = pc->column - (pc->align.ref->column + pc->align.ref->len());
      }
      chunk_t *tmp = pc;
      if (chunk_is_str(tmp, "(", 1) && (tmp->parent_type == CT_TYPEDEF))
      {
         tmp = chunk_get_next(tmp);
      }
      if ((chunk_is_star(tmp) && (m_star_style == SS_DANGLE)) ||
          (chunk_is_addr(tmp) && (m_amp_style == SS_DANGLE)))
      {
         col_adj = pc->align.start->column - pc->column;
         gap     = pc->align.start->column - (pc->align.ref->column + pc->align.ref->len());
      }
      if (m_right_align)
      {
         /* Adjust the width for signed numbers */
         int start_len = pc->align.start->len();
         if (pc->align.start->type == CT_NEG)
         {
            tmp = chunk_get_next(pc->align.start);
            if ((tmp != NULL) && (tmp->type == CT_NUMBER))
            {
               start_len += tmp->len();
            }
         }
         col_adj += start_len;
      }

      pc->align.col_adj = col_adj;

      /* See if this pushes out the max_col */
      int endcol = pc->column + col_adj;
      if (gap < m_gap)
      {
         endcol += m_gap - gap;
      }
      if (endcol > m_max_col)
      {
         m_max_col = endcol;
      }
   }

   if (cpd.settings[UO_align_on_tabstop].b && (m_aligned.Len() > 1))
   {
      m_max_col = align_tab_column(m_max_col);
   }

   for (idx = 0; idx < m_aligned.Len(); idx++)
   {
      ce = m_aligned.Get(idx);
      pc = ce->m_pc;
      if (idx == 0)
      {
         pc->flags |= PCF_ALIGN_START;

         pc->align.right_align = m_right_align;
         pc->align.amp_style   = (int)m_amp_style;
         pc->align.star_style  = (int)m_star_style;
      }
      pc->align.gap  = m_gap;
      pc->align.next = m_aligned.GetChunk(idx + 1);

      /* Indent the token, taking col_adj into account */
      tmp_col = m_max_col - pc->align.col_adj;
      LOG_FMT(LAS, "%s: line %d: '%s' to col %d (adj=%d)\n", __func__,
              pc->orig_line, pc->str.c_str(), tmp_col, pc->align.col_adj);
      align_to_column(pc, tmp_col);
   }

   if (ce != NULL)
   {
      last_seqnum = ce->m_seqnum;
      m_aligned.Reset();
   }
   m_min_col = 9999;
   m_max_col = 0;

   if (m_skipped.Empty())
   {
      /* Nothing was skipped, sync the seqnums */
      m_nl_seqnum = m_seqnum;
   }
   else
   {
      /* Remove all items with seqnum < last_seqnum */
      for (idx = 0; idx < m_skipped.Len(); idx++)
      {
         if (m_skipped.Get(idx)->m_seqnum < last_seqnum)
         {
            m_skipped.Zap(idx);
         }
      }
      m_skipped.Collapse();

      /* Add all items from the skipped list */
      ReAddSkipped();
   }
}


/**
 * Resets the stack, discarding anything that was previously added
 */
void AlignStack::Reset()
{
   m_aligned.Reset();
   m_skipped.Reset();
}


/**
 * Aligns everything else and resets the lists.
 */
void AlignStack::End()
{
   if (!m_aligned.Empty())
   {
      LOG_FMT(LAS, "End-");
      Flush();
   }

   m_aligned.Reset();
   m_skipped.Reset();
}
