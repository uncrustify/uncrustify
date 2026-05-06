/**
 * @file for_section.cpp
 * Identifies the section (init/compare/increment) of a 'for' statement
 * that a chunk lives in.
 *
 * @author  Rodrigo Madera
 * @license GPL v2+
 */

#include "for_section.h"

#include "chunk.h"


int get_for_section(const Chunk *pc)
{
   if (!pc->TestFlags(PCF_IN_FOR))
   {
      return(-1);
   }
   // PCF_IN_FOR is set on every chunk inside an SPAREN that has a CT_FOR
   // anywhere up the frame stack -- including chunks inside an 'if'/'while'/
   // 'switch' SPAREN nested in the body of a for. Walk back at the chunk's
   // own level so we only resolve to the *immediately* enclosing SPAREN; if
   // that one's parent is not CT_FOR, the chunk is not in a for-section.
   const size_t my_level = pc->GetLevel();
   int          count    = 0;

   for (Chunk *t = pc->GetPrev(); t->IsNotNullChunk(); t = t->GetPrev())
   {
      const size_t lvl = t->GetLevel();

      if (lvl > my_level)
      {
         continue;
      }

      if (lvl < my_level)
      {
         if (  t->Is(E_Token::CT_SPAREN_OPEN)
            && t->GetParentType() == E_Token::CT_FOR)
         {
            return(count);
         }
         return(-1);
      }

      if (  t->Is(E_Token::CT_SEMICOLON)
         && t->GetParentType() == E_Token::CT_FOR)
      {
         ++count;
      }
   }

   return(-1);
} // get_for_section
