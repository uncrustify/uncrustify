#include "token_enum.h"


E_Token getMatchingToken(E_Token token, bool openClose)
{
   if (  openClose
      && (  token == E_Token::CT_PAREN_OPEN
         || token == E_Token::CT_LPAREN_OPEN
         || token == E_Token::CT_SPAREN_OPEN
         || token == E_Token::CT_FPAREN_OPEN
         || token == E_Token::CT_RPAREN_OPEN
         || token == E_Token::CT_TPAREN_OPEN
         || token == E_Token::CT_BRACE_OPEN
         || token == E_Token::CT_VBRACE_OPEN
         || token == E_Token::CT_ANGLE_OPEN
         || token == E_Token::CT_SQUARE_OPEN
         || token == E_Token::CT_MACRO_OPEN))
   {
      return(static_cast<E_Token>(static_cast<unsigned short>(token) + 1));
   }

   if (  !openClose
      && (  token == E_Token::CT_PAREN_CLOSE
         || token == E_Token::CT_LPAREN_CLOSE
         || token == E_Token::CT_SPAREN_CLOSE
         || token == E_Token::CT_FPAREN_CLOSE
         || token == E_Token::CT_RPAREN_CLOSE
         || token == E_Token::CT_TPAREN_CLOSE
         || token == E_Token::CT_BRACE_CLOSE
         || token == E_Token::CT_VBRACE_CLOSE
         || token == E_Token::CT_ANGLE_CLOSE
         || token == E_Token::CT_SQUARE_CLOSE
         || token == E_Token::CT_MACRO_CLOSE))
   {
      return(static_cast<E_Token>(static_cast<unsigned short>(token) - 1));
   }
   return(E_Token::CT_NONE);
}
