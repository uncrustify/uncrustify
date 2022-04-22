/**
 * @file combine_skip.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 * extract from combine.cpp
 */

#include "combine_skip.h"

#include "combine_tools.h"


Chunk *skip_align(Chunk *start)
{
   Chunk *pc = start;

   if (chunk_is_token(pc, CT_ALIGN))
   {
      pc = pc->GetNextNcNnl();

      if (chunk_is_token(pc, CT_PAREN_OPEN))
      {
         pc = pc->GetNextType(CT_PAREN_CLOSE, pc->level);

         if (pc->IsNotNullChunk())
         {
            pc = pc->GetNextNcNnl();
         }

         if (chunk_is_token(pc, CT_COLON))
         {
            pc = pc->GetNextNcNnl();
         }
      }
   }
   return(pc);
}


Chunk *skip_expression(Chunk *pc)
{
   return(skip_to_expression_end(pc)->GetNextNcNnl());
}


Chunk *skip_expression_rev(Chunk *pc)
{
   return(skip_to_expression_start(pc)->GetPrevNcNnlNi());
}


static Chunk *skip_to_expression_edge(Chunk *pc, Chunk *(Chunk::*GetNextFn)(E_Scope scope)const)
{
   Chunk *prev = pc;

   if (prev == nullptr)
   {
      prev = Chunk::NullChunkPtr;
   }

   if (prev->IsNotNullChunk())
   {
      std::size_t level         = prev->level;
      Chunk       *next         = prev;
      std::size_t template_nest = get_cpp_template_angle_nest_level(prev);

      while (  next->IsNotNullChunk()
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
         next = (next->*GetNextFn)(E_Scope::PREPROC);
      }
   }
   return(prev);
}


Chunk *skip_to_expression_end(Chunk *pc)
{
   return(skip_to_expression_edge(pc, &Chunk::GetNextNcNnl));
}


Chunk *skip_to_expression_start(Chunk *pc)
{
   return(skip_to_expression_edge(pc, &Chunk::GetPrevNcNnlNi));
}


Chunk *skip_to_next_statement(Chunk *pc)
{
   if (pc == nullptr)
   {
      pc = Chunk::NullChunkPtr;
   }

   while (  pc->IsNotNullChunk()
         && !chunk_is_semicolon(pc)
         && chunk_is_not_token(pc, CT_BRACE_OPEN)
         && chunk_is_not_token(pc, CT_BRACE_CLOSE))
   {
      pc = pc->GetNextNcNnl();
   }
   return(pc);
}


Chunk *skip_parent_types(Chunk *colon)
{
   Chunk *pc = colon->GetNextNcNnlNpp();

   while (pc)
   {
      // Skip access specifier
      if (chunk_is_token(pc, CT_ACCESS))
      {
         pc = pc->GetNextNcNnlNpp();
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
      Chunk *next = skip_template_next(pc->GetNextNcNnlNpp());

      if (  chunk_is_token(next, CT_DC_MEMBER)
         || chunk_is_token(next, CT_COMMA))
      {
         pc = next->GetNextNcNnlNpp();
      }
      else if (next != nullptr)
      {
         LOG_FMT(LPCU, "%s -> %zu:%zu ('%s')\n", __func__,
                 next->orig_line, next->orig_col, next->Text());
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


Chunk *skip_template_prev(Chunk *ang_close)
{
   if (chunk_is_token(ang_close, CT_ANGLE_CLOSE))
   {
      Chunk *pc = ang_close->GetPrevType(CT_ANGLE_OPEN, ang_close->level);
      return(pc->GetPrevNcNnlNi());   // Issue #2279
   }
   return(ang_close);
}


Chunk *skip_tsquare_next(Chunk *ary_def)
{
   if (ary_def == nullptr)
   {
      return(Chunk::NullChunkPtr);
   }

   if (  chunk_is_token(ary_def, CT_SQUARE_OPEN)
      || chunk_is_token(ary_def, CT_TSQUARE))
   {
      return(ary_def->GetNextNisq());
   }
   return(ary_def);
}


Chunk *skip_attribute(Chunk *attr)
{
   Chunk *pc = attr;

   while (chunk_is_token(pc, CT_ATTRIBUTE))
   {
      pc = pc->GetNextNcNnl();

      if (chunk_is_token(pc, CT_FPAREN_OPEN))
      {
         pc = pc->GetNextType(CT_FPAREN_CLOSE, pc->level);
      }
   }
   return(pc);
}


Chunk *skip_attribute_next(Chunk *attr)
{
   Chunk *next = skip_attribute(attr);

   if (  next != attr
      && chunk_is_token(next, CT_FPAREN_CLOSE))
   {
      attr = next->GetNextNcNnl();
   }

   if (attr == nullptr)
   {
      return(Chunk::NullChunkPtr);
   }
   return(attr);
}


Chunk *skip_attribute_prev(Chunk *fp_close)
{
   Chunk *pc = fp_close;

   while (true)
   {
      if (  chunk_is_token(pc, CT_FPAREN_CLOSE)
         && get_chunk_parent_type(pc) == CT_ATTRIBUTE)
      {
         pc = pc->GetPrevType(CT_ATTRIBUTE, pc->level);
      }
      else if (chunk_is_not_token(pc, CT_ATTRIBUTE))
      {
         break;
      }
      pc = pc->GetPrevNcNnlNi(); // Issue #2279

      if (pc->IsNullChunk())     // Issue #3356
      {
         break;
      }
   }
   return(pc);
}


Chunk *skip_declspec(Chunk *pc)
{
   if (chunk_is_token(pc, CT_DECLSPEC))
   {
      pc = pc->GetNextNcNnl();

      if (chunk_is_token(pc, CT_PAREN_OPEN))
      {
         pc = chunk_skip_to_match(pc);
      }
   }
   return(pc);
}


Chunk *skip_declspec_next(Chunk *pc)
{
   Chunk *next = skip_declspec(pc);

   if (  next != pc
      && chunk_is_token(next, CT_PAREN_CLOSE))
   {
      pc = next->GetNextNcNnl();
   }
   return(pc);
}


Chunk *skip_declspec_prev(Chunk *pc)
{
   if (  chunk_is_token(pc, CT_PAREN_CLOSE)
      && get_chunk_parent_type(pc) == CT_DECLSPEC)
   {
      pc = chunk_skip_to_match_rev(pc);
      pc = pc->GetPrevNcNnlNi();

      if (chunk_is_token(pc, CT_DECLSPEC))
      {
         pc = pc->GetPrevNcNnlNi();
      }
   }
   return(pc);
}


Chunk *skip_matching_brace_bracket_paren_next(Chunk *pc)
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

         pc = pc->GetNextNcNnl();
      }
   }
   return(pc);
}


Chunk *skip_to_chunk_before_matching_brace_bracket_paren_rev(Chunk *pc)
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

         pc = pc->GetPrevNcNnlNi();
      }
   }
   return(pc);
}
