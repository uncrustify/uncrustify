/**
 * @file uncrustify.cpp
 * This file takes an input C/C++/D/Java file and reformats it.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#define DEFINE_PCF_NAMES
#define DEFINE_CHAR_TABLE

#include "uncrustify.h"

#include "align.h"
#include "args.h"
#include "backup.h"
#include "brace_cleanup.h"
#include "braces.h"
#include "char_table.h"
#include "chunk_list.h"
#include "combine.h"
#include "compat.h"
#include "detect.h"
#include "enum_cleanup.h"
#include "indent.h"
#include "keywords.h"
#include "lang_pawn.h"
#include "language_tools.h"
#include "log_levels.h"
#include "logger.h"
#include "md5.h"
#include "newlines.h"
#include "options.h"
#include "output.h"
#include "parens.h"
#include "prototypes.h"
#include "semicolons.h"
#include "sorting.h"
#include "space.h"
#include "token_names.h"
#include "tokenize.h"
#include "tokenize_cleanup.h"
#include "unc_ctype.h"
#include "uncrustify_types.h"
#include "uncrustify_version.h"
#include "unicode.h"
#include "universalindentgui.h"
#include "width.h"

#include <deque>
#include <map>
#include <vector>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>    // provides strcasecmp()
#endif
#ifdef HAVE_UTIME_H
#include <time.h>
#endif


// VS throws an error if an attribute newer than the requested standard level
// is used; everyone else just ignores it (or warns) like they are supposed to

#if __cplusplus >= 201703L
#define NODISCARD    [[nodiscard]]
#elif defined (__has_cpp_attribute)
#if __has_cpp_attribute(nodiscard)
#define NODISCARD    [[nodiscard]]
#else
#define NODISCARD
#endif
#else
#define NODISCARD
#endif


using namespace std;
using namespace uncrustify;


// Global data
cp_data_t cpd;


static size_t language_flags_from_name(const char *tag);


/**
 * Find the language for the file extension
 * Defaults to C
 *
 * @param filename   The name of the file
 * @return           LANG_xxx
 */
static size_t language_flags_from_filename(const char *filename);


static bool read_stdin(file_mem &fm);


static void uncrustify_start(const deque<int> &data);


static bool ends_with(const char *filename, const char *tag, bool case_sensitive);


/**
 * Does a source file.
 *
 * @param filename_in  the file to read
 * @param filename_out nullptr (stdout) or the file to write
 * @param parsed_file  nullptr or the filename for the parsed debug info
 * @param no_backup    don't create a backup, if filename_out == filename_in
 * @param keep_mtime   don't change the mtime (dangerous)
 */
static void do_source_file(const char *filename_in, const char *filename_out, const char *parsed_file, bool no_backup, bool keep_mtime);


static void add_file_header();


static void add_file_footer();


static void add_func_header(c_token_t type, file_mem &fm);


static void add_msg_header(c_token_t type, file_mem &fm);


static void process_source_list(const char *source_list, const char *prefix, const char *suffix, bool no_backup, bool keep_mtime);


static const char *make_output_filename(char *buf, size_t buf_size, const char *filename, const char *prefix, const char *suffix);


//! compare the content of two files
static bool file_content_matches(const string &filename1, const string &filename2);


static string fix_filename(const char *filename);


static bool bout_content_matches(const file_mem &fm, bool report_status);


/**
 * Loads a file into memory
 *
 * @param filename  name of file to load
 *
 * @retval true   file was loaded successfully
 * @retval false  file could not be loaded
 */
static int load_mem_file(const char *filename, file_mem &fm);


/**
 * Try to load the file from the config folder first and then by name
 *
 * @param filename  name of file to load
 *
 * @retval true   file was loaded successfully
 * @retval false  file could not be loaded
 */
static int load_mem_file_config(const std::string &filename, file_mem &fm);


//! print uncrustify version number and terminate
static void version_exit(void);


const char *path_basename(const char *path)
{
   if (path == nullptr)
   {
      return("");
   }

   const char *last_path = path;
   char       ch;

   while ((ch = *path) != 0) // check for end of string
   {
      path++;
      // Check both slash types to support Linux and Windows
      if ((ch == '/') || (ch == '\\'))
      {
         last_path = path;
      }
   }
   return(last_path);
}


int path_dirname_len(const char *filename)
{
   if (filename == nullptr)
   {
      return(0);
   }
   // subtracting addresses like this works only on big endian systems
   return(static_cast<int>(path_basename(filename) - filename));
}


void usage_error(const char *msg)
{
   if (msg != nullptr)
   {
      fprintf(stderr, "%s\n", msg);
      log_flush(true);
   }
   fprintf(stderr, "Try running with -h for usage information\n");
   log_flush(true);
}


void usage(const char *argv0)
{
   fprintf(stdout,
           "Usage:\n"
           "%s [options] [files ...]\n"
           "\n"
           "If no input files are specified, the input is read from stdin\n"
           "If reading from stdin, you should specify the language using -l\n"
           "or specify a filename using --assume for automatic language detection.\n"
           "\n"
           "If -F is used or files are specified on the command line,\n"
           "the output filename is 'prefix/filename' + suffix\n"
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
           " -c CFG       : Use the config file CFG.\n"
           " -f FILE      : Process the single file FILE (output to stdout, use with -o).\n"
           " -o FILE      : Redirect stdout to FILE.\n"
           " -F FILE      : Read files to process from FILE, one filename per line (- is stdin).\n"
           " --check      : Do not output the new text, instead verify that nothing changes when\n"
           "                the file(s) are processed.\n"
           "                The status of every file is printed to stderr.\n"
           "                The exit code is EXIT_SUCCESS if there were no changes, EXIT_FAILURE otherwise.\n"
           " files        : Files to process (can be combined with -F).\n"
           " --suffix SFX : Append SFX to the output filename. The default is '.uncrustify'\n"
           " --prefix PFX : Prepend PFX to the output filename path.\n"
           " --replace    : Replace source files (creates a backup).\n"
           " --no-backup  : Replace files, no backup. Useful if files are under source control.\n"
           " --if-changed : Write to stdout (or create output FILE) only if a change was detected.\n"
#ifdef HAVE_UTIME_H
           " --mtime      : Preserve mtime on replaced files.\n"
#endif
           " -l           : Language override: C, CPP, D, CS, JAVA, PAWN, OC, OC+, VALA.\n"
           " -t           : Load a file with types (usually not needed).\n"
           " -q           : Quiet mode - no output on stderr (-L will override).\n"
           " --frag       : Code fragment, assume the first line is indented correctly.\n"
           " --assume FN  : Uses the filename FN for automatic language detection if reading\n"
           "                from stdin unless -l is specified.\n"
           "\n"
           "Config/Help Options:\n"
           " -h -? --help --usage     : Print this message and exit.\n"
           " --version                : Print the version and exit.\n"
           " --show-config            : Print out option documentation and exit.\n"
           " --update-config          : Output a new config file. Use with -o FILE.\n"
           " --update-config-with-doc : Output a new config file. Use with -o FILE.\n"
           " --universalindent        : Output a config file for Universal Indent GUI.\n"
           " --detect                 : Detects the config from a source file. Use with '-f FILE'.\n"
           "                            Detection is fairly limited.\n"
           " --set <option>=<value>   : Sets a new value to a config option.\n"
           "\n"
           "Debug Options:\n"
           " -p FILE      : Dump debug info to a file. Must be used with the option -f FILE.\n"
           " -L SEV       : Set the log severity (see log_levels.h; note 'A' = 'all')\n"
           " -s           : Show the log severity in the logs.\n"
           " --decode     : Decode remaining args (chunk flags) and exit.\n"
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
           "      processing of parts of the source file (these can be overridden with\n"
           "      enable_processing_cmt and disable_processing_cmt).\n"
           "\n"
           "There are currently %zd options and minimal documentation.\n"
           "Try UniversalIndentGUI and good luck.\n"
           "\n"
           ,
           path_basename(argv0), get_option_count());
} // usage


