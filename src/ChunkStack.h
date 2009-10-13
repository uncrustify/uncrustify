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

class ChunkStack
{
public:
   struct Entry
   {
      int     m_seqnum;
      chunk_t *m_pc;
   };

protected:
   Entry *m_cse;     // the array of entries
   int   m_size;     // entries allocated
   int   m_len;      // entries used
   int   m_seqnum;   // current seq num

public:
   ChunkStack()
   {
      Init();
   }


   ChunkStack(const ChunkStack& cs);

   ~ChunkStack();

   void Set(const ChunkStack& cs);

   void Push(chunk_t *pc)
   {
      Push(pc, ++m_seqnum);
   }


   bool Empty() const
   {
      return(m_len == 0);
   }


   int Len() const
   {
      return(m_len);
   }


   const Entry *Top() const;
   const Entry *Get(int idx) const;
   chunk_t *GetChunk(int idx) const;

   chunk_t *Pop();
   void Push(chunk_t *pc, int seqnum);

   void Reset()
   {
      m_len = 0;
   }


   void Zap(int idx);
   void Collapse();

protected:
   void Init();
   void Resize(int newsize);
};

#endif   /* CHUNKSTACK_H_INCLUDED */
