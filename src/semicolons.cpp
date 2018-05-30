/**
 * @file semicolons.cpp
 * Removes extra semicolons
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "semicolons.h"
#include "uncrustify_types.h"
#include "chunk_list.h"
#include "ChunkStack.h"
#include "prototypes.h"
#include "uncrustify.h"
#include "language_tools.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "unc_ctype.h"


static void remove_semicolon(chunk_t *pc);


/**
 * We are on a semicolon that is after an unidentified brace close.
 * Check for what is before the brace open.
 * Do not remove if it is a square close, word, type, or paren close.
 */
static void check_unknown_brace_close(chunk_t *semi, chunk_t *brace_close);


static void remove_semicolon(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LDELSEMI, "%s(%d): Removed semicolon: orig_line is %zu, orig_col is %zu",
           __func__, __LINE__, pc->orig_line, pc->orig_col);
   log_func_stack_inline(LDELSEMI);
   // TODO: do we want to shift stuff back a column?
   chunk_del(pc);
}


void remove_extra_semicolons(void)
{
   LOG_FUNC_ENTRY();

   chunk_t *pc = chunk_get_head();
   while (pc != nullptr)
   {
      chunk_t *next = chunk_get_next_ncnl(pc);
      chunk_t *prev;
      if (  chunk_is_token(pc, CT_SEMICOLON)
         && !(pc->flags & PCF_IN_PREPROC)
         && (prev = chunk_get_prev_ncnl(pc)) != nullptr)
      {
         LOG_FMT(LSCANSEMI, "%s(%d): Semi orig_line is %zu, orig_col is %zu, parent is %s, prev = '%s' [%s/%s]\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, get_token_name(pc->parent_type),
                 prev->text(),
                 get_token_name(prev->type), get_token_name(prev->parent_type));

         if (pc->parent_type == CT_TYPEDEF)
         {
            // keep it
         }
         else if (  chunk_is_token(prev, CT_BRACE_CLOSE)
                 && (  prev->parent_type == CT_IF
                    || prev->parent_type == CT_ELSEIF
                    || prev->parent_type == CT_ELSE
                    || prev->parent_type == CT_SWITCH
                    || prev->parent_type == CT_WHILE
                    || prev->parent_type == CT_USING_STMT
                    || prev->parent_type == CT_FOR
                    || prev->parent_type == CT_FUNC_DEF
                    || prev->parent_type == CT_OC_MSG_DECL
                    || prev->parent_type == CT_FUNC_CLASS_DEF
                    || prev->parent_type == CT_NAMESPACE))
         {
            LOG_FUNC_CALL();
            remove_semicolon(pc);
         }
         else if (  chunk_is_token(prev, CT_BRACE_CLOSE)
                 && prev->parent_type == CT_NONE)
         {
            check_unknown_brace_close(pc, prev);
         }
         else if (chunk_is_token(prev, CT_SEMICOLON) && prev->parent_type != CT_FOR)
         {
            LOG_FUNC_CALL();
            remove_semicolon(pc);
         }
         else if (  language_is_set(LANG_D)
                 && (  prev->parent_type == CT_ENUM
                    || prev->parent_type == CT_UNION
                    || prev->parent_type == CT_STRUCT))
         {
            LOG_FUNC_CALL();
            remove_semicolon(pc);
         }
         else if (  language_is_set(LANG_JAVA)
                 && prev->parent_type == CT_SYNCHRONIZED)
         {
            LOG_FUNC_CALL();
            remove_semicolon(pc);
         }
         else if (chunk_is_token(prev, CT_BRACE_OPEN))
         {
            LOG_FUNC_CALL();
            remove_semicolon(pc);
         }
      }

      pc = next;
   }
} // remove_extra_semicolons


static void check_unknown_brace_close(chunk_t *semi, chunk_t *brace_close)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc = chunk_get_prev_type(brace_close, CT_BRACE_OPEN, brace_close->level);
   pc = chunk_get_prev_ncnl(pc);
   if (  !chunk_is_token(pc, CT_RETURN)
      && !chunk_is_token(pc, CT_WORD)
      && !chunk_is_token(pc, CT_TYPE)
      && !chunk_is_token(pc, CT_SQUARE_CLOSE)
      && !chunk_is_token(pc, CT_ANGLE_CLOSE)
      && !chunk_is_token(pc, CT_TSQUARE)
      && !chunk_is_paren_close(pc))
   {
      remove_semicolon(semi);
   }
}
