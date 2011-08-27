/**
 * @file detect.cpp
 * Scans the parsed file and tries to determine options.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "uncrustify_types.h"
#include "chunk_list.h"
#include "ChunkStack.h"
#include "align_stack.h"
#include "prototypes.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include "unc_ctype.h"


class sp_votes
{
protected:
   int      m_add;
   int      m_remove;
   int      m_force;
   argval_t *m_av;

public:
   sp_votes(argval_t& av)
   {
      m_add    = 0;
      m_remove = 0;
      m_force  = 0;
      m_av     = &av;
   }


   ~sp_votes();

   void vote(chunk_t *first, chunk_t *second);
};

void sp_votes::vote(chunk_t *first, chunk_t *second)
{
   if ((first == NULL) || chunk_is_newline(first) ||
       (second == NULL) || chunk_is_newline(second))
   {
      return;
   }

   int col_dif = second->column - (first->column + first->len());
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


/**
 * Figure out the result of the vote and maybe update *m_av
 */
sp_votes::~sp_votes()
{
   /* no change if no items were added */
   if ((m_remove == 0) && (m_add == 0) && (m_force == 0))
   {
      return;
   }

   if (m_remove == 0)
   {
      *m_av = (m_force > m_add) ? AV_FORCE : AV_ADD;
   }
   else if ((m_force == 0) && (m_add == 0))
   {
      *m_av = AV_REMOVE;
   }
   else
   {
      /* nothing conclusive. do not alter. */
   }
}


#define SP_VOTE_VAR(x)    sp_votes vote_ ## x(cpd.settings[UO_ ## x].a)

