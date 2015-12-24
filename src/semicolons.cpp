/**
 * @file semicolons.cpp
 * Removes extra semicolons
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "uncrustify_types.h"
#include "chunk_list.h"
#include "ChunkStack.h"
#include "prototypes.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include "unc_ctype.h"
#include <cassert>

static void check_unknown_brace_close(chunk_t *semi, chunk_t *brace_close);


static void remove_semicolon(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LDELSEMI, "%s: Removed semicolon at line %d, col %d",
           __func__, pc->orig_line, pc->orig_col);
   log_func_stack_inline(LDELSEMI);
   /* TODO: do we want to shift stuff back a column? */
   chunk_del(pc);
}


/**
 * Removes superfluous semicolons:
 *  - after brace close whose parent is IF, ELSE, SWITCH, WHILE, FOR, NAMESPACE
 *  - after another semicolon where parent is not FOR
 *  - (D) after brace close whose parent is ENUM/STRUCT/UNION
 *  - (Java) after brace close whose parent is SYNCHRONIZED
 *  - after an open brace
 *  - when not in a #DEFINE
 */
void remove_extra_semicolons(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;
   chunk_t *next;
   chunk_t *prev;

   pc = chunk_get_head();
   while (pc != NULL)
   {
      next = chunk_get_next_ncnl(pc);

      if ((pc->type == CT_SEMICOLON) && !(pc->flags & PCF_IN_PREPROC) &&
          ((prev = chunk_get_prev_ncnl(pc)) != NULL))
      {
         LOG_FMT(LSCANSEMI, "Semi on %d:%d parent=%s, prev = '%s' [%s/%s]\n",
                 pc->orig_line, pc->orig_col, get_token_name(pc->parent_type),
                 prev->str.c_str(),
                 get_token_name(prev->type), get_token_name(prev->parent_type));

         if (pc->parent_type == CT_TYPEDEF)
         {
            /* keep it */
         }
         else if ((prev->type == CT_BRACE_CLOSE) &&
                  ((prev->parent_type == CT_IF) ||
                   (prev->parent_type == CT_ELSEIF) ||
                   (prev->parent_type == CT_ELSE) ||
                   (prev->parent_type == CT_SWITCH) ||
                   (prev->parent_type == CT_WHILE) ||
                   (prev->parent_type == CT_USING_STMT) ||
                   (prev->parent_type == CT_FOR) ||
                   (prev->parent_type == CT_FUNC_DEF) ||
                   (prev->parent_type == CT_OC_MSG_DECL) ||
                   (prev->parent_type == CT_FUNC_CLASS_DEF) ||
                   (prev->parent_type == CT_NAMESPACE)))
         {
            LOG_FUNC_CALL();
            remove_semicolon(pc);
         }
         else if ((prev->type == CT_BRACE_CLOSE) &&
                  (prev->parent_type == CT_NONE))
         {
            check_unknown_brace_close(pc, prev);
         }
         else if ((prev->type == CT_SEMICOLON) &&
                  (prev->parent_type != CT_FOR))
         {
            LOG_FUNC_CALL();
            remove_semicolon(pc);
         }
         else if ((cpd.lang_flags & LANG_D) &&
                  ((prev->parent_type == CT_ENUM) ||
                   (prev->parent_type == CT_UNION) ||
                   (prev->parent_type == CT_STRUCT)))
         {
            LOG_FUNC_CALL();
            remove_semicolon(pc);
         }
         else if ((cpd.lang_flags & LANG_JAVA) &&
                  ((prev->parent_type == CT_SYNCHRONIZED)))
         {
            LOG_FUNC_CALL();
            remove_semicolon(pc);
         }
         else if (prev->type == CT_BRACE_OPEN)
         {
            LOG_FUNC_CALL();
            remove_semicolon(pc);
         }
      }

      pc = next;
   }
}


/**
 * We are on a semicolon that is after an unidentified brace close.
 * Check for what is before the brace open.
 * Do not remove if it is a square close, word, type, or paren close.
 */
static void check_unknown_brace_close(chunk_t *semi, chunk_t *brace_close)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;

   pc = chunk_get_prev_type(brace_close, CT_BRACE_OPEN, brace_close->level);
   pc = chunk_get_prev_ncnl(pc);
   if ((pc != NULL) &&
       (pc->type != CT_RETURN) &&
       (pc->type != CT_WORD) &&
       (pc->type != CT_TYPE) &&
       (pc->type != CT_SQUARE_CLOSE) &&
       (pc->type != CT_TSQUARE) &&
       !chunk_is_paren_close(pc))
   {
      remove_semicolon(semi);
   }
}
