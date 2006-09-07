/**
 * @file backup.cpp
 * Make a backup of a source file
 * The current plans are to use two files.
 *
 *  - A '.backup' file that contains the original contents
 *  - A '.backup-md5' file that contains the MD5 over the last output
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
 * $Id$
 */

 /* TODO : */

