/**
 * @file token_is_within_trailing_return.cpp
 *
 * @author  Guy Maurel, 2015-2023
 * extract from space.cpp
 * @license GPL v2+
 */

#include "token_is_within_trailing_return.h"


using namespace std;
using namespace uncrustify;


bool token_is_within_trailing_return(Chunk *pc)
{
   // look back for '->' type is TRAILING_RET
   // until E_Token::CT_FPAREN_CLOSE
   //   or  E_Token::CT_FPAREN_OPEN is found
   Chunk *prev = pc;

   while (prev->IsNotNullChunk())
   {
      if (prev->Is(E_Token::CT_TRAILING_RET))
      {
         return(true);
      }
      else if (  prev->Is(E_Token::CT_FPAREN_CLOSE)
              || prev->Is(E_Token::CT_FPAREN_OPEN)
              || prev->Is(E_Token::CT_SEMICOLON))                     // Issue #4080
      {
         return(false);
      }
      else
      {
         prev = prev->GetPrev();
      }
   }
   return(false);
} // token_is_within_trailing_return
