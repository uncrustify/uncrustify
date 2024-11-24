/**
 * @file uncrustify.cpp
 * This file takes an input C/C++/D/Java file and reformats it.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#define DEFINE_CHAR_TABLE

#include "uncrustify.h"

#include "align/align.h"
#include "align/nl_cont.h"
#include "align/preprocessor.h"
#include "align/trailing_comments.h"
#include "args.h"
#include "backup.h"
#include "braces.h"
#include "change_int_types.h"
#include "compat.h"
#include "detect.h"
#include "indent.h"
#include "keywords.h"
#include "lang_pawn.h"
#include "language_names.h"
#include "newlines/add.h"
#include "newlines/after.h"
#include "newlines/annotations.h"
#include "newlines/blank_line.h"
#include "newlines/chunk_pos.h"
#include "newlines/class_colon_pos.h"
#include "newlines/cleanup.h"
#include "newlines/eat_start_end.h"
#include "newlines/functions_remove_extra_blank_lines.h"
#include "newlines/remove.h"
#include "newlines/sparens.h"
#include "newlines/squeeze.h"
#include "output.h"
#include "parens.h"
#include "parent_for_pp.h"
#include "remove_duplicate_include.h"
#include "remove_extra_returns.h"
#include "rewrite_infinite_loops.h"
#include "semicolons.h"
#include "sorting.h"
#include "space.h"
#include "token_names.h"
#include "tokenizer/brace_cleanup.h"
#include "tokenizer/combine.h"
#include "tokenizer/enum_cleanup.h"
#include "tokenizer/mark_functor.h"
#include "tokenizer/mark_question_colon.h"
#include "tokenizer/parameter_pack_cleanup.h"
#include "tokenizer/tokenize.h"
#include "tokenizer/tokenize_cleanup.h"
#include "too_big_for_nl_max.h"
#include "unc_ctype.h"
#include "unc_tools.h"
#include "uncrustify_version.h"
#include "unicode.h"
#include "universalindentgui.h"
#include "width.h"

#include <cerrno>
#include <fcntl.h>
#include <map>
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


constexpr static auto LCURRENT = LUNC;


using namespace std;
using namespace uncrustify;


// Global data
cp_data_t cpd;


/**
 * Find the language for the file extension
 * Defaults to C
 *
 * @param filename   The name of the file
 * @return           LANG_xxx
 */
//static size_t language_flags_from_filename(const char *filename);


static bool read_stdin(file_mem &fm);


static void uncrustify_start(const deque<int> &data);


/**
 * Does a source file.
 *
 * @param filename_in  the file to read
 * @param filename_out nullptr (stdout) or the file to write
 * @param parsed_file  nullptr or the filename for the parsed debug info
 * @param dump_file    nullptr or the filename prefix for dumping formatting steps debug info
 * @param no_backup    don't create a backup, if filename_out == filename_in
 * @param keep_mtime   don't change the mtime (dangerous)
 * @param is_quiet     whether output should be quiet
 */
static void do_source_file(const char *filename_in, const char *filename_out, const char *parsed_file, const char *dump_file, bool no_backup, bool keep_mtime, bool is_quiet);


static void add_file_header();


static void add_file_footer();


static void add_func_header(E_Token type, file_mem &fm);


static void add_msg_header(E_Token type, file_mem &fm);


static void process_source_list(const char *source_list, const char *prefix, const char *suffix, bool no_backup, bool keep_mtime, bool is_quiet);


static const char *make_output_filename(char *buf, size_t buf_size, const char *filename, const char *prefix, const char *suffix);


//! compare the content of two files
static bool file_content_matches(const string &filename1, const string &filename2);


static bool bout_content_matches(const file_mem &fm, bool report_status, bool is_quiet);


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
static void version_exit();


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
      if (  (ch == '/')
         || (ch == '\\'))
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


static void tease()
{
   fprintf(stdout,
           "There are currently %zu options and minimal documentation.\n"
           "Try UniversalIndentGUI and good luck.\n", get_option_count());
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
           "The '-f' and '-o' options may not be used with '-F' or '--replace'.\n"
           "The '--prefix' and '--suffix' options may not be used with '--replace'.\n"
           "\n"
           "Basic Options:\n"
           " -c CFG       : Use the config file CFG, or defaults if CFG is set to '-'.\n"
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
           " --no-backup  : Do not create backup and md5 files. Useful if files are under source control.\n"
           " --if-changed : Write to stdout (or create output FILE) only if a change was detected.\n"
#ifdef HAVE_UTIME_H
           " --mtime      : Preserve mtime on replaced files.\n"
#endif
           " -l           : Language override: C, CPP, D, CS, JAVA, PAWN, OC, OC+, VALA.\n"
           " -t           : Load a file with types (usually not needed).\n"
           " -q           : Quiet mode - no output on stderr (-L will override).\n"
           "                refers to blocking logging info from being sent to stderr.\n"
           " --frag       : Code fragment, assume the first line is indented correctly.\n"
           " --assume FN  : Uses the filename FN for automatic language detection if reading\n"
           "                from stdin unless -l is specified. The filename is also used for formatting logic (ie. sorting headers).\n"
           "\n"
           "Config/Help Options:\n"
           " -h -? --help --usage     : Print this message and exit.\n"
           " --version                : Print the version and exit.\n"
           " --count-options          : Print the number of available options and exit.\n"
           " --show-config            : Print out option documentation and exit.\n"
           " --update-config          : Output a new config file. Use with -o FILE.\n"
           " --update-config-with-doc : Output a new config file. Use with -o FILE.\n"
           " --universalindent        : Output a config file for Universal Indent GUI.\n"
           " --detect                 : Detects the config from a source file. Use with '-f FILE'.\n"
           "                            Detection is fairly limited.\n"
           " --set <option>=<value>   : Sets a new value to a config option.\n"
           "\n"
           "Debug Options:\n"
           " -p FILE               : Dump debug info into FILE, or to stdout if FILE is set to '-'.\n"
           "                         Must be used in combination with '-f FILE'\n"
           " -ds FILE              : Dump parsing info at various moments of the formatting process.\n"
           " --dump-steps FILE       This creates a series of files named 'FILE_nnn.log', each\n"
           "                         corresponding to a formatting step in uncrustify.\n"
           "                         The file 'FILE_000.log' lists the formatting options in use.\n"
           "                         Must be used in combination with '-f FILE'\n"
           " -L SEV                : Set the log severity (see log_levels.h; note 'A' = 'all')\n"
           " -s                    : Show the log severity in the logs.\n"
           " --decode              : Decode remaining args (chunk flags) and exit.\n"
           " --tracking space:FILE : Prepare space tracking information for debugging.\n"
           " --tracking nl:FILE    : Prepare newline tracking information for debugging.\n"
           " --tracking start:FILE : Prepare start of statement tracking information for debugging.\n"
           "                         Cannot be used with the -o option'\n"
           " --find_deprecated     : look for deprecated option(s) and exit.\n"
           "\n"
           "Usage Examples\n"
           "cat foo.d | uncrustify -q -c my.cfg -l d\n"
           "uncrustify -c my.cfg -f foo.d\n"
           "uncrustify -c my.cfg -f foo.d -L0-2,20-23,51\n"
           "uncrustify -c my.cfg -f foo.d -o foo.d\n"
           "uncrustify -c my.cfg -f foo.d -o foo.d -ds dump\n"
           "uncrustify -c my.cfg foo.d\n"
           "uncrustify -c my.cfg --replace foo.d\n"
           "uncrustify -c my.cfg --no-backup foo.d\n"
           "uncrustify -c my.cfg --prefix=out -F files.txt\n"
           "\n"
           "Note: Use comments containing ' *INDENT-OFF*' and ' *INDENT-ON*' to disable\n"
           "      processing of parts of the source file (these can be overridden with\n"
           "      enable_processing_cmt and disable_processing_cmt).\n"
           "\n"
           ,
           path_basename(argv0));
   tease();
} // usage


