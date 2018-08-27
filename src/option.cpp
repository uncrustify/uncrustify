/**
 * @file option.cpp
 * Parses the options from the config file.
 *
 * @author  Ben Gardner
 * @author  Guy Maurel since version 0.62 for uncrustify4Qt
 *          October 2015, 2016
 * @license GPL v2+
 */
#include "option.h"

#include "uncrustify_types.h"
#include "args.h"
#include "prototypes.h"
#include "uncrustify_version.h"
#include "uncrustify.h"
#include "error_types.h"
#include "keywords.h"
#include <cstring>
#ifdef HAVE_STRINGS_H
#include <strings.h>  // strcasecmp()
#endif
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include "unc_ctype.h"


using namespace std;
using namespace uncrustify;


static const char *DOC_TEXT_END = R"___(
# Meaning of the settings:
#   Ignore - do not do any changes
#   Add    - makes sure there is 1 or more space/brace/newline/etc
#   Force  - makes sure there is exactly 1 space/brace/newline/etc,
#            behaves like Add in some contexts
#   Remove - removes space/brace/newline/etc
#
#
# - Token(s) can be treated as specific type(s) with the 'set' option:
#     `set tokenType tokenString [tokenString...]`
#
#     Example:
#       `set BOOL __AND__ __OR__`
#
#     tokenTypes are defined in src/token_enum.h, use them without the
#     'CT_' prefix: 'CT_BOOL' -> 'BOOL'
#
#
# - Token(s) can be treated as type(s) with the 'type' option.
#     `type tokenString [tokenString...]`
#
#     Example:
#       `type int c_uint_8 Rectangle`
#
#     This can also be achieved with `set TYPE int c_uint_8 Rectangle`
#
#
# To embed whitespace in tokenStrings use the '\' escape character, or quote
# the tokenStrings. These quotes are supported: "'`
#
#
# - Support for the auto detection of languages through the file ending can be
#   added using the 'file_ext' command.
#     `file_ext langType langString [langString..]`
#
#     Example:
#       `file_ext CPP .ch .cxx .cpp.in`
#
#     langTypes are defined in uncrusify_types.h in the lang_flag_e enum, use
#     them without the 'LANG_' prefix: 'LANG_CPP' -> 'CPP'
#
#
# - Custom macro-based indentation can be set up using 'macro-open',
#   'macro-else' and 'macro-close'.
#     `(macro-open | macro-else | macro-close) tokenString`
#
#     Example:
#       `macro-open  BEGIN_TEMPLATE_MESSAGE_MAP`
#       `macro-open  BEGIN_MESSAGE_MAP`
#       `macro-close END_MESSAGE_MAP`
#
#
)___";


map<uncrustify_options, option_map_value> option_name_map;
map<uncrustify_groups, group_map_value>   group_map;
static uncrustify_groups                  current_group; //defines the currently active options group
#ifdef DEBUG
static int                                checkGroupNumber  = -1;
static int                                checkOptionNumber = -1;
#endif // DEBUG

// print the name of the configuration file only once
bool headOfMessagePrinted = false;


//!  only compare alpha-numeric characters
static bool match_text(const char *str1, const char *str2);


//! Convert the value string to the correct type in dest.
static void convert_value(const option_map_value *entry, const char *val, op_val_t *dest);


/**
 * @brief adds an uncrustify option to the global option list
 *
 * The option group is taken from the global 'current_group' variable
 *
 * @param name        name of the option, maximal 60 characters
 * @param id          ENUM value of the option
 * @param type        kind of option r.g. AT_IARF, AT_NUM, etc.
 * @param short_desc  short human readable description
 * @param long_desc   long  human readable description
 * @param min_val     minimal value, only used for integer values
 * @param max_val     maximal value, only used for integer values
 */
static void unc_add_option(const char *name, uncrustify_options id, option_type_e type, const char *short_desc = nullptr, const char *long_desc = nullptr, int min_val = 0, int max_val = 16);


