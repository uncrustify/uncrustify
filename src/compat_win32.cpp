/**
 * @file compat_win32.cpp
 * Compatibility functions for win32
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#if defined (_WIN32) \
   && !defined (__CYGWIN__)

#include "windows_compat.h"
#include <string>
#include <cstdio>


bool unc_getenv(const char *name, std::string &str)
{
   DWORD len = GetEnvironmentVariableA(name, NULL, 0);
   char  *buf;

   if (len == 0)
   {
      if (GetLastError() == ERROR_ENVVAR_NOT_FOUND)
      {
         return(false);
      }
   }

   buf = (char *)malloc(len);
   if (buf)
   {
      len = GetEnvironmentVariableA(name, buf, len);
   }
   buf[len] = 0;

   str = buf;
   printf("%s: name=%s len=%d value=%s\n", __func__, name, (int)len, str.c_str());
   free(buf);

   return(true);
}


bool unc_homedir(std::string &home)
{
   if (unc_getenv("HOME", home))
   {
      return(true);
   }
   if (unc_getenv("USERPROFILE", home))
   {
      return(true);
   }
   std::string hd, hp;
   if (unc_getenv("HOMEDRIVE", hd) && unc_getenv("HOMEPATH", hp))
   {
      home = hd + hp;
      return(true);
   }
   return(false);
}


void convert_log_zu2lu(char *fmt)
{
   for (size_t i = 0; i < strlen(fmt); i++)
   {
      if (  (fmt[i] == '%')
         && (fmt[i + 1] == 'z')
         && (fmt[i + 2] == 'u'))
      {
         fmt[i + 1] = 'l';
      }
   }
}

#endif /* if defined(_WIN32) && !defined(__CYGWIN__) */
