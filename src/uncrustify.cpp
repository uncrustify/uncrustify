/**
 * @file uncrustify.cpp
 * This file takes an input C/C++/D/Java file and reformats it.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#define DEFINE_PCF_NAMES
#define DEFINE_CHAR_TABLE

#include "uncrustify_version.h"
#include "uncrustify_types.h"
#include "char_table.h"
#include "chunk_list.h"
#include "prototypes.h"
#include "token_names.h"
#include "args.h"
#include "logger.h"
#include "log_levels.h"
#include "md5.h"
#include "backup.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "unc_ctype.h"
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>  /* strcasecmp() */
#endif
#include <vector>
#include <deque>

/* Global data */
struct cp_data cpd;


static int language_flags_from_name(const char *tag);
static int language_flags_from_filename(const char *filename);
static const char *language_name_from_flags(int lang);
static bool read_stdin(file_mem& fm);
static void uncrustify_start(const deque<int>& data);
static void uncrustify_end();
static void uncrustify_file(const file_mem& fm, FILE *pfout,
                            const char *parsed_file);
static void do_source_file(const char *filename_in,
                           const char *filename_out,
                           const char *parsed_file,
                           bool no_backup, bool keep_mtime);
static void process_source_list(const char *source_list, const char *prefix,
                                const char *suffix, bool no_backup, bool keep_mtime);
static int load_header_files();

static const char *make_output_filename(char *buf, int buf_size,
                                        const char *filename,
                                        const char *prefix,
                                        const char *suffix);

static int load_mem_file(const char *filename, file_mem& fm);


/**
 * Replace the brain-dead and non-portable basename().
 * Returns a pointer to the character after the last '/'.
 * The returned value always points into path, unless path is NULL.
 *
 * Input            Returns
 * NULL          => ""
 * "/some/path/" => ""
 * "/some/path"  => "path"
 * "afile"       => "afile"
 *
 * @param path The path to look at
 * @return     Pointer to the character after the last path seperator
 */
const char *path_basename(const char *path)
{
   if (path == NULL)
   {
      return("");
   }

   const char *last_path = path;
   char       ch;

   while ((ch = *path) != 0)
   {
      path++;
      /* Check both slash types to support windows */
      if ((ch == '/') || (ch == '\\'))
      {
         last_path = path;
      }
   }
   return(last_path);
}


/**
 * Returns the length of the directory part of the filename.
 */
int path_dirname_len(const char *filename)
{
   if (filename == NULL)
   {
      return(0);
   }
   return((int)(path_basename(filename) - filename));
}


static void usage_exit(const char *msg, const char *argv0, int code)
{
   if (msg != NULL)
   {
      fprintf(stderr, "%s\n", msg);
   }
   if ((code != EXIT_SUCCESS) || (argv0 == NULL))
   {
      fprintf(stderr, "Try running with -h for usage information\n");
      exit(code);
   }
   fprintf(stdout,
           "Usage:\n"
           "%s [options] [files ...]\n"
           "\n"
           "If no input files are specified, the input is read from stdin\n"
           "If reading from stdin, you should specify the language using -l\n"
           "\n"
           "If -F is used or files are specified on the command line, the output filename is\n"
           "'prefix/filename' + suffix\n"
           "\n"
           "When reading from stdin or doing a single file via the '-f' option,\n"
           "the output is dumped to stdout, unless redirected with -o FILE.\n"
           "\n"
           "Errors are always dumped to stderr\n"
           "\n"
           "The '-f' and '-o' options may not be used with '-F', '--replace' or '--no-backup'.\n"
           "The '--prefix' and '--suffix' options may not be used with '--replace' or '--no-backup'.\n"
           "\n"
           "Basic Options:\n"
           " -c CFG       : use the config file CFG\n"
           " -f FILE      : process the single file FILE (output to stdout, use with -o)\n"
           " -o FILE      : Redirect stdout to FILE\n"
           " -F FILE      : read files to process from FILE, one filename per line (- is stdin)\n"
           " --check      : Do not output the new text, instead verify that nothing changes when\n"
           "                the file(s) are processed.\n"
           "                The status of every file is printed to stderr.\n"
           "                The exit code is EXIT_SUCCESS if there were no changes, EXIT_FAILURE otherwise.\n"
           " files        : files to process (can be combined with -F)\n"
           " --suffix SFX : Append SFX to the output filename. The default is '.uncrustify'\n"
           " --prefix PFX : Prepend PFX to the output filename path.\n"
           " --replace    : replace source files (creates a backup)\n"
           " --no-backup  : replace files, no backup. Useful if files are under source control\n"
#ifdef HAVE_UTIME_H
           " --mtime      : preserve mtime on replaced files\n"
#endif
           " -l           : language override: C, CPP, D, CS, JAVA, PAWN, OC, OC+, VALA\n"
           " -t           : load a file with types (usually not needed)\n"
           " -q           : quiet mode - no output on stderr (-L will override)\n"
           " --frag       : code fragment, assume the first line is indented correctly\n"
           "\n"
           "Config/Help Options:\n"
           " -h -? --help --usage     : print this message and exit\n"
           " --version                : print the version and exit\n"
           " --show-config            : print out option documentation and exit\n"
           " --update-config          : Output a new config file. Use with -o FILE\n"
           " --update-config-with-doc : Output a new config file. Use with -o FILE\n"
           " --universalindent        : Output a config file for Universal Indent GUI\n"
           " --detect                 : detects the config from a source file. Use with '-f FILE'\n"
           "                            Detection is fairly limited.\n"
           "\n"
           "Debug Options:\n"
           " -p FILE      : dump debug info to a file\n"
           " -L SEV       : Set the log severity (see log_levels.h)\n"
           " -s           : Show the log severity in the logs\n"
           " --decode     : decode remaining args (chunk flags) and exit\n"
           "\n"
           "Usage Examples\n"
           "cat foo.d | uncrustify -q -c my.cfg -l d\n"
           "uncrustify -c my.cfg -f foo.d\n"
           "uncrustify -c my.cfg -f foo.d -L0-2,20-23,51\n"
           "uncrustify -c my.cfg -f foo.d -o foo.d\n"
           "uncrustify -c my.cfg foo.d\n"
           "uncrustify -c my.cfg --replace foo.d\n"
           "uncrustify -c my.cfg --no-backup foo.d\n"
           "uncrustify -c my.cfg --prefix=out -F files.txt\n"
           "\n"
           "Note: Use comments containing ' *INDENT-OFF*' and ' *INDENT-ON*' to disable\n"
           "      processing of parts of the source file (these can be overridden with \n"
           "      enable_processing_cmt and disable_processing_cmt.\n"
           "\n"
           "There are currently %d options and minimal documentation.\n"
           "Try UniversalIndentGUI and good luck.\n"
           "\n"
           ,
           path_basename(argv0), UO_option_count);
   exit(code);
}


static void version_exit(void)
{
   printf("uncrustify %s\n", UNCRUSTIFY_VERSION);
   exit(0);
}


