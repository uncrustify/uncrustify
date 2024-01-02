/**
 * @file newline_template_option.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "newline_template_option.h"


iarf_e newline_template_option(Chunk *pc, iarf_e special, iarf_e base, iarf_e fallback)
{
   Chunk *const prev = pc->GetPrevNcNnl();

   if (  prev->Is(CT_ANGLE_OPEN)
      && special != IARF_IGNORE)
   {
      return(special);
   }
   else if (base != IARF_IGNORE)
   {
      return(base);
   }
   else
   {
      return(fallback);
   }
} // newline_template_option