static void version_exit(void)
{
   printf("%s\n", UNCRUSTIFY_VERSION);
   exit(EX_OK);
}


NODISCARD static int redir_stdout(const char *output_file)
{
   FILE *my_stdout = stdout;  // Reopen stdout

   if (output_file != nullptr)
   {
      my_stdout = freopen(output_file, "wb", stdout);
      if (my_stdout == nullptr)
      {
         LOG_FMT(LERR, "Unable to open %s for write: %s (%d)\n",
                 output_file, strerror(errno), errno);
         cpd.error_count++;
         usage_error();
         return(EX_IOERR);
      }
      LOG_FMT(LNOTE, "Redirecting output to %s\n", output_file);
   }
   return(EXIT_SUCCESS);
}

// Currently, the crash handler is only supported while building under MSVC
#if defined (WIN32) && defined (_MSC_VER)


void setup_crash_handling()
{
   // prevent crash popup. uncrustify is a batch processing tool and a popup is unacceptable.
   ::SetErrorMode(::GetErrorMode() | SEM_NOGPFAULTERRORBOX);

   struct local
   {
      static LONG WINAPI crash_filter(_In_ struct _EXCEPTION_POINTERS *exceptionInfo)
      {
         __try
         {
            LOG_FMT(LERR, "crash_filter: exception 0x%08X at [%d:%d] (ip=%p)",
                    exceptionInfo->ExceptionRecord->ExceptionCode,
                    cpd.line_number, cpd.column,
                    exceptionInfo->ExceptionRecord->ExceptionAddress);
            log_func_stack(LERR, " [CallStack:", "]\n", 0);

            // treat an exception the same as a parse failure. exceptions can result from parse failures where we
            // do not have specific handling (null-checks for particular parse paths etc.) and callers generally
            // won't care about the difference. they just want to know it failed.
            exit(EXIT_FAILURE);
         }
         __except (EXCEPTION_EXECUTE_HANDLER)
         {
            // have to be careful of crashes in crash handling code
         }

         // safety - note that this will not flush like we need, but at least will get the right return code
         ::ExitProcess(EXIT_FAILURE);
      }
   };

   // route all crashes through our own handler
   ::SetUnhandledExceptionFilter(local::crash_filter);
}

#else


void setup_crash_handling()
{
   // TODO: unixes
}

#endif


