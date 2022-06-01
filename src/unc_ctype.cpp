#include "options.h"


int unc_fix_ctype(int ch)
{
   if (  ch >= -1
      && ch <= 255)
   {
      return(ch);
   }
   return(0); // Issue #3025
}


int unc_isspace(int ch)
{
   if (  (ch == 12) // Issue #2386
      && uncrustify::options::use_form_feed_no_more_as_whitespace_character())
   {
      return(0);
   }
   else
   {
      return(isspace(unc_fix_ctype(ch)));
   }
}


int unc_isprint(int ch)
{
   return(isprint(unc_fix_ctype(ch)));
}


int unc_isalpha(int ch)
{
   return(isalpha(unc_fix_ctype(ch)));
}


int unc_isalnum(int ch)
{
   return(isalnum(unc_fix_ctype(ch)));
}


int unc_toupper(int ch)
{
   return(toupper(unc_fix_ctype(ch)));
}


int unc_tolower(int ch)
{
   return(tolower(unc_fix_ctype(ch)));
}


int unc_isxdigit(int ch)
{
   return(isxdigit(unc_fix_ctype(ch)));
}


int unc_isdigit(int ch)
{
   return(isdigit(unc_fix_ctype(ch)));
}


int unc_isupper(int ch)
{
   return(  isalpha(unc_fix_ctype(ch))
         && (unc_toupper(unc_fix_ctype(ch)) == ch));
}
