/**
 * @file combine_skip.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 * extract from combine.cpp
 */

#include "combine_skip.h"

#include "combine_tools.h"


chunk_t *skip_align(chunk_t *start)
{
   chunk_t *pc = start;

   if (chunk_is_token(pc, CT_ALIGN))
   {
      pc = chunk_get_next_ncnnl(pc);

      if (chunk_is_token(pc, CT_PAREN_OPEN))
      {
         pc = chunk_get_next_type(pc, CT_PAREN_CLOSE, pc->level);
         pc = chunk_get_next_ncnnl(pc);

         if (chunk_is_token(pc, CT_COLON))
         {
            pc = chunk_get_next_ncnnl(pc);
         }
      }
   }
   return(pc);
}


chunk_t *skip_expression(chunk_t *pc)
{
   return(chunk_get_next_ncnnl(skip_to_expression_end(pc)));
}


chunk_t *skip_expression_rev(chunk_t *pc)
{
   return(chunk_get_prev_ncnnlni(skip_to_expression_start(pc)));
}


static chunk_t *skip_to_expression_edge(chunk_t *pc, chunk_t *(*chunk_get_next)(chunk_t *cur, scope_e scope))
{
   chunk_t *prev = pc;

   if (  prev != nullptr
      && chunk_get_next != nullptr)
   {
      std::size_t level         = prev->level;
      chunk_t     *next         = prev;
      std::size_t template_nest = get_cpp_template_angle_nest_level(prev);

      while (  next != nullptr
            && next->level >= level)
      {
         /**
          * if we encounter a comma or semicolon at the level of the starting chunk,
          * return the current chunk
          */
         if (  next->level == level
            && (  chunk_is_token(next, CT_COMMA)
               || chunk_is_semicolon(next)))
         {
            break;
         }
         /**
          * check the template nest level; if the current chunk's nest level
          * is less than that of the starting chunk, return the current chunk
          */
         auto next_template_nest = get_cpp_template_angle_nest_level(next);

         if (template_nest > next_template_nest)
         {
            break;
         }
         prev = next;
         next = (*chunk_get_next)(next, scope_e::PREPROC);
      }
   }
   return(prev);
}


chunk_t *skip_to_expression_end(chunk_t *pc)
{
   return(skip_to_expression_edge(pc, chunk_get_next_ncnnl));
}


chunk_t *skip_to_expression_start(chunk_t *pc)
{
   return(skip_to_expression_edge(pc, chunk_get_prev_ncnnlni));
}


chunk_t *skip_to_next_statement(chunk_t *pc)
{
   while (  pc != nullptr
         && !chunk_is_semicolon(pc)
         && chunk_is_not_token(pc, CT_BRACE_OPEN)
         && chunk_is_not_token(pc, CT_BRACE_CLOSE))
   {
      pc = chunk_get_next_ncnnl(pc);
   }
   return(pc);
}


chunk_t *skip_parent_types(chunk_t *colon)
{
   auto pc = chunk_get_next_ncnnlnp(colon);

   while (pc)
   {
      // Skip access specifier
      if (chunk_is_token(pc, CT_ACCESS))
      {
         pc = chunk_get_next_ncnnlnp(pc);
         continue;
      }

      // Check for a type name
      if (!(  chunk_is_token(pc, CT_WORD)
           || chunk_is_token(pc, CT_TYPE)))
      {
         LOG_FMT(LPCU,
                 "%s is confused; expected a word at %zu:%zu "
                 "following type list at %zu:%zu\n", __func__,
                 colon->orig_line, colon->orig_col,
                 pc->orig_line, pc->orig_col);
         return(colon);
      }
      // Get next token
      auto next = skip_template_next(chunk_get_next_ncnnlnp(pc));

      if (  chunk_is_token(next, CT_DC_MEMBER)
         || chunk_is_token(next, CT_COMMA))
      {
         pc = chunk_get_next_ncnnlnp(next);
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
      return(chunk_get_prev_ncnnlni(pc));   // Issue #2279
   }
   return(ang_close);
}


chunk_t *skip_tsquare_next(chunk_t *ary_def)
{
   if (  chunk_is_token(ary_def, CT_SQUARE_OPEN)
      || chunk_is_token(ary_def, CT_TSQUARE))
   {
      return(chunk_get_next_nisq(ary_def));
   }
   return(ary_def);
}


chunk_t *skip_attribute(chunk_t *attr)
{
   chunk_t *pc = attr;

   while (chunk_is_token(pc, CT_ATTRIBUTE))
   {
      pc = chunk_get_next_ncnnl(pc);

      if (chunk_is_token(pc, CT_FPAREN_OPEN))
      {
         pc = chunk_get_next_type(pc, CT_FPAREN_CLOSE, pc->level);
      }
   }
   return(pc);
}


chunk_t *skip_attribute_next(chunk_t *attr)
{
   chunk_t *next = skip_attribute(attr);

   if (  next != attr
      && chunk_is_token(next, CT_FPAREN_CLOSE))
   {
      attr = chunk_get_next_ncnnl(next);
   }
   return(attr);
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
      pc = chunk_get_prev_ncnnlni(pc);   // Issue #2279
   }
   return(pc);
}


chunk_t *skip_declspec(chunk_t *pc)
{
   if (chunk_is_token(pc, CT_DECLSPEC))
   {
      pc = chunk_get_next_ncnnl(pc);

      if (chunk_is_token(pc, CT_PAREN_OPEN))
      {
         pc = chunk_skip_to_match(pc);
      }
   }
   return(pc);
}


chunk_t *skip_declspec_next(chunk_t *pc)
{
   chunk_t *next = skip_declspec(pc);

   if (  next != pc
      && chunk_is_token(next, CT_PAREN_CLOSE))
   {
      pc = chunk_get_next_ncnnl(next);
   }
   return(pc);
}


chunk_t *skip_declspec_prev(chunk_t *pc)
{
   if (  chunk_is_token(pc, CT_PAREN_CLOSE)
      && get_chunk_parent_type(pc) == CT_DECLSPEC)
   {
      pc = chunk_skip_to_match_rev(pc);
      pc = chunk_get_prev_ncnnlni(pc);

      if (chunk_is_token(pc, CT_DECLSPEC))
      {
         pc = chunk_get_prev_ncnnlni(pc);
      }
   }
   return(pc);
}


chunk_t *skip_matching_brace_bracket_paren_next(chunk_t *pc)
{
   if (  chunk_is_token(pc, CT_BRACE_OPEN)
      || chunk_is_token(pc, CT_PAREN_OPEN)
      || chunk_is_token(pc, CT_SQUARE_OPEN))
   {
      pc = chunk_skip_to_match(pc);

      if (pc != nullptr)
      {
         /**
          * a matching brace, square bracket, or paren was found;
          * retrieve the subsequent chunk
          */

         pc = chunk_get_next_ncnnl(pc);
      }
   }
   return(pc);
}


chunk_t *skip_to_chunk_before_matching_brace_bracket_paren_rev(chunk_t *pc)
{
   if (  chunk_is_token(pc, CT_BRACE_CLOSE)
      || chunk_is_token(pc, CT_PAREN_CLOSE)
      || chunk_is_token(pc, CT_SQUARE_CLOSE))
   {
      pc = chunk_skip_to_match_rev(pc);

      if (pc != nullptr)
      {
         /**
          * a matching brace, square bracket, or paren was found;
          * retrieve the preceding chunk
          */

         pc = chunk_get_prev_ncnnlni(pc);
      }
   }
   return(pc);
}