void unc_begin_group(uncrustify_groups id, const char *short_desc,
                     const char *long_desc)
{
#ifdef DEBUG
   /*
    * The order of the calls of 'unc_begin_group' in the function
    * 'register_options' is the master over all.
    * This order must be the same in the declaration of the enum uncrustify_groups
    * This will be checked here
    */
   checkGroupNumber++;
   if (checkGroupNumber != id)
   {
      // coveralls will complain
      fprintf(stderr, "FATAL: The order of 'groups for options' is not the same:\n");
      fprintf(stderr,
              "   Number in the options.cpp file = %d\n"
              "   Number in the options.h   file = %d\n"
              "   for the group '%s'\n", id, checkGroupNumber, short_desc);
      log_flush(true);
      exit(EX_SOFTWARE);
   }
#endif // DEBUG
   current_group = id;

   group_map_value value;

   value.id         = id;
   value.short_desc = short_desc;
   value.long_desc  = long_desc;

   group_map[id] = value;
}


static void unc_add_option(const char *name, uncrustify_options id,
                           option_type_e type, const char *short_desc,
                           const char *long_desc, int min_val, int max_val)
{
#ifdef DEBUG
   // The order of the calls of 'unc_add_option' in the function 'register_options'
   // is the master over all.
   // This order must be the same in the declaration of the enum uncrustify_options
   // This will be checked here
   checkOptionNumber++;
   if (checkOptionNumber != id)
   {
      // coveralls will complain
      fprintf(stderr, "FATAL: The order of 'options' is not the same:\n");
      fprintf(stderr,
              "   Number in the options.cpp file = %d\n"
              "   Number in the options.h   file = %d\n"
              "   for the option '%s'\n", id, checkOptionNumber, name);
      log_flush(true);
      exit(EX_SOFTWARE);
   }
#endif // DEBUG
#define OptionMaxLength    60
   int lengthOfTheOption = strlen(name);
   if (lengthOfTheOption > OptionMaxLength)
   {
      // coveralls will complain
      fprintf(stderr, "FATAL: length of the option name (%s) is too big (%d)\n", name, lengthOfTheOption);
      fprintf(stderr, "FATAL: the maximal length of an option name is %d characters\n", OptionMaxLength);
      log_flush(true);
      exit(EX_SOFTWARE);
   }
   group_map[current_group].options.push_back(id);

   option_map_value value;

   value.id         = id;
   value.group_id   = current_group;
   value.type       = type;
   value.name       = name;
   value.short_desc = short_desc;
   value.long_desc  = long_desc;
   value.min_val    = 0;

   // Calculate the max/min values
   switch (type)
   {
   case AT_BOOL:
      value.max_val = 1;
      break;

   case AT_IARF:
      value.max_val = 3;
      break;

   case AT_NUM:
      value.min_val = min_val;
      value.max_val = max_val;
      break;

   case AT_UNUM:
      value.min_val = min_val;
      value.max_val = max_val;
      break;

   case AT_LINE:
      value.max_val = 3;
      break;

   case AT_POS:
      value.max_val = 2;
      break;

   case AT_STRING:
      value.max_val = 0;
      break;

   default:
      // coveralls will complain
      fprintf(stderr, "FATAL: %s(%d): Illegal option type %d for '%s'\n",
              __func__, __LINE__, static_cast<int>(type), name);
      log_flush(true);
      exit(EX_SOFTWARE);
   }

   option_name_map[id] = value;
} // unc_add_option


static bool match_text(const char *str1, const char *str2)
{
   int matches = 0;

   while (*str1 != 0 && *str2 != 0)
   {
      if (!unc_isalnum(*str1))
      {
         str1++;
         continue;
      }
      if (!unc_isalnum(*str2))
      {
         str2++;
         continue;
      }
      if (unc_tolower(*str1) != unc_tolower(*str2))
      {
         return(false);
      }
      matches++;
      str1++;
      str2++;
   }
   return(  matches
         && (*str1 == 0)
         && (*str2 == 0));
}


const option_map_value *unc_find_option(const char *name)
{
   for (const auto &it : option_name_map)
   {
      if (match_text(it.second.name, name))
      {
         return(&it.second);
      }
   }
   return(nullptr);
}