static void version_exit()
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
         usage_error();
         exit(EX_IOERR);
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
   const int max_name_length = 19;

   // maxLengthOfTheName must be consider at the format line at the file
   // output.cpp, line 427: fprintf(pfile, "# Line              Tag                Parent...
   // and              430: ... fprintf(pfile, "%s# %3zu>%19.19s[%19.19s] ...
   // here                                                xx xx   xx xx
   for (const auto &token_name : token_names)
   {
      const size_t name_length = strlen(token_name);

      if (name_length > max_name_length)
      {
         fprintf(stderr, "%s(%d): The token name '%s' is too long (%d)\n",
                 __func__, __LINE__, token_name, static_cast<int>(name_length));
         fprintf(stderr, "%s(%d): the max token name length is %d\n",
                 __func__, __LINE__, max_name_length);
         log_flush(true);
         exit(EX_SOFTWARE);
      }
   }

   // make sure we have token_names.h in sync with token_enum.h
   static_assert(ARRAY_SIZE(token_names) == CT_TOKEN_COUNT_, "");
#endif // DEBUG

   Args arg(argc, argv);

   if (  arg.Present("--version")
      || arg.Present("-v"))
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

   if (arg.Present("--count-options"))
   {
      tease();
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

   if (arg.Present("--find_deprecated"))
   {
      cpd.find_deprecated = true;
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
         log_pcf_flags(LSYS, static_cast<E_PcfFlag>(strtoul(p_arg, nullptr, 16)));
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
      if (  parsed_file[0] == '-'
         && !parsed_file[1])
      {
         LOG_FMT(LNOTE, "Will print parsed data to stdout\n");
      }
      else
      {
         LOG_FMT(LNOTE, "Will export parsed data to: %s\n", parsed_file);
      }
   }
   // Get the dump file name prefix
   const char *dump_file_T;

   if (  ((dump_file_T = arg.Param("--dump-steps")) != nullptr)
      || ((dump_file_T = arg.Param("-ds")) != nullptr))
   {
      // and save it
      // Issue #3976
      set_dump_file_name(dump_file_T);
      LOG_FMT(LNOTE, "Will export formatting steps data to '%s_nnn.log' files\n", dump_file_name);
   }

   // Enable log severities
   if (  arg.Present("-s")
      || arg.Present("--show"))
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
   bool arg_l_is_set = false;

   // Check for a language override
   if ((p_arg = arg.Param("-l")) != nullptr)
   {
      arg_l_is_set   = true;
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
   const char  *prefix = arg.Param("--prefix");
   const char  *suffix = arg.Param("--suffix");
   const char  *assume = arg.Param("--assume");

   bool        no_backup        = arg.Present("--no-backup");
   bool        replace          = arg.Present("--replace");
   bool        keep_mtime       = arg.Present("--mtime");
   bool        update_config    = arg.Present("--update-config");
   bool        update_config_wd = arg.Present("--update-config-with-doc");
   bool        detect           = arg.Present("--detect");
   bool        pfile_csv        = arg.Present("--debug-csv-format");
   bool        is_quiet         = arg.Present("-q");

   std::string parsed_file_csv;

   if (pfile_csv)
   {
      if (  parsed_file == nullptr
         || (  parsed_file[0] == '-'
            && !parsed_file[1]))
      {
         fprintf(stderr,
                 "FAIL: --debug-csv-format option must be used in combination with '-p FILE', where FILE\n"
                 "      is not set to '-'\n");
         log_flush(true);
         exit(EX_CONFIG);
      }
      else if (!ends_with(parsed_file, ".csv", false))
      {
         parsed_file_csv = parsed_file;

         // user-specified parsed filename does not end in a ".csv" extension, so add it
         parsed_file_csv += ".csv";
         parsed_file      = parsed_file_csv.c_str();
      }
   }
   // Grab the output override
   const char *output_file = arg.Param("-o");

   // for debugging tracking
   const char *html_arg = nullptr;
   html_arg = arg.Param("--tracking");

   if (html_arg != nullptr)
   {
      const size_t max_args_length = 256;
      size_t       argLength       = strlen(html_arg);

      if (argLength > max_args_length)
      {
         fprintf(stderr, "The buffer is to short for the tracking argument '%s'\n", html_arg);
         log_flush(true);
         exit(EX_SOFTWARE);
      }
      char buffer[max_args_length];
      strcpy(buffer, html_arg);

      // Tokenize and extract key and value
      const char *tracking_art = strtok(buffer, ":");
      const char *html_file    = strtok(nullptr, "\0");

      if (  html_file != nullptr
         && cpd.html_file == nullptr)
      {
         if (strcmp(tracking_art, "space") == 0)
         {
            cpd.html_type = tracking_type_e::TT_SPACE;
         }
         else if (strcmp(tracking_art, "nl") == 0)
         {
            cpd.html_type = tracking_type_e::TT_NEWLINE;
         }
         else if (strcmp(tracking_art, "start") == 0)
         {
            cpd.html_type = tracking_type_e::TT_START;
         }
         else
         {
            fprintf(stderr, "tracking_art '%s' not implemented\n",
                    tracking_art);
            log_flush(true);
            return(EXIT_FAILURE);
         }
         cpd.html_file = strdup(html_file);
      }
   }
   LOG_FMT(LDATA, "%s\n", UNCRUSTIFY_VERSION);
   LOG_FMT(LDATA, "config_file  = %s\n", cfg_file.c_str());
   LOG_FMT(LDATA, "output_file  = %s\n", (output_file != nullptr) ? output_file : "null");
   LOG_FMT(LDATA, "source_file  = %s\n", (source_file != nullptr) ? source_file : "null");
   LOG_FMT(LDATA, "source_list  = %s\n", (source_list != nullptr) ? source_list : "null");
   LOG_FMT(LDATA, "tracking_art = %s\n", get_tracking_type_e_name(cpd.html_type));
   LOG_FMT(LDATA, "tracking     = %s\n", (cpd.html_file != nullptr) ? cpd.html_file : "null");
   LOG_FMT(LDATA, "prefix       = %s\n", (prefix != nullptr) ? prefix : "null");
   LOG_FMT(LDATA, "suffix       = %s\n", (suffix != nullptr) ? suffix : "null");
   LOG_FMT(LDATA, "assume       = %s\n", (assume != nullptr) ? assume : "null");
   LOG_FMT(LDATA, "replace      = %s\n", replace ? "true" : "false");
   LOG_FMT(LDATA, "no_backup    = %s\n", no_backup ? "true" : "false");
   LOG_FMT(LDATA, "detect       = %s\n", detect ? "true" : "false");
   LOG_FMT(LDATA, "check        = %s\n", cpd.do_check ? "true" : "false");
   LOG_FMT(LDATA, "if_changed   = %s\n", cpd.if_changed ? "true" : "false");

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
      if (replace)
      {
         if (  prefix != nullptr
            || suffix != nullptr)
         {
            usage_error("Cannot use --replace with --prefix or --suffix");
            return(EX_NOINPUT);
         }

         if (  source_file != nullptr
            || output_file != nullptr)
         {
            usage_error("Cannot use --replace with -f or -o");
            return(EX_NOINPUT);
         }
      }
      else if (!no_backup)
      {
         if (  prefix == nullptr
            && suffix == nullptr)
         {
            suffix = ".uncrustify";
         }
      }
   }

   /*
    * Try to load the config file, if available.
    * It is optional for "--universalindent", "--parsed" and "--detect", but
    * required for everything else.
    */
   if (  !cfg_file.empty()
      && cfg_file[0] != '-')
   {
      cpd.filename = cfg_file;

      if (!load_option_file(cpd.filename.c_str()))
      {
         usage_error("Unable to load the config file");
         return(EX_IOERR);
      }
      // test if all options are compatible to each other
      log_rule_B("nl_max");

      if (options::nl_max() > 0)
      {
         // test if one/some option(s) is/are not too big for that
         too_big_for_nl_max();
      }
   }
   // Set config options using command line arguments.
   idx = 0;

   while ((p_arg = arg.Params("--set", idx)) != nullptr)
   {
      size_t       argLength       = strlen(p_arg);
      const size_t max_args_length = 256;

      if (argLength > max_args_length)
      {
         fprintf(stderr, "The buffer is to short for the set argument '%s'\n", p_arg);
         log_flush(true);
         exit(EX_SOFTWARE);
      }
      char buffer[max_args_length];
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
   // Set the number of second(s) before terminating formatting the current file.
#ifdef WIN32
   if (options::debug_timeout() > 0)
   {
      fprintf(stderr, "The option 'debug_timeout' is not available under Windows.\n");
      log_flush(true);
      exit(EX_SOFTWARE);
   }
#else
   if (options::debug_timeout() > 0)
   {
      alarm((unsigned int)options::debug_timeout());
   }
#endif // ifdef WIN32

   if (detect)
   {
      file_mem fm;

      if (  source_file == nullptr
         || source_list != nullptr)
      {
         fprintf(stderr, "The --detect option requires a single input file\n");
         log_flush(true);
         return(EXIT_FAILURE);
      }

      // Do some simple language detection based on the filename extension
      if (  !cpd.lang_forced
         || cpd.lang_flags == 0)
      {
         cpd.lang_flags = language_flags_from_filename(source_file);
      }

      // Try to read in the source file
      if (load_mem_file(source_file, fm) < 0)
      {
         LOG_FMT(LERR, "Failed to load (%s)\n", source_file);
         exit(EX_IOERR);
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

   if (  update_config
      || update_config_wd)
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
    * Everything beyond this point aside from dumping the parse tree is silly
    * without a config file, so complain and bail if we don't have one.
    */
   if (  cfg_file.empty()
      && !parsed_file)
   {
      usage_error("Specify the config file with '-c file' or set UNCRUSTIFY_CONFIG");
      return(EX_IOERR);
   }
   // Done parsing args

   // Check for unused args (ignore them)
   idx   = 1;
   p_arg = arg.Unused(idx);

   // Check args - for multifile options
   if (  source_list != nullptr
      || (  p_arg != nullptr
         && cpd.html_type == tracking_type_e::TT_NONE))           // Issue #4066
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

   if (  cpd.do_check
      || cpd.if_changed)
   {
      cpd.bout = new deque<UINT8>();
   }
   idx = 1;

   if (  source_file == nullptr
      && source_list == nullptr
      && arg.Unused(idx) == nullptr)
   {
      if (!arg_l_is_set)                     // Issue #3064
      {
         if (assume == nullptr)
         {
            LOG_FMT(LERR, "If reading from stdin, you should specify the language using -l\n");
            LOG_FMT(LERR, "or specify a filename using --assume for automatic language detection.\n");
            return(EXIT_FAILURE);
         }
      }

      if (cpd.lang_flags == 0)
      {
         if (assume != nullptr)
         {
            cpd.lang_flags = language_flags_from_filename(assume);
         }
         else
         {
            cpd.lang_flags = e_LANG_C;
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
         exit(EX_IOERR);
      }
      cpd.filename = assume != nullptr ? assume : "stdin";

      // Done reading from stdin
      LOG_FMT(LSYS, "%s(%d): Parsing: %zu bytes (%zu chars) from stdin as language %s\n",
              __func__, __LINE__, fm.raw.size(), fm.data.size(),
              language_name_from_flags(cpd.lang_flags));

      // Issue #3427
      init_keywords_for_language();
      uncrustify_file(fm, stdout, parsed_file, dump_file_name, is_quiet);
   }
   else if (source_file != nullptr)
   {
      // Doing a single file
      do_source_file(source_file, output_file, parsed_file, dump_file_name, no_backup, keep_mtime, is_quiet);
   }
   else
   {
      if (parsed_file != nullptr)   // Issue #930
      {
         fprintf(stderr, "FAIL: -p option must be used with the -f option\n");
         log_flush(true);
         exit(EX_CONFIG);
      }

      if (dump_file_name[0] != char(0))                // Issue #3976
      {
         fprintf(stderr, "FAIL: -ds/--dump-steps option must be used with the -f option\n");
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
                        nullptr, nullptr, no_backup, keep_mtime, is_quiet);
      }

      if (source_list != nullptr)
      {
         process_source_list(source_list, prefix, suffix, no_backup, keep_mtime, is_quiet);
      }
   }
   clear_keyword_file();

   if (  cpd.do_check
      && cpd.check_fail_cnt != 0)
   {
      return(EXIT_FAILURE);
   }
   return(EXIT_SUCCESS);
} // main


static void process_source_list(const char *source_list,
                                const char *prefix, const char *suffix,
                                bool no_backup, bool keep_mtime, bool is_quiet)
{
   bool from_stdin = strcmp(source_list, "-") == 0;
   FILE *p_file    = from_stdin ? stdin : fopen(source_list, "r");

   if (p_file == nullptr)
   {
      LOG_FMT(LERR, "%s: fopen(%s) failed: %s (%d)\n",
              __func__, source_list, strerror(errno), errno);
      exit(EX_IOERR);
   }
   char linebuf[256];
   int  line = 0;

   while (fgets(linebuf, sizeof(linebuf), p_file) != nullptr)
   {
      line++;
      char *fname = linebuf;
      int  len    = strlen(fname);

      while (  len > 0
            && unc_isspace(*fname))
      {
         fname++;
         len--;
      }

      while (  len > 0
            && unc_isspace(fname[len - 1]))
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
      LOG_FMT(LFILELIST, "%3d file to uncrustify: %s\n", line, fname);

      if (fname[0] != '#')
      {
         char outbuf[1024];
         do_source_file(fname,
                        make_output_filename(outbuf, sizeof(outbuf), fname, prefix, suffix),
                        nullptr, nullptr, no_backup, keep_mtime, is_quiet);
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

   // Re-open stdin in binary mode to preserve newline characters
#ifdef WIN32
   _setmode(_fileno(stdin), _O_BINARY);
#endif

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
      if (  (outname[idx] == '/')
         || (outname[idx] == '\\'))
      {
         outname[idx] = PATH_SEP;
      }

      // search until end of subpath is found
      if (  idx > last_idx
         && (outname[idx] == PATH_SEP))
      {
         outname[idx] = 0; // mark the end of the subpath

         // create subfolder if it is not the start symbol of a path
         // and not a Windows drive letter
         if (  (strcmp(&outname[last_idx], ".") != 0)
            && (strcmp(&outname[last_idx], "..") != 0)
            && (!(  last_idx == 0
                 && idx == 2
                 && outname[1] == ':')))
         {
            int status;    // Coverity CID 75999
            status = mkdir(outname, 0750);

            if (  status != 0
               && errno != EEXIST)
            {
               LOG_FMT(LERR, "%s: Unable to create %s: %s (%d)\n",
                       __func__, outname, strerror(errno), errno);
               exit(EX_IOERR);
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
         exit(EX_IOERR);
      }
      else if (!decode_unicode(fm.raw, fm.data, fm.enc, fm.bom))
      {
         LOG_FMT(LERR, "%s: failed to decode the file '%s'\n", __func__, filename);
         exit(EX_IOERR);
      }
      else
      {
         LOG_FMT(LNOTE, "%s: '%s' encoding looks like %s (%d)\n", __func__, filename,
                 get_char_encoding(fm.enc), (int)fm.enc);
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
         exit(EX_IOERR);
      }
   }
   return(retval);
}


int load_header_files()
{
   int retval = 0;

   log_rule_B("cmt_insert_file_header");

   if (!options::cmt_insert_file_header().empty())
   {
      // try to load the file referred to by the options string
      retval |= load_mem_file_config(options::cmt_insert_file_header(),
                                     cpd.file_hdr);
   }
   log_rule_B("cmt_insert_file_footer");

   if (!options::cmt_insert_file_footer().empty())
   {
      retval |= load_mem_file_config(options::cmt_insert_file_footer(),
                                     cpd.file_ftr);
   }
   log_rule_B("cmt_insert_func_header");

   if (!options::cmt_insert_func_header().empty())
   {
      retval |= load_mem_file_config(options::cmt_insert_func_header(),
                                     cpd.func_hdr);
   }
   log_rule_B("cmt_insert_class_header");

   if (!options::cmt_insert_class_header().empty())
   {
      retval |= load_mem_file_config(options::cmt_insert_class_header(),
                                     cpd.class_hdr);
   }
   log_rule_B("cmt_insert_oc_msg_header");

   if (!options::cmt_insert_oc_msg_header().empty())
   {
      retval |= load_mem_file_config(options::cmt_insert_oc_msg_header(),
                                     cpd.oc_msg_hdr);
   }
   log_rule_B("cmt_reflow_fold_regex_file");

   if (!options::cmt_reflow_fold_regex_file().empty())
   {
      retval |= load_mem_file_config(options::cmt_reflow_fold_regex_file(),
                                     cpd.reflow_fold_regex);
   }
   return(retval);
} // load_header_files


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

   while (  len1 >= 0
         && len2 >= 0)
   {
      if (len1 == 0)
      {
         len1 = read(fd1, buf1, sizeof(buf1));
      }

      if (len2 == 0)
      {
         len2 = read(fd2, buf2, sizeof(buf2));
      }

      if (  len1 <= 0
         || len2 <= 0)
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

   return(  len1 == 0
         && len2 == 0);
} // file_content_matches


static bool bout_content_matches(const file_mem &fm, bool report_status, bool is_quiet)
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

   if (  is_same
      && report_status)
   {
      if (!is_quiet)
      {
         fprintf(stdout, "PASS: %s (%u bytes)\n",
                 cpd.filename.c_str(), static_cast<int>(fm.raw.size()));
      }
   }
   return(is_same);
} // bout_content_matches


static void do_source_file(const char *filename_in,
                           const char *filename_out,
                           const char *parsed_file,
                           const char *dump_file,
                           bool       no_backup,
                           bool       keep_mtime,
                           bool       is_quiet)
{
   FILE     *pfout      = nullptr;
   bool     did_open    = false;
   bool     need_backup = false;
   file_mem fm;
   string   filename_tmp;

   // Do some simple language detection based on the filename extension
   if (  !cpd.lang_forced
      || cpd.lang_flags == 0)
   {
      cpd.lang_flags = language_flags_from_filename(filename_in);
   }

   // Try to read in the source file
   if (load_mem_file(filename_in, fm) < 0)
   {
      LOG_FMT(LERR, "Failed to load (%s)\n", filename_in);
      exit(EX_IOERR);
   }
   LOG_FMT(LSYS, "%s: Parsing: %s as language %s\n",
           __func__, filename_in, language_name_from_flags(cpd.lang_flags));

   // check keyword sort
   assert(keywords_are_sorted());

   // Issue #3353
   init_keywords_for_language();

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
      uncrustify_file(fm, nullptr, parsed_file, dump_file, is_quiet, true);

      if (bout_content_matches(fm, false, is_quiet))
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
            filename_tmp += ".uncrustify";

            if (!no_backup)
            {
               if (backup_copy_file(filename_in, fm.raw) != EX_OK)
               {
                  LOG_FMT(LERR, "%s: Failed to create backup file for %s\n",
                          __func__, filename_in);
                  exit(EX_IOERR);
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
            exit(EX_IOERR);
         }
         did_open = true;
         //LOG_FMT(LSYS, "Output file %s\n", filename_out);
      }
   }

   if (cpd.if_changed)
   {
      for (UINT8 i : *cpd.bout)
      {
         fputc(i, pfout);
      }

      uncrustify_end();
   }
   else
   {
      uncrustify_file(fm, pfout, parsed_file, dump_file, is_quiet);
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
         if (  !cpd.if_changed
            && file_content_matches(filename_tmp, filename_out))
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
               exit(EX_IOERR);
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
   // don't add the file header if running as frag
   if (  !Chunk::GetHead()->IsComment()
      && !cpd.frag)
   {
      // TODO: detect the typical #ifndef FOO / #define FOO sequence
      tokenize(cpd.file_hdr.data, Chunk::GetHead());
   }
}


static void add_file_footer()
{
   Chunk *pc = Chunk::GetTail();

   // Back up if the file ends with a newline
   if (  pc->IsNotNullChunk()
      && pc->IsNewline())
   {
      pc = pc->GetPrev();
   }

   if (  pc->IsNotNullChunk()
      && (  !pc->IsComment()
         || !pc->GetPrev()->IsNewline()))
   {
      pc = Chunk::GetTail();

      if (!pc->IsNewline())
      {
         LOG_FMT(LSYS, "Adding a newline at the end of the file\n");
         newline_add_after(pc);
      }
      tokenize(cpd.file_ftr.data, Chunk::NullChunkPtr);
   }
}


static void add_func_header(E_Token type, file_mem &fm)
{
   Chunk *pc;
   Chunk *ref;
   Chunk *tmp;
   bool  do_insert;

   for (pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnlNpp())
   {
      if (pc->GetType() != type)
      {
         continue;
      }
      log_rule_B("cmt_insert_before_inlines");

      if (  pc->TestFlags(PCF_IN_CLASS)
         && !options::cmt_insert_before_inlines())
      {
         continue;
      }
      // Check for one liners for classes. Declarations only. Walk down the chunks.
      ref = pc;

      if (  ref->Is(CT_CLASS)
         && ref->GetParentType() == CT_NONE
         && ref->GetNext())
      {
         ref = ref->GetNext();

         if (  ref->Is(CT_TYPE)
            && ref->GetParentType() == type
            && ref->GetNext())
         {
            ref = ref->GetNext();

            if (  ref->Is(CT_SEMICOLON)
               && ref->GetLevel() == pc->GetLevel())
            {
               continue;
            }
         }
      }
      // Check for one liners for functions. There'll be a closing brace w/o any newlines. Walk down the chunks.
      ref = pc;

      if (  ref->Is(CT_FUNC_DEF)
         && ref->GetParentType() == CT_NONE
         && ref->GetNext())
      {
         int found_brace = 0;                                 // Set if a close brace is found before a newline

         while (  ref->IsNot(CT_NEWLINE)
               && (ref = ref->GetNext())) // TODO: is the assignment of ref wanted here?, better move it to the loop
         {
            if (ref->Is(CT_BRACE_CLOSE))
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

      while (  (ref = ref->GetPrev())
            && ref->IsNotNullChunk())
      {
         // Bail if we change level or find an access specifier colon
         if (  ref->GetLevel() != pc->GetLevel()
            || ref->Is(CT_ACCESS_COLON))
         {
            do_insert = true;
            break;
         }

         // If we hit an angle close, back up to the angle open
         if (ref->Is(CT_ANGLE_CLOSE))
         {
            ref = ref->GetPrevType(CT_ANGLE_OPEN, ref->GetLevel(), E_Scope::PREPROC);
            continue;
         }

         // Bail if we hit a preprocessor and cmt_insert_before_preproc is false
         if (ref->TestFlags(PCF_IN_PREPROC))
         {
            tmp = ref->GetPrevType(CT_PREPROC, ref->GetLevel());

            if (  tmp->IsNotNullChunk()
               && tmp->GetParentType() == CT_PP_IF)
            {
               tmp = tmp->GetPrevNnl();

               log_rule_B("cmt_insert_before_preproc");

               if (  tmp->IsComment()
                  && !options::cmt_insert_before_preproc())
               {
                  break;
               }
            }
         }

         // Ignore 'right' comments
         if (  ref->IsComment()
            && ref->GetPrev()->IsNewline())
         {
            break;
         }

         if (  ref->GetLevel() == pc->GetLevel()
            && (  ref->TestFlags(PCF_IN_PREPROC)
               || ref->Is(CT_SEMICOLON)
               || ref->Is(CT_BRACE_CLOSE)))
         {
            do_insert = true;
            break;
         }
      }

      if (  ref->IsNullChunk()
         && !Chunk::GetHead()->IsComment()
         && Chunk::GetHead()->GetParentType() == type)
      {
         /**
          * In addition to testing for preceding semicolons, closing braces, etc.,
          * we need to also account for the possibility that the function declaration
          * or definition occurs at the very beginning of the file
          */
         tokenize(fm.data, Chunk::GetHead());
      }
      else if (do_insert)
      {
         // Insert between after and ref
         Chunk *after = ref->GetNextNcNnl();
         tokenize(fm.data, after);

         for (tmp = ref->GetNext(); tmp != after; tmp = tmp->GetNext())
         {
            tmp->SetLevel(after->GetLevel());
         }
      }
   }
} // add_func_header


static void add_msg_header(E_Token type, file_mem &fm)
{
   Chunk *pc;
   Chunk *ref;
   Chunk *tmp;
   bool  do_insert;

   for (pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnlNpp())
   {
      if (pc->GetType() != type)
      {
         continue;
      }
      do_insert = false;

      /*
       * On a message declaration back up to a Objective-C scope
       * the same level
       */
      ref = pc;

      while ((ref = ref->GetPrev())->IsNotNullChunk())
      {
         // ignore the CT_TYPE token that is the result type
         if (  ref->GetLevel() != pc->GetLevel()
            && (  ref->Is(CT_TYPE)
               || ref->Is(CT_PTR_TYPE)))
         {
            continue;
         }

         // If we hit a parentheses around return type, back up to the open parentheses
         if (ref->Is(CT_PAREN_CLOSE))
         {
            ref = ref->GetPrevType(CT_PAREN_OPEN, ref->GetLevel(), E_Scope::PREPROC);
            continue;
         }

         // Bail if we hit a preprocessor and cmt_insert_before_preproc is false
         if (ref->TestFlags(PCF_IN_PREPROC))
         {
            tmp = ref->GetPrevType(CT_PREPROC, ref->GetLevel());

            if (  tmp->IsNotNullChunk()
               && tmp->GetParentType() == CT_PP_IF)
            {
               tmp = tmp->GetPrevNnl();

               log_rule_B("cmt_insert_before_preproc");

               if (  tmp->IsComment()
                  && !options::cmt_insert_before_preproc())
               {
                  break;
               }
            }
         }

         if (  ref->GetLevel() == pc->GetLevel()
            && (  ref->TestFlags(PCF_IN_PREPROC)
               || ref->Is(CT_OC_SCOPE)))
         {
            ref = ref->GetPrev();

            if (ref->IsNotNullChunk())
            {
               // Ignore 'right' comments
               if (  ref->IsNewline()
                  && ref->GetPrev()->IsComment())
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
         Chunk *after = ref->GetNextNcNnl();
         tokenize(fm.data, after);

         for (tmp = ref->GetNext(); tmp != after; tmp = tmp->GetNext())
         {
            tmp->SetLevel(after->GetLevel());
         }
      }
   }
} // add_msg_header


static void uncrustify_start(const deque<int> &data)
{
   // Parse the text into chunks
   tokenize(data, Chunk::NullChunkPtr);
   PROT_THE_LINE

   cpd.unc_stage = unc_stage_e::HEADER;

   // Get the column for the fragment indent
   if (cpd.frag)
   {
      Chunk *pc = Chunk::GetHead();

      cpd.frag_cols = pc->GetOrigCol();
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

   parameter_pack_cleanup();

   // At this point, the level information is available and accurate.

   if (language_is_set(lang_flag_e::LANG_PAWN))
   {
      pawn_prescan();
   }
   mark_question_colon();

   // Re-type chunks, combine chunks
   fix_symbols();

   tokenize_trailing_return_types();

   mark_comments();

   // Look at all colons ':' and mark labels, :? sequences, etc.
   combine_labels();

   enum_cleanup();

   mark_functor();
} // uncrustify_start


void uncrustify_file(const file_mem &fm, FILE *pfout, const char *parsed_file,
                     const char *dump_file, bool is_quiet, bool defer_uncrustify_end)
{
   const deque<int> &data = fm.data;

   // Save off the encoding and whether a BOM is required
   cpd.bom = fm.bom;
   cpd.enc = fm.enc;

   if (  options::utf8_force()
      || (  (cpd.enc == char_encoding_e::e_BYTE)
         && options::utf8_byte()))
   {
      log_rule_B("utf8_force");
      log_rule_B("utf8_byte");
      cpd.enc = char_encoding_e::e_UTF8;
   }
   iarf_e av;

   switch (cpd.enc)
   {
   case char_encoding_e::e_UTF8:
      log_rule_B("utf8_bom");
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
         exit(EX_IOERR);
      }
      count_column++;

      if (data[idx] == '\n')
      {
         count_line++;
         count_column = 1;
      }
   }

   uncrustify_start(data);
   dump_step(dump_file, "After uncrustify_start()");

   cpd.unc_stage = unc_stage_e::OTHER;

   /*
    * Done with detection. Do the rest only if the file will go somewhere.
    * The detection code needs as few changes as possible.
    */

   // Add comments before function defs and classes
   if (!cpd.func_hdr.data.empty())
   {
      log_rule_B("cmt_insert_function_header");

      add_func_header(CT_FUNC_DEF, cpd.func_hdr);

      if (options::cmt_insert_before_ctor_dtor())
      {
         add_func_header(CT_FUNC_CLASS_DEF, cpd.func_hdr);
      }
   }

   if (!cpd.class_hdr.data.empty())
   {
      log_rule_B("cmt_insert_class_header");
      add_func_header(CT_CLASS, cpd.class_hdr);
   }

   if (!cpd.oc_msg_hdr.data.empty())
   {
      log_rule_B("cmt_insert_oc_message_header");
      add_msg_header(CT_OC_MSG_DECL, cpd.oc_msg_hdr);
   }
   do_parent_for_pp();

   // Rewrite infinite loops
   if (options::mod_infinite_loop())
   {
      log_rule_B("mod_infinite_loop");
      rewrite_infinite_loops();
   }
   // Change virtual braces into real braces
   do_braces();

   // Scrub extra semicolons
   if (options::mod_remove_extra_semicolon())
   {
      log_rule_B("mod_remove_extra_semicolon");
      remove_extra_semicolons();
   }

   // Remove unnecessary returns
   if (options::mod_remove_empty_return())
   {
      log_rule_B("mod_remove_empty_return");
      remove_extra_returns();
   }

   // Add or remove redundant 'int' keyword of integer types
   if (  (  language_is_set(lang_flag_e::LANG_C)
         || language_is_set(lang_flag_e::LANG_CPP))
      && (  options::mod_int_short() != IARF_IGNORE
         || options::mod_short_int() != IARF_IGNORE
         || options::mod_int_long() != IARF_IGNORE
         || options::mod_long_int() != IARF_IGNORE
         || options::mod_int_signed() != IARF_IGNORE
         || options::mod_signed_int() != IARF_IGNORE
         || options::mod_int_unsigned() != IARF_IGNORE
         || options::mod_unsigned_int() != IARF_IGNORE))
   {
      log_rule_B("mod_int_short");
      log_rule_B("mod_short_int");
      log_rule_B("mod_int_long");
      log_rule_B("mod_long_int");
      log_rule_B("mod_int_signed");
      log_rule_B("mod_signed_int");
      log_rule_B("mod_int_unsigned");
      log_rule_B("mod_unsigned_int");
      change_int_types();
   }

   // Remove duplicate include
   if (options::mod_remove_duplicate_include())
   {
      log_rule_B("mod_remove_duplicate_include");
      remove_duplicate_include();
   }
   // Add parens
   do_parens();
   do_parens_assign();
   do_parens_return();

   // Modify line breaks as needed
   bool first = true;
   int  old_changes;

   if (options::nl_remove_extra_newlines() == 2)
   {
      log_rule_B("nl_remove_extra_newlines");
      newlines_remove_newlines();
   }
   cpd.pass_count = 3;

   dump_step(dump_file, "Before first while loop");

   do
   {
      old_changes = cpd.changes;

      LOG_FMT(LNEWLINE, "Newline loop start: %d\n", cpd.changes);

      annotations_newlines();
      newlines_cleanup_dup();
      newlines_sparens();
      newlines_cleanup_braces(first);
      newlines_cleanup_angles();                           // Issue #1167

      if (options::nl_after_multiline_comment())
      {
         log_rule_B("nl_after_multiline_comment");
         newline_after_multiline_comment();
      }

      if (options::nl_after_label_colon())
      {
         log_rule_B("nl_after_label_colon");
         newline_after_label_colon();
      }
      newlines_insert_blank_lines();

      if (options::pos_bool() != TP_IGNORE)
      {
         log_rule_B("pos_bool");
         newlines_chunk_pos(CT_BOOL, options::pos_bool());
      }

      if (options::pos_compare() != TP_IGNORE)
      {
         log_rule_B("pos_compare");
         newlines_chunk_pos(CT_COMPARE, options::pos_compare());
      }

      if (options::pos_conditional() != TP_IGNORE)
      {
         log_rule_B("pos_conditional");
         newlines_chunk_pos(CT_COND_COLON, options::pos_conditional());
         newlines_chunk_pos(CT_QUESTION, options::pos_conditional());
      }

      if (  options::pos_comma() != TP_IGNORE
         || options::pos_enum_comma() != TP_IGNORE)
      {
         log_rule_B("pos_comma");
         log_rule_B("pos_enum_comma");
         newlines_chunk_pos(CT_COMMA, options::pos_comma());
      }

      if (options::pos_assign() != TP_IGNORE)
      {
         log_rule_B("pos_assign");
         newlines_chunk_pos(CT_ASSIGN, options::pos_assign());
      }

      if (options::pos_arith() != TP_IGNORE)
      {
         log_rule_B("pos_arith");
         newlines_chunk_pos(CT_ARITH, options::pos_arith());
         newlines_chunk_pos(CT_CARET, options::pos_arith());
      }

      if (options::pos_shift() != TP_IGNORE)
      {
         log_rule_B("pos_shift");
         newlines_chunk_pos(CT_SHIFT, options::pos_shift());
      }
      newlines_class_colon_pos(CT_CLASS_COLON);
      newlines_class_colon_pos(CT_CONSTR_COLON);

      if (options::nl_squeeze_ifdef())
      {
         log_rule_B("nl_squeeze_ifdef");
         newlines_squeeze_ifdef();
      }

      if (options::nl_squeeze_paren_close())
      {
         log_rule_B("nl_squeeze_paren_close");
         newlines_squeeze_paren_close();
      }
      do_blank_lines();
      newlines_eat_start_end();
      newlines_functions_remove_extra_blank_lines();
      newlines_cleanup_dup();
      first = false;
      dump_step(dump_file, "Inside first while loop");
   } while (  old_changes != cpd.changes
           && cpd.pass_count-- > 0);

   mark_comments();

   // Scrub certain added semicolons
   if (  language_is_set(lang_flag_e::LANG_PAWN)
      && options::mod_pawn_semicolon())
   {
      log_rule_B("mod_pawn_semicolon");
      pawn_scrub_vsemi();
   }

   // Sort imports/using/include
   if (  options::mod_sort_import()
      || options::mod_sort_include()
      || options::mod_sort_using())
   {
      log_rule_B("mod_sort_import");
      log_rule_B("mod_sort_include");
      log_rule_B("mod_sort_using");
      sort_imports();
   }
   // Fix same-line inter-chunk spacing
   space_text();

   if (options::align_pp_define_span() > 0)
   {
      // Do any aligning of preprocessors
      log_rule_B("align_pp_define_span");
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
      log_rule_B("mod_add_long_switch_closebrace_comment");
      log_rule_B("mod_add_long_function_closebrace_comment");
      log_rule_B("mod_add_long_class_closebrace_comment");
      log_rule_B("mod_add_long_namespace_closebrace_comment");
      add_long_closebrace_comment();
   }

   // Insert trailing comments after certain preprocessor conditional blocks
   if (  (options::mod_add_long_ifdef_else_comment() > 0)
      || (options::mod_add_long_ifdef_endif_comment() > 0))
   {
      log_rule_B("mod_add_long_ifdef_else_comment");
      log_rule_B("mod_add_long_ifdef_endif_comment");
      add_long_preprocessor_conditional_block_comment();
   }
   // Align everything else, reindent and break at code_width
   first = true;

   dump_step(dump_file, "Before second while loop");

   do
   {
      align_all();
      indent_text();
      old_changes = cpd.changes;

      if (options::code_width() > 0)
      {
         log_rule_B("code_width");
         LOG_FMT(LNEWLINE, "%s(%d): Code_width loop start: %d\n",
                 __func__, __LINE__, cpd.changes);

         if (options::debug_max_number_of_loops() > 0)
         {
            log_rule_B("debug_max_number_of_loops");

            if (cpd.changes > options::debug_max_number_of_loops())                 // Issue #2432
            {
               LOG_FMT(LNEWLINE, "%s(%d): too many loops. Make a report, please.\n",
                       __func__, __LINE__);
               log_flush(true);
               exit(EX_SOFTWARE);
            }
         }
         do_code_width();

         if (  old_changes != cpd.changes
            && first)
         {
            // retry line breaks caused by splitting 1-liners
            newlines_cleanup_braces(false);
            newlines_insert_blank_lines();
            newlines_functions_remove_extra_blank_lines();
            newlines_remove_disallowed();
            first = false;
         }
      }
      dump_step(dump_file, "Inside second while loop");
   } while (old_changes != cpd.changes);

   // And finally, align the backslash newline stuff
   align_right_comments();

   if (options::align_nl_cont())
   {
      log_rule_B("align_nl_cont");
      align_backslash_newline();
   }
   dump_step(dump_file, "Final version");

   // which output is to be done?
   if (cpd.html_file == nullptr)
   {
      // Now render it all to the output file
      output_text(pfout);
   }
   else
   {
      make_folders(cpd.html_file);                    // Issue #4066
      // create the tracking file
      FILE *t_file;
      t_file = fopen(cpd.html_file, "wb");

      if (t_file == nullptr)
      {
         LOG_FMT(LERR, "%s: Unable to create %s: %s (%d)\n",
                 __func__, cpd.html_file, strerror(errno), errno);
         exit(EX_IOERR);
      }
      else
      {
         LOG_FMT(LDATA, "tracking is opened = %s\n", (cpd.html_file != nullptr) ? cpd.html_file : "null");
      }
      output_text(t_file);
      fclose(t_file);
      LOG_FMT(LDATA, "tracking is closed = %s\n", (cpd.html_file != nullptr) ? cpd.html_file : "null");
      exit(EX_OK);
   }

   // Special hook for dumping parsed data for debugging
   if (parsed_file != nullptr)
   {
      FILE *p_file;

      if (  parsed_file[0] == '-'
         && !parsed_file[1])
      {
         p_file = stdout;
      }
      else
      {
         p_file = fopen(parsed_file, "wb");
      }

      if (p_file != nullptr)
      {
         if (ends_with(parsed_file, ".csv", false))
         {
            output_parsed_csv(p_file);
         }
         else
         {
            output_parsed(p_file);
         }

         if (p_file != stdout)
         {
            fclose(p_file);
         }
      }
      else
      {
         LOG_FMT(LERR, "%s: Failed to open '%s' for write: %s (%d)\n",
                 __func__, parsed_file, strerror(errno), errno);
         exit(EX_IOERR);
      }
   }

   if (  cpd.do_check
      && !bout_content_matches(fm, true, is_quiet))
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
   Chunk *pc;

   cpd.unc_stage = unc_stage_e::CLEANUP;

   while ((pc = Chunk::GetHead())->IsNotNullChunk())
   {
      Chunk::Delete(pc);
   }

   if (cpd.bout)
   {
      cpd.bout->clear();
   }
   // Clean up some state variables
   cpd.unc_off     = false;
   cpd.al_cnt      = 0;
   cpd.did_newline = true;
   cpd.pp_level    = 0;
   cpd.changes     = 0;
   cpd.in_preproc  = CT_NONE;
   memset(cpd.le_counts, 0, sizeof(cpd.le_counts));
   cpd.preproc_ncnl_count                     = 0;
   cpd.ifdef_over_whole_file                  = 0;
   cpd.warned_unable_string_replace_tab_chars = false;
}


const char *get_token_name(E_Token token)
{
   if (  token >= 0
      && (token < static_cast<int> ARRAY_SIZE(token_names))
      && (token_names[token] != nullptr))
   {
      return(token_names[token]);
   }
   return("???");
}


E_Token find_token_name(const char *text)
{
   if (  text != nullptr
      && (*text != 0))
   {
      for (int idx = 1; idx < static_cast<int> ARRAY_SIZE(token_names); idx++)
      {
         if (strcasecmp(text, token_names[idx]) == 0)
         {
            return(static_cast<E_Token>(idx));
         }
      }
   }
   return(CT_NONE);
}
