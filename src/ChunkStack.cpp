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


void ChunkStack::Set(const ChunkStack& cs)
{
   m_cse.resize(cs.m_cse.size());
   for (int idx = 0; idx < (int)m_cse.size(); idx++)
   {
      m_cse[idx].m_pc     = cs.m_cse[idx].m_pc;
      m_cse[idx].m_seqnum = cs.m_cse[idx].m_seqnum;
   }
   m_seqnum = cs.m_seqnum;
}


const ChunkStack::Entry *ChunkStack::Top() const
{
   if (m_cse.size() > 0)
   {
      return(&m_cse[m_cse.size() - 1]);
   }
   return(NULL);
}


const ChunkStack::Entry *ChunkStack::Get(int idx) const
{
   if ((idx >= 0) && (idx < (int)m_cse.size()))
   {
      return(&m_cse[idx]);
   }
   return(NULL);
}


chunk_t *ChunkStack::GetChunk(int idx) const
{
   if ((idx >= 0) && (idx < (int)m_cse.size()))
   {
      return(m_cse[idx].m_pc);
   }
   return(NULL);
}


chunk_t *ChunkStack::Pop()
{
   chunk_t *pc = NULL;

   if (m_cse.size() > 0)
   {
      pc = m_cse[m_cse.size() - 1].m_pc;
      m_cse.pop_back();
   }
   return(pc);
}


void ChunkStack::Push(chunk_t *pc, int seqnum)
{
   m_cse.push_back(Entry(seqnum, pc));
   if (m_seqnum < seqnum)
   {
      m_seqnum = seqnum;
   }
}


/**
 * Mark an entry to be removed by Collapse()
 *
 * @param idx  The item to remove
 */
void ChunkStack::Zap(int idx)
{
   if ((idx >= 0) && (idx < (int)m_cse.size()))
   {
      m_cse[idx].m_pc = NULL;
   }
}


/**
 * Compresses down the stack by removing dead entries
 */
void ChunkStack::Collapse()
{
   int wr_idx = 0;
   int rd_idx;

   for (rd_idx = 0; rd_idx < (int)m_cse.size(); rd_idx++)
   {
      if (m_cse[rd_idx].m_pc != NULL)
      {
         if (rd_idx != wr_idx)
         {
            m_cse[wr_idx].m_pc     = m_cse[rd_idx].m_pc;
            m_cse[wr_idx].m_seqnum = m_cse[rd_idx].m_seqnum;
         }
         wr_idx++;
      }
   }
   m_cse.resize(wr_idx);
}