static void redir_stdout(const char *output_file)
{
   /* Reopen stdout */
   FILE *my_stdout = stdout;

   if (output_file != NULL)
   {
      my_stdout = freopen(output_file, "wb", stdout);
      if (my_stdout == NULL)
      {
         LOG_FMT(LERR, "Unable to open %s for write: %s (%d)\n",
                 output_file, strerror(errno), errno);
         usage_exit(NULL, NULL, 56);
      }
      LOG_FMT(LNOTE, "Redirecting output to %s\n", output_file);
   }
}


int main(int argc, char *argv[])
{
   string     cfg_file;
   const char *parsed_file = NULL;
   const char *source_file = NULL;
   const char *output_file = NULL;
   const char *source_list = NULL;
   log_mask_t mask;
   int        idx;
   const char *p_arg;

   /* If ran without options... check keyword sort and show the usage info */
   if (argc == 1)
   {
      keywords_are_sorted();
      usage_exit(NULL, argv[0], EXIT_SUCCESS);
   }

   /* Build options map */
   register_options();

   Args arg(argc, argv);

   if (arg.Present("--version") || arg.Present("-v"))
   {
      version_exit();
   }
   if (arg.Present("--help") || arg.Present("-h") ||
       arg.Present("--usage") || arg.Present("-?"))
   {
      usage_exit(NULL, argv[0], EXIT_SUCCESS);
   }

   if (arg.Present("--show-config"))
   {
      print_options(stdout);
      return EXIT_SUCCESS;
   }

   cpd.do_check = arg.Present("--check");

#ifdef WIN32
   /* tell windoze not to change what I write to stdout */
   (void)_setmode(_fileno(stdout), _O_BINARY);
#endif

   /* Init logging */
   log_init(cpd.do_check ? stdout : stderr);
   if (arg.Present("-q"))
   {
      logmask_from_string("", mask);
      log_set_mask(mask);
   }
   if (((p_arg = arg.Param("-L")) != NULL) ||
       ((p_arg = arg.Param("--log")) != NULL))
   {
      logmask_from_string(p_arg, mask);
      log_set_mask(mask);
   }
   cpd.frag = arg.Present("--frag");

   if (arg.Present("--decode"))
   {
      idx = 1;
      while ((p_arg = arg.Unused(idx)) != NULL)
      {
         log_pcf_flags(LSYS, strtoul(p_arg, NULL, 16));
      }
      return EXIT_SUCCESS;
   }

   /* Get the config file name */
   if (((p_arg = arg.Param("--config")) != NULL) ||
       ((p_arg = arg.Param("-c")) != NULL))
   {
      cfg_file = p_arg;
   }

   /* Try to find a config file at an alternate location */
   if (cfg_file.empty())
   {
      if (!unc_getenv("UNCRUSTIFY_CONFIG", cfg_file))
      {
         string home;

         if (unc_homedir(home))
         {
            struct stat tmp_stat;
            string      path;

            path = home + "/uncrustify.cfg";
            if (stat(path.c_str(), &tmp_stat) == 0)
            {
               cfg_file = path;
            }
            else
            {
               path = home + "/.uncrustify.cfg";
               if (stat(path.c_str(), &tmp_stat) == 0)
               {
                  cfg_file = path;
               }
            }
         }
      }
   }

   /* Get the parsed file name */
   if (((parsed_file = arg.Param("--parsed")) != NULL) ||
       ((parsed_file = arg.Param("-p")) != NULL))
   {
      LOG_FMT(LNOTE, "Will export parsed data to: %s\n", parsed_file);
   }

   /* Enable log sevs? */
   if (arg.Present("-s") || arg.Present("--show"))
   {
      log_show_sev(true);
   }

   /* Load the config file */
   set_option_defaults();

   /* Load type files */
   idx = 0;
   while ((p_arg = arg.Params("-t", idx)) != NULL)
   {
      load_keyword_file(p_arg);
   }

   /* add types */
   idx = 0;
   while ((p_arg = arg.Params("--type", idx)) != NULL)
   {
      add_keyword(p_arg, CT_TYPE);
   }

   /* Load define files */
   idx = 0;
   while ((p_arg = arg.Params("-d", idx)) != NULL)
   {
      load_define_file(p_arg);
   }

   /* add defines */
   idx = 0;
   while ((p_arg = arg.Params("--define", idx)) != NULL)
   {
      add_define(p_arg, NULL);
   }

   /* Check for a language override */
   if ((p_arg = arg.Param("-l")) != NULL)
   {
      cpd.lang_flags = language_flags_from_name(p_arg);
      if (cpd.lang_flags == 0)
      {
         LOG_FMT(LWARN, "Ignoring unknown language: %s\n", p_arg);
      }
      else
      {
         cpd.lang_forced = true;
      }
   }

   /* Get the source file name */
   if (((source_file = arg.Param("--file")) == NULL) &&
       ((source_file = arg.Param("-f")) == NULL))
   {
      // not using a single file, source_file is NULL
   }

   if (((source_list = arg.Param("--files")) == NULL) &&
       ((source_list = arg.Param("-F")) == NULL))
   {
      // not using a file list, source_list is NULL
   }

   const char *prefix = arg.Param("--prefix");
   const char *suffix = arg.Param("--suffix");

   bool no_backup        = arg.Present("--no-backup");
   bool replace          = arg.Present("--replace");
   bool keep_mtime       = arg.Present("--mtime");
   bool update_config    = arg.Present("--update-config");
   bool update_config_wd = arg.Present("--update-config-with-doc");
   bool detect           = arg.Present("--detect");

   /* Grab the output override */
   output_file = arg.Param("-o");

   LOG_FMT(LDATA, "config_file = %s\n", cfg_file.c_str());
   LOG_FMT(LDATA, "output_file = %s\n", (output_file != NULL) ? output_file : "null");
   LOG_FMT(LDATA, "source_file = %s\n", (source_file != NULL) ? source_file : "null");
   LOG_FMT(LDATA, "source_list = %s\n", (source_list != NULL) ? source_list : "null");
   LOG_FMT(LDATA, "prefix      = %s\n", (prefix != NULL) ? prefix : "null");
   LOG_FMT(LDATA, "suffix      = %s\n", (suffix != NULL) ? suffix : "null");
   LOG_FMT(LDATA, "replace     = %d\n", replace);
   LOG_FMT(LDATA, "no_backup   = %d\n", no_backup);
   LOG_FMT(LDATA, "detect      = %d\n", detect);
   LOG_FMT(LDATA, "check       = %d\n", cpd.do_check);

   if (cpd.do_check &&
       (output_file || replace || no_backup || keep_mtime || update_config ||
        update_config_wd || detect || prefix || suffix))
   {
      usage_exit("Cannot use --check with output options.", argv[0], 67);
   }

   if (!cpd.do_check)
   {
      if (replace || no_backup)
      {
         if ((prefix != NULL) || (suffix != NULL))
         {
            usage_exit("Cannot use --replace with --prefix or --suffix", argv[0], 66);
         }
         if ((source_file != NULL) || (output_file != NULL))
         {
            usage_exit("Cannot use --replace or --no-backup with -f or -o", argv[0], 66);
         }
      }
      else
      {
         if ((prefix == NULL) && (suffix == NULL))
         {
            suffix = ".uncrustify";
         }
      }
   }

   /* Try to load the config file, if available.
    * It is optional for "--universalindent" and "--detect", but required for
    * everything else.
    */
   if (!cfg_file.empty())
   {
      cpd.filename = cfg_file.c_str();
      if (load_option_file(cpd.filename) < 0)
      {
         usage_exit("Unable to load the config file", argv[0], 56);
      }
   }

   if (arg.Present("--universalindent"))
   {
      FILE *pfile = stdout;

      if (output_file != NULL)
      {
         pfile = fopen(output_file, "w");
         if (pfile == NULL)
         {
            fprintf(stderr, "Unable to open %s for write: %s (%d)\n",
                    output_file, strerror(errno), errno);
            return EXIT_FAILURE;
         }
      }

      print_universal_indent_cfg(pfile);
      fclose(pfile);

      return EXIT_SUCCESS;
   }

   if (detect)
   {
      file_mem fm;

      if ((source_file == NULL) || (source_list != NULL))
      {
         fprintf(stderr, "The --detect option requires a single input file\n");
         return EXIT_FAILURE;
      }

      /* Do some simple language detection based on the filename extension */
      if (!cpd.lang_forced || (cpd.lang_flags == 0))
      {
         cpd.lang_flags = language_flags_from_filename(source_file);
      }

      /* Try to read in the source file */
      if (load_mem_file(source_file, fm) < 0)
      {
         LOG_FMT(LERR, "Failed to load (%s)\n", source_file);
         cpd.error_count++;
         return EXIT_FAILURE;
      }

      uncrustify_start(fm.data);
      detect_options();
      uncrustify_end();

      redir_stdout(output_file);
      save_option_file(stdout, update_config_wd);
      return EXIT_SUCCESS;
   }

   if (update_config || update_config_wd)
   {
      /* TODO: complain if file-processing related options are present */
      redir_stdout(output_file);
      save_option_file(stdout, update_config_wd);
      return EXIT_SUCCESS;
   }

   /* Everything beyond this point requires a config file, so complain and
    * bail if we don't have one.
    */
   if (cfg_file.empty())
   {
      usage_exit("Specify the config file with '-c file' or set UNCRUSTIFY_CONFIG",
                 argv[0], 58);
   }

   /*
    *  Done parsing args
    */

   /* Check for unused args (ignore them) */
   idx   = 1;
   p_arg = arg.Unused(idx);

   /* Check args - for multifile options */
   if ((source_list != NULL) || (p_arg != NULL))
   {
      if (source_file != NULL)
      {
         usage_exit("Cannot specify both the single file option and a multi-file option.",
                    argv[0], 67);
      }

      if (output_file != NULL)
      {
         usage_exit("Cannot specify -o with a multi-file option.",
                    argv[0], 68);
      }
   }

   /* This relies on cpd.filename being the config file name */
   load_header_files();

   if (cpd.do_check)
   {
      cpd.bout = new deque<UINT8>();
   }

   if ((source_file == NULL) && (source_list == NULL) && (p_arg == NULL))
   {
      /* no input specified, so use stdin */
      if (cpd.lang_flags == 0)
      {
         cpd.lang_flags = LANG_C;
      }

      if (!cpd.do_check)
      {
         redir_stdout(output_file);
      }

      file_mem fm;
      if (!read_stdin(fm))
      {
         LOG_FMT(LERR, "Failed to read stdin\n");
         return(100);
      }

      cpd.filename = "stdin";

      /* Done reading from stdin */
      LOG_FMT(LSYS, "Parsing: %d bytes (%d chars) from stdin as language %s\n",
              (int)fm.raw.size(), (int)fm.data.size(),
              language_name_from_flags(cpd.lang_flags));

      uncrustify_file(fm, stdout, parsed_file);
   }
   else if (source_file != NULL)
   {
      /* Doing a single file */
      do_source_file(source_file, output_file, parsed_file, no_backup, keep_mtime);
   }
   else
   {
      /* Doing multiple files */
      if (prefix != NULL)
      {
         LOG_FMT(LSYS, "Output prefix: %s/\n", prefix);
      }
      if (suffix != NULL)
      {
         LOG_FMT(LSYS, "Output suffix: %s\n", suffix);
      }

      /* Do the files on the command line first */
      idx = 1;
      while ((p_arg = arg.Unused(idx)) != NULL)
      {
         char outbuf[1024];
         do_source_file(p_arg,
                        make_output_filename(outbuf, sizeof(outbuf), p_arg, prefix, suffix),
                        NULL, no_backup, keep_mtime);
      }

      if (source_list != NULL)
      {
         process_source_list(source_list, prefix, suffix, no_backup, keep_mtime);
      }
   }

   clear_keyword_file();
   clear_defines();

   if (cpd.do_check)
   {
      return cpd.check_fail_cnt ? EXIT_FAILURE : EXIT_SUCCESS;
   }

   return((cpd.error_count != 0) ? EXIT_FAILURE : EXIT_SUCCESS);
}


