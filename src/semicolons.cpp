/**
 * @file semicolons.cpp
 * Removes extra semicolons
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "semicolons.h"

#include "prototypes.h"


static void remove_semicolon(Chunk *pc);


/**
 * We are on a semicolon that is after an unidentified brace close.
 * Check for what is before the brace open.
 * Do not remove if it is a square close, word, type, or paren close.
 */
static void check_unknown_brace_close(Chunk *semi, Chunk *brace_close);


static void remove_semicolon(Chunk *pc)
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

   Chunk *pc = Chunk::GetHead();

   while (pc->IsNotNullChunk())
   {
      Chunk *next = pc->GetNextNcNnl();
      Chunk *prev;

      if (  chunk_is_token(pc, CT_SEMICOLON)
         && !pc->flags.test(PCF_IN_PREPROC)
         && (prev = pc->GetPrevNcNnl())->IsNotNullChunk())
      {
         LOG_FMT(LSCANSEMI, "%s(%d): Semi orig_line is %zu, orig_col is %zu, parent is %s, prev = '%s' [%s/%s]\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, get_token_name(get_chunk_parent_type(pc)),
                 prev->Text(),
                 get_token_name(prev->type), get_token_name(get_chunk_parent_type(prev)));

         if (get_chunk_parent_type(pc) == CT_TYPEDEF)
         {
            // keep it
         }
         else if (  chunk_is_token(prev, CT_BRACE_CLOSE)
                 && (  get_chunk_parent_type(prev) == CT_IF
                    || get_chunk_parent_type(prev) == CT_ELSEIF
                    || get_chunk_parent_type(prev) == CT_ELSE
                    || get_chunk_parent_type(prev) == CT_SWITCH
                    || get_chunk_parent_type(prev) == CT_WHILE
                    || get_chunk_parent_type(prev) == CT_USING_STMT
                    || get_chunk_parent_type(prev) == CT_FOR
                    || get_chunk_parent_type(prev) == CT_FUNC_DEF
                    || get_chunk_parent_type(prev) == CT_OC_MSG_DECL
                    || get_chunk_parent_type(prev) == CT_FUNC_CLASS_DEF
                    || get_chunk_parent_type(prev) == CT_NAMESPACE))
         {
            remove_semicolon(pc);
         }
         else if (  chunk_is_token(prev, CT_BRACE_CLOSE)
                 && get_chunk_parent_type(prev) == CT_NONE)
         {
            check_unknown_brace_close(pc, prev);
         }
         else if (  chunk_is_token(prev, CT_SEMICOLON)
                 && get_chunk_parent_type(prev) != CT_FOR)
         {
            remove_semicolon(pc);
         }
         else if (  language_is_set(LANG_D)
                 && (  get_chunk_parent_type(prev) == CT_ENUM
                    || get_chunk_parent_type(prev) == CT_UNION
                    || get_chunk_parent_type(prev) == CT_STRUCT))
         {
            remove_semicolon(pc);
         }
         else if (  language_is_set(LANG_JAVA)
                 && get_chunk_parent_type(prev) == CT_SYNCHRONIZED)
         {
            remove_semicolon(pc);
         }
         else if (chunk_is_token(prev, CT_BRACE_OPEN))
         {
            remove_semicolon(pc);
         }
      }
      pc = next;
   }
} // remove_extra_semicolons


static void check_unknown_brace_close(Chunk *semi, Chunk *brace_close)
{
   LOG_FUNC_ENTRY();
   Chunk *pc = brace_close->GetPrevType(CT_BRACE_OPEN, brace_close->level);

   pc = pc->GetPrevNcNnl();

   if (  pc->IsNotNullChunk()
      && pc->type != CT_RETURN
      && pc->type != CT_WORD
      && pc->type != CT_TYPE
      && pc->type != CT_SQUARE_CLOSE
      && pc->type != CT_ANGLE_CLOSE
      && pc->type != CT_TSQUARE
      && !chunk_is_paren_close(pc))
   {
      remove_semicolon(semi);
   }
}