int main(int argc, char *argv[])
{
   // initialize the global data
   cpd.unc_off_used = false;

   setup_crash_handling();

   init_keywords();

   // check keyword sort
   assert(keywords_are_sorted());

   // Build options map
   register_options();

   // If ran without options show the usage info and exit */
   if (argc == 1)
   {
      usage(argv[0]);
      return(EXIT_SUCCESS);
   }

#ifdef DEBUG
   // make sure we have 'name' not too big
#define MAXLENGTHOFTHENAME    19
   // maxLengthOfTheName must be consider at the format line at the file
   // output.cpp, line 427: fprintf(pfile, "# Line              Tag                Parent...
   // and              430: ... fprintf(pfile, "%s# %3zu>%19.19s[%19.19s] ...
   // here                                                xx xx   xx xx
   for (size_t token = 0; token < ARRAY_SIZE(token_names); token++)
   {
      size_t lengthOfTheName = strlen(token_names[token]);
      if (lengthOfTheName > MAXLENGTHOFTHENAME)
      {
         fprintf(stderr, "%s(%d): The token name '%s' is too long (%zu)\n",
                 __func__, __LINE__, token_names[token], lengthOfTheName);
         fprintf(stderr, "%s(%d): the max token name length is %d\n",
                 __func__, __LINE__, MAXLENGTHOFTHENAME);
         log_flush(true);
         exit(EX_SOFTWARE);
      }
   }

   // make sure we have token_names.h in sync with token_enum.h
   assert(ARRAY_SIZE(token_names) == CT_TOKEN_COUNT_);
#endif // DEBUG

   Args arg(argc, argv);
   if (arg.Present("--version") || arg.Present("-v"))
   {
      version_exit();
   }
   if (  arg.Present("--help")
      || arg.Present("-h")
      || arg.Present("--usage")
      || arg.Present("-?"))
   {
      usage(argv[0]);
      return(EXIT_SUCCESS);
   }

   if (arg.Present("--show-config"))
   {
      save_option_file(stdout, true);
      return(EXIT_SUCCESS);
   }

   cpd.do_check   = arg.Present("--check");
   cpd.if_changed = arg.Present("--if-changed");

#ifdef WIN32
   // tell Windows not to change what I write to stdout
   UNUSED(_setmode(_fileno(stdout), _O_BINARY));
#endif

   // Init logging
   log_init(cpd.do_check ? stdout : stderr);
   log_mask_t mask;
   if (arg.Present("-q"))
   {
      logmask_from_string("", mask);
      log_set_mask(mask);
   }

   const char *p_arg;
   if (  ((p_arg = arg.Param("-L")) != nullptr)
      || ((p_arg = arg.Param("--log")) != nullptr))
   {
      logmask_from_string(p_arg, mask);
      log_set_mask(mask);
   }
   cpd.frag = arg.Present("--frag");

   if (arg.Present("--decode"))
   {
      size_t idx = 1;
      while ((p_arg = arg.Unused(idx)) != nullptr)
      {
         log_pcf_flags(LSYS, strtoul(p_arg, nullptr, 16));
      }
      return(EXIT_SUCCESS);
   }

   // Get the config file name
   string cfg_file;
   if (  ((p_arg = arg.Param("--config")) != nullptr)
      || ((p_arg = arg.Param("-c")) != nullptr))
   {
      cfg_file = p_arg;
   }
   else if (!unc_getenv("UNCRUSTIFY_CONFIG", cfg_file))
   {
      // Try to find a config file at an alternate location
      string home;

      if (unc_homedir(home))
      {
         struct stat tmp_stat = {};

         const auto  path0 = home + "/.uncrustify.cfg";
         const auto  path1 = home + "/uncrustify.cfg";

         if (stat(path0.c_str(), &tmp_stat) == 0)
         {
            cfg_file = path0;
         }
         else if (stat(path1.c_str(), &tmp_stat) == 0)
         {
            cfg_file = path1;
         }
      }
   }

   // Get the parsed file name
   const char *parsed_file;
   if (  ((parsed_file = arg.Param("--parsed")) != nullptr)
      || ((parsed_file = arg.Param("-p")) != nullptr))
   {
      LOG_FMT(LNOTE, "Will export parsed data to: %s\n", parsed_file);
   }

   // Enable log severities
   if (arg.Present("-s") || arg.Present("--show"))
   {
      log_show_sev(true);
   }

   // Load type files
   size_t idx = 0;
   while ((p_arg = arg.Params("-t", idx)) != nullptr)
   {
      load_keyword_file(p_arg);
   }

   // add types
   idx = 0;
   while ((p_arg = arg.Params("--type", idx)) != nullptr)
   {
      add_keyword(p_arg, CT_TYPE);
   }

   // Check for a language override
   if ((p_arg = arg.Param("-l")) != nullptr)
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

   // Get the source file name
   const char *source_file;
   if (  ((source_file = arg.Param("--file")) == nullptr)
      && ((source_file = arg.Param("-f")) == nullptr))
   {
      // not using a single file, source_file is nullptr
   }

   // Get a source file list
   const char *source_list;
   if (  ((source_list = arg.Param("--files")) == nullptr)
      && ((source_list = arg.Param("-F")) == nullptr))
   {
      // not using a file list, source_list is nullptr
   }

   const char *prefix = arg.Param("--prefix");
   const char *suffix = arg.Param("--suffix");
   const char *assume = arg.Param("--assume");

   bool       no_backup        = arg.Present("--no-backup");
   bool       replace          = arg.Present("--replace");
   bool       keep_mtime       = arg.Present("--mtime");
   bool       update_config    = arg.Present("--update-config");
   bool       update_config_wd = arg.Present("--update-config-with-doc");
   bool       detect           = arg.Present("--detect");

   // Grab the output override
   const char *output_file = arg.Param("-o");

   LOG_FMT(LDATA, "config_file = %s\n", cfg_file.c_str());
   LOG_FMT(LDATA, "output_file = %s\n", (output_file != NULL) ? output_file : "null");
   LOG_FMT(LDATA, "source_file = %s\n", (source_file != NULL) ? source_file : "null");
   LOG_FMT(LDATA, "source_list = %s\n", (source_list != NULL) ? source_list : "null");
   LOG_FMT(LDATA, "prefix      = %s\n", (prefix != NULL) ? prefix : "null");
   LOG_FMT(LDATA, "suffix      = %s\n", (suffix != NULL) ? suffix : "null");
   LOG_FMT(LDATA, "assume      = %s\n", (assume != NULL) ? assume : "null");
   LOG_FMT(LDATA, "replace     = %d\n", replace);
   LOG_FMT(LDATA, "no_backup   = %d\n", no_backup);
   LOG_FMT(LDATA, "detect      = %d\n", detect);
   LOG_FMT(LDATA, "check       = %d\n", cpd.do_check);
   LOG_FMT(LDATA, "if_changed  = %d\n", cpd.if_changed);

   if (  cpd.do_check
      && (  output_file
         || replace
         || no_backup
         || keep_mtime
         || update_config
         || update_config_wd
         || detect
         || prefix
         || suffix
         || cpd.if_changed))
   {
      usage_error("Cannot use --check with output options.");
      return(EX_NOUSER);
   }

   if (!cpd.do_check)
   {
      if (replace || no_backup)
      {
         if (prefix != nullptr || suffix != nullptr)
         {
            usage_error("Cannot use --replace with --prefix or --suffix");
            return(EX_NOINPUT);
         }
         if (source_file != nullptr || output_file != nullptr)
         {
            usage_error("Cannot use --replace or --no-backup with -f or -o");
            return(EX_NOINPUT);
         }
      }
      else
      {
         if (prefix == nullptr && suffix == nullptr)
         {
            suffix = ".uncrustify";
         }
      }
   }

   /*
    * Try to load the config file, if available.
    * It is optional for "--universalindent" and "--detect", but required for
    * everything else.
    */
   if (!cfg_file.empty())
   {
      cpd.filename = cfg_file;
      if (!load_option_file(cpd.filename.c_str()))
      {
         usage_error("Unable to load the config file");
         return(EX_IOERR);
      }
      // test if all options are compatible to each other
      if (options::nl_max() > 0)
      {
         // test if one/some option(s) is/are not too big for that
         if (options::nl_func_var_def_blk() >= options::nl_max())
         {
            fprintf(stderr, "The option 'nl_func_var_def_blk' is too big against the option 'nl_max'\n");
            log_flush(true);
            exit(EX_CONFIG);
         }
      }
   }

   // Set config options using command line arguments.
   idx = 0;
   while ((p_arg = arg.Params("--set", idx)) != nullptr)
   {
      size_t argLength = strlen(p_arg);
#define MAXLENGTHFORARG    256
      if (argLength > MAXLENGTHFORARG)
      {
         fprintf(stderr, "The buffer is to short for the set argument '%s'\n", p_arg);
         log_flush(true);
         exit(EX_SOFTWARE);
      }
      char buffer[MAXLENGTHFORARG];
      strcpy(buffer, p_arg);

      // Tokenize and extract key and value
      const char *token  = strtok(buffer, "=");
      const char *option = token;

      token = strtok(nullptr, "=");
      const char *value = token;

      if (  option != nullptr
         && value != nullptr
         && strtok(nullptr, "=") == nullptr)   // end of argument reached
      {
         if (auto *opt = uncrustify::find_option(option))
         {
            if (!opt->read(value))
            {
               return(EXIT_FAILURE);
            }
         }
         else
         {
            fprintf(stderr, "Unknown option '%s' to override.\n", buffer);
            log_flush(true);
            return(EXIT_FAILURE);
         }
      }
      else
      {
         // TODO: consider using defines like EX_USAGE from sysexits.h
         usage_error("Error while parsing --set");
         return(EX_USAGE);
      }
   }

   if (arg.Present("--universalindent"))
   {
      FILE *pfile = stdout;

      if (output_file != nullptr)
      {
         pfile = fopen(output_file, "w");
         if (pfile == nullptr)
         {
            fprintf(stderr, "Unable to open %s for write: %s (%d)\n",
                    output_file, strerror(errno), errno);
            log_flush(true);
            return(EXIT_FAILURE);
         }
      }

      print_universal_indent_cfg(pfile);
      fclose(pfile);

      return(EXIT_SUCCESS);
   }

   if (detect)
   {
      file_mem fm;

      if (source_file == nullptr || source_list != nullptr)
      {
         fprintf(stderr, "The --detect option requires a single input file\n");
         log_flush(true);
         return(EXIT_FAILURE);
      }

      // Do some simple language detection based on the filename extension
      if (!cpd.lang_forced || cpd.lang_flags == 0)
      {
         cpd.lang_flags = language_flags_from_filename(source_file);
      }

      // Try to read in the source file
      if (load_mem_file(source_file, fm) < 0)
      {
         LOG_FMT(LERR, "Failed to load (%s)\n", source_file);
         cpd.error_count++;
         return(EXIT_FAILURE);
      }

      uncrustify_start(fm.data);
      detect_options();
      uncrustify_end();

      if (auto error = redir_stdout(output_file))
      {
         return(error);
      }
      save_option_file(stdout, update_config_wd);
      return(EXIT_SUCCESS);
   }

   if (update_config || update_config_wd)
   {
      // TODO: complain if file-processing related options are present
      if (auto error = redir_stdout(output_file))
      {
         return(error);
      }
      save_option_file(stdout, update_config_wd);
      return(EXIT_SUCCESS);
   }

   /*
    * Everything beyond this point requires a config file, so complain and
    * bail if we don't have one.
    */
   if (cfg_file.empty())
   {
      usage_error("Specify the config file with '-c file' or set UNCRUSTIFY_CONFIG");
      return(EX_IOERR);
   }

   // Done parsing args

   // Check for unused args (ignore them)
   idx   = 1;
   p_arg = arg.Unused(idx);

   // Check args - for multifile options
   if (source_list != nullptr || p_arg != nullptr)
   {
      if (source_file != nullptr)
      {
         usage_error("Cannot specify both the single file option and a multi-file option.");
         return(EX_NOUSER);
      }

      if (output_file != nullptr)
      {
         usage_error("Cannot specify -o with a multi-file option.");
         return(EX_NOHOST);
      }
   }

   // This relies on cpd.filename being the config file name
   load_header_files();

   if (cpd.do_check || cpd.if_changed)
   {
      cpd.bout = new deque<UINT8>();
   }

   if (  source_file == nullptr
      && source_list == nullptr
      && p_arg == nullptr)
   {
      // no input specified, so use stdin
      if (cpd.lang_flags == 0)
      {
         if (assume != nullptr)
         {
            cpd.lang_flags = language_flags_from_filename(assume);
         }
         else
         {
            cpd.lang_flags = LANG_C;
         }
      }

      if (!cpd.do_check)
      {
         if (auto error = redir_stdout(output_file))
         {
            return(error);
         }
      }

      file_mem fm;
      if (!read_stdin(fm))
      {
         LOG_FMT(LERR, "Failed to read stdin\n");
         cpd.error_count++;
         return(100);
      }

      cpd.filename = "stdin";

      // Done reading from stdin
      LOG_FMT(LSYS, "Parsing: %d bytes (%d chars) from stdin as language %s\n",
              (int)fm.raw.size(), (int)fm.data.size(),
              language_name_from_flags(cpd.lang_flags));

      uncrustify_file(fm, stdout, parsed_file);
   }
   else if (source_file != nullptr)
   {
      // Doing a single file
      do_source_file(source_file, output_file, parsed_file, no_backup, keep_mtime);
   }
   else
   {
      if (parsed_file != nullptr)  // Issue #930
      {
         fprintf(stderr, "FAIL: -p option must be used with the -f option\n");
         log_flush(true);
         exit(EX_CONFIG);
      }
      // Doing multiple files, TODO: multiple threads for parallel processing
      if (prefix != nullptr)
      {
         LOG_FMT(LSYS, "Output prefix: %s/\n", prefix);
      }
      if (suffix != nullptr)
      {
         LOG_FMT(LSYS, "Output suffix: %s\n", suffix);
      }

      // Do the files on the command line first
      idx = 1;
      while ((p_arg = arg.Unused(idx)) != nullptr)
      {
         char outbuf[1024];
         do_source_file(p_arg,
                        make_output_filename(outbuf, sizeof(outbuf), p_arg, prefix, suffix),
                        nullptr, no_backup, keep_mtime);
      }

      if (source_list != nullptr)
      {
         process_source_list(source_list, prefix, suffix, no_backup, keep_mtime);
      }
   }

   clear_keyword_file();

   if (cpd.error_count != 0)
   {
      return(EXIT_FAILURE);
   }
   if (cpd.do_check && cpd.check_fail_cnt != 0)
   {
      return(EXIT_FAILURE);
   }

   return(EXIT_SUCCESS);
} // main


