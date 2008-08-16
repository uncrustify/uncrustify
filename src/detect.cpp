/**
 * @file detect.cpp
 * Scans the parsed file and tries to determine options.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 *
 * $Id$
 */
#include "uncrustify_types.h"
#include "chunk_list.h"
#include "ChunkStack.h"
#include "align_stack.h"
#include "prototypes.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include "unc_ctype.h"


struct sp_votes
{
   int m_add;
   int m_remove;
   int m_force;

   sp_votes() { reset(); }
   void reset()
   {
      m_add    = 0;
      m_remove = 0;
      m_force  = 0;
   }

   void vote(chunk_t *first, chunk_t *second);
   argval_t result();
};


void sp_votes::vote(chunk_t *first, chunk_t *second)
{
   if (chunk_is_newline(first) || chunk_is_newline(second))
   {
      return;
   }

   int col_dif = second->column - (first->column + first->len);
   if (col_dif == 0)
   {
      m_remove++;
   }
   else if (col_dif == 1)
   {
      m_force++;
   }
   else
   {
      m_add++;
   }
}


argval_t sp_votes::result()
{
   /* Ignore if no items were added */
   if ((m_remove == 0) && (m_add == 0) && (m_force == 0))
   {
      return(AV_IGNORE);
   }

   if (m_remove == 0)
   {
      return((m_force > m_add) ? AV_FORCE : AV_ADD);
   }
   else if ((m_force == 0) && (m_add == 0))
   {
      return(AV_REMOVE);
   }
   else
   {
      return(AV_IGNORE);
   }
}


/**
 * Detect the sp_arith setting
 */
static void detect_sp_arith()
{
   chunk_t  *pc;
   sp_votes vote;

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next_ncnl(pc))
   {
      if (pc->type == CT_ARITH)
      {
         vote.vote(pc, chunk_get_next(pc));
         vote.vote(chunk_get_prev(pc), pc);
      }
   }
   cpd.settings[UO_sp_arith].a = vote.result();
}


/**
 * Call all the detect_xxxx() functions
 */
void detect_options(const char *data, int data_len)
{
   detect_sp_arith();
}