void register_options(void)
{
   // TODO
}


const group_map_value *get_group_name(size_t ug)
{
   for (const auto &it : group_map)
   {
      if (it.second.id == ug)
      {
         return(&it.second);
      }
   }
   return(nullptr);
}


const option_map_value *get_option_name(uncrustify_options option)
{
   const option_name_map_it it = option_name_map.find(option);

   return((it == option_name_map.end()) ? nullptr : (&it->second));
}


static void convert_value(const option_map_value *entry, const char *val, op_val_t *dest)
{
   if (entry->type == AT_LINE)
   {
      if (strcasecmp(val, "CRLF") == 0)
      {
         dest->le = LE_CRLF;
         return;
      }
      if (strcasecmp(val, "LF") == 0)
      {
         dest->le = LE_LF;
         return;
      }
      if (strcasecmp(val, "CR") == 0)
      {
         dest->le = LE_CR;
         return;
      }
      if (strcasecmp(val, "AUTO") != 0)
      {
         fprintf(stderr, "convert_value: %s:%d Expected 'Auto', 'LF', 'CRLF', or 'CR' for %s, got '%s'\n",
                 cpd.filename.c_str(), cpd.line_number, entry->name, val);
         log_flush(true);
         cpd.error_count++;
      }
      dest->le = LE_AUTO;
      return;
   }

   if (entry->type == AT_POS)
   {
      if (strcasecmp(val, "JOIN") == 0)
      {
         dest->tp = TP_JOIN;
         return;
      }
      if (strcasecmp(val, "LEAD") == 0)
      {
         dest->tp = TP_LEAD;
         return;
      }
      if (strcasecmp(val, "LEAD_BREAK") == 0)
      {
         dest->tp = TP_LEAD_BREAK;
         return;
      }
      if (strcasecmp(val, "LEAD_FORCE") == 0)
      {
         dest->tp = TP_LEAD_FORCE;
         return;
      }
      if (strcasecmp(val, "TRAIL") == 0)
      {
         dest->tp = TP_TRAIL;
         return;
      }
      if (strcasecmp(val, "TRAIL_BREAK") == 0)
      {
         dest->tp = TP_TRAIL_BREAK;
         return;
      }
      if (strcasecmp(val, "TRAIL_FORCE") == 0)
      {
         dest->tp = TP_TRAIL_FORCE;
         return;
      }
      if (strcasecmp(val, "IGNORE") != 0)
      {
         fprintf(stderr, "convert_value: %s:%d Expected 'Ignore', 'Join', 'Lead', 'Lead_Brake', "
                 "'Lead_Force', 'Trail', 'Trail_Break', 'Trail_Force' for %s, got '%s'\n",
                 cpd.filename.c_str(), cpd.line_number, entry->name, val);
         log_flush(true);
         cpd.error_count++;
      }
      dest->tp = TP_IGNORE;
      return;
   }

   const option_map_value *tmp;
   if (entry->type == AT_NUM || entry->type == AT_UNUM)
   {
      if (  unc_isdigit(*val)
         || (  unc_isdigit(val[1])
            && ((*val == '-') || (*val == '+'))))
      {
         if (entry->type == AT_UNUM && (*val == '-'))
         {
            fprintf(stderr, "%s: line %d\n  for the option '%s' is a negative value not possible: %s",
                    cpd.filename.c_str(), cpd.line_number, entry->name, val);
            log_flush(true);
            exit(EX_CONFIG);
         }
         if (entry->type == AT_NUM)
         {
            int n = strtol(val, nullptr, 0);
            // test the ranges Issue #672
            if (n < entry->min_val)
            {
               fprintf(stderr, "%s: line %d\n  for the option '%s' the value: %d is less than the min value: %d\n",
                       cpd.filename.c_str(), cpd.line_number, entry->name, n, entry->min_val);
               log_flush(true);
               exit(EX_CONFIG);
            }
            if (n > entry->max_val)
            {
               fprintf(stderr, "%s: line %d\n  for the option '%s' the value: %d is bigger than the max value: %d\n",
                       cpd.filename.c_str(), cpd.line_number, entry->name, n, entry->max_val);
               log_flush(true);
               exit(EX_CONFIG);
            }
            dest->n = n;
         }
         else
         {
            size_t u = strtoul(val, nullptr, 0);
            // test the ranges
            if (u > (size_t)entry->max_val)
            {
               fprintf(stderr, "%s: line %d\n  for the option '%s' the value: %zu is bigger than the max value: %d\n",
                       cpd.filename.c_str(), cpd.line_number, entry->name, u, entry->max_val);
               log_flush(true);
               exit(EX_CONFIG);
            }
            dest->u = u;
         }
         return;
      }

      // Try to see if it is a variable
      int mult = 1;
      if (*val == '-')
      {
         mult = -1;
         val++;
      }

      tmp = unc_find_option(val);
      if (tmp == nullptr)
      {
         fprintf(stderr, "%s:%d\n  for the assignment: unknown option '%s':",
                 cpd.filename.c_str(), cpd.line_number, val);
         log_flush(true);
         exit(EX_CONFIG);
      }

      // indent_case_brace = -indent_columns
      if (!headOfMessagePrinted)
      {
         LOG_FMT(LNOTE, "%s(%d): the configuration file is: %s\n",
                 __func__, __LINE__, cpd.filename.c_str());
         headOfMessagePrinted = true;
      }
      LOG_FMT(LNOTE, "%s(%d): line_number is %d, entry(%s) %s, tmp(%s) %s\n",
              __func__, __LINE__, cpd.line_number, get_argtype_name(entry->type),
              entry->name, get_argtype_name(tmp->type), tmp->name);

      if (tmp->type == AT_UNUM || tmp->type == AT_NUM)
      {
         long tmp_val;
         if (tmp->type == AT_UNUM)
         {
            tmp_val = cpd.settings[tmp->id].u * mult;
         }
         else
         {
            tmp_val = cpd.settings[tmp->id].n * mult;
         }

         if (entry->type == AT_NUM)
         {
            dest->n = tmp_val;
            return;
         }
         if (tmp_val >= 0) //dest->type == AT_UNUM
         {
            dest->u = tmp_val;
            return;
         }
         fprintf(stderr, "%s:%d\n  for the assignment: option '%s' could not have negative value %ld",
                 cpd.filename.c_str(), cpd.line_number, entry->name, tmp_val);
         log_flush(true);
         exit(EX_CONFIG);
      }

      fprintf(stderr, "%s:%d\n  for the assignment: expected type for %s is %s, got %s\n",
              cpd.filename.c_str(), cpd.line_number,
              entry->name, get_argtype_name(entry->type), get_argtype_name(tmp->type));
      log_flush(true);
      exit(EX_CONFIG);
   }

   if (entry->type == AT_BOOL)
   {
      if (  (strcasecmp(val, "true") == 0)
         || (strcasecmp(val, "t") == 0)
         || (strcmp(val, "1") == 0))
      {
         dest->b = true;
         return;
      }

      if (  (strcasecmp(val, "false") == 0)
         || (strcasecmp(val, "f") == 0)
         || (strcmp(val, "0") == 0))
      {
         dest->b = false;
         return;
      }

      bool btrue = true;
      if ((*val == '-') || (*val == '~'))
      {
         btrue = false;
         val++;
      }

      if (  ((tmp = unc_find_option(val)) != nullptr)
         && tmp->type == entry->type)
      {
         dest->b = cpd.settings[tmp->id].b ? btrue : !btrue;
         return;
      }

      fprintf(stderr, "convert_value: %s:%d Expected 'True' or 'False' for %s, got '%s'\n",
              cpd.filename.c_str(), cpd.line_number, entry->name, val);
      log_flush(true);
      cpd.error_count++;
      dest->b = false;
      return;
   }

   if (entry->type == AT_STRING)
   {
      dest->str = strdup(val);
      return;
   }

   // Must be AT_IARF
   if ((strcasecmp(val, "add") == 0) || (strcasecmp(val, "a") == 0))
   {
      dest->a = IARF_ADD;
      return;
   }
   if ((strcasecmp(val, "remove") == 0) || (strcasecmp(val, "r") == 0))
   {
      dest->a = IARF_REMOVE;
      return;
   }
   if ((strcasecmp(val, "force") == 0) || (strcasecmp(val, "f") == 0))
   {
      dest->a = IARF_FORCE;
      return;
   }
   if ((strcasecmp(val, "ignore") == 0) || (strcasecmp(val, "i") == 0))
   {
      dest->a = IARF_IGNORE;
      return;
   }
   if (  ((tmp = unc_find_option(val)) != nullptr)
      && tmp->type == entry->type)
   {
      dest->a = cpd.settings[tmp->id].a;
      return;
   }
   fprintf(stderr, "convert_value: %s:%d Expected 'Add', 'Remove', 'Force', or 'Ignore' for %s, got '%s'\n",
           cpd.filename.c_str(), cpd.line_number, entry->name, val);
   log_flush(true);
   cpd.error_count++;
   dest->a = IARF_IGNORE;
} // convert_value