static void process_source_list(const char *source_list,
                                const char *prefix, const char *suffix,
                                bool no_backup, bool keep_mtime)
{
   int  from_stdin = strcmp(source_list, "-") == 0;
   FILE *p_file    = from_stdin ? stdin : fopen(source_list, "r");

   if (p_file == nullptr)
   {
      LOG_FMT(LERR, "%s: fopen(%s) failed: %s (%d)\n",
              __func__, source_list, strerror(errno), errno);
      cpd.error_count++;
      return;
   }

   char linebuf[256];
   int  line = 0;

   while (fgets(linebuf, sizeof(linebuf), p_file) != nullptr)
   {
      line++;
      char *fname = linebuf;
      int  len    = strlen(fname);
      while (len > 0 && unc_isspace(*fname))
      {
         fname++;
         len--;
      }
      while (len > 0 && unc_isspace(fname[len - 1]))
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
                        nullptr, no_backup, keep_mtime);
      }
   }

   if (!from_stdin)
   {
      fclose(p_file);
   }
} // process_source_list


static bool read_stdin(file_mem &fm)
{
   deque<UINT8> dq;
   char         buf[4096];

   fm.raw.clear();
   fm.data.clear();
   fm.enc = char_encoding_e::e_ASCII;

   while (!feof(stdin))
   {
      int len = fread(buf, 1, sizeof(buf), stdin);
      for (int idx = 0; idx < len; idx++)
      {
         dq.push_back(buf[idx]);
      }
   }

   // Copy the raw data from the deque to the vector
   fm.raw.insert(fm.raw.end(), dq.begin(), dq.end());
   return(decode_unicode(fm.raw, fm.data, fm.enc, fm.bom));
}


static void make_folders(const string &filename)
{
   int  last_idx = 0;
   char outname[4096];

   snprintf(outname, sizeof(outname), "%s", filename.c_str());

   for (int idx = 0; outname[idx] != 0; idx++)
   {
      if ((outname[idx] == '/') || (outname[idx] == '\\'))
      {
         outname[idx] = PATH_SEP;
      }

      // search until end of subpath is found
      if (idx > last_idx && (outname[idx] == PATH_SEP))
      {
         outname[idx] = 0; // mark the end of the subpath

         // create subfolder if it is not the start symbol of a path
         // and not a Windows drive letter
         if (  (strcmp(&outname[last_idx], ".") != 0)
            && (strcmp(&outname[last_idx], "..") != 0)
            && (!(last_idx == 0 && idx == 2 && outname[1] == ':')))
         {
            int status;    // Coverity CID 75999
            status = mkdir(outname, 0750);
            if (status != 0 && errno != EEXIST)
            {
               LOG_FMT(LERR, "%s: Unable to create %s: %s (%d)\n",
                       __func__, outname, strerror(errno), errno);
               cpd.error_count++;
               return;
            }
         }
         outname[idx] = PATH_SEP; // reconstruct full path to search for next subpath
      }

      if (outname[idx] == PATH_SEP)
      {
         last_idx = idx + 1;
      }
   }
} // make_folders


static int load_mem_file(const char *filename, file_mem &fm)
{
   int         retval = -1;
   struct stat my_stat;
   FILE        *p_file;

   fm.raw.clear();
   fm.data.clear();
   fm.enc = char_encoding_e::e_ASCII;

   // Grab the stat info for the file, return if it cannot be read
   if (stat(filename, &my_stat) < 0)
   {
      return(-1);
   }

#ifdef HAVE_UTIME_H
   // Save off modification time (mtime)
   fm.utb.modtime = my_stat.st_mtime;
#endif

   // Try to read in the file
   p_file = fopen(filename, "rb");
   if (p_file == nullptr)
   {
      return(-1);
   }

   fm.raw.resize(my_stat.st_size);
   if (my_stat.st_size == 0) // check if file is empty
   {
      retval = 0;
      fm.bom = false;
      fm.enc = char_encoding_e::e_ASCII;
      fm.data.clear();
   }
   else
   {
      // read the raw data
      if (fread(&fm.raw[0], fm.raw.size(), 1, p_file) != 1)
      {
         LOG_FMT(LERR, "%s: fread(%s) failed: %s (%d)\n",
                 __func__, filename, strerror(errno), errno);
         cpd.error_count++;
      }
      else if (!decode_unicode(fm.raw, fm.data, fm.enc, fm.bom))
      {
         LOG_FMT(LERR, "%s: failed to decode the file '%s'\n", __func__, filename);
         cpd.error_count++;
      }
      else
      {
         LOG_FMT(LNOTE, "%s: '%s' encoding looks like %s (%d)\n", __func__, filename,
                 fm.enc == char_encoding_e::e_ASCII ? "ASCII" :
                 fm.enc == char_encoding_e::e_BYTE ? "BYTES" :
                 fm.enc == char_encoding_e::e_UTF8 ? "UTF-8" :
                 fm.enc == char_encoding_e::e_UTF16_LE ? "UTF-16-LE" :
                 fm.enc == char_encoding_e::e_UTF16_BE ? "UTF-16-BE" : "Error",
                 (int)fm.enc);
         retval = 0;
      }
   }
   fclose(p_file);
   return(retval);
} // load_mem_file


static int load_mem_file_config(const std::string &filename, file_mem &fm)
{
   int  retval;
   char buf[1024];

   snprintf(buf, sizeof(buf), "%.*s%s",
            path_dirname_len(cpd.filename.c_str()), cpd.filename.c_str(), filename.c_str());

   retval = load_mem_file(buf, fm);
   if (retval < 0)
   {
      retval = load_mem_file(filename.c_str(), fm);
      if (retval < 0)
      {
         LOG_FMT(LERR, "Failed to load (%s) or (%s)\n", buf, filename.c_str());
         cpd.error_count++;
      }
   }
   return(retval);
}