static void process_source_list(const char *source_list,
                                const char *prefix, const char *suffix,
                                bool no_backup, bool keep_mtime)
{
   int  from_stdin = strcmp(source_list, "-") == 0;
   FILE *p_file    = from_stdin ? stdin : fopen(source_list, "r");

   if (p_file == NULL)
   {
      LOG_FMT(LERR, "%s: fopen(%s) failed: %s (%d)\n",
              __func__, source_list, strerror(errno), errno);
      cpd.error_count++;
      return;
   }

   char linebuf[256];
   char *fname;
   int  line = 0;
   int  len;

   while (fgets(linebuf, sizeof(linebuf), p_file) != NULL)
   {
      line++;
      fname = linebuf;
      len   = strlen(fname);
      while ((len > 0) && unc_isspace(*fname))
      {
         fname++;
         len--;
      }
      while ((len > 0) && unc_isspace(fname[len - 1]))
      {
         len--;
      }
      fname[len] = 0;
      while (len-- > 0)
      {
         if (fname[len] == '\\')
         {
            fname[len] = '/';
         }
      }

      LOG_FMT(LFILELIST, "%3d] %s\n", line, fname);

      if (fname[0] != '#')
      {
         char outbuf[1024];
         do_source_file(fname,
                        make_output_filename(outbuf, sizeof(outbuf), fname, prefix, suffix),
                        NULL, no_backup, keep_mtime);
      }
   }

   if (!from_stdin)
   {
      fclose(p_file);
   }
}


static bool read_stdin(file_mem& fm)
{
   deque<UINT8> dq;
   char         buf[4096];
   int          len;
   int          idx;

   fm.raw.clear();
   fm.data.clear();
   fm.enc = ENC_ASCII;

   while (!feof(stdin))
   {
      len = fread(buf, 1, sizeof(buf), stdin);
      for (idx = 0; idx < len; idx++)
      {
         dq.push_back(buf[idx]);
      }
   }

   /* Copy the raw data from the deque to the vector */
   fm.raw.insert(fm.raw.end(), dq.begin(), dq.end());
   return(decode_unicode(fm.raw, fm.data, fm.enc, fm.bom));
}


