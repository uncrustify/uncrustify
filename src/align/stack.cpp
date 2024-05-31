/**
 * @file stack.cpp
 * Manages an align stack, which is just a pair of chunk stacks.
 * There can be at most 1 item per line in the stack.
 * The seqnum is actually a line counter.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "align/stack.h"

#include "align/tab_column.h"
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
   // WITH_STACKID_DEBUG;

   //LOG_FMT(LAS, "AlignStack::Start(%d):m_aligned.Reset()\n", __LINE__);
   m_aligned.Reset();
   //LOG_FMT(LAS, "AlignStack::Start(%d):m_skipped.Reset()\n", __LINE__);
   m_skipped.Reset();

   if (thresh >= 0)
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
} // AlignStack::Start


void AlignStack::ReAddSkipped()
{
   // produces much more log output. Use it only debugging purpose
   // WITH_STACKID_DEBUG;

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
} // AlignStack::ReAddSkipped


void AlignStack::Add(Chunk *start, size_t seqnum)
{
   // produces much more log output. Use it only debugging purpose
   // WITH_STACKID_DEBUG;
   LOG_FUNC_ENTRY();

   LOG_FMT(LAS, "AlignStack::%s(%d): Candidate '%s': orig line %zu, column %zu, type %s, level %zu\n",
           __func__, __LINE__, start->Text(), start->GetOrigLine(), start->GetColumn(), get_token_name(start->GetType()), start->GetLevel());
   LOG_FMT(LAS, "AlignStack::%s(%d): seqnum %zu m_seqnum %zu\n", __func__, __LINE__, seqnum, m_seqnum);

   // Assign a seqnum if needed
   if (seqnum == 0)
   {
      seqnum = m_seqnum;
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

   if (  options::align_on_tabstop()
      && m_star_style == SS_DANGLE)
   {
      m_star_style = SS_INCLUDE;
   }
   LOG_FMT(LAS, "AlignStack::%s(%d): m_star_style is %s\n",
           __func__, __LINE__, get_StarStyle_name(m_star_style));
   // Find ref. Back up to the real item that is aligned.
   Chunk *prev = start->GetPrev();

   while (  prev->IsPointerOperator()
         || prev->Is(CT_TPAREN_OPEN))
   {
      prev = prev->GetPrev();
   }

   if (prev->IsNullChunk())
   {
      LOG_FMT(LAS, "AlignStack::%s(%d): 'ref' chunk not found. Do not add.\n",
              __func__, __LINE__);
      return;
   }
   Chunk *ref = prev;

   if (ref->IsNewline())
   {
      ref = ref->GetNext();
   }
   // Find the item that we are going to align.
   Chunk *ali = start;

   if (m_star_style != SS_IGNORE)
   {
      // back up to the first '*' or '^' preceding the token
      Chunk *tmp_prev = ali->GetPrev();

      while (  tmp_prev->IsStar()
            || tmp_prev->IsMsRef())
      {
         ali      = tmp_prev;
         tmp_prev = ali->GetPrev();
      }

      if (tmp_prev->Is(CT_TPAREN_OPEN))
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

      while (tmp_prev->IsAddress())
      {
         ali      = tmp_prev;
         tmp_prev = ali->GetPrev();
      }
   }
   LOG_FMT(LAS, "AlignStack::%s(%3d): 'ref' orig line %zu, orig col %zu, text '%s', level %zu, type %s\n",
           __func__, __LINE__, ref->GetOrigLine(), ref->GetOrigCol(), ref->Text(), ref->GetLevel(), get_token_name(ref->GetType()));
   LOG_FMT(LAS, "AlignStack::%s(%3d): 'ali' orig line %zu, orig col %zu, text '%s', level %zu, type %s\n",
           __func__, __LINE__, ali->GetOrigLine(), ali->GetOrigCol(), ali->Text(), ali->GetLevel(), get_token_name(ali->GetType()));
   log_rule_B("align_keep_extra_space");

   // Tighten down the spacing between ref and start
   if (!options::align_keep_extra_space())
   {
      size_t tmp_col = ref->GetColumn();
      Chunk  *tmp    = ref;
      LOG_FMT(LAS, "AlignStack::%s(%3d): tmp_col is %zu\n",
              __func__, __LINE__, tmp_col);

      while (  tmp->IsNotNullChunk()
            && tmp != start)
      {
         Chunk *next = tmp->GetNext();

         if (next->IsNotNullChunk())
         {
            LOG_FMT(LAS, "AlignStack::%s(%3d): 'next' orig line %zu, orig col %zu, text '%s', level %zu, type %s\n",
                    __func__, __LINE__, next->GetOrigLine(), next->GetOrigCol(), next->Text(), next->GetLevel(), get_token_name(next->GetType()));
            tmp_col += space_col_align(tmp, next);
            LOG_FMT(LAS, "AlignStack::%s(%3d): 'next' column %zu, level %zu, tmp_col %zu\n",
                    __func__, __LINE__, next->GetColumn(), next->GetLevel(), tmp_col);

            if (next->GetColumn() != tmp_col)
            {
               LOG_FMT(LAS, "AlignStack::%s(%3d): call align_to_column\n", __func__, __LINE__);
               align_to_column(next, tmp_col);
            }
         }
         tmp = next;
      }
   }
   LOG_FMT(LAS, "AlignStack::%s(%3d): m_min_col %zu, m_max_col %zu, start_col %zu, m_thresh %zu, m_gap %zu\n",
           __func__, __LINE__, m_min_col, m_max_col, start->GetColumn(), m_thresh, m_gap);

   // Check threshold limits
   if (  m_max_col == 0
      || m_thresh == 0
      || (  ((start->GetColumn() + m_gap) <= (m_thresh + (m_absolute_thresh ? m_min_col : m_max_col))) // don't use subtraction here to prevent underflow
         && (  (start->GetColumn() + m_gap + m_thresh) >= m_max_col                                    // change the expression to mind negative expression
            || start->GetColumn() >= m_min_col)))
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
         gap = ali->GetColumn() - (ref->GetColumn() + ref->Len());
      }
      Chunk *tmp = ali;

      if (tmp->Is(CT_TPAREN_OPEN))
      {
         tmp = tmp->GetNext();
      }

      if (  (  tmp->IsStar()
            && m_star_style == SS_DANGLE)
         || (  tmp->IsAddress()
            && m_amp_style == SS_DANGLE)
         || (  tmp->IsNullable()
            && (m_star_style == SS_DANGLE))
         || (  tmp->IsMsRef()
            && m_star_style == SS_DANGLE))     // TODO: add m_msref_style
      {
         col_adj = start->GetColumn() - ali->GetColumn();
         gap     = start->GetColumn() - (ref->GetColumn() + ref->Len());
      }
      // See if this pushes out the max_col
      const size_t endcol = ali->GetColumn() + col_adj
                            + (gap < m_gap ? m_gap - gap : 0);

      ali->AlignData().col_adj = col_adj;
      ali->AlignData().ref     = ref;
      ali->AlignData().start   = start;
      m_aligned.Push_Back(ali, seqnum);
      m_last_added = 1;

      // Issue #2278
      if (ali->Is(CT_PTR_TYPE))
      {
         LOG_FMT(LAS, "AlignStack::%s(%d): add [%s][%s]: 'ali' orig line %zu, column %zu, type %s, level %zu\n",
                 __func__, __LINE__, ali->Text(), start->Text(), ali->GetOrigLine(), ali->GetColumn(), get_token_name(ali->GetType()), ali->GetLevel());
      }
      else
      {
         LOG_FMT(LAS, "AlignStack::%s(%3d): add [%s]: 'ali' orig line %zu, column %zu, type %s, level %zu\n",
                 __func__, __LINE__, ali->Text(), ali->GetOrigLine(), ali->GetColumn(), get_token_name(ali->GetType()), ali->GetLevel());
      }
      LOG_FMT(LAS, "AlignStack::%s(%3d): 'ali' alignment col_adj %d, ref '%s', endcol %zu\n",
              __func__, __LINE__, ali->GetAlignData().col_adj, ref->Text(), endcol);

      if (m_min_col > endcol)
      {
         m_min_col = endcol;
      }
      LOG_FMT(LAS, "AlignStack::%s(%3d): add aligned: seqnum %zu, m_nl_seqnum %zu, m_seqnum %zu\n",
              __func__, __LINE__, seqnum, m_nl_seqnum, m_seqnum);
      LOG_FMT(LAS, "AlignStack::%s(%3d): 'ali' orig line %zu, column %zu, m_min_col %zu, max_col old/new %zu/%zu\n",
              __func__, __LINE__, ali->GetOrigLine(), ali->GetColumn(), m_min_col, m_max_col, endcol);

      if (endcol > m_max_col)
      {
         m_max_col = endcol;

         /*
          * If there were any entries that were skipped, re-add them as they
          * may now be within the threshold
          */
         if (!m_skipped.Empty())
         {
            LOG_FMT(LAS, "AlignStack::%s(%3d): ReAddSkipped()\n", __func__, __LINE__);
            ReAddSkipped();
         }
      }
   }
   else
   {
      // The threshold check failed, so add it to the skipped l
      m_skipped.Push_Back(start, seqnum);
      m_last_added = 2;

      LOG_FMT(LAS, "AlignStack::add skipped [%zu/%zu/%zu]: line %zu, col %zu <= %zu + %zu\n",
              seqnum, m_nl_seqnum, m_seqnum,
              start->GetOrigLine(), start->GetColumn(), m_max_col, m_thresh);
   }
   LOG_FMT(LAS, "AlignStack::%s(%3d): end of add\n", __func__, __LINE__);
   // produces much more log output. Use it only debugging purpose
   // WITH_STACKID_DEBUG;
   // AlignStack::Debug();
} // AlignStack::Add


