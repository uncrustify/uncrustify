/**
 * @file logmask.cpp
 *
 * Functions to convert between a string and a severity mask.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "logmask.h"
#include <cstdio>      // snprintf()
#include <cstdlib>     // strtoul()
#include "unc_ctype.h"


char *logmask_to_str(const log_mask_t &mask, char *buf, int size)
{
   if (buf == nullptr || size <= 0)
   {
      return(buf);
   }

   int  last_sev = -1;
   bool is_range = false;
   int  len      = 0;

   for (int sev = 0; sev < 256; sev++)
   {
      if (logmask_test(mask, static_cast<log_sev_t>(sev)))
      {
         if (last_sev == -1)
         {
            len += snprintf(&buf[len], size - len, "%d,", sev);
         }
         else
         {
            is_range = true;
         }
         last_sev = sev;
      }
      else
      {
         if (is_range)
         {
            buf[len - 1] = '-';  // change last comma to a dash
            len         += snprintf(&buf[len], size - len, "%d,", last_sev);
            is_range     = false;
         }
         last_sev = -1;
      }
   }

   // handle a range that ends on the last bit
   if (is_range && last_sev != -1)
   {
      buf[len - 1] = '-';  // change last comma to a dash
      len         += snprintf(&buf[len], size - len, "%d", last_sev);
   }
   else
   {
      // Eat the last comma
      if (len > 0)
      {
         len--;
      }
   }

   buf[len] = 0;

   return(buf);
} // logmask_to_str


void logmask_from_string(const char *str, log_mask_t &mask)
{
   if (str == nullptr)
   {
      return;
   }

   logmask_set_all(mask, false);  // Start with a clean mask

   // If the first character is 'a' or 'A', set all severities
   if (unc_toupper(*str) == 'A')
   {
      logmask_set_all(mask, true);
      str++;
   }

   char *ptmp;
   bool was_dash   = false;
   int  last_level = -1;
   while (*str != 0)         // check string until termination character
   {
      if (unc_isspace(*str)) // ignore spaces and go on with next character
      {
         str++;
         continue;
      }

      if (unc_isdigit(*str))
      {
         int level = strtoul(str, &ptmp, 10);
         str = ptmp;

         logmask_set_sev(mask, static_cast<log_sev_t>(level), true);
         if (was_dash)
         {
            for (int idx = last_level + 1; idx < level; idx++)
            {
               logmask_set_sev(mask, static_cast<log_sev_t>(idx), true);
            }
            was_dash = false;
         }

         last_level = level;
      }
      else if (*str == '-') // a dash marks all bits until the next number
      {
         was_dash = true;
         str++;
      }
      else  // probably a comma
      {
         last_level = -1;
         was_dash   = false;
         str++;
      }
   }
} // logmask_from_string
