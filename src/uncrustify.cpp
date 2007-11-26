/**
 * @file uncrustify.cpp
 * This file takes an input C/C++/D/Java file and reformats it.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 *
 * $Id$
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
#include <cctype>
#ifdef HAVE_SYS_STAT_H
 #include <sys/stat.h>
#endif
#ifdef HAVE_STRINGS_H
 #include <strings.h> /* strcasecmp() */
#endif

/* Global data */
struct cp_data cpd;


static int language_from_tag(const char *tag);
static int language_from_filename(const char *filename);
static const char *language_to_string(int lang);
static char *read_stdin(int& out_len);
static void uncrustify_file(const char *data, int data_len, FILE *pfout,
                            const char *parsed_file);
static void do_source_file(const char *filename, FILE *pfout, const char *parsed_file,
                           const char *prefix, const char *suffix, bool no_backup, bool keep_mtime);
static void process_source_list(const char *source_list, const char *prefix,
                                const char *suffix, bool no_backup, bool keep_mtime);
static int load_header_files();

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
      if (ch == PATH_SEP)
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
   return(path_basename(filename) - filename);
}

static void usage_exit(const char *msg, const char *argv0, int code)
{
   if (msg != NULL)
   {
      fprintf(stderr, "%s\n", msg);
   }
   if (code != EXIT_SUCCESS)
   {
      fprintf(stderr, "Try running with -h for usage information\n");
      exit(code);
   }
   fprintf(stderr,
           "Usage:\n"
           "%s [options] [files ...]\n"
           "\n"
           "Basic Options:\n"
           " -c CFG       : use the config file CFG\n"
           " -f FILE      : process the single file FILE (output to stdout, use with -o)\n"
           " -o FILE      : Redirect stdout to FILE\n"
           " -F FILE      : read files to process from FILE, one filename per line\n"
           " files        : files to process (can be combined with -F)\n"
           " --suffix SFX : Append SFX to the output filename. The default is '.uncrustify'\n"
           " --prefix PFX : Prepend PFX to the output filename path.\n"
           " --replace    : replace source files (creates a backup)\n"
           " --no-backup  : replace files, no backup. Useful if files are under source control\n"
#ifdef HAVE_UTIME_H
           " --mtime      : preserve mtime on replaced files\n"
#endif
           " -l           : language override: C, CPP, D, CS, JAVA, PAWN\n"
           " -t           : load a file with types (usually not needed)\n"
           " -q           : quiet mode - no output on stderr (-L will override)\n"
           "\n"
           "Config/Help Options:\n"
           " -h -? --help --usage     : print this message and exit\n"
           " --version                : print the version and exit\n"
           " --show-config            : print out option documentation and exit\n"
           " --update-config          : Output a new config file. Use with -o FILE\n"
           " --update-config-with-doc : Output a new config file. Use with -o FILE\n"
           " --universalindent        : Output a config file for Universal Indent GUI\n"
           "\n"
           "Debug Options:\n"
           " -p FILE      : dump debug info to a file\n"
           " -L SEV       : Set the log severity (see log_levels.h)\n"
           " -s           : Show the log severity in the logs\n"
           " --decode FLAG: Print FLAG (chunk flags) as text and exit\n"
           "\n"
           "If no input files are specified, the input is read from stdin\n"
           "If -F is used or files are specified on the command line, the output is 'prefix/filename' + suffix\n"
           "Otherwise, the output is dumped to stdout, unless redirected with -o FILE.\n"
           "Errors are always dumped to stderr\n",
           path_basename(argv0));
   exit(code);
}

static void version_exit(void)
{
   printf("uncrustify %s\n", UNCRUSTIFY_VERSION);
   exit(0);
}

int main(int argc, char *argv[])
{
   char       *data        = NULL;
   int        data_len     = 0;
   const char *cfg_file    = "uncrustify.cfg";
   const char *parsed_file = NULL;
   const char *source_file = NULL;
   const char *output_file = NULL;
   const char *source_list = NULL;
   log_mask_t mask;
   int        idx;
   const char *p_arg;

   /* If ran without options... check keywork sort and show the usage info */
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
      print_options(stdout, true);
      return(0);
   }

#ifdef WIN32
   /* tell windoze not to change what I write to stdout */
   _setmode(_fileno(stdout), _O_BINARY);