static void make_folders(const string& filename)
{
   int  idx;
   int  last_idx = 0;
   char outname[4096];

   snprintf(outname, sizeof(outname), "%s", filename.c_str());

   for (idx = 0; outname[idx] != 0; idx++)
   {
      if ((outname[idx] == '/') || (outname[idx] == '\\'))
      {
         outname[idx] = PATH_SEP;
      }

      if ((idx > last_idx) && (outname[idx] == PATH_SEP))
      {
         outname[idx] = 0;

         if ((strcmp(&outname[last_idx], ".") != 0) &&
             (strcmp(&outname[last_idx], "..") != 0))
         {
            //fprintf(stderr, "%s: %s\n", __func__, outname);
            mkdir(outname, 0750);
         }
         outname[idx] = PATH_SEP;
      }

      if (outname[idx] == PATH_SEP)
      {
         last_idx = idx + 1;
      }
   }
}


/**
 * Loads a file into memory
 */
static int load_mem_file(const char *filename, file_mem& fm)
{
   int         retval = -1;
   struct stat my_stat;
   FILE        *p_file;

   fm.raw.clear();
   fm.data.clear();
   fm.enc = ENC_ASCII;

   /* Grab the stat info for the file */
   if (stat(filename, &my_stat) < 0)
   {
      return(-1);
   }

#ifdef HAVE_UTIME_H
   /* Save off mtime */
   fm.utb.modtime = my_stat.st_mtime;
#endif

   /* Try to read in the file */
   p_file = fopen(filename, "rb");
   if (p_file == NULL)
   {
      return(-1);
   }

   fm.raw.resize(my_stat.st_size);
   if (my_stat.st_size == 0)
   {
      /* Empty file */
      retval = 0;
      fm.bom = false;
      fm.enc = ENC_ASCII;
      fm.data.clear();
   }
   else
   {
      /* read the raw data */
      if (fread(&fm.raw[0], fm.raw.size(), 1, p_file) != 1)
      {
         LOG_FMT(LERR, "%s: fread(%s) failed: %s (%d)\n",
                 __func__, filename, strerror(errno), errno);
         cpd.error_count++;
      }
      else if (!decode_unicode(fm.raw, fm.data, fm.enc, fm.bom))
      {
         LOG_FMT(LERR, "%s: failed to decode the file '%s'\n", __func__, filename);
      }
      else
      {
         LOG_FMT(LNOTE, "%s: '%s' encoding looks like %s (%d)\n", __func__, filename,
                 fm.enc == ENC_ASCII ? "ASCII" :
                 fm.enc == ENC_BYTE ? "BYTES" :
                 fm.enc == ENC_UTF16_LE ? "UTF-16-LE" :
                 fm.enc == ENC_UTF16_BE ? "UTF-16-BE" : "Error",
                 fm.enc);
         retval = 0;
      }
   }
   fclose(p_file);
   return(retval);
}


/**
 * Try to load the file from the config folder first and then by name
 */
static int load_mem_file_config(const char *filename, file_mem& fm)
{
   int  retval;
   char buf[1024];

   snprintf(buf, sizeof(buf), "%.*s%s",
            path_dirname_len(cpd.filename), cpd.filename, filename);

   retval = load_mem_file(buf, fm);
   if (retval < 0)
   {
      retval = load_mem_file(filename, fm);
      if (retval < 0)
      {
         LOG_FMT(LERR, "Failed to load (%s) or (%s)\n", buf, filename);
         cpd.error_count++;
      }
   }
   return(retval);
}


static int load_header_files()
{
   int retval = 0;

   if ((cpd.settings[UO_cmt_insert_file_header].str != NULL) &&
       (cpd.settings[UO_cmt_insert_file_header].str[0] != 0))
   {
      retval |= load_mem_file_config(cpd.settings[UO_cmt_insert_file_header].str,
                                     cpd.file_hdr);
   }
   if ((cpd.settings[UO_cmt_insert_file_footer].str != NULL) &&
       (cpd.settings[UO_cmt_insert_file_footer].str[0] != 0))
   {
      retval |= load_mem_file_config(cpd.settings[UO_cmt_insert_file_footer].str,
                                     cpd.file_ftr);
   }
   if ((cpd.settings[UO_cmt_insert_func_header].str != NULL) &&
       (cpd.settings[UO_cmt_insert_func_header].str[0] != 0))
   {
      retval |= load_mem_file_config(cpd.settings[UO_cmt_insert_func_header].str,
                                     cpd.func_hdr);
   }
   if ((cpd.settings[UO_cmt_insert_class_header].str != NULL) &&
       (cpd.settings[UO_cmt_insert_class_header].str[0] != 0))
   {
      retval |= load_mem_file_config(cpd.settings[UO_cmt_insert_class_header].str,
                                     cpd.class_hdr);
   }
   if ((cpd.settings[UO_cmt_insert_oc_msg_header].str != NULL) &&
       (cpd.settings[UO_cmt_insert_oc_msg_header].str[0] != 0))
   {
      retval |= load_mem_file_config(cpd.settings[UO_cmt_insert_oc_msg_header].str,
                                     cpd.oc_msg_hdr);
   }
   return(retval);
}


static const char *make_output_filename(char *buf, int buf_size,
                                        const char *filename,
                                        const char *prefix,
                                        const char *suffix)
{
   int len = 0;

   if (prefix != NULL)
   {
      len = snprintf(buf, buf_size, "%s/", prefix);
   }

   snprintf(&buf[len], buf_size - len, "%s%s", filename,
            (suffix != NULL) ? suffix : "");

   return(buf);
}


/**
 * Reinvent the wheel with a file comparision function...
 */
static bool file_content_matches(const string& filename1, const string& filename2)
{
   struct stat st1, st2;
   int         fd1, fd2;
   UINT8       buf1[1024], buf2[1024];
   int         len1 = 0, len2 = 0;
   int         minlen;

   /* Check the sizes first */
   if ((stat(filename1.c_str(), &st1) != 0) ||
       (stat(filename2.c_str(), &st2) != 0) ||
       (st1.st_size != st2.st_size))
   {
      return(false);
   }

   if ((fd1 = open(filename1.c_str(), O_RDONLY)) < 0)
   {
      return(false);
   }
   if ((fd2 = open(filename2.c_str(), O_RDONLY)) < 0)
   {
      close(fd1);
      return(false);
   }

   while ((len1 >= 0) && (len2 >= 0))
   {
      if (len1 == 0)
      {
         len1 = read(fd1, buf1, sizeof(buf1));
      }
      if (len2 == 0)
      {
         len2 = read(fd2, buf2, sizeof(buf2));
      }
      if ((len1 <= 0) || (len2 <= 0))
      {
         break;
      }
      minlen = (len1 < len2) ? len1 : len2;
      if (memcmp(buf1, buf2, minlen) != 0)
      {
         break;
      }
      len1 -= minlen;
      len2 -= minlen;
   }

   close(fd1);
   close(fd2);

   return((len1 == 0) && (len2 == 0));
}


