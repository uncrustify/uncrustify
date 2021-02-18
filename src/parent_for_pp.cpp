/**
 * @file parent_for_pp.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "parent_for_pp.h"

#include "chunk_list.h"
#include "logger.h"
#include "uncrustify_types.h"

#include <vector>


//using namespace uncrustify;

//using std::vector;


void do_parent_for_pp(void)
{
   LOG_FUNC_ENTRY();

   vector<chunk_t *> viz;

   chunk_t           *pc = chunk_get_head();

   while ((pc = chunk_get_next_ncnnl(pc)) != nullptr)
   {
      // CT_PP_IF,            // #if, #ifdef, or #ifndef
      // CT_PP_ELSE,          // #else or #elif
      // CT_PP_ENDIF,         // #endif
      if (chunk_is_token(pc, CT_PP_IF))
      {
         LOG_FMT(LMCB, "%s(%d): IF: orig_line %zu, orig_col is %zu\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col);
         log_pcf_flags(LMCB, pc->flags);
         viz.push_back(pc);
      }
      else if (chunk_is_token(pc, CT_PP_ELSE))
      {
         LOG_FMT(LMCB, "%s(%d): ELSE: orig_line %zu, orig_col is %zu\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col);
         log_pcf_flags(LMCB, pc->flags);
         size_t  level = pc->pp_level;
         chunk_t *a    = viz.at(level - 1);
         chunk_set_parent(pc, a);
      }
      else if (chunk_is_token(pc, CT_PP_ENDIF))
      {
         LOG_FMT(LMCB, "%s(%d): ENDIF: orig_line %zu, orig_col is %zu\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col);
         log_pcf_flags(LMCB, pc->flags);
         size_t  level = pc->pp_level;
         chunk_t *a    = viz.at(level);
         chunk_set_parent(pc, a);
         viz.pop_back();
      }
   }
} // do_parent_for_pp