int set_option_value(const char *name, const char *value)
{
   const option_map_value *entry;

   if ((entry = unc_find_option(name)) != nullptr)
   {
      convert_value(entry, value, &cpd.settings[entry->id]);
      return(entry->id);
   }
   return(-1);
}


bool is_path_relative(const char *path)
{
#ifdef WIN32
   /*
    * Check for partition labels as indication for an absolute path
    * X:\path\to\file style absolute disk path
    */
   if (isalpha(path[0]) && path[1] == ':')
   {
      return(false);
   }

   /*
    * Check for double backslashs as indication for a network path
    * \\server\path\to\file style absolute UNC path
    */
   if (path[0] == '\\' && path[1] == '\\')
   {
      return(false);
   }
#endif

   /*
    * check fo a slash as indication for a filename with leading path
    * /path/to/file style absolute path
    */
   return(path[0] != '/');
}


void process_option_line(char *configLine, const char *filename)
{
   cpd.line_number++;

   char *ptr;
   // Chop off trailing comments
   if ((ptr = strchr(configLine, '#')) != nullptr)
   {
      *ptr = 0;
   }

   // Blow away the '=' to make things simple
   if ((ptr = strchr(configLine, '=')) != nullptr)
   {
      *ptr = ' ';
   }

   // Blow away all commas
   ptr = configLine;
   while ((ptr = strchr(ptr, ',')) != nullptr)
   {
      *ptr = ' ';
   }

   // Split the line
   char *args[32];
   int  argc = Args::SplitLine(configLine, args, ARRAY_SIZE(args) - 1);
   if (argc < 2)
   {
      if (argc > 0)
      {
         fprintf(stderr, "%s:%d Wrong number of arguments: %s...\n",
                 filename, cpd.line_number, configLine);
         log_flush(true);
         cpd.error_count++;
      }
      return;
   }
   args[argc] = nullptr;

   if (strcasecmp(args[0], "type") == 0)
   {
      for (int idx = 1; idx < argc; idx++)
      {
         add_keyword(args[idx], CT_TYPE);
      }
   }
   else if (strcasecmp(args[0], "macro-open") == 0)
   {
      add_keyword(args[1], CT_MACRO_OPEN);
   }
   else if (strcasecmp(args[0], "macro-close") == 0)
   {
      add_keyword(args[1], CT_MACRO_CLOSE);
   }
   else if (strcasecmp(args[0], "macro-else") == 0)
   {
      add_keyword(args[1], CT_MACRO_ELSE);
   }
   else if (strcasecmp(args[0], "set") == 0)
   {
      if (argc < 3)
      {
         fprintf(stderr, "%s:%d 'set' requires at least three arguments\n",
                 filename, cpd.line_number);
         log_flush(true);
      }
      else
      {
         c_token_t token = find_token_name(args[1]);
         if (token != CT_NONE)
         {
            LOG_FMT(LNOTE, "%s:%d set '%s':", filename, cpd.line_number, args[1]);
            for (int idx = 2; idx < argc; idx++)
            {
               LOG_FMT(LNOTE, " '%s'", args[idx]);
               add_keyword(args[idx], token);
            }
            LOG_FMT(LNOTE, "\n");
         }
         else
         {
            fprintf(stderr, "%s:%d unknown type '%s':", filename, cpd.line_number, args[1]);
            log_flush(true);
         }
      }
   }
#ifndef EMSCRIPTEN
   else if (strcasecmp(args[0], "include") == 0)
   {
      int save_line_no = cpd.line_number;

      if (is_path_relative(args[1]))
      {
         // include is a relative path to the current config file
         unc_text ut = filename;
         ut.resize(path_dirname_len(filename));
         ut.append(args[1]);
         UNUSED(load_option_file(ut.c_str()));
      }
      else
      {
         // include is an absolute Unix path
         UNUSED(load_option_file(args[1]));
      }

      cpd.line_number = save_line_no;
   }
#endif
   else if (strcasecmp(args[0], "file_ext") == 0)
   {
      if (argc < 3)
      {
         fprintf(stderr, "%s:%d 'file_ext' requires at least three arguments\n",
                 filename, cpd.line_number);
         log_flush(true);
      }
      else
      {
         for (int idx = 2; idx < argc; idx++)
         {
            const char *lang_name = extension_add(args[idx], args[1]);
            if (lang_name)
            {
               LOG_FMT(LNOTE, "%s:%d file_ext '%s' => '%s'\n",
                       filename, cpd.line_number, args[idx], lang_name);
            }
            else
            {
               fprintf(stderr, "%s:%d file_ext has unknown language '%s'\n",
                       filename, cpd.line_number, args[1]);
               log_flush(true);
            }
         }
      }
   }
   else
   {
      // must be a regular option = value
      const int id = set_option_value(args[0], args[1]);
      if (id < 0)
      {
         fprintf(stderr, "%s:%d Unknown symbol '%s'\n",
                 filename, cpd.line_number, args[0]);
         log_flush(true);
         cpd.error_count++;
      }
   }
} // process_option_line


