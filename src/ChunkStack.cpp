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


void ChunkStack::Set(const ChunkStack &cs)
{
   m_cse.resize(cs.m_cse.size());
   for (size_t idx = 0; idx < m_cse.size(); idx++)
   {
      m_cse[idx].m_pc     = cs.m_cse[idx].m_pc;
      m_cse[idx].m_seqnum = cs.m_cse[idx].m_seqnum;
   }
   m_seqnum = cs.m_seqnum;
}


const ChunkStack::Entry *ChunkStack::Top() const
{
   if (!m_cse.empty())
   {
      return(&m_cse[m_cse.size() - 1]);
   }
   return(nullptr);
}


const ChunkStack::Entry *ChunkStack::Get(size_t idx) const
{
   if (idx < m_cse.size())
   {
      return(&m_cse[idx]);
   }
   return(nullptr);
}


chunk_t *ChunkStack::GetChunk(size_t idx) const
{
   if (idx < m_cse.size())
   {
      return(m_cse[idx].m_pc);
   }
   return(nullptr);
}


chunk_t *ChunkStack::Pop_Front()
{
   chunk_t *pc = nullptr;

   if (!m_cse.empty())
   {
      pc = m_cse[0].m_pc;
      m_cse.pop_front();
   }
   return(pc);
}


chunk_t *ChunkStack::Pop_Back()
{
   chunk_t *pc = nullptr;

   if (!m_cse.empty())
   {
      pc = m_cse[m_cse.size() - 1].m_pc;
      m_cse.pop_back();
   }
   return(pc);
}


void ChunkStack::Push_Back(chunk_t *pc, size_t seqnum)
{
   m_cse.push_back(Entry(seqnum, pc));
   if (m_seqnum < seqnum)
   {
      m_seqnum = seqnum;
   }
}


void ChunkStack::Zap(size_t idx)
{
   if (idx < m_cse.size())
   {
      m_cse[idx].m_pc = nullptr;
   }
}


void ChunkStack::Collapse()
{
   size_t wr_idx = 0;

   for (size_t rd_idx = 0; rd_idx < m_cse.size(); rd_idx++)
   {
      if (m_cse[rd_idx].m_pc != nullptr)
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
