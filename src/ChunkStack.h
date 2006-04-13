/**
 * @file ChunkStack.h
 * Manages a simple stack of chunks
 *
 * $Id: ChunkStack.h 23 2006-02-27 01:54:05Z bengardner $
 */

#ifndef CHUNKSTACK_H_INCLUDED
#define CHUNKSTACK_H_INCLUDED

#include "uncrustify_types.h"

struct ChunkStackEntry
{
   int     m_seqnum;
   chunk_t *m_pc;
};

class ChunkStack
{
public:
   struct Entry
   {
      int     m_seqnum;
      chunk_t *m_pc;
   };

protected:
   Entry * m_cse;    // the array of entries
   int m_size;       // entries allocated
   int m_len;        // entries used
   int m_seqnum;     // current seq num

public:
   ChunkStack()
   {
      Init();
   }

   ChunkStack(const ChunkStack & cs);

   ~ChunkStack();

   void Push(chunk_t *pc)
   {
      Push(pc, ++m_seqnum);
   }

   bool Empty()
   {
      return(m_len == 0);
   }

   int Len()
   {
      return(m_len);
   }

   const Entry *Top();
   const Entry *Get(int idx);

   chunk_t *Pop();
   void Push(chunk_t *pc, int seqnum);

   void Reset()
   {
      m_len = 0;
   }

protected:
   void Init();
   void Resize(int newsize);
};

#endif   /* CHUNKSTACK_H_INCLUDED */

