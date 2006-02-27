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

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>


void process_chunk(chunk_t *pc);
void parse_file(char *data, int data_len);


static void usage_exit(const char *msg, const char *argv0, int code)
{
   if (msg != NULL)
   {
      fprintf(stderr, "%s\n", msg);
   }
   fprintf(stderr,
           "Usage:\n"
           "%s [-c cfg] [-f file] [-p parsed] [-t typefile] [--version] [-L sev] [-s]\n"
           " c : specify the config file\n"
           " f : specify the file to format\n"
           " p : debug - dump parsed tokens to this file\n"
           " L : debug log severities 0-255 for everything\n"
           " s : show log severities\n"
           " t : load a file with types\n"
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
   int                  op;
   int                  option_index   = 0;
   static struct option long_options[] = {
      { "version", 0, 0,   0 },
      { "file",    1, 0, 'f' },
      { "config",  1, 0, 'c' },
      { "parsed",  1, 0, 'p' },
      { "log",     1, 0, 'L' },
      { "show",    1, 0, 's' },
      { NULL,      0, 0,   0 }
   };

   if (argc < 2)
   {
      usage_exit(NULL, argv[0], 0);
   }

   log_init(stderr);

   memset(&cpd, 0, sizeof(cpd));

   cpd.lang_flags = LANG_C;

   chunk_list_init();

   while ((op = getopt_long(argc, argv, "c:p:f:L:t:",
                            long_options, &option_index)) != EOF)
   {
      switch (op)
      {
      case 0:
         if (option_index == 0)
         {
            version_exit();
         }
         break;

      case 'c':
         // Path to config file.
         cfg_file = optarg;
         break;

      case 'p':
         // Path to parse output.
         parsed_file = optarg;
         break;

      case 'f':
         // Path to input file.
         source_file = optarg;
         break;

      case 'L':
         logmask_from_string(optarg, &mask);
         log_set_mask(&mask);
         break;

      case 's':
         log_show_sev(TRUE);
         break;

      case 't':
         load_keyword_file(optarg);
         break;

      default:
         usage_exit("Bad command line option", argv[0], 1);
         break;
      }
   }

   set_arg_defaults();

   if (load_config_file(cfg_file) < 0)
   {
      usage_exit(NULL, argv[0], 56);
   }

   if (source_file == NULL)
   {
      usage_exit("Specify the file to process: -f file", argv[0], 57);
   }

   p_file = fopen(source_file, "r");
   if (p_file == NULL)
   {
      LOG_FMT(LERR, "open(%s) failed: %s\n", source_file, strerror(errno));
      return(1);
   }

   fstat(fileno(p_file), &my_stat);

   data_len = my_stat.st_size;
   data     = malloc(data_len + 1);
   fread(data, data_len, 1, p_file);
   data[data_len] = 0;
   fclose(p_file);

   LOG_FMT(LSYS, "Parsing: %s\n", source_file);

   /**
    * Parse the text into chunks
    */
   parse_buffer(data, data_len);

   /**
    * Re-type chunks, combine chunks, insert virtual braces
    */
   combine_labels();
   fix_symbols();

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