int load_header_files()
{
   int retval = 0;

   if (!options::cmt_insert_file_header().empty())
   {
      // try to load the file referred to by the options string
      retval |= load_mem_file_config(options::cmt_insert_file_header(),
                                     cpd.file_hdr);
   }
   if (!options::cmt_insert_file_footer().empty())
   {
      retval |= load_mem_file_config(options::cmt_insert_file_footer(),
                                     cpd.file_ftr);
   }
   if (!options::cmt_insert_func_header().empty())
   {
      retval |= load_mem_file_config(options::cmt_insert_func_header(),
                                     cpd.func_hdr);
   }
   if (!options::cmt_insert_class_header().empty())
   {
      retval |= load_mem_file_config(options::cmt_insert_class_header(),
                                     cpd.class_hdr);
   }
   if (!options::cmt_insert_oc_msg_header().empty())
   {
      retval |= load_mem_file_config(options::cmt_insert_oc_msg_header(),
                                     cpd.oc_msg_hdr);
   }
   return(retval);
}


static const char *make_output_filename(char *buf, size_t buf_size,
                                        const char *filename,
                                        const char *prefix,
                                        const char *suffix)
{
   int len = 0;

   if (prefix != nullptr)
   {
      len = snprintf(buf, buf_size, "%s/", prefix);
   }

   snprintf(&buf[len], buf_size - len, "%s%s", filename,
            (suffix != nullptr) ? suffix : "");

   return(buf);
}


static bool file_content_matches(const string &filename1, const string &filename2)
{
   struct stat st1, st2;
   int         fd1, fd2;

   // Check the file sizes first
   if (  (stat(filename1.c_str(), &st1) != 0)
      || (stat(filename2.c_str(), &st2) != 0)
      || st1.st_size != st2.st_size)
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

   int   len1 = 0;
   int   len2 = 0;
   UINT8 buf1[1024];
   UINT8 buf2[1024];
   memset(buf1, 0, sizeof(buf1));
   memset(buf2, 0, sizeof(buf2));
   while (len1 >= 0 && len2 >= 0)
   {
      if (len1 == 0)
      {
         len1 = read(fd1, buf1, sizeof(buf1));
      }
      if (len2 == 0)
      {
         len2 = read(fd2, buf2, sizeof(buf2));
      }
      if (len1 <= 0 || len2 <= 0)
      {
         break; // reached end of either files
         // TODO: what is if one file is longer than the other, do we miss that ?
      }
      int minlen = (len1 < len2) ? len1 : len2;
      if (memcmp(buf1, buf2, minlen) != 0)
      {
         break; // found a difference
      }
      len1 -= minlen;
      len2 -= minlen;
   }

   close(fd1);
   close(fd2);

   return(len1 == 0 && len2 == 0);
} // file_content_matches


static string fix_filename(const char *filename)
{
   char   *tmp_file;
   string rv;

   // Create 'outfile.uncrustify'
   tmp_file = new char[strlen(filename) + 16 + 1]; // + 1 for '//  + 1 for '/* + 1 for '\0' */' '
   if (tmp_file != nullptr)
   {
      sprintf(tmp_file, "%s.uncrustify", filename);
   }
   rv = tmp_file;
   delete[] tmp_file;
   return(rv);
}


static bool bout_content_matches(const file_mem &fm, bool report_status)
{
   bool is_same = true;

   // compare the old data vs the new data
   if (cpd.bout->size() != fm.raw.size())
   {
      if (report_status)
      {
         fprintf(stderr, "FAIL: %s (File size changed from %u to %u)\n",
                 cpd.filename.c_str(), static_cast<int>(fm.raw.size()),
                 static_cast<int>(cpd.bout->size()));
         log_flush(true);
      }
      is_same = false;
   }
   else
   {
      for (int idx = 0; idx < static_cast<int>(fm.raw.size()); idx++)
      {
         if (fm.raw[idx] != (*cpd.bout)[idx])
         {
            if (report_status)
            {
               fprintf(stderr, "FAIL: %s (Difference at byte %u)\n",
                       cpd.filename.c_str(), idx);
               log_flush(true);
            }
            is_same = false;
            break;
         }
      }
   }
   if (is_same && report_status)
   {
      fprintf(stdout, "PASS: %s (%u bytes)\n",
              cpd.filename.c_str(), static_cast<int>(fm.raw.size()));
   }

   return(is_same);
}


