/*
 * uncrustify_emscripten.cpp - JavaScript Emscripten binding interface
 *
 *  Created on: May 8, 2016
 *      Author: Daniel Chumak
 *
 * INTERFACE:
 * ============================================================================
 * unsure about these:
 *   --check       TODO ???
 *   --decode      TODO ???
 *   --detect      TODO needs uncrustify start and end which both are static
 *
 *
 * will not be included:
 * ----------------------------------------------------------------------------
 *   -t ( define via multiple --type )
 *   -d ( define via multiple --define )
 *   --assume ( no files available to guess the lang. based on the filename ending )
 *   --files ( no batch processing will be available )
 *   --prefix
 *   --suffix
 *   --no-backup
 *   --replace
 *   --mtime
 *   --universalindent
 *   --ds/--dump-steps
 *   -help, -h, --usage, -?
 *
 *
 * done:
 * ----------------------------------------------------------------------------
 *   --update-config ( use show_config() )
 *   --update-config-with-doc ( show_config( bool withDoc = true ) )
 *   --version, -v ( use get_version() )
 *   --log, -L ( use log_type_enable( log_sev_t sev, bool value ) )
 *   -q ( use quiet() )
 *   --config, -c ( use loadConfig( string _cfg ) )
 *   --file, -f ( use uncrustify( string _file ) )
 *   --show ( use log_type_show_name( bool ) )
 *   --frag ( use uncrustify( string _file, bool frag = true ) )
 *   --type ( use add_keyword( string _type, E_Token type ) )
 *   -l ( use uncrustify() )
 *   --parsed, -p  ( use debug() )
 */


#if defined (__linux__)


#include "keywords.h"
#include "log_levels.h"
#include "logger.h"
#include "option.h"
#include "options.h"
#include "output.h"
#include "prototypes.h"
#include "uncrustify.h"
#include "uncrustify_version.h"
#include "unicode.h"

#include <iostream>
#include <map>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <vector>


#ifdef EMSCRIPTEN
#include <emscripten/bind.h>
#include <emscripten/val.h>
using namespace emscripten;
#else
#define EMSCRIPTEN_BINDINGS(module)    void dummyFcn()
template<class T>
struct base {};
struct emscripten
{
   template<class... Args>
   emscripten value(Args...) { return{}; }

   template<class... Args>
   static emscripten function(Args...) { return{}; }

   template<class... Args>
   emscripten property(Args...) { return{}; }
};
using Dummy = emscripten;


template<class T>
Dummy enum_(char const *const)
{
   return(Dummy{});
}


template<class T>
Dummy register_vector(char const *const)
{
   return(Dummy{});
}


template<class... Args>
Dummy class_(char const *const)
{
   return(Dummy{});
}


template<class T>
Dummy select_overload(T)
{
   return(Dummy{});
}
#endif

using namespace std;
using namespace uncrustify;

