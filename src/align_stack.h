/**
 * @file align_stack.h
 * Manages a align stack, which is just a pair of chunk stacks with a few
 * fancy functions.
 *
 * $Id$
 */

#include "ChunkStack.h"

class AlignStack
{
public:
   ChunkStack  m_aligned;
   ChunkStack  m_skipped;
   ChunkStack  m_scratch;
   int         m_max_col;
   int         m_span;
   int         m_thresh;
   int         m_seqnum;
   int         m_nl_seqnum;

   AlignStack() {};
   ~AlignStack() {};

   void Start(int span, int threshold);
   void Add(chunk_t *pc, int seqnum = 0);
   void NewLines(int cnt);
   void Flush();
   void End();

protected:
   void ReAddSkipped();
};