int load_option_file(const char *filename)
{
   cpd.line_number = 0;

#ifdef WIN32
   // "/dev/null" not understood by "fopen" in Windows
   if (strcasecmp(filename, "/dev/null") == 0)
   {
      return(0);
   }
#endif

   FILE *pfile = fopen(filename, "r");
   if (pfile == nullptr)
   {
      fprintf(stderr, "%s: fopen(%s) failed: %s (%d)\n",
              __func__, filename, strerror(errno), errno);
      log_flush(true);
      cpd.error_count++;
      return(-1);
   }

   // Read in the file line by line
   char buffer[256];
   while (fgets(buffer, sizeof(buffer), pfile) != nullptr)
   {
      process_option_line(buffer, filename);
   }

   fclose(pfile);
   return(0);
} // load_option_file


const char *get_eol_marker()
{
   static char                eol[3] = { 0x0A, 0x00, 0x00 };

   const unc_text::value_type &lines = cpd.newline.get();

   for (size_t i = 0; i < lines.size(); ++i)
   {
      eol[i] = (char)lines[i];
   }

   return(eol);
}


int save_option_file_kernel(FILE *pfile, bool withDoc, bool only_not_default)
{
   int        count_the_not_default_options = 0;

   const char *eol_marker = get_eol_marker();

   fprintf(pfile, "# %s%s", UNCRUSTIFY_VERSION, eol_marker);

   // Print the options by group
   for (auto &jt : group_map)
   {
      bool first = true;

      for (auto option_id : jt.second.options)
      {
         const auto *option     = get_option_name(option_id);
         const auto val_string  = op_val_to_string(option->type, cpd.settings[option->id]);
         const auto val_default = op_val_to_string(option->type, cpd.defaults[option->id]);

         if (val_string != val_default)
         {
            count_the_not_default_options++;
         }
         else if (only_not_default)
         {
            continue;
         }
         // ...................................................................

         if (  withDoc
            && option->short_desc != nullptr
            && (*option->short_desc != 0))
         {
            if (first)
            {
               fprintf(pfile, "%s#%s", eol_marker, eol_marker);
               fprintf(pfile, "# %s\n", jt.second.short_desc);
               fprintf(pfile, "#%s%s", eol_marker, eol_marker);
            }

            fprintf(pfile, "%s# ", first ? "" : eol_marker);

            auto idx = 0;
            for ( ; option->short_desc[idx] != 0; idx++)
            {
               fputc(option->short_desc[idx], pfile);
               if (  option->short_desc[idx] == '\n'
                  && option->short_desc[idx + 1] != 0)
               {
                  fputs("# ", pfile);
               }
            }

            if (option->short_desc[idx - 1] != '\n')
            {
               fputs(eol_marker, pfile);
            }
         }
         first = false;

         const auto name_len = strlen(option->name);
         const int  pad      = (name_len < MAX_OPTION_NAME_LEN)
                               ? (MAX_OPTION_NAME_LEN - name_len) : 1;

         fprintf(pfile, "%s%*.s= ", option->name, pad, " ");

         if (option->type == AT_STRING)
         {
            fprintf(pfile, "\"%s\"", val_string.c_str());
         }
         else
         {
            fprintf(pfile, "%s", val_string.c_str());
         }
         if (withDoc)
         {
            const int val_len = val_string.length();
            fprintf(pfile, "%*.s # %s", 8 - val_len, " ",
                    argtype_to_string(option->type).c_str());
         }
         fputs(eol_marker, pfile);
      }
   }

   if (withDoc)
   {
      fprintf(pfile, "%s", DOC_TEXT_END);
   }

   print_keywords(pfile);    // Print custom keywords
   print_extensions(pfile);  // Print custom file extensions

   fprintf(pfile, "# option(s) with 'not default' value: %d%s#%s", count_the_not_default_options, eol_marker, eol_marker);

   return(0);
} // save_option_file_kernel


