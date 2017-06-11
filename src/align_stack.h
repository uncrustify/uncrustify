/**
 * @file align_stack.h
 * Manages a align stack, which is just a pair of chunk stacks with a few
 * fancy functions.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef ALIGN_STACK_H_INCLUDED
#define ALIGN_STACK_H_INCLUDED

#include "ChunkStack.h"

class AlignStack
{
public:
   enum StarStyle
   {
      SS_IGNORE,  //! don't look for prev stars
      SS_INCLUDE, //! include prev * before add
      SS_DANGLE   //! include prev * after add
   };

   ChunkStack m_aligned;      //! contains the token that is aligned
   ChunkStack m_skipped;      //! contains the tokens sent to Add()
   size_t     m_max_col;
   size_t     m_min_col;
   size_t     m_span;
   size_t     m_thresh;
   size_t     m_seqnum;
   size_t     m_nl_seqnum;
   size_t     m_gap;
   bool       m_right_align;
   StarStyle  m_star_style;
   StarStyle  m_amp_style;  //! do not include the first item if it causes it to be indented
   bool       m_skip_first; //! do not include the first item if it causes it to be indented


   AlignStack()
      : m_max_col(0)
      , m_min_col(0)
      , m_span(0)
      , m_thresh(0)
      , m_seqnum(0)
      , m_nl_seqnum(0)
      , m_gap(0)
      , m_right_align(false)
      , m_star_style(SS_IGNORE)
      , m_amp_style(SS_IGNORE)
      , m_skip_first(false)
      , m_last_added(0)
   {
   }


   AlignStack(const AlignStack &ref)
      : m_aligned(ref.m_aligned)
      , m_skipped(ref.m_skipped)
      , m_max_col(ref.m_max_col)
      , m_min_col(ref.m_min_col)
      , m_span(ref.m_span)
      , m_thresh(ref.m_thresh)
      , m_seqnum(ref.m_seqnum)
      , m_nl_seqnum(ref.m_nl_seqnum)
      , m_gap(ref.m_gap)
      , m_right_align(ref.m_right_align)
      , m_star_style(ref.m_star_style)
      , m_amp_style(ref.m_amp_style)
      , m_skip_first(ref.m_skip_first)
      , m_last_added(ref.m_last_added)
   {
   }


   ~AlignStack()
   {
   }

   /**
    * Resets the two ChunkLists and zeroes local vars.
    *
    * @param span       The row span limit
    * @param threshold  The column threshold
    */
   void Start(size_t span, size_t threshold = 0);


   /**
    * Adds an entry to the appropriate stack.
    *
    * @param pc      the chunk
    * @param seqnum  optional sequence number (0=assign one)
    */
   void Add(chunk_t *pc, size_t seqnum = 0);


   //! Adds some newline and calls Flush() if needed
   void NewLines(size_t cnt);


   /**
    * Aligns all the stuff in m_aligned.
    * Re-adds 'newer' items in m_skipped.
    */
   void Flush();


   //! Resets the stack, discarding anything that was previously added
   void Reset();


   //! Aligns everything else and resets the lists.
   void End();

protected:
   size_t     m_last_added; //! 0=none, 1=aligned, 2=skipped
   ChunkStack m_scratch;    //! used in ReAddSkipped()

   //! Calls Add on all the skipped items
   void   ReAddSkipped();
};


#endif /* ALIGN_STACK_H_INCLUDED */

