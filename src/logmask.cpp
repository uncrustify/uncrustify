/**
 * @file logmask.cpp
 *
 * Functions to convert between a string and a severity mask.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "logmask.h"
#include <cstdio>      /* snprintf() */
#include <cstdlib>     /* strtoul() */
#include "unc_ctype.h"


/**
 * Convert a logmask into a string
 *
 * @param mask the mask to convert
 * @param buf  the buffer to hold the string
 * @param size the size of the buffer
 * @return     buf (pass through)
 */
char *logmask_to_str(const log_mask_t& mask, char *buf, int size)
{
   int  last_sev = -1;
   bool is_range = false;
   int  sev;
   int  len = 0;

   if ((buf == NULL) || (size <= 0))
   {
      return(buf);
   }

   for (sev = 0; sev < 256; sev++)
   {
      if (logmask_test(mask, sev))
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
            buf[len - 1] = '-';  /* change last comma to a dash */
            len         += snprintf(&buf[len], size - len, "%d,", last_sev);
            is_range     = false;
         }
         last_sev = -1;
      }
   }

   /* handle a range that ends on the last bit */
   if (is_range && (last_sev != -1))
   {
      buf[len - 1] = '-';  /* change last comma to a dash */
      len         += snprintf(&buf[len], size - len, "%d", last_sev);
   }
   else
   {
      /* Eat the last comma */
      if (len > 0)
      {
         len--;
      }
   }

   buf[len] = 0;

   return(buf);
}


/**
 * Parses a string into a log severity
 *
 * @param str     The string to parse
 * @param mask    The mask to populate
 */
void logmask_from_string(const char *str, log_mask_t& mask)
{
   char *ptmp;
   bool was_dash   = false;
   int  last_level = -1;
   int  level;
   int  idx;

   if (str == NULL)
   {
      return;
   }

   /* Start with a clean mask */
   logmask_set_all(mask, false);

   /* If the first character is 'A', set all sevs */
   if (unc_toupper(*str) == 'A')
   {
      logmask_set_all(mask, true);
      str++;
   }

   while (*str != 0)
   {
      if (unc_isspace(*str))
      {
         str++;
         continue;
      }

      if (unc_isdigit(*str))
      {
         level = strtoul(str, &ptmp, 10);
         str   = ptmp;

         logmask_set_sev(mask, level, true);
         if (was_dash)
         {
            for (idx = last_level + 1; idx < level; idx++)
            {
               logmask_set_sev(mask, idx, true);
            }
            was_dash = false;
         }

         last_level = level;
      }
      else if (*str == '-')
      {
         was_dash = true;
         str++;
      }
      else  /* probably a comma */
      {
         last_level = -1;
         was_dash   = false;
         str++;
      }
   }
}