const char *fix_filename(const char *filename)
{
   char *tmp_file;

   /* Create 'outfile.uncrustify' */
   tmp_file = new char[strlen(filename) + 16 + 1]; /* + 1 for '\0' */
   if (tmp_file != NULL)
   {
      sprintf(tmp_file, "%s.uncrustify", filename);
   }
   return(tmp_file);
}


/**
 * Does a source file.
 *
 * @param filename_in  the file to read
 * @param filename_out NULL (stdout) or the file to write
 * @param parsed_file  NULL or the filename for the parsed debug info
 * @param no_backup    don't create a backup, if filename_out == filename_in
 * @param keep_mtime   don't change the mtime (dangerous)
 */
static void do_source_file(const char *filename_in,
                           const char *filename_out,
                           const char *parsed_file,
                           bool       no_backup,
                           bool       keep_mtime)
{
   FILE     *pfout      = NULL;
   bool     did_open    = false;
   bool     need_backup = false;
   file_mem fm;
   string   filename_tmp;

   /* Do some simple language detection based on the filename extension */
   if (!cpd.lang_forced || (cpd.lang_flags == 0))
   {
      cpd.lang_flags = language_flags_from_filename(filename_in);
   }

   /* Try to read in the source file */
   if (load_mem_file(filename_in, fm) < 0)
   {
      LOG_FMT(LERR, "Failed to load (%s)\n", filename_in);
      cpd.error_count++;
      return;
   }

   LOG_FMT(LSYS, "Parsing: %s as language %s\n",
           filename_in, language_name_from_flags(cpd.lang_flags));

   if (!cpd.do_check)
   {
      if (filename_out == NULL)
      {
         pfout = stdout;
      }
      else
      {
         /* If the out file is the same as the in file, then use a temp file */
         filename_tmp = filename_out;
         if (strcmp(filename_in, filename_out) == 0)
         {
            /* Create 'outfile.uncrustify' */
            filename_tmp = fix_filename(filename_out);

            if (!no_backup)
            {
               if (backup_copy_file(filename_in, fm.raw) != SUCCESS)
               {
                  LOG_FMT(LERR, "%s: Failed to create backup file for %s\n",
                          __func__, filename_in);
                  cpd.error_count++;
                  return;
               }
               need_backup = true;
            }
         }
         make_folders(filename_tmp);

         pfout = fopen(filename_tmp.c_str(), "wb");
         if (pfout == NULL)
         {
            LOG_FMT(LERR, "%s: Unable to create %s: %s (%d)\n",
                    __func__, filename_tmp.c_str(), strerror(errno), errno);
            cpd.error_count++;
            return;
         }
         did_open = true;
         //LOG_FMT(LSYS, "Output file %s\n", filename_out);
      }
   }

   cpd.filename = filename_in;
   uncrustify_file(fm, pfout, parsed_file);

   if (did_open)
   {
      fclose(pfout);

      if (need_backup)
      {
         backup_create_md5_file(filename_in);
      }

      if (filename_tmp != filename_out)
      {
         /* We need to compare and then do a rename */
         if (file_content_matches(filename_tmp, filename_out))
         {
            /* No change - remove tmp file */
            (void)unlink(filename_tmp.c_str());
         }
         else
         {
#ifdef WIN32

            /* windows can't rename a file if the target exists, so delete it
             * first. This may cause data loss if the tmp file gets deleted
             * or can't be renamed.
             */
            (void)unlink(filename_out);
#endif
            /* Change - rename filename_tmp to filename_out */
            if (rename(filename_tmp.c_str(), filename_out) != 0)
            {
               LOG_FMT(LERR, "%s: Unable to rename '%s' to '%s'\n",
                       __func__, filename_tmp.c_str(), filename_out);
               cpd.error_count++;
            }
         }
      }

#ifdef HAVE_UTIME_H
      if (keep_mtime)
      {
         /* update mtime -- don't care if it fails */
         fm.utb.actime = time(NULL);
         (void)utime(filename_in, &fm.utb);
      }
#endif
   }
}


static void add_file_header()
{
   if (!chunk_is_comment(chunk_get_head()))
   {
      /*TODO: detect the typical #ifndef FOO / #define FOO sequence */
      tokenize(cpd.file_hdr.data, chunk_get_head());
   }
}


static void add_file_footer()
{
   chunk_t *pc = chunk_get_tail();

   /* Back up if the file ends with a newline */
   if ((pc != NULL) && chunk_is_newline(pc))
   {
      pc = chunk_get_prev(pc);
   }
   if ((pc != NULL) &&
       (!chunk_is_comment(pc) || !chunk_is_newline(chunk_get_prev(pc))))
   {
      pc = chunk_get_tail();
      if (!chunk_is_newline(pc))
      {
         LOG_FMT(LSYS, "Adding a newline at the end of the file\n");
         newline_add_after(pc);
      }
      tokenize(cpd.file_ftr.data, NULL);
   }
}


static void add_func_header(c_token_t type, file_mem& fm)
{
   chunk_t *pc;
   chunk_t *ref;
   chunk_t *tmp;
   bool    do_insert;

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next_ncnlnp(pc))
   {
      if (pc->type != type)
      {
         continue;
      }

      do_insert = false;

      /* On a function proto or def. Back up to a close brace or semicolon on
       * the same level
       */
      ref = pc;
      while ((ref = chunk_get_prev(ref)) != NULL)
      {
         /* Bail if we change level or find an access specifier colon */
         if ((ref->level != pc->level) || (ref->type == CT_PRIVATE_COLON))
         {
            do_insert = true;
            break;
         }

         /* If we hit an angle close, back up to the angle open */
         if (ref->type == CT_ANGLE_CLOSE)
         {
            ref = chunk_get_prev_type(ref, CT_ANGLE_OPEN, ref->level, CNAV_PREPROC);
            continue;
         }

         /* Bail if we hit a preprocessor and cmt_insert_before_preproc is false */
         if (ref->flags & PCF_IN_PREPROC)
         {
            tmp = chunk_get_prev_type(ref, CT_PREPROC, ref->level);
            if ((tmp != NULL) && (tmp->parent_type == CT_PP_IF))
            {
               tmp = chunk_get_prev_nnl(tmp);
               if (chunk_is_comment(tmp) &&
                   !cpd.settings[UO_cmt_insert_before_preproc].b)
               {
                  break;
               }
            }
         }

         /* Ignore 'right' comments */
         if (chunk_is_comment(ref) && chunk_is_newline(chunk_get_prev(ref)))
         {
            break;
         }

         if ((ref->level == pc->level) &&
             ((ref->flags & PCF_IN_PREPROC) ||
              (ref->type == CT_SEMICOLON) ||
              (ref->type == CT_BRACE_CLOSE)))
         {
            do_insert = true;
            break;
         }
      }
      if (do_insert)
      {
         /* Insert between after and ref */
         chunk_t *after = chunk_get_next_ncnl(ref);
         tokenize(fm.data, after);
         for (tmp = chunk_get_next(ref); tmp != after; tmp = chunk_get_next(tmp))
         {
            tmp->level = after->level;
         }
      }
   }
}