#endif

   /* Init logging */
   log_init(stderr);
   if (arg.Present("-q"))
   {
      logmask_from_string("", &mask);
      log_set_mask(&mask);
   }
   if (((p_arg = arg.Param("-L")) != NULL) ||
       ((p_arg = arg.Param("--log")) != NULL))
   {
      logmask_from_string(p_arg, &mask);
      log_set_mask(&mask);
   }

   if ((p_arg = arg.Param("--decode")) != NULL)
   {
      log_pcf_flags(LSYS, strtoul(p_arg, NULL, 16));
      exit(EXIT_SUCCESS);
   }

   /* Get the config file name */
   if (((cfg_file = arg.Param("--config")) == NULL) &&
       ((cfg_file = arg.Param("-c")) == NULL))
   {
#ifdef WIN32
      usage_exit("Specify the config file: -c file", argv[0], 58);
#endif
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
      add_keyword(p_arg, CT_TYPE, LANG_ALL);
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
      cpd.lang_flags = language_from_tag(p_arg);
      if (cpd.lang_flags == 0)
      {
         LOG_FMT(LWARN, "Ignoring unknown language: %s\n", p_arg);
      }
   }

   /* Get the source file name */
   if (((source_file = arg.Param("--file")) == NULL) &&
       ((source_file = arg.Param("-f")) == NULL))
   {
      // not using a single file
   }

   if (((source_list = arg.Param("--files")) == NULL) &&
       ((source_list = arg.Param("-F")) == NULL))
   {
      // not using a file list
   }

   const char *prefix = arg.Param("--prefix");
   const char *suffix = arg.Param("--suffix");

   bool no_backup = arg.Present("--no-backup");
   if (arg.Present("--replace") || no_backup)
   {
      if ((prefix != NULL) || (suffix != NULL))
      {
         usage_exit("Cannot use --replace with --prefix or --suffix", argv[0], 66);
      }
   }
   else
   {
      if ((prefix == NULL) && (suffix == NULL))
      {
         suffix = ".uncrustify";
      }
   }
   bool keep_mtime = arg.Present("--mtime");

   /* Grab the output override */
   output_file = arg.Param("-o");

   bool update_config    = arg.Present("--update-config");
   bool update_config_wd = arg.Present("--update-config-with-doc");

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
            return(EXIT_FAILURE);
         }
      }

      print_universal_indent_cfg(pfile);

      return(EXIT_SUCCESS);
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
         usage_exit("Cannot specify both the single file option and a mulit-file option.",
                    argv[0], 67);
      }

      if (output_file != NULL)
      {
         printf("source_file = %s, p_arg = %s\n", source_file, p_arg);
         usage_exit("Cannot specify -o with a mulit-file option.",
                    argv[0], 68);
      }
   }

#ifndef WIN32
   char buf[512];
   if (cfg_file == NULL)
   {
      cfg_file = getenv("UNCRUSTIFY_CONFIG");
      if (cfg_file == NULL)
      {
         const char *home = getenv("HOME");

         if (home == NULL)
         {
            usage_exit("Specify the config file with '-c file' or set UNCRUSTIFY_CONFIG",
                       argv[0], 58);
         }
         snprintf(buf, sizeof(buf), "%s/.uncrustify.cfg", home);
         cfg_file = buf;
      }
   }
#endif

   cpd.filename = cfg_file;
   if (load_option_file(cfg_file) < 0)
   {
      usage_exit("Unable to load the config file", argv[0], 56);
   }

   /* Reopen stdout */
   FILE *my_stdout = stdout;
   if (output_file != NULL)
   {
      my_stdout = freopen(output_file, "wb", stdout);
      if (my_stdout == NULL)
      {
         LOG_FMT(LERR, "Unable to open %s for write: %s (%d)\n",
                 output_file, strerror(errno), errno);
         usage_exit(NULL, argv[0], 56);
      }
      LOG_FMT(LNOTE, "Redirecting output to %s\n", output_file);
   }

   if (update_config || update_config_wd)
   {
      save_option_file(stdout, update_config_wd);
      return(0);
   }

   /* This relies on cpd.filename being the config file name */
   load_header_files();

   if ((source_file == NULL) && (source_list == NULL) && (p_arg == NULL))
   {
      /* no input specified, so use stdin */
      if (cpd.lang_flags == 0)
      {
         cpd.lang_flags = LANG_C;
      }

      data = read_stdin(data_len);
      if (data == NULL)
      {
         LOG_FMT(LERR, "Out of memory\n");
         return(100);
      }

      cpd.filename = "stdin";

      /* Done reading from stdin */
      LOG_FMT(LSYS, "Parsing: %d bytes from stdin as language %s\n",
              data_len, language_to_string(cpd.lang_flags));

      uncrustify_file(data, data_len, stdout, parsed_file);
      free(data);
      data_len = 0;
      data     = NULL;
   }
   else if (source_file != NULL)
   {
      /* Doing a single file, output to stdout */
      do_source_file(source_file, stdout, parsed_file, NULL, NULL, no_backup, false);
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
         do_source_file(p_arg, NULL, NULL, prefix, suffix, no_backup, keep_mtime);
      }

      if (source_list != NULL)
      {
         process_source_list(source_list, prefix, suffix, no_backup, keep_mtime);
      }
   }

   clear_keyword_file();
   clear_defines();

   return((cpd.error_count != 0) ? 1 : 0);
}

