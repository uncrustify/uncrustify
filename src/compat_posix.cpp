/**
 * @file compat_posix.cpp
 * Compatibility functions for POSIX
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef WIN32

#include <cstdlib>
#include <string>

bool unc_getenv(const char *name, std::string& str)
{
   const char *val = getenv(name);

   if (val)
   {
      str = val;
      return true;
   }
   return false;
}

bool unc_homedir(std::string& home)
{
   if (unc_getenv("HOME", home))
   {
      return true;
   }
   return false;
}

#endif /* ifndef WIN32 */
