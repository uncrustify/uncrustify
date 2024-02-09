/**
 * @file combine_skip.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "tokenizer/combine_skip.h"

#include "tokenizer/combine_tools.h"


Chunk *skip_align(Chunk *start)
{
   Chunk *pc = start;

   if (pc->Is(CT_ALIGN))
   {
      pc = pc->GetNextNcNnl();

      if (pc->Is(CT_PAREN_OPEN))
      {
         pc = pc->GetNextType(CT_PAREN_CLOSE, pc->GetLevel());

         if (pc->IsNotNullChunk())
         {
            pc = pc->GetNextNcNnl();
         }

         if (pc->Is(CT_COLON))
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

   if (prev->IsNotNullChunk())
   {
      std::size_t level         = prev->GetLevel();
      Chunk       *next         = prev;
      std::size_t template_nest = get_cpp_template_angle_nest_level(prev);

      while (  next->IsNotNullChunk()
            && next->GetLevel() >= level)
      {
         /**
          * if we encounter a comma or semicolon at the level of the starting chunk,
          * return the current chunk
          */
         if (  next->GetLevel() == level
            && (  next->Is(CT_COMMA)
               || next->IsSemicolon()))
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
   while (  pc->IsNotNullChunk()
         && !pc->IsSemicolon()
         && pc->IsNot(CT_BRACE_OPEN)
         && pc->IsNot(CT_BRACE_CLOSE))
   {
      pc = pc->GetNextNcNnl();
   }
   return(pc);
}


Chunk *skip_template_prev(Chunk *ang_close)
{
   if (ang_close->Is(CT_ANGLE_CLOSE))
   {
      Chunk *pc = ang_close->GetPrevType(CT_ANGLE_OPEN, ang_close->GetLevel());
      return(pc->GetPrevNcNnlNi());   // Issue #2279
   }
   return(ang_close);
}


Chunk *skip_tsquare_next(Chunk *ary_def)
{
   if (  ary_def->Is(CT_SQUARE_OPEN)
      || ary_def->Is(CT_TSQUARE))
   {
      return(ary_def->GetNextNisq());
   }
   return(ary_def);
}


Chunk *skip_attribute(Chunk *attr)
{
   Chunk *pc = attr;

   while (pc->Is(CT_ATTRIBUTE))
   {
      pc = pc->GetNextNcNnl();

      if (pc->Is(CT_FPAREN_OPEN))
      {
         pc = pc->GetNextType(CT_FPAREN_CLOSE, pc->GetLevel());
      }
   }
   return(pc);
}


Chunk *skip_attribute_next(Chunk *attr)
{
   Chunk *next = skip_attribute(attr);

   if (  next != attr
      && next->Is(CT_FPAREN_CLOSE))
   {
      attr = next->GetNextNcNnl();
   }
   return(attr);
}


Chunk *skip_attribute_prev(Chunk *fp_close)
{
   Chunk *pc = fp_close;

   while (true)
   {
      if (  pc->Is(CT_FPAREN_CLOSE)
         && pc->GetParentType() == CT_ATTRIBUTE)
      {
         pc = pc->GetPrevType(CT_ATTRIBUTE, pc->GetLevel());
      }
      else if (pc->IsNot(CT_ATTRIBUTE))
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
   if (pc->Is(CT_DECLSPEC))
   {
      pc = pc->GetNextNcNnl();

      if (pc->Is(CT_PAREN_OPEN))
      {
         pc = pc->GetClosingParen();
      }
   }
   return(pc);
}


Chunk *skip_declspec_next(Chunk *pc)
{
   Chunk *next = skip_declspec(pc);

   if (  next != pc
      && next->Is(CT_PAREN_CLOSE))
   {
      pc = next->GetNextNcNnl();
   }
   return(pc);
}
