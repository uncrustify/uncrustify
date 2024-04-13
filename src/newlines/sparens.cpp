/**
 * @file sparens.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/sparens.h"

#include "chunk.h"
#include "log_rules.h"
#include "newlines/iarf.h"
#include "options.h"


using namespace uncrustify;


constexpr static auto LCURRENT = LNEWLINE;


void newlines_sparens()
{
   LOG_FUNC_ENTRY();

   for (Chunk *sparen_open = Chunk::GetHead()->GetNextType(CT_SPAREN_OPEN, ANY_LEVEL);
        sparen_open->IsNotNullChunk();
        sparen_open = sparen_open->GetNextType(CT_SPAREN_OPEN, ANY_LEVEL))
   {
      Chunk *sparen_close = sparen_open->GetNextType(CT_SPAREN_CLOSE, sparen_open->GetLevel());

      if (sparen_close->IsNullChunk())
      {
         continue;
      }
      Chunk *sparen_content_start = sparen_open->GetNextNnl();
      Chunk *sparen_content_end   = sparen_close->GetPrevNnl();
      bool  is_multiline          = (
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
         Chunk *ctrl_structure = sparen_open->GetPrevNcNnl();

         if (  ctrl_structure->Is(CT_IF)
            || ctrl_structure->Is(CT_ELSEIF))
         {
            log_rule_B("nl_before_if_closing_paren");
            newline_iarf_pair(sparen_content_end, sparen_close, options::nl_before_if_closing_paren());
         }
      }
   }
} // newlines_sparens
