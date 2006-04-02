/**
 * This file takes an input C/C++/D/Java file and reformats it.
 *
 * $Id$
 */

module uncrustify.main;

import std.cstream;
import uncrustify.args;
import uncrustify.log;
import uncrustify.settings;
import uncrustify.options;
import std.stdio;

char [] str_version = "0.0.12";
char [] str_package = "uncrustify-d";


static void usage_exit(char [] msg, char [] argv0, int code)
{
   if ((msg !is null) && (msg.length > 0))
   {
      writef("\n%s\n", msg);
   }
   writef("\nUsage:\n"
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
   //exit(code);
}

static void version_exit()
{
   writef("%s %s\n", str_package, str_version);
   //exit(0);
}

int main(char [][] args)
{
   Args arg = new Args(args[1..args.length]);

   if ((args.length == 1) ||
       arg.Present("--version") ||
       arg.Present("--help") ||
       arg.Present("--usage") ||
       arg.Present("-h"))
   {
      usage_exit(null, args[0], 0);
      return 0;
   }

   /* Grab the config file */
   char [] cfg_file;
   if (((cfg_file = arg.Param("-c")) is null) &&
       ((cfg_file = arg.Param("--config")) is null))
   {
      usage_exit("Missing parameter: -c config", args[0], 100);
      return 100;
   }

   /* Grab the source file */
   char [] source_file;
   if (((source_file = arg.Param("-f")) is null) &&
       ((source_file = arg.Param("--file")) is null))
   {
      usage_exit("Missing parameter: -f source", args[0], 101);
      return 101;
   }

   if (!std.file.exists(source_file))
   {
      writef("File not found: %s\n", source_file);
      return 102;
   }

   unc = new Uncrustify();

   /* Load the type files */
   int idx = 0;
   char [] tmp_str;
   while ((tmp_str = arg.Params("-t", idx)) !is null)
   {
      writef("Type file: %s\n", tmp_str);
   }

   char [] parsed_file;
   if ((parsed_file = arg.Param("-p")) !is null)
   {
      writef("Parsed file: %s\n", parsed_file);
   }

   if (((tmp_str = arg.Param("-L")) !is null) ||
       ((tmp_str = arg.Param("--log")) !is null))
   {
      writef("Log Sev: %s\n", tmp_str);

      unc.log.mask.FromString(tmp_str);

      //printf("Result: %.*s\n", unc.log.mask.ToString());
   }

   if ((tmp_str = arg.Param("-l")) !is null)
   {
      writef("Language: %s\n", tmp_str);
   }

   if (arg.Present("-s") || arg.Present("--show"))
   {
      writef("Enable Sev display\n");
   }

   idx = 0;
   while ((tmp_str = arg.GetUnused(idx)) !is null)
   {
      writef("Unused param at index %d: %s\n", idx, tmp_str);
   }

   byte [] filedata = cast(byte []) std.file.read(source_file);

   unc.log.LogHexBlock(0, filedata);

   //for (int idx = 0; idx < options_table.length; idx++)
   //{
   //   OptionEntry *oe;
   //
   //   oe = FindOptionEntry(options_table[idx].name);
   //   if (oe is null)
   //   {
   //      writef("%d] didn't find %s\n", idx, options_table[idx].name);
   //   }
   //   else
   //   {
   //      writef("%d] found it: id=%d name=%s type=%d\n", idx, oe.id, oe.name, oe.type);
   //   }
   //}

   LoadOptionFile(cfg_file);

   return 0;
}

class Uncrustify
{
   public Log     log;
   public int []  settings;

   this()
   {
      log = new Log();
      settings.length = Option.option_count;
   }
}

public Uncrustify unc;

//    struct stat          my_stat;
//    char                 *data;
//    int                  data_len;
//    FILE                 *p_file;
//    const char           *cfg_file    = "uncrustify.cfg";
//    const char           *parsed_file = NULL;
//    const char           *source_file = NULL;
//    log_mask_t           mask;
//    int                  op;
//    int                  option_index   = 0;
//    static struct option long_options[] =
//    {
//       { "version", 0, 0,   0 },
//       { "file",    1, 0, 'f' },
//       { "config",  1, 0, 'c' },
//       { "parsed",  1, 0, 'p' },
//       { "log",     1, 0, 'L' },
//       { "show",    1, 0, 's' },
//       { NULL,      0, 0,   0 }
//    };
//
//    if (argc < 2)
//    {
//       usage_exit(NULL, argv[0], 0);
//    }
//
//    log_init(stderr);
//
//    memset(&cpd, 0, sizeof(cpd));
//
//    chunk_list_init();
//
//    while ((op = getopt_long(argc, argv, "c:p:f:L:t:l:s",
//                             long_options, &option_index)) != EOF)
//    {
//       switch (op)
//       {
//       case 0:
//          if (option_index == 0)
//          {
//             version_exit();
//          }
//          break;
//
//       case 'c':
//          // Path to config file.
//          cfg_file = optarg;
//          break;
//
//       case 'p':
//          // Path to parse output.
//          parsed_file = optarg;
//          break;
//
//       case 'l':   // language override
//          cpd.lang_flags = language_from_tag(optarg);
//          if (cpd.lang_flags == 0)
//          {
//             fprintf(stderr, "Ignoring unknown language: %s\n", optarg);
//          }
//          break;
//
//       case 'f':
//          // Path to input file.
//          source_file = optarg;
//          break;
//
//       case 'L':
//          logmask_from_string(optarg, &mask);
//          log_set_mask(&mask);
//          break;
//
//       case 's':
//          log_show_sev(TRUE);
//          break;
//
//       case 't':
//          load_keyword_file(optarg);
//          break;
//
//       default:
//          usage_exit("Bad command line option", argv[0], 1);
//          break;
//       }
//    }
//
//    if (source_file == NULL)
//    {
//       usage_exit("Specify the file to process: -f file", argv[0], 57);
//    }
//
//    set_arg_defaults();
//
//    if (load_config_file(cfg_file) < 0)
//    {
//       usage_exit(NULL, argv[0], 56);
//    }
//
//    /* Do some simple language detection based on the filename */
//    if (cpd.lang_flags == 0)
//    {
//       cpd.lang_flags = language_from_filename(source_file);
//    }
//
//    p_file = fopen(source_file, "r");
//    if (p_file == NULL)
//    {
//       LOG_FMT(LERR, "open(%s) failed: %s\n", source_file, strerror(errno));
//       return(1);
//    }
//
//    fstat(fileno(p_file), &my_stat);
//
//    data_len = my_stat.st_size;
//    data     = malloc(data_len + 1);
//    fread(data, data_len, 1, p_file);
//    data[data_len] = 0;
//    fclose(p_file);
//
//    LOG_FMT(LSYS, "Parsing: %s as language %s\n",
//            source_file, language_to_string(cpd.lang_flags));
//
//    /**
//     * Parse the text into chunks
//     */
//    tokenize(data, data_len);
//
//    /**
//     * Change certain token types based on simple sequence.
//     * Example: change '[' + ']' to '[]'
//     * Note that level info is not yet available, so it is OK to do all
//     * processing that doesn't need to know level info. (that's very little!)
//     */
//    tokenize_cleanup();
//
//    /**
//     * Detect the brace and paren levels and insert virtual braces.
//     * This handles all that nasty preprocessor stuff
//     */
//    brace_cleanup();
//
//    /**
//     * At this point, the level information is available and accurate.
//     */
//
//    /**
//     * Re-type chunks, combine chunks
//     */
//    fix_symbols();
//
//    /**
//     * Look at all colons ':' and mark labels, :? sequences, etc.
//     */
//    combine_labels();
//
//    /**
//     * Change virtual braces into real braces...
//     */
//    do_braces();
//
//    /**
//     * Insert line breaks as needed
//     */
//    newlines_cleanup_braces();
//    if (cpd.settings[UO_nl_squeeze_ifdef])
//    {
//       newlines_squeeze_ifdef();
//    }
//
//    /**
//     * Fix same-line inter-chunk spacing
//     */
//    space_text();
//
//    mark_comments();
//
//    /**
//     * Do any aligning of preprocessors
//     */
//    if (cpd.settings[UO_align_pp_define_span] > 0)
//    {
//       align_preprocessor();
//    }
//
//    /**
//     * Indent the text
//     */
//    indent_text();
//
//    /**
//     * Aligning everything else and reindent
//     */
//    align_all();
//    indent_text();
//
//    /**
//     * And finally, align the backslash newline stuff
//     */
//    align_right_comments();
//    if (cpd.settings[UO_align_nl_cont] != 0)
//    {
//       align_backslash_newline();
//    }
//
//    if (parsed_file != NULL)
//    {
//       p_file = fopen(parsed_file, "w");
//       if (p_file != NULL)
//       {
//          output_parsed(p_file);
//          fclose(p_file);
//       }
//    }
//
//    /**
//     * Now render it all to the output file
//     */
//    output_text(stdout);
//
//    return(0);
// }
//
//
// const char *get_token_name(c_token_t token)
// {
//    if ((token >= 0) && (token < ARRAY_SIZE(token_names)) &&
//        (token_names[token] != NULL))
//    {
//       return(token_names[token]);
//    }
//    return("???");
// }
//
// static BOOL ends_with(const char *filename, const char *tag)
// {
//    int len1 = strlen(filename);
//    int len2 = strlen(tag);
//
//    if ((len2 <= len1) && (strcmp(&filename[len1 - len2], tag) == 0))
//    {
//       return(TRUE);
//    }
//    return(FALSE);
// }
//
// struct file_lang
// {
//    const char *ext;
//    const char *tag;
//    int        lang;
// };
// struct file_lang languages[] =
// {
//    { ".c",    "C",    LANG_C },
//    { ".h",    "",     LANG_C },
//    { ".cpp",  "CPP",  LANG_CPP },
//    { ".d",    "D",    LANG_D },
//    { ".cs",   "CS",   LANG_CS },
//    { ".java", "JAVA", LANG_JAVA },
// };
//
// /**
//  * Find the language for the file extension
//  * Default to C
//  *
//  * @param filename   The name of the file
//  * @return           LANG_xxx
//  */
// static int language_from_filename(const char *filename)
// {
//    int i;
//
//    for (i = 0; i < ARRAY_SIZE(languages); i++)
//    {
//       if (ends_with(filename, languages[i].ext))
//       {
//          return(languages[i].lang);
//       }
//    }
//    return(LANG_C);
// }
//
// /**
//  * Find the language for the file extension
//  * Default to C
//  *
//  * @param filename   The name of the file
//  * @return           LANG_xxx
//  */
// static int language_from_tag(const char *tag)
// {
//    int i;
//
//    for (i = 0; i < ARRAY_SIZE(languages); i++)
//    {
//       if (strcasecmp(tag, languages[i].tag) == 0)
//       {
//          return(languages[i].lang);
//       }
//    }
//    return(0);
// }
//
// /**
//  * Gets the tag text for a language
//  *
//  * @param lang    The LANG_xxx enum
//  * @return        A string
//  */
// static const char *language_to_string(int lang)
// {
//    int i;
//
//    for (i = 0; i < ARRAY_SIZE(languages); i++)
//    {
//       if ((languages[i].lang & lang) != 0)
//       {
//          return(languages[i].tag);
//       }
//    }
//    return("???");
// }
//
