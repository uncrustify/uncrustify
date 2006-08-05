/**
 * @file uncrustify.cpp
 * This file takes an input C/C++/D/Java file and reformats it.
 *
 * $Id$
 */
#define DEFINE_PCF_NAMES

#include "uncrustify_version.h"
#include "uncrustify_types.h"
#include "char_table.h"
#include "chunk_list.h"
#include "prototypes.h"
#include "token_names.h"
#include "args.h"
#include "logger.h"
#include "log_levels.h"

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
static void do_source_file(const char *filename, FILE *pfout, const char *parsed_file);
static void process_source_list(const char *source_list);

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
      if (ch == '/')
      {
         last_path = path;
      }
   }
   return(last_path);
}

static void usage_exit(const char *msg, const char *argv0, int code)
{
   if (msg != NULL)
   {
      fprintf(stderr, "%s\n", msg);
   }
   fprintf(stderr,
           "Usage:\n"
           "%s -c cfg [-f file] [-F filelist] [-p parsed] [-t typefile] [--version] [-l lang] [-L sev] [-s] [files ...]\n"
           " c : specify the config file\n"
           " f : specify the file to format\n"
           " F : specify a file that contains a list of files to process\n"
           " files : whitespace-separated list of files to process\n"
           " t : load a file with types\n"
           " d : load a file with defines\n"
           " l : language override: C, CPP, D, CS, JAVA, PAWN\n"
           "--show-config : print out a list of all available options and exit\n"
           "--version : print the version and exit\n"
           "\n"
           "If no input files are specified, the input is read from stdin\n"
           "If -F is used, the output is is the filename + .uncrustify\n"
           "Otherwise, the output is dumped to stdout.\n"
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
   char       *data;
   int        data_len;
   const char *cfg_file    = "uncrustify.cfg";
   const char *parsed_file = NULL;
   const char *source_file = NULL;
   const char *source_list = NULL;
   log_mask_t mask;
   int        idx;
   const char *p_arg;


   if (argc < 2)
   {
      usage_exit(NULL, argv[0], 0);
   }

   Args arg(argc, argv);

   if (arg.Present("--version") || arg.Present("-v"))
   {
      version_exit();
   }
   if (arg.Present("--help") || arg.Present("-h") ||
       arg.Present("--usage") || arg.Present("-?"))
   {
      usage_exit(NULL, argv[0], 0);
   }

   if (arg.Present("--show-config"))
   {
      print_options(stdout);
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

   /* Get the config file name */
   if (((cfg_file = arg.Param("--config")) == NULL) &&
       ((cfg_file = arg.Param("-c")) == NULL))
   {
      usage_exit("Specify the config file: -c file", argv[0], 58);
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
         fprintf(stderr, "Ignoring unknown language: %s\n", p_arg);
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

   /* Check for unused args (ignore them) */
   idx   = 1;
   p_arg = arg.Unused(idx);

   /* Check args */
   if ((source_file != NULL) && ((source_list != NULL) || (p_arg != NULL)))
   {
      usage_exit("Cannot specify both the single file option and a mulit-file option.",
                 argv[0], 0);
   }

   /* Load the config file */
   set_option_defaults();
   if (load_option_file(cfg_file) < 0)
   {
      usage_exit("Unable to load the config file", argv[0], 56);
   }

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

      /* Done reading from stdin */
      LOG_FMT(LSYS, "Parsing: %d bytes from stdin as language %s\n",
              data_len, language_to_string(cpd.lang_flags));

      uncrustify_file(data, data_len, stdout, parsed_file);
   }
   else if (source_file != NULL)
   {
      /* Doing a single file, output to stdout */
      do_source_file(source_file, stdout, parsed_file);
   }
   else
   {
      /* Doing multiple files */

      /* Do the files on the command line first */
      idx = 1;
      while ((p_arg = arg.Unused(idx)) != NULL)
      {
         do_source_file(p_arg, NULL, NULL);
      }

      if (source_list != NULL)
      {
         process_source_list(source_list);
      }
   }

   return(0);
}


static void process_source_list(const char *source_list)
{
   FILE *p_file = fopen(source_list, "r");

   if (p_file == NULL)
   {
      LOG_FMT(LERR, "Unable to read %s\n", source_list);
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
         do_source_file(args[0], NULL, NULL);
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

   /* Make sure the buffer is terminated */
   data[data_len] = 0;

   out_len = data_len;

   return(data);
}


/**
 * Does a source file.
 * If pfout is NULL, the source fileaname + ".uncrustify" is used.
 *
 * @param filename the file to read
 * @param pfout    NULL or the output stream
 */
static void do_source_file(const char *filename, FILE *pfout, const char *parsed_file)
{
   int         data_len;
   char        *data;
   bool        did_open = false;
   FILE        *p_file;
   struct stat my_stat;


   /* Do some simple language detection based on the filename */
   if (cpd.lang_flags == 0)
   {
      cpd.lang_flags = language_from_filename(filename);
   }

   /* Try to read in the source file */
   p_file = fopen(filename, "rb");
   if (p_file == NULL)
   {
      LOG_FMT(LERR, "open(%s) failed: %s\n", filename, strerror(errno));
      return;
   }

   /*note: could have also just MMAP'd the file, if supported */
   fstat(fileno(p_file), &my_stat);

   data_len = my_stat.st_size;
   data     = (char *)malloc(data_len + 1);
   fread(data, data_len, 1, p_file);
   data[data_len] = 0;
   fclose(p_file);

   LOG_FMT(LSYS, "Parsing: %s as language %s\n",
           filename, language_to_string(cpd.lang_flags));

   if (pfout == NULL)
   {
      char outname[512];

      snprintf(outname, sizeof(outname), "%s.uncrustify", filename);

      pfout = fopen(outname, "wb");
      if (pfout == NULL)
      {
         LOG_FMT(LERR, "Unable to create %s: %s (%d)\n",
                 outname, strerror(errno), errno);
         free(data);
         return;
      }
      did_open = true;
      //LOG_FMT(LSYS, "Output file %s\n", outname);
   }

   uncrustify_file(data, data_len, pfout, parsed_file);

   free(data);

   if (did_open)
   {
      fclose(pfout);
   }
}


static void uncrustify_file(const char *data, int data_len, FILE *pfout,
                            const char *parsed_file)
{
   /**
    * Parse the text into chunks
    */
   tokenize(data, data_len);

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
    * Look at all colons ':' and mark labels, :? sequences, etc.
    */
   combine_labels();

   /**
    * Change virtual braces into real braces...
    */
   do_braces();

   /**
    * Insert line breaks as needed
    */
   do_blank_lines();
   newlines_cleanup_braces();
   if (cpd.settings[UO_nl_squeeze_ifdef].b)
   {
      newlines_squeeze_ifdef();
   }
   if (cpd.settings[UO_nl_bool_pos].n != 0)
   {
      newlines_bool_pos();
   }
   newlines_eat_start_end();

   /**
    * Fix same-line inter-chunk spacing
    */
   space_text();

   mark_comments();

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
   indent_text();
   indent_preproc();

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
   { ".java", "JAVA", LANG_JAVA },
   { ".pawn", "PAWN", LANG_PAWN },
   { ".p",    "",     LANG_PAWN },
   { ".sma",  "",     LANG_PAWN },
   { ".inl",  "",     LANG_PAWN },
   { ".h",    "",     LANG_CPP  },
   { ".cxx",  "",     LANG_CPP  },
   { ".hpp",  "",     LANG_CPP  },
   { ".hxx",  "",     LANG_CPP  },
};


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

   log_str(sev, "[", 1);

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