static void process_source_list(const char *source_list,
                                const char *prefix, const char *suffix,
                                bool no_backup, bool keep_mtime)
{
   FILE *p_file = fopen(source_list, "r");

   if (p_file == NULL)
   {
      LOG_FMT(LERR, "%s: fopen(%s) failed: %s (%d)\n",
              __func__, source_list, strerror(errno), errno);
      cpd.error_count++;
      return;
   }

   char linebuf[256];
   int  argc;
   char *args[3];
   int  line = 0;
   int  idx;

   while (fgets(linebuf, sizeof(linebuf), p_file) != NULL)
   {
      line++;
      argc = Args::SplitLine(linebuf, args, ARRAY_SIZE(args));

      LOG_FMT(LFILELIST, "%3d]", line);
      for (idx = 0; idx < argc; idx++)
      {
         LOG_FMT(LFILELIST, " [%s]", args[idx]);
      }
      LOG_FMT(LFILELIST, "\n");

      if ((argc == 1) && (*args[0] != '#'))
      {
         do_source_file(args[0], NULL, NULL, prefix, suffix, no_backup, keep_mtime);
      }
   }
}

static char *read_stdin(int& out_len)
{
   char *data;
   char *new_data;
   int  data_size;
   int  data_len;
   int  len;

   /* Start with 64k */
   data_size = 64 * 1024;
   data      = (char *)malloc(data_size);
   data_len  = 0;

   if (data == NULL)
   {
      return(NULL);
   }

   while ((len = fread(&data[data_len], 1, data_size - data_len, stdin)) > 0)
   {
      data_len += len;
      if (data_len == data_size)
      {
         /* Double the buffer size */
         data_size *= 2;
         if ((new_data = (char *)realloc(data, data_size)) == NULL)
         {
            free(data);
            return(NULL);
         }
         data = new_data;
      }
   }

   assert(data_len < data_size);
   /* Make sure the buffer is terminated */
   data[data_len] = 0;

   out_len = data_len;

   return(data);
}