namespace
{

/**
 * Loads options from a file represented as a single char array.
 * Modifies: input char array, cpd.line_number
 * Expects: \0 terminated char array
 *
 * @param configString char array that holds the whole config
 * @return EXIT_SUCCESS on success
 */
int load_option_fileChar(char *configString)
{
   char *delimPos       = &configString[0];
   char *subStringStart = &configString[0];

   cpd.line_number = 0;

   // TODO: handle compat_level
   int compat_level = 0;

   while (true)
   {
      delimPos = strchr(delimPos, '\n');

      if (delimPos == nullptr)
      {
         break;
      }
      // replaces \n with \0 -> string including multiple terminated substrings
      *delimPos = '\0';


      process_option_line(subStringStart, "", compat_level);

      delimPos++;
      subStringStart = delimPos;
   }
   //get last line, expectation: ends with \0
   process_option_line(subStringStart, "", compat_level);

   return(EXIT_SUCCESS);
}


/**
 * adds a new keyword to Uncrustify's dynamic keyword map (dkwm, keywords.cpp)
 *
 * @param tag:  keyword that is going to be added
 * @param type: type of the keyword
 */
void _add_keyword(string tag, E_Token type)
{
   if (tag.empty())
   {
      LOG_FMT(LERR, "%s: input string is empty\n", __func__);
      return;
   }
   add_keyword(tag, type);
}


//! clears Uncrustify's dynamic keyword map (dkwm, keywords.cpp)
void clear_keywords()
{
   clear_keyword_file();
}


/**
 * Show or hide the severity prefix "<1>"
 *
 * @param b: true=show, false=hide
 */
void show_log_type(bool b)
{
   log_show_sev(b);
}


//! returns the UNCRUSTIFY_VERSION string
string get_version()
{
   return(UNCRUSTIFY_VERSION);
}


//! disables all logging messages
void set_quiet()
{
   // set empty mask
   log_set_mask({});
}


/**
 * resets value of an option to its default
 *
 * @param name:  name of the option
 * @return options enum value of the found option or -1 if option was not found
 */
int reset_option(string name)
{
   if (name.empty())
   {
      LOG_FMT(LERR, "%s: name string is empty\n", __func__);
      return(-1);
   }
   const auto option = find_option(name.c_str());

   if (option == nullptr)
   {
      LOG_FMT(LERR, "Option %s not found\n", name.c_str());
      return(-1);
   }
   option->reset();
   return(0);
}


/**
 * sets value of an option
 *
 * @param name:  name of the option
 * @param value: value that is going to be set
 * @return options enum value of the found option or -1 if option was not found
 */
int set_option(string name, string value)
{
   if (name.empty())
   {
      LOG_FMT(LERR, "%s: name string is empty\n", __func__);
      return(-1);
   }

   if (value.empty())
   {
      LOG_FMT(LERR, "%s: value string is empty\n", __func__);
      return(-1);
   }
   const auto option = find_option(name.c_str());

   if (option == nullptr)
   {
      LOG_FMT(LERR, "Option %s not found\n", name.c_str());
      return(-1);
   }

   if (!option->read(value.c_str()))
   {
      LOG_FMT(
         LERR,
         "Failed to set value %s for option: %s of type: %s\n",
         name.c_str(),
         value.c_str(),
         to_string(option->type())
         );
      return(-1);
   }
   return(0);
}


/**
 * returns value of an option
 *
 * @param name: name of the option
 * @return currently set value of the option
 */
string get_option(string name)
{
   if (name.empty())
   {
      LOG_FMT(LERR, "%s: input string is empty\n", __func__);
      return("");
   }
   const auto option = find_option(name.c_str());

   if (option == nullptr)
   {
      LOG_FMT(LWARN, "Option %s not found\n", name.c_str());
      return("");
   }
   return(option->str());
}


/**
 * returns the config file string based on the current configuration
 *
 * @param withDoc:          false=without documentation,
 *                          true=with documentation text lines
 * @param only_not_default: false=containing all options,
 *                          true=containing only options with non default values
 * @return returns the config file string based on the current configuration
 */
string show_config(bool withDoc, bool only_not_default)
{
   char   *buf;
   size_t len;

   FILE   *stream = open_memstream(&buf, &len);

   if (stream == nullptr)
   {
      LOG_FMT(LERR, "Failed to open_memstream\n");
      fflush(stream);
      fclose(stream);
      free(buf);
      return("");
   }
   save_option_file(stream, withDoc, only_not_default);

   fflush(stream);
   fclose(stream);

   string out(buf);

   free(buf);

   return(out);
}


/**
 * returns the config file string with all options based on the current configuration
 *
 * @param withDoc: false= without documentation, true=with documentation text lines
 * @return returns the config file string with all options based on the current configuration
 */
string show_config(bool withDoc)
{
   return(show_config(withDoc, false));
}


//!returns the config file string with all options and without documentation based on the current configuration
string show_config()
{
   return(show_config(false, false));
}


std::vector<OptionGroup *> get_groups()
{
   std::vector<OptionGroup *> groups;

   groups.reserve(5);

   for (size_t i = 0; true; ++i)
   {
      OptionGroup *group = get_option_group(i);

      if (!group)
      {
         break;
      }
      groups.push_back(group);
   }

   return(groups);
}


std::vector<GenericOption *> get_options()
{
   std::vector<GenericOption *> options;

   options.reserve(get_option_count());

   for (size_t i = 0; true; ++i)
   {
      OptionGroup *group = get_option_group(i);

      if (!group)
      {
         break;
      }
      options.insert(
         end(options),
         begin(group->options),
         end(group->options)
         );
   }

   return(options);
}


//! resets all options to their default values
void reset_options()
{
   auto options = get_options();

   for (auto *option : options)
   {
      option->reset();
   }
}


/**
 * initializes the current libUncrustify instance,
 * used only for emscripten binding here and will be automatically called while
 * module initialization
 */
void _initialize()
{
   register_options();
   log_init(stdout);

   LOG_FMT(LSYS, "Initialized libUncrustify - " UNCRUSTIFY_VERSION "\n");
}


//! destroys the current libUncrustify instance
void destruct()
{
   clear_keyword_file();
}


/**
 * reads option file string, sets the defined options
 *
 * @return returns EXIT_SUCCESS on success
 */
int _loadConfig(intptr_t _cfg)
{
   // reset everything in case a config was loaded previously
   clear_keyword_file();
   reset_options();

   // embind complains about char* so we use an int to get the pointer and cast
   // it, memory management is done in /emscripten/postfix_module.js
   char *cfg = reinterpret_cast<char *>(_cfg);

   if (load_option_fileChar(cfg) != EXIT_SUCCESS)
   {
      LOG_FMT(LERR, "unable to load the config\n");
      return(EXIT_FAILURE);
   }
   // This relies on cpd.filename being the config file name
   load_header_files();

   LOG_FMT(LSYS, "finished loading config\n");
   return(EXIT_SUCCESS);
}


/**
 * format string
 *
 * @param file: pointer to the file char* string that is going to be formatted
 * @param langIDX: specifies in which language the input file is written
 * @param frag: true=fragmented code input, false=unfragmented code input
 * @param defer: true=do not perform cleanup of Uncrustify structures
 *
 * @return pointer to the formatted file char* string
 */
intptr_t _uncrustify(intptr_t _file, lang_flag_e langIDX, bool frag, bool defer)
{
   // Problem: uncrustify originally is not a lib and uses global vars such as
   // cpd.error_count for the whole program execution
   // to know if errors occurred during the formatting step we reset this var here
   cpd.filename = "stdin";
   cpd.frag     = frag;

   if (langIDX == (lang_flag_e)0)   // 0 == undefined
   {
      LOG_FMT(LWARN, "language of input file not defined, C++ will be assumed\n");
      cpd.lang_flags = e_LANG_CPP;
   }
   else
   {
      cpd.lang_flags = static_cast<size_t>(langIDX);
   }
   // embind complains about char* so we use an intptr_t to get the pointer and
   // cast it, memory management is done in /emscripten/postfix_module.js
   char       *file = reinterpret_cast<char *>(_file);

   MemoryFile fm;
   char       c;

   for (auto idx = 0; (c = file[idx]) != 0; ++idx)
   {
      fm.raw.push_back(c);
   }

   if (!decode_unicode(fm))
   {
      LOG_FMT(LERR, "Failed to read code\n");
      return(0);
   }
   // Done reading from stdin
   LOG_FMT(LSYS, "Parsing: %zu bytes (%zu chars) from stdin as language %s\n",
           fm.raw.size(), fm.data.size(),
           language_name_from_flags(cpd.lang_flags));


   char   *buf = nullptr;
   size_t len  = 0;

   // uncrustify uses FILE instead of streams for its outputs
   // to redirect FILE writes into a char* open_memstream is used
   // windows lacks open_memstream, only UNIX/BSD is supported
   // apparently emscripten has its own implementation, if that is not working
   // see: stackoverflow.com/questions/10305095#answer-10341073
   FILE *stream = open_memstream(&buf, &len);

   if (stream == nullptr)
   {
      LOG_FMT(LERR, "Failed to open_memstream\n");
      return(0);
   }
   // TODO One way to implement the --parsed, -p functionality would
   // be to let the uncrustify_file function run, throw away the formatted
   // output and return the debug as a string. For this uncrustify_file would
   // need to accept a stream, FILE or a char array pointer in which the output
   // will be stored.
   // Another option would be to check, inside the uncrustify_file function,
   // if the current filename string matches stdout or stderr and use those as
   // output locations. This is the easier fix but the debug info in the
   // browsers console is littered with other unneeded text.
   // Finally, the ugliest solution, would be also possible to re-route
   // either stdout or stderr inside the Module var of emscripten to a js
   // function which passes the debug output into a dedicated output js target.
   // This therefore would introduce the dependency on the user to always have
   // the output js target available.
   uncrustify_file(fm, stream, nullptr, nullptr, defer);

   fflush(stream);
   fclose(stream);

   if (len == 0)
   {
      return(0);
   }
   // buf is deleted inside js code
   return(reinterpret_cast<intptr_t>(buf));
} // uncrustify


/**
 * format string
 *
 * @param file: pointer to the file char* string that is going to be formatted
 * @param langIDX: specifies in which language the input file is written
 * @param frag: true=fragmented code input, false=unfragmented code input
 *
 * @return pointer to the formatted file char* string
 */
intptr_t _uncrustify(intptr_t file, lang_flag_e langIDX, bool frag)
{
   return(_uncrustify(file, langIDX, frag, false));
}


/**
 * format string, assume unfragmented code input
 *
 * @param file: pointer to the file char* string that is going to be formatted
 * @param langIDX: specifies in which language the input file is written
 *
 * @return pointer to the formatted file char* string
 */
intptr_t _uncrustify(intptr_t file, lang_flag_e langIDX)
{
   return(_uncrustify(file, langIDX, false, false));
}


/**
 * generate debug output
 *
 * @param file: pointer to the file char* string that is going to be formatted
 * @param langIDX: specifies in which language the input file is written
 * @param frag: true=fragmented code input, false=unfragmented code input
 *
 * @return pointer to the debug file char* string
 */
intptr_t _debug(intptr_t _file, lang_flag_e langIDX, bool frag)
{
   auto formatted_str_ptr = _uncrustify(_file, langIDX, frag, true);
   char *formatted_str    = reinterpret_cast<char *>(formatted_str_ptr);

   // Lazy solution: Throw away the formatted file output.
   // Maybe later add option to return both formatted file string and debug
   // file string together ... somehow.
   free(formatted_str);

   char   *buf    = nullptr;
   size_t len     = 0;
   FILE   *stream = open_memstream(&buf, &len);

   if (stream == nullptr)
   {
      LOG_FMT(LERR, "Failed to open_memstream\n");
      return(0);
   }
   output_parsed(stream);
   fflush(stream);
   fclose(stream);

   // start deferred _uncrustify cleanup
   uncrustify_end();

   if (len == 0)
   {
      return(0);
   }
   // buf is deleted inside js code
   return(reinterpret_cast<intptr_t>(buf));
} // uncrustify


/**
 * generate debug output, assume unfragmented code input
 *
 * @param file: pointer to the file char* string that is going to be formatted
 * @param langIDX: specifies in which language the input file is written
 *
 * @return pointer to the debug file char* string
 */
intptr_t _debug(intptr_t _file, lang_flag_e langIDX)
{
   return(_debug(_file, langIDX, false));
}

} // namespace