int save_option_file(FILE *pfile, bool withDoc)
{
   return(save_option_file_kernel(pfile, withDoc, false));
}


void set_option_defaults(void)
{
#ifdef DEBUG
   // test all the default values if they are in the allowed interval
   for (const auto &id : option_name_map)
   {
      option_map_value value = id.second;
      if (value.type == AT_UNUM)
      {
         size_t min_value     = value.min_val;
         size_t max_value     = value.max_val;
         size_t default_value = cpd.defaults[id.first].u;
         if (default_value > max_value)
         {
            // coveralls will complain
            fprintf(stderr, "option '%s' is not correctly set:\n", id.second.name);
            fprintf(stderr, "The default value '%zu' is more than the max value '%zu'.\n",
                    default_value, max_value);
            log_flush(true);
            exit(EX_SOFTWARE);
         }
         if (min_value > 0 && default_value < min_value)
         {
            // coveralls will complain
            fprintf(stderr, "option '%s' is not correctly set:\n", id.second.name);
            fprintf(stderr, "The default value '%zu' is less than the min value '%zu'.\n",
                    default_value, min_value);
            log_flush(true);
            exit(EX_SOFTWARE);
         }
      }

      if (value.type == AT_NUM)
      {
         int min_value     = value.min_val;
         int max_value     = value.max_val;
         int default_value = cpd.defaults[id.first].n;
         if (default_value > max_value)
         {
            // coveralls will complain
            fprintf(stderr, "option '%s' is not correctly set:\n", id.second.name);
            fprintf(stderr, "The default value '%d' is more than the max value '%d'.\n",
                    default_value, max_value);
            log_flush(true);
            exit(EX_SOFTWARE);
         }
         if (default_value < min_value)
         {
            // coveralls will complain
            fprintf(stderr, "option '%s' is not correctly set:\n", id.second.name);
            fprintf(stderr, "The default value '%d' is less than the min value '%d'.\n",
                    default_value, min_value);
            log_flush(true);
            exit(EX_SOFTWARE);
         }
      }
   }
#endif // DEBUG

   // copy all the default values to settings array
   for (unsigned int count = 0; count < UO_option_count; count++)
   {
      cpd.settings[count] = cpd.defaults[count];
   }
} // set_option_defaults