/**
 * Detect spacing options
 */
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

   chunk_t *prev = chunk_get_head();
   chunk_t *pc   = chunk_get_next(prev);
   chunk_t *next;

   while (pc != NULL)
   {
      next = chunk_get_next(pc);
      if (next == NULL)
      {
         break;
      }

      if (pc->type == CT_ARITH)
      {
         vote_sp_arith.vote(pc, next);
         vote_sp_arith.vote(prev, pc);
      }
      if (pc->type == CT_ASSIGN)
      {
         if ((pc->flags & PCF_IN_ENUM) == 0)
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
      if (pc->type == CT_SQUARE_OPEN)
      {
         vote_sp_before_square.vote(prev, pc);
         vote_sp_inside_square.vote(pc, next);
      }
      if (pc->type == CT_SQUARE_CLOSE)
      {
         vote_sp_inside_square.vote(prev, pc);
      }
      if (pc->type == CT_TSQUARE)
      {
         vote_sp_before_squares.vote(prev, pc);
      }
      if (pc->type == CT_BOOL)
      {
         vote_sp_bool.vote(prev, pc);
         vote_sp_bool.vote(pc, next);
      }
      if (pc->type == CT_COMPARE)
      {
         vote_sp_compare.vote(prev, pc);
         vote_sp_compare.vote(pc, next);
      }
      if (pc->type == CT_PAREN_CLOSE)
      {
         vote_sp_inside_paren.vote(prev, pc);
      }
      if (pc->type == CT_PAREN_OPEN)
      {
         vote_sp_inside_paren.vote(pc, next);
      }
      if ((chunk_is_paren_open(pc) && chunk_is_paren_open(next)) ||
          (chunk_is_paren_close(pc) && chunk_is_paren_close(next)))
      {
         vote_sp_paren_paren.vote(pc, next);
      }
      if (chunk_is_paren_close(pc) && (next->type == CT_BRACE_OPEN))
      {
         vote_sp_paren_brace.vote(pc, next);
      }
      if (pc->type == CT_PTR_TYPE)
      {
         if (prev->type == CT_PTR_TYPE)
         {
            vote_sp_between_ptr_star.vote(prev, pc);
         }
         else if (next->type != CT_WORD)
         {
            vote_sp_before_unnamed_ptr_star.vote(prev, pc);
         }
         else
         {
            vote_sp_before_ptr_star.vote(prev, pc);
         }
         if (CharTable::IsKw1(next->str[0]))
         {
            vote_sp_after_ptr_star.vote(pc, next);
         }
      }
      if (pc->type == CT_BYREF)
      {
         if (next->type != CT_WORD)
         {
            vote_sp_before_unnamed_byref.vote(prev, pc);
         }
         else
         {
            vote_sp_before_byref.vote(prev, pc);
         }
         vote_sp_after_byref.vote(pc, next);
      }
      if ((pc->type != CT_PTR_TYPE) &&
          ((prev->type == CT_QUALIFIER) || (prev->type == CT_TYPE)))
      {
         vote_sp_after_type.vote(prev, pc);
      }
      if (pc->type == CT_ANGLE_OPEN)
      {
         vote_sp_inside_angle.vote(pc, next);
         if (prev->type == CT_TEMPLATE)
         {
            vote_sp_template_angle.vote(prev, pc);
         }
         else
         {
            vote_sp_before_angle.vote(prev, pc);
         }
      }
      if (pc->type == CT_ANGLE_CLOSE)
      {
         vote_sp_inside_angle.vote(prev, pc);
         if (chunk_is_paren_open(next))
         {
            vote_sp_angle_paren.vote(prev, pc);
         }
         else if ((next->type == CT_WORD) || CharTable::IsKw1(next->str[0]))
         {
            vote_sp_angle_word.vote(prev, pc);
         }
         else
         {
            vote_sp_after_angle.vote(pc, next);
         }
      }

      if (pc->type == CT_SPAREN_OPEN)
      {
         vote_sp_before_sparen.vote(prev, pc);
         vote_sp_inside_sparen.vote(pc, next);
      }
      if (pc->type == CT_SPAREN_CLOSE)
      {
         vote_sp_inside_sparen.vote(prev, pc);
         if (next->type == CT_BRACE_OPEN)
         {
            vote_sp_sparen_brace.vote(pc, next);
         }
         else
         {
            vote_sp_after_sparen.vote(pc, next);
         }
      }
      if (pc->type == CT_SEMICOLON)
      {
         if (pc->parent_type == CT_FOR)
         {
            if (prev->type == CT_SPAREN_OPEN)
            {
               /* emtpy, ie for (;;) */
               vote_sp_before_semi_for_empty.vote(prev, pc);
            }
            else if (next->type == CT_SPAREN_CLOSE)
            {
               /* emtpy, ie for (;;) */
               vote_sp_after_semi_for_empty.vote(pc, next);
            }
            else if (prev->type != CT_SEMICOLON)
            {
               vote_sp_before_semi_for.vote(prev, pc);
            }
         }
         else if (prev->type == CT_VBRACE_OPEN)
         {
            vote_sp_special_semi.vote(chunk_get_prev(prev), pc);
         }
         else
         {
            vote_sp_before_semi.vote(prev, pc);
         }
      }
      if (pc->type == CT_COMMA)
      {
         vote_sp_before_comma.vote(prev, pc);
         vote_sp_after_comma.vote(pc, next);
      }
      if (pc->type == CT_CLASS_COLON)
      {
         vote_sp_before_class_colon.vote(prev, pc);
         vote_sp_after_class_colon.vote(pc, next);
      }
      if (pc->type == CT_BRACE_OPEN)
      {
         if (prev->type == CT_ELSE)
         {
            vote_sp_else_brace.vote(prev, pc);
         }
         else if (prev->type == CT_CATCH)
         {
            vote_sp_catch_brace.vote(prev, pc);
         }
         else if (prev->type == CT_FINALLY)
         {
            vote_sp_catch_brace.vote(prev, pc);
         }
         else if (prev->type == CT_TRY)
         {
            vote_sp_catch_brace.vote(prev, pc);
         }
         else if (prev->type == CT_GETSET)
         {
            vote_sp_catch_brace.vote(prev, pc);
         }
         if (next->type == CT_BRACE_CLOSE)
         {
            vote_sp_inside_braces_empty.vote(pc, next);
         }
         else
         {
            vote_sp_inside_braces.vote(pc, next);
         }
      }
      if (pc->type == CT_BRACE_CLOSE)
      {
         vote_sp_inside_braces.vote(prev, pc);
         if (next->type == CT_ELSE)
         {
            vote_sp_brace_else.vote(pc, next);
         }
         else if (next->type == CT_CATCH)
         {
            vote_sp_brace_catch.vote(pc, next);
         }
         else if (next->type == CT_FINALLY)
         {
            vote_sp_brace_finally.vote(pc, next);
         }
      }

      prev = pc;
      pc   = next;
   }
}


