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
#include "defines.h"
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


void add_define(const char *tag, const char *value)
{
   if (tag == nullptr || *tag == 0)
   {
      return;
   }
   value = value ? value : "";

   // Try to update an existing entry first
   defmap::iterator it = defines.find(tag);
   if (it != defines.end())
   {
      (*it).second = value;
      LOG_FMT(LDEFVAL, "%s: updated '%s' = '%s'\n", __func__, tag, value);
      return;
   }

   // Insert a new entry
   defines.insert(defmap::value_type(tag, value));
   LOG_FMT(LDEFVAL, "%s: added '%s' = '%s'\n", __func__, tag, value);
}


int load_define_file(const char *filename)
{
   FILE *pf = fopen(filename, "r");

   if (pf == nullptr)
   {
      LOG_FMT(LERR, "%s: fopen(%s) failed: %s (%d)\n",
              __func__, filename, strerror(errno), errno);
      cpd.error_count++;
      return(EX_IOERR);
   }

   char   buf[160];
   char   *args[3];
   size_t line_no = 0;
   while (fgets(buf, sizeof(buf), pf) != nullptr)
   {
      line_no++;

      // remove comments
      char *ptr;
      if ((ptr = strchr(buf, '#')) != nullptr)
      {
         *ptr = 0; // set string end where comment begins
      }

      int argc = Args::SplitLine(buf, args, ARRAY_SIZE(args) - 1);
      args[argc] = nullptr;

      if (argc > 0)
      {
         if (argc <= 2 && CharTable::IsKw1(*args[0]))
         {
            LOG_FMT(LDEFVAL, "%s: line %zu - %s\n", filename, line_no, args[0]);
            add_define(args[0], args[1]);
         }
         else
         {
            LOG_FMT(LWARN, "%s: line %zu invalid (starts with '%s')\n",
                    filename, line_no, args[0]);
            cpd.error_count++;
         }
      }
   }

   fclose(pf);
   return(EX_OK);
} // load_define_file


void print_defines(FILE *pfile)
{
   defmap::iterator it;

   for (it = defines.begin(); it != defines.end(); ++it)
   {
      fprintf(pfile, "define %*.s%s \"%s\"\n",
              MAX_OPTION_NAME_LEN - 6, " ", (*it).first.c_str(), (*it).second.c_str());
   }
}


void clear_defines(void)
{
   defines.clear();
}
