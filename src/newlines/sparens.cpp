/**
 * @file sparens.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/sparens.h"

#include "chunk.h"
#include "for_section.h"
#include "log_rules.h"
#include "newlines/iarf.h"
#include "options.h"


using namespace uncrustify;


constexpr static auto LCURRENT = LNEWLINE;


void newlines_sparens()
{
   LOG_FUNC_ENTRY();

   for (Chunk *sparen_open = Chunk::GetHead()->GetNextType(E_Token::CT_SPAREN_OPEN, ANY_LEVEL);
        sparen_open->IsNotNullChunk();
        sparen_open = sparen_open->GetNextType(E_Token::CT_SPAREN_OPEN, ANY_LEVEL))
   {
      Chunk *sparen_close = sparen_open->GetNextType(E_Token::CT_SPAREN_CLOSE, sparen_open->GetLevel());

      if (sparen_close->IsNullChunk())
      {
         continue;
      }
      Chunk const *sparen_content_start = sparen_open->GetNextNnl();
      Chunk       *sparen_content_end   = sparen_close->GetPrevNnl();
      bool        is_multiline          = (
         sparen_content_start != sparen_content_end
                                          && !sparen_content_start->IsOnSameLine(sparen_content_end));

      // Add a newline after '(' if an if/for/while/switch condition spans multiple lines,
      // as e.g. required by the ROS 2 development style guidelines:
      // https://index.ros.org/doc/ros2/Contributing/Developer-Guide/#open-versus-cuddled-braces
      if (is_multiline)
      {
         log_rule_B("nl_multi_line_sparen_open");
         newline_iarf(sparen_open, options::nl_multi_line_sparen_open());
      }

      // Add a newline before ')' if an if/for/while/switch condition spans multiple lines. Overrides nl_before_if_closing_paren if both are specified.
      if (  is_multiline
         && options::nl_multi_line_sparen_close() != IARF_IGNORE)
      {
         log_rule_B("nl_multi_line_sparen_close");
         newline_iarf(sparen_content_end, options::nl_multi_line_sparen_close());
      }
      else
      {
         // add/remove trailing newline in an if condition
         const Chunk *ctrl_structure = sparen_open->GetPrevNcNnl();

         if (  ctrl_structure->Is(E_Token::CT_IF)
            || ctrl_structure->Is(E_Token::CT_ELSEIF))
         {
            log_rule_B("nl_before_if_closing_paren");
            newline_iarf_pair(sparen_content_end, sparen_close, options::nl_before_if_closing_paren());
         }
      }

      // Add or remove newline before operators inside the control parens of
      // a 'for' statement: nl_for_assign / nl_for_compare / nl_for_increment.
      // nl_for_increment overrides nl_for_assign in the increment section.
      if (sparen_open->GetParentType() == E_Token::CT_FOR)
      {
         for (Chunk *pc = sparen_open->GetNext();
              pc->IsNotNullChunk() && pc != sparen_close;
              pc = pc->GetNext())
         {
            if (!pc->TestFlags(PCF_IN_FOR))
            {
               continue;
            }
            Chunk *prev = pc->GetPrev();

            if (prev->IsNullChunk())
            {
               continue;
            }

            if (pc->Is(E_Token::CT_ASSIGN))
            {
               if (  get_for_section(pc) == 2
                  && options::nl_for_increment() != IARF_IGNORE)
               {
                  log_rule_B("nl_for_increment");
                  newline_iarf_pair(prev, pc, options::nl_for_increment());
               }
               else if (options::nl_for_assign() != IARF_IGNORE)
               {
                  log_rule_B("nl_for_assign");
                  newline_iarf_pair(prev, pc, options::nl_for_assign());
               }
            }
            else if (  pc->Is(E_Token::CT_COMPARE)
                    && options::nl_for_compare() != IARF_IGNORE)
            {
               log_rule_B("nl_for_compare");
               newline_iarf_pair(prev, pc, options::nl_for_compare());
            }
            else if (  (  pc->Is(E_Token::CT_INCDEC_BEFORE)
                       || pc->Is(E_Token::CT_INCDEC_AFTER))
                    && get_for_section(pc) == 2
                    && options::nl_for_increment() != IARF_IGNORE)
            {
               log_rule_B("nl_for_increment");
               newline_iarf_pair(prev, pc, options::nl_for_increment());
            }
         }
      }
   }
} // newlines_sparens