string argtype_to_string(option_type_e argtype)
{
   switch (argtype)
   {
   case AT_BOOL:
      return("false/true");

   case AT_IARF:
      return("ignore/add/remove/force");

   case AT_NUM:
      return("number");

   case AT_LINE:
      return("auto/lf/crlf/cr");

   case AT_POS:
      return("ignore/join/lead/lead_break/lead_force/trail/trail_break/trail_force");

   case AT_STRING:
      return("string");

   case AT_UNUM:
      return("unsigned number");

   default:
      // coveralls will complain
      fprintf(stderr, "%s(%d): Unknown argtype '%d'\n",
              __func__, __LINE__, static_cast<int>(argtype));
      log_flush(true);
      exit(EX_SOFTWARE);
   }
}


const char *get_argtype_name(option_type_e argtype)
{
   switch (argtype)
   {
   case AT_BOOL:
      return("AT_BOOL");

   case AT_IARF:
      return("AT_IARF");

   case AT_NUM:
      return("AT_NUM");

   case AT_LINE:
      return("AT_LINE");

   case AT_POS:
      return("AT_POS");

   case AT_STRING:
      return("AT_STRING");

   case AT_UNUM:
      return("AT_UNUM");

   default:
      // coveralls will complain
      fprintf(stderr, "%s(%d): Unknown argtype '%d'\n",
              __func__, __LINE__, static_cast<int>(argtype));
      log_flush(true);
      exit(EX_SOFTWARE);
   }
}