// unc_add_option("sp_after_operator", UO_sp_after_operator, AT_IARF,
//                "Add or remove space between 'operator' and operator sign");
// unc_add_option("sp_after_operator_sym", UO_sp_after_operator_sym, AT_IARF,
//                "Add or remove space between the operator symbol and the open paren, as in 'operator ++('");
// unc_add_option("sp_after_cast", UO_sp_after_cast, AT_IARF,
//                "Add or remove space after C/D cast, ie 'cast(int)a' vs 'cast(int) a' or '(int)a' vs '(int) a'");
// unc_add_option("sp_inside_paren_cast", UO_sp_inside_paren_cast, AT_IARF,
//                "Add or remove spaces inside cast parens");
// unc_add_option("sp_sizeof_paren", UO_sp_sizeof_paren, AT_IARF,
//                "Add or remove space between 'sizeof' and '('");
// unc_add_option("sp_after_tag", UO_sp_after_tag, AT_IARF,
//                "Add or remove space after the tag keyword (Pawn)");
// unc_add_option("sp_inside_braces_enum", UO_sp_inside_braces_enum, AT_IARF,
//                "Add or remove space inside enum '{' and '}'");
// unc_add_option("sp_inside_braces_struct", UO_sp_inside_braces_struct, AT_IARF,
//                "Add or remove space inside struct/union '{' and '}'");
// unc_add_option("sp_type_func", UO_sp_type_func, AT_IARF,
//                "Add or remove space between return type and function name\n"
//                "A minimum of 1 is forced except for pointer return types.");
// unc_add_option("sp_func_proto_paren", UO_sp_func_proto_paren, AT_IARF,
//                "Add or remove space between function name and '(' on function declaration");
// unc_add_option("sp_func_def_paren", UO_sp_func_def_paren, AT_IARF,
//                "Add or remove space between function name and '(' on function definition");
// unc_add_option("sp_inside_fparens", UO_sp_inside_fparens, AT_IARF,
//                "Add or remove space inside empty function '()'");
// unc_add_option("sp_inside_fparen", UO_sp_inside_fparen, AT_IARF,
//                "Add or remove space inside function '(' and ')'");
// unc_add_option("sp_square_fparen", UO_sp_square_fparen, AT_IARF,
//                "Add or remove space between ']' and '(' when part of a function call.");
// unc_add_option("sp_fparen_brace", UO_sp_fparen_brace, AT_IARF,
//                "Add or remove space between ')' and '{' of function");
// unc_add_option("sp_func_call_paren", UO_sp_func_call_paren, AT_IARF,
//                "Add or remove space between function name and '(' on function calls");
// unc_add_option("sp_func_class_paren", UO_sp_func_class_paren, AT_IARF,
//                "Add or remove space between a constructor/destructor and the open paren");
// unc_add_option("sp_return_paren", UO_sp_return_paren, AT_IARF,
//                "Add or remove space between 'return' and '('");
// unc_add_option("sp_attribute_paren", UO_sp_attribute_paren, AT_IARF,
//                "Add or remove space between '__attribute__' and '('");
// unc_add_option("sp_defined_paren", UO_sp_defined_paren, AT_IARF,
//                "Add or remove space between 'defined' and '(' in '#if defined (FOO)'");
// unc_add_option("sp_macro", UO_sp_macro, AT_IARF,
//                "Add or remove space between macro and value");
// unc_add_option("sp_macro_func", UO_sp_macro_func, AT_IARF,
//                "Add or remove space between macro function ')' and value");
// unc_add_option("sp_before_dc", UO_sp_before_dc, AT_IARF,
//                "Add or remove space before the '::' operator");
// unc_add_option("sp_after_dc", UO_sp_after_dc, AT_IARF,
//                "Add or remove space after the '::' operator");
// unc_add_option("sp_d_array_colon", UO_sp_d_array_colon, AT_IARF,
//                "Add or remove around the D named array initializer ':' operator");
// unc_add_option("sp_not", UO_sp_not, AT_IARF,
//                "Add or remove space after the '!' (not) operator.");
// unc_add_option("sp_inv", UO_sp_inv, AT_IARF, "Add or remove space after the '~' (invert) operator.");
// unc_add_option("sp_addr", UO_sp_addr, AT_IARF,
//                "Add or remove space after the '&' (address-of) operator.\n"
//                "This does not affect the spacing after a '&' that is part of a type.");
// unc_add_option("sp_member", UO_sp_member, AT_IARF,
//                "Add or remove space around the '.' or '->' operators\n");
// unc_add_option("sp_deref", UO_sp_deref, AT_IARF,
//                "Add or remove space after the '*' (dereference) operator.\n"
//                "This does not affect the spacing after a '*' that is part of a type.");
// unc_add_option("sp_sign", UO_sp_sign, AT_IARF,
//                "Add or remove space after '+' or '-', as in 'x = -5' or 'y = +7'");
// unc_add_option("sp_incdec", UO_sp_incdec, AT_IARF,
//                "Add or remove space before or after '++' and '--', as in '(--x)' or 'y++;'");
//
// unc_add_option("sp_before_nl_cont", UO_sp_before_nl_cont, AT_IARF,
//                "Add or remove space before a backslash-newline at the end of a line");
//
// unc_add_option("sp_after_oc_scope", UO_sp_after_oc_scope, AT_IARF,
//                "Add or remove space after the scope '+' or '-', as in '-(void) foo;' or '+(int) bar;'");
// unc_add_option("sp_before_oc_colon", UO_sp_before_oc_colon, AT_IARF,
//                "Add or remove space after the colon in message specs\n"
//                "'-(int) f: (int) x;' vs '+(int) f : (int) x;'");
// unc_add_option("sp_after_oc_type", UO_sp_after_oc_type, AT_IARF,
//                "Add or remove space after the (type) in message specs\n"
//                "'-(int) f: (int) x;' vs '+(int)f : (int)x;'");
//
// unc_add_option("sp_cond_colon", UO_sp_cond_colon, AT_IARF,
//                "Add or remove space around the ':' in 'b ? t : f'");
// unc_add_option("sp_cond_question", UO_sp_cond_question, AT_IARF,
//                "Add or remove space around the '?' in 'b ? t : f'");


/**
 * Call all the detect_xxxx() functions
 */
void detect_options()
{
   detect_space_options();
}
