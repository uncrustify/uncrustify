/**
 * @file detect.cpp
 * Scans the parsed file and tries to determine options.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "detect.h"

#include "prototypes.h"


using namespace uncrustify;


//! Detect spacing options
static void detect_space_options();


class sp_votes
{
protected:
   size_t         m_add    = 0;
   size_t         m_remove = 0;
   size_t         m_force  = 0;
   Option<iarf_e> &m_option;

public:
   sp_votes(Option<iarf_e> &opt)
      : m_option(opt)
   {}

   //! Figure out the result of the vote and maybe update *m_av
   ~sp_votes();

   void vote(Chunk *first, Chunk *second);
};


void sp_votes::vote(Chunk *first, Chunk *second)
{
   if (  first->IsNullChunk()
      || first->IsNewline()
      || second->IsNullChunk()
      || second->IsNewline())
   {
      return;
   }
   int col_dif = second->GetColumn() - (first->GetColumn() + first->Len());

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


//! Figure out the result of the vote and maybe update *m_av
sp_votes::~sp_votes()
{
   // no change if no items were added
   if (  m_remove == 0
      && m_add == 0
      && m_force == 0)
   {
      return;
   }

   if (m_remove == 0)
   {
      m_option = (m_force > m_add) ? IARF_FORCE : IARF_ADD;
   }
   else if (  m_force == 0
           && m_add == 0)
   {
      m_option = IARF_REMOVE;
   }
   else
   {
      // nothing conclusive. do not alter.
   }
}


// generates "vote_sp_xxx" variable name from uncrustify option name "options::xxx()"
#define SP_VOTE_VAR(x)    sp_votes vote_ ## x(options::x)


static void detect_space_options()
{
   SP_VOTE_VAR(sp_arith);
   SP_VOTE_VAR(sp_before_assign);
   SP_VOTE_VAR(sp_after_assign);
   SP_VOTE_VAR(sp_enum_before_assign);
   SP_VOTE_VAR(sp_enum_after_assign);
   SP_VOTE_VAR(sp_bool);
   SP_VOTE_VAR(sp_compare);
   SP_VOTE_VAR(sp_inside_paren);
   SP_VOTE_VAR(sp_paren_paren);
   SP_VOTE_VAR(sp_paren_brace);
   SP_VOTE_VAR(sp_before_ptr_star);
   SP_VOTE_VAR(sp_before_unnamed_ptr_star);
   SP_VOTE_VAR(sp_between_ptr_star);
   SP_VOTE_VAR(sp_between_ptr_ref);
   SP_VOTE_VAR(sp_after_ptr_star);
   SP_VOTE_VAR(sp_after_byref);
   SP_VOTE_VAR(sp_before_byref);
   SP_VOTE_VAR(sp_before_unnamed_byref);
   SP_VOTE_VAR(sp_after_type);
   SP_VOTE_VAR(sp_template_angle);
   SP_VOTE_VAR(sp_before_angle);
   SP_VOTE_VAR(sp_inside_angle);
   SP_VOTE_VAR(sp_after_angle);
   SP_VOTE_VAR(sp_angle_paren);
   SP_VOTE_VAR(sp_angle_word);
   SP_VOTE_VAR(sp_before_square);
   SP_VOTE_VAR(sp_before_squares);
   SP_VOTE_VAR(sp_inside_square);
   SP_VOTE_VAR(sp_before_sparen);
   SP_VOTE_VAR(sp_inside_sparen);
   SP_VOTE_VAR(sp_after_sparen);
   SP_VOTE_VAR(sp_sparen_brace);
   SP_VOTE_VAR(sp_special_semi);
   SP_VOTE_VAR(sp_before_semi);
   SP_VOTE_VAR(sp_before_semi_for);
   SP_VOTE_VAR(sp_before_semi_for_empty);
   SP_VOTE_VAR(sp_after_semi_for_empty);
   SP_VOTE_VAR(sp_after_comma);
   SP_VOTE_VAR(sp_before_comma);
   SP_VOTE_VAR(sp_after_class_colon);
   SP_VOTE_VAR(sp_before_class_colon);
   SP_VOTE_VAR(sp_inside_braces);
   SP_VOTE_VAR(sp_inside_braces_empty);
   SP_VOTE_VAR(sp_else_brace);
   SP_VOTE_VAR(sp_brace_else);
   SP_VOTE_VAR(sp_catch_brace);
   SP_VOTE_VAR(sp_brace_catch);
   SP_VOTE_VAR(sp_finally_brace);
   SP_VOTE_VAR(sp_brace_finally);
   SP_VOTE_VAR(sp_try_brace);
   SP_VOTE_VAR(sp_getset_brace);

   Chunk *prev = Chunk::GetHead();
   Chunk *pc   = prev->GetNext();
   Chunk *next;

   while (pc->IsNotNullChunk())
   {
      next = pc->GetNext();

      if (next->IsNullChunk())
      {
         break;
      }

      if (  pc->Is(CT_ARITH)
         || pc->Is(CT_SHIFT))
      {
         vote_sp_arith.vote(pc, next);
         vote_sp_arith.vote(prev, pc);
      }

      if (pc->Is(CT_ASSIGN))
      {
         if (!pc->TestFlags(PCF_IN_ENUM))
         {
            vote_sp_before_assign.vote(prev, pc);
            vote_sp_after_assign.vote(pc, next);
         }
         else
         {
            vote_sp_enum_before_assign.vote(prev, pc);
            vote_sp_enum_after_assign.vote(pc, next);
         }
      }

      if (pc->Is(CT_SQUARE_OPEN))
      {
         vote_sp_before_square.vote(prev, pc);
         vote_sp_inside_square.vote(pc, next);
      }

      if (pc->Is(CT_SQUARE_CLOSE))
      {
         vote_sp_inside_square.vote(prev, pc);
      }

      if (pc->Is(CT_TSQUARE))
      {
         vote_sp_before_squares.vote(prev, pc);
      }

      if (pc->Is(CT_BOOL))
      {
         vote_sp_bool.vote(prev, pc);
         vote_sp_bool.vote(pc, next);
      }

      if (pc->Is(CT_COMPARE))
      {
         vote_sp_compare.vote(prev, pc);
         vote_sp_compare.vote(pc, next);
      }

      if (pc->Is(CT_PAREN_CLOSE))
      {
         vote_sp_inside_paren.vote(prev, pc);
      }

      if (pc->Is(CT_PAREN_OPEN))
      {
         vote_sp_inside_paren.vote(pc, next);
      }

      if (  (  pc->IsParenOpen()
            && next->IsParenOpen())
         || (  pc->IsParenClose()
            && next->IsParenClose()))
      {
         vote_sp_paren_paren.vote(pc, next);
      }

      if (  pc->IsParenClose()
         && next->Is(CT_BRACE_OPEN))
      {
         vote_sp_paren_brace.vote(pc, next);
      }

      if (pc->Is(CT_PTR_TYPE))
      {
         if (prev->Is(CT_PTR_TYPE))
         {
            vote_sp_between_ptr_star.vote(prev, pc);
         }
         else if (next->IsNot(CT_WORD))
         {
            vote_sp_before_unnamed_ptr_star.vote(prev, pc);
         }
         else
         {
            vote_sp_before_ptr_star.vote(prev, pc);
         }

         if (CharTable::IsKw1(next->GetStr()[0]))
         {
            vote_sp_after_ptr_star.vote(pc, next);
         }
      }

      if (pc->Is(CT_BYREF))
      {
         if (next->IsNot(CT_WORD))
         {
            vote_sp_before_unnamed_byref.vote(prev, pc);
         }

         if (prev->Is(CT_PTR_TYPE))
         {
            vote_sp_between_ptr_ref.vote(prev, pc);
         }
         else
         {
            vote_sp_before_byref.vote(prev, pc);
         }
         vote_sp_after_byref.vote(pc, next);
      }

      if (  pc->IsNot(CT_PTR_TYPE)
         && (  prev->Is(CT_QUALIFIER)
            || prev->Is(CT_TYPE)))
      {
         vote_sp_after_type.vote(prev, pc);
      }

      if (pc->Is(CT_ANGLE_OPEN))
      {
         vote_sp_inside_angle.vote(pc, next);

         if (prev->Is(CT_TEMPLATE))
         {
            vote_sp_template_angle.vote(prev, pc);
         }
         else
         {
            vote_sp_before_angle.vote(prev, pc);
         }
      }

      if (pc->Is(CT_ANGLE_CLOSE))
      {
         vote_sp_inside_angle.vote(prev, pc);

         if (next->IsParenOpen())
         {
            vote_sp_angle_paren.vote(prev, pc);
         }
         else if (  next->Is(CT_WORD)
                 || CharTable::IsKw1(next->GetStr()[0]))
         {
            vote_sp_angle_word.vote(prev, pc);
         }
         else
         {
            vote_sp_after_angle.vote(pc, next);
         }
      }

      if (pc->Is(CT_SPAREN_OPEN))
      {
         vote_sp_before_sparen.vote(prev, pc);
         vote_sp_inside_sparen.vote(pc, next);
      }

      if (pc->Is(CT_SPAREN_CLOSE))
      {
         vote_sp_inside_sparen.vote(prev, pc);

         if (next->Is(CT_BRACE_OPEN))
         {
            vote_sp_sparen_brace.vote(pc, next);
         }
         else
         {
            vote_sp_after_sparen.vote(pc, next);
         }
      }

      if (pc->Is(CT_SEMICOLON))
      {
         if (pc->GetParentType() == CT_FOR)
         {
            if (prev->Is(CT_SPAREN_OPEN))
            {
               // empty, ie for (;;)
               //               ^ is prev
               //                ^ is pc
               vote_sp_before_semi_for_empty.vote(prev, pc);
            }
            else if (next->Is(CT_SPAREN_CLOSE))
            {
               // empty, ie for (;;)
               //                 ^ is pc
               //                  ^ is next
               vote_sp_after_semi_for_empty.vote(pc, next);
            }
            else if (prev->IsNot(CT_SEMICOLON))
            {
               // empty, ie for (; i < 8;)
               //                       ^ is pc
               // or
               //                      ^ is prev
               vote_sp_before_semi_for.vote(prev, pc);
            }
         }
         else if (prev->Is(CT_VBRACE_OPEN))
         {
            vote_sp_special_semi.vote(prev->GetPrev(), pc);
         }
         else
         {
            vote_sp_before_semi.vote(prev, pc);
         }
      }

      if (pc->Is(CT_COMMA))
      {
         vote_sp_before_comma.vote(prev, pc);
         vote_sp_after_comma.vote(pc, next);
      }

      if (pc->Is(CT_CLASS_COLON))
      {
         vote_sp_before_class_colon.vote(prev, pc);
         vote_sp_after_class_colon.vote(pc, next);
      }

      if (pc->Is(CT_BRACE_OPEN))
      {
         if (prev->Is(CT_ELSE))
         {
            vote_sp_else_brace.vote(prev, pc);
         }
         else if (prev->Is(CT_CATCH))
         {
            vote_sp_catch_brace.vote(prev, pc);
         }
         else if (prev->Is(CT_FINALLY))
         {
            vote_sp_catch_brace.vote(prev, pc);
         }
         else if (prev->Is(CT_TRY))
         {
            vote_sp_catch_brace.vote(prev, pc);
         }
         else if (prev->Is(CT_GETSET))
         {
            vote_sp_catch_brace.vote(prev, pc);
         }

         if (next->Is(CT_BRACE_CLOSE))
         {
            vote_sp_inside_braces_empty.vote(pc, next);
         }
         else
         {
            vote_sp_inside_braces.vote(pc, next);
         }
      }

      if (pc->Is(CT_BRACE_CLOSE))
      {
         vote_sp_inside_braces.vote(prev, pc);

         if (next->Is(CT_ELSE))
         {
            vote_sp_brace_else.vote(pc, next);
         }
         else if (next->Is(CT_CATCH))
         {
            vote_sp_brace_catch.vote(pc, next);
         }
         else if (next->Is(CT_FINALLY))
         {
            vote_sp_brace_finally.vote(pc, next);
         }
      }
      prev = pc;
      pc   = next;
   }
} // detect_space_options


void detect_options()
{
   detect_space_options();
}
