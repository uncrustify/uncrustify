/**
 * @file LogMask.d
 * Logger and LogMask classes
 *
 * $Id$
 */

module uncrustify.LogMask;

import std.cstream;
import std.string;
import std.stdio;
import std.format;
import std.date;


/* These are the 'standard' named log severities */
enum LogSev : byte
{
   Sys = 0,
   Error,
   Warn,
   Note,
   Info,
   Data,
}

class LogMask
{
   bool [] mask;

   this ()
   {
      mask.length = 256;
   }
   this (char [] str)
   {
      this();
      FromString(str);
   }

   void Reset(bool value)
   {
      mask[0 .. mask.length] = value;
   }

   bool IsSet(size_t idx)
   {
      return((idx < mask.length) ? mask[idx] : false);
   }

   void Set(size_t idx, bool val)
   {
      if (idx < mask.length)
      {
         mask[idx] = val;
      }
   }

   int opApply(int delegate(inout bool) dg)
   {
      int result = 0;
      for (int i = 0; !result && (i < mask.length); i++)
      {
         result = dg(mask[i]);
      }
      return(result);
   }

   bool opIndex(size_t idx)
   {
      return((idx < mask.length) ? mask[idx] : false);
   }

   char [] ToString()
   {
      char []  str;
      int      last_sev = -1;
      bool     is_range = false;

      for (int sev = 0; sev < mask.length; sev++)
      {
         if (mask[sev])
         {
            if (last_sev == -1)
            {
               str ~= std.string.toString(sev) ~ ",";
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
               str[str.length - 1] =  '-';
               str ~= std.string.toString(last_sev) ~ ",";
               is_range = false;
            }
            last_sev = -1;
         }
      }

      /* handle a range that ends on the last bit */
      if (is_range && (last_sev != -1))
      {
         str[str.length - 1] =  '-';
         str ~= std.string.toString(last_sev);
      }
      else
      {
         /* Eat the last comma */
         if (str.length > 0)
         {
            str.length = str.length - 1;
         }
      }
      return(str);
   }

   void FromString(char [] str)
   {
      /* Start with a clean mask */
      Reset(false);

      if ((str is null) || (str.length < 1))
      {
         return;
      }

      int  idx = 0;
      bool on_digit = false;
      int  level = 0;
      bool on_dash = false;
      int  last_level = -1;
      char ch = ' ';

      while (idx <= str.length)
      {
         ch = (idx < str.length) ? str[idx] : ' ';
         idx++;

         if (std.ctype.isdigit(ch))
         {
            if (!on_digit)
            {
               level = 0;
               on_digit = true;
            }
            level = (level * 10) + cast(int)(ch - '0');
         }
         else
         {
            /* if on a digit, it is done */
            if (on_digit)
            {
               if (level < mask.length)
               {
                  mask[level] = true;
               }
               if (level >= mask.length)
               {
                  level = mask.length - 1;
               }
               /* if we hit a dash, this is the end of it */
               if (on_dash)
               {
                  for (int sev = last_level + 1; sev < level; sev++)
                  {
                     mask[sev] = true;
                  }
                  on_dash = false;
               }
               last_level = level;
               on_digit = false;
            }

            if (ch == '-')
            {
               on_dash = true;
            }
            else
            {
               /* probably a comma or space */
               last_level = -1;
               on_dash = false;
            }
         }
      }
   }
}

