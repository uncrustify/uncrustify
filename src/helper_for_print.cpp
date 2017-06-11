/**
 * @file helper_for_print.cpp
 *
 * Functions to help for printing with fprintf family
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */
#include "helper_for_print.h"
#include <cstdio>
#include <stdarg.h>
#include <stdlib.h>


char *make_message(const char *fmt, ...)
{
   size_t  n;
   size_t  size = 100;  // Guess we need no more than 100 bytes
   char    *p;
   char    *np;
   va_list ap;

   if ((p = (char *)malloc(size)) == NULL)
   {
      return(NULL);
   }

   while (1)
   {
      // Try to print in the allocated space

      va_start(ap, fmt);
      n = static_cast<size_t>(vsnprintf(p, size, fmt, ap));
      va_end(ap);

      // If that worked, return the string
      if (n < size)
      {
         return(p);
      }

      // Else try again with more space
      size = n + 1;         // Precisely what is needed

      if ((np = (char *)realloc(p, size)) == NULL)
      {
         free(p);
         return(NULL);
      }
      p = np;
   }
} // make_message
