/**
 * @file logmask.c
 *
 * Functions to convert between a string and a severity mask.
 */


#include "logmask.h"
#include <stdio.h>      /* snprintf() */
#include <stdlib.h>     /* strtoul() */
#include <ctype.h>      /* isdigit() */


/**
 * Convert a logmask into a string
 *
 * @param mask the mask to convert
 * @param buf  the buffer to hold the string
 * @param size the size of the buffer
 * @return     buf (pass through)
 */
char *logmask_to_str(const log_mask_t *mask, char *buf, int size)
{
   int  last_sev = -1;
   BOOL is_range = FALSE;
   int  sev;
   int  len = 0;

   if ((mask == NULL) || (buf == NULL) || (size <= 0))
   {
      if ((buf != NULL) && (size > 0))
      {
         *buf = 0;
      }
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
            is_range = TRUE;
         }
         last_sev = sev;
      }
      else
      {
         if (is_range)
         {
            buf[len - 1] = '-';  /* change last comma to a dash */
            len         += snprintf(&buf[len], size - len, "%d,", last_sev);
            is_range     = FALSE;
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
void logmask_from_string(const char *str, log_mask_t *mask)
{
   char *ptmp;
   BOOL was_dash   = FALSE;
   int  last_level = -1;
   int  level;
   int  idx;

   if ((str == NULL) || (mask == NULL))
   {
      return;
   }

   /* Start with a clean mask */
   logmask_set_all(mask, FALSE);

   while (*str != 0)
   {
      if (isspace(*str))
      {
         str++;
         continue;
      }

      if (isdigit(*str))
      {
         level = strtoul(str, &ptmp, 10);
         str   = ptmp;

         logmask_set_sev(mask, level, TRUE);
         if (was_dash)
         {
            for (idx = last_level + 1; idx < level; idx++)
            {
               logmask_set_sev(mask, idx, TRUE);
            }
            was_dash = FALSE;
         }

         last_level = level;
      }
      else if (*str == '-')
      {
         was_dash = TRUE;
         str++;
      }
      else  /* probably a comma */
      {
         last_level = -1;
         was_dash   = FALSE;
         str++;
      }
   }
}

