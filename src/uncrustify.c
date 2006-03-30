/**
 * This file takes an input C/C++/D/Java file and reformats it.
 *
 * $Id$
 */
#define DEFINE_GLOBAL_DATA

#include "cparse_types.h"
#include "char_table.h"
#include "chunk_list.h"
#include "prototypes.h"
#include "token_names.h"
#include "args.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> /* strcasecmp() */
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>


static int language_from_tag(const char *tag);
static int language_from_filename(const char *filename);
static const char *language_to_string(int lang);


static void usage_exit(const char *msg, const char *argv0, int code)
{
   if (msg != NULL)
   {
      fprintf(stderr, "%s\n", msg);
   }
   fprintf(stderr,
           "Usage:\n"
           "%s [-c cfg] [-f file] [-p parsed] [-t typefile] [--version] [-l lang] [-L sev] [-s]\n"
           " c : specify the config file\n"
           " f : specify the file to format\n"
           " p : debug - dump parsed tokens to this file\n"
           " L : debug log severities 0-255 for everything\n"
           " s : show log severities\n"
           " t : load a file with types\n"
           " l : language override: C, CPP, D, CS, JAVA\n"
           "--version : print the version and exit\n"
           "The output is dumped to stdout, errors are dumped to stderr\n",
           argv0);
   exit(code);
}

static void version_exit(void)
{
   printf("%s %s\n", PACKAGE, VERSION);
   exit(0);
}

int main(int argc, char *argv[])
{
   struct stat          my_stat;
   char                 *data;
   int                  data_len;
   FILE                 *p_file;
   const char           *cfg_file    = "uncrustify.cfg";
   const char           *parsed_file = NULL;
   const char           *source_file = NULL;
   log_mask_t           mask;
   int         idx;
   const char  *p_arg;


   if (argc < 2)
   {
      usage_exit(NULL, argv[0], 0);
   }

   arg_init(argc, argv);

   if (arg_present("--version") || arg_present("-v"))
   {
      version_exit();
   }
   if (arg_present("--help") || arg_present("-h") ||
       arg_present("--usage") || arg_present("-?"))
   {
      usage_exit(NULL, argv[0], 0);
   }

   /* Init logging */
   log_init(stderr);
   if (((p_arg = arg_param("-L")) != NULL) ||
       ((p_arg = arg_param("--log")) != NULL))
   {
      logmask_from_string(p_arg, &mask);
         log_set_mask(&mask);
   }

   /* Get the source file name */
   if (((source_file = arg_param("--file")) == NULL) &&
       ((source_file = arg_param("-f")) == NULL))
   {
      usage_exit("Specify the file to process: -f file", argv[0], 57);
   }

   /* Get the config file name */
   if (((cfg_file = arg_param("--config")) == NULL) &&
       ((cfg_file = arg_param("-c")) == NULL))
   {
      usage_exit("Specify the config file: -c file", argv[0], 58);
   }

   /* Get the parsed file name */
   if (((parsed_file = arg_param("--parsed")) != NULL) ||
       ((parsed_file = arg_param("-p")) != NULL))
   {
      LOG_FMT(LNOTE, "Will export parsed data to: %s\n", parsed_file);
   }

   /* Enable log sevs? */
   if (arg_present("-s") || arg_present("--show"))
   {
         log_show_sev(TRUE);
   }

   /* Load type files */
   idx = 0;
   while ((p_arg = arg_params("-t", &idx)) != NULL)
   {
      load_keyword_file(p_arg);
   }

   /* Check for a language override */
   if ((p_arg = arg_param("-l")) != NULL)
   {
      cpd.lang_flags = language_from_tag(p_arg);
      if (cpd.lang_flags == 0)
      {
         fprintf(stderr, "Ignoring unknown language: %s\n", p_arg);
      }
   }

   /* Prepare to load chunks */
   chunk_list_init();


   /* Check for unused args (ignore them) */
   idx = 1;
   while ((p_arg = arg_unused(&idx)) != NULL)
   {
      LOG_FMT(LWARN, "Unused argument: %s\n", p_arg);
   }

   /* Load the config file */
   set_option_defaults();
   if (load_option_file(cfg_file) < 0)
   {
      usage_exit(NULL, argv[0], 56);
   }

   /* Do some simple language detection based on the filename */
   if (cpd.lang_flags == 0)
   {
      cpd.lang_flags = language_from_filename(source_file);
   }

   /* Try to read in the source file */
   p_file = fopen(source_file, "r");
   if (p_file == NULL)
   {
      LOG_FMT(LERR, "open(%s) failed: %s\n", source_file, strerror(errno));
      return(1);
   }

   /*note: could have also just MMAP'd the file */
   fstat(fileno(p_file), &my_stat);

   data_len = my_stat.st_size;
   data     = malloc(data_len + 1);
   fread(data, data_len, 1, p_file);
   data[data_len] = 0;
   fclose(p_file);

   LOG_FMT(LSYS, "Parsing: %s as language %s\n",
           source_file, language_to_string(cpd.lang_flags));

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
   newlines_cleanup_braces();
   if (cpd.settings[UO_nl_squeeze_ifdef])
   {
      newlines_squeeze_ifdef();
   }

   /**
    * Fix same-line inter-chunk spacing
    */
   space_text();

   mark_comments();

   /**
    * Do any aligning of preprocessors
    */
   if (cpd.settings[UO_align_pp_define_span] > 0)
   {
      align_preprocessor();
   }

   /**
    * Indent the text
    */
   indent_text();

   /**
    * Aligning everything else and reindent
    */
   align_all();
   indent_text();

   /**
    * And finally, align the backslash newline stuff
    */
   align_right_comments();
   if (cpd.settings[UO_align_nl_cont] != 0)
   {
      align_backslash_newline();
   }

   if (parsed_file != NULL)
   {
      p_file = fopen(parsed_file, "w");
      if (p_file != NULL)
      {
         output_parsed(p_file);
         fclose(p_file);
      }
   }

   /**
    * Now render it all to the output file
    */
   output_text(stdout);

   return(0);
}


const char *get_token_name(c_token_t token)
{
   if ((token >= 0) && (token < ARRAY_SIZE(token_names)) &&
       (token_names[token] != NULL))
   {
      return(token_names[token]);
   }
   return("???");
}

static BOOL ends_with(const char *filename, const char *tag)
{
   int len1 = strlen(filename);
   int len2 = strlen(tag);

   if ((len2 <= len1) && (strcmp(&filename[len1 - len2], tag) == 0))
   {
      return(TRUE);
   }
   return(FALSE);
}

struct file_lang
{
   const char *ext;
   const char *tag;
   int        lang;
};
struct file_lang languages[] =
{
   { ".c",    "C",    LANG_C },
   { ".h",    "",     LANG_C },
   { ".cpp",  "CPP",  LANG_CPP },
   { ".d",    "D",    LANG_D },
   { ".cs",   "CS",   LANG_CS },
   { ".java", "JAVA", LANG_JAVA },
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

   for (i = 0; i < ARRAY_SIZE(languages); i++)
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
 * Default to C
 *
 * @param filename   The name of the file
 * @return           LANG_xxx
 */
static int language_from_tag(const char *tag)
{
   int i;

   for (i = 0; i < ARRAY_SIZE(languages); i++)
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

   for (i = 0; i < ARRAY_SIZE(languages); i++)
   {
      if ((languages[i].lang & lang) != 0)
      {
         return(languages[i].tag);
      }
   }
   return("???");
}