static void add_msg_header(c_token_t type, file_mem& fm)
{
   chunk_t *pc;
   chunk_t *ref;
   chunk_t *tmp;
   bool    do_insert;

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next_ncnlnp(pc))
   {
      if (pc->type != type)
      {
         continue;
      }

      do_insert = false;

      /* On a function proto or def. Back up to a close brace or semicolon on
       * the same level
       */
      ref = pc;
      while ((ref = chunk_get_prev(ref)) != NULL)
      {
         /* ignore the CT_TYPE token that is the result type */
         if ((ref->level != pc->level) &&
             ((ref->type == CT_TYPE) ||
              (ref->type == CT_PTR_TYPE)))
         {
            continue;
         }

         if ((ref->level != pc->level) && (ref->type == CT_OC_CATEGORY))
         {
            ref = chunk_get_next_ncnl(ref);
            if (ref)
            {
               do_insert = true;
            }
            break;
         }

         /* Bail if we change level or find an access specifier colon */
         if ((ref->level != pc->level) || (ref->type == CT_PRIVATE_COLON))
         {
            do_insert = true;
            break;
         }

         /* If we hit an angle close, back up to the angle open */
         if (ref->type == CT_ANGLE_CLOSE)
         {
            ref = chunk_get_prev_type(ref, CT_ANGLE_OPEN, ref->level, CNAV_PREPROC);
            continue;
         }

         /* Bail if we hit a preprocessor and cmt_insert_before_preproc is false */
         if (ref->flags & PCF_IN_PREPROC)
         {
            tmp = chunk_get_prev_type(ref, CT_PREPROC, ref->level);
            if ((tmp != NULL) && (tmp->parent_type == CT_PP_IF))
            {
               tmp = chunk_get_prev_nnl(tmp);
               if (chunk_is_comment(tmp) &&
                   !cpd.settings[UO_cmt_insert_before_preproc].b)
               {
                  break;
               }
            }
         }

         /* Ignore 'right' comments */
         if (chunk_is_comment(ref) && chunk_is_newline(chunk_get_prev(ref)))
         {
            break;
         }

         if ((ref->level == pc->level) &&
             ((ref->flags & PCF_IN_PREPROC) ||
              (ref->type == CT_SEMICOLON) ||
              (ref->type == CT_BRACE_CLOSE) ||
              (ref->type == CT_OC_CLASS)))
         {
            do_insert = true;
            break;
         }
      }

      if (do_insert)
      {
         /* Insert between after and ref */
         chunk_t *after = chunk_get_next_ncnl(ref);
         tokenize(fm.data, after);
         for (tmp = chunk_get_next(ref); tmp != after; tmp = chunk_get_next(tmp))
         {
            tmp->level = after->level;
         }
      }
   }
}


static void uncrustify_start(const deque<int>& data)
{
   /**
    * Parse the text into chunks
    */
   tokenize(data, NULL);

   /* Get the column for the fragment indent */
   if (cpd.frag)
   {
      chunk_t *pc = chunk_get_head();

      cpd.frag_cols = (pc != NULL) ? pc->orig_col : 0;
   }

   /* Add the file header */
   if (cpd.file_hdr.data.size() > 0)
   {
      add_file_header();
   }

   /* Add the file footer */
   if (cpd.file_ftr.data.size() > 0)
   {
      add_file_footer();
   }

   /**
    * Change certain token types based on simple sequence.
    * Example: change '[' + ']' to '[]'
    * Note that level info is not yet available, so it is OK to do all
    * processing that doesn't need to know level info. (that's very little!)
    */
   tokenize_cleanup();

   /**
    * Detect the brace and paren levels and insert virtual braces.
    * This handles all that nasty preprocessor stuff
    */
   brace_cleanup();

   /**
    * At this point, the level information is available and accurate.
    */

   if ((cpd.lang_flags & LANG_PAWN) != 0)
   {
      pawn_prescan();
   }

   /**
    * Re-type chunks, combine chunks
    */
   fix_symbols();

   mark_comments();

   /**
    * Look at all colons ':' and mark labels, :? sequences, etc.
    */
   combine_labels();
}


