/**
 * @file options.c
 * Parses the options from the config file.
 *
 * $Id$
 */

#define DEFINE_OPTION_NAME_TABLE

#include "uncrustify_types.h"
#include "args.h"
#include "prototypes.h"
#include <cstring>
#ifdef HAVE_STRINGS_H
#include <strings.h> /* strcasecmp() */
#endif
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cctype>


const options_name_tab *get_option_name(int uo)
{
   int idx;

   for (idx = 0; idx < (int)ARRAY_SIZE(option_name_table); idx++)
   {
      if (option_name_table[idx].id == uo)
      {
         return(&option_name_table[idx]);
      }
   }
   return(NULL);
}

static int name_compare_func(const void *p1, const void *p2)
{
   const options_name_tab *e1 = (const options_name_tab *)p1;
   const options_name_tab *e2 = (const options_name_tab *)p2;

   return(strcmp(e1->name, e2->name));
}


/**
 * Search the sorted name table to find a match
 */
static options_name_tab *find_entry(const char *name)
{
   options_name_tab *entry;
   options_name_tab tmp;

   tmp.name = name;
   entry    = (options_name_tab *)bsearch(&tmp, &option_name_table[0],
                                          ARRAY_SIZE(option_name_table),
                                          sizeof(option_name_table[0]),
                                          name_compare_func);

   return(entry);
}

/**
 * Convert the value string to a number.
 */
static int convert_value(struct options_name_tab *entry, const char *val)
{
   struct options_name_tab *tmp;
   bool btrue;
   int  mult;

   if (entry->type == AT_LINE)
   {
      if (strcasecmp(val, "CRLF") == 0)
      {
         return(LE_CRLF);
      }
      if (strcasecmp(val, "LF") == 0)
      {
         return(LE_LF);
      }
      if (strcasecmp(val, "CR") == 0)
      {
         return(LE_CR);
      }
      if (strcasecmp(val, "AUTO") != 0)
      {
         LOG_FMT(LWARN, "Expected AUTO, LF, CRLF, or CR for %s, got %s\n",
                 entry->name, val);
      }
      return(LE_AUTO);
   }

   if (entry->type == AT_NUM)
   {
      if (isdigit(*val) ||
          (isdigit(val[1]) && ((*val == '-') || (*val == '+'))))
      {
         return(strtol(val, NULL, 0));
      }
      else
      {
         /* Try to see if it is a variable */
         mult = 1;
         if (*val == '-')
         {
            mult = -1;
            val++;
         }

         if (((tmp = find_entry(val)) != NULL) && (tmp->type == entry->type))
         {
            return(cpd.settings[tmp->id].n * mult);
         }
      }
      LOG_FMT(LWARN, "Expected a number for %s, got %s\n", entry->name, val);
      return(0);
   }

   if (entry->type == AT_BOOL)
   {
      if ((strcasecmp(val, "true") == 0) ||
          (strcasecmp(val, "t") == 0) ||
          (strcmp(val, "1") == 0))
      {
         return(1);
      }

      if ((strcasecmp(val, "false") == 0) ||
          (strcasecmp(val, "f") == 0) ||
          (strcmp(val, "0") == 0))
      {
         return(0);
      }

      btrue = true;
      if ((*val == '-') || (*val == '~'))
      {
         btrue = false;
         val++;
      }

      if (((tmp = find_entry(val)) != NULL) && (tmp->type == entry->type))
      {
         return(cpd.settings[tmp->id].b ? btrue : !btrue);
      }
      LOG_FMT(LWARN, "Expected 'True' or 'False' for %s, got %s\n", entry->name, val);
      return(0);
   }

   /* Must be AT_IARF */

   if ((strcasecmp(val, "add") == 0) || (strcasecmp(val, "a") == 0))
   {
      return(AV_ADD);
   }
   if ((strcasecmp(val, "remove") == 0) || (strcasecmp(val, "r") == 0))
   {
      return(AV_REMOVE);
   }
   if ((strcasecmp(val, "force") == 0) || (strcasecmp(val, "f") == 0))
   {
      return(AV_FORCE);
   }
   if ((strcasecmp(val, "ignore") == 0) || (strcasecmp(val, "i") == 0))
   {
      return(AV_IGNORE);
   }
   if (((tmp = find_entry(val)) != NULL) && (tmp->type == entry->type))
   {
      return(cpd.settings[tmp->id].a);
   }
   LOG_FMT(LWARN, "Expected 'Add', 'Remove', 'Force', or 'Ignore' for %s, got %s\n",
           entry->name, val);
   return(0);
}

int set_option_value(const char *name, const char *value)
{
   struct options_name_tab *entry;

   if ((entry = find_entry(name)) != NULL)
   {
      cpd.settings[entry->id].n = convert_value(entry, value);
      return(entry->id);
   }
   return(-1);
}

int load_option_file(const char *filename)
{
   FILE *pfile;
   char buffer[256];
   char *ptr;
   int  line_num = 0;
   int  id;
   char *args[32];
   int  argc;
   int  idx;

   pfile = fopen(filename, "r");
   if (pfile == NULL)
   {
      LOG_FMT(LERR, "failed to open config file %s: %s\n",
              filename, strerror(errno));
      return(-1);
   }

   /* Read in the file line by line */
   while (fgets(buffer, sizeof(buffer), pfile) != NULL)
   {
      line_num++;

      /* Chop off trailing comments */
      if ((ptr = strchr(buffer, '#')) != NULL)
      {
         *ptr = 0;
      }

      /* Blow away the '=' to make things simple */
      if ((ptr = strchr(buffer, '=')) != NULL)
      {
         *ptr = ' ';
      }

      /* Blow away all commas */
      ptr = buffer;
      while ((ptr = strchr(ptr, ',')) != NULL)
      {
         *ptr = ' ';
      }

      /* Split the line */
      argc = Args::SplitLine(buffer, args, ARRAY_SIZE(args) - 1);
      if (argc < 2)
      {
         if (argc > 0)
         {
            LOG_FMT(LWARN, "%s:Ignoring line %d: wrong number of arguments: %s...\n",
                    filename, line_num, buffer);
         }
         continue;
      }
      args[argc] = NULL;

      if (strcasecmp(args[0], "type") == 0)
      {
         for (idx = 1; idx < argc; idx++)
         {
            add_keyword(args[idx], CT_TYPE, LANG_ALL);
         }
      }
      else if (strcasecmp(args[0], "define") == 0)
      {
         add_define(args[1], args[2]);
      }
      else
      {
         /* must be a regular option = value */
         if ((id = set_option_value(args[0], args[1])) < 0)
         {
            LOG_FMT(LWARN, "%s:%d - Unknown symbol '%s'\n",
                    filename, line_num, args[0]);
         }
      }
   }

   fclose(pfile);
   return(0);
}

/**
 * Sets non-zero settings defaults
 *
 * TODO: select from various sets? - ie, K&R, GNU, Linux, Ben
 */
void set_option_defaults(void)
{
   cpd.settings[UO_newlines].le          = LE_AUTO;
   cpd.settings[UO_input_tab_size].n     = 8;
   cpd.settings[UO_output_tab_size].n    = 8;
   cpd.settings[UO_indent_columns].n     = 8;
   cpd.settings[UO_indent_with_tabs].n   = 1;
   cpd.settings[UO_indent_label].n       = 1;
   cpd.settings[UO_string_escape_char].n = '\\';
}
