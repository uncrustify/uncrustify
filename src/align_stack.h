/**
 * @file align_stack.h
 * Manages a align stack, which is just a pair of chunk stacks with a few
 * fancy functions.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "ChunkStack.h"

class AlignStack
{
public:
   enum StarStyle
   {
      SS_IGNORE,  // don't look for prev stars
      SS_INCLUDE, // include prev * before add
      SS_DANGLE   // include prev * after add
   };

   ChunkStack m_aligned;   /* contains the token that is aligned */
   ChunkStack m_skipped;   /* contains the tokens sent to Add() */
   int        m_max_col;
   int        m_min_col;
   int        m_span;
   int        m_thresh;
   int        m_seqnum;
   int        m_nl_seqnum;
   int        m_gap;
   bool       m_right_align;
   StarStyle  m_star_style;
   StarStyle  m_amp_style;
   /* do not include the first item if it causes it to be indented */
   bool       m_skip_first;


   AlignStack() :
      m_max_col(0), m_min_col(0), m_span(0), m_thresh(0), m_seqnum(0),
      m_nl_seqnum(0), m_gap(0), m_right_align(false),
      m_star_style(SS_IGNORE), m_amp_style(SS_IGNORE), m_skip_first(false),
      m_last_added(0)
   {
   }


   AlignStack(const AlignStack &ref) :
      m_aligned(ref.m_aligned), m_skipped(ref.m_skipped),
      m_max_col(ref.m_max_col), m_min_col(ref.m_min_col),
      m_span(ref.m_span), m_thresh(ref.m_thresh), m_seqnum(ref.m_seqnum),
      m_nl_seqnum(ref.m_nl_seqnum), m_gap(ref.m_gap),
      m_right_align(ref.m_right_align), m_star_style(ref.m_star_style),
      m_amp_style(ref.m_amp_style), m_skip_first(ref.m_skip_first),
      m_last_added(ref.m_last_added)
   {
   }


   ~AlignStack()
   {
   }


   void Start(int span, int threshold = 0);
   void Add(chunk_t *pc, int seqnum = 0);
   void NewLines(int cnt);
   void Flush();
   void Reset();
   void End();

protected:
   int m_last_added; /* 0=none, 1=aligned, 2=skipped */
   void ReAddSkipped();

   ChunkStack m_scratch; /* used in ReAddSkipped() */
};