static void uncrustify_file(const file_mem& fm, FILE *pfout,
                            const char *parsed_file)
{
   const deque<int>& data = fm.data;

   /* Save off the encoding and whether a BOM is required */
   cpd.bom = fm.bom;
   cpd.enc = fm.enc;
   if (cpd.settings[UO_utf8_force].b ||
       ((cpd.enc == ENC_BYTE) && cpd.settings[UO_utf8_byte].b))
   {
      cpd.enc = ENC_UTF8;
   }
   argval_t av;
   switch (cpd.enc)
   {
   case ENC_UTF8:
      av = cpd.settings[UO_utf8_bom].a;
      break;

   case ENC_UTF16_LE:
   case ENC_UTF16_BE:
      av = AV_FORCE;
      break;

   default:
      av = AV_IGNORE;
      break;
   }
   if (av == AV_REMOVE)
   {
      cpd.bom = false;
   }
   else if (av != AV_IGNORE)
   {
      cpd.bom = true;
   }

   /* Check for embedded 0's (represents a decoding failure or corrupt file) */
   for (int idx = 0; idx < (int)data.size() - 1; idx++)
   {
      if (data[idx] == 0)
      {
         LOG_FMT(LERR, "An embedded 0 was found in '%s'.\n", cpd.filename);
         LOG_FMT(LERR, "The file may be encoded in an unsupported Unicode format.\n");
         LOG_FMT(LERR, "Aborting.\n");
         cpd.error_count++;
         return;
      }
   }

   uncrustify_start(data);

   /**
    * Done with detection. Do the rest only if the file will go somewhere.
    * The detection code needs as few changes as possible.
    */
   {
      /**
       * Add comments before function defs and classes
       */
      if (cpd.func_hdr.data.size() > 0)
      {
         add_func_header(CT_FUNC_DEF, cpd.func_hdr);
      }
      if (cpd.class_hdr.data.size() > 0)
      {
         add_func_header(CT_CLASS, cpd.class_hdr);
      }
      if (cpd.oc_msg_hdr.data.size() > 0)
      {
         add_msg_header(CT_OC_MSG_DECL, cpd.oc_msg_hdr);
      }

      /**
       * Change virtual braces into real braces...
       */
      do_braces();

      /* Scrub extra semicolons */
      if (cpd.settings[UO_mod_remove_extra_semicolon].b)
      {
         remove_extra_semicolons();
      }

      /* Remove unnecessary returns */
      if (cpd.settings[UO_mod_remove_empty_return].b)
      {
         remove_extra_returns();
          
      }
      
       /* Remove brace  only has single return statement */
       if (cpd.settings[UO_mod_remove_brace_only_has_return].b)
       {
           remove_brace_only_has_return();
       }

      /**
       * Add parens
       */
      do_parens();

      /**
       * Modify line breaks as needed
       */
      bool first = true;
      int  old_changes;

      if (cpd.settings[UO_nl_remove_extra_newlines].n == 2)
      {
         newlines_remove_newlines();
      }
      cpd.pass_count = 3;
      do
      {
         old_changes = cpd.changes;

         LOG_FMT(LNEWLINE, "Newline loop start: %d\n", cpd.changes);

         annotations_newlines();
         newlines_cleanup_dup();
         newlines_cleanup_braces(first);
         if (cpd.settings[UO_nl_after_multiline_comment].b)
         {
            newline_after_multiline_comment();
         }
         if (cpd.settings[UO_nl_after_label_colon].b)
         {
            newline_after_label_colon();
         }
         newlines_insert_blank_lines();
         if (cpd.settings[UO_pos_bool].tp != TP_IGNORE)
         {
            newlines_chunk_pos(CT_BOOL, cpd.settings[UO_pos_bool].tp);
         }
         if (cpd.settings[UO_pos_compare].tp != TP_IGNORE)
         {
            newlines_chunk_pos(CT_COMPARE, cpd.settings[UO_pos_compare].tp);
         }
         if (cpd.settings[UO_pos_conditional].tp != TP_IGNORE)
         {
            newlines_chunk_pos(CT_COND_COLON, cpd.settings[UO_pos_conditional].tp);
            newlines_chunk_pos(CT_QUESTION, cpd.settings[UO_pos_conditional].tp);
         }
         if (cpd.settings[UO_pos_comma].tp != TP_IGNORE)
         {
            newlines_chunk_pos(CT_COMMA, cpd.settings[UO_pos_comma].tp);
         }
         if (cpd.settings[UO_pos_assign].tp != TP_IGNORE)
         {
            newlines_chunk_pos(CT_ASSIGN, cpd.settings[UO_pos_assign].tp);
         }
         if (cpd.settings[UO_pos_arith].tp != TP_IGNORE)
         {
            newlines_chunk_pos(CT_ARITH, cpd.settings[UO_pos_arith].tp);
            newlines_chunk_pos(CT_CARET, cpd.settings[UO_pos_arith].tp);
         }
         newlines_class_colon_pos(CT_CLASS_COLON);
         newlines_class_colon_pos(CT_CONSTR_COLON);
         if (cpd.settings[UO_nl_squeeze_ifdef].b)
         {
            newlines_squeeze_ifdef();
         }
         do_blank_lines();
         newlines_eat_start_end();
         newlines_eat_extra_blank_lines();
         newlines_cleanup_dup();
         first = false;
      } while ((old_changes != cpd.changes) && (cpd.pass_count-- > 0));

      mark_comments();

      /**
       * Add balanced spaces around nested params
       */
      if (cpd.settings[UO_sp_balance_nested_parens].b)
      {
         space_text_balance_nested_parens();
      }

      /* Scrub certain added semicolons */
      if (((cpd.lang_flags & LANG_PAWN) != 0) &&
          cpd.settings[UO_mod_pawn_semicolon].b)
      {
         pawn_scrub_vsemi();
      }

      /* Sort imports/using/include */
      if (cpd.settings[UO_mod_sort_import].b ||
          cpd.settings[UO_mod_sort_include].b ||
          cpd.settings[UO_mod_sort_using].b)
      {
         sort_imports();
      }

      /**
       * Fix same-line inter-chunk spacing
       */
      space_text();

      /**
       * Do any aligning of preprocessors
       */
      if (cpd.settings[UO_align_pp_define_span].n > 0)
      {
         align_preprocessor();
      }

      /**
       * Indent the text
       */
      indent_preproc();
      indent_text();

      /* Insert trailing comments after certain close braces */
      if ((cpd.settings[UO_mod_add_long_switch_closebrace_comment].n > 0) ||
          (cpd.settings[UO_mod_add_long_function_closebrace_comment].n > 0) ||
          (cpd.settings[UO_mod_add_long_namespace_closebrace_comment].n > 0))
      {
         add_long_closebrace_comment();
      }

      /* Insert trailing comments after certain preprocessor conditional blocks */
      if ((cpd.settings[UO_mod_add_long_ifdef_else_comment].n > 0) ||
          (cpd.settings[UO_mod_add_long_ifdef_endif_comment].n > 0))
      {
         add_long_preprocessor_conditional_block_comment();
      }

      /**
       * Align everything else, reindent and break at code_width
       */
      first          = true;
      cpd.pass_count = 3;
      do
      {
         align_all();
         indent_text();
         old_changes = cpd.changes;
         if (cpd.settings[UO_code_width].n > 0)
         {
            LOG_FMT(LNEWLINE, "Code_width loop start: %d\n", cpd.changes);

            do_code_width();
            if ((old_changes != cpd.changes) && first)
            {
               /* retry line breaks caused by splitting 1-liners */
               newlines_cleanup_braces(false);
               newlines_insert_blank_lines();
               first = false;
            }
         }
      } while ((old_changes != cpd.changes) && (cpd.pass_count-- > 0));

      /**
       * And finally, align the backslash newline stuff
       */
      align_right_comments();
      if (cpd.settings[UO_align_nl_cont].b)
      {
         align_backslash_newline();
      }

      /**
       * Now render it all to the output file
       */
      output_text(pfout);
   }

   /* Special hook for dumping parsed data for debugging */
   if (parsed_file != NULL)
   {
      FILE *p_file = fopen(parsed_file, "w");
      if (p_file != NULL)
      {
         output_parsed(p_file);
         fclose(p_file);
      }
      else
      {
         LOG_FMT(LERR, "%s: Failed to open '%s' for write: %s (%d)\n",
                 __func__, parsed_file, strerror(errno), errno);
      }
   }

   if (cpd.do_check)
   {
      bool is_same = true;
      /* compare the old data vs the new data */
      if (cpd.bout->size() != fm.raw.size())
      {
         fprintf(stderr, "FAIL: %s (File size changed from %u to %u)\n",
                 cpd.filename,
                 (int)fm.raw.size(), (int)cpd.bout->size());
         cpd.check_fail_cnt++;
         is_same = false;
      }
      else
      {
         for (int idx = 0; idx < (int)fm.raw.size(); idx++)
         {
            if (fm.raw[idx] != (*cpd.bout)[idx])
            {
               fprintf(stderr, "FAIL: %s (Difference at byte %u)\n",
                       cpd.filename, idx);
               cpd.check_fail_cnt++;
               is_same = false;
               break;
            }
         }
      }
      if (is_same)
      {
         fprintf(stdout, "PASS: %s (%u bytes)\n", cpd.filename, (int)fm.raw.size());
      }
   }

   uncrustify_end();
}


static void uncrustify_end()
{
   /* Free all the memory */
   chunk_t *pc;

   while ((pc = chunk_get_head()) != NULL)
   {
      chunk_del(pc);
   }

   if (cpd.bout)
   {
      cpd.bout->clear();
   }

   /* Clean up some state variables */
   cpd.unc_off     = false;
   cpd.al_cnt      = 0;
   cpd.did_newline = true;
   cpd.frame_count = 0;
   cpd.pp_level    = 0;
   cpd.changes     = 0;
   cpd.in_preproc  = CT_NONE;
   cpd.consumed    = false;
   memset(cpd.le_counts, 0, sizeof(cpd.le_counts));
   cpd.preproc_ncnl_count    = 0;
   cpd.ifdef_over_whole_file = 0;
}