static void make_folders(char *outname)
{
   int idx;
   int last_idx = 0;

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
int load_mem_file(const char *filename, file_mem& fm)
{
   int         retval = -1;
   struct stat my_stat;
   FILE        *p_file;

   fm.data   = NULL;
   fm.length = 0;

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

   fm.length = my_stat.st_size;
   fm.data   = (char *)malloc(fm.length + 1);
   if (fm.data == NULL)
   {
      LOG_FMT(LERR, "%s: fopen(%s) failed: out of memory\n", __func__, filename);
      cpd.error_count++;
   }
   else if (fread(fm.data, fm.length, 1, p_file) != 1)
   {
      LOG_FMT(LERR, "%s: fread(%s) failed: %s (%d)\n",
              __func__, filename, strerror(errno), errno);
      cpd.error_count++;
   }
   else
   {
      fm.data[fm.length] = 0;
      retval = 0;
   }
   fclose(p_file);

   if ((retval != 0) && (fm.data != NULL))
   {
      free(fm.data);
      fm.data   = NULL;
      fm.length = 0;
   }
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
   return(retval);
}

/**
 * Does a source file.
 * If pfout is NULL, the source fileaname + ".uncrustify" is used.
 *
 * @param filename the file to read
 * @param pfout    NULL or the output stream
 */
static void do_source_file(const char *filename, FILE *pfout, const char *parsed_file,
                           const char *prefix, const char *suffix, bool no_backup, bool keep_mtime)
{
   bool     did_open    = false;
   bool     need_backup = false;
   file_mem fm;

   /* Do some simple language detection based on the filename */
   if (cpd.lang_flags == 0)
   {
      cpd.lang_flags = language_from_filename(filename);
   }

   /* Try to read in the source file */
   if (load_mem_file(filename, fm) < 0)
   {
      LOG_FMT(LERR, "Failed to load (%s)\n", filename);
      cpd.error_count++;
      return;
   }

   LOG_FMT(LSYS, "Parsing: %s as language %s\n",
           filename, language_to_string(cpd.lang_flags));

   if (pfout == NULL)
   {
      char outname[1024];
      int  len = 0;

      if (!no_backup && (prefix == NULL) && (suffix == NULL))
      {
         if (backup_copy_file(filename, fm.data, fm.length) != SUCCESS)
         {
            LOG_FMT(LERR, "%s: Failed to create backup file for %s\n",
                    __func__, filename);
            free(fm.data);
            return;
         }
         need_backup = true;
      }

      if (prefix != NULL)
      {
         len = snprintf(outname, sizeof(outname), "%s/", prefix);
      }

      snprintf(&outname[len], sizeof(outname) - len, "%s%s", filename,
               (suffix != NULL) ? suffix : "");

      make_folders(outname);

      pfout = fopen(outname, "wb");
      if (pfout == NULL)
      {
         LOG_FMT(LERR, "%s: Unable to create %s: %s (%d)\n",
                 __func__, outname, strerror(errno), errno);
         cpd.error_count++;
         free(fm.data);
         return;
      }
      did_open = true;
      //LOG_FMT(LSYS, "Output file %s\n", outname);
   }

   cpd.filename = filename;
   uncrustify_file(fm.data, fm.length, pfout, parsed_file);

   free(fm.data);

   if (did_open)
   {
      fclose(pfout);

      if (need_backup && !no_backup)
      {
         backup_create_md5_file(filename);
      }

#ifdef HAVE_UTIME_H
      if (keep_mtime)
      {
         /* update mtime -- don't care if it fails */
         fm.utb.actime = time(NULL);
         (void)utime(filename, &fm.utb);
      }
#endif
   }
}

static void add_file_header()
{
   if (!chunk_is_comment(chunk_get_head()))
   {
      /*TODO: detect the typical #ifndef FOO / #define FOO sequence */
      tokenize(cpd.file_hdr.data, cpd.file_hdr.length, chunk_get_head());
   }
}

static void add_func_header(c_token_t type, file_mem& fm)
{
   chunk_t *pc;
   chunk_t *ref;

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next_ncnlnp(pc))
   {
      if (pc->type != type)
      {
         continue;
      }

      /* On a function proto or def. Back up to a close brace or semicolon on
       * the same level
       */
      ref = pc;
      while ((ref = chunk_get_prev(ref)) != NULL)
      {
         if (chunk_is_comment(ref) ||
             ((ref->level != pc->level) &&
              (ref->flags & PCF_IN_PREPROC)))
         {
            break;
         }

         if ((ref->level == pc->level) &&
             ((ref->flags & PCF_IN_PREPROC) ||
              (ref->type == CT_SEMICOLON) ||
              (ref->type == CT_BRACE_CLOSE)))
         {
            tokenize(fm.data, fm.length, chunk_get_next_nnl(ref));
            break;
         }
      }
   }
}

