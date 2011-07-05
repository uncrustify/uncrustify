/**
 * @file ChunkStack.h
 * Manages a simple stack of chunks
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef CHUNKSTACK_H_INCLUDED
#define CHUNKSTACK_H_INCLUDED

#include "uncrustify_types.h"
#include <deque>

class ChunkStack
{
public:
   struct Entry
   {
      Entry() : m_seqnum(0), m_pc(0) { }
      Entry(const Entry& ref) : m_seqnum(ref.m_seqnum), m_pc(ref.m_pc) { }
      Entry(int sn, chunk_t *pc) : m_seqnum(sn), m_pc(pc) { }
      int     m_seqnum;
      chunk_t *m_pc;
   };

protected:
   deque<Entry> m_cse;
   int m_seqnum;   // current seq num

public:
   ChunkStack() : m_seqnum(0)
   {
   }

   ChunkStack(const ChunkStack& cs)
   {
      Set(cs);
   }

   virtual ~ChunkStack()
   {
   }

   void Set(const ChunkStack& cs);

   void Push(chunk_t *pc)
   {
      Push(pc, ++m_seqnum);
   }

   bool Empty() const
   {
      return(m_cse.empty());
   }

   int Len() const
   {
      return(m_cse.size());
   }

   const Entry *Top() const;
   const Entry *Get(int idx) const;
   chunk_t *GetChunk(int idx) const;

   chunk_t *Pop();
   void Push(chunk_t *pc, int seqnum);

   void Reset()
   {
      m_cse.clear();
   }

   void Zap(int idx);
   void Collapse();
};

#endif   /* CHUNKSTACK_H_INCLUDED */