const char *get_token_name(c_token_t token)
{
   if ((token >= 0) && (token < (int)ARRAY_SIZE(token_names)) &&
       (token_names[token] != NULL))
   {
      return(token_names[token]);
   }
   return("???");
}


/**
 * Grab the token id for the text.
 * returns CT_NONE on failure to match
 */
c_token_t find_token_name(const char *text)
{
   int idx;

   if ((text != NULL) && (*text != 0))
   {
      for (idx = 1; idx < (int)ARRAY_SIZE(token_names); idx++)
      {
         if (strcasecmp(text, token_names[idx]) == 0)
         {
            return((c_token_t)idx);
         }
      }
   }
   return(CT_NONE);
}


static bool ends_with(const char *filename, const char *tag, bool case_sensitive = true)
{
   int len1 = strlen(filename);
   int len2 = strlen(tag);

   return((len2 <= len1) &&
          ((case_sensitive && (strcmp(&filename[len1 - len2], tag) == 0)) ||
           (!case_sensitive && (strcasecmp(&filename[len1 - len2], tag) == 0))));
}


struct lang_name_t
{
   const char *name;
   int        lang;
};

static lang_name_t language_names[] =
{
   { "C",    LANG_C             },
   { "CPP",  LANG_CPP           },
   { "D",    LANG_D             },
   { "CS",   LANG_CS            },
   { "VALA", LANG_VALA          },
   { "JAVA", LANG_JAVA          },
   { "PAWN", LANG_PAWN          },
   { "OC",   LANG_OC            },
   { "OC+",  LANG_OC | LANG_CPP },
   { "ECMA", LANG_ECMA          },
};


int language_flags_from_name(const char *name)
{
   int i;

   for (i = 0; i < (int)ARRAY_SIZE(language_names); i++)
   {
      if (strcasecmp(name, language_names[i].name) == 0)
      {
         return language_names[i].lang;
      }
   }
   return 0;
}


/**
 * Gets the tag text for a language
 *
 * @param lang    The LANG_xxx enum
 * @return        A string
 */
static const char *language_name_from_flags(int lang)
{
   int i;

   /* Check for an exact match first */
   for (i = 0; i < (int)ARRAY_SIZE(language_names); i++)
   {
      if (language_names[i].lang == lang)
      {
         return language_names[i].name;
      }
   }

   /* Check for the first set language bit */
   for (i = 0; i < (int)ARRAY_SIZE(language_names); i++)
   {
      if ((language_names[i].lang & lang) != 0)
      {
         return language_names[i].name;
      }
   }
   return "???";
}


struct lang_ext_t
{
   const char *ext;
   const char *name;
};

/* maps file extensions to language names */
struct lang_ext_t language_exts[] =
{
   { ".c",    "C"    },
   { ".cpp",  "CPP"  },
   { ".d",    "D"    },
   { ".cs",   "CS"   },
   { ".vala", "VALA" },
   { ".java", "JAVA" },
   { ".pawn", "PAWN" },
   { ".p",    "PAWN" },
   { ".sma",  "PAWN" },
   { ".inl",  "PAWN" },
   { ".h",    "CPP"  },
   { ".cxx",  "CPP"  },
   { ".hpp",  "CPP"  },
   { ".hxx",  "CPP"  },
   { ".cc",   "CPP"  },
   { ".cp",   "CPP"  },
   { ".C",    "CPP"  },
   { ".CPP",  "CPP"  },
   { ".c++",  "CPP"  },
   { ".di",   "D"    },
   { ".m",    "OC"   },
   { ".mm",   "OC+"  },
   { ".sqc",  "C"    }, // embedded SQL
   { ".es",   "ECMA" },
};

/**
 * Set idx = 0 before the first call.
 * Done when returns NULL
 */
const char *get_file_extension(int& idx)
{
   const char *val = NULL;

   if (idx < (int)ARRAY_SIZE(language_exts))
   {
      val = language_exts[idx].ext;
   }
   idx++;
   return val;
}


// maps a file extension to a language flag. include the ".", as in ".c".
// These ARE case sensitive user file extensions.
typedef std::map<string, string>   extension_map_t;
static extension_map_t g_ext_map;

const char *extension_add(const char *ext_text, const char *lang_text)
{
   int lang_flags = language_flags_from_name(lang_text);

   if (lang_flags)
   {
      const char *lang_name = language_name_from_flags(lang_flags);
      g_ext_map[string(ext_text)] = lang_name;
      return lang_name;
   }
   return NULL;
}


/**
 * Prints custom file extensions to the file
 */
void print_extensions(FILE *pfile)
{
   for (int idx = 0; idx < (int)ARRAY_SIZE(language_names); idx++)
   {
      const char *lang_name = language_names[idx].name;
      bool       did_one    = false;
      for (extension_map_t::iterator it = g_ext_map.begin(); it != g_ext_map.end(); ++it)
      {
         if (strcmp(it->second.c_str(), lang_name) == 0)
         {
            if (!did_one)
            {
               fprintf(pfile, "file_ext %s", it->second.c_str());
               did_one = true;
            }
            fprintf(pfile, " %s", it->first.c_str());
         }
      }
      if (did_one)
      {
         fprintf(pfile, "\n");
      }
   }
}


/**
 * Find the language for the file extension
 * Default to C
 *
 * @param filename   The name of the file
 * @return           LANG_xxx
 */
static int language_flags_from_filename(const char *filename)
{
   int i;

   /* check custom extensions first */
   for (extension_map_t::iterator it = g_ext_map.begin(); it != g_ext_map.end(); ++it)
   {
      if (ends_with(filename, it->first.c_str()))
      {
         return language_flags_from_name(it->second.c_str());
      }
   }

   for (i = 0; i < (int)ARRAY_SIZE(language_exts); i++)
   {
      if (ends_with(filename, language_exts[i].ext))
      {
         return language_flags_from_name(language_exts[i].name);
      }
   }

   /* check again without case sensitivity */
   for (extension_map_t::iterator it = g_ext_map.begin(); it != g_ext_map.end(); ++it)
   {
      if (ends_with(filename, it->first.c_str(), false))
      {
         return language_flags_from_name(it->second.c_str());
      }
   }
   for (i = 0; i < (int)ARRAY_SIZE(language_exts); i++)
   {
      if (ends_with(filename, language_exts[i].ext, false))
      {
         return language_flags_from_name(language_exts[i].name);
      }
   }
   return LANG_C;
}


void log_pcf_flags(log_sev_t sev, UINT64 flags)
{
   if (!log_sev_on(sev))
   {
      return;
   }

   log_fmt(sev, "[0x%" PRIx64 ":", flags);

   const char *tolog = NULL;
   for (int i = 0; i < (int)ARRAY_SIZE(pcf_names); i++)
   {
      if ((flags & (1ULL << i)) != 0)
      {
         if (tolog != NULL)
         {
            log_str(sev, tolog, strlen(tolog));
            log_str(sev, ",", 1);
         }
         tolog = pcf_names[i];
      }
   }

   if (tolog != NULL)
   {
      log_str(sev, tolog, strlen(tolog));
   }

   log_str(sev, "]\n", 2);
}
