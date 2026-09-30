/**
 * @file compat_posix.cpp
 * Compatibility functions for POSIX
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef WIN32

#include <limits.h>
#include <string>
#include <unistd.h>

#include "uncrustify_types.h"


bool unc_getcwd(std::string &cwd)
{
   char buf[PATH_MAX];

   if (getcwd(buf, sizeof(buf)) != nullptr)
   {
      cwd = buf;
      return(true);
   }
   return(false);
}


bool unc_getenv(const char *name, std::string &str)
{
   const char *val = getenv(name);

   if (val != nullptr)
   {
      str = val;
      return(true);
   }
   return(false);
}


bool unc_homedir(std::string &home)
{
   return(unc_getenv("HOME", home));
}


void convert_log_zu2lu(char *fmt)
{
   UNUSED(fmt);
   // nothing to do
}

#endif /* ifndef WIN32 */
