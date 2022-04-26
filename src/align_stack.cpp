/**
 * @file align_stack.cpp
 * Manages an align stack, which is just a pair of chunk stacks.
 * There can be at most 1 item per line in the stack.
 * The seqnum is actually a line counter.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align_stack.h"

#include "align_tab_column.h"
#include "indent.h"
#include "space.h"
#include "unc_tools.h"                   // to get stackID and get_A_Number()


constexpr static auto LCURRENT = LAS;

using namespace uncrustify;

using std::numeric_limits;


void AlignStack::Start(size_t span, int thresh)
{
   stackID = get_A_Number();   // for debugging purpose only

   // produces much more log output. Use it only debugging purpose
   //WITH_STACKID_DEBUG;

   //LOG_FMT(LAS, "AlignStack::Start(%d):m_aligned.Reset()\n", __LINE__);
   m_aligned.Reset();
   //LOG_FMT(LAS, "AlignStack::Start(%d):m_skipped.Reset()\n", __LINE__);
   m_skipped.Reset();

   if (thresh > 0)
   {
      m_absolute_thresh = false;
      m_thresh          = thresh;
   }
   else
   {
      m_absolute_thresh = true;
      m_thresh          = -thresh;
   }
   m_span        = span;
   m_min_col     = numeric_limits<size_t>::max();
   m_max_col     = 0;
   m_nl_seqnum   = 0;
   m_seqnum      = 0;
   m_gap         = 0;
   m_right_align = false;
   m_star_style  = SS_IGNORE;
   m_amp_style   = SS_IGNORE;
}


void AlignStack::ReAddSkipped()
{
   // produces much more log output. Use it only debugging purpose
   //WITH_STACKID_DEBUG;

   if (m_skipped.Empty())
   {
      return;
   }
   // Make a copy of the ChunkStack and clear m_skipped
   m_scratch.Set(m_skipped);
   //LOG_FMT(LAS, "AlignStack::ReAddSkipped(%d):m_skipped.Reset()\n", __LINE__);
   m_skipped.Reset();

   // Need to add them in order so that m_nl_seqnum is correct
   for (size_t idx = 0; idx < m_scratch.Len(); idx++)
   {
      const ChunkStack::Entry *ce = m_scratch.Get(idx);
      LOG_FMT(LAS, "AlignStack::ReAddSkipped [%zu] - ", ce->m_seqnum);
      Add(ce->m_pc, ce->m_seqnum);
   }

   NewLines(0); // Check to see if we need to flush right away
}


void AlignStack::Add(Chunk *start, size_t seqnum)
{
   // produces much more log output. Use it only debugging purpose
   //WITH_STACKID_DEBUG;
   LOG_FUNC_ENTRY();

   LOG_FMT(LAS, "AlignStack::%s(%d): Candidate is '%s': orig_line is %zu, column is %zu, type is %s, level is %zu\n",
           __func__, __LINE__, start->Text(), start->orig_line, start->column, get_token_name(start->type), start->level);
   LOG_FMT(LAS, "AlignStack::%s(%d): seqnum is %zu\n", __func__, __LINE__, seqnum);

   // Assign a seqnum if needed
   if (seqnum == 0)
   {
      LOG_FMT(LAS, "AlignStack::%s(%d): m_seqnum is %zu\n", __func__, __LINE__, m_seqnum);
      seqnum = m_seqnum;
      LOG_FMT(LAS, "AlignStack::%s(%d): seqnum is %zu\n", __func__, __LINE__, seqnum);
   }
   m_last_added = 0;

   // Threshold check should begin after
   // tighten down the spacing between ref and start

   /*
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
   log_rule_B("align_on_tabstop");

   if (  options::align_on_tabstop()
      && m_star_style == SS_DANGLE)
   {
      m_star_style = SS_INCLUDE;
   }
   LOG_FMT(LAS, "AlignStack::%s(%d): m_star_style is %s\n",
           __func__, __LINE__, get_StarStyle_name(m_star_style));
   // Find ref. Back up to the real item that is aligned.
   Chunk *prev = Chunk::NullChunkPtr;

   if (start != nullptr)
   {
      prev = start;
   }

   while (  (prev = prev->GetPrev()) != nullptr
         && prev->IsNotNullChunk()
         && (  chunk_is_ptr_operator(prev)
            || chunk_is_token(prev, CT_TPAREN_OPEN)))
   {
      // do nothing - we want prev when this exits
   }

   if (prev == nullptr)
   {
      return;
   }
   Chunk *ref = prev;

   if (chunk_is_newline(ref))
   {
      ref = ref->GetNext();
   }
   // Find the item that we are going to align.
   Chunk *ali = Chunk::NullChunkPtr;

   if (start != nullptr)
   {
      ali = start;
   }

   if (m_star_style != SS_IGNORE)
   {
      // back up to the first '*' or '^' preceding the token
      Chunk *tmp_prev = ali->GetPrev();

      while (  tmp_prev->IsStar()
            || chunk_is_msref(tmp_prev))
      {
         ali      = tmp_prev;
         tmp_prev = ali->GetPrev();
      }

      if (chunk_is_token(tmp_prev, CT_TPAREN_OPEN))
      {
         ali      = tmp_prev;
         tmp_prev = ali->GetPrev();
         // this is correct, even Coverity says:
         // CID 76021 (#1 of 1): Unused value (UNUSED_VALUE)returned_pointer: Assigning value from
         // ali->GetPrev(nav_e::ALL) to prev here, but that stored value is overwritten before it can be used.
      }
   }

   if (m_amp_style != SS_IGNORE)
   {
      // back up to the first '&' preceding the token
      Chunk *tmp_prev = ali->GetPrev();

      while (chunk_is_addr(tmp_prev))
      {
         ali      = tmp_prev;
         tmp_prev = ali->GetPrev();
      }
   }
   log_rule_B("align_keep_extra_space");

   // Tighten down the spacing between ref and start
   if (!options::align_keep_extra_space())
   {
      size_t tmp_col = ref->column;
      Chunk  *tmp    = ref;
      LOG_FMT(LAS, "AlignStack::%s(%d): tmp_col is %zu\n",
              __func__, __LINE__, tmp_col);

      while (  tmp->IsNotNullChunk()
            && tmp != start)
      {
         Chunk *next = tmp->GetNext();

         if (next->IsNotNullChunk())
         {
            LOG_FMT(LAS, "AlignStack::%s(%d): next->orig_line is %zu, orig_col is %zu, Text() '%s', level is %zu, type is %s\n",
                    __func__, __LINE__, next->orig_line, next->orig_col, next->Text(), next->level, get_token_name(next->type));
            tmp_col += space_col_align(tmp, next);
            LOG_FMT(LAS, "AlignStack::%s(%d): next->column is %zu, level is %zu, tmp_col is %zu\n",
                    __func__, __LINE__, next->column, next->level, tmp_col);

            if (next->column != tmp_col)
            {
               LOG_FMT(LAS, "AlignStack::%s(%d): Call align_to_column\n", __func__, __LINE__);
               align_to_column(next, tmp_col);
            }
         }
         tmp = next;
      }
   }

   // Check threshold limits
   if (  m_max_col == 0
      || m_thresh == 0
      || (  ((start->column + m_gap) <= (m_thresh + (m_absolute_thresh ? m_min_col : m_max_col))) // don't use subtraction here to prevent underflow
         && (  (start->column + m_gap + m_thresh) >= m_max_col                                    // change the expression to mind negative expression
            || start->column >= m_min_col)))
   {
      // we are adding it, so update the newline seqnum
      if (seqnum > m_nl_seqnum)
      {
         m_nl_seqnum = seqnum;
      }
      // Set the column adjust and gap
      size_t col_adj = 0; // Amount the column is shifted for 'dangle' mode
      size_t gap     = 0;

      if (ref != ali)
      {
         gap = ali->column - (ref->column + ref->Len());
      }
      Chunk *tmp = ali;

      if (chunk_is_token(tmp, CT_TPAREN_OPEN))
      {
         tmp = tmp->GetNext();
      }

      if (  (  tmp->IsStar()
            && m_star_style == SS_DANGLE)
         || (  chunk_is_addr(tmp)
            && m_amp_style == SS_DANGLE)
         || (  chunk_is_nullable(tmp)
            && (m_star_style == SS_DANGLE))
         || (  chunk_is_msref(tmp)
            && m_star_style == SS_DANGLE))     // TODO: add m_msref_style
      {
         col_adj = start->column - ali->column;
         gap     = start->column - (ref->column + ref->Len());
      }
      // See if this pushes out the max_col
      const size_t endcol = ali->column + col_adj
                            + (gap < m_gap ? m_gap - gap : 0);

      ali->align.col_adj = col_adj;
      ali->align.ref     = ref;
      ali->align.start   = start;
      m_aligned.Push_Back(ali, seqnum);
      m_last_added = 1;

      // Issue #2278
      if (chunk_is_token(ali, CT_PTR_TYPE))
      {
         LOG_FMT(LAS, "AlignStack::%s(%d): Add-[%s][%s]: ali->orig_line is %zu, column is %zu, type is %s, level is %zu\n",
                 __func__, __LINE__, ali->Text(), start->Text(), ali->orig_line, ali->column, get_token_name(ali->type), ali->level);
      }
      else
      {
         LOG_FMT(LAS, "AlignStack::%s(%d): Add-[%s]: ali->orig_line is %zu, column is %zu, type is %s, level is %zu\n",
                 __func__, __LINE__, ali->Text(), ali->orig_line, ali->column, get_token_name(ali->type), ali->level);
      }
      LOG_FMT(LAS, "AlignStack::%s(%d):    ali->align.col_adj is %d, ref '%s', endcol is %zu\n",
              __func__, __LINE__, ali->align.col_adj, ref->Text(), endcol);

      if (m_min_col > endcol)
      {
         m_min_col = endcol;
      }

      if (endcol > m_max_col)
      {
         LOG_FMT(LAS, "AlignStack::%s(%d): Add-aligned: seqnum is %zu, m_nl_seqnum is %zu, m_seqnum is %zu\n",
                 __func__, __LINE__, seqnum, m_nl_seqnum, m_seqnum);
         LOG_FMT(LAS, "AlignStack::%s(%d):    ali->orig_line is %zu, ali->column is %zu, max_col old is %zu, new is %zu, m_min_col is %zu\n",
                 __func__, __LINE__, ali->orig_line, ali->column, m_max_col, endcol, m_min_col);
         m_max_col = endcol;

         /*
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
         LOG_FMT(LAS, "AlignStack::%s(%d): Add-aligned: seqnum is %zu, m_nl_seqnum is %zu, m_seqnum is %zu\n",
                 __func__, __LINE__, seqnum, m_nl_seqnum, m_seqnum);
         LOG_FMT(LAS, "AlignStack::%s(%d):    ali->orig_line is %zu, ali->column is %zu, max_col old is %zu, new is %zu, m_min_col is %zu\n",
                 __func__, __LINE__, ali->orig_line, ali->column, m_max_col, endcol, m_min_col);
      }
   }
   else
   {
      // The threshold check failed, so add it to the skipped list
      m_skipped.Push_Back(start, seqnum);
      m_last_added = 2;

      LOG_FMT(LAS, "AlignStack::Add-skipped [%zu/%zu/%zu]: line %zu, col %zu <= %zu + %zu\n",
              seqnum, m_nl_seqnum, m_seqnum,
              start->orig_line, start->column, m_max_col, m_thresh);
   }
   // produces much more log output. Use it only debugging purpose
   //WITH_STACKID_DEBUG;
} // AlignStack::Add


void AlignStack::NewLines(size_t cnt)
{
   // produces much more log output. Use it only debugging purpose
   //WITH_STACKID_DEBUG;

   if (m_aligned.Empty())
   {
      //LOG_FMT(LAS, "AlignStack::Newlines(%d): nothing to do, is empty\n", __LINE__);
      return;
   }
   LOG_FMT(LAS, "AlignStack::Newlines(%d): cnt is %zu\n", __LINE__, cnt);
   m_seqnum += cnt;
   LOG_FMT(LAS, "AlignStack::Newlines(%d): m_seqnum is %zu, m_nl_seqnum is %zu, m_span is %zu\n",
           __LINE__, m_seqnum, m_nl_seqnum, m_span);

   if (m_seqnum > (m_nl_seqnum + m_span))
   {
      LOG_FMT(LAS, "AlignStack::Newlines(%d): cnt is %zu\n", __LINE__, cnt);
      Flush();
   }
}


void AlignStack::Flush()
{
   // produces much more log output. Use it only debugging purpose
   //WITH_STACKID_DEBUG;

   LOG_FMT(LAS, "AlignStack::%s(%d): Len() is %zu\n",
           __func__, __LINE__, Len());

   if (Len() > 0)
   {
      LOG_FMT(LAS, "   (min is %zu, max is %zu)\n",
              m_min_col, m_max_col);
   }

   if (Len() == 1)
   {
      // check if we have *one* typedef in the line
      Chunk *pc   = m_aligned.Get(0)->m_pc;
      Chunk *temp = pc->GetPrevType(CT_TYPEDEF, pc->level);

      if (temp->IsNotNullChunk())
      {
         if (pc->orig_line == temp->orig_line)
         {
            // reset the gap only for *this* stack
            m_gap = 1;
         }
      }
   }
   m_last_added = 0;
   m_max_col    = 0;

   // produces much more log output. Use it only debugging purpose
   //WITH_STACKID_DEBUG;
   LOG_FMT(LAS, "AlignStack::%s(%d): Debug the stack, Len is %zu\n",
           __func__, __LINE__, Len());

   for (size_t idx = 0; idx < Len(); idx++)
   {
      Chunk *pc = m_aligned.Get(idx)->m_pc;
      LOG_FMT(LAS, "AlignStack::%s(%d): idx is %zu, pc->Text() is '%s', pc->align.col_adj is %d\n",
              __func__, __LINE__, idx, pc->Text(), pc->align.col_adj);
   }

   // Recalculate the max_col - it may have shifted since the last Add()
   for (size_t idx = 0; idx < Len(); idx++)
   {
      Chunk *pc = m_aligned.Get(idx)->m_pc;

      // Set the column adjust and gap
      size_t col_adj = 0;
      size_t gap     = 0;

      if (pc != pc->align.ref)
      {
         gap = pc->column - (pc->align.ref->column + pc->align.ref->Len());
      }

      if (m_star_style == SS_DANGLE)
      {
         Chunk *tmp = (chunk_is_token(pc, CT_TPAREN_OPEN)) ? pc->GetNext() : pc;

         if (chunk_is_ptr_operator(tmp))
         {
            col_adj = pc->align.start->column - pc->column;
            gap     = pc->align.start->column - (pc->align.ref->column + pc->align.ref->Len());
         }
      }

      if (m_right_align)
      {
         // Adjust the width for signed numbers
         if (pc->align.start != nullptr)
         {
            size_t start_len = pc->align.start->Len();

            if (pc->align.start->type == CT_NEG)
            {
               Chunk *next = pc->align.start->GetNext();

               if (chunk_is_token(next, CT_NUMBER))
               {
                  start_len += next->Len();
               }
            }
            col_adj += start_len;
         }
      }
      pc->align.col_adj = col_adj;

      // See if this pushes out the max_col
      const size_t endcol = pc->column + col_adj
                            + (gap < m_gap ? m_gap - gap : 0);

      if (endcol > m_max_col)
      {
         m_max_col = endcol;
      }
   }

   log_rule_B("align_on_tabstop");

   if (  options::align_on_tabstop()
      && Len() > 1)
   {
      m_max_col = align_tab_column(m_max_col);
   }
   LOG_FMT(LAS, "AlignStack::%s(%d): Len() is %zu\n",
           __func__, __LINE__, Len());

   const ChunkStack::Entry *ce = nullptr;

   for (size_t idx = 0; idx < Len(); idx++)
   {
      ce = m_aligned.Get(idx);
      LOG_FMT(LAS, "AlignStack::%s(%d): idx is %zu, ce->m_pc->Text() is '%s', orig_line is %zu, orig_col is %zu, align.col_adj is %d\n",
              __func__, __LINE__, idx, ce->m_pc->Text(), ce->m_pc->orig_line, ce->m_pc->orig_col, ce->m_pc->align.col_adj);
   }

   for (size_t idx = 0; idx < Len(); idx++)
   {
      ce = m_aligned.Get(idx);
      Chunk        *pc = ce->m_pc;

      const size_t tmp_col = m_max_col - pc->align.col_adj;

      if (idx == 0)
      {
         if (  m_skip_first
            && pc->column != tmp_col)
         {
            LOG_FMT(LAS, "AlignStack::%s(%d): orig_line is %zu, orig_col is %zu, dropping first item due to skip_first\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col);
            m_skip_first = false;
            m_aligned.Pop_Front();
            Flush();
            m_skip_first = true;
            return;
         }
         chunk_flags_set(pc, PCF_ALIGN_START);

         pc->align.right_align = m_right_align;
         pc->align.amp_style   = m_amp_style;
         pc->align.star_style  = m_star_style;
      }
      pc->align.gap  = m_gap;
      pc->align.next = m_aligned.GetChunk(idx + 1);

      // Indent the token, taking col_adj into account
      LOG_FMT(LAS, "AlignStack::%s(%d): orig_line is %zu, orig_col is %zu, Text() '%s', set to col %zu (adj is %d)\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), tmp_col, pc->align.col_adj);
      align_to_column(pc, tmp_col);
   }

   size_t last_seqnum = 0;

   if (ce != nullptr)
   {
      last_seqnum = ce->m_seqnum;
      //LOG_FMT(LAS, "AlignStack::Flush(%d):m_aligned.Reset()\n", __LINE__);
      m_aligned.Reset();
   }
   m_min_col = numeric_limits<size_t>::max(); // use unrealistic high numbers
   m_max_col = 0;                             // as start value

   if (m_skipped.Empty())
   {
      // Nothing was skipped, sync the sequence numbers
      m_nl_seqnum = m_seqnum;
   }
   else
   {
      // Remove all items with seqnum < last_seqnum
      for (size_t idx = 0; idx < m_skipped.Len(); idx++)
      {
         if (m_skipped.Get(idx)->m_seqnum < last_seqnum)
         {
            m_skipped.Zap(idx);
         }
      }

      m_skipped.Collapse();

      ReAddSkipped(); // Add all items from the skipped list
   }
} // AlignStack::Flush


void AlignStack::Reset()
{
   //WITH_STACKID_DEBUG;
   //LOG_FMT(LAS, "AlignStack::Reset(%d):m_aligned.Reset()\n", __LINE__);
   m_aligned.Reset();
   //LOG_FMT(LAS, "AlignStack::Reset(%d):m_skipped.Reset()\n", __LINE__);
   m_skipped.Reset();
}


void AlignStack::End()
{
   //WITH_STACKID_DEBUG;

   if (!m_aligned.Empty())
   {
      //LOG_FMT(LAS, "AlignStack::End(%d):\n", __LINE__);
      Flush();
   }
   //LOG_FMT(LAS, "AlignStack::End(%d):m_aligned.Reset()\n", __LINE__);
   m_aligned.Reset();
   //LOG_FMT(LAS, "AlignStack::End(%d):m_skipped.Reset()\n", __LINE__);
   m_skipped.Reset();
}


size_t AlignStack::Len()
{
   return(m_aligned.Len());
}


void AlignStack::Debug()
{
   //WITH_STACKID_DEBUG;

   size_t length = Len();

   if (length > 0)
   {
      LOG_FMT(LAS, "AlignStack::%s(%d): Debug the stack, Len is %zu\n",
              __func__, __LINE__, Len());

      for (size_t idx = 0; idx < length; idx++)
      {
         Chunk *pc = m_aligned.Get(idx)->m_pc;

         if (chunk_is_token(pc, CT_PTR_TYPE))
         {
            LOG_FMT(LAS, "AlignStack::%s(%d): idx is %zu, [%s][%s]: orig_line is %zu, orig_col is %zu, type is %s, level is %zu, brace_level is %zu\n",
                    __func__, __LINE__, idx, pc->Text(), pc->next->Text(), pc->orig_line, pc->orig_col, get_token_name(pc->type), pc->level, pc->brace_level);
         }
         else
         {
            LOG_FMT(LAS, "AlignStack::%s(%d): idx is %zu, [%s]: orig_line is %zu, orig_col is %zu, type is %s, level is %zu, brace_level is %zu\n",
                    __func__, __LINE__, idx, pc->Text(), pc->orig_line, pc->orig_col, get_token_name(pc->type), pc->level, pc->brace_level);
         }
      }
   }
}


const char *AlignStack::get_StarStyle_name(StarStyle star_style)
{
   switch (star_style)
   {
   case StarStyle::SS_IGNORE:
      return("SS_IGNORE");

   case StarStyle::SS_INCLUDE:
      return("SS_INCLUDE");

   case StarStyle::SS_DANGLE:
      return("SS_DANGLE");
   }
   return("?????");
} // get_StarStyle_name
