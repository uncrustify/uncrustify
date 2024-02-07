/**
 * @file flag_decltype.cpp
 *
 * @license GPL v2+
 */

#include "chunk.h"


bool flag_cpp_decltype(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   if (pc->Is(CT_DECLTYPE))
   {
      auto paren_open = pc->GetNextNcNnl();

      if (paren_open->Is(CT_PAREN_OPEN))
      {
         // We would like to simply call Chunk::SkipToMatch(), but it finds
         // a match based on level, and the level is 0 for all chunks in some
         // cases, like the following example.
         //
         // template <typename T>
         // decltype(std::declval<T &>().put(foo), std::true_type())
         // has_something(Tag<2>);
         //
         // This means that IN_DECLTYPE is only set for tokens through the
         // closing parenthesis right before ".put" in the above example.
         //
         // So, we will manually look for the matching closing parenthesis.
         paren_open->SetFlagBits(PCF_IN_DECLTYPE);
         pc = paren_open->GetNextNcNnl();

         for (int level = 1; pc->IsNotNullChunk() && level > 0; pc = pc->GetNextNcNnl())
         {
            level += pc->Is(CT_PAREN_OPEN);
            level -= pc->Is(CT_PAREN_CLOSE);
            pc->SetFlagBits(PCF_IN_DECLTYPE);
         }

         return(pc->IsNotNullChunk());
      }
   }
   return(false);
} // mark_cpp_decltype
