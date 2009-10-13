/**
 * @file ChunkStack.cpp
 * Manages a chunk stack
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "ChunkStack.h"
#include <cstdio>
#include <cstdlib>

ChunkStack::ChunkStack(const ChunkStack& cs)
{
   Set(cs);
}


ChunkStack::~ChunkStack()
{
   if (m_cse != NULL)
   {
      free(m_cse);
      m_cse  = NULL;
      m_size = m_len = 0;
   }
}


void ChunkStack::Set(const ChunkStack& cs)
{
   Init();
   Resize(cs.m_len);
   for (int idx = 0; idx < cs.m_len; idx++)
   {
      Push(cs.m_cse[idx].m_pc, cs.m_cse[idx].m_seqnum);
   }
   m_seqnum = cs.m_seqnum;
}


const ChunkStack::Entry *ChunkStack::Top() const
{
   if (m_len > 0)
   {
      return(&m_cse[m_len - 1]);
   }
   return(NULL);
}


const ChunkStack::Entry *ChunkStack::Get(int idx) const
{
   if ((idx < m_len) && (idx >= 0))
   {
      return(&m_cse[idx]);
   }
   return(NULL);
}


chunk_t *ChunkStack::GetChunk(int idx) const
{
   if ((idx < m_len) && (idx >= 0))
   {
      return(m_cse[idx].m_pc);
   }
   return(NULL);
}


chunk_t *ChunkStack::Pop()
{
   if (m_len > 0)
   {
      m_len--;
      return(m_cse[m_len].m_pc);
   }
   return(NULL);
}


void ChunkStack::Push(chunk_t *pc, int seqnum)
{
   if (m_len >= m_size)
   {
      Resize(m_len + 64);
   }
   m_cse[m_len].m_pc     = pc;
   m_cse[m_len].m_seqnum = seqnum;
   m_len++;
   if (m_seqnum < seqnum)
   {
      m_seqnum = seqnum;
   }
}


void ChunkStack::Init()
{
   m_cse    = NULL;
   m_size   = 0;
   m_len    = 0;
   m_seqnum = 0;
}


void ChunkStack::Resize(int newsize)
{
   if (m_size < newsize)
   {
      m_size = newsize;
      m_cse  = (Entry *)realloc(m_cse, m_size * sizeof(ChunkStack::Entry));
      assert(m_cse != NULL);
      /*TODO: check for out-of-memory? */
   }
}


/**
 * Mark an entry to be removed by Collapse()
 *
 * @param idx  The item to remove
 */
void ChunkStack::Zap(int idx)
{
   if ((idx < m_len) && (idx >= 0))
   {
      assert(m_cse != NULL);
      m_cse[idx].m_pc = NULL;
   }
}


/**
 * Compresses down the stack by removing dead entries
 */
void ChunkStack::Collapse()
{
   int oldlen = m_len;

   m_len = 0;

   for (int idx = 0; idx < oldlen; idx++)
   {
      assert(m_cse != NULL);
      if (m_cse[idx].m_pc != NULL)
      {
         m_cse[m_len].m_pc     = m_cse[idx].m_pc;
         m_cse[m_len].m_seqnum = m_cse[idx].m_seqnum;
         m_len++;
      }
   }
}


//
//int main(int argc, char **argv)
//{
//   ChunkStack cs;
//
//   cs.Push((chunk_t *)1);
//   cs.Push((chunk_t *)2);
//   cs.Push((chunk_t *)3);
//   cs.Push((chunk_t *)4);
//
//   while (!cs.Empty())
//   {
//      chunk_t *pc = cs.Pop();
//      printf("pc = %p\n", pc);
//   }
//
//   return (0);
//}
//
