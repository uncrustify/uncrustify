/**
 * @file backup.cpp
 * Make a backup of a source file
 * The current plans are to use two files.
 *
 *  - A '.unc-backup~' file that contains the original contents
 *  - A '.unc-backup-md5~' file that contains the MD5 over the last output
 *    that uncrustify generated
 *
 * The logic goes like this:
 *  1. If there isn't a .backup-md5 or the md5 over the input file doesn't
 *     match what is in .backup-md5, then copy the source file to .backup.
 *
 *  2. Create the output file.
 *
 *  3. Calculate the md5 over the output file.
 *     Create the .backup-md5 file.
 *
 * This will let you run uncrustify multiple times over the same file without
 * losing the original file.  If you edit the file, then a new backup is made.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "prototypes.h"
#include "backup.h"
#include "md5.h"
#include "logger.h"
#include <cstdio>
#include <cerrno>
#include "unc_ctype.h"
#include <cstring>

/**
 * Check the backup-md5 file and copy the input file to a backup if needed.
 */
int backup_copy_file(const char *filename, const char *file_data, int file_len)
{
   char  newpath[1024];
   char  buffer[128];
   char  md5_str_in[33];
   char  md5_str[33];
   UINT8 dig[16];

   md5_str_in[0] = 0;

   MD5::Calc(file_data, file_len, dig);
   snprintf(md5_str, sizeof(md5_str),
            "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
            dig[0], dig[1], dig[2], dig[3],
            dig[4], dig[5], dig[6], dig[7],
            dig[8], dig[9], dig[10], dig[11],
            dig[12], dig[13], dig[14], dig[15]);

   /* Create the backup-md5 filename, open it and read the md5 */
   snprintf(newpath, sizeof(newpath), "%s%s", filename, UNC_BACKUP_MD5_SUFFIX);

   FILE *thefile = fopen(newpath, "rb");
   if (thefile != NULL)
   {
      if (fgets(buffer, sizeof(buffer), thefile) != NULL)
      {
         for (int i = 0; buffer[i] != 0; i++)
         {
            if (unc_isxdigit(buffer[i]))
            {
               md5_str_in[i] = unc_tolower(buffer[i]);
            }
            else
            {
               md5_str_in[i] = 0;
               break;
            }
         }
      }
      fclose(thefile);
   }

   /* if the MD5s match, then there is no need to back up the file */
   if (memcmp(md5_str, md5_str_in, 32) == 0)
   {
      LOG_FMT(LNOTE, "%s: MD5 match for %s\n", __func__, filename);
      return(SUCCESS);
   }

   LOG_FMT(LNOTE, "%s: MD5 mismatch - backing up %s\n", __func__, filename);

   /* Create the backup file */
   snprintf(newpath, sizeof(newpath), "%s%s", filename, UNC_BACKUP_SUFFIX);

   thefile = fopen(newpath, "wb");
   if (thefile != NULL)
   {
      int retval   = fwrite(file_data, file_len, 1, thefile);
      int my_errno = errno;

      fclose(thefile);

      if (retval == 1)
      {
         return(SUCCESS);
      }
      LOG_FMT(LERR, "fwrite(%s) failed: %s (%d)\n",
              newpath, strerror(my_errno), my_errno);
   }
   else
   {
      LOG_FMT(LERR, "fopen(%s) failed: %s (%d)\n",
              newpath, strerror(errno), errno);
   }
   return(FAILURE);
}


/**
 * This should be called after the file was written to disk.
 * It will be read back and an md5 will be calculated over it.
 */
void backup_create_md5_file(const char *filename)
{
   UINT8 dig[16];
   MD5   md5;
   FILE  *thefile;
   UINT8 buf[4096];
   INT32 len;
   char  newpath[1024];

   md5.Init();

   thefile = fopen(filename, "rb");
   if (thefile == NULL)
   {
      LOG_FMT(LERR, "%s: fopen(%s) failed: %s (%d)\n",
              __func__, filename, strerror(errno), errno);
      return;
   }

   while ((len = fread(buf, 1, sizeof(buf), thefile)) > 0)
   {
      md5.Update(buf, len);
   }

   fclose(thefile);
   md5.Final(dig);

   snprintf(newpath, sizeof(newpath), "%s%s", filename, UNC_BACKUP_MD5_SUFFIX);

   thefile = fopen(newpath, "wb");
   if (thefile != NULL)
   {
      fprintf(thefile,
              "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x  %s\n",
              dig[0], dig[1], dig[2], dig[3],
              dig[4], dig[5], dig[6], dig[7],
              dig[8], dig[9], dig[10], dig[11],
              dig[12], dig[13], dig[14], dig[15],
              path_basename(filename));

      fclose(thefile);
   }
}
