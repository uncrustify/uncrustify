/**
 * @file option.cpp
 * Parses the options from the config file.
 *
 * @author  Ben Gardner
 * @author  Guy Maurel October 2015, 2021
 * @author  Matthew Woehlke since version 0.67
 * @license GPL v2+
 */
#include "option.h"

#include "keywords.h"
#include "language_names.h"
#include "uncrustify.h"
#include "uncrustify_version.h"

#include <fstream>
#include <unordered_map>

#include <cctype>                    // to get std::tolower
#include <cstdarg>                   // to get va_start, va_end


namespace uncrustify
{

namespace
{

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
#     'CT_' prefix: 'CT_BOOL' => 'BOOL'
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
#     them without the 'LANG_' prefix: 'LANG_CPP' => 'CPP'
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


std::vector<OptionGroup>                         option_groups;
std::unordered_map<std::string, GenericOption *> option_map;

#define LOG_CONFIG(...) \
   log_config(); LOG_FMT(LNOTE, __VA_ARGS__);


//-----------------------------------------------------------------------------
constexpr int option_level(int major, int minor, int patch = 0)
{
   return((major << 20) | (minor << 10) | (patch << 0));
}


//-----------------------------------------------------------------------------
void log_config()
{
   // Print the name of the configuration file only once
   static bool config_name_logged = false;

   if (!config_name_logged)
   {
      LOG_FMT(LNOTE, "log_config: the configuration file is: %s\n",
              cpd.filename.c_str());
      config_name_logged = true;
   }
}


//-----------------------------------------------------------------------------
// This identity function exists so that all Option<T>::str can simply call
// to_string(m_val); this function will be used by Option<string>
std::string to_string(const std::string &in)
{
   return(in);
}

using std::to_string;


//-----------------------------------------------------------------------------
std::string to_lower(const char *in, std::string::size_type size = 0)
{
   std::string out;

   if (size > 0)
   {
      out.reserve(size);
   }

   while (*in)
   {
      out += static_cast<char>(std::tolower(*in));
      ++in;
   }
   return(out);
}


//-----------------------------------------------------------------------------
std::string to_lower(const std::string &in)
{
   return(to_lower(in.data(), in.size()));
}


//-----------------------------------------------------------------------------
bool is_arg_sep(int ch)
{
   return(  isspace(ch)
         || ch == ','
         || ch == '=');
}


//-----------------------------------------------------------------------------
bool is_varg_sep(int ch)
{
   return(ch == '.');
}


//-----------------------------------------------------------------------------
std::vector<std::string> split_args(std::string in, const char *filename,
                                    bool (*is_sep)(int))
{
   std::vector<std::string> out;
   std::string::size_type   n = 0;
   std::string::size_type   k = in.size();

   // Parse input string
   while (n < k)
   {
      // Skip leading space
      while (  n < k
            && is_sep(in[n]))
      {
         ++n;
      }

      // Detect comments or trailing space
      if (  n >= k
         || in[n] == '#')
      {
         break;
      }

      // Detect and extract quoted string
      if (const auto *quote = strchr("\'\"`", in[n]))
      {
         const auto start = ++n;

         for ((void)n; in[n] != *quote; ++n)
         {
            if (  n < k
               && in[n] == '\\')
            {
               in.erase(n, 1);
               --k;
            }

            if (n >= k)
            {
               OptionWarning w{ filename };
               w("found unterminated quoted-string");
               return{};
            }
         }

         out.push_back(in.substr(start, n - start));

         if (  ++n < k
            && !is_sep(in[n]))
         {
            OptionWarning w{ filename };
            w("unexpected text following quoted-string");
            return{};
         }
         continue;
      }
      // Extract anything else
      const auto start = n;

      for ((void)n;
           (  n < k
           && !is_sep(in[n]));
           ++n)
      {
         if (in[n] == '\\')
         {
            in.erase(n, 1);
            --k;
         }

         if (n >= k)
         {
            OptionWarning w{ filename };
            w("found unterminated quoted-string");
            return{};
         }
      }

      out.push_back(in.substr(start, n - start));
   }
   return(out);
} // split_args


//-----------------------------------------------------------------------------
bool is_path_relative(const std::string &path)
{
   assert(!path.empty());

#ifdef WIN32
   // Check for partition labels as indication for an absolute path
   // 'X:\path\to\file' style absolute disk path
   if (  path.size() > 1
      && isalpha(path[0])
      && path[1] == ':')
   {
      return(false);
   }

   // Check for double backslashes as indication for a network path
   // '\\server\path\to\file style' absolute UNC path
   if (  path.size() > 1
      && path[0] == '\\'
      && path[1] == '\\')
   {
      return(false);
   }
#endif

   // Check for a slash as indication for a filename with leading path
   // '/path/to/file' style absolute path
   return(path[0] != '/');
}


//-----------------------------------------------------------------------------
void print_description(FILE *pfile, std::string description,
                       const char *eol_marker)
{
   // Descriptions always start with a '\n', so skip the first character
   for (std::string::size_type start = 1, length = description.length();
        (  start != std::string::npos
        && start < length);
        ++start)
   {
      // Check for empty line so we can squelch trailing whitespace
      if (description[start] == '\n')
      {
         fprintf(pfile, "#%s", eol_marker);
      }
      else
      {
         const auto end = description.find('\n', start);
         fprintf(pfile, "# %s%s",
                 description.substr(start, end - start).c_str(), eol_marker);
         start = end;
      }
   }
}


//-----------------------------------------------------------------------------
void deprecated_stop_or_not()
{
   if (cpd.find_deprecated)
   {
      exit(EX_OK);
   }
}


//-----------------------------------------------------------------------------
bool process_option_line_compat_0_68(const std::string              &cmd,
                                     const std::vector<std::string> &args,
                                     const char                     *filename)
{
   if (cmd == "sp_cpp_lambda_paren")
   {
      OptionWarning w{ filename, OptionWarning::MINOR };
      w("option '%s' is deprecated; use '%s' instead.\n",
        cmd.c_str(), options::sp_cpp_lambda_square_paren.name());
      UNUSED(options::sp_cpp_lambda_square_paren.read(args[1].c_str()));
      return(true);
   }
   return(false);
} // process_option_line_compat_0_68


bool process_option_line_compat_0_70(const std::string &cmd, const char *filename)
{
   if (cmd == "sp_word_brace")                     // Issue #2428
   {
      OptionWarning w{ filename, OptionWarning::MINOR };
      w("option '%s' is deprecated; did you want to use '%s' instead?\n",
        cmd.c_str(), options::sp_type_brace_init_lst.name());
      deprecated_stop_or_not();
      return(true);
   }
   return(false);
} // process_option_line_compat_0_70


bool process_option_line_compat_0_73(const std::string &cmd, const char *filename)
{
   if (cmd == "indent_sing_line_comments")         // Issue #3249
   {
      OptionWarning w{ filename, OptionWarning::MINOR };
      w("option '%s' is deprecated; did you want to use '%s' instead?\n",
        cmd.c_str(), options::indent_single_line_comments_before.name());
      deprecated_stop_or_not();
      return(true);
   }

   if (cmd == "sp_before_tr_emb_cmt")              // Issue #3339
   {
      OptionWarning w{ filename, OptionWarning::MINOR };
      w("option '%s' is deprecated; did you want to use '%s' instead?\n",
        cmd.c_str(), options::sp_before_tr_cmt.name());
      deprecated_stop_or_not();
      return(true);
   }

   if (cmd == "sp_num_before_tr_emb_cmt")          // Issue #3339
   {
      OptionWarning w{ filename, OptionWarning::MINOR };
      w("option '%s' is deprecated; did you want to use '%s' instead?\n",
        cmd.c_str(), options::sp_num_before_tr_cmt.name());
      deprecated_stop_or_not();
      return(true);
   }
   return(false);
} // process_option_line_compat_0_73


bool process_option_line_compat_0_74(const std::string &cmd, const char *filename)
{
   if (cmd == "sp_type_question")         // PR #3638
   {
      OptionWarning w{ filename, OptionWarning::MINOR };
      w("option '%s' is deprecated; did you want to use '%s' instead?\n",
        cmd.c_str(), options::sp_before_ptr_star.name());
      deprecated_stop_or_not();
      return(true);
   }
   return(false);
} // process_option_line_compat_0_74


bool process_option_line_compat_0_75(const std::string &cmd, const char *filename)
{
   if (cmd == "pp_space")
   {
      OptionWarning w{ filename, OptionWarning::MINOR };
      w("option '%s' is deprecated; it has been replaced by '%s'.\n",
        cmd.c_str(), options::pp_space_after.name());
      deprecated_stop_or_not();
      return(true);
   }

   if (cmd == "pp_space_before")
   {
      OptionWarning w{ filename, OptionWarning::MINOR };
      w("option '%s' is deprecated; it was a temporary option used\n"
        "during the development of version 0.76. Use '%s' and '%s' instead.\n",
        cmd.c_str(), options::pp_indent.name(), options::pp_indent_count.name());

      return(true);
   }
   return(false);
} // process_option_line_compat_0_75


bool process_option_line_compat_0_76(const std::string &cmd, const std::vector<std::string> &args, const char *filename)
{
   if (cmd == "nl_func_var_def_blk")
   {
      OptionWarning w{ filename, OptionWarning::MINOR };
      w("option '%s' is deprecated; it has been replaced by '%s'.\n"
        "You can also use '%s' for additional functionality.\n",
        cmd.c_str(), options::nl_var_def_blk_end_func_top.name(),
        options::nl_var_def_blk_end.name());
      deprecated_stop_or_not();
      UNUSED(options::nl_var_def_blk_end_func_top.read(args[1].c_str()));
      return(true);
   }
   return(false);
} // process_option_line_compat_0_76


bool process_option_line_compat_0_78(const std::string &cmd, const char *filename)
{
   if (cmd == "pp_warn_unbalanced_if")
   {
      OptionWarning w{ filename, OptionWarning::MINOR };
      w("option '%s' is deprecated; it has been replaced by '%s'.\n",
        cmd.c_str(), options::pp_unbalanced_if_action.name());
      deprecated_stop_or_not();
      return(true);
   }

   if (cmd == "sp_balance_nested_parens")
   {
      OptionWarning w{ filename, OptionWarning::MINOR };
      w("option '%s' never works; it has been replaced by '%s'.\n",
        cmd.c_str(), options::sp_paren_paren.name());
      deprecated_stop_or_not();
      return(true);
   }
   return(false);
} // process_option_line_compat_0_78

} // namespace

///////////////////////////////////////////////////////////////////////////////

//BEGIN Option<T> and helpers


//-----------------------------------------------------------------------------
OptionWarning::OptionWarning(const char *filename, Severity severity)
{
   UNUSED(severity);

   if (cpd.line_number != 0)
   {
      fprintf(stderr, "%s:%u: ", filename, cpd.line_number);
   }
   else
   {
      fprintf(stderr, "%s: ", filename);
   }
}


//-----------------------------------------------------------------------------
OptionWarning::OptionWarning(const GenericOption *opt, Severity severity)
{
   UNUSED(severity);

   fprintf(stderr, "Option<%s>: at %s:%d: ", to_string(opt->type()),
           cpd.filename.c_str(), cpd.line_number);
}


//-----------------------------------------------------------------------------
OptionWarning::~OptionWarning()
{
   fprintf(stderr, "\n");
   log_flush(true);
}


//-----------------------------------------------------------------------------
void OptionWarning::operator()(const char *fmt, ...)
{
   va_list args;

   va_start(args, fmt);
   vfprintf(stderr, fmt, args);
   va_end(args);
}


//-----------------------------------------------------------------------------
void GenericOption::warnUnexpectedValue(const char *actual) const
{
   OptionWarning w{ this };

   auto          values = possibleValues();

   if (values[1] == nullptr)
   {
      w("Expected %s ", *values);
   }
   else
   {
      w("Expected one of ");

      while (*values)
      {
         w("'%s'", *values);

         if (*(++values))
         {
            w(", ");
         }
      }
   }
   w(", for '%s'; got '%s'", name(), actual);
}


//-----------------------------------------------------------------------------
void GenericOption::warnIncompatibleReference(const GenericOption *ref) const
{
   OptionWarning w{ this };

   w("%s references option %s with incompatible type %s",
     name(), ref->name(), to_string(ref->type()));
}


//-----------------------------------------------------------------------------
template<typename T>
bool read_enum(const char *in, Option<T> &out)
{
   assert(in);

   if (convert_string(in, out.m_val))
   {
      return(true);
   }

   if (const auto *const opt = find_option(in))
   {
      if (opt->type() != out.type())
      {
         out.warnIncompatibleReference(opt);
         return(false);
      }
      auto &topt = *static_cast<const Option<T> *>(opt);
      out.m_val = topt();
      return(true);
   }
   out.warnUnexpectedValue(in);
   return(false);
}


//-----------------------------------------------------------------------------
template<typename T>
bool read_number(const char *in, Option<T> &out)
{
   assert(in);

   char       *c;
   const auto val = std::strtol(in, &c, 10);

   if (  *c == 0
      && out.validate(val))
   {
      out.m_val = static_cast<T>(val);
      return(true);
   }
   bool invert = false;

   if (strchr("-", in[0]))
   {
      invert = true;
      ++in;
   }

   if (const auto *const opt = find_option(in))
   {
      LOG_CONFIG("%s(%d): line_number is %d, option(%s) %s, ref(%s) %s\n",
                 __func__, __LINE__, cpd.line_number,
                 to_string(out.type()), out.name(),
                 to_string(opt->type()), opt->name());

      long tval;

      if (opt->type() == OT_NUM)
      {
         auto &sopt = *static_cast<const Option<signed> *>(opt);
         tval = static_cast<long>(sopt());
      }
      else if (opt->type() == OT_UNUM)
      {
         auto &uopt = *static_cast<const Option<unsigned> *>(opt);
         tval = static_cast<long>(uopt());
      }
      else
      {
         out.warnIncompatibleReference(opt);
         return(false);
      }
      const auto rval = (invert ? -tval : tval);

      if (out.validate(rval))
      {
         out.m_val = static_cast<T>(rval);
         return(true);
      }
      return(false);
   }
   out.warnUnexpectedValue(in);
   return(false);
} // read_number


//-----------------------------------------------------------------------------
template<typename T>
void Option<T>::reset()
{
   m_val = m_default;
}


//-----------------------------------------------------------------------------
template<typename T>
std::string Option<T>::str() const
{
   return(to_string(m_val));
}


//-----------------------------------------------------------------------------
template<typename T>
std::string Option<T>::defaultStr() const
{
   return(m_default != T{} ? to_string(m_default) : std::string{});
}

// Explicit instantiations
template class Option<bool>;
template class Option<iarf_e>;
template class Option<line_end_e>;
template class Option<token_pos_e>;
template class Option<signed>;
template class Option<unsigned>;
template class Option<std::string>;

//END Option<T> and helpers

///////////////////////////////////////////////////////////////////////////////

//BEGIN Option<bool>


//-----------------------------------------------------------------------------
template<>
option_type_e Option<bool>::type() const
{
   return(OT_BOOL);
}


//-----------------------------------------------------------------------------
template<>
const char *const *Option<bool>::possibleValues() const
{
   static char const *values[] = { "true", "false", nullptr };

   return(values);
}


//-----------------------------------------------------------------------------
template<>
bool Option<bool>::read(const char *in)
{
   assert(in);

   if (convert_string(in, m_val))
   {
      return(true);
   }
   bool invert = false;

   if (strchr("~!-", in[0]))
   {
      invert = true;
      ++in;
   }

   if (const auto *const opt = find_option(in))
   {
      if (opt->type() != OT_BOOL)
      {
         warnIncompatibleReference(opt);
         return(false);
      }
      auto &bopt = *static_cast<const Option<bool> *>(opt);
      m_val = (invert ? !bopt() : bopt());
      return(true);
   }
   warnUnexpectedValue(in);
   return(false);
}

//END Option<bool>

///////////////////////////////////////////////////////////////////////////////

//BEGIN Option<iarf_e>


//-----------------------------------------------------------------------------
template<>
option_type_e Option<iarf_e>::type() const
{
   return(OT_IARF);
}


//-----------------------------------------------------------------------------
template<>
const char *const *Option<iarf_e>::possibleValues() const
{
   return(iarf_values);
}


//-----------------------------------------------------------------------------
template<>
bool Option<iarf_e>::read(const char *in)
{
   return(read_enum(in, *this));
}

//END Option<iarf_e>

///////////////////////////////////////////////////////////////////////////////

//BEGIN Option<line_end_e>


//-----------------------------------------------------------------------------
template<>
option_type_e Option<line_end_e>::type() const
{
   return(OT_LINEEND);
}


//-----------------------------------------------------------------------------
template<>
const char *const *Option<line_end_e>::possibleValues() const
{
   return(line_end_values);
}


//-----------------------------------------------------------------------------
template<>
bool Option<line_end_e>::read(const char *in)
{
   return(read_enum(in, *this));
}

//END Option<line_end_e>

///////////////////////////////////////////////////////////////////////////////

//BEGIN Option<token_pos_e>


//-----------------------------------------------------------------------------
template<>
option_type_e Option<token_pos_e>::type() const
{
   return(OT_TOKENPOS);
}


//-----------------------------------------------------------------------------
template<>
const char *const *Option<token_pos_e>::possibleValues() const
{
   return(token_pos_values);
}


//-----------------------------------------------------------------------------
template<>
bool Option<token_pos_e>::read(const char *in)
{
   return(read_enum(in, *this));
}

//END Option<token_pos_e>

///////////////////////////////////////////////////////////////////////////////

//BEGIN Option<signed>


//-----------------------------------------------------------------------------
template<>
option_type_e Option<signed>::type() const
{
   return(OT_NUM);
}


//-----------------------------------------------------------------------------
template<>
const char *const *Option<signed>::possibleValues() const
{
   static char const *values[] = { "number", nullptr };

   return(values);
}


//-----------------------------------------------------------------------------
template<>
bool Option<signed>::read(const char *in)
{
   return(read_number(in, *this));
}

//END Option<signed>

///////////////////////////////////////////////////////////////////////////////

//BEGIN Option<unsigned>


//-----------------------------------------------------------------------------
template<>
option_type_e Option<unsigned>::type() const
{
   return(OT_UNUM);
}


//-----------------------------------------------------------------------------
template<>
const char *const *Option<unsigned>::possibleValues() const
{
   static char const *values[] = { "unsigned number", nullptr };

   return(values);
}


//-----------------------------------------------------------------------------
template<>
bool Option<unsigned>::read(const char *in)
{
   return(read_number(in, *this));
}

//END Option<unsigned>

///////////////////////////////////////////////////////////////////////////////

//BEGIN Option<string>


//-----------------------------------------------------------------------------
template<>
option_type_e Option<std::string>::type() const
{
   return(OT_STRING);
}


//-----------------------------------------------------------------------------
template<>
const char *const *Option<std::string>::possibleValues() const
{
   static char const *values[] = { "string", nullptr };

   return(values);
}


//-----------------------------------------------------------------------------
template<>
bool Option<std::string>::read(const char *in)
{
   m_val = in;
   return(true);
}

//END Option<string>

///////////////////////////////////////////////////////////////////////////////

//BEGIN global functions for options


//-----------------------------------------------------------------------------
void begin_option_group(const char *description)
{
   auto g = OptionGroup{ description, {} };

   option_groups.push_back(g);
}


//-----------------------------------------------------------------------------
void register_option(GenericOption *option)
{
   assert(!option_groups.empty());

   option_groups.back().options.push_back(option);
   option_map.emplace(option->name(), option);
}


//-----------------------------------------------------------------------------
uncrustify::GenericOption *find_option(const char *name)
{
   const auto iter = option_map.find(to_lower(name));

   if (iter != option_map.end())
   {
      return(iter->second);
   }
   return(nullptr);
}


//-----------------------------------------------------------------------------
OptionGroup *get_option_group(size_t i)
{
   if (i >= option_groups.size())
   {
      return(nullptr);
   }
   return(&option_groups[i]);
}


//-----------------------------------------------------------------------------
size_t get_option_count()
{
   return(option_map.size());
}


//-----------------------------------------------------------------------------
void process_option_line(const std::string &config_line, const char *filename,
                         int &compat_level)
{
   // Split line into arguments, and punt if no arguments are present
   auto args = split_args(config_line, filename, is_arg_sep);

   if (args.empty())
   {
      return;
   }
   // Check for necessary arguments
   const auto &cmd = to_lower(args.front());

   if (  cmd == "set"
      || cmd == "file_ext")
   {
      if (args.size() < 3)
      {
         OptionWarning w{ filename };
         w("%s requires at least three arguments", cmd.c_str());
         return;
      }
   }
   else
   {
      if (args.size() < 2)
      {
         OptionWarning w{ filename };
         w("%s requires at least two arguments", cmd.c_str());
         return;
      }
   }

   if (cmd == "type")
   {
      for (size_t i = 1; i < args.size(); ++i)
      {
         add_keyword(args[i], CT_TYPE);
      }
   }
   else if (cmd == "macro-open")
   {
      add_keyword(args[1], CT_MACRO_OPEN);
   }
   else if (cmd == "macro-close")
   {
      add_keyword(args[1], CT_MACRO_CLOSE);
   }
   else if (cmd == "macro-else")
   {
      add_keyword(args[1], CT_MACRO_ELSE);
   }
   else if (cmd == "set")
   {
      const auto token = find_token_name(args[1].c_str());

      if (token != CT_NONE)
      {
         LOG_FMT(LNOTE, "%s:%d set '%s':",
                 filename, cpd.line_number, args[1].c_str());

         for (size_t i = 2; i < args.size(); ++i)
         {
            LOG_FMT(LNOTE, " '%s'", args[i].c_str());
            add_keyword(args[i], token);
         }

         LOG_FMT(LNOTE, "\n");
      }
      else
      {
         OptionWarning w{ filename };
         w("%s: unknown type '%s'", cmd.c_str(), args[1].c_str());
      }
   }
#ifndef EMSCRIPTEN
   else if (cmd == "include")
   {
      auto       this_line_number = cpd.line_number;
      const auto &include_path    = args[1];

      if (include_path.empty())
      {
         OptionWarning w{ filename };
         w("include: path cannot be empty");
      }
      else if (is_path_relative(include_path))
      {
         // include is a relative path to the current config file
         UncText ut = std::string{ filename };
         ut.resize(static_cast<unsigned>(path_dirname_len(filename)));
         ut.append(include_path);
         UNUSED(load_option_file(ut.c_str(), compat_level));
      }
      else
      {
         // include is an absolute path
         UNUSED(load_option_file(include_path.c_str(), compat_level));
      }
      cpd.line_number = this_line_number;
   }
#endif
   else if (cmd == "file_ext")
   {
      auto *const lang_arg = args[1].c_str();

      for (size_t i = 2; i < args.size(); ++i)
      {
         auto *const lang_name = extension_add(args[i].c_str(), lang_arg);

         if (lang_name)
         {
            LOG_FMT(LNOTE, "%s:%d file_ext '%s' => '%s'\n",
                    filename, cpd.line_number, args[i].c_str(), lang_name);
         }
         else
         {
            OptionWarning w{ filename };
            w("file_ext: unknown language '%s'", lang_arg);
            break;
         }
      }
   }
   else if (cmd == "using")
   {
      auto vargs = split_args(args[1], filename, is_varg_sep);

      if (vargs.size() == 2)
      {
         compat_level = option_level(std::stoi(vargs[0]), std::stoi(vargs[1]));
      }
      else if (vargs.size() == 3)
      {
         compat_level = option_level(std::stoi(vargs[0]),
                                     std::stoi(vargs[1]),
                                     std::stoi(vargs[2]));
      }
      else
      {
         OptionWarning w{ filename };
         w("%s requires a version number in the form MAJOR.MINOR[.PATCH]",
           cmd.c_str());
      }
   }
   else
   {
      // Must be a regular option = value
      if (compat_level < option_level(0, 69))
      {
         if (process_option_line_compat_0_68(cmd, args, filename))
         {
            return;
         }
      }

      if (compat_level < option_level(0, 71))
      {
         if (process_option_line_compat_0_70(cmd, filename))
         {
            return;
         }
      }

      if (compat_level < option_level(0, 74))
      {
         if (process_option_line_compat_0_73(cmd, filename))
         {
            return;
         }
      }

      if (compat_level < option_level(0, 75))
      {
         if (process_option_line_compat_0_74(cmd, filename))
         {
            return;
         }
      }

      if (compat_level < option_level(0, 76))
      {
         if (process_option_line_compat_0_75(cmd, filename))
         {
            return;
         }
      }

      if (compat_level < option_level(0, 77))
      {
         if (process_option_line_compat_0_76(cmd, args, filename))
         {
            return;
         }
      }

      if (compat_level < option_level(0, 79))
      {
         if (process_option_line_compat_0_78(cmd, filename))
         {
            return;
         }
      }
      const auto oi = option_map.find(cmd);

      if (oi == option_map.end())
      {
         OptionWarning w{ filename };
         w("unknown option '%s'", args[0].c_str());
      }
      else
      {
         UNUSED(oi->second->read(args[1].c_str()));
      }
   }
} // process_option_line


//-----------------------------------------------------------------------------
bool load_option_file(const char *filename, int compat_level)
{
   cpd.line_number = 0;

#ifdef WIN32
   // "/dev/null" not understood by "fopen" in Windows
   if (strcasecmp(filename, "/dev/null") == 0)
   {
      return(true);
   }
#endif

   std::ifstream in;
   in.open(filename, std::ifstream::in);

   if (!in.good())
   {
      OptionWarning w{ filename };
      w("file could not be opened: %s (%d)\n",
        strerror(errno), errno);
      exit(EX_SOFTWARE);
   }
   // Read in the file line by line
   std::string line;

   while (std::getline(in, line))
   {
      // check all characters of the line
      size_t howmany = line.length();
      int    ch;

      for (size_t n = 0; n < howmany; n++)
      {
         ch = line[n];

         // do not check characters in comment part of line
         if ('#' == ch)
         {
            break;
         }

         // ch >= 0 && ch <= 255
         if (  ch < 0
            || ch > 255)
         {
            // error
            // related to PR #3298
            fprintf(stderr, "%s: line %u: Character at position %zu, is not printable.\n", filename, cpd.line_number + 1, n + 1);
            exit(EX_SOFTWARE);
         }
      }

      ++cpd.line_number;
      process_option_line(line, filename, compat_level);
   }

   if (cpd.find_deprecated)
   {
      fprintf(stderr, "no deprecated option found.\n");
      exit(EX_OK);
   }
   return(true);
} // load_option_file


//-----------------------------------------------------------------------------
const char *get_eol_marker()
{
   static char eol[3] = { 0x0A, 0x00, 0x00 };

   const auto  &lines = cpd.newline.get();

   for (size_t i = 0; i < lines.size(); ++i)
   {
      eol[i] = static_cast<char>(lines[i]);
   }

   return(eol);
}


//-----------------------------------------------------------------------------
void save_option_file(FILE *pfile, bool with_doc, bool minimal)
{
   int        non_default_values = 0;
   const char *eol_marker        = get_eol_marker();

   fprintf(pfile, "# %s%s", UNCRUSTIFY_VERSION, eol_marker);

   // Print the options by group
   for (auto &og : option_groups)
   {
      bool first = true;

      for (auto *option : og.options)
      {
         const auto val = option->str();

         if (!option->isDefault())
         {
            ++non_default_values;
         }
         else if (minimal)
         {
            continue;
         }
         //....................................................................

         if (with_doc)
         {
            assert(option->description() != nullptr);
            assert(*option->description() != 0);

            if (first)
            {
               fprintf(pfile, "%s#%s", eol_marker, eol_marker);
               print_description(pfile, og.description, eol_marker);
               fprintf(pfile, "#%s", eol_marker);
            }
            fprintf(pfile, "%s", eol_marker);
            print_description(pfile, option->description(), eol_marker);

            const auto ds = option->defaultStr();

            if (!ds.empty())
            {
               fprintf(pfile, "#%s# Default: %s%s",
                       eol_marker, ds.c_str(), eol_marker);
            }
         }
         first = false;

         const int name_len = static_cast<int>(strlen(option->name()));
         const int pad      = name_len < uncrustify::limits::MAX_OPTION_NAME_LEN
                              ? (uncrustify::limits::MAX_OPTION_NAME_LEN - name_len)
                              : 1;

         fprintf(pfile, "%s%*.s= ", option->name(), pad, " ");

         if (option->type() == OT_STRING)
         {
            fprintf(pfile, "\"%s\"", val.c_str());
         }
         else
         {
            fprintf(pfile, "%s", val.c_str());
         }

         if (with_doc)
         {
            const int val_len = static_cast<int>(val.length());
            fprintf(pfile, "%*.s # ", 8 - val_len, " ");

            for (auto pv = option->possibleValues(); *pv; ++pv)
            {
               fprintf(pfile, "%s%s", *pv, pv[1] ? "/" : "");
            }
         }
         fputs(eol_marker, pfile);
      }
   }

   if (with_doc)
   {
      fprintf(pfile, "%s", DOC_TEXT_END);
   }
   print_custom_keywords(pfile);   // Print custom keywords
   print_extensions(pfile);        // Print custom file extensions

   fprintf(pfile, "# option(s) with 'not default' value: %d%s#%s",
           non_default_values, eol_marker, eol_marker);
} // save_option_file

} // namespace uncrustify
