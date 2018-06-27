/**
 * @file BlockNumbering.cpp
 *
 * @author  Guy Maurel
 *          Juni 2018
 * @license GPL v2+
 */

#include "BlockNumbering.h"
#include "chunk_list.h"
#include "log_levels.h"
#include "logger.h"

static size_t Number = 0;


size_t GetBlockNumber()
{
   Number++;
   return(Number);
}


void NumberTheBlocks()
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LALASS, "%s(%d): Number the blocks.\n", __func__, __LINE__);

   size_t  BlockNumber = GetBlockNumber();
   chunk_t *pc         = chunk_get_head();
   while (pc != nullptr)
   {
      if (chunk_is_token(pc, CT_NEWLINE))
      {
         LOG_FMT(LALASS, "%s(%d): orig_line is %zu, orig_col is %zu, <Newline>\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col);
      }
      else
      {
         LOG_FMT(LALASS, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s'\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
      }

      if (  chunk_is_token(pc, CT_BRACE_OPEN)
         || chunk_is_token(pc, CT_FPAREN_OPEN)
         || chunk_is_token(pc, CT_ANGLE_OPEN))
      {
         // get a new number
         BlockNumber = GetBlockNumber();
      }

      pc->Block_Number = BlockNumber;

      if (  chunk_is_token(pc, CT_BRACE_CLOSE)
         || chunk_is_token(pc, CT_FPAREN_CLOSE)
         || chunk_is_token(pc, CT_ANGLE_CLOSE))
      {
         // look for the opening
         chunk_t *opening = chunk_get_prev_type(pc, (c_token_t)(pc->type - 1), pc->level);
         // look for the previous
         chunk_t *prev = chunk_get_prev(opening);
         // this is the 'old' block number
         if (prev == nullptr)
         {
            BlockNumber = 0;
         }
         else
         {
            BlockNumber = prev->Block_Number;
         }
      }

      pc = chunk_get_next(pc);
   }
} // NumberTheBlocks