static void do_source_file(const char *filename_in,
                           const char *filename_out,
                           const char *parsed_file,
                           bool       no_backup,
                           bool       keep_mtime)
{
   FILE     *pfout      = nullptr;
   bool     did_open    = false;
   bool     need_backup = false;
   file_mem fm;
   string   filename_tmp;

   // Do some simple language detection based on the filename extension
   if (!cpd.lang_forced || cpd.lang_flags == 0)
   {
      cpd.lang_flags = language_flags_from_filename(filename_in);
   }

   // Try to read in the source file
   if (load_mem_file(filename_in, fm) < 0)
   {
      LOG_FMT(LERR, "Failed to load (%s)\n", filename_in);
      cpd.error_count++;
      return;
   }

   LOG_FMT(LSYS, "Parsing: %s as language %s\n",
           filename_in, language_name_from_flags(cpd.lang_flags));

   cpd.filename = filename_in;

   /*
    * If we're only going to write on an actual change, then build the output
    * buffer now and if there were changes, run it through the normal file
    * write path.
    *
    * Future: many code paths could be simplified if 'bout' were always used and not
    * optionally selected in just for do_check and if_changed.
    */
   if (cpd.if_changed)
   {
      /*
       * Cleanup is deferred because we need 'bout' preserved long enough
       * to write it to a file (if it changed).
       */
      uncrustify_file(fm, nullptr, parsed_file, true);
      if (bout_content_matches(fm, false))
      {
         uncrustify_end();
         return;
      }
   }

   if (!cpd.do_check)
   {
      if (filename_out == nullptr)
      {
         pfout = stdout;
      }
      else
      {
         // If the out file is the same as the in file, then use a temp file
         filename_tmp = filename_out;
         if (strcmp(filename_in, filename_out) == 0)
         {
            // Create 'outfile.uncrustify'
            filename_tmp = fix_filename(filename_out);

            if (!no_backup)
            {
               if (backup_copy_file(filename_in, fm.raw) != EX_OK)
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
         if (pfout == nullptr)
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

   if (cpd.if_changed)
   {
      for (deque<UINT8>::const_iterator i = cpd.bout->begin(), end = cpd.bout->end(); i != end; ++i)
      {
         fputc(*i, pfout);
      }
      uncrustify_end();
   }
   else
   {
      uncrustify_file(fm, pfout, parsed_file);
   }

   if (did_open)
   {
      fclose(pfout);

      if (need_backup)
      {
         backup_create_md5_file(filename_in);
      }

      if (filename_tmp != filename_out)
      {
         // We need to compare and then do a rename (but avoid redundant test when if_changed set)
         if (!cpd.if_changed && file_content_matches(filename_tmp, filename_out))
         {
            // No change - remove tmp file
            UNUSED(unlink(filename_tmp.c_str()));
         }
         else
         {
            // Change - rename filename_tmp to filename_out

#ifdef WIN32
            /*
             * Atomic rename in windows can't go through stdio rename() func because underneath
             * it calls MoveFileExW without MOVEFILE_REPLACE_EXISTING.
             */
            if (!MoveFileEx(filename_tmp.c_str(), filename_out, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED))
#else
            if (rename(filename_tmp.c_str(), filename_out) != 0)
#endif
            {
               LOG_FMT(LERR, "%s: Unable to rename '%s' to '%s'\n",
                       __func__, filename_tmp.c_str(), filename_out);
               cpd.error_count++;
            }
         }
      }

      if (keep_mtime)
      {
#ifdef HAVE_UTIME_H
         // update mtime -- don't care if it fails
         fm.utb.actime = time(nullptr);
         UNUSED(utime(filename_in, &fm.utb));
#endif
      }
   }
} // do_source_file


static void add_file_header()
{
   if (!chunk_is_comment(chunk_get_head()))
   {
      // TODO: detect the typical #ifndef FOO / #define FOO sequence
      tokenize(cpd.file_hdr.data, chunk_get_head());
   }
}


static void add_file_footer()
{
   chunk_t *pc = chunk_get_tail();

   // Back up if the file ends with a newline
   if (pc != nullptr && chunk_is_newline(pc))
   {
      pc = chunk_get_prev(pc);
   }
   if (  pc != nullptr
      && (!chunk_is_comment(pc) || !chunk_is_newline(chunk_get_prev(pc))))
   {
      pc = chunk_get_tail();
      if (!chunk_is_newline(pc))
      {
         LOG_FMT(LSYS, "Adding a newline at the end of the file\n");
         newline_add_after(pc);
      }
      tokenize(cpd.file_ftr.data, nullptr);
   }
}


static void add_func_header(c_token_t type, file_mem &fm)
{
   chunk_t *pc;
   chunk_t *ref;
   chunk_t *tmp;
   bool    do_insert;

   for (pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next_ncnlnp(pc))
   {
      if (pc->type != type)
      {
         continue;
      }
      if (  (pc->flags & PCF_IN_CLASS)
         && !options::cmt_insert_before_inlines())
      {
         continue;
      }

      // Check for one liners for classes. Declarations only. Walk down the chunks.
      ref = pc;

      if (  chunk_is_token(ref, CT_CLASS)
         && ref->parent_type == CT_NONE
         && ref->next)
      {
         ref = ref->next;
         if (  chunk_is_token(ref, CT_TYPE)
            && ref->parent_type == type
            && ref->next)
         {
            ref = ref->next;
            if (chunk_is_token(ref, CT_SEMICOLON) && ref->parent_type == CT_NONE)
            {
               continue;
            }
         }
      }

      // Check for one liners for functions. There'll be a closing brace w/o any newlines. Walk down the chunks.
      ref = pc;

      if (  chunk_is_token(ref, CT_FUNC_DEF)
         && ref->parent_type == CT_NONE
         && ref->next)
      {
         int found_brace = 0;                                 // Set if a close brace is found before a newline
         while (ref->type != CT_NEWLINE && (ref = ref->next)) // TODO: is the assignment of ref wanted here?, better move it to the loop
         {
            if (chunk_is_token(ref, CT_BRACE_CLOSE))
            {
               found_brace = 1;
               break;
            }
         }
         if (found_brace)
         {
            continue;
         }
      }

      do_insert = false;

      /*
       * On a function proto or def. Back up to a close brace or semicolon on
       * the same level
       */
      ref = pc;
      while ((ref = chunk_get_prev(ref)) != nullptr)
      {
         // Bail if we change level or find an access specifier colon
         if (ref->level != pc->level || chunk_is_token(ref, CT_PRIVATE_COLON))
         {
            do_insert = true;
            break;
         }

         // If we hit an angle close, back up to the angle open
         if (chunk_is_token(ref, CT_ANGLE_CLOSE))
         {
            ref = chunk_get_prev_type(ref, CT_ANGLE_OPEN, ref->level, scope_e::PREPROC);
            continue;
         }

         // Bail if we hit a preprocessor and cmt_insert_before_preproc is false
         if (ref->flags & PCF_IN_PREPROC)
         {
            tmp = chunk_get_prev_type(ref, CT_PREPROC, ref->level);
            if (tmp != nullptr && tmp->parent_type == CT_PP_IF)
            {
               tmp = chunk_get_prev_nnl(tmp);
               if (  chunk_is_comment(tmp)
                  && !options::cmt_insert_before_preproc())
               {
                  break;
               }
            }
         }

         // Ignore 'right' comments
         if (chunk_is_comment(ref) && chunk_is_newline(chunk_get_prev(ref)))
         {
            break;
         }

         if (  ref->level == pc->level
            && (  (ref->flags & PCF_IN_PREPROC)
               || chunk_is_token(ref, CT_SEMICOLON)
               || chunk_is_token(ref, CT_BRACE_CLOSE)))
         {
            do_insert = true;
            break;
         }
      }
      if (do_insert)
      {
         // Insert between after and ref
         chunk_t *after = chunk_get_next_ncnl(ref);
         tokenize(fm.data, after);
         for (tmp = chunk_get_next(ref); tmp != after; tmp = chunk_get_next(tmp))
         {
            tmp->level = after->level;
         }
      }
   }
} // add_func_header


static void add_msg_header(c_token_t type, file_mem &fm)
{
   chunk_t *pc;
   chunk_t *ref;
   chunk_t *tmp;
   bool    do_insert;

   for (pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next_ncnlnp(pc))
   {
      if (pc->type != type)
      {
         continue;
      }

      do_insert = false;

      /*
       * On a message declaration back up to a Objective-C scope
       * the same level
       */
      ref = pc;
      while ((ref = chunk_get_prev(ref)) != nullptr)
      {
         // ignore the CT_TYPE token that is the result type
         if (  ref->level != pc->level
            && (chunk_is_token(ref, CT_TYPE) || chunk_is_token(ref, CT_PTR_TYPE)))
         {
            continue;
         }

         // If we hit a parentheses around return type, back up to the open parentheses
         if (chunk_is_token(ref, CT_PAREN_CLOSE))
         {
            ref = chunk_get_prev_type(ref, CT_PAREN_OPEN, ref->level, scope_e::PREPROC);
            continue;
         }

         // Bail if we hit a preprocessor and cmt_insert_before_preproc is false
         if (ref->flags & PCF_IN_PREPROC)
         {
            tmp = chunk_get_prev_type(ref, CT_PREPROC, ref->level);
            if (tmp != nullptr && tmp->parent_type == CT_PP_IF)
            {
               tmp = chunk_get_prev_nnl(tmp);
               if (  chunk_is_comment(tmp)
                  && !options::cmt_insert_before_preproc())
               {
                  break;
               }
            }
         }
         if (  ref->level == pc->level
            && ((ref->flags & PCF_IN_PREPROC) || chunk_is_token(ref, CT_OC_SCOPE)))
         {
            ref = chunk_get_prev(ref);
            if (ref != nullptr)
            {
               // Ignore 'right' comments
               if (chunk_is_newline(ref) && chunk_is_comment(chunk_get_prev(ref)))
               {
                  break;
               }
               do_insert = true;
            }
            break;
         }
      }

      if (do_insert)
      {
         // Insert between after and ref
         chunk_t *after = chunk_get_next_ncnl(ref);
         tokenize(fm.data, after);
         for (tmp = chunk_get_next(ref); tmp != after; tmp = chunk_get_next(tmp))
         {
            tmp->level = after->level;
         }
      }
   }
} // add_msg_header


static void uncrustify_start(const deque<int> &data)
{
   // Parse the text into chunks
   tokenize(data, nullptr);

   cpd.unc_stage = unc_stage_e::HEADER;

   // Get the column for the fragment indent
   if (cpd.frag)
   {
      chunk_t *pc = chunk_get_head();

      cpd.frag_cols = (pc != nullptr) ? pc->orig_col : 0;
   }

   // Add the file header
   if (!cpd.file_hdr.data.empty())
   {
      add_file_header();
   }

   // Add the file footer
   if (!cpd.file_ftr.data.empty())
   {
      add_file_footer();
   }

   /*
    * Change certain token types based on simple sequence.
    * Example: change '[' + ']' to '[]'
    * Note that level info is not yet available, so it is OK to do all
    * processing that doesn't need to know level info. (that's very little!)
    */
   tokenize_cleanup();

   /*
    * Detect the brace and paren levels and insert virtual braces.
    * This handles all that nasty preprocessor stuff
    */
   brace_cleanup();

   // At this point, the level information is available and accurate.

   if (language_is_set(LANG_PAWN))
   {
      pawn_prescan();
   }

   // Re-type chunks, combine chunks
   fix_symbols();

   mark_comments();

   // Look at all colons ':' and mark labels, :? sequences, etc.
   combine_labels();

   enum_cleanup();
} // uncrustify_start


void uncrustify_file(const file_mem &fm, FILE *pfout,
                     const char *parsed_file, bool defer_uncrustify_end)
{
   const deque<int> &data = fm.data;

   // Save off the encoding and whether a BOM is required
   cpd.bom = fm.bom;
   cpd.enc = fm.enc;
   if (  options::utf8_force()
      || ((cpd.enc == char_encoding_e::e_BYTE) && options::utf8_byte()))
   {
      cpd.enc = char_encoding_e::e_UTF8;
   }
   iarf_e av;
   switch (cpd.enc)
   {
   case char_encoding_e::e_UTF8:
      av = options::utf8_bom();
      break;

   case char_encoding_e::e_UTF16_LE:
   case char_encoding_e::e_UTF16_BE:
      av = IARF_FORCE;
      break;

   default:
      av = IARF_IGNORE;
      break;
   }
   if (av == IARF_REMOVE)
   {
      cpd.bom = false;
   }
   else if (av != IARF_IGNORE)
   {
      cpd.bom = true;
   }

   // Check for embedded 0's (represents a decoding failure or corrupt file)
   size_t count_line   = 1;
   size_t count_column = 1;
   for (int idx = 0; idx < static_cast<int>(data.size()) - 1; idx++)
   {
      if (data[idx] == 0)
      {
         LOG_FMT(LERR, "An embedded 0 was found in '%s' %zu:%zu.\n",
                 cpd.filename.c_str(), count_line, count_column);
         LOG_FMT(LERR, "The file may be encoded in an unsupported Unicode format.\n");
         LOG_FMT(LERR, "Aborting.\n");
         cpd.error_count++;
         return;
      }

      count_column++;
      if (data[idx] == '\n')
      {
         count_line++;
         count_column = 1;
      }
   }

   uncrustify_start(data);

   cpd.unc_stage = unc_stage_e::OTHER;

   /*
    * Done with detection. Do the rest only if the file will go somewhere.
    * The detection code needs as few changes as possible.
    */
   {
      // Add comments before function defs and classes
      if (!cpd.func_hdr.data.empty())
      {
         add_func_header(CT_FUNC_DEF, cpd.func_hdr);
         if (options::cmt_insert_before_ctor_dtor())
         {
            add_func_header(CT_FUNC_CLASS_DEF, cpd.func_hdr);
         }
      }
      if (!cpd.class_hdr.data.empty())
      {
         add_func_header(CT_CLASS, cpd.class_hdr);
      }
      if (!cpd.oc_msg_hdr.data.empty())
      {
         add_msg_header(CT_OC_MSG_DECL, cpd.oc_msg_hdr);
      }

      do_braces();  // Change virtual braces into real braces...

      // Scrub extra semicolons
      if (options::mod_remove_extra_semicolon())
      {
         remove_extra_semicolons();
      }

      // Remove unnecessary returns
      if (options::mod_remove_empty_return())
      {
         remove_extra_returns();
      }

      // Add parens
      do_parens();

      // Modify line breaks as needed
      bool first = true;
      int  old_changes;

      if (options::nl_remove_extra_newlines() == 2)
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
         if (options::nl_after_multiline_comment())
         {
            newline_after_multiline_comment();
         }
         if (options::nl_after_label_colon())
         {
            newline_after_label_colon();
         }
         newlines_insert_blank_lines();
         if (options::pos_bool() != TP_IGNORE)
         {
            newlines_chunk_pos(CT_BOOL, options::pos_bool());
         }
         if (options::pos_compare() != TP_IGNORE)
         {
            newlines_chunk_pos(CT_COMPARE, options::pos_compare());
         }
         if (options::pos_conditional() != TP_IGNORE)
         {
            newlines_chunk_pos(CT_COND_COLON, options::pos_conditional());
            newlines_chunk_pos(CT_QUESTION, options::pos_conditional());
         }
         if (options::pos_comma() != TP_IGNORE || options::pos_enum_comma() != TP_IGNORE)
         {
            newlines_chunk_pos(CT_COMMA, options::pos_comma());
         }
         if (options::pos_assign() != TP_IGNORE)
         {
            newlines_chunk_pos(CT_ASSIGN, options::pos_assign());
         }
         if (options::pos_arith() != TP_IGNORE)
         {
            newlines_chunk_pos(CT_ARITH, options::pos_arith());
            newlines_chunk_pos(CT_CARET, options::pos_arith());
         }
         newlines_class_colon_pos(CT_CLASS_COLON);
         newlines_class_colon_pos(CT_CONSTR_COLON);
         if (options::nl_squeeze_ifdef())
         {
            newlines_squeeze_ifdef();
         }
         if (options::nl_squeeze_paren_close())
         {
            newlines_squeeze_paren_close();
         }
         do_blank_lines();
         newlines_eat_start_end();
         newlines_functions_remove_extra_blank_lines();
         newlines_cleanup_dup();
         first = false;
      } while (old_changes != cpd.changes && cpd.pass_count-- > 0);

      mark_comments();

      // Add balanced spaces around nested params
      if (options::sp_balance_nested_parens())
      {
         space_text_balance_nested_parens();
      }

      // Scrub certain added semicolons
      if (language_is_set(LANG_PAWN) && options::mod_pawn_semicolon())
      {
         pawn_scrub_vsemi();
      }

      // Sort imports/using/include
      if (  options::mod_sort_import()
         || options::mod_sort_include()
         || options::mod_sort_using())
      {
         sort_imports();
      }

      // Fix same-line inter-chunk spacing
      space_text();

      // Do any aligning of preprocessors
      if (options::align_pp_define_span() > 0)
      {
         align_preprocessor();
      }

      // Indent the text
      indent_preproc();
      indent_text();

      // Insert trailing comments after certain close braces
      if (  (options::mod_add_long_switch_closebrace_comment() > 0)
         || (options::mod_add_long_function_closebrace_comment() > 0)
         || (options::mod_add_long_class_closebrace_comment() > 0)
         || (options::mod_add_long_namespace_closebrace_comment() > 0))
      {
         add_long_closebrace_comment();
      }

      // Insert trailing comments after certain preprocessor conditional blocks
      if (  (options::mod_add_long_ifdef_else_comment() > 0)
         || (options::mod_add_long_ifdef_endif_comment() > 0))
      {
         add_long_preprocessor_conditional_block_comment();
      }

      // Align everything else, reindent and break at code_width
      first = true;
      do
      {
         align_all();
         indent_text();
         old_changes = cpd.changes;
         if (options::code_width() > 0)
         {
            LOG_FMT(LNEWLINE, "%s(%d): Code_width loop start: %d\n",
                    __func__, __LINE__, cpd.changes);
            do_code_width();
            if (old_changes != cpd.changes && first)
            {
               // retry line breaks caused by splitting 1-liners
               newlines_cleanup_braces(false);
               newlines_insert_blank_lines();
               first = false;
            }
         }
      } while (old_changes != cpd.changes);

      // And finally, align the backslash newline stuff
      align_right_comments();
      if (options::align_nl_cont())
      {
         align_backslash_newline();
      }

      // Now render it all to the output file
      output_text(pfout);
   }

   // Special hook for dumping parsed data for debugging
   if (parsed_file != nullptr)
   {
      FILE *p_file = fopen(parsed_file, "wb");
      if (p_file != nullptr)
      {
         output_parsed(p_file);
         fclose(p_file);
      }
      else
      {
         LOG_FMT(LERR, "%s: Failed to open '%s' for write: %s (%d)\n",
                 __func__, parsed_file, strerror(errno), errno);
         cpd.error_count++;
      }
   }

   if (cpd.do_check && !bout_content_matches(fm, true))
   {
      cpd.check_fail_cnt++;
   }

   if (!defer_uncrustify_end)
   {
      uncrustify_end();
   }
} // uncrustify_file


void uncrustify_end()
{
   // Free all the memory
   chunk_t *pc;

   cpd.unc_stage = unc_stage_e::CLEANUP;

   while ((pc = chunk_get_head()) != nullptr)
   {
      chunk_del(pc);
   }

   if (cpd.bout)
   {
      cpd.bout->clear();
   }

   // Clean up some state variables
   cpd.unc_off     = false;
   cpd.al_cnt      = 0;
   cpd.did_newline = true;
   cpd.frames.clear();
   cpd.pp_level   = 0;
   cpd.changes    = 0;
   cpd.in_preproc = CT_NONE;
   cpd.consumed   = false;
   memset(cpd.le_counts, 0, sizeof(cpd.le_counts));
   cpd.preproc_ncnl_count                     = 0;
   cpd.ifdef_over_whole_file                  = 0;
   cpd.warned_unable_string_replace_tab_chars = false;
}


const char *get_token_name(c_token_t token)
{
   if (  token >= 0
      && (token < static_cast<int> ARRAY_SIZE(token_names))
      && (token_names[token] != nullptr))
   {
      return(token_names[token]);
   }
   return("???");
}


c_token_t find_token_name(const char *text)
{
   if (text != nullptr && (*text != 0))
   {
      for (int idx = 1; idx < static_cast<int> ARRAY_SIZE(token_names); idx++)
      {
         if (strcasecmp(text, token_names[idx]) == 0)
         {
            return(static_cast<c_token_t>(idx));
         }
      }
   }
   return(CT_NONE);
}


static bool ends_with(const char *filename, const char *tag, bool case_sensitive = true)
{
   int len1 = strlen(filename);
   int len2 = strlen(tag);

   return(  len2 <= len1
         && (  (case_sensitive && (strcmp(&filename[len1 - len2], tag) == 0))
            || (  !case_sensitive
               && (strcasecmp(&filename[len1 - len2], tag) == 0))));
}


struct lang_name_t
{
   const char *name;
   size_t     lang;
};

static lang_name_t language_names[] =
{
   { "C",        LANG_C                        },
   { "CPP",      LANG_CPP                      },
   { "D",        LANG_D                        },
   { "CS",       LANG_CS                       },
   { "VALA",     LANG_VALA                     },
   { "JAVA",     LANG_JAVA                     },
   { "PAWN",     LANG_PAWN                     },
   { "OC",       LANG_OC                       },
   { "OC+",      LANG_OC | LANG_CPP            },
   { "CS+",      LANG_CS | LANG_CPP            },
   { "ECMA",     LANG_ECMA                     },
   { "C-Header", LANG_OC | LANG_CPP | FLAG_HDR },
};


static size_t language_flags_from_name(const char *name)
{
   for (const auto &language : language_names)
   {
      if (strcasecmp(name, language.name) == 0)
      {
         return(language.lang);
      }
   }
   return(0);
}


const char *language_name_from_flags(size_t lang)
{
   // Check for an exact match first
   for (auto &language_name : language_names)
   {
      if (language_name.lang == lang)
      {
         return(language_name.name);
      }
   }

   // Check for the first set language bit
   for (auto &language_name : language_names)
   {
      if ((language_name.lang & lang) != 0)
      {
         return(language_name.name);
      }
   }
   return("???");
}


//! type to map a programming language to a typically used filename extension
struct lang_ext_t
{
   const char *ext;  //! filename extension typically used for ...
   const char *name; //! a programming language
};

//! known filename extensions linked to the corresponding programming language
struct lang_ext_t language_exts[] =
{
   { ".c",    "C"        },
   { ".cpp",  "CPP"      },
   { ".d",    "D"        },
   { ".cs",   "CS"       },
   { ".vala", "VALA"     },
   { ".java", "JAVA"     },
   { ".pawn", "PAWN"     },
   { ".p",    "PAWN"     },
   { ".sma",  "PAWN"     },
   { ".inl",  "PAWN"     },
   { ".h",    "C-Header" },
   { ".cxx",  "CPP"      },
   { ".hpp",  "CPP"      },
   { ".hxx",  "CPP"      },
   { ".cc",   "CPP"      },
   { ".cp",   "CPP"      },
   { ".C",    "CPP"      },
   { ".CPP",  "CPP"      },
   { ".c++",  "CPP"      },
   { ".di",   "D"        },
   { ".m",    "OC"       },
   { ".mm",   "OC+"      },
   { ".sqc",  "C"        },  // embedded SQL
   { ".es",   "ECMA"     },
};


const char *get_file_extension(int &idx)
{
   const char *val = nullptr;

   if (idx < static_cast<int> ARRAY_SIZE(language_exts))
   {
      val = language_exts[idx].ext;
   }
   idx++;
   return(val);
}


typedef std::map<string, string> extension_map_t;
/**
 * maps a file extension to a language flag.
 *
 * @note The "." need to be included, as in ".c". The file extensions
 *       ARE case sensitive.
 */
static extension_map_t g_ext_map;


const char *extension_add(const char *ext_text, const char *lang_text)
{
   size_t lang_flags = language_flags_from_name(lang_text);

   if (lang_flags)
   {
      const char *lang_name = language_name_from_flags(lang_flags);
      g_ext_map[string(ext_text)] = lang_name;
      return(lang_name);
   }
   return(nullptr);
}


void print_extensions(FILE *pfile)
{
   for (auto &language : language_names)
   {
      bool did_one = false;
      for (auto &extension_val : g_ext_map)
      {
         if (strcmp(extension_val.second.c_str(), language.name) == 0)
         {
            if (!did_one)
            {
               fprintf(pfile, "file_ext %s", extension_val.second.c_str());
               did_one = true;
            }
            fprintf(pfile, " %s", extension_val.first.c_str());
         }
      }

      if (did_one)
      {
         fprintf(pfile, "\n");
      }
   }
}


// TODO: better use enum lang_t for source file language
static size_t language_flags_from_filename(const char *filename)
{
   // check custom extensions first
   for (const auto &extension_val : g_ext_map)
   {
      if (ends_with(filename, extension_val.first.c_str()))
      {
         return(language_flags_from_name(extension_val.second.c_str()));
      }
   }

   for (auto &lanugage : language_exts)
   {
      if (ends_with(filename, lanugage.ext))
      {
         return(language_flags_from_name(lanugage.name));
      }
   }

   // check again without case sensitivity
   for (auto &extension_val : g_ext_map)
   {
      if (ends_with(filename, extension_val.first.c_str(), false))
      {
         return(language_flags_from_name(extension_val.second.c_str()));
      }
   }
   for (auto &lanugage : language_exts)
   {
      if (ends_with(filename, lanugage.ext, false))
      {
         return(language_flags_from_name(lanugage.name));
      }
   }
   return(LANG_C);
}


void log_pcf_flags(log_sev_t sev, UINT64 flags)
{
   if (!log_sev_on(sev))
   {
      return;
   }

   log_fmt(sev, "[0x%" PRIx64 ":", flags);

   const char *tolog = nullptr;
   for (size_t i = 0; i < ARRAY_SIZE(pcf_names); i++)
   {
      if (flags & (1ULL << i))
      {
         if (tolog != nullptr)
         {
            log_str(sev, tolog, strlen(tolog));
            log_str(sev, ",", 1);
         }
         tolog = pcf_names[i];
      }
   }

   if (tolog != nullptr)
   {
      log_str(sev, tolog, strlen(tolog));
   }

   log_str(sev, "]\n", 2);
}
