/**
 * @file backup.h
 * Handles backing up file data.
 *
 * It works like this:
 *
 * 1. Read in the file data
 *
 * 2. Call backup_copy_file() to create a backup of the input, if needed
 *
 * 3. Do the uncrustify magic and write the output file
 *
 * 4. Call backup_create_md5_file()
 *
 * This will let you run uncrustify multiple times over the same file without
 * losing the original file.  If you edit the file, then a new backup is made.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef BACKUP_H_INCLUDED
#define BACKUP_H_INCLUDED

#define UNC_BACKUP_SUFFIX        ".unc-backup~"
#define UNC_BACKUP_MD5_SUFFIX    ".unc-backup.md5~"


/**
 * @brief Check the backup-md5 file and copy the input file to a backup if needed.
 *
 * If there isn't a FILENAME+UNC_BACKUP_MD5_SUFFIX or the md5 over the data
 * doesn't match what is in FILENAME+UNC_BACKUP_MD5_SUFFIX, then write the
 * data to FILENAME+UNC_BACKUP_SUFFIX.
 *
 * Note that if this fails, we shouldn't overwrite to original file with the
 * output.
 *
 * @param filename   The file that was read (full path)
 * @param file_data  The file data
 * @param file_len   The file length
 *
 * @retval EX_OK     successfully created backup file
 * @retval EX_IOERR  could not create backup file
 */
int backup_copy_file(const char *filename, const std::vector<UINT8> &data);


/**
 * This calculates the MD5 over the file and writes the MD5 to
 * FILENAME+UNC_BACKUP_MD5_SUFFIX.*
 * This should be called after the file was written to disk.
 * We really don't care if it fails, as the MD5 just prevents us from backing
 * up a file that uncrustify created.
 *
 * This should be called after the file was written to disk.
 * It will be read back and an md5 will be calculated over it.
 *
 * @param filename  The file that was written (full path)
 */
void backup_create_md5_file(const char *filename);


#endif /* BACKUP_H_INCLUDED */