void AlignStack::NewLines(size_t cnt)
{
   // produces much more log output. Use it only debugging purpose
   // WITH_STACKID_DEBUG;

   if (m_aligned.Empty())
   {
      //LOG_FMT(LAS, "AlignStack::Newlines(%d): nothing to do, empty\n", __LINE__);
      return;
   }
   LOG_FMT(LAS, "AlignStack::Newlines(%d): cnt %zu\n", __LINE__, cnt);
   m_seqnum += cnt;
   LOG_FMT(LAS, "AlignStack::Newlines(%d): m_seqnum %zu, m_nl_seqnum %zu, m_span %zu\n",
           __LINE__, m_seqnum, m_nl_seqnum, m_span);

   if (m_seqnum > (m_nl_seqnum + m_span))
   {
      LOG_FMT(LAS, "AlignStack::Newlines(%d): cnt %zu\n", __LINE__, cnt);
      Flush();
   }
} // AlignStack::NewLines


void AlignStack::Flush()
{
   // produces much more log output. Use it only debugging purpose
   // WITH_STACKID_DEBUG;

   LOG_FMT(LAS, "AlignStack::%s(%d): Len() is %zu\n",
           __func__, __LINE__, Len());

   if (Len() > 0)
   {
      LOG_FMT(LAS, "AlignStack::%s(%d):   min is %zu, max is %zu\n",
              __func__, __LINE__, m_min_col, m_max_col);
   }
   else
   {
      return;
   }

   if (Len() == 1)
   {
      // check if we have *one* typedef in the line
      Chunk *pc   = m_aligned.Get(0)->m_pc;
      Chunk *temp = pc->GetPrevType(CT_TYPEDEF, pc->GetLevel());

      if (temp->IsNotNullChunk())
      {
         if (pc->GetOrigLine() == temp->GetOrigLine())
         {
            // reset the gap only for *this* stack
            m_gap = 1;
         }
      }
   }
   m_last_added = 0;
   m_max_col    = 0;

   // produces much more log output. Use it only debugging purpose
   LOG_FMT(LAS, "AlignStack::%s(%d): Debug the stack, Len() is %zu\n",
           __func__, __LINE__, Len());

   for (size_t idx = 0; idx < Len(); idx++)
   {
      Chunk *pc = m_aligned.Get(idx)->m_pc;
      LOG_FMT(LAS, "AlignStack::%s(%d):   idx is %zu, pc->Text() is '%s', orig line is %zu, orig col is %zu, alignment col_adj is %d\n",
              __func__, __LINE__, idx, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol(), pc->GetAlignData().col_adj);
   }

   // Recalculate the max_col - it may have shifted since the last Add()
   for (size_t idx = 0; idx < Len(); idx++)
   {
      Chunk *pc = m_aligned.Get(idx)->m_pc;

      // Set the column adjust and gap
      size_t col_adj = 0;
      size_t gap     = 0;

      if (pc != pc->GetAlignData().ref)
      {
         gap = pc->GetColumn() - (pc->GetAlignData().ref->GetColumn() + pc->GetAlignData().ref->Len());
      }

      if (m_star_style == SS_DANGLE)
      {
         Chunk *tmp = (pc->Is(CT_TPAREN_OPEN)) ? pc->GetNext() : pc;

         if (tmp->IsPointerOperator())
         {
            col_adj = pc->GetAlignData().start->GetColumn() - pc->GetColumn();
            gap     = pc->GetAlignData().start->GetColumn() - (pc->GetAlignData().ref->GetColumn() + pc->GetAlignData().ref->Len());
         }
      }

      if (m_right_align)
      {
         // Adjust the width for signed numbers
         if (pc->GetAlignData().start->IsNotNullChunk())
         {
            size_t start_len = pc->GetAlignData().start->Len();

            if (pc->GetAlignData().start->GetType() == CT_NEG)
            {
               Chunk *next = pc->GetAlignData().start->GetNext();

               if (next->Is(CT_NUMBER))
               {
                  start_len += next->Len();
               }
            }
            col_adj += start_len;
         }
      }
      pc->AlignData().col_adj = col_adj;

      // See if this pushes out the max_col
      const size_t endcol = pc->GetColumn() + col_adj
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
   LOG_FMT(LAS, "AlignStack::%s(%d): Debug the stack, Len() is %zu\n",
           __func__, __LINE__, Len());

   for (size_t idx = 0; idx < Len(); idx++)
   {
      Chunk *pc = m_aligned.Get(idx)->m_pc;
      LOG_FMT(LAS, "AlignStack::%s(%d):   idx is %zu, pc->Text() is '%s', orig line is %zu, orig col is %zu, alignment col_adj is %d\n",
              __func__, __LINE__, idx, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol(), pc->GetAlignData().col_adj);
   }

   const ChunkStack::Entry *ce = nullptr;

   for (size_t idx = 0; idx < Len(); idx++)
   {
      ce = m_aligned.Get(idx);
      Chunk        *pc = ce->m_pc;

      const size_t tmp_col = m_max_col - pc->GetAlignData().col_adj;

      if (idx == 0)
      {
         if (  m_skip_first
            && pc->GetColumn() != tmp_col)
         {
            LOG_FMT(LAS, "AlignStack::%s(%d): orig line is %zu, orig col is %zu, dropping first item due to skip_first\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol());
            m_skip_first = false;
            m_aligned.Pop_Front();
            Flush();
            m_skip_first = true;
            return;
         }
         pc->SetFlagBits(PCF_ALIGN_START);

         pc->AlignData().right_align = m_right_align;
         pc->AlignData().amp_style   = m_amp_style;
         pc->AlignData().star_style  = m_star_style;
      }
      pc->AlignData().gap  = m_gap;
      pc->AlignData().next = m_aligned.GetChunk(idx + 1);

      // Indent the token, taking col_adj into account
      LOG_FMT(LAS, "AlignStack::%s(%d): orig line is %zu, orig col is %zu, Text() '%s', set to col %zu (adj is %d)\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), tmp_col, pc->GetAlignData().col_adj);
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
   // WITH_STACKID_DEBUG;
   //LOG_FMT(LAS, "AlignStack::Reset(%d):m_aligned.Reset()\n", __LINE__);
   m_aligned.Reset();
   //LOG_FMT(LAS, "AlignStack::Reset(%d):m_skipped.Reset()\n", __LINE__);
   m_skipped.Reset();
} // AlignStack::Reset


void AlignStack::End()
{
   // WITH_STACKID_DEBUG;

   if (!m_aligned.Empty())
   {
      //LOG_FMT(LAS, "AlignStack::End(%d):\n", __LINE__);
      Flush();
   }
   //LOG_FMT(LAS, "AlignStack::End(%d):m_aligned.Reset()\n", __LINE__);
   m_aligned.Reset();
   //LOG_FMT(LAS, "AlignStack::End(%d):m_skipped.Reset()\n", __LINE__);
   m_skipped.Reset();
} // AlignStack::End


size_t AlignStack::Len()
{
   // WITH_STACKID_DEBUG;
   return(m_aligned.Len());
} // AlignStack::Len


void AlignStack::Debug()
{
   // WITH_STACKID_DEBUG;

   size_t length = Len();

   if (length > 0)
   {
      LOG_FMT(LAS, "AlignStack::%s(%d): Debug the stack, Len is %zu\n",
              __func__, __LINE__, Len());

      for (size_t idx = 0; idx < length; idx++)
      {
         Chunk *pc = m_aligned.Get(idx)->m_pc;

         if (pc->Is(CT_PTR_TYPE))
         {
            LOG_FMT(LAS, "AlignStack::%s(%d): idx is %zu, [%s][%s]: orig line is %zu, orig col is %zu, type is %s, level is %zu, brace level is %zu\n",
                    __func__, __LINE__, idx, pc->Text(), pc->GetNext()->Text(), pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()), pc->GetLevel(), pc->GetBraceLevel());
         }
         else
         {
            LOG_FMT(LAS, "AlignStack::%s(%d): idx is %zu, [%s]: orig line is %zu, orig col is %zu, type is %s, level is %zu, brace level is %zu\n",
                    __func__, __LINE__, idx, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()), pc->GetLevel(), pc->GetBraceLevel());
         }
      }
   }
} // AlignStack::Debug


const char *AlignStack::get_StarStyle_name(StarStyle star_style)
{
   // WITH_STACKID_DEBUG;
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
} // AlignStack::get_StarStyle_name