static void uncrustify_file(const char *data, int data_len, FILE *pfout,
                            const char *parsed_file)
{
   /**
    * Parse the text into chunks
    */
   tokenize(data, data_len, NULL);

   /* Add the file header */
   if (cpd.file_hdr.data != NULL)
   {
      add_file_header();
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

   /**
    * Add comments before function defs and classes
    */
   if (cpd.func_hdr.data != NULL)
   {
      add_func_header(CT_FUNC_DEF, cpd.func_hdr);
   }
   if (cpd.class_hdr.data != NULL)
   {
      add_func_header(CT_CLASS, cpd.class_hdr);
   }

   /* Scrub extra semicolons */
   if (cpd.settings[UO_mod_remove_extra_semicolon].b)
   {
      remove_extra_semicolons();
   }

   /**
    * Look at all colons ':' and mark labels, :? sequences, etc.
    */
   combine_labels();

   /**
    * Change virtual braces into real braces...
    */
   do_braces();

   /**
    * Add parens
    */
   do_parens();

   /**
    * Insert line breaks as needed
    */
   do_blank_lines();
   newlines_cleanup_braces();
   if (cpd.settings[UO_nl_after_multiline_comment].b)
   {
      newline_after_multiline_comment();
   }
   newlines_insert_blank_lines();
   if (cpd.settings[UO_nl_squeeze_ifdef].b)
   {
      newlines_squeeze_ifdef();
   }
   if (cpd.settings[UO_pos_bool].tp != TP_IGNORE)
   {
      newlines_chunk_pos(CT_BOOL, cpd.settings[UO_pos_bool].tp);
   }
   if (cpd.settings[UO_pos_comma].tp != TP_IGNORE)
   {
      newlines_chunk_pos(CT_COMMA, cpd.settings[UO_pos_comma].tp);
   }
   newlines_class_colon_pos();
   newlines_eat_start_end();
   newlines_cleanup_dup();

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

   /* Insert trailing comments after certain close braces */
   if ((cpd.settings[UO_mod_add_long_switch_closebrace_comment].n > 0) ||
       (cpd.settings[UO_mod_add_long_function_closebrace_comment].n > 0))
   {
      add_long_closebrace_comment();
   }

   /* Sort imports */
   if (cpd.settings[UO_mod_sort_import].b)
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

   /**
    * Aligning everything else and reindent
    */
   align_all();
   indent_text();

   if (cpd.settings[UO_code_width].n > 0)
   {
      int max_passes = 3;
      int prev_changes;
      do
      {
         prev_changes = cpd.changes;
         do_code_width();
         if (prev_changes != cpd.changes)
         {
            indent_text();
         }
      } while ((prev_changes != cpd.changes) && (--max_passes > 0));
   }

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

   /* Special hook for dumping parsed data for debugging */
   if (parsed_file != NULL)
   {
      FILE *p_file = fopen(parsed_file, "w");
      if (p_file != NULL)
      {
         output_parsed(p_file);
         fclose(p_file);
      }
   }

   /* Free all the memory */
   chunk_t *pc;
   while ((pc = chunk_get_head()) != NULL)
   {
      chunk_del(pc);
   }
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

static bool ends_with(const char *filename, const char *tag)
{
   int len1 = strlen(filename);
   int len2 = strlen(tag);

   if ((len2 <= len1) && (strcmp(&filename[len1 - len2], tag) == 0))
   {
      return(true);
   }
   return(false);
}

struct file_lang
{
   const char *ext;
   const char *tag;
   int        lang;
};

struct file_lang languages[] =
{
   { ".c",    "C",    LANG_C    },
   { ".cpp",  "CPP",  LANG_CPP  },
   { ".d",    "D",    LANG_D    },
   { ".cs",   "CS",   LANG_CS   },
   { ".vala", "VALA", LANG_VALA },
   { ".java", "JAVA", LANG_JAVA },
   { ".pawn", "PAWN", LANG_PAWN },
   { ".p",    "",     LANG_PAWN },
   { ".sma",  "",     LANG_PAWN },
   { ".inl",  "",     LANG_PAWN },
   { ".h",    "",     LANG_CPP  },
   { ".cxx",  "",     LANG_CPP  },
   { ".hpp",  "",     LANG_CPP  },
   { ".hxx",  "",     LANG_CPP  },
   { ".cc",   "",     LANG_CPP  },
   { ".di",   "",     LANG_D    },
   { ".m",    "OC",   LANG_OC   },
   { ".sqc",  "",     LANG_C    }, // embedded SQL
};

/**
 * Set idx = 0 before the first call.
 * Done when returns NULL
 */
const char *get_file_extension(int& idx)
{
   const char *val = NULL;

   if (idx < (int)ARRAY_SIZE(languages))
   {
      val = languages[idx].ext;
   }
   idx++;
   return(val);
}

/**
 * Find the language for the file extension
 * Default to C
 *
 * @param filename   The name of the file
 * @return           LANG_xxx
 */
static int language_from_filename(const char *filename)
{
   int i;

   for (i = 0; i < (int)ARRAY_SIZE(languages); i++)
   {
      if (ends_with(filename, languages[i].ext))
      {
         return(languages[i].lang);
      }
   }
   return(LANG_C);
}

/**
 * Find the language for the file extension
 *
 * @param filename   The name of the file
 * @return           LANG_xxx or 0 (no match)
 */
static int language_from_tag(const char *tag)
{
   int i;

   for (i = 0; i < (int)ARRAY_SIZE(languages); i++)
   {
      if (strcasecmp(tag, languages[i].tag) == 0)
      {
         return(languages[i].lang);
      }
   }
   return(0);
}

/**
 * Gets the tag text for a language
 *
 * @param lang    The LANG_xxx enum
 * @return        A string
 */
static const char *language_to_string(int lang)
{
   int i;

   for (i = 0; i < (int)ARRAY_SIZE(languages); i++)
   {
      if ((languages[i].lang & lang) != 0)
      {
         return(languages[i].tag);
      }
   }
   return("???");
}

void log_pcf_flags(log_sev_t sev, UINT32 flags)
{
   if (!log_sev_on(sev))
   {
      return;
   }

   log_fmt(sev, "[0x%X:", flags);

   const char *tolog = NULL;
   for (int i = 0; i < (int)ARRAY_SIZE(pcf_names); i++)
   {
      if ((flags & (1 << i)) != 0)
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
