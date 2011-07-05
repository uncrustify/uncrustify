/**
 * @file defines.cpp
 * Manages the table of defines for some future time when these will be used to
 * help decide whether a block of #if'd code should be formatted.
 *
 * !! This isn't used right now. !!
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "uncrustify_types.h"
#include "char_table.h"
#include "args.h"
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <map>
#include "unc_ctype.h"
#include "chunk_list.h"
#include "prototypes.h"

using namespace std;

typedef map<string, string> defmap;
defmap defines;


/**
 * Adds an entry to the define list
 *
 * @param tag        The tag (string) must be zero terminated
 * @param value      NULL or the value of the define
 */
void add_define(const char *tag, const char *value)
{
   if ((tag == NULL) || (*tag == 0))
   {
      return;
   }
   value = value ? value : "";

   /* Try to update an existing entry first */
   defmap::iterator it = defines.find(tag);
   if (it != defines.end())
   {
      (*it).second = value;
      LOG_FMT(LDEFVAL, "%s: updated '%s' = '%s'\n", __func__, tag, value);
      return;
   }

   /* Insert a new entry */
   defines.insert(defmap::value_type(tag, value));
   LOG_FMT(LDEFVAL, "%s: added '%s' = '%s'\n", __func__, tag, value);
}


/**
 * Loads the defines from a file
 *
 * @param filename   The path to the file to load
 * @return           SUCCESS or FAILURE
 */
int load_define_file(const char *filename)
{
   FILE *pf;
   char buf[160];
   char *ptr;
   char *args[3];
   int  argc;
   int  line_no = 0;

   pf = fopen(filename, "r");
   if (pf == NULL)
   {
      LOG_FMT(LERR, "%s: fopen(%s) failed: %s (%d)\n",
              __func__, filename, strerror(errno), errno);
      cpd.error_count++;
      return(FAILURE);
   }

   while (fgets(buf, sizeof(buf), pf) != NULL)
   {
      line_no++;

      /* remove comments */
      if ((ptr = strchr(buf, '#')) != NULL)
      {
         *ptr = 0;
      }

      argc       = Args::SplitLine(buf, args, ARRAY_SIZE(args) - 1);
      args[argc] = 0;

      if (argc > 0)
      {
         if ((argc <= 2) && CharTable::IsKw1(*args[0]))
         {
            LOG_FMT(LDEFVAL, "%s: line %d - %s\n", filename, line_no, args[0]);
            add_define(args[0], args[1]);
         }
         else
         {
            LOG_FMT(LWARN, "%s: line %d invalid (starts with '%s')\n",
                    filename, line_no, args[0]);
            cpd.error_count++;
         }
      }
   }

   fclose(pf);
   return(SUCCESS);
}


void output_defines(FILE *pfile)
{
   if (defines.size() > 0)
   {
      fprintf(pfile, "-== User Defines ==-\n");
      defmap::iterator it;
      for (it = defines.begin(); it != defines.end(); ++it)
      {
         if ((*it).second.size() > 0)
         {
            fprintf(pfile, "%s = %s\n", (*it).first.c_str(), (*it).second.c_str());
         }
         else
         {
            fprintf(pfile, "%s\n", (*it).first.c_str());
         }
      }
   }
}


void print_defines(FILE *pfile)
{
   defmap::iterator it;
   for (it = defines.begin(); it != defines.end(); ++it)
   {
      fprintf(pfile, "define %*.s%s \"%s\"\n",
              cpd.max_option_name_len - 6, " ", (*it).first.c_str(), (*it).second.c_str());
   }
}


void clear_defines(void)
{
   defines.clear();
}