string bool_to_string(bool val)
{
   if (val)
   {
      return("true");
   }

   return("false");
}


string argval_to_string(iarf_e argval)
{
   switch (argval)
   {
   case IARF_IGNORE:
      return("ignore");

   case IARF_ADD:
      return("add");

   case IARF_REMOVE:
      return("remove");

   case IARF_FORCE:
      return("force");

   default:
      // coveralls will complain
      fprintf(stderr, "%s(%d): Unknown argval '%d'\n",
              __func__, __LINE__, static_cast<int>(argval));
      log_flush(true);
      return("");
   }
}


string lineends_to_string(lineend_e linends)
{
   switch (linends)
   {
   case LE_LF:
      return("lf");

   case LE_CRLF:
      return("crlf");

   case LE_CR:
      return("cr");

   case LE_AUTO:
      return("auto");

   default:
      // coveralls will complain
      fprintf(stderr, "%s(%d): Unknown lineends '%d'\n",
              __func__, __LINE__, static_cast<int>(linends));
      log_flush(true);
      return("");
   }
}


string tokenpos_to_string(tokenpos_e tokenpos)
{
   switch (tokenpos)
   {
   case TP_IGNORE:
      return("ignore");

   case TP_JOIN:
      return("join");

   case TP_LEAD:
      return("lead");

   case TP_LEAD_BREAK:
      return("lead_break");

   case TP_LEAD_FORCE:
      return("lead_force");

   case TP_TRAIL:
      return("trail");

   case TP_TRAIL_BREAK:
      return("trail_break");

   case TP_TRAIL_FORCE:
      return("trail_force");

   default:
      // coveralls will complain
      fprintf(stderr, "%s(%d) Unknown tokenpos '%d'\n",
              __func__, __LINE__, static_cast<int>(tokenpos));
      log_flush(true);
      return("");
   }
}


string op_val_to_string(option_type_e argtype, op_val_t op_val)
{
   switch (argtype)
   {
   case AT_BOOL:
      return(bool_to_string(op_val.b));

   case AT_IARF:
      return(argval_to_string(op_val.a));

   case AT_NUM:
      return(to_string(op_val.n));

   case AT_UNUM:
      return(to_string(op_val.u));

   case AT_LINE:
      return(lineends_to_string(op_val.le));

   case AT_POS:
      return(tokenpos_to_string(op_val.tp));

   case AT_STRING:
      return(op_val.str != nullptr ? op_val.str : "");

   default:
      // coveralls will complain
      fprintf(stderr, "%s(%d): Unknown argtype '%d'\n",
              __func__, __LINE__, static_cast<int>(argtype));
      log_flush(true);
      exit(EX_SOFTWARE);
   }
}
