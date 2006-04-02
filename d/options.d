/**
 * @file options.d
 * Parses the options from the config file.
 *
 * $Id$
 */

module uncrustify.options;

import uncrustify.settings;
import uncrustify.main;
import std.conv;
import std.ctype;

/**
 * Convert the value string to a number.
 */
int ConvertOptionValue(OptionEntry *entry, char [] val)
{
   OptionEntry *tmp;
   bool        btrue;
   int         mult;

   if (entry.type == ArgType.NUM)
   {
      if (std.ctype.isdigit(val[0]) ||
          (std.ctype.isdigit(val[1]) && ((val[0] == '-') || (val[0] == '+'))))
      {
         return(toInt(val));
      }
      else
      {
         /* Try to see if it is a variable */
         mult = 1;
         if (val[0] == '-')
         {
            mult = -1;
            val  = val[1..$];
         }

         if (((tmp = FindOptionEntry(val)) !is null) &&
             (tmp.type == entry.type))
         {
            return(unc.settings[tmp.id] * mult);
         }
      }
      Log(LogSev.Warn, "Expected a number for %.*s, got %.*s\n", entry.name, val);
      return(0);
   }

   if (entry.type == ArgType.BOOL)
   {
      if ((icmp(val, "true") == 0) ||
          (icmp(val, "t") == 0) ||
          (cmp(val, "1") == 0))
      {
         return(1);
      }

      if ((icmp(val, "false") == 0) ||
          (icmp(val, "f") == 0) ||
          (cmp(val, "0") == 0))
      {
         return(0);
      }

      btrue = true;
      if ((val[0] == '-') || (val[0] == '~'))
      {
         btrue = false;
         val   = val[1..$];
      }

      if (((tmp = FindOptionEntry(val)) !is null) && (tmp.type == entry.type))
      {
         return(unc.settings[tmp.id] ? btrue : !btrue);
      }
      Log(LogSev.Warn, "Expected 'True' or 'False' for %.*s, got %.*s\n",
              entry.name, val);
      return(0);
   }

   if ((icmp(val, "add") == 0) || (icmp(val, "a") == 0))
   {
      return(ArgVal.ADD);
   }
   if ((icmp(val, "remove") == 0) || (icmp(val, "r") == 0))
   {
      return(ArgVal.REMOVE);
   }
   if ((icmp(val, "force") == 0) || (icmp(val, "f") == 0))
   {
      return(ArgVal.FORCE);
   }
   if ((icmp(val, "ignore") == 0) || (icmp(val, "i") == 0))
   {
      return(ArgVal.IGNORE);
   }
   if (((tmp = FindOptionEntry(val)) !is null) && (tmp.type == entry.type))
   {
      return(unc.settings[tmp.id]);
   }
   Log(LogSev.Warn, "Expected 'Add', 'Remove', 'Force', or 'Ignore' for %.*s, got %.*s\n",
           entry.name, val);
   return(0);
}

char [] SkipBlanks(char [] str)
{
   int idx = 0;
   while ((str[idx] == ' ') || (str[idx] == '\t'))
   {
      idx++;
   }
   return(str[idx .. $]);
}

char [] ltrim(char [] str)
{
   int idx = 0;
   while ((idx < str.length) && isspace(str[idx]))
   {
      idx++;
   }
   return(str[idx .. $]);
}

char [] ChopWord(char [] theline)
{
   int idx = 0;
   while ((idx < theline.length) &&
          (std.ctype.isalnum(theline[idx]) ||
          (theline[idx] == '_') || (theline[idx] == '-')))
   {
      idx++;
   }
   return(theline[0 .. idx]);
}

int SetOptionValue(char [] name, char [] value)
{
   OptionEntry *entry;

   if ((entry = FindOptionEntry(name)) !is null)
   {
      unc.settings[entry.id] = ConvertOptionValue(entry, value);
      return(entry.id);
   }
   return(-1);
}

int LoadOptionFile(char [] filename)
{
   char [] theline;
   char [] buffer;
   char [] name;
   char [] val;
   int  line_num = 0;
   int  id;
   int  idx;

   //byte [] filedata = cast(byte []) std.file.read(source_file);


   /* Will throw an exception if no file found */
   std.stream.File file = new std.stream.File();
   file.open(filename, FileMode.In);

   /* Read in the file line by line */
   while (!file.eof())
   {
      theline = file.readLine(buffer);
      line_num++;

      /* Chop off trailing comments */
      if ((idx = std.string.find(theline, '#')) >= 0)
      {
         theline = theline[0 .. idx];
      }

      /* Remove leading and trailing whitespace */
      theline = ltrim(theline);
      if (theline.length == 0)
      {
         continue;
      }

      /* Pull off the word */
      name = ChopWord(theline);
      theline = theline[name.length .. $];

      /* skip blanks */
      theline = ltrim(theline);
      if ((theline.length == 0) || (theline[0] != '='))
      {
         Log(LogSev.Warn, "%s:%d - Didn't find a '=' for '%s'\n",
                 filename, line_num, name);
         /* TODO: default to some sane ON value - maybe 1? */
         continue;
      }

      /* Skip the '=' */
      theline = theline[1 .. $];

      /* skip blanks */
      theline = ltrim(theline);

      if (theline.length == 0)
      {
         Log(LogSev.Warn, "%s:%d - Didn't find a value for '%s'\n",
                 filename, line_num, name);
         continue;
      }

      val = ChopWord(theline);

      /* Set the value */
      if ((id = SetOptionValue(name, val)) < 0)
      {
         Log(LogSev.Warn, "%s:%d - Unknown symbol '%s'\n",
                 filename, line_num, name);
      }
      else
      {
         Log(LogSev.Note, "%s:%d - Set '%s' to %d\n",
                 filename, line_num, name, unc.settings[id]);
      }
   }
   return(0);
}

/**
 * Sets non-zero settings defaults
 *
 * TODO: select from various sets? - ie, K&R, GNU, Linux, Ben
 */
void SetOptionDefaults()
{
   unc.settings[Option.input_tab_size]   = 8;
   unc.settings[Option.output_tab_size]  = 8;
   unc.settings[Option.indent_columns]   = 8;
   unc.settings[Option.indent_with_tabs] = 1;
}

