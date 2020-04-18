/**
 * @file combine_skip.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 * extract from combine.cpp
 */

#include "combine_skip.h"


chunk_t *skip_align(chunk_t *start)
{
   chunk_t *pc = start;

   if (chunk_is_token(pc, CT_ALIGN))
   {
      pc = chunk_get_next_ncnl(pc);

      if (chunk_is_token(pc, CT_PAREN_OPEN))
      {
         pc = chunk_get_next_type(pc, CT_PAREN_CLOSE, pc->level);
         pc = chunk_get_next_ncnl(pc);

         if (chunk_is_token(pc, CT_COLON))
         {
            pc = chunk_get_next_ncnl(pc);
         }
      }
   }
   return(pc);
}


chunk_t *skip_expression(chunk_t *start)
{
   chunk_t *pc = start;

   while (pc != nullptr && pc->level >= start->level)
   {
      if (  pc->level == start->level
         && (chunk_is_semicolon(pc) || chunk_is_token(pc, CT_COMMA)))
      {
         return(pc);
      }
      pc = chunk_get_next_ncnl(pc);
   }
   return(pc);
}


chunk_t *skip_to_next_statement(chunk_t *pc)
{
   while (  pc != nullptr
         && !chunk_is_semicolon(pc)
         && pc->type != CT_BRACE_OPEN
         && pc->type != CT_BRACE_CLOSE)
   {
      pc = chunk_get_next_ncnl(pc);
   }
   return(pc);
}


chunk_t *skip_parent_types(chunk_t *colon)
{
   auto pc = chunk_get_next_ncnlnp(colon);

   while (pc)
   {
      // Skip access specifier
      if (chunk_is_token(pc, CT_ACCESS))
      {
         pc = chunk_get_next_ncnlnp(pc);
         continue;
      }

      // Check for a type name
      if (!(chunk_is_token(pc, CT_WORD) || chunk_is_token(pc, CT_TYPE)))
      {
         LOG_FMT(LPCU,
                 "%s is confused; expected a word at %zu:%zu "
                 "following type list at %zu:%zu\n", __func__,
                 colon->orig_line, colon->orig_col,
                 pc->orig_line, pc->orig_col);
         return(colon);
      }
      // Get next token
      auto next = skip_template_next(chunk_get_next_ncnlnp(pc));

      if (chunk_is_token(next, CT_DC_MEMBER) || chunk_is_token(next, CT_COMMA))
      {
         pc = chunk_get_next_ncnlnp(next);
      }
      else if (next)
      {
         LOG_FMT(LPCU, "%s -> %zu:%zu ('%s')\n", __func__,
                 next->orig_line, next->orig_col, next->text());
         return(next);
      }
      else
      {
         break;
      }
   }
   LOG_FMT(LPCU, "%s: did not find end of type list (start was %zu:%zu)\n",
           __func__, colon->orig_line, colon->orig_col);
   return(colon);
} // skip_parent_types


chunk_t *skip_template_prev(chunk_t *ang_close)
{
   if (chunk_is_token(ang_close, CT_ANGLE_CLOSE))
   {
      chunk_t *pc = chunk_get_prev_type(ang_close, CT_ANGLE_OPEN, ang_close->level);
      return(chunk_get_prev_ncnlni(pc));   // Issue #2279
   }
   return(ang_close);
}


chunk_t *skip_tsquare_next(chunk_t *ary_def)
{
   if (chunk_is_token(ary_def, CT_SQUARE_OPEN) || chunk_is_token(ary_def, CT_TSQUARE))
   {
      return(chunk_get_next_nisq(ary_def));
   }
   return(ary_def);
}


chunk_t *skip_attribute_next(chunk_t *attr)
{
   chunk_t *pc = attr;

   while (chunk_is_token(pc, CT_ATTRIBUTE))
   {
      pc = chunk_get_next_ncnl(pc);

      if (chunk_is_token(pc, CT_FPAREN_OPEN))
      {
         pc = chunk_get_next_type(pc, CT_FPAREN_CLOSE, pc->level);
         pc = chunk_get_next_ncnl(pc);
      }
   }
   return(pc);
}


chunk_t *skip_attribute_prev(chunk_t *fp_close)
{
   chunk_t *pc = fp_close;

   while (true)
   {
      if (  chunk_is_token(pc, CT_FPAREN_CLOSE)
         && get_chunk_parent_type(pc) == CT_ATTRIBUTE)
      {
         pc = chunk_get_prev_type(pc, CT_ATTRIBUTE, pc->level);
      }
      else if (chunk_is_not_token(pc, CT_ATTRIBUTE))
      {
         break;
      }
      pc = chunk_get_prev_ncnlni(pc);   // Issue #2279
   }
   return(pc);
}
