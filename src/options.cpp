/**
 * @file options.c
 * Parses the options from the config file.
 *
 * $Id$
 */

#define DEFINE_OPTION_NAME_TABLE

#include "uncrustify_types.h"
#include <cstring>
//#include <strings.h> /* strcasecmp() */
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cctype>


const options_name_tab *get_option_name(int uo)
{
   int idx;

   for (idx = 0; idx < ARRAY_SIZE(option_name_table); idx++)
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
                                          ARRAY_SIZE(option_name_table) - 1,
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
   bool                    btrue;
   int                     mult;

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

static char *skip_blanks(char *ptr)
{
   while ((*ptr == ' ') || (*ptr == '\t'))
   {
      ptr++;
   }
   return(ptr);
}

static char *skip_word(char *ptr)
{
   while (isalnum(*ptr) || (*ptr == '_') || (*ptr == '-'))
   {
      ptr++;
   }
   return(ptr);
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
   char *name;
   char *val;
   int  line_num = 0;
   int  id;

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

      /* Check for a blank line */
      ptr = skip_blanks(buffer);
      if ((*ptr == 0) || (*ptr == '\n') || (*ptr == '\r'))
      {
         continue;
      }
      name = ptr;
      ptr  = skip_word(name);
      /* zero everything until the '=' */
      while ((*ptr != '=') && (*ptr != 0))
      {
         *ptr = 0;
         ptr++;
      }
      if (*ptr != '=')
      {
         LOG_FMT(LWARN, "%s:%d - Didn't find a '=' for '%s'\n",
                 filename, line_num, name);
         /* TODO: default to some sane ON value - maybe 1? */
         continue;
      }
      *ptr = 0;
      val  = skip_blanks(ptr + 1);
      if (*val == 0)
      {
         LOG_FMT(LWARN, "%s:%d - Didn't find a value for '%s'\n",
                 filename, line_num, name);
         continue;
      }
      ptr  = skip_word(val);
      *ptr = 0;

      /* Set the value */
      if ((id = set_option_value(name, val)) < 0)
      {
         LOG_FMT(LWARN, "%s:%d - Unknown symbol '%s'\n",
                 filename, line_num, name);
      }
      else
      {
         //         fprintf(stderr, "%s:%d - Set '%s' to %d\n",
         //                 filename, line_num, name, cpd.settings[id]);
      }
   }
   return(0);
}

/**
 * Sets non-zero settings defaults
 *
 * TODO: select from various sets? - ie, K&R, GNU, Linux, Ben
 */
void set_option_defaults(void)
{
   cpd.settings[UO_input_tab_size].n   = 8;
   cpd.settings[UO_output_tab_size].n  = 8;
   cpd.settings[UO_indent_columns].n   = 8;
   cpd.settings[UO_indent_with_tabs].n = 1;
}