EMSCRIPTEN_BINDINGS(MainModule)
{
   // region enum bindings
   enum_<option_type_e>("OptionType")
      .value("BOOL", option_type_e::BOOL)
      .value("IARF", option_type_e::IARF)
      .value("LINEEND", option_type_e::LINEEND)
      .value("TOKENPOS", option_type_e::TOKENPOS)
      .value("NUM", option_type_e::NUM)
      .value("UNUM", option_type_e::UNUM)
      .value("STRING", option_type_e::STRING);

   enum_<iarf_e>("IARF")
      .value("IGNORE", IARF_IGNORE)
      .value("ADD", IARF_ADD)
      .value("REMOVE", IARF_REMOVE)
      .value("FORCE", IARF_FORCE);

   enum_<line_end_e>("LineEnd")
      .value("LF", line_end_e::LF)
      .value("CRLF", line_end_e::CRLF)
      .value("CR", line_end_e::CR)
      .value("AUTO", line_end_e::AUTO);

   enum_<token_pos_e>("TokenPos")
      .value("IGNORE", token_pos_e::IGNORE)
      .value("BREAK", token_pos_e::BREAK)
      .value("FORCE", token_pos_e::FORCE)
      .value("LEAD", token_pos_e::LEAD)
      .value("TRAIL", token_pos_e::TRAIL)
      .value("JOIN", token_pos_e::JOIN)
      .value("LEAD_BREAK", token_pos_e::LEAD_BREAK)
      .value("LEAD_FORCE", token_pos_e::LEAD_FORCE)
      .value("TRAIL_BREAK", token_pos_e::TRAIL_BREAK)
      .value("TRAIL_FORCE", token_pos_e::TRAIL_FORCE);

   enum_<log_sev_t>("LogType")
      .value("SYS", log_sev_t::LSYS)
      .value("ERR", log_sev_t::LERR)
      .value("WARN", log_sev_t::LWARN)
      .value("NOTE", log_sev_t::LNOTE)
      .value("INFO", log_sev_t::LINFO)
      .value("DATA", log_sev_t::LDATA)
      .value("FILELIST", log_sev_t::LFILELIST)
      .value("LINEENDS", log_sev_t::LLINEENDS)
      .value("CASTS", log_sev_t::LCASTS)
      .value("ALBR", log_sev_t::LALBR)
      .value("ALTD", log_sev_t::LALTD)
      .value("ALPP", log_sev_t::LALPP)
      .value("ALPROTO", log_sev_t::LALPROTO)
      .value("ALNLC", log_sev_t::LALNLC)
      .value("ALTC", log_sev_t::LALTC)
      .value("ALADD", log_sev_t::LALADD)
      .value("ALASS", log_sev_t::LALASS)
      .value("FVD", log_sev_t::LFVD)
      .value("FVD2", log_sev_t::LFVD2)
      .value("INDENT", log_sev_t::LINDENT)
      .value("INDENT2", log_sev_t::LINDENT2)
      .value("INDPSE", log_sev_t::LINDPSE)
      .value("INDPC", log_sev_t::LINDPC)
      .value("NEWLINE", log_sev_t::LNEWLINE)
      .value("PF", log_sev_t::LPF)
      .value("STMT", log_sev_t::LSTMT)
      .value("TOK", log_sev_t::LTOK)
      .value("ALRC", log_sev_t::LALRC)
      .value("CMTIND", log_sev_t::LCMTIND)
      .value("INDLINE", log_sev_t::LINDLINE)
      .value("SIB", log_sev_t::LSIB)
      .value("RETURN", log_sev_t::LRETURN)
      .value("BRDEL", log_sev_t::LBRDEL)
      .value("FCN", log_sev_t::LFCN)
      .value("FCNP", log_sev_t::LFCNP)
      .value("PCU", log_sev_t::LPCU)
      .value("DYNKW", log_sev_t::LDYNKW)
      .value("OUTIND", log_sev_t::LOUTIND)
      .value("BCSAFTER", log_sev_t::LBCSAFTER)
      .value("BCSPOP", log_sev_t::LBCSPOP)
      .value("BCSPUSH", log_sev_t::LBCSPUSH)
      .value("BCSSWAP", log_sev_t::LBCSSWAP)
      .value("FTOR", log_sev_t::LFTOR)
      .value("AS", log_sev_t::LAS)
      .value("PPIS", log_sev_t::LPPIS)
      .value("TYPEDEF", log_sev_t::LTYPEDEF)
      .value("VARDEF", log_sev_t::LVARDEF)
      .value("DEFVAL", log_sev_t::LDEFVAL)
      .value("PVSEMI", log_sev_t::LPVSEMI)
      .value("PFUNC", log_sev_t::LPFUNC)
      .value("SPLIT", log_sev_t::LSPLIT)
      .value("FTYPE", log_sev_t::LFTYPE)
      .value("TEMPL", log_sev_t::LTEMPL)
      .value("PARADD", log_sev_t::LPARADD)
      .value("PARADD2", log_sev_t::LPARADD2)
      .value("BLANKD", log_sev_t::LBLANKD)
      .value("TEMPFUNC", log_sev_t::LTEMPFUNC)
      .value("SCANSEMI", log_sev_t::LSCANSEMI)
      .value("DELSEMI", log_sev_t::LDELSEMI)
      .value("FPARAM", log_sev_t::LFPARAM)
      .value("NL1LINE", log_sev_t::LNL1LINE)
      .value("PFCHK", log_sev_t::LPFCHK)
      .value("AVDB", log_sev_t::LAVDB)
      .value("SORT", log_sev_t::LSORT)
      .value("SPACE", log_sev_t::LSPACE)
      .value("ALIGN", log_sev_t::LALIGN)
      .value("ALAGAIN", log_sev_t::LALAGAIN)
      .value("OPERATOR", log_sev_t::LOPERATOR)
      .value("ASFCP", log_sev_t::LASFCP)
      .value("INDLINED", log_sev_t::LINDLINED)
      .value("BCTRL", log_sev_t::LBCTRL)
      .value("RMRETURN", log_sev_t::LRMRETURN)
      .value("PPIF", log_sev_t::LPPIF)
      .value("MCB", log_sev_t::LMCB)
      .value("BRCH", log_sev_t::LBRCH)
      .value("FCNR", log_sev_t::LFCNR)
      .value("OCCLASS", log_sev_t::LOCCLASS)
      .value("OCMSG", log_sev_t::LOCMSG)
      .value("BLANK", log_sev_t::LBLANK)
      .value("OBJCWORD", log_sev_t::LOBJCWORD)
      .value("CHANGE", log_sev_t::LCHANGE)
      .value("CONTTEXT", log_sev_t::LCONTTEXT)
      .value("ANNOT", log_sev_t::LANNOT)
      .value("OCBLK", log_sev_t::LOCBLK)
      .value("FLPAREN", log_sev_t::LFLPAREN)
      .value("OCMSGD", log_sev_t::LOCMSGD)
      .value("INDENTAG", log_sev_t::LINDENTAG)
      .value("NFD", log_sev_t::LNFD)
      .value("JDBI", log_sev_t::LJDBI)
      .value("SETPAR", log_sev_t::LSETPAR)
      .value("SETTYP", log_sev_t::LSETTYP)
      .value("SETFLG", log_sev_t::LSETFLG)
      .value("NLFUNCT", log_sev_t::LNLFUNCT)
      .value("CHUNK", log_sev_t::LCHUNK)
      .value("GUY98", log_sev_t::LGUY98)
      .value("GUY", log_sev_t::LGUY);

   enum_<E_Token>("TokenType")
      .value("NONE", E_Token::NONE)
      .value("EOFILE", E_Token::EOFILE)
      .value("UNKNOWN", E_Token::UNKNOWN)
      .value("JUNK", E_Token::JUNK)
      .value("WHITESPACE", E_Token::WHITESPACE)
      .value("SPACE", E_Token::SPACE)
      .value("NEWLINE", E_Token::NEWLINE)
      .value("NL_CONT", E_Token::NL_CONT)
      .value("COMMENT_CPP", E_Token::COMMENT_CPP)
      .value("COMMENT", E_Token::COMMENT)
      .value("COMMENT_MULTI", E_Token::COMMENT_MULTI)
      .value("COMMENT_EMBED", E_Token::COMMENT_EMBED)
      .value("COMMENT_START", E_Token::COMMENT_START)
      .value("COMMENT_END", E_Token::COMMENT_END)
      .value("COMMENT_WHOLE", E_Token::COMMENT_WHOLE)
      .value("COMMENT_CPP_ENDIF", E_Token::COMMENT_CPP_ENDIF)
      .value("COMMENT_ENDIF", E_Token::COMMENT_ENDIF)
      .value("IGNORED", E_Token::IGNORED)
      .value("WORD", E_Token::WORD)
      .value("NUMBER", E_Token::NUMBER)
      .value("NUMBER_FP", E_Token::NUMBER_FP)
      .value("STRING", E_Token::STRING)
      .value("STRING_MULTI", E_Token::STRING_MULTI)
      .value("IF", E_Token::IF)
      .value("ELSE", E_Token::ELSE)
      .value("ELSEIF", E_Token::ELSEIF)
      .value("FOR", E_Token::FOR)
      .value("WHILE", E_Token::WHILE)
      .value("WHILE_OF_DO", E_Token::WHILE_OF_DO)
      .value("SWITCH", E_Token::SWITCH)
      .value("CASE", E_Token::CASE)
      .value("DO", E_Token::DO)
      .value("SYNCHRONIZED", E_Token::SYNCHRONIZED)
      .value("VOLATILE", E_Token::VOLATILE)
      .value("TYPEDEF", E_Token::TYPEDEF)
      .value("STRUCT", E_Token::STRUCT)
      .value("ENUM", E_Token::ENUM)
      .value("ENUM_CLASS", E_Token::ENUM_CLASS)
      .value("SIZEOF", E_Token::SIZEOF)
      .value("DECLTYPE", E_Token::DECLTYPE)
      .value("RETURN", E_Token::RETURN)
      .value("BREAK", E_Token::BREAK)
      .value("UNION", E_Token::UNION)
      .value("GOTO", E_Token::GOTO)
      .value("CONTINUE", E_Token::CONTINUE)
      .value("C_CAST", E_Token::C_CAST)
      .value("CPP_CAST", E_Token::CPP_CAST)
      .value("D_CAST", E_Token::D_CAST)
      .value("TYPE_CAST", E_Token::TYPE_CAST)
      .value("TYPENAME", E_Token::TYPENAME)
      .value("TEMPLATE", E_Token::TEMPLATE)
      .value("WHERE_SPEC", E_Token::WHERE_SPEC)
      .value("ASSIGN", E_Token::ASSIGN)
      .value("ASSIGN_NL", E_Token::ASSIGN_NL)
      .value("SASSIGN", E_Token::SASSIGN)
      .value("ASSIGN_DEFAULT_ARG", E_Token::ASSIGN_DEFAULT_ARG)
      .value("ASSIGN_FUNC_PROTO", E_Token::ASSIGN_FUNC_PROTO)
      .value("COMPARE", E_Token::COMPARE)
      .value("SCOMPARE", E_Token::SCOMPARE)
      .value("BOOL", E_Token::BOOL)
      .value("SBOOL", E_Token::SBOOL)
      .value("ARITH", E_Token::ARITH)
      .value("SARITH", E_Token::SARITH)
      .value("CARET", E_Token::CARET)
      .value("DEREF", E_Token::DEREF)
      .value("INCDEC_BEFORE", E_Token::INCDEC_BEFORE)
      .value("INCDEC_AFTER", E_Token::INCDEC_AFTER)
      .value("MEMBER", E_Token::MEMBER)
      .value("DC_MEMBER", E_Token::DC_MEMBER)
      .value("C99_MEMBER", E_Token::C99_MEMBER)
      .value("INV", E_Token::INV)
      .value("DESTRUCTOR", E_Token::DESTRUCTOR)
      .value("NOT", E_Token::NOT)
      .value("D_TEMPLATE", E_Token::D_TEMPLATE)
      .value("ADDR", E_Token::ADDR)
      .value("NEG", E_Token::NEG)
      .value("POS", E_Token::POS)
      .value("STAR", E_Token::STAR)
      .value("PLUS", E_Token::PLUS)
      .value("MINUS", E_Token::MINUS)
      .value("AMP", E_Token::AMP)
      .value("BYREF", E_Token::BYREF)
      .value("POUND", E_Token::POUND)
      .value("PREPROC", E_Token::PREPROC)
      .value("PREPROC_INDENT", E_Token::PREPROC_INDENT)
      .value("PREPROC_BODY", E_Token::PREPROC_BODY)
      .value("PP", E_Token::PP)
      .value("ELLIPSIS", E_Token::ELLIPSIS)
      .value("RANGE", E_Token::RANGE)
      .value("NULLCOND", E_Token::NULLCOND)
      .value("SEMICOLON", E_Token::SEMICOLON)
      .value("VSEMICOLON", E_Token::VSEMICOLON)
      .value("COLON", E_Token::COLON)
      .value("ASM_COLON", E_Token::ASM_COLON)
      .value("CASE_COLON", E_Token::CASE_COLON)
      .value("CLASS_COLON", E_Token::CLASS_COLON)
      .value("CONSTR_COLON", E_Token::CONSTR_COLON)
      .value("D_ARRAY_COLON", E_Token::D_ARRAY_COLON)
      .value("COND_COLON", E_Token::COND_COLON)
      .value("WHERE_COLON", E_Token::WHERE_COLON)
      .value("QUESTION", E_Token::QUESTION)
      .value("COMMA", E_Token::COMMA)
      .value("ASM", E_Token::ASM)
      .value("ATTRIBUTE", E_Token::ATTRIBUTE)
      .value("AUTORELEASEPOOL", E_Token::AUTORELEASEPOOL)
      .value("OC_AVAILABLE", E_Token::OC_AVAILABLE)
      .value("OC_AVAILABLE_VALUE", E_Token::OC_AVAILABLE_VALUE)
      .value("CATCH", E_Token::CATCH)
      .value("WHEN", E_Token::WHEN)
      .value("WHERE", E_Token::WHERE)
      .value("CLASS", E_Token::CLASS)
      .value("DELETE", E_Token::DELETE)
      .value("EXPORT", E_Token::EXPORT)
      .value("FRIEND", E_Token::FRIEND)
      .value("NAMESPACE", E_Token::NAMESPACE)
      .value("PACKAGE", E_Token::PACKAGE)
      .value("NEW", E_Token::NEW)
      .value("OPERATOR", E_Token::OPERATOR)
      .value("OPERATOR_VAL", E_Token::OPERATOR_VAL)
      .value("ASSIGN_OPERATOR", E_Token::ASSIGN_OPERATOR)
      .value("ACCESS", E_Token::ACCESS)
      .value("ACCESS_COLON", E_Token::ACCESS_COLON)
      .value("THROW", E_Token::THROW)
      .value("NOEXCEPT", E_Token::NOEXCEPT)
      .value("TRY", E_Token::TRY)
      .value("BRACED_INIT_LIST", E_Token::BRACED_INIT_LIST)
      .value("USING", E_Token::USING)
      .value("USING_STMT", E_Token::USING_STMT)
      .value("USING_ALIAS", E_Token::USING_ALIAS)
      .value("D_WITH", E_Token::D_WITH)
      .value("D_MODULE", E_Token::D_MODULE)
      .value("SUPER", E_Token::SUPER)
      .value("DELEGATE", E_Token::DELEGATE)
      .value("BODY", E_Token::BODY)
      .value("DEBUG", E_Token::DEBUG)
      .value("DEBUGGER", E_Token::DEBUGGER)
      .value("INVARIANT", E_Token::INVARIANT)
      .value("UNITTEST", E_Token::UNITTEST)
      .value("UNSAFE", E_Token::UNSAFE)
      .value("FINALLY", E_Token::FINALLY)
      .value("FIXED", E_Token::FIXED)
      .value("IMPORT", E_Token::IMPORT)
      .value("D_SCOPE", E_Token::D_SCOPE)
      .value("D_SCOPE_IF", E_Token::D_SCOPE_IF)
      .value("LAZY", E_Token::LAZY)
      .value("D_MACRO", E_Token::D_MACRO)
      .value("D_VERSION", E_Token::D_VERSION)
      .value("D_VERSION_IF", E_Token::D_VERSION_IF)
      .value("PAREN_OPEN", E_Token::PAREN_OPEN)
      .value("PAREN_CLOSE", E_Token::PAREN_CLOSE)
      .value("ANGLE_OPEN", E_Token::ANGLE_OPEN)
      .value("ANGLE_CLOSE", E_Token::ANGLE_CLOSE)
      .value("SPAREN_OPEN", E_Token::SPAREN_OPEN)
      .value("SPAREN_CLOSE", E_Token::SPAREN_CLOSE)
      .value("FPAREN_OPEN", E_Token::FPAREN_OPEN)
      .value("FPAREN_CLOSE", E_Token::FPAREN_CLOSE)
      .value("TPAREN_OPEN", E_Token::TPAREN_OPEN)
      .value("TPAREN_CLOSE", E_Token::TPAREN_CLOSE)
      .value("BRACE_OPEN", E_Token::BRACE_OPEN)
      .value("BRACE_CLOSE", E_Token::BRACE_CLOSE)
      .value("VBRACE_OPEN", E_Token::VBRACE_OPEN)
      .value("VBRACE_CLOSE", E_Token::VBRACE_CLOSE)
      .value("SQUARE_OPEN", E_Token::SQUARE_OPEN)
      .value("SQUARE_CLOSE", E_Token::SQUARE_CLOSE)
      .value("TSQUARE", E_Token::TSQUARE)
      .value("MACRO_OPEN", E_Token::MACRO_OPEN)
      .value("MACRO_CLOSE", E_Token::MACRO_CLOSE)
      .value("MACRO_ELSE", E_Token::MACRO_ELSE)
      .value("LABEL", E_Token::LABEL)
      .value("LABEL_COLON", E_Token::LABEL_COLON)
      .value("FUNCTION", E_Token::FUNCTION)
      .value("FUNC_CALL", E_Token::FUNC_CALL)
      .value("FUNC_CALL_USER", E_Token::FUNC_CALL_USER)
      .value("FUNC_DEF", E_Token::FUNC_DEF)
      .value("FUNC_TYPE", E_Token::FUNC_TYPE)
      .value("FUNC_VAR", E_Token::FUNC_VAR)
      .value("FUNC_PROTO", E_Token::FUNC_PROTO)
      .value("FUNC_START", E_Token::FUNC_START)
      .value("FUNC_CLASS_DEF", E_Token::FUNC_CLASS_DEF)
      .value("FUNC_CLASS_PROTO", E_Token::FUNC_CLASS_PROTO)
      .value("FUNC_CTOR_VAR", E_Token::FUNC_CTOR_VAR)
      .value("FUNC_WRAP", E_Token::FUNC_WRAP)
      .value("PROTO_WRAP", E_Token::PROTO_WRAP)
      .value("MACRO_FUNC", E_Token::MACRO_FUNC)
      .value("MACRO", E_Token::MACRO)
      .value("QUALIFIER", E_Token::QUALIFIER)
      .value("EXTERN", E_Token::EXTERN)
      .value("DECLSPEC", E_Token::DECLSPEC)
      .value("ALIGN", E_Token::ALIGN)
      .value("TYPE", E_Token::TYPE)
      .value("PTR_TYPE", E_Token::PTR_TYPE)
      .value("TYPE_WRAP", E_Token::TYPE_WRAP)
      .value("CPP_LAMBDA", E_Token::CPP_LAMBDA)
      .value("CPP_LAMBDA_RET", E_Token::CPP_LAMBDA_RET)
      .value("TRAILING_RET", E_Token::TRAILING_RET)
      .value("BIT_COLON", E_Token::BIT_COLON)
      .value("OC_DYNAMIC", E_Token::OC_DYNAMIC)
      .value("OC_END", E_Token::OC_END)
      .value("OC_IMPL", E_Token::OC_IMPL)
      .value("OC_INTF", E_Token::OC_INTF)
      .value("OC_PROTOCOL", E_Token::OC_PROTOCOL)
      .value("OC_PROTO_LIST", E_Token::OC_PROTO_LIST)
      .value("OC_GENERIC_SPEC", E_Token::OC_GENERIC_SPEC)
      .value("OC_PROPERTY", E_Token::OC_PROPERTY)
      .value("OC_CLASS", E_Token::OC_CLASS)
      .value("OC_CLASS_EXT", E_Token::OC_CLASS_EXT)
      .value("OC_CATEGORY", E_Token::OC_CATEGORY)
      .value("OC_SCOPE", E_Token::OC_SCOPE)
      .value("OC_MSG", E_Token::OC_MSG)
      .value("OC_MSG_CLASS", E_Token::OC_MSG_CLASS)
      .value("OC_MSG_FUNC", E_Token::OC_MSG_FUNC)
      .value("OC_MSG_NAME", E_Token::OC_MSG_NAME)
      .value("OC_MSG_SPEC", E_Token::OC_MSG_SPEC)
      .value("OC_MSG_DECL", E_Token::OC_MSG_DECL)
      .value("OC_RTYPE", E_Token::OC_RTYPE)
      .value("OC_ATYPE", E_Token::OC_ATYPE)
      .value("OC_COLON", E_Token::OC_COLON)
      .value("OC_DICT_COLON", E_Token::OC_DICT_COLON)
      .value("OC_SEL", E_Token::OC_SEL)
      .value("OC_SEL_NAME", E_Token::OC_SEL_NAME)
      .value("OC_BLOCK", E_Token::OC_BLOCK)
      .value("OC_BLOCK_ARG", E_Token::OC_BLOCK_ARG)
      .value("OC_BLOCK_TYPE", E_Token::OC_BLOCK_TYPE)
      .value("OC_BLOCK_EXPR", E_Token::OC_BLOCK_EXPR)
      .value("OC_BLOCK_CARET", E_Token::OC_BLOCK_CARET)
      .value("OC_AT", E_Token::OC_AT)
      .value("OC_PROPERTY_ATTR", E_Token::OC_PROPERTY_ATTR)
      .value("PP_DEFINE", E_Token::PP_DEFINE)
      .value("PP_DEFINED", E_Token::PP_DEFINED)
      .value("PP_INCLUDE", E_Token::PP_INCLUDE)
      .value("PP_IF", E_Token::PP_IF)
      .value("PP_ELSE", E_Token::PP_ELSE)
      .value("PP_ENDIF", E_Token::PP_ENDIF)
      .value("PP_ASSERT", E_Token::PP_ASSERT)
      .value("PP_EMIT", E_Token::PP_EMIT)
      .value("PP_ENDINPUT", E_Token::PP_ENDINPUT)
      .value("PP_ERROR", E_Token::PP_ERROR)
      .value("PP_FILE", E_Token::PP_FILE)
      .value("PP_LINE", E_Token::PP_LINE)
      .value("PP_SECTION", E_Token::PP_SECTION)
      .value("PP_ASM", E_Token::PP_ASM)
      .value("PP_UNDEF", E_Token::PP_UNDEF)
      .value("PP_PROPERTY", E_Token::PP_PROPERTY)
      .value("PP_BODYCHUNK", E_Token::PP_BODYCHUNK)
      .value("PP_PRAGMA", E_Token::PP_PRAGMA)
      .value("PP_REGION", E_Token::PP_REGION)
      .value("PP_ENDREGION", E_Token::PP_ENDREGION)
      .value("PP_REGION_INDENT", E_Token::PP_REGION_INDENT)
      .value("PP_IF_INDENT", E_Token::PP_IF_INDENT)
      .value("PP_IGNORE", E_Token::PP_IGNORE)
      .value("PP_OTHER", E_Token::PP_OTHER)
      .value("CHAR", E_Token::CHAR)
      .value("DEFINED", E_Token::DEFINED)
      .value("FORWARD", E_Token::FORWARD)
      .value("NATIVE", E_Token::NATIVE)
      .value("STATE", E_Token::STATE)
      .value("STOCK", E_Token::STOCK)
      .value("TAGOF", E_Token::TAGOF)
      .value("DOT", E_Token::DOT)
      .value("TAG", E_Token::TAG)
      .value("TAG_COLON", E_Token::TAG_COLON)
      .value("LOCK", E_Token::LOCK)
      .value("AS", E_Token::AS)
      .value("IN", E_Token::IN)
      .value("BRACED", E_Token::BRACED)
      .value("THIS", E_Token::THIS)
      .value("BASE", E_Token::BASE)
      .value("DEFAULT", E_Token::DEFAULT)
      .value("GETSET", E_Token::GETSET)
      .value("GETSET_EMPTY", E_Token::GETSET_EMPTY)
      .value("CONCAT", E_Token::CONCAT)
      .value("CS_SQ_STMT", E_Token::CS_SQ_STMT)
      .value("CS_SQ_COLON", E_Token::CS_SQ_COLON)
      .value("CS_PROPERTY", E_Token::CS_PROPERTY)
      .value("SQL_EXEC", E_Token::SQL_EXEC)
      .value("SQL_BEGIN", E_Token::SQL_BEGIN)
      .value("SQL_END", E_Token::SQL_END)
      .value("SQL_WORD", E_Token::SQL_WORD)
      .value("SQL_ASSIGN", E_Token::SQL_ASSIGN)
      .value("CONSTRUCT", E_Token::CONSTRUCT)
      .value("LAMBDA", E_Token::LAMBDA)
      .value("ASSERT", E_Token::ASSERT)
      .value("ANNOTATION", E_Token::ANNOTATION)
      .value("FOR_COLON", E_Token::FOR_COLON)
      .value("DOUBLE_BRACE", E_Token::DOUBLE_BRACE)
      .value("CNG_HASINC", E_Token::CNG_HASINC)
      .value("CNG_HASINCN", E_Token::CNG_HASINCN)
      .value("Q_EMIT", E_Token::Q_EMIT)
      .value("Q_FOREACH", E_Token::Q_FOREACH)
      .value("Q_FOREVER", E_Token::Q_FOREVER)
      .value("Q_GADGET", E_Token::Q_GADGET)
      .value("Q_OBJECT", E_Token::Q_OBJECT)
      .value("MODE", E_Token::MODE)
      .value("DI", E_Token::DI)
      .value("HI", E_Token::HI)
      .value("QI", E_Token::QI)
      .value("SI", E_Token::SI)
      .value("NOTHROW", E_Token::NOTHROW)
      .value("WORD_", E_Token::WORD_);

   enum_<lang_flag_e>("Language")
      .value("C", lang_flag_e::LANG_C)
      .value("CPP", lang_flag_e::LANG_CPP)
      .value("D", lang_flag_e::LANG_D)
      .value("CS", lang_flag_e::LANG_CS)
      .value("JAVA", lang_flag_e::LANG_JAVA)
      .value("OC", lang_flag_e::LANG_OC)
      .value("VALA", lang_flag_e::LANG_VALA)
      .value("PAWN", lang_flag_e::LANG_PAWN)
      .value("ECMA", lang_flag_e::LANG_ECMA);

   // endregion enum bindings

   register_vector<std::string>("strings");

   class_<GenericOption>("GenericOption")
      .function("type", &GenericOption::type)
      .function("description", select_overload<std::string(const GenericOption &)>(
                   [](const GenericOption &o)
   {
      return((o.description() != nullptr) ? string(o.description()) : "");
   }))
      .function("name", select_overload<std::string(const GenericOption &)>(
                   [](const GenericOption &o)
   {
      return((o.name() != nullptr) ? string(o.name()) : "");
   }))
      .function("possible_values", select_overload<std::vector<std::string>(const GenericOption &)>(
                   [](const GenericOption &o)
   {
      std::vector<std::string> strings;

      auto ptr = o.possibleValues();

      for (auto c = *ptr; c; c = *++ptr)
      {
         strings.push_back(std::string{ c });
      }

      return(strings);
   }))
      .function("default", &GenericOption::defaultStr)
      .function("min", &GenericOption::minStr)
      .function("max", &GenericOption::maxStr)
      .function("is_default", &GenericOption::isDefault)
      .function("reset", &GenericOption::reset)
      .function("set", select_overload<bool(GenericOption &o, const std::string &s)>(
                   [](GenericOption &o, const std::string &s)
   {
      return(o.read(s.c_str()));
   }))
      .function("value", &GenericOption::str);

   register_vector<GenericOption *>("options");

   class_<Option<iarf_e>, base<GenericOption> >("OptionIARF")
      .function("value", &Option<iarf_e>::operator());

   class_<Option<line_end_e>, base<GenericOption> >("OptionLineEnd")
      .function("value", &Option<line_end_e>::operator());

   class_<Option<token_pos_e>, base<GenericOption> >("OptionTokenPos")
      .function("value", &Option<token_pos_e>::operator());

   class_<Option<unsigned>, base<GenericOption> >("OptionUnsigned")
      .function("value", &Option<unsigned>::operator());

   class_<Option<signed>, base<GenericOption> >("OptionSigned")
      .function("value", &Option<signed>::operator());

   class_<Option<std::string>, base<GenericOption> >("OptionString")
      .function("value", &Option<std::string>::operator());

   class_<OptionGroup>("OptionGroup")
      .property("description", select_overload<std::string(const OptionGroup &)>(
                   [](const OptionGroup &g)
   {
      return(std::string(g.description));
   }))
      .property("options", &OptionGroup::options);

   register_vector<OptionGroup *>("groups");

   emscripten::function("get_options", &get_options);
   emscripten::function("get_groups", &get_groups);

   emscripten::function("_initialize", &_initialize);
   emscripten::function("destruct", &destruct);

   emscripten::function("get_version", &get_version);

   emscripten::function("add_keyword", &_add_keyword);
   emscripten::function("clear_keywords", &clear_keywords);

   emscripten::function("reset_options", &reset_options);
   emscripten::function("option_reset_value", &reset_option);
   emscripten::function("option_set_value", &set_option);
   emscripten::function("option_get_value", &get_option);

   emscripten::function("_load_config", &_loadConfig);
   emscripten::function("show_config", select_overload<string(bool, bool)>(&show_config));
   emscripten::function("show_config", select_overload<string(bool)>(&show_config));
   emscripten::function("show_config", select_overload<string()>(&show_config));

   emscripten::function("log_type_enable", &log_set_sev);
   emscripten::function("log_type_show_name", &show_log_type);
   emscripten::function("quiet", &set_quiet);

   emscripten::function("_uncrustify", select_overload<intptr_t(intptr_t, lang_flag_e, bool, bool)>(&_uncrustify));
   emscripten::function("_uncrustify", select_overload<intptr_t(intptr_t, lang_flag_e, bool)>(&_uncrustify));
   emscripten::function("_uncrustify", select_overload<intptr_t(intptr_t, lang_flag_e)>(&_uncrustify));

   emscripten::function("_debug", select_overload<intptr_t(intptr_t, lang_flag_e, bool)>(&_debug));
   emscripten::function("_debug", select_overload<intptr_t(intptr_t, lang_flag_e)>(&_debug));
};

#endif
